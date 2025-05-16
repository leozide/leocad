#include "lc_global.h"
#include "lc_traintrack.h"
#include "lc_library.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_model.h"
#include "lc_application.h"

// todo:
// when moving existing pieces:
//   need to check for parts intersecting instead of mouse intersection because it's hard to connect tracks
//   look into the snapping that happens when there are no available connections
//   fix comments in UpdateFreeMoveTool
// hide some of the 12v tracks to avoid bloat
// auto replace cross when going over a straight section
// set focus connection after adding

std::map<quint32, PieceInfo*> lcTrainTrackInfo::mSleepers;

void lcTrainTrackInfo::Initialize(lcPiecesLibrary* Library)
{
	QFile ConfigFile(QLatin1String(":/resources/traintrack.json"));

	if (!ConfigFile.open(QIODevice::ReadOnly))
		return;

	QJsonParseError Error;
	QJsonDocument Document = QJsonDocument::fromJson(ConfigFile.readAll(), &Error);

	if (Error.error != QJsonParseError::NoError)
	{
		qDebug() << Error.errorString();
	}

	QJsonObject Root = Document.object();

	if (Root["Version"].toInt() != 1)
		return;

	QJsonArray JsonPieces = Root["Pieces"].toArray();

	for (QJsonArray::const_iterator PiecesIt = JsonPieces.constBegin(); PiecesIt != JsonPieces.constEnd(); ++PiecesIt)
	{
		QJsonObject JsonPiece = PiecesIt->toObject();

		QJsonArray JsonConnections = JsonPiece["Connections"].toArray();
		std::vector<lcTrainTrackConnection> Connections;

		for (QJsonArray::const_iterator ConnectionIt = JsonConnections.constBegin(); ConnectionIt != JsonConnections.constEnd(); ++ConnectionIt)
		{
			QJsonObject JsonConnection = ConnectionIt->toObject();

			QJsonArray JsonPosition = JsonConnection["Position"].toArray();
			lcVector3 Position(JsonPosition[0].toDouble(), JsonPosition[1].toDouble(), JsonPosition[2].toDouble());

			float Rotation = JsonConnection["Rotation"].toDouble() * LC_DTOR;
			QString ConnectionGroup = JsonConnection["Type"].toString();
			lcTrainTrackConnectionSleeper ConnectionSleeper = lcTrainTrackConnectionSleeper::None;

			if (ConnectionGroup.startsWith('+'))
			{
				ConnectionSleeper = lcTrainTrackConnectionSleeper::NeedsSleeper;
				ConnectionGroup = ConnectionGroup.mid(1);
			}
			else if (ConnectionGroup.startsWith('-'))
			{
				ConnectionSleeper = lcTrainTrackConnectionSleeper::HasSleeper;
				ConnectionGroup = ConnectionGroup.mid(1);
			}

			Connections.emplace_back(lcTrainTrackConnection{ lcMatrix44(lcMatrix33RotationZ(Rotation), Position), { qHash(ConnectionGroup), ConnectionSleeper } });
		}

		int Color = JsonPiece["Color"].toInt(16);
		QJsonArray JsonIDs = JsonPiece["IDs"].toArray();

		for (QJsonArray::const_iterator IDIt = JsonIDs.constBegin(); IDIt != JsonIDs.constEnd(); ++IDIt)
		{
			PieceInfo* Info = Library->FindPiece(IDIt->toString().toLatin1(), nullptr, false, false);

			if (Info)
				Info->SetTrainTrackInfo(new lcTrainTrackInfo(Connections, Color, true));
		}

		JsonIDs = JsonPiece["HiddenIDs"].toArray();

		for (QJsonArray::const_iterator IDIt = JsonIDs.constBegin(); IDIt != JsonIDs.constEnd(); ++IDIt)
		{
			PieceInfo* Info = Library->FindPiece(IDIt->toString().toLatin1(), nullptr, false, false);

			if (Info)
				Info->SetTrainTrackInfo(new lcTrainTrackInfo(Connections, Color, false));
		}
	}

	QJsonObject JsonSleepers = Root["Sleepers"].toObject();

	mSleepers.clear();

	for (QJsonObject::const_iterator SleepersIt = JsonSleepers.constBegin(); SleepersIt != JsonSleepers.constEnd(); ++SleepersIt)
	{
		quint32 Group = qHash(SleepersIt.key());
		QString PartId = SleepersIt->toString();

		PieceInfo* SleeperInfo = Library->FindPiece(PartId.toLatin1(), nullptr, false, false);

		if (SleeperInfo)
			mSleepers[Group] = SleeperInfo;
	}
}

int lcTrainTrackInfo::GetPieceConnectionIndex(const lcPiece* Piece1, int ConnectionIndex1, const lcPiece* Piece2)
{
	const lcTrainTrackInfo* TrainTrackInfo1 = Piece1->mPieceInfo->GetTrainTrackInfo();
	const lcTrainTrackInfo* TrainTrackInfo2 = Piece2->mPieceInfo->GetTrainTrackInfo();

	const std::vector<lcTrainTrackConnection>& Connections2 = TrainTrackInfo2->GetConnections();
	lcMatrix44 Transform1 = lcMul(TrainTrackInfo1->GetConnections()[ConnectionIndex1].Transform, Piece1->mModelWorld);

	for (int ConnectionIndex2 = 0; ConnectionIndex2 < static_cast<int>(Connections2.size()); ConnectionIndex2++)
	{
		const lcTrainTrackConnection& Connection2 = Connections2[ConnectionIndex2];
		const lcMatrix44 Transform2 = lcMul(Connection2.Transform, Piece2->mModelWorld);

		if (lcLengthSquared(Transform1.GetTranslation() - Transform2.GetTranslation()) > 0.1f)
			continue;

		float Dot = lcDot3(Transform1[0], Transform2[0]);

		if (Dot < -0.99f && Dot > -1.01f)
			return ConnectionIndex2;
	}

	return -1;
}

std::vector<lcInsertPieceInfo> lcTrainTrackInfo::GetInsertPieceInfo(lcPiece* CurrentPiece, PieceInfo* Info, lcPiece* MovingPiece, int ColorIndex, quint32 PreferredSection, bool AllowNewPieces, std::optional<lcVector3> ClosestPoint)
{
	std::vector<lcInsertPieceInfo> Pieces;
	const lcTrainTrackInfo* CurrentTrackInfo = CurrentPiece->mPieceInfo->GetTrainTrackInfo();

	if (!CurrentTrackInfo || CurrentTrackInfo->GetConnections().empty() || (MovingPiece && Info != MovingPiece->mPieceInfo))
		return Pieces;

	quint32 ConnectionIndex = 0;

	if (ClosestPoint)
	{
		lcMatrix44 InverseMatrix = lcMatrix44AffineInverse(CurrentPiece->mModelWorld);
		lcVector3 LocalClosestPoint = lcMul31(ClosestPoint.value(), InverseMatrix);
		float BestDistance = FLT_MAX;

		for (quint32 CheckIndex = 0; CheckIndex < CurrentTrackInfo->GetConnections().size(); CheckIndex++)
		{
			if (CurrentPiece->IsTrainTrackConnected(CheckIndex))
				continue;

			if (MovingPiece)
			{
				const std::vector<lcTrainTrackConnection>& MovingConnections = Info->GetTrainTrackInfo()->GetConnections();
				bool CanConnect = false;

				for (quint32 MovingConnectionIndex = 0; MovingConnectionIndex < MovingConnections.size(); MovingConnectionIndex++)
				{
					if (!MovingPiece->IsTrainTrackConnected(MovingConnectionIndex) && AreConnectionsCompatible(CurrentTrackInfo->GetConnections()[CheckIndex].Type, MovingConnections[MovingConnectionIndex].Type, AllowNewPieces))
					{
						CanConnect = true;
						break;
					}
				}

				if (!CanConnect)
					continue;
			}
			else
			{
				if (!Info->GetTrainTrackInfo()->CanConnectTo(CurrentTrackInfo->GetConnections()[CheckIndex].Type, AllowNewPieces))
					continue;
			}

			float Distance = (CurrentTrackInfo->GetConnections()[CheckIndex].Transform.GetTranslation() - LocalClosestPoint).LengthSquared();

			if (Distance < BestDistance)
			{
				ConnectionIndex = CheckIndex;
				BestDistance = Distance;
			}
		}

		if (BestDistance == FLT_MAX)
			return Pieces;
	}
	else
	{
		if (PreferredSection != LC_PIECE_SECTION_INVALID && PreferredSection >= LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
			ConnectionIndex = PreferredSection - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;

		quint32 CheckIndex;

		for (CheckIndex = 0; CheckIndex < CurrentTrackInfo->GetConnections().size(); CheckIndex++)
			if (!CurrentPiece->IsTrainTrackConnected((ConnectionIndex + CheckIndex) % CurrentTrackInfo->GetConnections().size()))
				break;

		if (CheckIndex == CurrentTrackInfo->GetConnections().size())
			return Pieces;

		ConnectionIndex = (ConnectionIndex + CheckIndex) % CurrentTrackInfo->GetConnections().size();
	}

	const lcTrainTrackConnectionType& CurrentConnectionType = CurrentTrackInfo->GetConnections()[ConnectionIndex].Type;
	const std::vector<lcTrainTrackConnection>& NewConnections = Info->GetTrainTrackInfo()->GetConnections();
	quint32 NewConnectionIndex;// = ConnectionIndex ? 0 : 1;

	for (NewConnectionIndex = 0; NewConnectionIndex < NewConnections.size(); NewConnectionIndex++)
	{
		if (MovingPiece)
		{
			if (MovingPiece->IsTrainTrackConnected(NewConnectionIndex))
				continue;

			const std::vector<lcTrainTrackConnection>& MovingConnections = Info->GetTrainTrackInfo()->GetConnections();

			if (AreConnectionsCompatible(CurrentTrackInfo->GetConnections()[ConnectionIndex].Type, MovingConnections[NewConnectionIndex].Type, AllowNewPieces))
				break;
		}
		else
		{
			if (AreConnectionsCompatible(CurrentConnectionType, NewConnections[NewConnectionIndex].Type, AllowNewPieces))
				break;
		}
	}

	if (NewConnectionIndex == NewConnections.size())
		return Pieces;

	if (CurrentConnectionType.Sleeper == lcTrainTrackConnectionSleeper::NeedsSleeper && NewConnections[NewConnectionIndex].Type.Sleeper == lcTrainTrackConnectionSleeper::NeedsSleeper)
	{
		std::map<quint32, PieceInfo*>::const_iterator SleeperIt = mSleepers.find(CurrentConnectionType.Group);

		if (SleeperIt == mSleepers.end())
			return Pieces;

		PieceInfo* SleeperInfo = SleeperIt->second;

		if (!SleeperInfo)
			return Pieces;

		std::optional<lcMatrix44> SleeperTransform = GetConnectionTransform(CurrentPiece->mPieceInfo, CurrentPiece->mModelWorld, ConnectionIndex, SleeperInfo, 0);

		if (!SleeperTransform)
			return Pieces;

		std::optional<lcMatrix44> Transform = GetConnectionTransform(SleeperInfo, SleeperTransform.value(), 1, Info, NewConnectionIndex);

		if (Transform)
		{
			Pieces.emplace_back(lcInsertPieceInfo{ SleeperInfo, SleeperTransform.value(), lcGetColorIndex(SleeperInfo->GetTrainTrackInfo()->GetColorCode()) });
			Pieces.emplace_back(lcInsertPieceInfo{ Info, Transform.value(), ColorIndex });
		}
	}
	else
	{
		std::optional<lcMatrix44> Transform = GetConnectionTransform(CurrentPiece->mPieceInfo, CurrentPiece->mModelWorld, ConnectionIndex, Info, NewConnectionIndex);

		if (Transform)
			Pieces.emplace_back(lcInsertPieceInfo{ Info, Transform.value(), ColorIndex });
	}

	return Pieces;
}

std::optional<lcMatrix44> lcTrainTrackInfo::GetConnectionTransform(PieceInfo* CurrentInfo, const lcMatrix44& CurrentTransform, quint32 CurrentConnectionIndex, PieceInfo* Info, quint32 NewConnectionIndex)
{
	if (!CurrentInfo || !Info)
		return std::nullopt;

	const lcTrainTrackInfo* CurrentTrackInfo = CurrentInfo->GetTrainTrackInfo();

	if (!CurrentTrackInfo || CurrentTrackInfo->GetConnections().empty())
		return std::nullopt;

	if (CurrentConnectionIndex >= CurrentTrackInfo->GetConnections().size())
		return std::nullopt;

	lcTrainTrackInfo* NewTrackInfo = Info->GetTrainTrackInfo();

	if (!NewTrackInfo || NewConnectionIndex >= NewTrackInfo->mConnections.size())
		return std::nullopt;

	lcMatrix44 Transform;

//	if (TrainTrackType != lcTrainTrackType::Left)
//		Transform = NewTrackInfo->mConnections[NewConnectionIndex].Transform;
//	else
//	{
		Transform = lcMatrix44AffineInverse(NewTrackInfo->mConnections[NewConnectionIndex].Transform);
		Transform = lcMul(Transform, lcMatrix44RotationZ(LC_PI));
//	}

	Transform = lcMul(Transform, CurrentTrackInfo->GetConnections()[CurrentConnectionIndex].Transform);
	Transform = lcMul(Transform, CurrentTransform);

	return Transform;
}

std::optional<lcMatrix44> lcTrainTrackInfo::CalculateTransformToConnection(const lcMatrix44& ConnectionTransform, PieceInfo* Info, quint32 ConnectionIndex)
{
	lcTrainTrackInfo* TrackInfo = Info->GetTrainTrackInfo();

	if (!TrackInfo || ConnectionIndex >= TrackInfo->mConnections.size())
		return std::nullopt;

	lcMatrix44 Transform;

//	if (TrainTrackType != lcTrainTrackType::Left)
//		Transform = NewTrackInfo->mConnections[NewConnectionIndex].Transform;
//	else
//	{
		Transform = lcMatrix44AffineInverse(TrackInfo->mConnections[ConnectionIndex].Transform);
//		Transform = lcMul(Transform, lcMatrix44RotationZ(LC_PI));
//	}

	Transform = lcMul(Transform, ConnectionTransform);

//	Transform = lcMul(Transform, CurrentTrackInfo->GetConnections()[CurrentConnectionIndex].Transform);
//	Transform = lcMul(Transform, CurrentPiece->mModelWorld);

	return Transform;
}

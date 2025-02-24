#include "lc_global.h"
#include "lc_traintrack.h"
#include "lc_library.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_application.h"

// todo:
// add the rest of the 9v and 12v tracks, look into 4.5v
// see if we should remove some of the 12v tracks to avoid bloat
// lcView::GetPieceInsertTransform should use track connections when dragging a new track over an existing one
// better insert gizmo mouse detection
// auto replace cross when going over a straight section
// set focus connection after adding

std::map<quint32, PieceInfo*> lcTrainTrackInfo::mSleepers;

void lcTrainTrackInfo::Initialize(lcPiecesLibrary* Library)
{
	QFile ConfigFile(QLatin1String(":/resources/traintrack.json"));

	if (!ConfigFile.open(QIODevice::ReadOnly))
		return;

	QJsonDocument Document = QJsonDocument::fromJson(ConfigFile.readAll());
	QJsonObject Root = Document.object();

	if (Root["Version"].toInt() != 1)
		return;

	QJsonObject JsonPieces = Root["Pieces"].toObject();

	for (QJsonObject::const_iterator PiecesIt = JsonPieces.constBegin(); PiecesIt != JsonPieces.constEnd(); ++PiecesIt)
	{
		QJsonObject JsonPiece = PiecesIt->toObject();
		QJsonArray JsonParts = JsonPiece["Parts"].toArray();

		for (QJsonArray::const_iterator PartsIt = JsonParts.constBegin(); PartsIt != JsonParts.constEnd(); ++PartsIt)
		{
			PieceInfo* Info = Library->FindPiece(PartsIt->toString().toLatin1(), nullptr, false, false);

			if (!Info)
				continue;

			lcTrainTrackInfo* TrainTrackInfo = new lcTrainTrackInfo();

			Info->SetTrainTrackInfo(TrainTrackInfo);

			QJsonArray JsonConnections = JsonPiece["Connections"].toArray();

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

				TrainTrackInfo->AddConnection(lcMatrix44(lcMatrix33RotationZ(Rotation), Position), { qHash(ConnectionGroup), ConnectionSleeper } );
			}
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

std::vector<lcTrainTrackInsert> lcTrainTrackInfo::GetPieceInsertTransforms(lcPiece* CurrentPiece, PieceInfo* Info, quint32 Section)
{
	std::vector<lcTrainTrackInsert> Pieces;
	const lcTrainTrackInfo* CurrentTrackInfo = CurrentPiece->mPieceInfo->GetTrainTrackInfo();

	if (!CurrentTrackInfo || CurrentTrackInfo->GetConnections().empty())
		return Pieces;

	quint32 ConnectionIndex = 0;

	if (Section != LC_PIECE_SECTION_INVALID && Section >= LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
	{
		ConnectionIndex = Section - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
	}
	else
	{
		for (ConnectionIndex = 0; ConnectionIndex < CurrentTrackInfo->GetConnections().size(); ConnectionIndex++)
			if (!CurrentPiece->IsTrainTrackConnected(ConnectionIndex))
				break;

		if (ConnectionIndex == CurrentTrackInfo->GetConnections().size())
			return Pieces;
	}

	const lcTrainTrackConnectionType& CurrentConnectionType = CurrentTrackInfo->GetConnections()[ConnectionIndex].Type;
	const std::vector<lcTrainTrackConnection>& NewConnections = Info->GetTrainTrackInfo()->GetConnections();
	quint32 NewConnectionIndex;// = ConnectionIndex ? 0 : 1;

	for (NewConnectionIndex = 0; NewConnectionIndex < NewConnections.size(); NewConnectionIndex++)
		if (AreConnectionsCompatible(CurrentConnectionType, NewConnections[NewConnectionIndex].Type))
			break;

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
			Pieces.emplace_back(lcTrainTrackInsert{ SleeperInfo, SleeperTransform.value(), 8 });
			Pieces.emplace_back(lcTrainTrackInsert{ Info, Transform.value(), 16 });
		}
	}
	else
	{
		std::optional<lcMatrix44> Transform = GetConnectionTransform(CurrentPiece->mPieceInfo, CurrentPiece->mModelWorld, ConnectionIndex, Info, NewConnectionIndex);

		if (Transform)
			Pieces.emplace_back(lcTrainTrackInsert{ Info, Transform.value(), 16 });
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

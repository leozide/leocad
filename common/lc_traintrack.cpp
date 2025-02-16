#include "lc_global.h"
#include "lc_traintrack.h"
#include "lc_library.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_application.h"

// todo:
// auto replace cross when going over a straight section
// add other track types
// set focus connection after adding

void lcTrainTrackInit(lcPiecesLibrary* Library)
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
		PieceInfo* Info = Library->FindPiece(PiecesIt.key().toLatin1(), nullptr, false, false);

		if (!Info)
			continue;

		lcTrainTrackInfo* TrainTrackInfo = new lcTrainTrackInfo();

		Info->SetTrainTrackInfo(TrainTrackInfo);

		QJsonObject JsonPiece = PiecesIt->toObject();
		QJsonArray JsonConnections = JsonPiece["Connections"].toArray();

		for (QJsonArray::const_iterator ConnectionIt = JsonConnections.constBegin(); ConnectionIt != JsonConnections.constEnd(); ++ConnectionIt)
		{
			QJsonObject JsonConnection = ConnectionIt->toObject();

			QJsonArray JsonPosition = JsonConnection["Position"].toArray();
			lcVector3 Position(JsonPosition[0].toDouble(), JsonPosition[1].toDouble(), JsonPosition[2].toDouble());

			float Rotation = JsonConnection["Rotation"].toDouble() * LC_DTOR;
			quint32 Type = qHash(JsonConnection["Type"].toString());

			TrainTrackInfo->AddConnection(lcMatrix44(lcMatrix33RotationZ(Rotation), Position), Type);
		}
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

std::optional<lcMatrix44> lcTrainTrackInfo::GetPieceInsertTransform(lcPiece* CurrentPiece, PieceInfo* Info, quint32 Section)
{
	const lcTrainTrackInfo* CurrentTrackInfo = CurrentPiece->mPieceInfo->GetTrainTrackInfo();

	if (!CurrentTrackInfo || CurrentTrackInfo->GetConnections().empty())
		return std::nullopt;

	const quint32 FocusSection = CurrentPiece->GetFocusSection();
	quint32 ConnectionIndex = 0;

	if (FocusSection != LC_PIECE_SECTION_INVALID && FocusSection >= LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
	{
		ConnectionIndex = FocusSection - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
	}
	else if (Section != LC_PIECE_SECTION_INVALID && Section >= LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
	{
		ConnectionIndex = Section - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
	}
	else
	{
		for (ConnectionIndex = 0; ConnectionIndex < CurrentTrackInfo->GetConnections().size(); ConnectionIndex++)
			if (!CurrentPiece->IsTrainTrackConnected(ConnectionIndex))
				break;

		if (ConnectionIndex == CurrentTrackInfo->GetConnections().size())
			return std::nullopt;
	}

	return GetConnectionTransform(CurrentPiece, ConnectionIndex, Info, ConnectionIndex ? 0 : 1);
}

std::optional<lcMatrix44> lcTrainTrackInfo::GetConnectionTransform(lcPiece* CurrentPiece, quint32 CurrentConnectionIndex, PieceInfo* Info, quint32 NewConnectionIndex)
{
	if (!CurrentPiece || !Info)
		return std::nullopt;

	const lcTrainTrackInfo* CurrentTrackInfo = CurrentPiece->mPieceInfo->GetTrainTrackInfo();

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
	Transform = lcMul(Transform, CurrentPiece->mModelWorld);

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

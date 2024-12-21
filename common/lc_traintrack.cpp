#include "lc_global.h"
#include "lc_traintrack.h"
#include "lc_library.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_application.h"

// todo:
// auto replace cross when going over a straight section
// redo gizmo
// add cross to gizmo
// move config to json
// add other track types
// set focus connection after adding

void lcTrainTrackInit(lcPiecesLibrary* Library)
{
	PieceInfo* Info = Library->FindPiece("74746.dat", nullptr, false, false);

	if (Info)
	{
		lcTrainTrackInfo* TrainTrackInfo = new lcTrainTrackInfo();

		TrainTrackInfo->AddConnection({lcMatrix44Translation(lcVector3(160.0f, 0.0f, 0.0f))});
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-160.0f, 0.0f, 0.0f))});

		Info->SetTrainTrackInfo(TrainTrackInfo);
	}

	Info = Library->FindPiece("74747.dat", nullptr, false, false);

	if (Info)
	{
		lcTrainTrackInfo* TrainTrackInfo = new lcTrainTrackInfo();

		const float CurveX = sinf(LC_DTOR * 11.25f) * 800.0f;
		const float CurveY = (cosf(LC_DTOR * 11.25f) * 800.0f) - 800.0f;

		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(-11.25f * LC_DTOR), lcVector3(CurveX, CurveY, 0.0f))});
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(-168.75f * LC_DTOR), lcVector3(-CurveX, CurveY, 0.0f))});

		Info->SetTrainTrackInfo(TrainTrackInfo);
	}

	const float BranchX = 320.0f + 320.0f + (-(sinf(LC_DTOR * 22.5f) * 800.0f));
	const float BranchY = 320.0f + ((cosf(LC_DTOR * 22.5f) * 800.0f) - 800.0f);

	Info = Library->FindPiece("2861c04.dat", nullptr, false, false);

	if (Info)
	{
		lcTrainTrackInfo* TrainTrackInfo = new lcTrainTrackInfo();

		TrainTrackInfo->AddConnection({lcMatrix44Translation(lcVector3(320.0f, 0.0f, 0.0f))});
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-320.0f, 0.0f, 0.0f))});
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(22.5f * LC_DTOR), lcVector3(BranchX, BranchY, 0.0f))});	

		Info->SetTrainTrackInfo(TrainTrackInfo);
	}

	Info = Library->FindPiece("2859c04.dat", nullptr, false, false);

	if (Info)
	{
		lcTrainTrackInfo* TrainTrackInfo = new lcTrainTrackInfo();

		TrainTrackInfo->AddConnection({lcMatrix44Translation(lcVector3(320.0f, 0.0f, 0.0f))});
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-320.0f, 0.0f, 0.0f))});
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(-22.5f * LC_DTOR), lcVector3(BranchX, -BranchY, 0.0f))});	

		Info->SetTrainTrackInfo(TrainTrackInfo);
	}

    Info = Library->FindPiece("32087.dat", nullptr, false, false);

	if (Info)
	{
		lcTrainTrackInfo* TrainTrackInfo = new lcTrainTrackInfo();

		TrainTrackInfo->AddConnection({lcMatrix44Translation(lcVector3(160.0f, 0.0f, 0.0f))});
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(LC_PI / 2.0f), lcVector3(0.0f, 160.0f, 0.0f))});
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-160.0f, 0.0f, 0.0f))});
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(-LC_PI / 2.0f), lcVector3(0.0f, -160.0f, 0.0f))});

		Info->SetTrainTrackInfo(TrainTrackInfo);
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

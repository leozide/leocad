#include "lc_global.h"
#include "lc_traintrack.h"
#include "lc_library.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_application.h"

// todo:
// add cross 32087.dat and auto replace when going over a straight section
// detect existing connections
// new gizmo mesh
// move config to json
// add other track types

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
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(22.5f * LC_DTOR), lcVector3(BranchX, BranchY, 0.0f))});	
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-320.0f, 0.0f, 0.0f))});

		Info->SetTrainTrackInfo(TrainTrackInfo);
	}

	Info = Library->FindPiece("2859c04.dat", nullptr, false, false);

	if (Info)
	{
		lcTrainTrackInfo* TrainTrackInfo = new lcTrainTrackInfo();

		TrainTrackInfo->AddConnection({lcMatrix44Translation(lcVector3(320.0f, 0.0f, 0.0f))});
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(-22.5f * LC_DTOR), lcVector3(BranchX, -BranchY, 0.0f))});	
		TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-320.0f, 0.0f, 0.0f))});

		Info->SetTrainTrackInfo(TrainTrackInfo);
	}
}

std::pair<PieceInfo*, lcMatrix44> lcTrainTrackInfo::GetPieceInsertTransform(lcPiece* Piece, quint32 ConnectionIndex, lcTrainTrackType TrainTrackType) const
{
	if (ConnectionIndex >= mConnections.size())
		return { nullptr, lcMatrix44Identity() };

	const char* PieceNames[] =
	{
		"74746.dat",
		"74747.dat",
		"74747.dat",
		"2861c04.dat",
		"2859c04.dat"
	};

	PieceInfo* Info = lcGetPiecesLibrary()->FindPiece(PieceNames[static_cast<int>(TrainTrackType)], nullptr, false, false);

	if (!Info)
		return { nullptr, lcMatrix44Identity() };

	lcTrainTrackInfo* TrainTrackInfo = Info->GetTrainTrackInfo();

	if (!TrainTrackInfo || TrainTrackInfo->mConnections.empty())
		return { nullptr, lcMatrix44Identity() };

	lcMatrix44 Transform;

	if (TrainTrackType != lcTrainTrackType::Left)
		Transform = TrainTrackInfo->mConnections[0].Transform;
	else
	{
		Transform = lcMatrix44AffineInverse(TrainTrackInfo->mConnections[0].Transform);
		Transform = lcMul(Transform, lcMatrix44RotationZ(LC_PI));
	}

	Transform = lcMul(Transform, mConnections[ConnectionIndex].Transform);
	Transform = lcMul(Transform, Piece->mModelWorld);

	return { Info, Transform };
}

bool lcTrainTrackInfo::ArePiecesConnected(const lcPiece* Piece1, int ConnectionIndex1, const lcPiece* Piece2)
{
	const lcTrainTrackInfo* TrainTrackInfo1 = Piece1->mPieceInfo->GetTrainTrackInfo();
	const lcTrainTrackInfo* TrainTrackInfo2 = Piece2->mPieceInfo->GetTrainTrackInfo();

	lcMatrix44 Transform1 = lcMul(TrainTrackInfo1->GetConnections()[ConnectionIndex1].Transform, Piece1->mModelWorld);

	for (const lcTrainTrackConnection& Connection2 : TrainTrackInfo2->GetConnections())
	{
		lcMatrix44 Transform2 = lcMul(Connection2.Transform, Piece2->mModelWorld);

		if (lcLengthSquared(Transform1.GetTranslation() - Transform2.GetTranslation()) > 0.1f)
			continue;

		float Dot = lcDot3(Transform1[0], Transform2[0]);

		if (Dot < -0.99f && Dot > -1.01f)
			return true;
	}

	return false;
}

std::optional<lcMatrix44> lcTrainTrackInfo::GetPieceInsertTransform(lcPiece* CurrentPiece, PieceInfo* Info)
{
	if (!CurrentPiece || !Info)
		return std::nullopt;

	const lcTrainTrackInfo* CurrentTrackInfo = CurrentPiece->mPieceInfo->GetTrainTrackInfo();

	if (!CurrentTrackInfo || CurrentTrackInfo->GetConnections().empty())
		return std::nullopt;

	const quint32 FocusSection = CurrentPiece->GetFocusSection();
	quint32 ConnectionIndex = 0;

	if (FocusSection == LC_PIECE_SECTION_POSITION || FocusSection == LC_PIECE_SECTION_INVALID)
	{
		for (ConnectionIndex = 0; ConnectionIndex < CurrentTrackInfo->GetConnections().size(); ConnectionIndex++)
			if (CurrentPiece->IsTrainTrackConnectionVisible(ConnectionIndex))
				break;
	}
	else
	{
		if (FocusSection < LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
			return std::nullopt;

		ConnectionIndex = FocusSection - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
	}

	if (ConnectionIndex >= CurrentTrackInfo->GetConnections().size())
		return std::nullopt;

	lcTrainTrackInfo* NewTrackInfo = Info->GetTrainTrackInfo();

	if (!NewTrackInfo || NewTrackInfo->mConnections.empty())
		return std::nullopt;

	lcMatrix44 Transform;

//	if (TrainTrackType != lcTrainTrackType::Left)
		Transform = NewTrackInfo->mConnections[0].Transform;
//	else
//	{
//		Transform = lcMatrix44AffineInverse(TrainTrackInfo->mConnections[0].Transform);
//		Transform = lcMul(Transform, lcMatrix44RotationZ(LC_PI));
//	}

	Transform = lcMul(Transform, CurrentTrackInfo->GetConnections()[ConnectionIndex].Transform);
	Transform = lcMul(Transform, CurrentPiece->mModelWorld);

	return Transform;
}

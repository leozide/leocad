#include "lc_global.h"
#include "lc_traintrack.h"
#include "lc_library.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_application.h"

// todo:
// add cross 32087.dat and auto replace when going over a straight section
// draw icon on thumbnails
// detect existing connections
// new gizmo mesh
// move config to json
// add other track types

std::pair<PieceInfo*, lcMatrix44> lcTrainTrackInfo::GetPieceInsertPosition(lcPiece* Piece, quint32 ConnectionIndex, lcTrainTrackType TrainTrackType) const
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

#pragma once

#include "lc_context.h"

class lcViewManipulator
{
public:
	lcViewManipulator(lcView* View);

	void DrawSelectMove(lcTrackButton TrackButton, lcTrackTool TrackTool, quint32 TrackToolSection);
	void DrawRotate(lcTrackButton TrackButton, lcTrackTool TrackTool);

	std::pair<lcTrackTool, quint32> UpdateSelectMove();
	lcTrackTool UpdateRotate();

	static void CreateResources(lcContext* Context);
	static void DestroyResources(lcContext* Context);

protected:
	void DrawTrainTrack(lcPiece* Piece, lcContext* Context, lcTrackTool TrackTool, quint32 TrackToolSection);
	std::tuple<lcTrackTool, quint32, float> UpdateSelectMoveTrainTrack(lcPiece* Piece, const lcVector3& OverlayCenter, float OverlayScale, const lcVector3& Start, const lcVector3& End, const lcVector3 (&PlaneNormals)[3]) const;

	static bool IsTrackToolAllowed(lcTrackTool TrackTool, quint32 AllowedTransforms);

	lcView* mView = nullptr;

	static lcVertexBuffer mRotateMoveVertexBuffer;
	static lcIndexBuffer mRotateMoveIndexBuffer;

	static constexpr int mMoveIndexStart = 0;
	static constexpr int mMoveIndexCount = 36;
	static constexpr int mRotateIndexStart = mMoveIndexStart + mMoveIndexCount * 3;
	static constexpr int mRotateIndexCount = 120;
	static constexpr int mMovePlaneIndexStart = mRotateIndexStart + mRotateIndexCount * 3;
	static constexpr int mMovePlaneIndexCount = 4;
	static constexpr int mTrainTrackRotateIndexStart = mMovePlaneIndexStart + mMovePlaneIndexCount * 3;
	static constexpr int mTrainTrackRotateIndexCount = 96;
	static constexpr int mTrainTrackInsertIndexStart = mTrainTrackRotateIndexStart + mTrainTrackRotateIndexCount * 2;
	static constexpr int mTrainTrackInsertIndexCount = 72;
	static constexpr int mIndexCount = mTrainTrackInsertIndexStart + mTrainTrackInsertIndexCount;
	static const GLushort mRotateMoveIndices[mIndexCount];
	static lcVector3 mRotateMoveVertices[51 + 138 + 10 + 74 + 28];
};

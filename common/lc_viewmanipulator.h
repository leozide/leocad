#pragma once

#include "lc_context.h"

class lcViewManipulator
{
public:
	lcViewManipulator(lcView* View);

	void DrawSelectMove(lcTrackButton TrackButton, lcTrackTool TrackTool);
	void DrawRotate(lcTrackButton TrackButton, lcTrackTool TrackTool);

	lcTrackTool UpdateSelectMove();
	lcTrackTool UpdateRotate();

	static void CreateResources(lcContext* Context);
	static void DestroyResources(lcContext* Context);

protected:
	static bool IsTrackToolAllowed(lcTrackTool TrackTool, quint32 AllowedTransforms);

	lcView* mView = nullptr;

	static lcVertexBuffer mRotateMoveVertexBuffer;
	static lcIndexBuffer mRotateMoveIndexBuffer;
};

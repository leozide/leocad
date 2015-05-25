#include "lc_global.h"
#include <stdlib.h>
#include "lc_mainwindow.h"
#include "project.h"
#include "camera.h"
#include "view.h"
#include "system.h"
#include "tr.h"
#include "lc_mesh.h"
#include "texfont.h"
#include "lc_texture.h"
#include "preview.h"
#include "piece.h"
#include "pieceinf.h"

lcVertexBuffer View::mRotateMoveVertexBuffer;
lcIndexBuffer View::mRotateMoveIndexBuffer;

View::View(lcModel* Model)
{
	mModel = Model;
	mCamera = NULL;
	memset(mGridSettings, 0, sizeof(mGridSettings));

	mDragState = LC_DRAGSTATE_NONE;
	mTrackButton = LC_TRACKBUTTON_NONE;
	mTrackTool = LC_TRACKTOOL_NONE;

	View* ActiveView = gMainWindow->GetActiveView();
	if (ActiveView)
		SetCamera(ActiveView->mCamera, false);
	else
		SetDefaultCamera();
}

View::~View()
{
	mContext->DestroyVertexBuffer(mGridBuffer);

	if (gMainWindow)
		gMainWindow->RemoveView(this);

	foreach (lcCamera* Camera, mCameras)
		if (Camera && Camera->IsSimple())
			delete Camera;
}

void View::CreateResources(lcContext* Context)
{
	gGridTexture = new lcTexture;
	gGridTexture->CreateGridTexture();

	CreateSelectMoveOverlayMesh(Context);
}

void View::CreateSelectMoveOverlayMesh(lcContext* Context)
{
	float Verts[(51 + 138 + 10) * 3];
	float* CurVert = Verts;

	const float OverlayMovePlaneSize = 0.5f;
	const float OverlayMoveArrowSize = 1.5f;
	const float OverlayMoveArrowCapSize = 0.9f;
	const float OverlayMoveArrowCapRadius = 0.1f;
	const float OverlayMoveArrowBodySize = 1.2f;
	const float OverlayMoveArrowBodyRadius = 0.05f;
	const float OverlayRotateArrowStart = 1.0f;
	const float OverlayRotateArrowEnd = 1.5f;
	const float OverlayRotateArrowCenter = 1.2f;

	*CurVert++ = OverlayMoveArrowSize; *CurVert++ = 0.0f; *CurVert++ = 0.0f;

	for (int EdgeIdx = 0; EdgeIdx < 8; EdgeIdx++)
	{
		*CurVert++ = OverlayMoveArrowCapSize;
		*CurVert++ = cosf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius;
		*CurVert++ = sinf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius;
	}

	*CurVert++ = 0.0f; *CurVert++ = -OverlayMoveArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = OverlayMoveArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = -OverlayMoveArrowBodyRadius;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = OverlayMoveArrowBodyRadius;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = -OverlayMoveArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = OverlayMoveArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = 0.0f; *CurVert++ = -OverlayMoveArrowBodyRadius;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = 0.0f; *CurVert++ = OverlayMoveArrowBodyRadius;

	for (int VertIdx = 0; VertIdx < 17; VertIdx++)
	{
		*CurVert = *(CurVert - 50); CurVert++;
		*CurVert = *(CurVert - 52); CurVert++;
		*CurVert = *(CurVert - 51); CurVert++;
	}

	for (int VertIdx = 0; VertIdx < 17; VertIdx++)
	{
		*CurVert = *(CurVert - 100); CurVert++;
		*CurVert = *(CurVert - 102); CurVert++;
		*CurVert = *(CurVert - 104); CurVert++;
	}

	*CurVert++ = 0.0f; *CurVert++ = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius; *CurVert++ = OverlayRotateArrowStart;

	for (int EdgeIdx = 0; EdgeIdx < 8; EdgeIdx++)
	{
		*CurVert++ = cosf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius;
		*CurVert++ = sinf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius + OverlayRotateArrowEnd - OverlayMoveArrowCapRadius;
		*CurVert++ = OverlayRotateArrowCenter;
	}

	*CurVert++ = 0.0f; *CurVert++ = OverlayRotateArrowStart; *CurVert++ = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius;

	for (int EdgeIdx = 0; EdgeIdx < 8; EdgeIdx++)
	{
		*CurVert++ = cosf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius;
		*CurVert++ = OverlayRotateArrowCenter;
		*CurVert++ = sinf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius + OverlayRotateArrowEnd - OverlayMoveArrowCapRadius;
	}

	for (int EdgeIdx = 0; EdgeIdx < 7; EdgeIdx++)
	{
		const float Radius1 = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter - OverlayMoveArrowBodyRadius;
		const float Radius2 = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter + OverlayMoveArrowBodyRadius;
		float x = cosf(LC_2PI / 4 * EdgeIdx / 6);
		float y = sinf(LC_2PI / 4 * EdgeIdx / 6);

		*CurVert++ = 0.0f;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius1;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius1;
		*CurVert++ = 0.0f;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius2;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius2;
	}

	for (int EdgeIdx = 0; EdgeIdx < 7; EdgeIdx++)
	{
		const float Radius = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter;
		float x = cosf(LC_2PI / 4 * EdgeIdx / 6);
		float y = sinf(LC_2PI / 4 * EdgeIdx / 6);

		*CurVert++ = -OverlayMoveArrowBodyRadius;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius;
		*CurVert++ = OverlayMoveArrowBodyRadius;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius;
	}

	for (int VertIdx = 0; VertIdx < 46; VertIdx++)
	{
		*CurVert = *(CurVert - 137); CurVert++;
		*CurVert = *(CurVert - 139); CurVert++;
		*CurVert = *(CurVert - 138); CurVert++;
	}

	for (int VertIdx = 0; VertIdx < 46; VertIdx++)
	{
		*CurVert = *(CurVert - 274); CurVert++;
		*CurVert = *(CurVert - 276); CurVert++;
		*CurVert = *(CurVert - 278); CurVert++;
	}

	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize; *CurVert++ = OverlayMovePlaneSize;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize;
	*CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize;
	*CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMovePlaneSize; *CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f; *CurVert++ = 0.0f;

	const GLushort Indices[108 + 360 + 12] =
	{
		0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7, 0, 7, 8, 0, 8, 1,
		9, 10, 14, 14, 13, 9, 11, 12, 15, 15, 16, 12,
		17, 18, 19, 17, 19, 20, 17, 20, 21, 17, 21, 22, 17, 22, 23, 17, 23, 24, 17, 24, 25, 17, 25, 18,
		26, 27, 31, 31, 30, 26, 28, 29, 32, 32, 33, 29,
		34, 35, 36, 34, 36, 37, 34, 37, 38, 34, 38, 39, 34, 39, 40, 34, 40, 41, 34, 41, 42, 34, 42, 35,
		43, 44, 48, 48, 47, 43, 45, 46, 49, 49, 50, 46,
		51, 52, 53, 51, 53, 54, 51, 54, 55, 51, 55, 56, 51, 56, 57, 51, 57, 58, 51, 58, 59, 51, 59, 52,
		60, 61, 62, 60, 62, 63, 60, 63, 64, 60, 64, 65, 60, 65, 66, 60, 66, 67, 60, 67, 68, 60, 68, 61,
		69, 70, 71, 71, 72, 70, 71, 72, 73, 73, 74, 72, 73, 74, 75, 75, 76, 74, 75, 76, 77, 77, 78, 76, 77, 78, 79, 79, 80, 78, 79, 80, 81, 81, 82, 80,
		83, 84, 85, 85, 86, 84, 85, 86, 87, 87, 88, 86, 87, 88, 89, 89, 90, 88, 89, 90, 91, 91, 92, 90, 91, 92, 93, 93, 94, 92, 93, 94, 95, 95, 96, 94,
		97, 98, 99, 97, 99, 100, 97, 100, 101, 97, 101, 102, 97, 102, 103, 97, 103, 104, 97, 104, 105, 97, 105, 98,
		106, 107, 108, 106, 108, 109, 106, 109, 110, 106, 110, 111, 106, 111, 112, 106, 112, 113, 106, 113, 114, 106, 114, 107,
		115, 116, 117, 117, 118, 116, 117, 118, 119, 119, 120, 118, 119, 120, 121, 121, 122, 120, 121, 122, 123, 123, 124, 122, 123, 124, 125, 125, 126, 124, 125, 126, 127, 127, 128, 126,
		129, 130, 131, 131, 132, 130, 131, 132, 133, 133, 134, 132, 133, 134, 135, 135, 136, 134, 135, 136, 137, 137, 138, 136, 137, 138, 139, 139, 140, 138, 139, 140, 141, 141, 142, 140,
		143, 144, 145, 143, 145, 146, 143, 146, 147, 143, 147, 148, 143, 148, 149, 143, 149, 150, 143, 150, 151, 143, 151, 144,
		152, 153, 154, 152, 154, 155, 152, 155, 156, 152, 156, 157, 152, 157, 158, 152, 158, 159, 152, 159, 160, 152, 160, 153,
		161, 162, 163, 163, 164, 162, 163, 164, 165, 165, 166, 164, 165, 166, 167, 167, 168, 166, 167, 168, 169, 169, 170, 168, 169, 170, 171, 171, 172, 170, 171, 172, 173, 173, 174, 172,
		175, 176, 177, 177, 178, 176, 177, 178, 179, 179, 180, 178, 179, 180, 181, 181, 182, 180, 181, 182, 183, 183, 184, 182, 183, 184, 185, 185, 186, 184, 185, 186, 187, 187, 188, 186,
		189, 190, 191, 192, 189, 193, 194, 195, 189, 196, 197, 198
	};

	mRotateMoveVertexBuffer = Context->CreateVertexBuffer(sizeof(Verts), Verts);
	mRotateMoveIndexBuffer = Context->CreateIndexBuffer(sizeof(Indices), Indices);
}

void View::DestroyResources(lcContext* Context)
{
	delete gGridTexture;
	gGridTexture = NULL;

	Context->DestroyVertexBuffer(mRotateMoveVertexBuffer);
	Context->DestroyIndexBuffer(mRotateMoveIndexBuffer);
}

void View::SetModel(lcModel* Model)
{
	if (mModel == Model)
		return;

	mModel = Model;

	lcCamera* Camera = mCameras.value(mModel);

	if (Camera)
		mCamera = Camera;
	else
	{
		Camera = mCamera;
		mCamera = new lcCamera(true);

		if (Camera)
			mCamera->CopyPosition(Camera);
		else
			mCamera->SetViewpoint(LC_VIEWPOINT_HOME);
	}

	mCameras[mModel] = mCamera;
}

void View::ClearCameras()
{
	mCamera = NULL;
	mCameras.clear();
}

void View::RemoveCamera()
{
	if (mCamera && mCamera->IsSimple())
		return;

	lcCamera* Camera = mCamera;
	mCamera = new lcCamera(true);

	if (Camera)
		mCamera->CopyPosition(Camera);
	else
		mCamera->SetViewpoint(LC_VIEWPOINT_HOME);

	mCameras[mModel] = mCamera;

	gMainWindow->UpdateCurrentCamera(-1);
	Redraw();
}

void View::SetCamera(lcCamera* Camera, bool ForceCopy)
{
	if (Camera->IsSimple() || ForceCopy)
	{
		if (!mCamera || !mCamera->IsSimple())
			mCamera = new lcCamera(true);

		mCamera->CopyPosition(Camera);
	}
	else
	{
		if (mCamera && mCamera->IsSimple())
			delete mCamera;

		mCamera = Camera;
	}

	mCameras[mModel] = mCamera;
}

void View::SetCameraIndex(int Index)
{
	const lcArray<lcCamera*>& Cameras = mModel->GetCameras();

	if (Index >= Cameras.GetSize())
		return;

	lcCamera* Camera = Cameras[Index];
	SetCamera(Camera, false);

	gMainWindow->UpdateCurrentCamera(Index);
	Redraw();
}

void View::SetViewpoint(lcViewpoint Viewpoint)
{
	if (!mCamera || !mCamera->IsSimple())
	{
		mCamera = new lcCamera(true);
		mCameras[mModel] = mCamera;
	}

	mCamera->SetViewpoint(Viewpoint);
	ZoomExtents();
	Redraw();
}

void View::SetDefaultCamera()
{
	if (!mCamera || !mCamera->IsSimple())
	{
		mCamera = new lcCamera(true);
		mCameras[mModel] = mCamera;
	}

	mCamera->SetViewpoint(LC_VIEWPOINT_HOME);
}

lcMatrix44 View::GetProjectionMatrix() const
{
	float AspectRatio = (float)mWidth / (float)mHeight;

	if (mCamera->m_pTR)
		mCamera->m_pTR->BeginTile();

	if (mCamera->IsOrtho())
	{
		float OrthoHeight = mCamera->GetOrthoHeight() / 2.0f;
		float OrthoWidth = OrthoHeight * AspectRatio;

		return lcMatrix44Ortho(-OrthoWidth, OrthoWidth, -OrthoHeight, OrthoHeight, mCamera->m_zNear, mCamera->m_zFar * 4);
	}
	else
		return lcMatrix44Perspective(mCamera->m_fovy, AspectRatio, mCamera->m_zNear, mCamera->m_zFar);
}

LC_CURSOR_TYPE View::GetCursor() const
{
	if (mTrackTool == LC_TRACKTOOL_SELECT && mInputState.Control)
		return LC_CURSOR_SELECT_GROUP;

	const LC_CURSOR_TYPE CursorFromTrackTool[] =
	{
		LC_CURSOR_SELECT,      // LC_TRACKTOOL_NONE
		LC_CURSOR_BRICK,       // LC_TRACKTOOL_INSERT
		LC_CURSOR_LIGHT,       // LC_TRACKTOOL_POINTLIGHT
		LC_CURSOR_SPOTLIGHT,   // LC_TRACKTOOL_SPOTLIGHT
		LC_CURSOR_CAMERA,      // LC_TRACKTOOL_CAMERA
		LC_CURSOR_SELECT,      // LC_TRACKTOOL_SELECT
		LC_CURSOR_MOVE,        // LC_TRACKTOOL_MOVE_X
		LC_CURSOR_MOVE,        // LC_TRACKTOOL_MOVE_Y
		LC_CURSOR_MOVE,        // LC_TRACKTOOL_MOVE_Z
		LC_CURSOR_MOVE,        // LC_TRACKTOOL_MOVE_XY
		LC_CURSOR_MOVE,        // LC_TRACKTOOL_MOVE_XZ
		LC_CURSOR_MOVE,        // LC_TRACKTOOL_MOVE_YZ
		LC_CURSOR_MOVE,        // LC_TRACKTOOL_MOVE_XYZ
		LC_CURSOR_ROTATE,      // LC_TRACKTOOL_ROTATE_X
		LC_CURSOR_ROTATE,      // LC_TRACKTOOL_ROTATE_Y
		LC_CURSOR_ROTATE,      // LC_TRACKTOOL_ROTATE_Z
		LC_CURSOR_ROTATE,      // LC_TRACKTOOL_ROTATE_XY
		LC_CURSOR_ROTATE,      // LC_TRACKTOOL_ROTATE_XYZ
		LC_CURSOR_DELETE,      // LC_TRACKTOOL_ERASER
		LC_CURSOR_PAINT,       // LC_TRACKTOOL_PAINT
		LC_CURSOR_ZOOM,        // LC_TRACKTOOL_ZOOM
		LC_CURSOR_PAN,         // LC_TRACKTOOL_PAN
		LC_CURSOR_ROTATEX,     // LC_TRACKTOOL_ORBIT_X
		LC_CURSOR_ROTATEY,     // LC_TRACKTOOL_ORBIT_Y
		LC_CURSOR_ROTATE_VIEW, // LC_TRACKTOOL_ORBIT_XY
		LC_CURSOR_ROLL,        // LC_TRACKTOOL_ROLL
		LC_CURSOR_ZOOM_REGION  // LC_TRACKTOOL_ZOOM_REGION
	};

	return CursorFromTrackTool[mTrackTool];
}

lcVector3 View::GetMoveDirection(const lcVector3& Direction) const
{
	if (lcGetPreferences().mFixedAxes)
		return Direction;

	// TODO: rewrite this
	lcVector3 axis = Direction;

	lcVector3 Pts[3] = { lcVector3(5.0f, 5.0f, 0.1f), lcVector3(10.0f, 5.0f, 0.1f), lcVector3(5.0f, 10.0f, 0.1f) };
	UnprojectPoints(Pts, 3);

	float ax, ay;
	lcVector3 vx((Pts[1][0] - Pts[0][0]), (Pts[1][1] - Pts[0][1]), 0);//Pts[1][2] - Pts[0][2] };
	vx.Normalize();
	lcVector3 x(1, 0, 0);
	ax = acosf(lcDot(vx, x));

	lcVector3 vy((Pts[2][0] - Pts[0][0]), (Pts[2][1] - Pts[0][1]), 0);//Pts[2][2] - Pts[0][2] };
	vy.Normalize();
	lcVector3 y(0, -1, 0);
	ay = acosf(lcDot(vy, y));

	if (ax > 135)
		axis[0] = -axis[0];

	if (ay < 45)
		axis[1] = -axis[1];

	if (ax >= 45 && ax <= 135)
	{
		float tmp = axis[0];

		ax = acosf(lcDot(vx, y));
		if (ax > 90)
		{
			axis[0] = -axis[1];
			axis[1] = tmp;
		}
		else
		{
			axis[0] = axis[1];
			axis[1] = -tmp;
		}
	}

	return axis;
}

lcMatrix44 View::GetPieceInsertPosition() const
{
	PieceInfo* CurPiece = gMainWindow->mPreviewWidget->GetCurrentPiece();
	lcPiece* HitPiece = (lcPiece*)FindObjectUnderPointer(true).Object;

	if (HitPiece)
	{
		lcVector3 Dist(0, 0, HitPiece->mPieceInfo->m_fDimensions[2] - CurPiece->m_fDimensions[5]);
		Dist = mModel->SnapPosition(Dist);

		lcVector3 Position = lcMul31(Dist, HitPiece->mModelWorld);
		lcMatrix44 WorldMatrix = HitPiece->mModelWorld;
		WorldMatrix.SetTranslation(Position);

		return WorldMatrix;
	}

	lcVector3 ClickPoints[2] = { lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f), lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f) };
	UnprojectPoints(ClickPoints, 2);

	lcVector3 Intersection;

	if (lcLinePlaneIntersection(&Intersection, ClickPoints[0], ClickPoints[1], lcVector4(0, 0, 1, CurPiece->m_fDimensions[5])))
	{
		Intersection = mModel->SnapPosition(Intersection);
		return lcMatrix44Translation(Intersection);
	}

	return lcMatrix44Translation(UnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f)));
}

lcObjectSection View::FindObjectUnderPointer(bool PiecesOnly) const
{
	lcVector3 StartEnd[2] =
	{
		lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f),
		lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f)
	};

	UnprojectPoints(StartEnd, 2);

	lcObjectRayTest ObjectRayTest;

	ObjectRayTest.PiecesOnly = PiecesOnly;
	ObjectRayTest.ViewCamera = mCamera;
	ObjectRayTest.Start = StartEnd[0];
	ObjectRayTest.End = StartEnd[1];
	ObjectRayTest.Distance = FLT_MAX;
	ObjectRayTest.ObjectSection.Object = NULL;
	ObjectRayTest.ObjectSection.Section = 0;;

	mModel->RayTest(ObjectRayTest);

	return ObjectRayTest.ObjectSection;
}

lcArray<lcObject*> View::FindObjectsInBox(float x1, float y1, float x2, float y2) const
{
	float Left, Top, Bottom, Right;

	if (x1 < x2)
	{
		Left = x1;
		Right = x2;
	}
	else
	{
		Left = x2;
		Right = x1;
	}

	if (y1 > y2)
	{
		Top = y1;
		Bottom = y2;
	}
	else
	{
		Top = y2;
		Bottom = y1;
	}

	lcVector3 Corners[6] =
	{
		lcVector3(Left, Top, 0),
		lcVector3(Left, Bottom, 0),
		lcVector3(Right, Bottom, 0),
		lcVector3(Right, Top, 0),
		lcVector3(Left, Top, 1),
		lcVector3(Right, Bottom, 1)
	};

	UnprojectPoints(Corners, 6);

	lcVector3 PlaneNormals[6];
	PlaneNormals[0] = lcNormalize(lcCross(Corners[4] - Corners[0], Corners[1] - Corners[0])); // Left
	PlaneNormals[1] = lcNormalize(lcCross(Corners[5] - Corners[2], Corners[3] - Corners[2])); // Right
	PlaneNormals[2] = lcNormalize(lcCross(Corners[3] - Corners[0], Corners[4] - Corners[0])); // Top
	PlaneNormals[3] = lcNormalize(lcCross(Corners[1] - Corners[2], Corners[5] - Corners[2])); // Bottom
	PlaneNormals[4] = lcNormalize(lcCross(Corners[1] - Corners[0], Corners[3] - Corners[0])); // Front
	PlaneNormals[5] = lcNormalize(lcCross(Corners[1] - Corners[2], Corners[3] - Corners[2])); // Back

	lcObjectBoxTest ObjectBoxTest;
	ObjectBoxTest.ViewCamera = mCamera;
	ObjectBoxTest.Planes[0] = lcVector4(PlaneNormals[0], -lcDot(PlaneNormals[0], Corners[0]));
	ObjectBoxTest.Planes[1] = lcVector4(PlaneNormals[1], -lcDot(PlaneNormals[1], Corners[5]));
	ObjectBoxTest.Planes[2] = lcVector4(PlaneNormals[2], -lcDot(PlaneNormals[2], Corners[0]));
	ObjectBoxTest.Planes[3] = lcVector4(PlaneNormals[3], -lcDot(PlaneNormals[3], Corners[5]));
	ObjectBoxTest.Planes[4] = lcVector4(PlaneNormals[4], -lcDot(PlaneNormals[4], Corners[0]));
	ObjectBoxTest.Planes[5] = lcVector4(PlaneNormals[5], -lcDot(PlaneNormals[5], Corners[5]));

	mModel->BoxTest(ObjectBoxTest);

	return ObjectBoxTest.Objects;
}

void View::OnDraw()
{
	bool DrawInterface = mWidget != NULL;

	mModel->GetScene(mScene, mCamera, DrawInterface);

	if (DrawInterface && mTrackTool == LC_TRACKTOOL_INSERT)
	{
		PieceInfo* Info = gMainWindow->mPreviewWidget->GetCurrentPiece();

		if (Info)
			Info->AddRenderMeshes(mScene, GetPieceInsertPosition(), gMainWindow->mColorIndex, true, true);
	}

	mContext->SetDefaultState();
	mContext->SetViewport(0, 0, mWidth, mHeight);

	mModel->DrawBackground(mContext);

	const lcPreferences& Preferences = lcGetPreferences();
	const lcModelProperties& Properties = mModel->GetProperties();

	mContext->SetViewMatrix(mCamera->mWorldView);
	mContext->SetProjectionMatrix(GetProjectionMatrix());
	mContext->SetProgram(LC_PROGRAM_SIMPLE); // todo: lighting

	if (Preferences.mLightingMode != LC_LIGHTING_FLAT)
	{
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
		glEnable(GL_COLOR_MATERIAL);
		glShadeModel(GL_SMOOTH);

		float Shininess = 64.0f;
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &Shininess);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, lcVector4(0.8f, 0.8f, 0.8f, 1.0f));
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lcVector4(0.8f, 0.8f, 0.8f, 1.0f));

		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lcVector4(Properties.mAmbientColor, 1.0f));

//		for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
//			mLights[LightIdx]->Setup(LightIdx);

		glEnable(GL_LIGHTING);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (Properties.mFogEnabled)
	{
		glFogi(GL_FOG_MODE, GL_EXP);
		glFogf(GL_FOG_DENSITY, Properties.mFogDensity);
		glFogfv(GL_FOG_COLOR, lcVector4(Properties.mFogColor, 1.0f));
		glEnable(GL_FOG);
	}

	mContext->SetLineWidth(Preferences.mLineWidth);

	mContext->DrawOpaqueMeshes(mScene.mOpaqueMeshes);
	mContext->DrawTranslucentMeshes(mScene.mTranslucentMeshes);

	mContext->UnbindMesh(); // context remove

	if (Preferences.mLightingMode != LC_LIGHTING_FLAT)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (Properties.mFogEnabled)
		glDisable(GL_FOG);

	if (DrawInterface)
	{
		mContext->SetProgram(LC_PROGRAM_SIMPLE);
		mContext->DrawInterfaceObjects(mScene.mInterfaceObjects);

		mContext->SetLineWidth(1.0f);

		if (Preferences.mDrawGridStuds || Preferences.mDrawGridLines)
			DrawGrid();

		if (Preferences.mDrawAxes)
			DrawAxes();

		lcTool Tool = gMainWindow->GetTool();

		if ((Tool == LC_TOOL_SELECT || Tool == LC_TOOL_MOVE) && mTrackButton == LC_TRACKBUTTON_NONE && mModel->AnyObjectsSelected())
			DrawSelectMoveOverlay();
		else if (GetCurrentTool() == LC_TOOL_MOVE && mTrackButton != LC_TRACKBUTTON_NONE)
			DrawSelectMoveOverlay();
		else if ((Tool == LC_TOOL_ROTATE || (Tool == LC_TOOL_SELECT && mTrackButton != LC_TRACKBUTTON_NONE && mTrackTool >= LC_TRACKTOOL_ROTATE_X && mTrackTool <= LC_TRACKTOOL_ROTATE_XYZ)) && mModel->AnyPiecesSelected())
			DrawRotateOverlay();
		else if ((mTrackTool == LC_TRACKTOOL_SELECT || mTrackTool == LC_TRACKTOOL_ZOOM_REGION) && mTrackButton == LC_TRACKBUTTON_LEFT)
			DrawSelectZoomRegionOverlay();
		else if (Tool == LC_TOOL_ROTATE_VIEW && mTrackButton == LC_TRACKBUTTON_NONE)
			DrawRotateViewOverlay();

		DrawViewport();
	}
}

void View::DrawSelectMoveOverlay()
{
	mContext->SetProgram(LC_PROGRAM_SIMPLE);
	mContext->SetViewMatrix(mCamera->mWorldView);
	mContext->SetProjectionMatrix(GetProjectionMatrix());

	glDisable(GL_DEPTH_TEST);

	lcMatrix44 RelativeRotation = mModel->GetRelativeRotation();
	lcVector3 OverlayCenter = mModel->GetFocusOrSelectionCenter();
	bool AnyPiecesSelected = mModel->AnyPiecesSelected();

	lcMatrix44 WorldMatrix = lcMul(RelativeRotation, lcMatrix44Translation(OverlayCenter));

	const float OverlayScale = GetOverlayScale();
	WorldMatrix = lcMul(lcMatrix44Scale(lcVector3(OverlayScale, OverlayScale, OverlayScale)), WorldMatrix);

	mContext->SetWorldMatrix(WorldMatrix);

	mContext->SetIndexBuffer(mRotateMoveIndexBuffer);
	mContext->SetVertexBuffer(mRotateMoveVertexBuffer);
	mContext->SetVertexFormat(0, 3, 0, 0);

	if (mTrackButton == LC_TRACKBUTTON_NONE || (mTrackTool >= LC_TRACKTOOL_MOVE_X && mTrackTool <= LC_TRACKTOOL_MOVE_XYZ))
	{
		if ((mTrackTool == LC_TRACKTOOL_MOVE_X) || (mTrackTool == LC_TRACKTOOL_MOVE_XY) || (mTrackTool == LC_TRACKTOOL_MOVE_XZ))
		{
			mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			mContext->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
		}
		else if (mTrackButton == LC_TRACKBUTTON_NONE)
		{
			mContext->SetColor(0.8f, 0.0f, 0.0f, 1.0f);
			mContext->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
		}

		if ((mTrackTool == LC_TRACKTOOL_MOVE_Y) || (mTrackTool == LC_TRACKTOOL_MOVE_XY) || (mTrackTool == LC_TRACKTOOL_MOVE_YZ))
		{
			mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			mContext->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 36 * 2);
		}
		else if (mTrackButton == LC_TRACKBUTTON_NONE)
		{
			mContext->SetColor(0.0f, 0.8f, 0.0f, 1.0f);
			mContext->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 36 * 2);
		}

		if ((mTrackTool == LC_TRACKTOOL_MOVE_Z) || (mTrackTool == LC_TRACKTOOL_MOVE_XZ) || (mTrackTool == LC_TRACKTOOL_MOVE_YZ))
		{
			mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			mContext->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 72 * 2);
		}
		else if (mTrackButton == LC_TRACKBUTTON_NONE)
		{
			mContext->SetColor(0.0f, 0.0f, 0.8f, 1.0f);
			mContext->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 72 * 2);
		}
	}

	if (gMainWindow->GetTool() == LC_TOOL_SELECT && mTrackButton == LC_TRACKBUTTON_NONE && AnyPiecesSelected)
	{
		if (mTrackTool == LC_TRACKTOOL_ROTATE_X)
			mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
		else
			mContext->SetColor(0.8f, 0.0f, 0.0f, 1.0f);

		mContext->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, 108 * 2);

		if (mTrackTool == LC_TRACKTOOL_ROTATE_Y)
			mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
		else
			mContext->SetColor(0.0f, 0.8f, 0.0f, 1.0f);

		mContext->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, (108 + 120) * 2);

		if (mTrackTool == LC_TRACKTOOL_ROTATE_Z)
			mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
		else
			mContext->SetColor(0.0f, 0.0f, 0.8f, 1.0f);

		mContext->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, (108 + 240) * 2);
	}

	if ((mTrackTool == LC_TRACKTOOL_MOVE_XY) || (mTrackTool == LC_TRACKTOOL_MOVE_XZ) || (mTrackTool == LC_TRACKTOOL_MOVE_YZ))
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		mContext->SetColor(0.8f, 0.8f, 0.0f, 0.3f);

		if (mTrackTool == LC_TRACKTOOL_MOVE_XY)
			mContext->DrawIndexedPrimitives(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, (108 + 360 + 8) * 2);
		else if (mTrackTool == LC_TRACKTOOL_MOVE_XZ)
			mContext->DrawIndexedPrimitives(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, (108 + 360 + 4) * 2);
		else if (mTrackTool == LC_TRACKTOOL_MOVE_YZ)
			mContext->DrawIndexedPrimitives(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, (108 + 360) * 2);

		glDisable(GL_BLEND);
	}

	glEnable(GL_DEPTH_TEST);

	mContext->ClearIndexBuffer(); // context remove
}

void View::DrawRotateOverlay()
{
	const float OverlayScale = GetOverlayScale();
	const float OverlayRotateRadius = 2.0f;

	mContext->SetProgram(LC_PROGRAM_SIMPLE);
	mContext->SetViewMatrix(mCamera->mWorldView);
	mContext->SetProjectionMatrix(GetProjectionMatrix());

	glDisable(GL_DEPTH_TEST);

	int j;

	lcMatrix44 RelativeRotation = mModel->GetRelativeRotation();
	lcVector3 OverlayCenter = mModel->GetFocusOrSelectionCenter();
	lcVector3 MouseToolDistance = mModel->SnapRotation(mModel->GetMouseToolDistance());
	bool HasAngle = false;

	// Draw a disc showing the rotation amount.
	if (MouseToolDistance.LengthSquared() != 0.0f && (mTrackButton != LC_TRACKBUTTON_NONE))
	{
		lcVector4 Rotation;
		float Angle, Step;

		HasAngle = true;

		switch (mTrackTool)
		{
		case LC_TRACKTOOL_ROTATE_X:
			mContext->SetColor(0.8f, 0.0f, 0.0f, 0.3f);
			Angle = MouseToolDistance[0];
			Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
			break;
		case LC_TRACKTOOL_ROTATE_Y:
			mContext->SetColor(0.0f, 0.8f, 0.0f, 0.3f);
			Angle = MouseToolDistance[1];
			Rotation = lcVector4(90.0f, 0.0f, 0.0f, 1.0f);
			break;
		case LC_TRACKTOOL_ROTATE_Z:
			mContext->SetColor(0.0f, 0.0f, 0.8f, 0.3f);
			Angle = MouseToolDistance[2];
			Rotation = lcVector4(90.0f, 0.0f, -1.0f, 0.0f);
			break;
		default:
			Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
			Angle = 0.0f;
			break;
		};

		if (Angle > 0.0f)
		{
			Step = 360.0f / 32;
		}
		else
		{
			Angle = -Angle;
			Step = -360.0f / 32;
		}

		if (fabsf(Angle) >= fabsf(Step))
		{
			lcMatrix44 WorldMatrix = lcMul(RelativeRotation, lcMatrix44Translation(OverlayCenter));
			WorldMatrix = lcMul(lcMatrix44FromAxisAngle(lcVector3(Rotation[1], Rotation[2], Rotation[3]), Rotation[0] * LC_DTOR), WorldMatrix);

			mContext->SetWorldMatrix(WorldMatrix);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);

			lcVector3 Verts[33];
			Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);
			int NumVerts = 1;

			glVertexPointer(3, GL_FLOAT, 0, Verts);

			float StartAngle;
			int i = 0;

			if (Step < 0)
				StartAngle = -Angle;
			else
				StartAngle = Angle;

			do
			{
				float x = cosf((Step * i - StartAngle) * LC_DTOR) * OverlayRotateRadius * OverlayScale;
				float y = sinf((Step * i - StartAngle) * LC_DTOR) * OverlayRotateRadius * OverlayScale;

				Verts[NumVerts++] = lcVector3(0.0f, x, y);

				if (NumVerts == 33)
				{
					glDrawArrays(GL_TRIANGLE_FAN, 0, NumVerts);
					Verts[1] = Verts[32];
					NumVerts = 2;
				}

				i++;
				if (Step > 0)
					Angle -= Step;
				else
					Angle += Step;

			} while (Angle >= 0.0f);

			if (NumVerts > 2)
				glDrawArrays(GL_TRIANGLE_FAN, 0, NumVerts);

			glDisable(GL_BLEND);
		}
	}

	// Draw the circles.
	if (gMainWindow->GetTool() == LC_TOOL_ROTATE && !HasAngle && mTrackButton == LC_TRACKBUTTON_NONE)
	{
		lcMatrix44 Mat = lcMatrix44AffineInverse(mCamera->mWorldView);
		Mat.SetTranslation(OverlayCenter);

		lcVector3 Verts[32];

		for (j = 0; j < 32; j++)
		{
			lcVector3 Pt;

			Pt[0] = cosf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
			Pt[1] = sinf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
			Pt[2] = 0.0f;

			Verts[j] = lcMul31(Pt, Mat);
		}

		mContext->SetColor(0.1f, 0.1f, 0.1f, 1.0f);
		mContext->SetWorldMatrix(lcMatrix44Identity());

		glVertexPointer(3, GL_FLOAT, 0, Verts);
		glDrawArrays(GL_LINE_LOOP, 0, 32);
	}

	lcVector3 ViewDir = mCamera->mTargetPosition - mCamera->mPosition;
	ViewDir.Normalize();

	// Transform ViewDir to local space.
	ViewDir = lcMul30(ViewDir, lcMatrix44AffineInverse(RelativeRotation));

	lcMatrix44 WorldMatrix = lcMul(RelativeRotation, lcMatrix44Translation(OverlayCenter));
	mContext->SetWorldMatrix(WorldMatrix);

	// Draw each axis circle.
	for (int i = 0; i < 3; i++)
	{
		if (mTrackTool == LC_TRACKTOOL_ROTATE_X + i)
		{
			mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
		}
		else
		{
			if (gMainWindow->GetTool() != LC_TOOL_ROTATE || HasAngle || mTrackButton != LC_TRACKBUTTON_NONE)
				continue;

			switch (i)
			{
			case 0:
				mContext->SetColor(0.8f, 0.0f, 0.0f, 1.0f);
				break;
			case 1:
				mContext->SetColor(0.0f, 0.8f, 0.0f, 1.0f);
				break;
			case 2:
				mContext->SetColor(0.0f, 0.0f, 0.8f, 1.0f);
				break;
			}
		}

		lcVector3 Verts[64];
		int NumVerts = 0;

		for (j = 0; j < 32; j++)
		{
			lcVector3 v1, v2;

			switch (i)
			{
			case 0:
				v1 = lcVector3(0.0f, cosf(LC_2PI * j / 32), sinf(LC_2PI * j / 32));
				v2 = lcVector3(0.0f, cosf(LC_2PI * (j + 1) / 32), sinf(LC_2PI * (j + 1) / 32));
				break;

			case 1:
				v1 = lcVector3(cosf(LC_2PI * j / 32), 0.0f, sinf(LC_2PI * j / 32));
				v2 = lcVector3(cosf(LC_2PI * (j + 1) / 32), 0.0f, sinf(LC_2PI * (j + 1) / 32));
				break;

			case 2:
				v1 = lcVector3(cosf(LC_2PI * j / 32), sinf(LC_2PI * j / 32), 0.0f);
				v2 = lcVector3(cosf(LC_2PI * (j + 1) / 32), sinf(LC_2PI * (j + 1) / 32), 0.0f);
				break;
			}

			if (gMainWindow->GetTool() != LC_TOOL_ROTATE || HasAngle || mTrackButton != LC_TRACKBUTTON_NONE || lcDot(ViewDir, v1 + v2) <= 0.0f)
			{
				Verts[NumVerts++] = v1 * (OverlayRotateRadius * OverlayScale);
				Verts[NumVerts++] = v2 * (OverlayRotateRadius * OverlayScale);
			}
		}

		glVertexPointer(3, GL_FLOAT, 0, Verts);
		glDrawArrays(GL_LINES, 0, NumVerts);
	}

	// Draw tangent vector.
	if (mTrackButton != LC_TRACKBUTTON_NONE && ((mTrackTool == LC_TRACKTOOL_ROTATE_X) || (mTrackTool == LC_TRACKTOOL_ROTATE_Y) || (mTrackTool == LC_TRACKTOOL_ROTATE_Z)))
	{
		const float OverlayRotateArrowSize = 1.5f;
		const float OverlayRotateArrowCapSize = 0.25f;

		lcVector4 Rotation;
		float Angle;

		switch (mTrackTool)
		{
		case LC_TRACKTOOL_ROTATE_X:
			Angle = MouseToolDistance[0];
			Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
			break;
		case LC_TRACKTOOL_ROTATE_Y:
			Angle = MouseToolDistance[1];
			Rotation = lcVector4(90.0f, 0.0f, 0.0f, 1.0f);
			break;
		case LC_TRACKTOOL_ROTATE_Z:
			Angle = MouseToolDistance[2];
			Rotation = lcVector4(90.0f, 0.0f, -1.0f, 0.0f);
			break;
		default:
			Angle = 0.0f;
			Rotation = lcVector4(0.0f, 0.0f, 1.0f, 0.0f);
			break;
		};

		lcMatrix44 WorldMatrix = lcMul(RelativeRotation, lcMatrix44Translation(OverlayCenter));
		WorldMatrix = lcMul(lcMatrix44FromAxisAngle(lcVector3(Rotation[1], Rotation[2], Rotation[3]), Rotation[0] * LC_DTOR), WorldMatrix);
		mContext->SetWorldMatrix(WorldMatrix);

		mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);

		if (HasAngle)
		{
			float StartY = OverlayScale * OverlayRotateRadius;
			float EndZ = (Angle > 0.0f) ? OverlayScale * OverlayRotateArrowSize : -OverlayScale * OverlayRotateArrowSize;
			float TipZ = (Angle > 0.0f) ? -OverlayScale * OverlayRotateArrowCapSize : OverlayScale * OverlayRotateArrowCapSize;

			lcVector3 Verts[6];

			Verts[0] = lcVector3(0.0f, StartY, 0.0f);
			Verts[1] = lcVector3(0.0f, StartY, EndZ);

			Verts[2] = lcVector3(0.0f, StartY, EndZ);
			Verts[3] = lcVector3(0.0f, StartY + OverlayScale * OverlayRotateArrowCapSize, EndZ + TipZ);

			Verts[4] = lcVector3(0.0f, StartY, EndZ);
			Verts[5] = lcVector3(0.0f, StartY - OverlayScale * OverlayRotateArrowCapSize, EndZ + TipZ);

			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINES, 0, 6);
		}

		// Draw text.
		lcVector3 ScreenPos = ProjectPoint(OverlayCenter);

		mContext->SetWorldMatrix(lcMatrix44Identity());
		mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
		mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		gTexFont.MakeCurrent();
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		char buf[32];
		sprintf(buf, "[%.2f]", fabsf(Angle));

		int cx, cy;
		gTexFont.GetStringDimensions(&cx, &cy, buf);

		mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
		gTexFont.PrintText(mContext, ScreenPos[0] - (cx / 2), ScreenPos[1] + (cy / 2), 0.0f, buf);

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
	}

	glEnable(GL_DEPTH_TEST);
}

void View::DrawSelectZoomRegionOverlay()
{
	mContext->SetProgram(LC_PROGRAM_SIMPLE);
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));

	glDisable(GL_DEPTH_TEST);

	float pt1x = (float)mMouseDownX;
	float pt1y = (float)mMouseDownY;
	float pt2x = (float)mInputState.x;
	float pt2y = (float)mInputState.y;

	float Left, Right, Bottom, Top;

	if (pt1x < pt2x)
	{
		Left = pt1x;
		Right = pt2x;
	}
	else
	{
		Left = pt2x;
		Right = pt1x;
	}

	if (pt1y < pt2y)
	{
		Bottom = pt1y;
		Top = pt2y;
	}
	else
	{
		Bottom = pt2y;
		Top = pt1y;
	}

	Left = lcMax(Left, 0.0f);
	Right = lcMin(Right, mWidth - 1.0f);
	Bottom = lcMax(Bottom, 0.0f);
	Top = lcMin(Top, mHeight - 1.0f);

	float BorderX = lcMin(2.0f, Right - Left);
	float BorderY = lcMin(2.0f, Top - Bottom);

	float Verts[14][2] =
	{
		{ Left, Bottom },
		{ Left + BorderX, Bottom + BorderY },
		{ Right, Bottom },
		{ Right - BorderX, Bottom + BorderY },
		{ Right, Top },
		{ Right - BorderX, Top - BorderY },
		{ Left, Top },
		{ Left + BorderX, Top - BorderY },
		{ Left, Bottom },
		{ Left + BorderX, Bottom + BorderY },
		{ Left + BorderX, Bottom + BorderY },
		{ Right - BorderX, Bottom + BorderY },
		{ Left + BorderX, Top - BorderY },
		{ Right - BorderX, Top - BorderY },
	};

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormat(0, 2, 0, 0);

	mContext->SetColor(0.25f, 0.25f, 1.0f, 1.0f);
	mContext->DrawPrimitives(GL_TRIANGLE_STRIP, 0, 10);

	mContext->SetColor(0.25f, 0.25f, 1.0f, 0.25f);
	mContext->DrawPrimitives(GL_TRIANGLE_STRIP, 10, 4);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	mContext->ClearVertexBuffer(); // context remove
}

void View::DrawRotateViewOverlay()
{
	int x, y, w, h;

	x = 0;
	y = 0;
	w = mWidth;
	h = mHeight;

	mContext->SetProgram(LC_PROGRAM_SIMPLE);
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0, w, 0, h, -1, 1));

	glDisable(GL_DEPTH_TEST);
	mContext->SetColor(0.0f, 0.0f, 0.0f, 1.0f);

	float Verts[32 * 16 * 2];
	float* CurVert = Verts;

	float r = lcMin(w, h) * 0.35f;
	float cx = x + w / 2.0f;
	float cy = y + h / 2.0f;

	for (int i = 0; i < 32; i++)
	{
		*CurVert++ = cosf((float)i / 32.0f * (2.0f * LC_PI)) * r + cx;
		*CurVert++ = sinf((float)i / 32.0f * (2.0f * LC_PI)) * r + cy;
	}

	const float OverlayCameraSquareSize = lcMax(8.0f, (w + h) / 200.0f);

	*CurVert++ = cx + OverlayCameraSquareSize; *CurVert++ = cy + r + OverlayCameraSquareSize;
	*CurVert++ = cx - OverlayCameraSquareSize; *CurVert++ = cy + r + OverlayCameraSquareSize;
	*CurVert++ = cx - OverlayCameraSquareSize; *CurVert++ = cy + r - OverlayCameraSquareSize;
	*CurVert++ = cx + OverlayCameraSquareSize; *CurVert++ = cy + r - OverlayCameraSquareSize;
	*CurVert++ = cx + OverlayCameraSquareSize; *CurVert++ = cy - r + OverlayCameraSquareSize;
	*CurVert++ = cx - OverlayCameraSquareSize; *CurVert++ = cy - r + OverlayCameraSquareSize;
	*CurVert++ = cx - OverlayCameraSquareSize; *CurVert++ = cy - r - OverlayCameraSquareSize;
	*CurVert++ = cx + OverlayCameraSquareSize; *CurVert++ = cy - r - OverlayCameraSquareSize;
	*CurVert++ = cx + r + OverlayCameraSquareSize; *CurVert++ = cy + OverlayCameraSquareSize;
	*CurVert++ = cx + r - OverlayCameraSquareSize; *CurVert++ = cy + OverlayCameraSquareSize;
	*CurVert++ = cx + r - OverlayCameraSquareSize; *CurVert++ = cy - OverlayCameraSquareSize;
	*CurVert++ = cx + r + OverlayCameraSquareSize; *CurVert++ = cy - OverlayCameraSquareSize;
	*CurVert++ = cx - r + OverlayCameraSquareSize; *CurVert++ = cy + OverlayCameraSquareSize;
	*CurVert++ = cx - r - OverlayCameraSquareSize; *CurVert++ = cy + OverlayCameraSquareSize;
	*CurVert++ = cx - r - OverlayCameraSquareSize; *CurVert++ = cy - OverlayCameraSquareSize;
	*CurVert++ = cx - r + OverlayCameraSquareSize; *CurVert++ = cy - OverlayCameraSquareSize;

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormat(0, 2, 0, 0);

	GLushort Indices[64 + 32] = 
	{
		0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16,
		17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31, 0,
		32, 33, 33, 34, 34, 35, 35, 32, 36, 37, 37, 38, 38, 39, 39, 36,
		40, 41, 41, 42, 42, 43, 43, 40, 44, 45, 45, 46, 46, 47, 47, 44
	};

	mContext->SetIndexBufferPointer(Indices);
	mContext->DrawIndexedPrimitives(GL_LINES, 96, GL_UNSIGNED_SHORT, 0);

	glEnable(GL_DEPTH_TEST);

	mContext->ClearVertexBuffer(); // context remove
}

void View::DrawGrid()
{
	const lcPreferences& Preferences = lcGetPreferences();

	mContext->SetWorldMatrix(lcMatrix44Identity());

	const int Spacing = lcMax(Preferences.mGridLineSpacing, 1);
	int MinX, MaxX, MinY, MaxY;
	float BoundingBox[6];

	bool GridSizeValid = mModel->GetPiecesBoundingBox(BoundingBox);

	if (mTrackTool == LC_TRACKTOOL_INSERT)
	{
		PieceInfo* CurPiece = gMainWindow->mPreviewWidget->GetCurrentPiece();

		if (CurPiece)
		{
			lcVector3 Points[8] =
			{
				lcVector3(CurPiece->m_fDimensions[0], CurPiece->m_fDimensions[1], CurPiece->m_fDimensions[5]),
				lcVector3(CurPiece->m_fDimensions[3], CurPiece->m_fDimensions[1], CurPiece->m_fDimensions[5]),
				lcVector3(CurPiece->m_fDimensions[0], CurPiece->m_fDimensions[1], CurPiece->m_fDimensions[2]),
				lcVector3(CurPiece->m_fDimensions[3], CurPiece->m_fDimensions[4], CurPiece->m_fDimensions[5]),
				lcVector3(CurPiece->m_fDimensions[3], CurPiece->m_fDimensions[4], CurPiece->m_fDimensions[2]),
				lcVector3(CurPiece->m_fDimensions[0], CurPiece->m_fDimensions[4], CurPiece->m_fDimensions[2]),
				lcVector3(CurPiece->m_fDimensions[0], CurPiece->m_fDimensions[4], CurPiece->m_fDimensions[5]),
				lcVector3(CurPiece->m_fDimensions[3], CurPiece->m_fDimensions[1], CurPiece->m_fDimensions[2])
			};

			lcMatrix44 WorldMatrix = GetPieceInsertPosition();

			for (int i = 0; i < 8; i++)
			{
				lcVector3 Point = lcMul31(Points[i], WorldMatrix);

				if (Point[0] < BoundingBox[0]) BoundingBox[0] = Point[0];
				if (Point[1] < BoundingBox[1]) BoundingBox[1] = Point[1];
				if (Point[2] < BoundingBox[2]) BoundingBox[2] = Point[2];
				if (Point[0] > BoundingBox[3]) BoundingBox[3] = Point[0];
				if (Point[1] > BoundingBox[4]) BoundingBox[4] = Point[1];
				if (Point[2] > BoundingBox[5]) BoundingBox[5] = Point[2];
			}

			GridSizeValid = true;
		}
	}

	if (GridSizeValid)
	{
		MinX = (int)(floorf(BoundingBox[0] / (20.0f * Spacing))) - 1;
		MinY = (int)(floorf(BoundingBox[1] / (20.0f * Spacing))) - 1;
		MaxX = (int)(ceilf(BoundingBox[3] / (20.0f * Spacing))) + 1;
		MaxY = (int)(ceilf(BoundingBox[4] / (20.0f * Spacing))) + 1;

		MinX = lcMin(MinX, -2);
		MinY = lcMin(MinY, -2);
		MaxX = lcMax(MaxX, 2);
		MaxY = lcMax(MaxY, 2);
	}
	else
	{
		MinX = -2;
		MinY = -2;
		MaxX = 2;
		MaxY = 2;
	}

	if (!mGridBuffer.IsValid() || MinX != mGridSettings[0] || MinY != mGridSettings[1] || MaxX != mGridSettings[2] || MaxY != mGridSettings[3] ||
	    Spacing != mGridSettings[4] || (Preferences.mDrawGridStuds ? 1 : 0) != mGridSettings[5] || (Preferences.mDrawGridLines ? 1 : 0) != mGridSettings[6])
	{
		int VertexBufferSize = 0;

		if (Preferences.mDrawGridStuds)
			VertexBufferSize += 4 * 5 * sizeof(float);

		if (Preferences.mDrawGridLines)
			VertexBufferSize += 2 * (MaxX - MinX + MaxY - MinY + 2) * 3 * sizeof(float);

		float* Verts = (float*)malloc(VertexBufferSize);
		float* CurVert = Verts;

		if (Preferences.mDrawGridStuds)
		{
			float Left = MinX * 20.0f * Spacing;
			float Right = MaxX * 20.0f * Spacing;
			float Top = MinY * 20.0f * Spacing;
			float Bottom = MaxY * 20.0f * Spacing;
			float Z = 0;
			float U = (MaxX - MinX) * Spacing;
			float V = (MaxY - MinY) * Spacing;

			*CurVert++ = Left;
			*CurVert++ = Top;
			*CurVert++ = Z;
			*CurVert++ = 0.0f;
			*CurVert++ = V;

			*CurVert++ = Left;
			*CurVert++ = Bottom;
			*CurVert++ = Z;
			*CurVert++ = 0.0f;
			*CurVert++ = 0.0f;

			*CurVert++ = Right;
			*CurVert++ = Bottom;
			*CurVert++ = Z;
			*CurVert++ = U;
			*CurVert++ = 0.0f;

			*CurVert++ = Right;
			*CurVert++ = Top;
			*CurVert++ = Z;
			*CurVert++ = U;
			*CurVert++ = V;
		}

		if (Preferences.mDrawGridLines)
		{
			float LineSpacing = Spacing * 20.0f;

			for (int Step = MinX; Step < MaxX + 1; Step++)
			{
				*CurVert++ = Step * LineSpacing;
				*CurVert++ = MinY * LineSpacing;
				*CurVert++ = 0.0f;
				*CurVert++ = Step * LineSpacing;
				*CurVert++ = MaxY * LineSpacing;
				*CurVert++ = 0.0f;
			}

			for (int Step = MinY; Step < MaxY + 1; Step++)
			{
				*CurVert++ = MinX * LineSpacing;
				*CurVert++ = Step * LineSpacing;
				*CurVert++ = 0.0f;
				*CurVert++ = MaxX * LineSpacing;
				*CurVert++ = Step * LineSpacing;
				*CurVert++ = 0.0f;
			}
		}

		mGridSettings[0] = MinX;
		mGridSettings[1] = MinY;
		mGridSettings[2] = MaxX;
		mGridSettings[3] = MaxY;
		mGridSettings[4] = Spacing;
		mGridSettings[5] = (Preferences.mDrawGridStuds ? 1 : 0);
		mGridSettings[6] = (Preferences.mDrawGridLines ? 1 : 0);

		mContext->DestroyVertexBuffer(mGridBuffer);
		mGridBuffer = mContext->CreateVertexBuffer(VertexBufferSize, Verts);
		free(Verts);
	}

	int BufferOffset = 0;
	mContext->SetVertexBuffer(mGridBuffer);

	if (Preferences.mDrawGridStuds)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBindTexture(GL_TEXTURE_2D, gGridTexture->mTexture);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		mContext->SetProgram(LC_PROGRAM_TEXTURE);
		mContext->SetColor(lcVector4FromColor(Preferences.mGridStudColor));

		mContext->SetVertexFormat(0, 3, 2, 0);
		mContext->DrawPrimitives(GL_QUADS, 0, 4);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);

		BufferOffset = 4 * 5 * sizeof(float);
	}

	if (Preferences.mDrawGridLines)
	{
		mContext->SetLineWidth(1.0f);
		mContext->SetProgram(LC_PROGRAM_SIMPLE);
		mContext->SetColor(lcVector4FromColor(Preferences.mGridLineColor));

		int NumVerts = 2 * (MaxX - MinX + MaxY - MinY + 2);

		mContext->SetVertexFormat(BufferOffset, 3, 0, 0);
		mContext->DrawPrimitives(GL_LINES, 0, NumVerts);
	}

	mContext->ClearVertexBuffer(); // context remove
}

void View::DrawAxes()
{
//	glClear(GL_DEPTH_BUFFER_BIT);

	const float Verts[28 * 3] =
	{
		 0.00f,  0.00f,  0.00f, 20.00f,  0.00f,  0.00f, 12.00f,  3.00f,  0.00f, 12.00f,  2.12f,  2.12f,
		12.00f,  0.00f,  3.00f, 12.00f, -2.12f,  2.12f, 12.00f, -3.00f,  0.00f, 12.00f, -2.12f, -2.12f,
		12.00f,  0.00f, -3.00f, 12.00f,  2.12f, -2.12f,  0.00f, 20.00f,  0.00f,  3.00f, 12.00f,  0.00f,
		 2.12f, 12.00f,  2.12f,  0.00f, 12.00f,  3.00f, -2.12f, 12.00f,  2.12f, -3.00f, 12.00f,  0.00f,
		-2.12f, 12.00f, -2.12f,  0.00f, 12.00f, -3.00f,  2.12f, 12.00f, -2.12f,  0.00f,  0.00f, 20.00f,
		 0.00f,  3.00f, 12.00f,  2.12f,  2.12f, 12.00f,  3.00f,  0.00f, 12.00f,  2.12f, -2.12f, 12.00f,
		 0.00f, -3.00f, 12.00f, -2.12f, -2.12f, 12.00f, -3.00f,  0.00f, 12.00f, -2.12f,  2.12f, 12.00f,
	};

	const GLushort Indices[78] =
	{
		 0,  1,  0, 10,  0, 19,
		 1,  2,  3,  1,  3,  4,  1,  4,  5,  1,  5,  6,  1,  6,  7,  1,  7,  8,  1,  8,  9,  1,  9,  2,
		10, 11, 12, 10, 12, 13, 10, 13, 14, 10, 14, 15, 10, 15, 16, 10, 16, 17, 10, 17, 18, 10, 18, 11,
		19, 20, 21, 19, 21, 22, 19, 22, 23, 19, 23, 24, 19, 24, 25, 19, 25, 26, 19, 26, 27, 19, 27, 20
	};

	lcMatrix44 TranslationMatrix = lcMatrix44Translation(lcVector3(25.375f, 25.375f, 0.0f));
	lcMatrix44 WorldViewMatrix = mCamera->mWorldView;
	WorldViewMatrix.SetTranslation(lcVector3(0, 0, 0));

	mContext->SetProgram(LC_PROGRAM_SIMPLE);
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMul(WorldViewMatrix, TranslationMatrix));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0, mWidth, 0, mHeight, -50, 50));

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormat(0, 3, 0, 0);
	mContext->SetIndexBufferPointer(Indices);

	mContext->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_LINES, 6, GL_UNSIGNED_SHORT, 0);

	mContext->SetColor(0.8f, 0.0f, 0.0f, 1.0f),
	mContext->DrawIndexedPrimitives(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, 6 * 2);
	mContext->SetColor(0.0f, 0.8f, 0.0f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, (6 + 24) * 2);
	mContext->SetColor(0.0f, 0.0f, 0.8f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, (6 + 48) * 2);

	mContext->SetProgram(LC_PROGRAM_TEXTURE);
	mContext->SetViewMatrix(TranslationMatrix);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	gTexFont.MakeCurrent();
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	float TextBuffer[4 * 5 * 3];
	lcVector3 PosX = lcMul30(lcVector3(25.0f, 0.0f, 0.0f), WorldViewMatrix);
	gTexFont.GetGlyphQuad(PosX.x, PosX.y, PosX.z, 'X', TextBuffer);
	lcVector3 PosY = lcMul30(lcVector3(0.0f, 25.0f, 0.0f), WorldViewMatrix);
	gTexFont.GetGlyphQuad(PosY.x, PosY.y, PosY.z, 'Y', TextBuffer + 5 * 4);
	lcVector3 PosZ = lcMul30(lcVector3(0.0f, 0.0f, 25.0f), WorldViewMatrix);
	gTexFont.GetGlyphQuad(PosZ.x, PosZ.y, PosZ.z, 'Z', TextBuffer + 5 * 4 * 2);

	mContext->SetVertexBufferPointer(TextBuffer);
	mContext->SetVertexFormat(0, 3, 2, 0);

	mContext->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	mContext->DrawPrimitives(GL_QUADS, 0, 4 * 3);

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	mContext->ClearVertexBuffer(); // context remove
}

void View::DrawViewport()
{
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	if (gMainWindow->GetActiveView() == this)
	{
		mContext->SetProgram(LC_PROGRAM_SIMPLE);
		mContext->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
		float Verts[8] = { 0.0f, 0.0f, mWidth - 1.0f, 0.0f, mWidth - 1.0f, mHeight - 1.0f, 0.0f, mHeight - 1.0f };

		mContext->SetVertexBufferPointer(Verts);
		mContext->SetVertexFormat(0, 2, 0, 0);
		mContext->DrawPrimitives(GL_LINE_LOOP, 0, 4);
	}

	mContext->ClearVertexBuffer(); // context remove

	const char* CameraName = mCamera->GetName();

	if (CameraName[0])
	{
		mContext->SetProgram(LC_PROGRAM_TEXTURE);
		mContext->SetColor(0.0f, 0.0f, 0.0f, 1.0f);

		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		gTexFont.MakeCurrent();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		gTexFont.PrintText(mContext, 3.0f, (float)mHeight - 1.0f - 6.0f, 0.0f, CameraName);

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
	}

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
}

void View::OnInitialUpdate()
{
	gMainWindow->AddView(this);
}

void View::OnUpdateCursor()
{
	SetCursor(GetCursor());
}

lcTool View::GetCurrentTool() const
{
	const lcTool ToolFromTrackTool[] =
	{
		LC_TOOL_SELECT,      // LC_TRACKTOOL_NONE
		LC_TOOL_INSERT,      // LC_TRACKTOOL_INSERT
		LC_TOOL_LIGHT,       // LC_TRACKTOOL_POINTLIGHT
		LC_TOOL_SPOTLIGHT,   // LC_TRACKTOOL_SPOTLIGHT
		LC_TOOL_CAMERA,      // LC_TRACKTOOL_CAMERA
		LC_TOOL_SELECT,      // LC_TRACKTOOL_SELECT
		LC_TOOL_MOVE,        // LC_TRACKTOOL_MOVE_X
		LC_TOOL_MOVE,        // LC_TRACKTOOL_MOVE_Y
		LC_TOOL_MOVE,        // LC_TRACKTOOL_MOVE_Z
		LC_TOOL_MOVE,        // LC_TRACKTOOL_MOVE_XY
		LC_TOOL_MOVE,        // LC_TRACKTOOL_MOVE_XZ
		LC_TOOL_MOVE,        // LC_TRACKTOOL_MOVE_YZ
		LC_TOOL_MOVE,        // LC_TRACKTOOL_MOVE_XYZ
		LC_TOOL_ROTATE,      // LC_TRACKTOOL_ROTATE_X
		LC_TOOL_ROTATE,      // LC_TRACKTOOL_ROTATE_Y
		LC_TOOL_ROTATE,      // LC_TRACKTOOL_ROTATE_Z
		LC_TOOL_ROTATE,      // LC_TRACKTOOL_ROTATE_XY
		LC_TOOL_ROTATE,      // LC_TRACKTOOL_ROTATE_XYZ
		LC_TOOL_ERASER,      // LC_TRACKTOOL_ERASER
		LC_TOOL_PAINT,       // LC_TRACKTOOL_PAINT
		LC_TOOL_ZOOM,        // LC_TRACKTOOL_ZOOM
		LC_TOOL_PAN,         // LC_TRACKTOOL_PAN
		LC_TOOL_ROTATE_VIEW, // LC_TRACKTOOL_ORBIT_X
		LC_TOOL_ROTATE_VIEW, // LC_TRACKTOOL_ORBIT_Y
		LC_TOOL_ROTATE_VIEW, // LC_TRACKTOOL_ORBIT_XY
		LC_TOOL_ROLL,        // LC_TRACKTOOL_ROLL
		LC_TOOL_ZOOM_REGION  // LC_TRACKTOOL_ZOOM_REGION
	};

	return ToolFromTrackTool[mTrackTool];
}

float View::GetOverlayScale() const
{
	lcVector3 OverlayCenter = mModel->GetFocusOrSelectionCenter();

	lcVector3 ScreenPos = ProjectPoint(OverlayCenter);
	ScreenPos[0] += 10.0f;
	lcVector3 Point = UnprojectPoint(ScreenPos);

	lcVector3 Dist(Point - OverlayCenter);
	return Dist.Length() * 5.0f;
}

void View::BeginPieceDrag()
{
	mDragState = LC_DRAGSTATE_PIECE;
	UpdateTrackTool();
}

void View::EndPieceDrag(bool Accept)
{
	if (Accept)
		mModel->InsertPieceToolClicked(GetPieceInsertPosition());

	mDragState = LC_DRAGSTATE_NONE;
	UpdateTrackTool();
}

void View::SetProjection(bool Ortho)
{
	if (mCamera->IsSimple())
	{
		mCamera->SetOrtho(Ortho);
		Redraw();

		gMainWindow->UpdatePerspective();
	}
	else
		mModel->SetObjectProperty(mCamera, LC_CAMERA_PROPERTY_ORTHO, &Ortho);
}

void View::LookAt()
{
	mModel->LookAt(mCamera);
}

void View::ZoomExtents()
{
	mModel->ZoomExtents(mCamera, (float)mWidth / (float)mHeight);
}

void View::UpdateTrackTool()
{
	lcTool CurrentTool = gMainWindow->GetTool();
	lcTrackTool NewTrackTool = mTrackTool;
	int x = mInputState.x;
	int y = mInputState.y;
	bool Redraw = false;

	switch (CurrentTool)
	{
	case LC_TOOL_INSERT:
		NewTrackTool = LC_TRACKTOOL_INSERT;
		break;

	case LC_TOOL_LIGHT:
		NewTrackTool = LC_TRACKTOOL_POINTLIGHT;
		break;

	case LC_TOOL_SPOTLIGHT:
		NewTrackTool = LC_TRACKTOOL_SPOTLIGHT;
		break;

	case LC_TOOL_CAMERA:
		NewTrackTool = LC_TRACKTOOL_CAMERA;
		break;

	case LC_TOOL_SELECT:
	case LC_TOOL_MOVE:
		{
			const float OverlayScale = GetOverlayScale();
			const float OverlayMovePlaneSize = 0.5f * OverlayScale;
			const float OverlayMoveArrowSize = 1.5f * OverlayScale;
			const float OverlayMoveArrowCapRadius = 0.1f * OverlayScale;
			const float OverlayRotateArrowStart = 1.0f * OverlayScale;
			const float OverlayRotateArrowEnd = 1.5f * OverlayScale;

			NewTrackTool = (CurrentTool == LC_TOOL_MOVE) ? LC_TRACKTOOL_MOVE_XYZ : LC_TRACKTOOL_SELECT;

			lcVector3 OverlayCenter;
			if (!mModel->GetFocusOrSelectionCenter(OverlayCenter))
				break;

			// Intersect the mouse with the 3 planes.
			lcVector3 PlaneNormals[3] =
			{
				lcVector3(1.0f, 0.0f, 0.0f),
				lcVector3(0.0f, 1.0f, 0.0f),
				lcVector3(0.0f, 0.0f, 1.0f),
			};

			lcMatrix44 RelativeRotation = mModel->GetRelativeRotation();

			for (int i = 0; i < 3; i++)
				PlaneNormals[i] = lcMul30(PlaneNormals[i], RelativeRotation);

			lcVector3 StartEnd[2] = { lcVector3((float)x, (float)y, 0.0f), lcVector3((float)x, (float)y, 1.0f) };
			UnprojectPoints(StartEnd, 2);
			const lcVector3& Start = StartEnd[0];
			const lcVector3& End = StartEnd[1];
			float ClosestIntersectionDistance = FLT_MAX;

			for (int AxisIndex = 0; AxisIndex < 3; AxisIndex++)
			{
				lcVector4 Plane(PlaneNormals[AxisIndex], -lcDot(PlaneNormals[AxisIndex], OverlayCenter));
				lcVector3 Intersection;

				if (!lcLinePlaneIntersection(&Intersection, Start, End, Plane))
					continue;

				float IntersectionDistance = lcLengthSquared(Intersection - Start);

				if (IntersectionDistance > ClosestIntersectionDistance)
					continue;

				lcVector3 Dir(Intersection - OverlayCenter);

				float Proj1 = lcDot(Dir, PlaneNormals[(AxisIndex + 1) % 3]);
				float Proj2 = lcDot(Dir, PlaneNormals[(AxisIndex + 2) % 3]);

				if (Proj1 > 0.0f && Proj1 < OverlayMovePlaneSize && Proj2 > 0.0f && Proj2 < OverlayMovePlaneSize)
				{
					lcTrackTool PlaneModes[] = { LC_TRACKTOOL_MOVE_YZ, LC_TRACKTOOL_MOVE_XZ, LC_TRACKTOOL_MOVE_XY };

					NewTrackTool = PlaneModes[AxisIndex];

					ClosestIntersectionDistance = IntersectionDistance;
				}

				if (CurrentTool == LC_TOOL_SELECT && Proj1 > OverlayRotateArrowStart && Proj1 < OverlayRotateArrowEnd && Proj2 > OverlayRotateArrowStart && Proj2 < OverlayRotateArrowEnd && mModel->AnyPiecesSelected())
				{
					lcTrackTool PlaneModes[] = { LC_TRACKTOOL_ROTATE_X, LC_TRACKTOOL_ROTATE_Y, LC_TRACKTOOL_ROTATE_Z };

					NewTrackTool = PlaneModes[AxisIndex];

					ClosestIntersectionDistance = IntersectionDistance;
				}

				if (fabs(Proj1) < OverlayMoveArrowCapRadius && Proj2 > 0.0f && Proj2 < OverlayMoveArrowSize)
				{
					lcTrackTool DirModes[] = { LC_TRACKTOOL_MOVE_Z, LC_TRACKTOOL_MOVE_X, LC_TRACKTOOL_MOVE_Y };

					NewTrackTool = DirModes[AxisIndex];

					ClosestIntersectionDistance = IntersectionDistance;
				}

				if (fabs(Proj2) < OverlayMoveArrowCapRadius && Proj1 > 0.0f && Proj1 < OverlayMoveArrowSize)
				{
					lcTrackTool DirModes[] = { LC_TRACKTOOL_MOVE_Y, LC_TRACKTOOL_MOVE_Z, LC_TRACKTOOL_MOVE_X };

					NewTrackTool = DirModes[AxisIndex];

					ClosestIntersectionDistance = IntersectionDistance;
				}
			}

			Redraw = true;
		}
		break;

	case LC_TOOL_ROTATE:
		{
			const float OverlayScale = GetOverlayScale();
			const float OverlayRotateRadius = 2.0f;

			NewTrackTool = LC_TRACKTOOL_ROTATE_XYZ;

			lcVector3 OverlayCenter;
			if (!mModel->GetFocusOrSelectionCenter(OverlayCenter))
				break;

			// Calculate the distance from the mouse pointer to the center of the sphere.
			lcVector3 StartEnd[2] = { lcVector3((float)x, (float)y, 0.0f), lcVector3((float)x, (float)y, 1.0f) };
			UnprojectPoints(StartEnd, 2);
			const lcVector3& SegStart = StartEnd[0];
			const lcVector3& SegEnd = StartEnd[1];

			lcVector3 Line = SegEnd - SegStart;
			lcVector3 Vec = OverlayCenter - SegStart;

			float u = lcDot(Vec, Line) / Line.LengthSquared();

			// Closest point in the line to the mouse.
			lcVector3 Closest = SegStart + Line * u;

			float Distance = (Closest - OverlayCenter).Length();
			const float Epsilon = 0.25f * OverlayScale;

			if (Distance > (OverlayRotateRadius * OverlayScale + Epsilon))
			{
				NewTrackTool = LC_TRACKTOOL_ROTATE_XYZ;
			}
			else if (Distance < (OverlayRotateRadius * OverlayScale + Epsilon))
			{
				// 3D rotation unless we're over one of the axis circles.
				NewTrackTool = LC_TRACKTOOL_ROTATE_XYZ;

				// Point P on a line defined by two points P1 and P2 is described by P = P1 + u (P2 - P1)
				// A sphere centered at P3 with radius r is described by (x - x3)^2 + (y - y3)^2 + (z - z3)^2 = r^2
				// Substituting the equation of the line into the sphere gives a quadratic equation where:
				// a = (x2 - x1)^2 + (y2 - y1)^2 + (z2 - z1)^2
				// b = 2[ (x2 - x1) (x1 - x3) + (y2 - y1) (y1 - y3) + (z2 - z1) (z1 - z3) ]
				// c = x32 + y32 + z32 + x12 + y12 + z12 - 2[x3 x1 + y3 y1 + z3 z1] - r2
				// The solutions to this quadratic are described by: (-b +- sqrt(b^2 - 4 a c) / 2 a
				// The exact behavior is determined by b^2 - 4 a c:
				// If this is less than 0 then the line does not intersect the sphere.
				// If it equals 0 then the line is a tangent to the sphere intersecting it at one point
				// If it is greater then 0 the line intersects the sphere at two points.

				float x1 = SegStart[0], y1 = SegStart[1], z1 = SegStart[2];
				float x2 = SegEnd[0], y2 = SegEnd[1], z2 = SegEnd[2];
				float x3 = OverlayCenter[0], y3 = OverlayCenter[1], z3 = OverlayCenter[2];
				float r = OverlayRotateRadius * OverlayScale;

				// TODO: rewrite using vectors.
				float a = (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1) + (z2 - z1)*(z2 - z1);
				float b = 2 * ((x2 - x1)*(x1 - x3) + (y2 - y1)*(y1 - y3) + (z2 - z1)*(z1 - z3));
				float c = x3*x3 + y3*y3 + z3*z3 + x1*x1 + y1*y1 + z1*z1 - 2*(x3*x1 + y3*y1 + z3*z1) - r*r;
				float f = b * b - 4 * a * c;

				if (f >= 0.0f)
				{
					lcVector3 ViewDir(mCamera->mTargetPosition - mCamera->mPosition);

					float u1 = (-b + sqrtf(f)) / (2*a);
					float u2 = (-b - sqrtf(f)) / (2*a);

					lcVector3 Intersections[2] =
					{
						lcVector3(x1 + u1*(x2-x1), y1 + u1*(y2-y1), z1 + u1*(z2-z1)),
						lcVector3(x1 + u2*(x2-x1), y1 + u2*(y2-y1), z1 + u2*(z2-z1))
					};

					lcMatrix44 RelativeRotation = mModel->GetRelativeRotation();

					for (int i = 0; i < 2; i++)
					{
						lcVector3 Dist = Intersections[i] - OverlayCenter;

						if (lcDot(ViewDir, Dist) > 0.0f)
							continue;

						Dist = lcMul30(Dist, RelativeRotation);

						// Check if we're close enough to one of the axis.
						Dist.Normalize();

						float dx = fabsf(Dist[0]);
						float dy = fabsf(Dist[1]);
						float dz = fabsf(Dist[2]);

						if (dx < dy)
						{
							if (dx < dz)
							{
								if (dx < Epsilon)
									NewTrackTool = LC_TRACKTOOL_ROTATE_X;
							}
							else
							{
								if (dz < Epsilon)
									NewTrackTool = LC_TRACKTOOL_ROTATE_Z;
							}
						}
						else
						{
							if (dy < dz)
							{
								if (dy < Epsilon)
									NewTrackTool = LC_TRACKTOOL_ROTATE_Y;
							}
							else
							{
								if (dz < Epsilon)
									NewTrackTool = LC_TRACKTOOL_ROTATE_Z;
							}
						}

						if (NewTrackTool != LC_TRACKTOOL_ROTATE_XYZ)
						{
							switch (NewTrackTool)
							{
							case LC_TRACKTOOL_ROTATE_X:
								Dist[0] = 0.0f;
								break;
							case LC_TRACKTOOL_ROTATE_Y:
								Dist[1] = 0.0f;
								break;
							case LC_TRACKTOOL_ROTATE_Z:
								Dist[2] = 0.0f;
								break;
							default:
								break;
							}

							Dist *= r;
							break;
						}
					}
				}
			}

			Redraw = true;
		}
		break;

	case LC_TOOL_ERASER:
		NewTrackTool = LC_TRACKTOOL_ERASER;
		break;

	case LC_TOOL_PAINT:
		NewTrackTool = LC_TRACKTOOL_PAINT;
		break;

	case LC_TOOL_ZOOM:
		NewTrackTool = LC_TRACKTOOL_ZOOM;
		break;

	case LC_TOOL_PAN:
		NewTrackTool = LC_TRACKTOOL_PAN;
		break;

	case LC_TOOL_ROTATE_VIEW:
		{
			int vx, vy, vw, vh;

			vx = 0;
			vy = 0;
			vw = mWidth;
			vh = mHeight;

			int cx = vx + vw / 2;
			int cy = vy + vh / 2;

			float d = sqrtf((float)((cx - x) * (cx - x) + (cy - y) * (cy - y)));
			float r = lcMin(vw, vh) * 0.35f;

			const float SquareSize = lcMax(8.0f, (vw + vh) / 200.0f);

			if ((d < r + SquareSize) && (d > r - SquareSize))
			{
				if ((cx - x < SquareSize) && (cx - x > -SquareSize))
					NewTrackTool = LC_TRACKTOOL_ORBIT_Y;

				if ((cy - y < SquareSize) && (cy - y > -SquareSize))
					NewTrackTool = LC_TRACKTOOL_ORBIT_X;
			}
			else
			{
				if (d < r)
					NewTrackTool = LC_TRACKTOOL_ORBIT_XY;
				else
					NewTrackTool = LC_TRACKTOOL_ROLL;
			}
		}
		break;

	case LC_TOOL_ROLL:
		NewTrackTool = LC_TRACKTOOL_ROLL;
		break;

	case LC_TOOL_ZOOM_REGION:
		NewTrackTool = LC_TRACKTOOL_ZOOM_REGION;
		break;
	}

	switch (mDragState)
	{
	case LC_DRAGSTATE_NONE:
		break;

	case LC_DRAGSTATE_PIECE:
		NewTrackTool = LC_TRACKTOOL_INSERT;
		Redraw = true;
		break;
	}

	if (NewTrackTool != mTrackTool)
	{
		mTrackTool = NewTrackTool;
		OnUpdateCursor();

		if (Redraw)
			gMainWindow->UpdateAllViews();
	}

/*
	if (CurrentTool != LC_TOOL_SELECT && CurrentTool != LC_TOOL_MOVE && CurrentTool != LC_TOOL_ROTATE)
		return LC_TRACKTOOL_NONE;

	int Viewport[4] = { 0, 0, mWidth, mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];

	const lcMatrix44& ModelView = mCamera->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(mCamera->mFOV, Aspect, mCamera->mNear, mCamera->mFar);

	lcVector3 OverlayCenter = mModel->mActiveModel->GetFocusOrSelectionCenter();

	lcVector3 ScreenPos = lcProjectPoint(OverlayCenter, ModelView, Projection, Viewport);
	ScreenPos[0] += 10.0f;
	lcVector3 Point = lcUnprojectPoint(ScreenPos, ModelView, Projection, Viewport);

	lcVector3 Dist(Point - OverlayCenter);
	float OverlayScale = Dist.Length() * 5.0f;

	if (InterfaceScale)
		*InterfaceScale = OverlayScale;

	if (InterfaceCenter)
		*InterfaceCenter = OverlayCenter;
*/
}

void View::StartTracking(lcTrackButton TrackButton)
{
	LC_ASSERT(mTrackButton == LC_TRACKBUTTON_NONE);

	mTrackButton = TrackButton;
	mMouseDownX = mInputState.x;
	mMouseDownY = mInputState.y;
	lcTool Tool = GetCurrentTool();

	switch (Tool)
	{
	case LC_TOOL_INSERT:
	case LC_TOOL_LIGHT:
		break;

	case LC_TOOL_SPOTLIGHT:
		{
			lcVector3 PositionTarget[2] =
			{
				lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f),
				lcVector3((float)mInputState.x + 1.0f, (float)mInputState.y - 1.0f, 0.9f)
			};

			UnprojectPoints(PositionTarget, 2);
			mModel->BeginSpotLightTool(PositionTarget[0], PositionTarget[1]);
		}
		break;

	case LC_TOOL_CAMERA:
		{
			lcVector3 PositionTarget[2] =
			{
				lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f),
				lcVector3((float)mInputState.x + 1.0f, (float)mInputState.y - 1.0f, 0.9f)
			};

			UnprojectPoints(PositionTarget, 2);
			mModel->BeginCameraTool(PositionTarget[0], PositionTarget[1]);
		}
		break;

	case LC_TOOL_SELECT:
		break;

	case LC_TOOL_MOVE:
	case LC_TOOL_ROTATE:
		mModel->BeginMouseTool();
		break;

	case LC_TOOL_ERASER:
	case LC_TOOL_PAINT:
		break;

	case LC_TOOL_ZOOM:
	case LC_TOOL_PAN:
	case LC_TOOL_ROTATE_VIEW:
	case LC_TOOL_ROLL:
		mModel->BeginMouseTool();
		break;

	case LC_TOOL_ZOOM_REGION:
		break;
	}

	OnUpdateCursor();
}

void View::StopTracking(bool Accept)
{
	if (mTrackButton == LC_TRACKBUTTON_NONE)
		return;

	lcTool Tool = GetCurrentTool();

	switch (Tool)
	{
	case LC_TOOL_INSERT:
	case LC_TOOL_LIGHT:
		break;

	case LC_TOOL_SPOTLIGHT:
	case LC_TOOL_CAMERA:
		mModel->EndMouseTool(Tool, Accept);
		break;

	case LC_TOOL_SELECT:
		if (Accept && mMouseDownX != mInputState.x && mMouseDownY != mInputState.y)
		{
			lcArray<lcObject*> Objects = FindObjectsInBox(mMouseDownX, mMouseDownY, mInputState.x, mInputState.y);

			if (mInputState.Control)
				mModel->AddToSelection(Objects);
			else
				mModel->SetSelectionAndFocus(Objects, NULL, 0);
		}
		break;

	case LC_TOOL_MOVE:
	case LC_TOOL_ROTATE:
		mModel->EndMouseTool(Tool, Accept);
		break;

	case LC_TOOL_ERASER:
	case LC_TOOL_PAINT:
		break;

	case LC_TOOL_ZOOM:
	case LC_TOOL_PAN:
	case LC_TOOL_ROTATE_VIEW:
	case LC_TOOL_ROLL:
		mModel->EndMouseTool(Tool, Accept);
		break;

	case LC_TOOL_ZOOM_REGION:
		{
			if (mInputState.x == mMouseDownX || mInputState.y == mMouseDownY)
				break;

			lcVector3 Points[6] =
			{
				lcVector3((mMouseDownX + lcMin(mInputState.x, mWidth - 1)) / 2, (mMouseDownY + lcMin(mInputState.y, mHeight - 1)) / 2, 0.0f),
				lcVector3((mMouseDownX + lcMin(mInputState.x, mWidth - 1)) / 2, (mMouseDownY + lcMin(mInputState.y, mHeight - 1)) / 2, 1.0f),
				lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f),
				lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f),
				lcVector3(mMouseDownX, mMouseDownY, 0.0f),
				lcVector3(mMouseDownX, mMouseDownY, 1.0f)
			};

			UnprojectPoints(Points, 5);

			lcVector3 Center = mModel->GetSelectionOrModelCenter();

			lcVector3 PlaneNormal(mCamera->mPosition - mCamera->mTargetPosition);
			lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
			lcVector3 Target, Corners[2];

			if (lcLinePlaneIntersection(&Target, Points[0], Points[1], Plane) && lcLinePlaneIntersection(&Corners[0], Points[2], Points[3], Plane) && lcLinePlaneIntersection(&Corners[1], Points[3], Points[4], Plane))
			{
				float AspectRatio = (float)mWidth / (float)mHeight;
				mModel->ZoomRegionToolClicked(mCamera, AspectRatio, Points[0], Target, Corners);
			}
		}
		break;
	}

	mTrackButton = LC_TRACKBUTTON_NONE;
	UpdateTrackTool();
	gMainWindow->UpdateAllViews();
}

void View::CancelTrackingOrClearSelection()
{
	if (mTrackButton != LC_TRACKBUTTON_NONE)
		StopTracking(false);
	else
		mModel->ClearSelection(true);
}

void View::OnLeftButtonDown()
{
	if (mTrackButton != LC_TRACKBUTTON_NONE)
	{
		StopTracking(false);
		return;
	}

	gMainWindow->SetActiveView(this);

	if (mInputState.Alt)
	{
		mTrackTool = LC_TRACKTOOL_ORBIT_XY;
		OnUpdateCursor();
	}
	else if (mTrackTool == LC_TRACKTOOL_MOVE_XYZ)
		mTrackTool = LC_TRACKTOOL_MOVE_XY;
	else if (mTrackTool == LC_TRACKTOOL_ROTATE_XYZ)
		mTrackTool = LC_TRACKTOOL_ROTATE_XY;

	switch (mTrackTool)
	{
	case LC_TRACKTOOL_NONE:
		break;

	case LC_TRACKTOOL_INSERT:
		{
			PieceInfo* CurPiece = gMainWindow->mPreviewWidget->GetCurrentPiece();

			if (!CurPiece)
				break;

			mModel->InsertPieceToolClicked(GetPieceInsertPosition());

			if (!mInputState.Control)
				gMainWindow->SetTool(LC_TOOL_SELECT);

			UpdateTrackTool();
		}
		break;

	case LC_TRACKTOOL_POINTLIGHT:
		{
			mModel->PointLightToolClicked(UnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f)));

			if (!mInputState.Control)
				gMainWindow->SetTool(LC_TOOL_SELECT);

			UpdateTrackTool();
		}
		break;

	case LC_TRACKTOOL_SPOTLIGHT:
	case LC_TRACKTOOL_CAMERA:
		StartTracking(LC_TRACKBUTTON_LEFT);
		break;
		
	case LC_TRACKTOOL_SELECT:
		{
			lcObjectSection ObjectSection = FindObjectUnderPointer(false);

			if (mInputState.Control)
				mModel->FocusOrDeselectObject(ObjectSection);
			else
				mModel->ClearSelectionAndSetFocus(ObjectSection);

			StartTracking(LC_TRACKBUTTON_LEFT);
		}
		break;

	case LC_TRACKTOOL_MOVE_X:
	case LC_TRACKTOOL_MOVE_Y:
	case LC_TRACKTOOL_MOVE_Z:
	case LC_TRACKTOOL_MOVE_XY:
	case LC_TRACKTOOL_MOVE_XZ:
	case LC_TRACKTOOL_MOVE_YZ:
	case LC_TRACKTOOL_MOVE_XYZ:
		if (mModel->AnyObjectsSelected())
			StartTracking(LC_TRACKBUTTON_LEFT);
		break;

	case LC_TRACKTOOL_ROTATE_X:
	case LC_TRACKTOOL_ROTATE_Y:
	case LC_TRACKTOOL_ROTATE_Z:
	case LC_TRACKTOOL_ROTATE_XY:
	case LC_TRACKTOOL_ROTATE_XYZ:
		if (mModel->AnyPiecesSelected())
			StartTracking(LC_TRACKBUTTON_LEFT);
		break;

	case LC_TRACKTOOL_ERASER:
		mModel->EraserToolClicked(FindObjectUnderPointer(false).Object);
		break;

	case LC_TRACKTOOL_PAINT:
		mModel->PaintToolClicked(FindObjectUnderPointer(true).Object);
		break;

	case LC_TRACKTOOL_ZOOM:
	case LC_TRACKTOOL_PAN:
	case LC_TRACKTOOL_ORBIT_X:
	case LC_TRACKTOOL_ORBIT_Y:
	case LC_TRACKTOOL_ORBIT_XY:
	case LC_TRACKTOOL_ROLL:
	case LC_TRACKTOOL_ZOOM_REGION:
		StartTracking(LC_TRACKBUTTON_LEFT);
		break;
	}
}

void View::OnLeftButtonUp()
{
	StopTracking(mTrackButton == LC_TRACKBUTTON_LEFT);
}

void View::OnLeftButtonDoubleClick()
{
	gMainWindow->SetActiveView(this);

	lcObjectSection ObjectSection = FindObjectUnderPointer(false);

	if (mInputState.Control)
		mModel->FocusOrDeselectObject(ObjectSection);
	else
		mModel->ClearSelectionAndSetFocus(ObjectSection);
}

void View::OnMiddleButtonDown()
{
	if (mTrackButton != LC_TRACKBUTTON_NONE)
	{
		StopTracking(false);
		return;
	}

	gMainWindow->SetActiveView(this);

	if (mInputState.Alt)
	{
		mTrackTool = LC_TRACKTOOL_PAN;
		OnUpdateCursor();
		StartTracking(LC_TRACKBUTTON_MIDDLE);
	}
}

void View::OnMiddleButtonUp()
{
	StopTracking(mTrackButton == LC_TRACKBUTTON_MIDDLE);
}

void View::OnRightButtonDown()
{
	if (mTrackButton != LC_TRACKBUTTON_NONE)
	{
		StopTracking(false);
		return;
	}

	gMainWindow->SetActiveView(this);

	if (mInputState.Alt)
	{
		mTrackTool = LC_TRACKTOOL_ZOOM;
		OnUpdateCursor();
	}
	else if (mTrackTool == LC_TRACKTOOL_MOVE_XYZ)
		mTrackTool = LC_TRACKTOOL_MOVE_Z;
	else if (mTrackTool == LC_TRACKTOOL_ROTATE_XYZ)
		mTrackTool = LC_TRACKTOOL_ROTATE_Z;

	switch (mTrackTool)
	{
	case LC_TRACKTOOL_NONE:
	case LC_TRACKTOOL_INSERT:
	case LC_TRACKTOOL_POINTLIGHT:
	case LC_TRACKTOOL_SPOTLIGHT:
	case LC_TRACKTOOL_CAMERA:
	case LC_TRACKTOOL_SELECT:
		break;

	case LC_TRACKTOOL_MOVE_X:
	case LC_TRACKTOOL_MOVE_Y:
	case LC_TRACKTOOL_MOVE_Z:
	case LC_TRACKTOOL_MOVE_XY:
	case LC_TRACKTOOL_MOVE_XZ:
	case LC_TRACKTOOL_MOVE_YZ:
	case LC_TRACKTOOL_MOVE_XYZ:
		if (mModel->AnyObjectsSelected())
			StartTracking(LC_TRACKBUTTON_RIGHT);
		break;

	case LC_TRACKTOOL_ROTATE_X:
	case LC_TRACKTOOL_ROTATE_Y:
	case LC_TRACKTOOL_ROTATE_Z:
	case LC_TRACKTOOL_ROTATE_XY:
	case LC_TRACKTOOL_ROTATE_XYZ:
		if (mModel->AnyPiecesSelected())
			StartTracking(LC_TRACKBUTTON_RIGHT);
		break;

	case LC_TRACKTOOL_ERASER:
	case LC_TRACKTOOL_PAINT:
		break;

	case LC_TRACKTOOL_ZOOM:
		StartTracking(LC_TRACKBUTTON_RIGHT);
		break;

	case LC_TRACKTOOL_PAN:
	case LC_TRACKTOOL_ORBIT_X:
	case LC_TRACKTOOL_ORBIT_Y:
	case LC_TRACKTOOL_ORBIT_XY:
	case LC_TRACKTOOL_ROLL:
	case LC_TRACKTOOL_ZOOM_REGION:
		break;
	}
}

void View::OnRightButtonUp()
{
	if (mTrackButton == LC_TRACKBUTTON_NONE)
		ShowPopupMenu();
	else
		StopTracking(mTrackButton == LC_TRACKBUTTON_RIGHT);
}

void View::OnMouseMove()
{
	if (mTrackButton == LC_TRACKBUTTON_NONE)
	{
		UpdateTrackTool();

		if (mTrackTool == LC_TRACKTOOL_INSERT)
			gMainWindow->UpdateAllViews();

		return;
	}

	const float MouseSensitivity = 1.0f / (21.0f - lcGetPreferences().mMouseSensitivity);

	switch (mTrackTool)
	{
	case LC_TRACKTOOL_NONE:
	case LC_TRACKTOOL_INSERT:
	case LC_TRACKTOOL_POINTLIGHT:
		break;

	case LC_TRACKTOOL_SPOTLIGHT:
		mModel->UpdateSpotLightTool(UnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f)));
		break;

	case LC_TRACKTOOL_CAMERA:
		mModel->UpdateCameraTool(UnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f)));
		break;

	case LC_TRACKTOOL_SELECT:
		Redraw();
		break;

	case LC_TRACKTOOL_MOVE_X:
	case LC_TRACKTOOL_MOVE_Y:
	case LC_TRACKTOOL_MOVE_Z:
	case LC_TRACKTOOL_MOVE_XY:
	case LC_TRACKTOOL_MOVE_XZ:
	case LC_TRACKTOOL_MOVE_YZ:
	case LC_TRACKTOOL_MOVE_XYZ:
		{
			lcVector3 Points[4] =
			{
				lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f),
				lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f),
				lcVector3(mMouseDownX, mMouseDownY, 0.0f),
				lcVector3(mMouseDownX, mMouseDownY, 1.0f)
			};

			UnprojectPoints(Points, 4);

			const lcVector3& CurrentStart = Points[0];
			const lcVector3& CurrentEnd = Points[1];
			const lcVector3& MouseDownStart = Points[2];
			const lcVector3& MouseDownEnd = Points[3];
			lcVector3 Center = mModel->GetFocusOrSelectionCenter();

			if (mTrackTool == LC_TRACKTOOL_MOVE_X || mTrackTool == LC_TRACKTOOL_MOVE_Y || mTrackTool == LC_TRACKTOOL_MOVE_Z)
			{
				lcVector3 Direction;
				if (mTrackTool == LC_TRACKTOOL_MOVE_X)
					Direction = lcVector3(1.0, 0.0f, 0.0f);
				else if (mTrackTool == LC_TRACKTOOL_MOVE_Y)
					Direction = lcVector3(0.0, 1.0f, 0.0f);
				else
					Direction = lcVector3(0.0, 0.0f, 1.0f);

				lcMatrix44 RelativeRotation = mModel->GetRelativeRotation();
				Direction = lcMul30(Direction, RelativeRotation);

				lcVector3 Intersection;
				lcClosestPointsBetweenLines(Center, Center + Direction, CurrentStart, CurrentEnd, &Intersection, NULL);

				lcVector3 MoveStart;
				lcClosestPointsBetweenLines(Center, Center + Direction, MouseDownStart, MouseDownEnd, &MoveStart, NULL);

				lcVector3 Distance = Intersection - MoveStart;
				Distance = lcMul30(Distance, lcMatrix44AffineInverse(RelativeRotation));
				mModel->UpdateMoveTool(Distance);
			}
			else if (mTrackTool == LC_TRACKTOOL_MOVE_XY || mTrackTool == LC_TRACKTOOL_MOVE_XZ || mTrackTool == LC_TRACKTOOL_MOVE_YZ)
			{
				lcVector3 PlaneNormal;

				if (mTrackTool == LC_TRACKTOOL_MOVE_XY)
					PlaneNormal = lcVector3(0.0f, 0.0f, 1.0f);
				else if (mTrackTool == LC_TRACKTOOL_MOVE_XZ)
					PlaneNormal = lcVector3(0.0f, 1.0f, 0.0f);
				else
					PlaneNormal = lcVector3(1.0f, 0.0f, 0.0f);

				lcMatrix44 RelativeRotation = mModel->GetRelativeRotation();
				PlaneNormal = lcMul30(PlaneNormal, RelativeRotation);
				lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
				lcVector3 Intersection;

				if (lcLinePlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane))
				{
					lcVector3 MoveStart;

					if (lcLinePlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
					{
						lcVector3 Distance = Intersection - MoveStart;
						Distance = lcMul30(Distance, lcMatrix44AffineInverse(RelativeRotation));
						mModel->UpdateMoveTool(Distance);
					}
				}
			}
			else
			{

				lcVector3 PlaneNormal = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);
				lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
				lcVector3 Intersection;

				if (lcLinePlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane))
				{
					lcVector3 MoveStart;

					if (lcLinePlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
					{
						lcVector3 Distance = Intersection - MoveStart;
						mModel->UpdateMoveTool(Distance);
					}
				}
			}
		}
		break;

	case LC_TRACKTOOL_ROTATE_X:
	case LC_TRACKTOOL_ROTATE_Y:
	case LC_TRACKTOOL_ROTATE_Z:
		{
			lcVector3 ScreenX = lcNormalize(lcCross(mCamera->mTargetPosition - mCamera->mPosition, mCamera->mUpVector));
			lcVector3 ScreenY = mCamera->mUpVector;
			lcVector3 Dir1;

			switch (mTrackTool)
			{
			case LC_TRACKTOOL_ROTATE_X:
				Dir1 = lcVector3(1, 0, 0);
				break;
			case LC_TRACKTOOL_ROTATE_Y:
				Dir1 = lcVector3(0, 1, 0);
				break;
			case LC_TRACKTOOL_ROTATE_Z:
				Dir1 = lcVector3(0, 0, 1);
				break;
			default:
				break;
			}

			lcVector3 MoveX, MoveY;

			float dx1 = lcDot(ScreenX, Dir1);
			float dy1 = lcDot(ScreenY, Dir1);

			if (fabsf(dx1) > fabsf(dy1))
			{
				if (dx1 >= 0.0f)
					MoveX = Dir1;
				else
					MoveX = -Dir1;

				MoveY = lcVector3(0, 0, 0);
			}
			else
			{
				MoveX = lcVector3(0, 0, 0);

				if (dy1 > 0.0f)
					MoveY = Dir1;
				else
					MoveY = -Dir1;
			}

			MoveX *= 36.0f * (float)(mInputState.x - mMouseDownX) * MouseSensitivity;
			MoveY *= 36.0f * (float)(mInputState.y - mMouseDownY) * MouseSensitivity;

			mModel->UpdateRotateTool(MoveX + MoveY);
		}
		break;

	case LC_TRACKTOOL_ROTATE_XY:
		{
			lcVector3 ScreenZ = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);
			lcVector3 ScreenX = lcCross(ScreenZ, mCamera->mUpVector);
			lcVector3 ScreenY = mCamera->mUpVector;

			lcVector3 MoveX = 36.0f * (float)(mInputState.x - mMouseDownX) * MouseSensitivity * ScreenX;
			lcVector3 MoveY = 36.0f * (float)(mInputState.y - mMouseDownY) * MouseSensitivity * ScreenY;
			mModel->UpdateRotateTool(MoveX + MoveY);
		}
		break;

	case LC_TRACKTOOL_ROTATE_XYZ:
		{
			lcVector3 ScreenZ = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);

			mModel->UpdateRotateTool(36.0f * (float)(mInputState.y - mMouseDownY) * MouseSensitivity * ScreenZ);
		}
		break;

	case LC_TRACKTOOL_ERASER:
	case LC_TRACKTOOL_PAINT:
		break;

	case LC_TRACKTOOL_ZOOM:
		mModel->UpdateZoomTool(mCamera, 2.0f * MouseSensitivity * (mInputState.y - mMouseDownY));
		break;

	case LC_TRACKTOOL_PAN:
		{
			lcVector3 Points[4] =
			{
				lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f),
				lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f),
				lcVector3(mMouseDownX, mMouseDownY, 0.0f),
				lcVector3(mMouseDownX, mMouseDownY, 1.0f)
			};

			UnprojectPoints(Points, 4);

			const lcVector3& CurrentStart = Points[0];
			const lcVector3& CurrentEnd = Points[1];
			const lcVector3& MouseDownStart = Points[2];
			const lcVector3& MouseDownEnd = Points[3];
			lcVector3 Center = mModel->GetSelectionOrModelCenter();

			lcVector3 PlaneNormal(mCamera->mPosition - mCamera->mTargetPosition);
			lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
			lcVector3 Intersection;

			if (lcLinePlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane))
			{
				lcVector3 MoveStart;

				if (lcLinePlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
					mModel->UpdatePanTool(mCamera, MoveStart - Intersection);
			}
		}
		break;

	case LC_TRACKTOOL_ORBIT_X:
		mModel->UpdateOrbitTool(mCamera, 0.1f * MouseSensitivity * (mInputState.x - mMouseDownX), 0.0f);
		break;

	case LC_TRACKTOOL_ORBIT_Y:
		mModel->UpdateOrbitTool(mCamera, 0.0f, 0.1f * MouseSensitivity * (mInputState.y - mMouseDownY));
		break;

	case LC_TRACKTOOL_ORBIT_XY:
		mModel->UpdateOrbitTool(mCamera, 0.1f * MouseSensitivity * (mInputState.x - mMouseDownX), 0.1f * MouseSensitivity * (mInputState.y - mMouseDownY));
		break;

	case LC_TRACKTOOL_ROLL:
		mModel->UpdateRollTool(mCamera, 2.0f * MouseSensitivity * (mInputState.x - mMouseDownX) * LC_DTOR);
		break;

	case LC_TRACKTOOL_ZOOM_REGION:
		Redraw();
		break;
	}
}

void View::OnMouseWheel(float Direction)
{
	mModel->Zoom(mCamera, (int)((mInputState.Control ? 100 : 10) * Direction));
}

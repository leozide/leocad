#include "lc_global.h"
#include <stdlib.h>
#include "lc_mainwindow.h"
#include "camera.h"
#include "view.h"
#include "texfont.h"
#include "lc_texture.h"
#include "piece.h"
#include "pieceinf.h"
#include "lc_synth.h"

lcVertexBuffer View::mRotateMoveVertexBuffer;
lcIndexBuffer View::mRotateMoveIndexBuffer;

View::View(lcModel* Model)
	: mViewSphere(this)
{
	mModel = Model;
	mActiveSubmodelInstance = nullptr;
	mCamera = nullptr;
	mHighlight = false;
	memset(mGridSettings, 0, sizeof(mGridSettings));

	mDragState = lcDragState::NONE;
	mTrackButton = LC_TRACKBUTTON_NONE;
	mTrackTool = LC_TRACKTOOL_NONE;
	mTrackToolFromOverlay = false;

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

	if (mCamera && mCamera->IsSimple())
		delete mCamera;
}

lcModel* View::GetActiveModel() const
{
	return !mActiveSubmodelInstance ? mModel : mActiveSubmodelInstance->mPieceInfo->GetModel();
}

void View::SetTopSubmodelActive()
{
	lcModel* ActiveModel = GetActiveModel();

	if (mActiveSubmodelInstance)
	{
		ActiveModel->SetActive(false);
		mActiveSubmodelInstance = nullptr;
	}

	GetActiveModel()->UpdateInterface();
}

void View::SetSelectedSubmodelActive()
{
	lcModel* ActiveModel = GetActiveModel();
	lcObject* Object = ActiveModel->GetFocusObject();

	if (mActiveSubmodelInstance)
	{
		ActiveModel->SetActive(false);
		mActiveSubmodelInstance = nullptr;
	}

	if (Object && Object->IsPiece())
	{
		lcPiece* Piece = (lcPiece*)Object;

		if (Piece->mPieceInfo->IsModel())
		{
			mActiveSubmodelTransform = lcMatrix44Identity();
			mModel->GetPieceWorldMatrix(Piece, mActiveSubmodelTransform);
			mActiveSubmodelInstance = Piece;
			ActiveModel = mActiveSubmodelInstance->mPieceInfo->GetModel();
			ActiveModel->SetActive(true);
			RemoveCamera();
		}
	}

	GetActiveModel()->UpdateInterface();
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
	gGridTexture = nullptr;

	Context->DestroyVertexBuffer(mRotateMoveVertexBuffer);
	Context->DestroyIndexBuffer(mRotateMoveIndexBuffer);
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
}

void View::SetCamera(const char* CameraName)
{
	const lcArray<lcCamera*>& Cameras = mModel->GetCameras();

	for (int CameraIdx = 0; CameraIdx < Cameras.GetSize(); CameraIdx++)
	{
		if (qstricmp(CameraName, Cameras[CameraIdx]->m_strName) == 0)
		{
			SetCameraIndex(CameraIdx);
			return;
		}
	}
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
		lcCamera* OldCamera = mCamera;

		mCamera = new lcCamera(true);

		if (OldCamera)
			mCamera->CopySettings(OldCamera);
	}

	mCamera->SetViewpoint(Viewpoint);
	ZoomExtents();
	Redraw();

	gMainWindow->UpdateCurrentCamera(-1);
}

void View::SetViewpoint(const lcVector3& Position)
{
	if (!mCamera || !mCamera->IsSimple())
	{
		lcCamera* OldCamera = mCamera;

		mCamera = new lcCamera(true);

		if (OldCamera)
			mCamera->CopySettings(OldCamera);
	}

	mCamera->SetViewpoint(Position);
	ZoomExtents();
	Redraw();

	gMainWindow->UpdateCurrentCamera(-1);
}

void View::SetCameraAngles(float Latitude, float Longitude)
{
	if (!mCamera || !mCamera->IsSimple())
	{
		lcCamera* OldCamera = mCamera;

		mCamera = new lcCamera(true);

		if (OldCamera)
			mCamera->CopySettings(OldCamera);
	}

	mCamera->SetAngles(Latitude, Longitude, 1.0f);
	ZoomExtents();
	Redraw();
}

void View::SetDefaultCamera()
{
	if (!mCamera || !mCamera->IsSimple())
		mCamera = new lcCamera(true);

	mCamera->SetViewpoint(LC_VIEWPOINT_HOME);

	gMainWindow->UpdateCurrentCamera(-1);
}

lcMatrix44 View::GetProjectionMatrix() const
{
	float AspectRatio = (float)mWidth / (float)mHeight;

	if (mCamera->IsOrtho())
	{
		float OrthoHeight = mCamera->GetOrthoHeight() / 2.0f;
		float OrthoWidth = OrthoHeight * AspectRatio;

		return lcMatrix44Ortho(-OrthoWidth, OrthoWidth, -OrthoHeight, OrthoHeight, mCamera->m_zNear, mCamera->m_zFar * 4);
	}
	else
		return lcMatrix44Perspective(mCamera->m_fovy, AspectRatio, mCamera->m_zNear, mCamera->m_zFar);
}

lcMatrix44 View::GetTileProjectionMatrix(int CurrentRow, int CurrentColumn, int CurrentTileWidth, int CurrentTileHeight) const
{
	int ImageWidth = mRenderImage.width();
	int ImageHeight = mRenderImage.height();

	double ImageLeft, ImageRight, ImageBottom, ImageTop, Near, Far;
	double AspectRatio = (double)ImageWidth / (double)ImageHeight;

	if (mCamera->IsOrtho())
	{
		float OrthoHeight = mCamera->GetOrthoHeight() / 2.0f;
		float OrthoWidth = OrthoHeight * AspectRatio;

		ImageLeft = -OrthoWidth;
		ImageRight = OrthoWidth;
		ImageBottom = -OrthoHeight;
		ImageTop = OrthoHeight;
		Near = mCamera->m_zNear;
		Far = mCamera->m_zFar * 4;
	}
	else
	{
		double xmin, xmax, ymin, ymax;
		ymax = mCamera->m_zNear * tan(mCamera->m_fovy * 3.14159265 / 360.0);
		ymin = -ymax;
		xmin = ymin * AspectRatio;
		xmax = ymax * AspectRatio;

		ImageLeft = xmin;
		ImageRight = xmax;
		ImageBottom = ymin;
		ImageTop = ymax;
		Near = mCamera->m_zNear;
		Far = mCamera->m_zFar;
	}

	double Left = ImageLeft + (ImageRight - ImageLeft) * (CurrentColumn * mWidth) / ImageWidth;
	double Right = Left + (ImageRight - ImageLeft) * CurrentTileWidth / ImageWidth;
	double Bottom = ImageBottom + (ImageTop - ImageBottom) * (CurrentRow * mHeight) / ImageHeight;
	double Top = Bottom + (ImageTop - ImageBottom) * CurrentTileHeight / ImageHeight;

	if (mCamera->IsOrtho())
		return lcMatrix44Ortho(Left, Right, Bottom, Top, Near, Far);
	else
		return lcMatrix44Frustum(Left, Right, Bottom, Top, Near, Far);
}

LC_CURSOR_TYPE View::GetCursor() const
{
	if (mTrackTool == LC_TRACKTOOL_SELECT)
	{
		if (mInputState.Modifiers & Qt::ControlModifier)
			return LC_CURSOR_SELECT_ADD;

		if (mInputState.Modifiers & Qt::ShiftModifier)
			return LC_CURSOR_SELECT_REMOVE;
	}

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
		LC_CURSOR_MOVE,        // LC_TRACKTOOL_SCALE_PLUS
		LC_CURSOR_MOVE,        // LC_TRACKTOOL_SCALE_MINUS
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

void View::ShowContextMenu() const
{
	QGLWidget* Widget = (QGLWidget*)mWidget;
	QAction** Actions = gMainWindow->mActions;

	QMenu* Popup = new QMenu(Widget);

	Popup->addAction(Actions[LC_EDIT_CUT]);
	Popup->addAction(Actions[LC_EDIT_COPY]);
	Popup->addAction(Actions[LC_EDIT_PASTE]);
	Popup->addAction(Actions[LC_PIECE_DUPLICATE]);

	Popup->addSeparator();

	Popup->addAction(Actions[LC_PIECE_CONTROL_POINT_INSERT]);
	Popup->addAction(Actions[LC_PIECE_CONTROL_POINT_REMOVE]);

	Popup->addSeparator();

	Popup->addAction(Actions[LC_PIECE_EDIT_SELECTED_SUBMODEL]);
	Popup->addAction(Actions[LC_PIECE_EDIT_END_SUBMODEL]);
	Popup->addAction(Actions[LC_PIECE_VIEW_SELECTED_MODEL]);
	Popup->addAction(Actions[LC_PIECE_INLINE_SELECTED_MODELS]);
	Popup->addAction(Actions[LC_PIECE_MOVE_SELECTION_TO_MODEL]);

	Popup->addSeparator();

	Popup->addMenu(gMainWindow->GetToolsMenu());
	Popup->addMenu(gMainWindow->GetViewpointMenu());
	Popup->addMenu(gMainWindow->GetCameraMenu());
	Popup->addMenu(gMainWindow->GetProjectionMenu());
	Popup->addMenu(gMainWindow->GetShadingMenu());

	Popup->addSeparator();

	Popup->addAction(Actions[LC_VIEW_SPLIT_HORIZONTAL]);
	Popup->addAction(Actions[LC_VIEW_SPLIT_VERTICAL]);
	Popup->addAction(Actions[LC_VIEW_REMOVE_VIEW]);
	Popup->addAction(Actions[LC_VIEW_RESET_VIEWS]);

	Popup->exec(QCursor::pos());
	delete Popup;
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

lcMatrix44 View::GetPieceInsertPosition(bool IgnoreSelected, PieceInfo* Info) const
{
	lcPiece* HitPiece = (lcPiece*)FindObjectUnderPointer(true, IgnoreSelected).Object;
	lcModel* ActiveModel = GetActiveModel();

	if (HitPiece)
	{
		lcVector3 Position(0, 0, HitPiece->GetBoundingBox().Max.z - Info->GetBoundingBox().Min.z);

		if (gMainWindow->GetRelativeTransform())
			Position = lcMul31(ActiveModel->SnapPosition(Position), HitPiece->mModelWorld);
		else
			Position = ActiveModel->SnapPosition(lcMul31(Position, HitPiece->mModelWorld));

		lcMatrix44 WorldMatrix = HitPiece->mModelWorld;
		WorldMatrix.SetTranslation(Position);

		return WorldMatrix;
	}

	std::array<lcVector3, 2> ClickPoints = {{ lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f), lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f) }};
	UnprojectPoints(ClickPoints.data(), 2);

	if (ActiveModel != mModel)
	{
		lcMatrix44 InverseMatrix = lcMatrix44AffineInverse(mActiveSubmodelTransform);

		for (lcVector3& Point : ClickPoints)
			Point = lcMul31(Point, InverseMatrix);
	}

	lcVector3 Intersection;

	const lcBoundingBox& BoundingBox = Info->GetBoundingBox();
	if (lcLineSegmentPlaneIntersection(&Intersection, ClickPoints[0], ClickPoints[1], lcVector4(0, 0, 1, BoundingBox.Min.z)))
	{
		Intersection = ActiveModel->SnapPosition(Intersection);
		return lcMatrix44Translation(Intersection);
	}

	return lcMatrix44Translation(UnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f)));
}

lcVector3 View::GetCameraLightInsertPosition() const
{
	lcModel* ActiveModel = GetActiveModel();

	std::array<lcVector3, 2> ClickPoints = { { lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f), lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f) } };
	UnprojectPoints(ClickPoints.data(), 2);

	if (ActiveModel != mModel)
	{
		lcMatrix44 InverseMatrix = lcMatrix44AffineInverse(mActiveSubmodelTransform);

		for (lcVector3& Point : ClickPoints)
			Point = lcMul31(Point, InverseMatrix);
	}

	lcVector3 Min, Max;
	lcVector3 Center;

	if (ActiveModel->GetPiecesBoundingBox(Min, Max))
		Center = (Min + Max) / 2.0f;
	else
		Center = lcVector3(0.0f, 0.0f, 0.0f);

	return lcRayPointClosestPoint(Center, ClickPoints[0], ClickPoints[1]);
}

void View::GetRayUnderPointer(lcVector3& Start, lcVector3& End) const
{
	lcVector3 StartEnd[2] =
	{
		lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f),
		lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f)
	};

	UnprojectPoints(StartEnd, 2);

	Start = StartEnd[0];
	End = StartEnd[1];
}

lcObjectSection View::FindObjectUnderPointer(bool PiecesOnly, bool IgnoreSelected) const
{
	lcVector3 StartEnd[2] =
	{
		lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f),
		lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f)
	};

	UnprojectPoints(StartEnd, 2);

	lcObjectRayTest ObjectRayTest;

	ObjectRayTest.PiecesOnly = PiecesOnly;
	ObjectRayTest.IgnoreSelected = IgnoreSelected;
	ObjectRayTest.ViewCamera = mCamera;
	ObjectRayTest.Start = StartEnd[0];
	ObjectRayTest.End = StartEnd[1];
	ObjectRayTest.Distance = FLT_MAX;
	ObjectRayTest.ObjectSection.Object = nullptr;
	ObjectRayTest.ObjectSection.Section = 0;;

	lcModel* ActiveModel = GetActiveModel();

	if (ActiveModel != mModel)
	{
		lcMatrix44 InverseMatrix = lcMatrix44AffineInverse(mActiveSubmodelTransform);

		ObjectRayTest.Start = lcMul31(ObjectRayTest.Start, InverseMatrix);
		ObjectRayTest.End = lcMul31(ObjectRayTest.End, InverseMatrix);
	}

	ActiveModel->RayTest(ObjectRayTest);

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

	std::array<lcVector3, 6> Corners =
	{{
		lcVector3(Left, Top, 0),
		lcVector3(Left, Bottom, 0),
		lcVector3(Right, Bottom, 0),
		lcVector3(Right, Top, 0),
		lcVector3(Left, Top, 1),
		lcVector3(Right, Bottom, 1)
	}};

	UnprojectPoints(Corners.data(), (int)Corners.size());

	lcModel* ActiveModel = GetActiveModel();

	if (ActiveModel != mModel)
	{
		lcMatrix44 InverseMatrix = lcMatrix44AffineInverse(mActiveSubmodelTransform);

		for (lcVector3& Point : Corners)
			Point = lcMul31(Point, InverseMatrix);
	}

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

	ActiveModel->BoxTest(ObjectBoxTest);

	return ObjectBoxTest.Objects;
}

bool View::BeginRenderToImage(int Width, int Height)
{
	GLint MaxTexture;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTexture);

	MaxTexture = qMin(MaxTexture, 2048);
	MaxTexture /= QGLFormat::defaultFormat().sampleBuffers() ? QGLFormat::defaultFormat().samples() : 1;

	int TileWidth = qMin(Width, MaxTexture);
	int TileHeight = qMin(Height, MaxTexture);

	mWidth = TileWidth;
	mHeight = TileHeight;
	mRenderImage = QImage(Width, Height, QImage::Format_ARGB32);

	mRenderFramebuffer = mContext->CreateRenderFramebuffer(TileWidth, TileHeight);
	mContext->BindFramebuffer(mRenderFramebuffer.first);
	return mRenderFramebuffer.first.IsValid();
}

void View::EndRenderToImage()
{
	mRenderImage = QImage();
	mContext->DestroyRenderFramebuffer(mRenderFramebuffer);
	mContext->ClearFramebuffer();
}

void View::OnDraw()
{
	if (!mModel)
		return;

	const lcPreferences& Preferences = lcGetPreferences();

	bool DrawInterface = mWidget != nullptr;
	mScene.SetAllowLOD(Preferences.mAllowLOD && mWidget != nullptr);

	mModel->GetScene(mScene, mCamera, DrawInterface, mHighlight, mActiveSubmodelInstance, mActiveSubmodelTransform);

	if (DrawInterface && mTrackTool == LC_TRACKTOOL_INSERT)
	{
		PieceInfo* Info = gMainWindow->GetCurrentPieceInfo();

		if (Info)
		{
			lcMatrix44 WorldMatrix = GetPieceInsertPosition(false, gMainWindow->GetCurrentPieceInfo());

			if (GetActiveModel() != mModel)
				WorldMatrix = lcMul(WorldMatrix, mActiveSubmodelTransform);

			Info->AddRenderMeshes(mScene, WorldMatrix, gMainWindow->mColorIndex, lcRenderMeshState::FOCUSED, true);
		}
	}

	int TotalTileRows = 1;
	int TotalTileColumns = 1;

	if (!mRenderImage.isNull())
	{
		int ImageWidth = mRenderImage.width();
		int ImageHeight = mRenderImage.height();

		if (ImageWidth > mWidth || ImageHeight > mHeight)
		{
			TotalTileColumns = (mWidth + ImageWidth - 1) / mWidth;
			TotalTileRows = (mHeight + ImageHeight - 1) / mHeight;
		}
	}

	for (int CurrentTileRow = 0; CurrentTileRow < TotalTileRows; CurrentTileRow++)
	{
		for (int CurrentTileColumn = 0; CurrentTileColumn < TotalTileColumns; CurrentTileColumn++)
		{
			mContext->SetDefaultState();
			mContext->SetViewport(0, 0, mWidth, mHeight);

			mModel->DrawBackground(this);

			int CurrentTileWidth, CurrentTileHeight;

			if (!mRenderImage.isNull() && (TotalTileRows > 1 || TotalTileColumns > 1))
			{
				if (CurrentTileRow < TotalTileRows - 1)
					CurrentTileHeight = mHeight;
				else
					CurrentTileHeight = mRenderImage.height() - (TotalTileRows - 1) * (mHeight);

				if (CurrentTileColumn < TotalTileColumns - 1)
					CurrentTileWidth = mWidth;
				else
					CurrentTileWidth = mRenderImage.width() - (TotalTileColumns - 1) * (mWidth);

				mContext->SetViewport(0, 0, CurrentTileWidth, CurrentTileHeight);
				mContext->SetProjectionMatrix(GetTileProjectionMatrix(CurrentTileRow, CurrentTileColumn, CurrentTileWidth, CurrentTileHeight));
			}
			else
			{
				CurrentTileWidth = mWidth;
				CurrentTileHeight = mHeight;

				mContext->SetProjectionMatrix(GetProjectionMatrix());
			}

			if (DrawInterface)
			{
				mContext->SetViewMatrix(mScene.GetViewMatrix());
				DrawGrid();
			}

			mContext->SetLineWidth(Preferences.mLineWidth);

			mScene.Draw(mContext);

			if (!mRenderImage.isNull())
			{
				quint8* Buffer = (quint8*)malloc(mWidth * mHeight * 4);
				uchar* ImageBuffer = mRenderImage.bits();

				mContext->GetRenderFramebufferImage(mRenderFramebuffer, Buffer);

				quint32 TileY = 0, SrcY = 0;
				if (CurrentTileRow != TotalTileRows - 1)
					TileY = (TotalTileRows - CurrentTileRow - 1) * mHeight - ((mHeight - mRenderImage.height() % mHeight) % mHeight);
				else if (TotalTileRows > 1)
					SrcY = (mHeight - mRenderImage.height() % mHeight) % mHeight;

				quint32 TileStart = ((CurrentTileColumn * mWidth) + (TileY * mRenderImage.width())) * 4;

				for (int y = 0; y < CurrentTileHeight; y++)
				{
					quint8* src = Buffer + (SrcY + y) * mWidth * 4;
					quint8* dst = ImageBuffer + TileStart + y * mRenderImage.width() * 4;

					memcpy(dst, src, CurrentTileWidth * 4);
				}

				free(Buffer);
			}
		}
	}

	if (DrawInterface)
	{
		mScene.DrawInterfaceObjects(mContext);

		mContext->SetLineWidth(1.0f);

		if (Preferences.mDrawAxes)
			DrawAxes();

		lcTool Tool = gMainWindow->GetTool();
		lcModel* ActiveModel = GetActiveModel();

		if ((Tool == LC_TOOL_SELECT || Tool == LC_TOOL_MOVE) && mTrackButton == LC_TRACKBUTTON_NONE && ActiveModel->AnyObjectsSelected())
			DrawSelectMoveOverlay();
		else if (GetCurrentTool() == LC_TOOL_MOVE && mTrackButton != LC_TRACKBUTTON_NONE)
			DrawSelectMoveOverlay();
		else if ((Tool == LC_TOOL_ROTATE || (Tool == LC_TOOL_SELECT && mTrackButton != LC_TRACKBUTTON_NONE && mTrackTool >= LC_TRACKTOOL_ROTATE_X && mTrackTool <= LC_TRACKTOOL_ROTATE_XYZ)) && ActiveModel->AnyPiecesSelected())
			DrawRotateOverlay();
		else if ((mTrackTool == LC_TRACKTOOL_SELECT || mTrackTool == LC_TRACKTOOL_ZOOM_REGION) && mTrackButton == LC_TRACKBUTTON_LEFT)
			DrawSelectZoomRegionOverlay();
		else if (Tool == LC_TOOL_ROTATE_VIEW && mTrackButton == LC_TRACKBUTTON_NONE)
			DrawRotateViewOverlay();

		mViewSphere.Draw();
		DrawViewport();
	}

	mContext->ClearResources();
}

void View::DrawSelectMoveOverlay()
{
	mContext->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
	mContext->SetViewMatrix(mCamera->mWorldView);
	mContext->SetProjectionMatrix(GetProjectionMatrix());

	glDisable(GL_DEPTH_TEST);

	lcVector3 OverlayCenter;
	lcMatrix33 RelativeRotation;
	lcModel* ActiveModel = GetActiveModel();
	ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation);
	bool AnyPiecesSelected = ActiveModel->AnyPiecesSelected();

	lcMatrix44 WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);

	if (ActiveModel != mModel)
		WorldMatrix = lcMul(WorldMatrix, mActiveSubmodelTransform);

	const float OverlayScale = GetOverlayScale();
	WorldMatrix = lcMul(lcMatrix44Scale(lcVector3(OverlayScale, OverlayScale, OverlayScale)), WorldMatrix);

	mContext->SetWorldMatrix(WorldMatrix);

	mContext->SetIndexBuffer(mRotateMoveIndexBuffer);
	mContext->SetVertexBuffer(mRotateMoveVertexBuffer);
	mContext->SetVertexFormatPosition(3);

	lcObject* Focus = ActiveModel->GetFocusObject();
	quint32 AllowedTransforms = Focus ? Focus->GetAllowedTransforms() : LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z | LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z;

	if (mTrackButton == LC_TRACKBUTTON_NONE || (mTrackTool >= LC_TRACKTOOL_MOVE_X && mTrackTool <= LC_TRACKTOOL_MOVE_XYZ))
	{
		if (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_X)
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
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Y)
		{
			if (((mTrackTool == LC_TRACKTOOL_MOVE_Y) || (mTrackTool == LC_TRACKTOOL_MOVE_XY) || (mTrackTool == LC_TRACKTOOL_MOVE_YZ)) && (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Y))
			{
				mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
				mContext->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 36 * 2);
			}
			else if (mTrackButton == LC_TRACKBUTTON_NONE)
			{
				mContext->SetColor(0.0f, 0.8f, 0.0f, 1.0f);
				mContext->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 36 * 2);
			}
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Z)
		{
			if (((mTrackTool == LC_TRACKTOOL_MOVE_Z) || (mTrackTool == LC_TRACKTOOL_MOVE_XZ) || (mTrackTool == LC_TRACKTOOL_MOVE_YZ)) && (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Z))
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
	}

	if (gMainWindow->GetTool() == LC_TOOL_SELECT && mTrackButton == LC_TRACKBUTTON_NONE && AnyPiecesSelected)
	{
		if (AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_X)
		{
			if (mTrackTool == LC_TRACKTOOL_ROTATE_X)
				mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			else
				mContext->SetColor(0.8f, 0.0f, 0.0f, 1.0f);

			mContext->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, 108 * 2);
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_Y)
		{
			if (mTrackTool == LC_TRACKTOOL_ROTATE_Y)
				mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			else
				mContext->SetColor(0.0f, 0.8f, 0.0f, 1.0f);

			mContext->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, (108 + 120) * 2);
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_Z)
		{
			if (mTrackTool == LC_TRACKTOOL_ROTATE_Z)
				mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			else
				mContext->SetColor(0.0f, 0.0f, 0.8f, 1.0f);

			mContext->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, (108 + 240) * 2);
		}
	}

	if ((mTrackTool == LC_TRACKTOOL_MOVE_XY) || (mTrackTool == LC_TRACKTOOL_MOVE_XZ) || (mTrackTool == LC_TRACKTOOL_MOVE_YZ))
	{
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

	if (Focus && Focus->IsPiece())
	{
		lcPiece* Piece = (lcPiece*)Focus;
		quint32 Section = Piece->GetFocusSection();

		if (Section >= LC_PIECE_SECTION_CONTROL_POINT_1 && Section <= LC_PIECE_SECTION_CONTROL_POINT_8 && Piece->mPieceInfo->GetSynthInfo() && Piece->mPieceInfo->GetSynthInfo()->IsCurve())
		{
			int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_1;
			float Strength = Piece->GetControlPoints()[ControlPointIndex].Scale;
			const float ScaleStart = 2.0f;
			float Length = ScaleStart + Strength / OverlayScale;
			const float OverlayScaleInnerRadius = 0.075f;
			const float OverlayScaleRadius = 0.125f;

			lcVector3 Verts[38];
			int NumVerts = 0;

			Verts[NumVerts++] = lcVector3(Length - OverlayScaleRadius, 0.0f, 0.0f);
			Verts[NumVerts++] = lcVector3(OverlayScaleRadius - Length, 0.0f, 0.0f);

			float SinTable[9], CosTable[9];

			for (int Step = 0; Step <= 8; Step++)
			{
				SinTable[Step] = sinf((float)Step / 8.0f * LC_2PI);
				CosTable[Step] = cosf((float)Step / 8.0f * LC_2PI);
			}

			for (int Step = 0; Step <= 8; Step++)
			{
				float x = CosTable[Step];
				float y = SinTable[Step];

				Verts[NumVerts++] = lcVector3(Length + x * OverlayScaleInnerRadius, 0.0f, y * OverlayScaleInnerRadius);
				Verts[NumVerts++] = lcVector3(Length + x * OverlayScaleRadius, 0.0f, y * OverlayScaleRadius);
			}

			for (int Step = 0; Step <= 8; Step++)
			{
				float x = CosTable[Step];
				float y = SinTable[Step];

				Verts[NumVerts++] = lcVector3(-Length + x * OverlayScaleInnerRadius, 0.0f, y * OverlayScaleInnerRadius);
				Verts[NumVerts++] = lcVector3(-Length + x * OverlayScaleRadius, 0.0f, y * OverlayScaleRadius);
			}

			if (mTrackTool == LC_TRACKTOOL_SCALE_PLUS || mTrackTool == LC_TRACKTOOL_SCALE_MINUS)
				mContext->SetColor(0.8f, 0.8f, 0.0f, 0.3f);
			else
				mContext->SetColor(0.0f, 0.0f, 0.8f, 1.0f);

			mContext->SetVertexBufferPointer(Verts);
			mContext->ClearIndexBuffer();
			mContext->SetVertexFormatPosition(3);

			mContext->DrawPrimitives(GL_LINES, 0, 2);
			mContext->DrawPrimitives(GL_TRIANGLE_STRIP, 2, 18);
			mContext->DrawPrimitives(GL_TRIANGLE_STRIP, 20, 18);
		}
	}

	glEnable(GL_DEPTH_TEST);
}

void View::DrawRotateOverlay()
{
	const float OverlayScale = GetOverlayScale();
	const float OverlayRotateRadius = 2.0f;

	mContext->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
	mContext->SetViewMatrix(mCamera->mWorldView);
	mContext->SetProjectionMatrix(GetProjectionMatrix());

	glDisable(GL_DEPTH_TEST);

	int j;

	lcVector3 OverlayCenter;
	lcMatrix33 RelativeRotation;
	lcModel* ActiveModel = GetActiveModel();
	ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation);
	lcVector3 MouseToolDistance = ActiveModel->SnapRotation(ActiveModel->GetMouseToolDistance());
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
			lcMatrix44 WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);
			WorldMatrix = lcMul(lcMatrix44FromAxisAngle(lcVector3(Rotation[1], Rotation[2], Rotation[3]), Rotation[0] * LC_DTOR), WorldMatrix);

			mContext->SetWorldMatrix(WorldMatrix);

			glEnable(GL_BLEND);

			lcVector3 Verts[33];
			Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);
			int NumVerts = 1;

			mContext->SetVertexBufferPointer(Verts);
			mContext->SetVertexFormatPosition(3);

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
					mContext->DrawPrimitives(GL_TRIANGLE_FAN, 0, NumVerts);
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
				mContext->DrawPrimitives(GL_TRIANGLE_FAN, 0, NumVerts);

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

		mContext->SetVertexBufferPointer(Verts);
		mContext->SetVertexFormatPosition(3);

		mContext->DrawPrimitives(GL_LINE_LOOP, 0, 32);
	}

	lcVector3 ViewDir = mCamera->mTargetPosition - mCamera->mPosition;
	ViewDir.Normalize();

	// Transform ViewDir to local space.
	ViewDir = lcMul(ViewDir, lcMatrix33AffineInverse(RelativeRotation));

	mContext->SetWorldMatrix(lcMatrix44(RelativeRotation, OverlayCenter));

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

		mContext->SetVertexBufferPointer(Verts);
		mContext->SetVertexFormatPosition(3);

		mContext->DrawPrimitives(GL_LINES, 0, NumVerts);
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

		lcMatrix44 WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);
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

			mContext->SetVertexBufferPointer(Verts);
			mContext->SetVertexFormatPosition(3);

			mContext->DrawPrimitives(GL_LINES, 0, 6);
		}

		// Draw text.
		lcVector3 ScreenPos = ProjectPoint(OverlayCenter);

		mContext->SetMaterial(LC_MATERIAL_UNLIT_TEXTURE_MODULATE);
		mContext->SetWorldMatrix(lcMatrix44Identity());
		mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
		mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));
		mContext->BindTexture2D(gTexFont.GetTexture());
		glEnable(GL_BLEND);

		char buf[32];
		sprintf(buf, "[%.2f]", fabsf(Angle));

		int cx, cy;
		gTexFont.GetStringDimensions(&cx, &cy, buf);

		mContext->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
		gTexFont.PrintText(mContext, ScreenPos[0] - (cx / 2), ScreenPos[1] + (cy / 2), 0.0f, buf);

		glDisable(GL_BLEND);
	}

	glEnable(GL_DEPTH_TEST);
}

void View::DrawSelectZoomRegionOverlay()
{
	mContext->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
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

	glEnable(GL_BLEND);

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormatPosition(2);

	mContext->SetColor(0.25f, 0.25f, 1.0f, 1.0f);
	mContext->DrawPrimitives(GL_TRIANGLE_STRIP, 0, 10);

	mContext->SetColor(0.25f, 0.25f, 1.0f, 0.25f);
	mContext->DrawPrimitives(GL_TRIANGLE_STRIP, 10, 4);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void View::DrawRotateViewOverlay()
{
	int x, y, w, h;

	x = 0;
	y = 0;
	w = mWidth;
	h = mHeight;

	mContext->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
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
	mContext->SetVertexFormatPosition(2);

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
}

void View::DrawGrid()
{
	const lcPreferences& Preferences = lcGetPreferences();
	if (!Preferences.mDrawGridStuds && !Preferences.mDrawGridLines)
		return;

	mContext->SetWorldMatrix(lcMatrix44Identity());

	const int Spacing = lcMax(Preferences.mGridLineSpacing, 1);
	int MinX, MaxX, MinY, MaxY;
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	bool GridSizeValid = mModel->GetPiecesBoundingBox(Min, Max);

	if (mTrackTool == LC_TRACKTOOL_INSERT)
	{
		PieceInfo* CurPiece = gMainWindow->GetCurrentPieceInfo();

		if (CurPiece)
		{
			lcVector3 Points[8];
			lcGetBoxCorners(CurPiece->GetBoundingBox(), Points);

			lcMatrix44 WorldMatrix = GetPieceInsertPosition(false, CurPiece);

			for (int i = 0; i < 8; i++)
			{
				lcVector3 Point = lcMul31(Points[i], WorldMatrix);

				Min = lcMin(Point, Min);
				Max = lcMax(Point, Max);
			}

			GridSizeValid = true;
		}
	}

	if (GridSizeValid)
	{
		MinX = (int)(floorf(Min[0] / (20.0f * Spacing))) - 1;
		MinY = (int)(floorf(Min[1] / (20.0f * Spacing))) - 1;
		MaxX = (int)(ceilf(Max[0] / (20.0f * Spacing))) + 1;
		MaxY = (int)(ceilf(Max[1] / (20.0f * Spacing))) + 1;

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
		if (!Verts)
			return;
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
			*CurVert++ = Top;
			*CurVert++ = Z;
			*CurVert++ = U;
			*CurVert++ = V;

			*CurVert++ = Right;
			*CurVert++ = Bottom;
			*CurVert++ = Z;
			*CurVert++ = U;
			*CurVert++ = 0.0f;
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
		mContext->BindTexture2D(gGridTexture->mTexture);
		glEnable(GL_BLEND);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.25f);

		mContext->SetMaterial(LC_MATERIAL_UNLIT_TEXTURE_MODULATE);
		mContext->SetColor(lcVector4FromColor(Preferences.mGridStudColor));

		mContext->SetVertexFormat(0, 3, 0, 2, 0, false);
		mContext->DrawPrimitives(GL_TRIANGLE_STRIP, 0, 4);

		glDisable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);

		BufferOffset = 4 * 5 * sizeof(float);
	}

	if (Preferences.mDrawGridLines)
	{
		mContext->SetLineWidth(1.0f);
		mContext->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
		mContext->SetColor(lcVector4FromColor(Preferences.mGridLineColor));

		int NumVerts = 2 * (MaxX - MinX + MaxY - MinY + 2);

		mContext->SetVertexFormat(BufferOffset, 3, 0, 0, 0, false);
		mContext->DrawPrimitives(GL_LINES, 0, NumVerts);
	}
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

	lcMatrix44 TranslationMatrix = lcMatrix44Translation(lcVector3(30.375f, 30.375f, 0.0f));
	lcMatrix44 WorldViewMatrix = mCamera->mWorldView;
	WorldViewMatrix.SetTranslation(lcVector3(0, 0, 0));

	mContext->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMul(WorldViewMatrix, TranslationMatrix));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0, mWidth, 0, mHeight, -50, 50));

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormatPosition(3);
	mContext->SetIndexBufferPointer(Indices);

	mContext->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_LINES, 6, GL_UNSIGNED_SHORT, 0);

	mContext->SetColor(0.8f, 0.0f, 0.0f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, 6 * 2);
	mContext->SetColor(0.0f, 0.8f, 0.0f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, (6 + 24) * 2);
	mContext->SetColor(0.0f, 0.0f, 0.8f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, (6 + 48) * 2);

	mContext->SetMaterial(LC_MATERIAL_UNLIT_TEXTURE_MODULATE);
	mContext->SetViewMatrix(TranslationMatrix);
	mContext->BindTexture2D(gTexFont.GetTexture());
	glEnable(GL_BLEND);

	float TextBuffer[6 * 5 * 3];
	lcVector3 PosX = lcMul30(lcVector3(25.0f, 0.0f, 0.0f), WorldViewMatrix);
	gTexFont.GetGlyphTriangles(PosX.x, PosX.y, PosX.z, 'X', TextBuffer);
	lcVector3 PosY = lcMul30(lcVector3(0.0f, 25.0f, 0.0f), WorldViewMatrix);
	gTexFont.GetGlyphTriangles(PosY.x, PosY.y, PosY.z, 'Y', TextBuffer + 5 * 6);
	lcVector3 PosZ = lcMul30(lcVector3(0.0f, 0.0f, 25.0f), WorldViewMatrix);
	gTexFont.GetGlyphTriangles(PosZ.x, PosZ.y, PosZ.z, 'Z', TextBuffer + 5 * 6 * 2);

	mContext->SetVertexBufferPointer(TextBuffer);
	mContext->SetVertexFormat(0, 3, 0, 2, 0, false);

	mContext->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	mContext->DrawPrimitives(GL_TRIANGLES, 0, 6 * 3);

	glDisable(GL_BLEND);
}

void View::DrawViewport()
{
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));

	mContext->SetDepthWrite(false);
	glDisable(GL_DEPTH_TEST);

	if (gMainWindow->GetActiveView() == this)
	{
		mContext->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
		mContext->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
		float Verts[8] = { 0.0f, 0.0f, mWidth - 1.0f, 0.0f, mWidth - 1.0f, mHeight - 1.0f, 0.0f, mHeight - 1.0f };

		mContext->SetVertexBufferPointer(Verts);
		mContext->SetVertexFormatPosition(2);
		mContext->DrawPrimitives(GL_LINE_LOOP, 0, 4);
	}

	const char* CameraName = mCamera->GetName();

	if (CameraName[0])
	{
		mContext->SetMaterial(LC_MATERIAL_UNLIT_TEXTURE_MODULATE);
		mContext->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
		mContext->BindTexture2D(gTexFont.GetTexture());

		glEnable(GL_BLEND);

		gTexFont.PrintText(mContext, 3.0f, (float)mHeight - 1.0f - 6.0f, 0.0f, CameraName);

		glDisable(GL_BLEND);
	}

	mContext->SetDepthWrite(true);
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
		LC_TOOL_MOVE,        // LC_TRACKTOOL_SCALE_PLUS
		LC_TOOL_MOVE,        // LC_TRACKTOOL_SCALE_MINUS
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

lcTrackTool View::GetOverrideTrackTool(Qt::MouseButton Button) const
{
	if (mTrackToolFromOverlay)
		return LC_TRACKTOOL_NONE;

	lcTool OverrideTool = gMouseShortcuts.GetTool(Button, mInputState.Modifiers);

	if (OverrideTool == LC_NUM_TOOLS)
		return LC_TRACKTOOL_NONE;

	lcTrackTool TrackToolFromTool[LC_NUM_TOOLS] =
	{
		LC_TRACKTOOL_INSERT,     // LC_TOOL_INSERT
		LC_TRACKTOOL_POINTLIGHT, // LC_TOOL_LIGHT
		LC_TRACKTOOL_SPOTLIGHT,  // LC_TOOL_SPOTLIGHT
		LC_TRACKTOOL_CAMERA,     // LC_TOOL_CAMERA
		LC_TRACKTOOL_SELECT,     // LC_TOOL_SELECT
		LC_TRACKTOOL_MOVE_XYZ,   // LC_TOOL_MOVE
		LC_TRACKTOOL_ROTATE_XYZ, // LC_TOOL_ROTATE
		LC_TRACKTOOL_ERASER,     // LC_TOOL_ERASER
		LC_TRACKTOOL_PAINT,      // LC_TOOL_PAINT
		LC_TRACKTOOL_ZOOM,       // LC_TOOL_ZOOM
		LC_TRACKTOOL_PAN,        // LC_TOOL_PAN
		LC_TRACKTOOL_ORBIT_XY,   // LC_TOOL_ROTATE_VIEW
		LC_TRACKTOOL_ROLL,       // LC_TOOL_ROLL
		LC_TRACKTOOL_ZOOM_REGION // LC_TOOL_ZOOM_REGION
	};

	return TrackToolFromTool[OverrideTool];
}

float View::GetOverlayScale() const
{
	lcVector3 OverlayCenter;
	lcMatrix33 RelativeRotation;
	lcModel* ActiveModel = GetActiveModel();
	ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation);

	lcVector3 ScreenPos = ProjectPoint(OverlayCenter);
	ScreenPos[0] += 10.0f;
	lcVector3 Point = UnprojectPoint(ScreenPos);

	lcVector3 Dist(Point - OverlayCenter);
	return Dist.Length() * 5.0f;
}

void View::BeginDrag(lcDragState DragState)
{
	mDragState = DragState;
	UpdateTrackTool();
}

void View::EndDrag(bool Accept)
{
	if (Accept)
	{
		lcModel* ActiveModel = GetActiveModel();

		switch (mDragState)
		{
		case lcDragState::NONE:
			break;

		case lcDragState::PIECE:
			ActiveModel->InsertPieceToolClicked(GetPieceInsertPosition(false, gMainWindow->GetCurrentPieceInfo()));
			break;

		case lcDragState::COLOR:
			ActiveModel->PaintToolClicked(FindObjectUnderPointer(true, false).Object);
			break;
		}
	}

	mDragState = lcDragState::NONE;
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
	{
		lcModel* ActiveModel = GetActiveModel();
		if (ActiveModel)
			ActiveModel->SetCameraOrthographic(mCamera, Ortho);
	}
}

void View::LookAt()
{
	lcModel* ActiveModel = GetActiveModel();
	if (ActiveModel)
		ActiveModel->LookAt(mCamera);
}

void View::ZoomExtents()
{
	lcModel* ActiveModel = GetActiveModel();
	if (ActiveModel)
		ActiveModel->ZoomExtents(mCamera, (float)mWidth / (float)mHeight);
}

void View::MoveCamera(const lcVector3& Direction)
{
	lcModel* ActiveModel = GetActiveModel();
	if (ActiveModel)
		ActiveModel->MoveCamera(mCamera, Direction);
}

void View::Zoom(float Amount)
{
	lcModel* ActiveModel = GetActiveModel();
	if (ActiveModel)
		ActiveModel->Zoom(mCamera, Amount);
}

void View::UpdateTrackTool()
{
	lcTool CurrentTool = gMainWindow->GetTool();
	lcTrackTool NewTrackTool = mTrackTool;
	int x = mInputState.x;
	int y = mInputState.y;
	bool Redraw = false;
	mTrackToolFromOverlay = false;
	lcModel* ActiveModel = GetActiveModel();

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
			const float OverlayScaleRadius = 0.125f;

			NewTrackTool = (CurrentTool == LC_TOOL_MOVE) ? LC_TRACKTOOL_MOVE_XYZ : LC_TRACKTOOL_SELECT;
			mMouseDownPiece = nullptr;

			lcVector3 OverlayCenter;
			lcMatrix33 RelativeRotation;
			if (!ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation))
				break;

			lcMatrix44 WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);

			if (ActiveModel != mModel)
				WorldMatrix = lcMul(WorldMatrix, mActiveSubmodelTransform);
			OverlayCenter = WorldMatrix.GetTranslation();

			lcVector3 PlaneNormals[3] =
			{
				lcVector3(1.0f, 0.0f, 0.0f),
				lcVector3(0.0f, 1.0f, 0.0f),
				lcVector3(0.0f, 0.0f, 1.0f),
			};

			for (int i = 0; i < 3; i++)
				PlaneNormals[i] = lcMul30(PlaneNormals[i], WorldMatrix);

			lcVector3 StartEnd[2] = { lcVector3((float)x, (float)y, 0.0f), lcVector3((float)x, (float)y, 1.0f) };
			UnprojectPoints(StartEnd, 2);
			const lcVector3& Start = StartEnd[0];
			const lcVector3& End = StartEnd[1];
			float ClosestIntersectionDistance = FLT_MAX;

			lcObject* Focus = ActiveModel->GetFocusObject();
			int ControlPointIndex = -1;
			if (Focus && Focus->IsPiece())
			{
				lcPiece* Piece = (lcPiece*)Focus;
				quint32 Section = Piece->GetFocusSection();

				if (Section >= LC_PIECE_SECTION_CONTROL_POINT_1 && Section <= LC_PIECE_SECTION_CONTROL_POINT_8)
					ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_1;
			}

			quint32 AllowedTransforms = Focus ? Focus->GetAllowedTransforms() : LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z | LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z;

			for (int AxisIndex = 0; AxisIndex < 3; AxisIndex++)
			{
				lcVector4 Plane(PlaneNormals[AxisIndex], -lcDot(PlaneNormals[AxisIndex], OverlayCenter));
				lcVector3 Intersection;

				if (!lcLineSegmentPlaneIntersection(&Intersection, Start, End, Plane))
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

					if (IsTrackToolAllowed(PlaneModes[AxisIndex], AllowedTransforms))
					{
						NewTrackTool = PlaneModes[AxisIndex];
						ClosestIntersectionDistance = IntersectionDistance;
					}
				}

				if (CurrentTool == LC_TOOL_SELECT && Proj1 > OverlayRotateArrowStart && Proj1 < OverlayRotateArrowEnd && Proj2 > OverlayRotateArrowStart && Proj2 < OverlayRotateArrowEnd && ActiveModel->AnyPiecesSelected())
				{
					lcTrackTool PlaneModes[] = { LC_TRACKTOOL_ROTATE_X, LC_TRACKTOOL_ROTATE_Y, LC_TRACKTOOL_ROTATE_Z };

					if (IsTrackToolAllowed(PlaneModes[AxisIndex], AllowedTransforms))
					{
						NewTrackTool = PlaneModes[AxisIndex];
						ClosestIntersectionDistance = IntersectionDistance;
					}
				}

				if (fabs(Proj1) < OverlayMoveArrowCapRadius && Proj2 > 0.0f && Proj2 < OverlayMoveArrowSize)
				{
					lcTrackTool DirModes[] = { LC_TRACKTOOL_MOVE_Z, LC_TRACKTOOL_MOVE_X, LC_TRACKTOOL_MOVE_Y };

					if (IsTrackToolAllowed(DirModes[AxisIndex], AllowedTransforms))
					{
						NewTrackTool = DirModes[AxisIndex];
						ClosestIntersectionDistance = IntersectionDistance;
					}
				}

				if (fabs(Proj2) < OverlayMoveArrowCapRadius && Proj1 > 0.0f && Proj1 < OverlayMoveArrowSize)
				{
					lcTrackTool DirModes[] = { LC_TRACKTOOL_MOVE_Y, LC_TRACKTOOL_MOVE_Z, LC_TRACKTOOL_MOVE_X };

					if (IsTrackToolAllowed(DirModes[AxisIndex], AllowedTransforms))
					{
						NewTrackTool = DirModes[AxisIndex];
						ClosestIntersectionDistance = IntersectionDistance;
					}
				}

				lcPiece* Piece = (lcPiece*)Focus;

				if (ControlPointIndex != -1 && Piece->mPieceInfo->GetSynthInfo() && Piece->mPieceInfo->GetSynthInfo()->IsCurve())
				{
					float Strength = Piece->GetControlPoints()[ControlPointIndex].Scale;
					const float ScaleStart = (2.0f - OverlayScaleRadius) * OverlayScale + Strength;
					const float ScaleEnd = (2.0f + OverlayScaleRadius) * OverlayScale + Strength;

					if (AxisIndex == 1 && fabs(Proj1) < OverlayScaleRadius * OverlayScale)
					{
						if (Proj2 > ScaleStart && Proj2 < ScaleEnd)
						{
							if (IsTrackToolAllowed(LC_TRACKTOOL_SCALE_PLUS, AllowedTransforms))
							{
								NewTrackTool = LC_TRACKTOOL_SCALE_PLUS;
								ClosestIntersectionDistance = IntersectionDistance;
							}
						}
						else if (Proj2 < -ScaleStart && Proj2 > -ScaleEnd)
						{
							if (IsTrackToolAllowed(LC_TRACKTOOL_SCALE_MINUS, AllowedTransforms))
							{
								NewTrackTool = LC_TRACKTOOL_SCALE_MINUS;
								ClosestIntersectionDistance = IntersectionDistance;
							}
						}
					}
					else if (AxisIndex == 2 && fabs(Proj2) < OverlayScaleRadius * OverlayScale)
					{
						if (Proj1 > ScaleStart && Proj1 < ScaleEnd)
						{
							if (IsTrackToolAllowed(LC_TRACKTOOL_SCALE_PLUS, AllowedTransforms))
							{
								NewTrackTool = LC_TRACKTOOL_SCALE_PLUS;
								ClosestIntersectionDistance = IntersectionDistance;
							}
						}
						else if (Proj1 < -ScaleStart && Proj1 > -ScaleEnd)
						{
							if (IsTrackToolAllowed(LC_TRACKTOOL_SCALE_MINUS, AllowedTransforms))
							{
								NewTrackTool = LC_TRACKTOOL_SCALE_MINUS;
								ClosestIntersectionDistance = IntersectionDistance;
							}
						}
					}
				}
			}

			if (CurrentTool == LC_TOOL_SELECT && NewTrackTool == LC_TRACKTOOL_SELECT && mInputState.Modifiers == Qt::NoModifier)
			{
				lcObjectSection ObjectSection = FindObjectUnderPointer(false, false);
				lcObject* Object = ObjectSection.Object;

				if (Object && Object->IsPiece() && ObjectSection.Section == LC_PIECE_SECTION_POSITION && Object->IsSelected())
				{
					lcPiece* Piece = (lcPiece*)Object;
					mMouseDownPosition = Piece->mModelWorld.GetTranslation();
					mMouseDownPiece = Piece->mPieceInfo;
					NewTrackTool = LC_TRACKTOOL_MOVE_XYZ;
				}
			}

			mTrackToolFromOverlay = NewTrackTool != LC_TRACKTOOL_MOVE_XYZ && NewTrackTool != LC_TRACKTOOL_SELECT;
			Redraw = true;
		}
		break;

	case LC_TOOL_ROTATE:
		{
			const float OverlayScale = GetOverlayScale();
			const float OverlayRotateRadius = 2.0f;

			NewTrackTool = LC_TRACKTOOL_ROTATE_XYZ;

			lcVector3 OverlayCenter;
			lcMatrix33 RelativeRotation;
			if (!ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation))
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

					for (int i = 0; i < 2; i++)
					{
						lcVector3 Dist = Intersections[i] - OverlayCenter;

						if (lcDot(ViewDir, Dist) > 0.0f)
							continue;

						Dist = lcMul(Dist, RelativeRotation);

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

							mTrackToolFromOverlay = true;
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
				{
					NewTrackTool = LC_TRACKTOOL_ORBIT_Y;
					mTrackToolFromOverlay = true;
				}

				if ((cy - y < SquareSize) && (cy - y > -SquareSize))
				{
					NewTrackTool = LC_TRACKTOOL_ORBIT_X;
					mTrackToolFromOverlay = true;
				}
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

	case LC_NUM_TOOLS:
		break;
	}

	switch (mDragState)
	{
	case lcDragState::NONE:
		break;

	case lcDragState::PIECE:
		NewTrackTool = LC_TRACKTOOL_INSERT;
		Redraw = true;
		break;

	case lcDragState::COLOR:
		NewTrackTool = LC_TRACKTOOL_PAINT;
		break;
	}

	if (NewTrackTool != mTrackTool)
	{
		mTrackTool = NewTrackTool;
		OnUpdateCursor();

		if (Redraw)
			gMainWindow->UpdateAllViews();
	}
}

bool View::IsTrackToolAllowed(lcTrackTool TrackTool, quint32 AllowedTransforms) const
{
	switch (TrackTool)
	{
	case LC_TRACKTOOL_NONE:
	case LC_TRACKTOOL_INSERT:
	case LC_TRACKTOOL_POINTLIGHT:
	case LC_TRACKTOOL_SPOTLIGHT:
	case LC_TRACKTOOL_CAMERA:
	case LC_TRACKTOOL_SELECT:
		return true;

	case LC_TRACKTOOL_MOVE_X:
		return AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_X;

	case LC_TRACKTOOL_MOVE_Y:
		return AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Y;

	case LC_TRACKTOOL_MOVE_Z:
		return AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Z;

	case LC_TRACKTOOL_MOVE_XY:
		return (AllowedTransforms & (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y)) == (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y);

	case LC_TRACKTOOL_MOVE_XZ:
		return (AllowedTransforms & (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Z)) == (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Z);

	case LC_TRACKTOOL_MOVE_YZ:
		return (AllowedTransforms & (LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z)) == (LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z);

	case LC_TRACKTOOL_MOVE_XYZ:
		return (AllowedTransforms & (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z)) == (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z);

	case LC_TRACKTOOL_ROTATE_X:
		return AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_X;

	case LC_TRACKTOOL_ROTATE_Y:
		return AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_Y;

	case LC_TRACKTOOL_ROTATE_Z:
		return AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_Z;

	case LC_TRACKTOOL_ROTATE_XY:
		return (AllowedTransforms & (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y)) == (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y);

	case LC_TRACKTOOL_ROTATE_XYZ:
		return (AllowedTransforms & (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z)) == (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z);

	case LC_TRACKTOOL_SCALE_PLUS:
	case LC_TRACKTOOL_SCALE_MINUS:
		return AllowedTransforms & (LC_OBJECT_TRANSFORM_SCALE_X | LC_OBJECT_TRANSFORM_SCALE_Y | LC_OBJECT_TRANSFORM_SCALE_Z);

	case LC_TRACKTOOL_ERASER:
	case LC_TRACKTOOL_PAINT:
	case LC_TRACKTOOL_ZOOM:
	case LC_TRACKTOOL_PAN:
	case LC_TRACKTOOL_ORBIT_X:
	case LC_TRACKTOOL_ORBIT_Y:
	case LC_TRACKTOOL_ORBIT_XY:
	case LC_TRACKTOOL_ROLL:
	case LC_TRACKTOOL_ZOOM_REGION:
		return true;
	}

	return false;
}

void View::StartOrbitTracking()
{
	mTrackTool = LC_TRACKTOOL_ORBIT_XY;
	OnUpdateCursor();
	OnButtonDown(LC_TRACKBUTTON_LEFT);
}

void View::StartTracking(lcTrackButton TrackButton)
{
	mTrackButton = TrackButton;
	mTrackUpdated = false;
	mMouseDownX = mInputState.x;
	mMouseDownY = mInputState.y;
	lcTool Tool = GetCurrentTool();
	lcModel* ActiveModel = GetActiveModel();

	switch (Tool)
	{
	case LC_TOOL_INSERT:
	case LC_TOOL_LIGHT:
		break;

	case LC_TOOL_SPOTLIGHT:
		{
			lcVector3 Position = GetCameraLightInsertPosition();
			lcVector3 Target = Position + lcVector3(0.1f, 0.1f, 0.1f);
			ActiveModel->BeginSpotLightTool(Position, Target);
		}
		break;

	case LC_TOOL_CAMERA:
		{
			lcVector3 Position = GetCameraLightInsertPosition();
			lcVector3 Target = Position + lcVector3(0.1f, 0.1f, 0.1f);
			ActiveModel->BeginCameraTool(Position, Target);
		}
		break;

	case LC_TOOL_SELECT:
		break;

	case LC_TOOL_MOVE:
	case LC_TOOL_ROTATE:
		ActiveModel->BeginMouseTool();
		break;

	case LC_TOOL_ERASER:
	case LC_TOOL_PAINT:
		break;

	case LC_TOOL_ZOOM:
	case LC_TOOL_PAN:
	case LC_TOOL_ROTATE_VIEW:
	case LC_TOOL_ROLL:
		ActiveModel->BeginMouseTool();
		break;

	case LC_TOOL_ZOOM_REGION:
		break;

	case LC_NUM_TOOLS:
		break;
	}

	OnUpdateCursor();
}

void View::StopTracking(bool Accept)
{
	if (mTrackButton == LC_TRACKBUTTON_NONE)
		return;

	lcTool Tool = GetCurrentTool();
	lcModel* ActiveModel = GetActiveModel();

	switch (Tool)
	{
	case LC_TOOL_INSERT:
	case LC_TOOL_LIGHT:
		break;

	case LC_TOOL_SPOTLIGHT:
	case LC_TOOL_CAMERA:
		ActiveModel->EndMouseTool(Tool, Accept);
		break;

	case LC_TOOL_SELECT:
		if (Accept && mMouseDownX != mInputState.x && mMouseDownY != mInputState.y)
		{
			lcArray<lcObject*> Objects = FindObjectsInBox(mMouseDownX, mMouseDownY, mInputState.x, mInputState.y);

			if (mInputState.Modifiers & Qt::ControlModifier)
				ActiveModel->AddToSelection(Objects, true, true);
			else if (mInputState.Modifiers & Qt::ShiftModifier)
				ActiveModel->RemoveFromSelection(Objects);
			else
				ActiveModel->SetSelectionAndFocus(Objects, nullptr, 0, true);
		}
		break;

	case LC_TOOL_MOVE:
	case LC_TOOL_ROTATE:
		ActiveModel->EndMouseTool(Tool, Accept);
		break;

	case LC_TOOL_ERASER:
	case LC_TOOL_PAINT:
		break;

	case LC_TOOL_ZOOM:
	case LC_TOOL_PAN:
	case LC_TOOL_ROTATE_VIEW:
	case LC_TOOL_ROLL:
		ActiveModel->EndMouseTool(Tool, Accept);
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

			lcVector3 Center = ActiveModel->GetSelectionOrModelCenter();

			lcVector3 PlaneNormal(mCamera->mPosition - mCamera->mTargetPosition);
			lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
			lcVector3 Target, Corners[2];

			if (lcLineSegmentPlaneIntersection(&Target, Points[0], Points[1], Plane) && lcLineSegmentPlaneIntersection(&Corners[0], Points[2], Points[3], Plane) && lcLineSegmentPlaneIntersection(&Corners[1], Points[3], Points[4], Plane))
			{
				float AspectRatio = (float)mWidth / (float)mHeight;
				ActiveModel->ZoomRegionToolClicked(mCamera, AspectRatio, Points[0], Target, Corners);
			}
		}
		break;

	case LC_NUM_TOOLS:
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
	{
		lcModel* ActiveModel = GetActiveModel();
		if (ActiveModel)
			ActiveModel->ClearSelection(true);
	}
}

void View::OnButtonDown(lcTrackButton TrackButton)
{
	lcModel* ActiveModel = GetActiveModel();

	switch (mTrackTool)
	{
	case LC_TRACKTOOL_NONE:
		break;

	case LC_TRACKTOOL_INSERT:
		{
			PieceInfo* CurPiece = gMainWindow->GetCurrentPieceInfo();

			if (!CurPiece)
				break;

			ActiveModel->InsertPieceToolClicked(GetPieceInsertPosition(false, gMainWindow->GetCurrentPieceInfo()));

			if ((mInputState.Modifiers & Qt::ControlModifier) == 0)
				gMainWindow->SetTool(LC_TOOL_SELECT);

			UpdateTrackTool();
		}
		break;

	case LC_TRACKTOOL_POINTLIGHT:
		{
			ActiveModel->PointLightToolClicked(GetCameraLightInsertPosition());

			if ((mInputState.Modifiers & Qt::ControlModifier) == 0)
				gMainWindow->SetTool(LC_TOOL_SELECT);

			UpdateTrackTool();
		}
		break;

	case LC_TRACKTOOL_SPOTLIGHT:
	case LC_TRACKTOOL_CAMERA:
		StartTracking(TrackButton);
		break;
		
	case LC_TRACKTOOL_SELECT:
		{
			lcObjectSection ObjectSection = FindObjectUnderPointer(false, false);

			if (mInputState.Modifiers & Qt::ControlModifier)
				ActiveModel->FocusOrDeselectObject(ObjectSection);
			else if (mInputState.Modifiers & Qt::ShiftModifier)
				ActiveModel->RemoveFromSelection(ObjectSection);
			else
				ActiveModel->ClearSelectionAndSetFocus(ObjectSection, true);

			StartTracking(TrackButton);
		}
		break;

	case LC_TRACKTOOL_MOVE_X:
	case LC_TRACKTOOL_MOVE_Y:
	case LC_TRACKTOOL_MOVE_Z:
	case LC_TRACKTOOL_MOVE_XY:
	case LC_TRACKTOOL_MOVE_XZ:
	case LC_TRACKTOOL_MOVE_YZ:
	case LC_TRACKTOOL_MOVE_XYZ:
		if (ActiveModel->AnyObjectsSelected())
			StartTracking(TrackButton);
		break;

	case LC_TRACKTOOL_ROTATE_X:
	case LC_TRACKTOOL_ROTATE_Y:
	case LC_TRACKTOOL_ROTATE_Z:
	case LC_TRACKTOOL_ROTATE_XY:
	case LC_TRACKTOOL_ROTATE_XYZ:
		if (ActiveModel->AnyPiecesSelected())
			StartTracking(TrackButton);
		break;

	case LC_TRACKTOOL_SCALE_PLUS:
	case LC_TRACKTOOL_SCALE_MINUS:
		if (ActiveModel->AnyPiecesSelected())
			StartTracking(TrackButton);
		break;

	case LC_TRACKTOOL_ERASER:
		ActiveModel->EraserToolClicked(FindObjectUnderPointer(false, false).Object);
		break;

	case LC_TRACKTOOL_PAINT:
		ActiveModel->PaintToolClicked(FindObjectUnderPointer(true, false).Object);
		break;

	case LC_TRACKTOOL_ZOOM:
	case LC_TRACKTOOL_PAN:
	case LC_TRACKTOOL_ORBIT_X:
	case LC_TRACKTOOL_ORBIT_Y:
	case LC_TRACKTOOL_ORBIT_XY:
	case LC_TRACKTOOL_ROLL:
	case LC_TRACKTOOL_ZOOM_REGION:
		StartTracking(TrackButton);
		break;
	}
}

void View::OnLeftButtonDown()
{
	if (mTrackButton != LC_TRACKBUTTON_NONE)
	{
		StopTracking(false);
		return;
	}

	gMainWindow->SetActiveView(this);

	if (mViewSphere.OnLeftButtonDown())
		return;

	lcTrackTool OverrideTool = GetOverrideTrackTool(Qt::LeftButton);

	if (OverrideTool != LC_TRACKTOOL_NONE)
	{
		mTrackTool = OverrideTool;
		OnUpdateCursor();
	}

	OnButtonDown(LC_TRACKBUTTON_LEFT);
}

void View::OnLeftButtonUp()
{
	StopTracking(mTrackButton == LC_TRACKBUTTON_LEFT);

	if (mViewSphere.OnLeftButtonUp())
		return;
}

void View::OnLeftButtonDoubleClick()
{
	gMainWindow->SetActiveView(this);

	lcObjectSection ObjectSection = FindObjectUnderPointer(false, false);
	lcModel* ActiveModel = GetActiveModel();

	if (mInputState.Modifiers & Qt::ControlModifier)
		ActiveModel->FocusOrDeselectObject(ObjectSection);
	else if (mInputState.Modifiers & Qt::ShiftModifier)
		ActiveModel->RemoveFromSelection(ObjectSection);
	else
		ActiveModel->ClearSelectionAndSetFocus(ObjectSection, true);
}

void View::OnMiddleButtonDown()
{
	if (mTrackButton != LC_TRACKBUTTON_NONE)
	{
		StopTracking(false);
		return;
	}

	gMainWindow->SetActiveView(this);
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
	lcTrackTool OverrideTool = GetOverrideTrackTool(Qt::MiddleButton);

	if (OverrideTool != LC_TRACKTOOL_NONE)
	{
		mTrackTool = OverrideTool;
		OnUpdateCursor();
	}
#endif
	OnButtonDown(LC_TRACKBUTTON_MIDDLE);
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

	lcTrackTool OverrideTool = GetOverrideTrackTool(Qt::RightButton);

	if (OverrideTool != LC_TRACKTOOL_NONE)
	{
		mTrackTool = OverrideTool;
		OnUpdateCursor();
	}

	OnButtonDown(LC_TRACKBUTTON_RIGHT);
}

void View::OnRightButtonUp()
{
	bool ShowMenu = mTrackButton == LC_TRACKBUTTON_NONE || !mTrackUpdated;

	if (mTrackButton != LC_TRACKBUTTON_NONE)
		StopTracking(mTrackButton == LC_TRACKBUTTON_RIGHT);

	if (ShowMenu)
		ShowContextMenu();
}

void View::OnBackButtonUp()
{
	gMainWindow->HandleCommand(LC_VIEW_TIME_PREVIOUS);
}

void View::OnForwardButtonUp()
{
	gMainWindow->HandleCommand(LC_VIEW_TIME_NEXT);
}

void View::OnMouseMove()
{
	lcModel* ActiveModel = GetActiveModel();

	if (!ActiveModel)
		return;

	if (mTrackButton == LC_TRACKBUTTON_NONE)
	{
		if (mViewSphere.OnMouseMove())
		{
			lcTrackTool NewTrackTool = mViewSphere.IsDragging() ? LC_TRACKTOOL_ORBIT_XY : LC_TRACKTOOL_NONE;

			if (NewTrackTool != mTrackTool)
			{
				mTrackTool = NewTrackTool;
				OnUpdateCursor();
			}

			return;
		}

		UpdateTrackTool();

		if (mTrackTool == LC_TRACKTOOL_INSERT)
			gMainWindow->UpdateAllViews();

		return;
	}

	mTrackUpdated = true;
	const float MouseSensitivity = 0.5f / (21.0f - lcGetPreferences().mMouseSensitivity);

	switch (mTrackTool)
	{
	case LC_TRACKTOOL_NONE:
	case LC_TRACKTOOL_INSERT:
	case LC_TRACKTOOL_POINTLIGHT:
		break;

	case LC_TRACKTOOL_SPOTLIGHT:
		ActiveModel->UpdateSpotLightTool(GetCameraLightInsertPosition());
		break;

	case LC_TRACKTOOL_CAMERA:
		ActiveModel->UpdateCameraTool(GetCameraLightInsertPosition());
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
	case LC_TRACKTOOL_SCALE_PLUS:
	case LC_TRACKTOOL_SCALE_MINUS:
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

			lcVector3 Center;
			lcMatrix33 RelativeRotation;
			ActiveModel->GetMoveRotateTransform(Center, RelativeRotation);

			if (mTrackTool == LC_TRACKTOOL_MOVE_X || mTrackTool == LC_TRACKTOOL_MOVE_Y || mTrackTool == LC_TRACKTOOL_MOVE_Z)
			{
				lcVector3 Direction;
				if (mTrackTool == LC_TRACKTOOL_MOVE_X)
					Direction = lcVector3(1.0f, 0.0f, 0.0f);
				else if (mTrackTool == LC_TRACKTOOL_MOVE_Y)
					Direction = lcVector3(0.0f, 1.0f, 0.0f);
				else
					Direction = lcVector3(0.0f, 0.0f, 1.0f);

				Direction = lcMul(Direction, RelativeRotation);

				lcVector3 Intersection;
				lcClosestPointsBetweenLines(Center, Center + Direction, CurrentStart, CurrentEnd, &Intersection, nullptr);

				lcVector3 MoveStart;
				lcClosestPointsBetweenLines(Center, Center + Direction, MouseDownStart, MouseDownEnd, &MoveStart, nullptr);

				lcVector3 Distance = Intersection - MoveStart;
				Distance = lcMul(Distance, lcMatrix33AffineInverse(RelativeRotation));
				ActiveModel->UpdateMoveTool(Distance, mTrackButton != LC_TRACKBUTTON_LEFT);
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

				PlaneNormal = lcMul(PlaneNormal, RelativeRotation);
				lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
				lcVector3 Intersection;

				if (lcLineSegmentPlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane))
				{
					lcVector3 MoveStart;

					if (lcLineSegmentPlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
					{
						lcVector3 Distance = Intersection - MoveStart;
						Distance = lcMul(Distance, lcMatrix33AffineInverse(RelativeRotation));
						ActiveModel->UpdateMoveTool(Distance, mTrackButton != LC_TRACKBUTTON_LEFT);
					}
				}
			}
			else if (mTrackTool == LC_TRACKTOOL_MOVE_XYZ && mMouseDownPiece)
			{
				lcMatrix44 NewPosition = GetPieceInsertPosition(true, mMouseDownPiece);
				lcVector3 Distance = NewPosition.GetTranslation() - mMouseDownPosition;
				ActiveModel->UpdateMoveTool(Distance, mTrackButton != LC_TRACKBUTTON_LEFT);
			}
			else if (mTrackTool == LC_TRACKTOOL_SCALE_PLUS || mTrackTool == LC_TRACKTOOL_SCALE_MINUS)
			{
				lcVector3 Direction;
				if (mTrackTool == LC_TRACKTOOL_SCALE_PLUS)
					Direction = lcVector3(1.0f, 0.0f, 0.0f);
				else
					Direction = lcVector3(-1.0f, 0.0f, 0.0f);

				Direction = lcMul(Direction, RelativeRotation);

				lcVector3 Intersection;
				lcClosestPointsBetweenLines(Center, Center + Direction, CurrentStart, CurrentEnd, &Intersection, nullptr);

				lcObject* Focus = ActiveModel->GetFocusObject();
				if (Focus && Focus->IsPiece())
				{
					lcPiece* Piece = (lcPiece*)Focus;
					quint32 Section = Piece->GetFocusSection();

					if (Section >= LC_PIECE_SECTION_CONTROL_POINT_1 && Section <= LC_PIECE_SECTION_CONTROL_POINT_8)
					{
						const float ScaleMax = 200.0f;
						const float OverlayScale = GetOverlayScale();
						const float ScaleStart = 2.0f * OverlayScale;

						lcVector3 Position = Piece->GetSectionPosition(Section);
						lcVector3 Start = Position + Direction * ScaleStart;

						float Distance = lcLength(Intersection - Start);
						if (lcDot(Direction, Intersection - Start) < 0.0f)
							Distance = 0.0f;

						float Scale = lcClamp(Distance, 0.1f, ScaleMax);

						ActiveModel->UpdateScaleTool(Scale);
					}
				}
			}
			else
			{
				lcVector3 PlaneNormal = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);
				lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
				lcVector3 Intersection;

				if (lcLineSegmentPlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane))
				{
					lcVector3 MoveStart;

					if (lcLineSegmentPlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
					{
						lcVector3 Distance = Intersection - MoveStart;
						ActiveModel->UpdateMoveTool(Distance, mTrackButton != LC_TRACKBUTTON_LEFT);
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
				Dir1 = lcVector3(1.0f, 0.0f, 0.0f);
				break;
			case LC_TRACKTOOL_ROTATE_Y:
				Dir1 = lcVector3(0.0f, 1.0f, 0.0f);
				break;
			case LC_TRACKTOOL_ROTATE_Z:
				Dir1 = lcVector3(0.0f, 0.0f, 1.0f);
				break;
			default:
				Dir1 = lcVector3(0.0f, 0.0f, 1.0f);
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

			ActiveModel->UpdateRotateTool(MoveX + MoveY, mTrackButton != LC_TRACKBUTTON_LEFT);
		}
		break;

	case LC_TRACKTOOL_ROTATE_XY:
		{
			lcVector3 ScreenZ = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);
			lcVector3 ScreenX = lcCross(ScreenZ, mCamera->mUpVector);
			lcVector3 ScreenY = mCamera->mUpVector;

			lcVector3 MoveX = 36.0f * (float)(mInputState.x - mMouseDownX) * MouseSensitivity * ScreenX;
			lcVector3 MoveY = 36.0f * (float)(mInputState.y - mMouseDownY) * MouseSensitivity * ScreenY;
			ActiveModel->UpdateRotateTool(MoveX + MoveY, mTrackButton != LC_TRACKBUTTON_LEFT);
		}
		break;

	case LC_TRACKTOOL_ROTATE_XYZ:
		{
			lcVector3 ScreenZ = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);

			ActiveModel->UpdateRotateTool(36.0f * (float)(mInputState.y - mMouseDownY) * MouseSensitivity * ScreenZ, mTrackButton != LC_TRACKBUTTON_LEFT);
		}
		break;

	case LC_TRACKTOOL_ERASER:
	case LC_TRACKTOOL_PAINT:
		break;

	case LC_TRACKTOOL_ZOOM:
		ActiveModel->UpdateZoomTool(mCamera, 2.0f * MouseSensitivity * (mInputState.y - mMouseDownY));
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
			lcVector3 Center = ActiveModel->GetSelectionOrModelCenter();

			lcVector3 PlaneNormal(mCamera->mPosition - mCamera->mTargetPosition);
			lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
			lcVector3 Intersection, MoveStart;

			if (!lcLineSegmentPlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane) || !lcLineSegmentPlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
			{
				Center = MouseDownStart + lcNormalize(MouseDownEnd - MouseDownStart) * 10.0f;
				Plane = lcVector4(PlaneNormal, -lcDot(PlaneNormal, Center));

				if (!lcLineSegmentPlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane) || !lcLineSegmentPlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
					break;
			}

			ActiveModel->UpdatePanTool(mCamera, MoveStart - Intersection);
		}
		break;

	case LC_TRACKTOOL_ORBIT_X:
		ActiveModel->UpdateOrbitTool(mCamera, 0.1f * MouseSensitivity * (mInputState.x - mMouseDownX), 0.0f);
		break;

	case LC_TRACKTOOL_ORBIT_Y:
		ActiveModel->UpdateOrbitTool(mCamera, 0.0f, 0.1f * MouseSensitivity * (mInputState.y - mMouseDownY));
		break;

	case LC_TRACKTOOL_ORBIT_XY:
		ActiveModel->UpdateOrbitTool(mCamera, 0.1f * MouseSensitivity * (mInputState.x - mMouseDownX), 0.1f * MouseSensitivity * (mInputState.y - mMouseDownY));
		break;

	case LC_TRACKTOOL_ROLL:
		ActiveModel->UpdateRollTool(mCamera, 2.0f * MouseSensitivity * (mInputState.x - mMouseDownX) * LC_DTOR);
		break;

	case LC_TRACKTOOL_ZOOM_REGION:
		Redraw();
		break;
	}
}

void View::OnMouseWheel(float Direction)
{
	mModel->Zoom(mCamera, (int)(((mInputState.Modifiers & Qt::ControlModifier) ? 100 : 10) * Direction));
}

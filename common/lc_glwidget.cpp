#include "lc_global.h"
#include "lc_glwidget.h"
#include "lc_application.h"
#include "lc_context.h"
#include "piece.h"
#include "camera.h"
#include "pieceinf.h"
#include "texfont.h"
#include "lc_model.h"
#include "lc_scene.h"

lcGLWidget* lcGLWidget::mLastFocusedView;

lcGLWidget::lcGLWidget(lcModel* Model)
	: mModel(Model), mScene(new lcScene())
{
	mContext = new lcContext();
}

lcGLWidget::~lcGLWidget()
{
	if (mLastFocusedView == this)
		mLastFocusedView = nullptr;

	if (mDeleteContext)
		delete mContext;
}

lcModel* lcGLWidget::GetActiveModel() const
{
	return !mActiveSubmodelInstance ? mModel : mActiveSubmodelInstance->mPieceInfo->GetModel();
}

void lcGLWidget::SetFocus(bool Focus)
{
	if (Focus)
		mLastFocusedView = this;
}

void lcGLWidget::SetMousePosition(int MouseX, int MouseY)
{
	mMouseX = MouseX;
	mMouseY = MouseY;
}

void lcGLWidget::SetMouseModifiers(Qt::KeyboardModifiers MouseModifiers)
{
	mMouseModifiers = MouseModifiers;
}

void lcGLWidget::SetContext(lcContext* Context)
{
	if (mDeleteContext)
		delete mContext;

	mContext = Context;
	mDeleteContext = false;
}

void lcGLWidget::MakeCurrent()
{
	mWidget->makeCurrent();
}

void lcGLWidget::Redraw()
{
	mWidget->update();
}

lcCursor lcGLWidget::GetCursor() const
{
	if (mTrackButton != lcTrackButton::None)
		return lcCursor::Hidden;

	if (mTrackTool == lcTrackTool::Select)
	{
		if (mMouseModifiers & Qt::ControlModifier)
			return lcCursor::SelectAdd;

		if (mMouseModifiers & Qt::ShiftModifier)
			return lcCursor::SelectRemove;
	}

	constexpr lcCursor CursorFromTrackTool[] =
	{
		lcCursor::Select,      // lcTrackTool::None
		lcCursor::Brick,       // lcTrackTool::Insert
		lcCursor::Light,       // lcTrackTool::PointLight
		lcCursor::Spotlight,   // lcTrackTool::SpotLight
		lcCursor::Camera,      // lcTrackTool::Camera
		lcCursor::Select,      // lcTrackTool::Select
		lcCursor::Move,        // lcTrackTool::MoveX
		lcCursor::Move,        // lcTrackTool::MoveY
		lcCursor::Move,        // lcTrackTool::MoveZ
		lcCursor::Move,        // lcTrackTool::MoveXY
		lcCursor::Move,        // lcTrackTool::MoveXZ
		lcCursor::Move,        // lcTrackTool::MoveYZ
		lcCursor::Move,        // lcTrackTool::MoveXYZ
		lcCursor::Rotate,      // lcTrackTool::RotateX
		lcCursor::Rotate,      // lcTrackTool::RotateY
		lcCursor::Rotate,      // lcTrackTool::RotateZ
		lcCursor::Rotate,      // lcTrackTool::RotateXY
		lcCursor::Rotate,      // lcTrackTool::RotateXYZ
		lcCursor::Move,        // lcTrackTool::ScalePlus
		lcCursor::Move,        // lcTrackTool::ScaleMinus
		lcCursor::Delete,      // lcTrackTool::Eraser
		lcCursor::Paint,       // lcTrackTool::Paint
		lcCursor::ColorPicker, // lcTrackTool::ColorPicker
		lcCursor::Zoom,        // lcTrackTool::Zoom
		lcCursor::Pan,         // lcTrackTool::Pan
		lcCursor::RotateX,     // lcTrackTool::OrbitX
		lcCursor::RotateY,     // lcTrackTool::OrbitY
		lcCursor::RotateView,  // lcTrackTool::OrbitXY
		lcCursor::Roll,        // lcTrackTool::Roll
		lcCursor::ZoomRegion   // lcTrackTool::ZoomRegion
	};

	LC_ARRAY_SIZE_CHECK(CursorFromTrackTool, lcTrackTool::Count);

	if (mTrackTool >= lcTrackTool::None && mTrackTool < lcTrackTool::Count)
		return CursorFromTrackTool[static_cast<int>(mTrackTool)];

	return lcCursor::Select;
}

void lcGLWidget::SetCursor(lcCursor CursorType)
{
	if (mCursor == CursorType)
		return;

	struct lcCursorInfo
	{
		int x, y;
		const char* Name;
	};

	constexpr lcCursorInfo Cursors[] =
	{
		{  0,  0, "" },                                 // lcCursor::Hidden
		{  0,  0, "" },                                 // lcCursor::Default
		{  8,  3, ":/resources/cursor_insert" },        // lcCursor::Brick
		{ 15, 15, ":/resources/cursor_light" },         // lcCursor::Light
		{  7, 10, ":/resources/cursor_spotlight" },     // lcCursor::Spotlight
		{ 15,  9, ":/resources/cursor_camera" },        // lcCursor::Camera
		{  0,  2, ":/resources/cursor_select" },        // lcCursor::Select
		{  0,  2, ":/resources/cursor_select_add" },    // lcCursor::SelectAdd
		{  0,  2, ":/resources/cursor_select_remove" }, // lcCursor::SelectRemove
		{ 15, 15, ":/resources/cursor_move" },          // lcCursor::Move
		{ 15, 15, ":/resources/cursor_rotate" },        // lcCursor::Rotate
		{ 15, 15, ":/resources/cursor_rotatex" },       // lcCursor::RotateX
		{ 15, 15, ":/resources/cursor_rotatey" },       // lcCursor::RotateY
		{  0, 10, ":/resources/cursor_delete" },        // lcCursor::Delete
		{ 14, 14, ":/resources/cursor_paint" },         // lcCursor::Paint
		{  1, 13, ":/resources/cursor_color_picker" },  // lcCursor::ColorPicker
		{ 15, 15, ":/resources/cursor_zoom" },          // lcCursor::Zoom
		{  9,  9, ":/resources/cursor_zoom_region" },   // lcCursor::ZoomRegion
		{ 15, 15, ":/resources/cursor_pan" },           // lcCursor::Pan
		{ 15, 15, ":/resources/cursor_roll" },          // lcCursor::Roll
		{ 15, 15, ":/resources/cursor_rotate_view" },   // lcCursor::RotateView
	};

	LC_ARRAY_SIZE_CHECK(Cursors, lcCursor::Count);

	if (CursorType == lcCursor::Hidden)
	{
		mWidget->setCursor(Qt::BlankCursor);
		mCursor = CursorType;
	}
	else if (CursorType >= lcCursor::First && CursorType < lcCursor::Count)
	{
		const lcCursorInfo& Cursor = Cursors[static_cast<int>(CursorType)];
		mWidget->setCursor(QCursor(QPixmap(Cursor.Name), Cursor.x, Cursor.y));
		mCursor = CursorType;
	}
	else
	{
		mWidget->unsetCursor();
		mCursor = lcCursor::Default;
	}
}

void lcGLWidget::UpdateCursor()
{
	SetCursor(GetCursor());
}

lcTool lcGLWidget::GetCurrentTool() const
{
	constexpr lcTool ToolFromTrackTool[] =
	{
		lcTool::Select,      // lcTrackTool::None
		lcTool::Insert,      // lcTrackTool::Insert
		lcTool::Light,       // lcTrackTool::PointLight
		lcTool::SpotLight,   // lcTrackTool::SpotLight
		lcTool::Camera,      // lcTrackTool::Camera
		lcTool::Select,      // lcTrackTool::Select
		lcTool::Move,        // lcTrackTool::MoveX
		lcTool::Move,        // lcTrackTool::MoveY
		lcTool::Move,        // lcTrackTool::MoveZ
		lcTool::Move,        // lcTrackTool::MoveXY
		lcTool::Move,        // lcTrackTool::MoveXZ
		lcTool::Move,        // lcTrackTool::MoveYZ
		lcTool::Move,        // lcTrackTool::MoveXYZ
		lcTool::Rotate,      // lcTrackTool::RotateX
		lcTool::Rotate,      // lcTrackTool::RotateY
		lcTool::Rotate,      // lcTrackTool::RotateZ
		lcTool::Rotate,      // lcTrackTool::RotateXY
		lcTool::Rotate,      // lcTrackTool::RotateXYZ
		lcTool::Move,        // lcTrackTool::ScalePlus
		lcTool::Move,        // lcTrackTool::ScaleMinus
		lcTool::Eraser,      // lcTrackTool::Eraser
		lcTool::Paint,       // lcTrackTool::Paint
		lcTool::ColorPicker, // lcTrackTool::ColorPicker
		lcTool::Zoom,        // lcTrackTool::Zoom
		lcTool::Pan,         // lcTrackTool::Pan
		lcTool::RotateView,  // lcTrackTool::OrbitX
		lcTool::RotateView,  // lcTrackTool::OrbitY
		lcTool::RotateView,  // lcTrackTool::OrbitXY
		lcTool::Roll,        // lcTrackTool::Roll
		lcTool::ZoomRegion   // lcTrackTool::ZoomRegion
	};

	LC_ARRAY_SIZE_CHECK(ToolFromTrackTool, lcTrackTool::Count);

	if (mTrackTool >= lcTrackTool::None && mTrackTool < lcTrackTool::Count)
		return ToolFromTrackTool[static_cast<int>(mTrackTool)];

	return lcTool::Select;
}

lcMatrix44 lcGLWidget::GetProjectionMatrix() const
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

lcVector3 lcGLWidget::ProjectPoint(const lcVector3& Point) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	return lcProjectPoint(Point, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
}

lcVector3 lcGLWidget::UnprojectPoint(const lcVector3& Point) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	return lcUnprojectPoint(Point, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
}

void lcGLWidget::UnprojectPoints(lcVector3* Points, int NumPoints) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	lcUnprojectPoints(Points, NumPoints, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
}

void lcGLWidget::ZoomExtents()
{
	lcModel* ActiveModel = GetActiveModel();
	if (ActiveModel)
		ActiveModel->ZoomExtents(mCamera, (float)mWidth / (float)mHeight);
}

void lcGLWidget::SetViewpoint(lcViewpoint Viewpoint)
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

	emit CameraChanged();
}

void lcGLWidget::SetViewpoint(const lcVector3& Position)
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

	emit CameraChanged();
}

void lcGLWidget::SetViewpoint(const lcVector3& Position, const lcVector3& Target, const lcVector3& Up)
{
	if (!mCamera || !mCamera->IsSimple())
	{
		lcCamera* OldCamera = mCamera;

		mCamera = new lcCamera(true);

		if (OldCamera)
			mCamera->CopySettings(OldCamera);
	}

	mCamera->SetViewpoint(Position, Target, Up);
	Redraw();

	emit CameraChanged();
}

void lcGLWidget::SetCameraAngles(float Latitude, float Longitude)
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

void lcGLWidget::SetDefaultCamera()
{
	if (!mCamera || !mCamera->IsSimple())
		mCamera = new lcCamera(true);

	mCamera->SetViewpoint(lcViewpoint::Home);

	emit CameraChanged();
}

void lcGLWidget::SetCamera(lcCamera* Camera, bool ForceCopy)
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

void lcGLWidget::SetCamera(const QString& CameraName)
{
	const lcArray<lcCamera*>& Cameras = mModel->GetCameras();

	for (int CameraIdx = 0; CameraIdx < Cameras.GetSize(); CameraIdx++)
	{
		if (CameraName.compare(Cameras[CameraIdx]->GetName(), Qt::CaseInsensitive) == 0)
		{
			SetCameraIndex(CameraIdx);
			return;
		}
	}
}

void lcGLWidget::SetCameraIndex(int Index)
{
	const lcArray<lcCamera*>& Cameras = mModel->GetCameras();

	if (Index >= Cameras.GetSize())
		return;

	lcCamera* Camera = Cameras[Index];
	SetCamera(Camera, false);

	emit CameraChanged();
	Redraw();
}

void lcGLWidget::StartTracking(lcTrackButton TrackButton)
{
	mTrackButton = TrackButton;
	mTrackUpdated = false;
	mMouseDownX = mMouseX;
	mMouseDownY = mMouseY;
	lcTool Tool = GetCurrentTool();
	lcModel* ActiveModel = GetActiveModel();

	switch (Tool)
	{
		case lcTool::Insert:
		case lcTool::Light:
			break;

		case lcTool::SpotLight:
		{
			lcVector3 Position = GetCameraLightInsertPosition();
			lcVector3 Target = Position + lcVector3(0.1f, 0.1f, 0.1f);
			ActiveModel->BeginSpotLightTool(Position, Target);
		}
		break;

		case lcTool::Camera:
		{
			lcVector3 Position = GetCameraLightInsertPosition();
			lcVector3 Target = Position + lcVector3(0.1f, 0.1f, 0.1f);
			ActiveModel->BeginCameraTool(Position, Target);
		}
		break;

		case lcTool::Select:
			break;

		case lcTool::Move:
		case lcTool::Rotate:
			ActiveModel->BeginMouseTool();
			break;

		case lcTool::Eraser:
		case lcTool::Paint:
		case lcTool::ColorPicker:
			break;

		case lcTool::Zoom:
		case lcTool::Pan:
		case lcTool::RotateView:
		case lcTool::Roll:
			ActiveModel->BeginMouseTool();
			break;

		case lcTool::ZoomRegion:
			break;

		case lcTool::Count:
			break;
	}

	UpdateCursor();
}

lcVector3 lcGLWidget::GetCameraLightInsertPosition() const
{
	lcModel* ActiveModel = GetActiveModel();

	std::array<lcVector3, 2> ClickPoints = { { lcVector3((float)mMouseX, (float)mMouseY, 0.0f), lcVector3((float)mMouseX, (float)mMouseY, 1.0f) } };
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

void lcGLWidget::DrawBackground() const
{
	const lcPreferences& Preferences = lcGetPreferences();

	if (!Preferences.mBackgroundGradient)
	{
		lcVector3 BackgroundColor = lcVector3FromColor(Preferences.mBackgroundSolidColor);
		glClearColor(BackgroundColor[0], BackgroundColor[1], BackgroundColor[2], 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}

	lcContext* Context = mContext;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Context->SetDepthWrite(false);
	glDisable(GL_DEPTH_TEST);

	float ViewWidth = (float)mWidth;
	float ViewHeight = (float)mHeight;

	Context->SetWorldMatrix(lcMatrix44Identity());
	Context->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	Context->SetProjectionMatrix(lcMatrix44Ortho(0.0f, ViewWidth, 0.0f, ViewHeight, -1.0f, 1.0f));

	Context->SetSmoothShading(true);

	const lcVector3 Color1 = lcVector3FromColor(Preferences.mBackgroundGradientColorTop);
	const lcVector3 Color2 = lcVector3FromColor(Preferences.mBackgroundGradientColorBottom);

	float Verts[] =
	{
		ViewWidth, ViewHeight, Color1[0], Color1[1], Color1[2], 1.0f,
		0.0f,      ViewHeight, Color1[0], Color1[1], Color1[2], 1.0f,
		0.0f,      0.0f,       Color2[0], Color2[1], Color2[2], 1.0f,
		ViewWidth, 0.0f,       Color2[0], Color2[1], Color2[2], 1.0f
	};

	Context->SetMaterial(lcMaterialType::UnlitVertexColor);
	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormat(0, 2, 0, 0, 4, false);

	Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, 4);

	Context->SetSmoothShading(false);

	glEnable(GL_DEPTH_TEST);
	Context->SetDepthWrite(true);
}

void lcGLWidget::DrawViewport() const
{
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));

	mContext->SetDepthWrite(false);
	glDisable(GL_DEPTH_TEST);

	mContext->SetMaterial(lcMaterialType::UnlitColor);

	if (mLastFocusedView == this)
		mContext->SetColor(lcVector4FromColor(lcGetPreferences().mActiveViewColor));
	else
		mContext->SetColor(lcVector4FromColor(lcGetPreferences().mInactiveViewColor));

	float Verts[8] = { 0.0f, 0.0f, mWidth - 1.0f, 0.0f, mWidth - 1.0f, mHeight - 1.0f, 0.0f, mHeight - 1.0f };

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormatPosition(2);
	mContext->DrawPrimitives(GL_LINE_LOOP, 0, 4);

	QString CameraName = mCamera->GetName();

	if (!CameraName.isEmpty())
	{
		mContext->SetMaterial(lcMaterialType::UnlitTextureModulate);
		mContext->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
		mContext->BindTexture2D(gTexFont.GetTexture());

		glEnable(GL_BLEND);

		gTexFont.PrintText(mContext, 3.0f, (float)mHeight - 1.0f - 6.0f, 0.0f, CameraName.toLatin1().constData());

		glDisable(GL_BLEND);
	}

	mContext->SetDepthWrite(true);
	glEnable(GL_DEPTH_TEST);
}

void lcGLWidget::DrawAxes() const
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

	mContext->SetMaterial(lcMaterialType::UnlitColor);
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

	mContext->SetMaterial(lcMaterialType::UnlitTextureModulate);
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

	mContext->SetColor(lcVector4FromColor(lcGetPreferences().mAxesColor));
	mContext->DrawPrimitives(GL_TRIANGLES, 0, 6 * 3);

	glDisable(GL_BLEND);
}

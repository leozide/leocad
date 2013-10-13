#include "lc_global.h"
#include "view.h"
#include "lc_mainwindow.h"
#include "project.h"
#include "pieceinf.h"
#include "lc_piece.h"
#include "lc_camera.h"
#include "system.h"
#include "texfont.h"

View::View(Project *project)
{
	mProject = project;
	mCamera = NULL;

	mMouseTrack = LC_MOUSETRACK_NONE;
	mTrackTool = LC_TRACKTOOL_NONE;

	if (gMainWindow->mActiveView)
		SetCamera(gMainWindow->mActiveView->mCamera, false);
	else
		SetDefaultCamera();
}

View::~View()
{
	if (gMainWindow)
	{
		gMainWindow->mViews.Remove(this);

		if (gMainWindow->mActiveView == this)
		{
			if (gMainWindow->mViews.GetSize() > 0)
				gMainWindow->SetActiveView(gMainWindow->mViews[0]);
		}
	}

	if (mCamera && mCamera->IsSimple())
		delete mCamera;
}

void View::SetCamera(lcCamera* Camera, bool ForceCopy)
{
	if (Camera->IsSimple() || ForceCopy)
	{
		if (!mCamera || !mCamera->IsSimple())
			mCamera = new lcCamera(true);

		mCamera->CopySettings(Camera);
	}
	else
	{
		if (mCamera && mCamera->IsSimple())
			delete mCamera;

		mCamera = Camera;
	}
}

void View::SetDefaultCamera()
{
	if (!mCamera || !mCamera->IsSimple())
		mCamera = new lcCamera(true);

	mCamera->SetViewpoint(LC_VIEWPOINT_HOME);
}

LC_CURSOR_TYPE View::GetCursor() const
{
	// TODO: check if we're the focused window and return just the default arrow if we aren't.
	lcTool Tool = GetMouseTool(mTrackTool);

	switch (Tool)
	{
		case LC_TOOL_SELECT:
			if (mInputState.Control)
				return LC_CURSOR_SELECT_GROUP;
			else
				return LC_CURSOR_SELECT;

		case LC_TOOL_INSERT:
			return LC_CURSOR_BRICK;

		case LC_TOOL_LIGHT:
			return LC_CURSOR_LIGHT;

		case LC_TOOL_SPOTLIGHT:
			return LC_CURSOR_SPOTLIGHT;

		case LC_TOOL_CAMERA:
			return LC_CURSOR_CAMERA;

		case LC_TOOL_MOVE:
			return LC_CURSOR_MOVE;

		case LC_TOOL_ROTATE:
			return LC_CURSOR_ROTATE;

		case LC_TOOL_ERASER:
			return LC_CURSOR_DELETE;

		case LC_TOOL_PAINT:
			return LC_CURSOR_PAINT;

		case LC_TOOL_ZOOM:
			return LC_CURSOR_ZOOM;

		case LC_TOOL_ZOOM_REGION:
			return LC_CURSOR_ZOOM_REGION;

		case LC_TOOL_PAN:
			return LC_CURSOR_PAN;

		case LC_TOOL_ROTATE_VIEW:
			switch (mTrackTool)
			{
				case LC_TRACKTOOL_ROTATE_VIEW_X:
					return LC_CURSOR_ROTATEX;
				case LC_TRACKTOOL_ROTATE_VIEW_Y:
					return LC_CURSOR_ROTATEY;
				case LC_TRACKTOOL_ROTATE_VIEW_Z:
					return LC_CURSOR_ROLL;
				case LC_TRACKTOOL_ROTATE_VIEW:
				default:
					return LC_CURSOR_ROTATE_VIEW;
			}

		case LC_TOOL_ROLL:
			return LC_CURSOR_ROLL;

		default:
			LC_ASSERT(false);
			return LC_CURSOR_DEFAULT;
	}
}

void View::OnDraw()
{
	glViewport(0, 0, mWidth, mHeight);
	glEnableClientState(GL_VERTEX_ARRAY);

	mProject->mActiveModel->DrawBackground(this);
	mProject->mActiveModel->DrawScene(this, true);

	if (mWidget)
	{
		DrawMouseTracking();
		DrawViewport();
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}

void View::OnInitialUpdate()
{
	MakeCurrent();

	gMainWindow->mViews.Add(this);

	if (!gMainWindow->mActiveView)
		gMainWindow->SetActiveView(this);

	mProject->RenderInitialize();
}

void View::OnUpdateCursor()
{
	SetCursor(GetCursor());
}

void View::OnLeftButtonDown()
{
	if (mMouseTrack != LC_MOUSETRACK_NONE)
	{
		StopTracking(false);
		return;
	}

	gMainWindow->SetActiveView(this);

	lcTrackTool TrackTool = mInputState.Alt ? LC_TRACKTOOL_ROTATE_VIEW : GetTrackTool(NULL, NULL);
	lcTool Tool = GetMouseTool(TrackTool);

/*
	int x = view->mInputState.x;
	int y = view->mInputState.y;
	bool Control = view->mInputState.Control;
	bool Alt = view->mInputState.Alt;

	m_bTrackCancel = false;
	m_nDownX = x;
	m_nDownY = y;
	m_MouseTotalDelta = lcVector3(0, 0, 0);
	m_MouseSnapLeftover = lcVector3(0, 0, 0);
*/
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];

	const lcMatrix44& ModelView = mCamera->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(mCamera->mFOV, Aspect, mCamera->mNear, mCamera->mFar);
/*
	lcVector3 point = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];
*/

	switch (Tool)
	{
	case LC_TOOL_INSERT:
		{
			lcVector3 Position;
			lcVector4 AxisAngle;

			GetPieceInsertPosition(&Position, &AxisAngle);
			mProject->mActiveModel->AddPiece(mProject->m_pCurPiece, gMainWindow->mColorIndex, Position, AxisAngle);

			if (!mInputState.Control)
				gMainWindow->SetCurrentTool(LC_TOOL_SELECT);
		}
		break;

	case LC_TOOL_CAMERA:
		{
			lcVector3 Position = lcUnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f), ModelView, Projection, Viewport);
			lcVector3 TargetPosition = lcUnprojectPoint(lcVector3((float)mInputState.x + 1.0f, (float)mInputState.y - 1.0f, 0.9f), ModelView, Projection, Viewport);

			lcVector3 FrontVector(lcNormalize(TargetPosition - Position));
			lcVector3 UpVector(0, 0, 1);
			lcVector3 SideVector = lcCross(FrontVector, UpVector);

			if (fabsf(lcDot(UpVector, SideVector)) > 0.99f)
				SideVector = lcVector3(1, 0, 0);

			UpVector = lcCross(SideVector, FrontVector);
			UpVector.Normalize();

			mProject->mActiveModel->BeginCreateCameraTool(Position, TargetPosition, UpVector);

			StartTracking(LC_MOUSETRACK_LEFT, TrackTool);
		}
		break;

	case LC_TOOL_SELECT:
		{
			lcObjectSection ObjectSection = FindClosestObject(false);

			if (mInputState.Control)
				mProject->mActiveModel->ClearSelectionOrSetFocus(ObjectSection);
			else
				mProject->mActiveModel->SetFocus(ObjectSection);

			StartTracking(LC_MOUSETRACK_LEFT, TrackTool);
		}
		break;

	case LC_TOOL_ERASER:
		mProject->mActiveModel->RemoveObject(FindClosestObject(false).Object);
		break;
/*
	case LC_TOOL_PAINT:
		{
			Object* Closest = FindObjectFromPoint(view, x, y);

			if ((Closest != NULL) && (Closest->GetType() == LC_OBJECT_PIECE))
			{
				Piece* pPiece = (Piece*)Closest;

				if (pPiece->mColorIndex != gMainWindow->mColorIndex)
				{
					pPiece->SetColorIndex(gMainWindow->mColorIndex);

					SetModifiedFlag(true);
					CheckPoint("Painting");
					gMainWindow->UpdateFocusObject(GetFocusObject());
					gMainWindow->UpdateAllViews();
				}
			}
		} break;

	case LC_TOOL_LIGHT:
		{
			GLint max;
			int count = 0;
			Light *pLight;

			glGetIntegerv (GL_MAX_LIGHTS, &max);
			for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				count++;

			if (count == max)
				break;

			pLight = new Light (m_fTrack[0], m_fTrack[1], m_fTrack[2]);

			SelectAndFocusNone (false);

			pLight->CreateName(m_pLights);
			pLight->m_pNext = m_pLights;
			m_pLights = pLight;
			gMainWindow->UpdateFocusObject(pLight);
			pLight->Select (true, true, false);
			UpdateSelection ();

//			AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, (WPARAM)pNew, OT_PIECE);
			UpdateSelection();
			gMainWindow->UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Inserting");
		} break;

	case LC_TOOL_SPOTLIGHT:
		{
			GLint max;
			int count = 0;
			Light *pLight;

			glGetIntegerv (GL_MAX_LIGHTS, &max);
			for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				count++;

			if (count == max)
				break;

			lcVector3 tmp = lcUnprojectPoint(lcVector3(x+1.0f, y-1.0f, 0.9f), ModelView, Projection, Viewport);
			SelectAndFocusNone(false);
			StartTracking(LC_TRACK_START_LEFT);
			pLight = new Light (m_fTrack[0], m_fTrack[1], m_fTrack[2], tmp[0], tmp[1], tmp[2]);
			pLight->GetTarget ()->Select (true, true, false);
			pLight->m_pNext = m_pLights;
			m_pLights = pLight;
			UpdateSelection();
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdateFocusObject(pLight);
		}
		break;
*/
	case LC_TOOL_MOVE:
		{
			if (!mProject->mActiveModel->GetSelectedObjects().GetSize())
				break;

			mProject->mActiveModel->BeginMoveTool();

			StartTracking(LC_MOUSETRACK_LEFT, TrackTool);
		}
		break;
/*
	case LC_TOOL_ROTATE:
		{
			if (!mProject->mActiveModel->GetSelectedObjects().GetSize())
				break;

			mProject->mActiveModel->BeginRotateTool();

			StartTracking(LC_MOUSETRACK_LEFT, TrackTool);
			m_OverlayDelta = lcVector3(0.0f, 0.0f, 0.0f);
		}
		break;
		*/
	case LC_TOOL_ZOOM:
		mProject->mActiveModel->BeginEditCameraTool(LC_ACTION_ZOOM_CAMERA, lcVector3(0.0f, 0.0f, 0.0f));
		StartTracking(LC_MOUSETRACK_LEFT, TrackTool);
		break;

	case LC_TOOL_PAN:
		mProject->mActiveModel->BeginEditCameraTool(LC_ACTION_PAN_CAMERA, lcVector3(0.0f, 0.0f, 0.0f));
		StartTracking(LC_MOUSETRACK_LEFT, TrackTool);
		break;

	case LC_TOOL_ROTATE_VIEW:
		{
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
/*			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
					pPiece->CompareBoundingBox(bs);
*/			lcVector3 Center((bs[0] + bs[3]) / 2, (bs[1] + bs[4]) / 2, (bs[2] + bs[5]) / 2);

			mProject->mActiveModel->BeginEditCameraTool(LC_ACTION_ORBIT_CAMERA, Center);
			StartTracking(LC_MOUSETRACK_LEFT, TrackTool);
		}
		break;

	case LC_TOOL_ROLL:
		mProject->mActiveModel->BeginEditCameraTool(LC_ACTION_ROLL_CAMERA, lcVector3(0.0f, 0.0f, 0.0f));
		StartTracking(LC_MOUSETRACK_LEFT, TrackTool);
		break;

	case LC_TOOL_ZOOM_REGION:
		StartTracking(LC_MOUSETRACK_LEFT, TrackTool);
		break;
	}
}

void View::OnLeftButtonUp()
{
	StopTracking(mMouseTrack == LC_MOUSETRACK_LEFT);
}

void View::OnLeftButtonDoubleClick()
{
	gMainWindow->SetActiveView(this);

	lcObjectSection ObjectSection = FindClosestObject(false);

	if (mInputState.Control)
		mProject->mActiveModel->ClearSelectionOrSetFocus(ObjectSection);
	else
		mProject->mActiveModel->SetFocus(ObjectSection);
}

void View::OnMiddleButtonDown()
{
	if (mMouseTrack != LC_MOUSETRACK_NONE)
	{
		StopTracking(false);
		return;
	}

	gMainWindow->SetActiveView(this);

	lcTrackTool TrackTool = mInputState.Alt ? LC_TRACKTOOL_PAN : GetTrackTool(NULL, NULL);
	lcTool Tool = GetMouseTool(TrackTool);

	switch (Tool)
	{
	case LC_TOOL_PAN:
		mProject->mActiveModel->BeginEditCameraTool(LC_ACTION_PAN_CAMERA, lcVector3(0.0f, 0.0f, 0.0f));
		StartTracking(LC_MOUSETRACK_MIDDLE, TrackTool);
		break;

	default:
		break;
	}
}

void View::OnMiddleButtonUp()
{
	StopTracking(mMouseTrack == LC_MOUSETRACK_MIDDLE);
}

void View::OnRightButtonDown()
{
	if (mMouseTrack != LC_MOUSETRACK_NONE)
	{
		StopTracking(false);
		return;
	}

	gMainWindow->SetActiveView(this);

	lcTrackTool TrackTool = mInputState.Alt ? LC_TRACKTOOL_ZOOM : GetTrackTool(NULL, NULL);
	lcTool Tool = GetMouseTool(TrackTool);

	switch (Tool)
	{
	case LC_TOOL_MOVE:
		{
			if (!mProject->mActiveModel->GetSelectedObjects().GetSize())
				break;

			mProject->mActiveModel->BeginMoveTool();

			StartTracking(LC_MOUSETRACK_RIGHT, TrackTool);
		}
		break;
	/*
	case LC_TOOL_ROTATE:
		{
			if (!mProject->mActiveModel->GetSelectedObjects().GetSize())
				break;

			mProject->mActiveModel->BeginRotateTool();

			StartTracking(LC_MOUSETRACK_LEFT, TrackTool);
			m_OverlayDelta = lcVector3(0.0f, 0.0f, 0.0f);
		}
		break;
*/
	case LC_TOOL_ZOOM:
		mProject->mActiveModel->BeginEditCameraTool(LC_ACTION_ZOOM_CAMERA, lcVector3(0.0f, 0.0f, 0.0f));
		StartTracking(LC_MOUSETRACK_RIGHT, TrackTool);
		break;

	default:
		break;
	}
}

void View::OnRightButtonUp()
{
	if (mMouseTrack == LC_MOUSETRACK_NONE)
		ShowPopupMenu();

	StopTracking(mMouseTrack == LC_MOUSETRACK_RIGHT);
}

void View::OnMouseMove()
{
	lcTool CurrentTool = gMainWindow->GetCurrentTool();

	if (CurrentTool == LC_TOOL_INSERT)
	{
		gMainWindow->UpdateAllViews();

		return;
	}
	else if (mMouseTrack == LC_MOUSETRACK_NONE)
	{
		if (CurrentTool == LC_TOOL_SELECT || CurrentTool == LC_TOOL_MOVE || CurrentTool == LC_TOOL_ROTATE)
		{
			lcTrackTool TrackTool = GetTrackTool(NULL, NULL);

			if (TrackTool != mTrackTool)
			{
				mTrackTool = TrackTool;
				OnUpdateCursor();
				Redraw();
			}
		}

		return;
	}

	lcTool Tool = GetMouseTool(mTrackTool);
	const float MouseSensitivity = 1.0f / (21.0f - g_App->mPreferences->mMouseSensitivity);

	int Viewport[4] = { 0, 0, mWidth, mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];

	const lcMatrix44& ModelView = mCamera->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(mCamera->mFOV, Aspect, mCamera->mNear, mCamera->mFar);
/*
	lcVector3 tmp = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	ptx = tmp[0]; pty = tmp[1]; ptz = tmp[2];
*/

	switch (Tool)
	{
	case LC_TOOL_INSERT:
	case LC_TOOL_LIGHT:
		break;
/*
	case LC_TOOL_SPOTLIGHT:
		{
			float mouse = 10.0f * MouseSensitivity
			float delta[3] = { (ptx - m_fTrack[0])*mouse,
				(pty - m_fTrack[1])*mouse, (ptz - m_fTrack[2])*mouse };

			m_fTrack[0] = ptx;
			m_fTrack[1] = pty;
			m_fTrack[2] = ptz;

			Light* pLight = m_pLights;

			pLight->Move (1, m_bAnimation, false, delta[0], delta[1], delta[2]);
			pLight->UpdatePosition (1, m_bAnimation);

			gMainWindow->UpdateFocusObject(pLight);
			gMainWindow->UpdateAllViews();
		}
		break;
*/
	case LC_TOOL_CAMERA:
		{
			lcVector3 Position = lcUnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f), ModelView, Projection, Viewport);
			mProject->mActiveModel->UpdateCreateCameraTool(Position, gMainWindow->mAddKeys);
		}
		break;

	case LC_TOOL_SELECT:
		Redraw();
		break;
#if 0
	case LC_TOOL_MOVE:
		{
			lcCamera* Camera = view->mCamera;
			lcVector3 Distance;

			if ((m_OverlayActive && (m_OverlayMode != LC_OVERLAY_MOVE_XYZ)) /*|| (!Camera->IsSide())*/)
			{
				lcVector3 ScreenX = lcNormalize(lcCross(Camera->mTargetPosition - Camera->mPosition, Camera->mUpVector));
				lcVector3 ScreenY = Camera->mUpVector;
				lcVector3 Dir1(0.0f, 0.0f, 0.0f), Dir2(0.0f, 0.0f, 0.0f);
				bool SingleDir = true;

				int OverlayMode;

				if (m_OverlayActive && (m_OverlayMode != LC_OVERLAY_MOVE_XYZ))
					OverlayMode = m_OverlayMode;
				else if (m_nTracking == LC_TRACK_LEFT)
					OverlayMode = LC_OVERLAY_MOVE_XY;
				else
					OverlayMode = LC_OVERLAY_MOVE_Z;

				switch (OverlayMode)
				{
				case LC_OVERLAY_MOVE_X:
					Dir1 = lcVector3(1, 0, 0);
					break;
				case LC_OVERLAY_MOVE_Y:
					Dir1 = lcVector3(0, 1, 0);
					break;
				case LC_OVERLAY_MOVE_Z:
					Dir1 = lcVector3(0, 0, 1);
					break;
				case LC_OVERLAY_MOVE_XY:
					Dir1 = lcVector3(1, 0, 0);
					Dir2 = lcVector3(0, 1, 0);
					SingleDir = false;
					break;
				case LC_OVERLAY_MOVE_XZ:
					Dir1 = lcVector3(1, 0, 0);
					Dir2 = lcVector3(0, 0, 1);
					SingleDir = false;
					break;
				case LC_OVERLAY_MOVE_YZ:
					Dir1 = lcVector3(0, 1, 0);
					Dir2 = lcVector3(0, 0, 1);
					SingleDir = false;
					break;
				}

				// Transform the translation axis.
				lcVector3 Axis1 = Dir1;
				lcVector3 Axis2 = Dir2;

				if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
				{
					Object* Focus = GetFocusObject();

					if ((Focus != NULL) && Focus->IsPiece())
					{
						const lcMatrix44& ModelWorld = ((Piece*)Focus)->mModelWorld;

						Axis1 = lcMul30(Dir1, ModelWorld);
						Axis2 = lcMul30(Dir2, ModelWorld);
					}
				}

				// Find out what direction the mouse is going to move stuff.
				lcVector3 MoveX, MoveY;

				if (SingleDir)
				{
					float dx1 = lcDot(ScreenX, Axis1);
					float dy1 = lcDot(ScreenY, Axis1);

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
				}
				else
				{
					float dx1 = lcDot(ScreenX, Axis1);
					float dy1 = lcDot(ScreenY, Axis1);
					float dx2 = lcDot(ScreenX, Axis2);
					float dy2 = lcDot(ScreenY, Axis2);

					if (fabsf(dx1) > fabsf(dx2))
					{
						if (dx1 >= 0.0f)
							MoveX = Dir1;
						else
							MoveX = -Dir1;

						if (dy2 >= 0.0f)
							MoveY = Dir2;
						else
							MoveY = -Dir2;
					}
					else
					{
						if (dx2 >= 0.0f)
							MoveX = Dir2;
						else
							MoveX = -Dir2;

						if (dy1 > 0.0f)
							MoveY = Dir1;
						else
							MoveY = -Dir1;
					}
				}

				MoveX *= (float)(x - m_nDownX) * 0.25f / (21 - m_nMouse);
				MoveY *= (float)(y - m_nDownY) * 0.25f / (21 - m_nMouse);

				Distance = MoveX + MoveY;
			}
			else
			{
				// 3D movement.
				lcVector3 ScreenZ = lcNormalize(Camera->mTargetPosition - Camera->mPosition);
				lcVector3 ScreenX = lcCross(ScreenZ, Camera->mUpVector);
				lcVector3 ScreenY = Camera->mUpVector;

				if (m_nTracking == LC_TRACK_LEFT)
				{
					lcVector3 MoveX, MoveY;

					MoveX = ScreenX * (float)(x - m_nDownX) * 0.25f / (float)(21 - m_nMouse);
					MoveY = ScreenY * (float)(y - m_nDownY) * 0.25f / (float)(21 - m_nMouse);

					Distance = MoveX + MoveY;
				}
				else
				{
					Distance = ScreenZ * (float)(y - m_nDownY) * 0.25f / (float)(21 - m_nMouse);
				}
			}

			Distance = GetMoveDistance(Distance, true, true);

			mActiveModel->UpdateMoveTool(Distance, m_nCurStep, m_bAddKeys);

			if (m_OverlayActive)
			{
				if (!GetFocusPosition(m_OverlayCenter))
					GetSelectionCenter(m_OverlayCenter);
			}

			if (m_nTracking != LC_TRACK_NONE)
				UpdateOverlayScale();
		}
		break;

	case LC_TOOL_ROTATE:
		{
			lcCamera* Camera = gMainWindow->mActiveView->mCamera;
			bool Redraw;

			if ((m_OverlayActive && (m_OverlayMode != LC_OVERLAY_ROTATE_XYZ)) /*|| (!Camera->IsSide())*/)
			{
				lcVector3 ScreenX = lcNormalize(lcCross(Camera->mTargetPosition - Camera->mPosition, Camera->mUpVector));
				lcVector3 ScreenY = Camera->mUpVector;
				lcVector3 Dir1, Dir2;
				bool SingleDir = true;

				int OverlayMode;

				if (m_OverlayActive && (m_OverlayMode != LC_OVERLAY_ROTATE_XYZ))
					OverlayMode = m_OverlayMode;
				else if (m_nTracking == LC_TRACK_LEFT)
					OverlayMode = LC_OVERLAY_ROTATE_XY;
				else
					OverlayMode = LC_OVERLAY_ROTATE_Z;

				switch (OverlayMode)
				{
				case LC_OVERLAY_ROTATE_X:
					Dir1 = lcVector3(1, 0, 0);
					break;
				case LC_OVERLAY_ROTATE_Y:
					Dir1 = lcVector3(0, 1, 0);
					break;
				case LC_OVERLAY_ROTATE_Z:
					Dir1 = lcVector3(0, 0, 1);
					break;
				case LC_OVERLAY_ROTATE_XY:
					Dir1 = lcVector3(1, 0, 0);
					Dir2 = lcVector3(0, 1, 0);
					SingleDir = false;
					break;
				case LC_OVERLAY_ROTATE_XZ:
					Dir1 = lcVector3(1, 0, 0);
					Dir2 = lcVector3(0, 0, 1);
					SingleDir = false;
					break;
				case LC_OVERLAY_ROTATE_YZ:
					Dir1 = lcVector3(0, 1, 0);
					Dir2 = lcVector3(0, 0, 1);
					SingleDir = false;
					break;
				default:
					Dir1 = lcVector3(1, 0, 0);
					break;
				}

				// Find out what direction the mouse is going to move stuff.
				lcVector3 MoveX, MoveY;

				if (SingleDir)
				{
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
				}
				else
				{
					float dx1 = lcDot(ScreenX, Dir1);
					float dy1 = lcDot(ScreenY, Dir1);
					float dx2 = lcDot(ScreenX, Dir2);
					float dy2 = lcDot(ScreenY, Dir2);

					if (fabsf(dx1) > fabsf(dx2))
					{
						if (dx1 >= 0.0f)
							MoveX = Dir1;
						else
							MoveX = -Dir1;

						if (dy2 >= 0.0f)
							MoveY = Dir2;
						else
							MoveY = -Dir2;
					}
					else
					{
						if (dx2 >= 0.0f)
							MoveX = Dir2;
						else
							MoveX = -Dir2;

						if (dy1 > 0.0f)
							MoveY = Dir1;
						else
							MoveY = -Dir1;
					}
				}

				MoveX *= (float)(x - m_nDownX) * 36.0f / (21 - m_nMouse);
				MoveY *= (float)(y - m_nDownY) * 36.0f / (21 - m_nMouse);

				m_nDownX = x;
				m_nDownY = y;

				lcVector3 Delta = MoveX + MoveY + m_MouseSnapLeftover;
				Redraw = RotateSelectedObjects(Delta, m_MouseSnapLeftover, true, true);
				m_MouseTotalDelta += Delta;
			}
			else
			{
				// 3D movement.
				lcVector3 ScreenZ = lcNormalize(Camera->mTargetPosition - Camera->mPosition);
				lcVector3 ScreenX = lcCross(ScreenZ, Camera->mUpVector);
				lcVector3 ScreenY = Camera->mUpVector;

				lcVector3 Delta;

				if (m_nTracking == LC_TRACK_LEFT)
				{
					lcVector3 MoveX, MoveY;

					MoveX = ScreenX * (float)(x - m_nDownX) * 36.0f / (float)(21 - m_nMouse);
					MoveY = ScreenY * (float)(y - m_nDownY) * 36.0f / (float)(21 - m_nMouse);

					Delta = MoveX + MoveY + m_MouseSnapLeftover;
				}
				else
				{
					lcVector3 MoveZ;

					MoveZ = ScreenZ * (float)(y - m_nDownY) * 36.0f / (float)(21 - m_nMouse);

					Delta = MoveZ + m_MouseSnapLeftover;
				}

				m_nDownX = x;
				m_nDownY = y;

				Redraw = RotateSelectedObjects(Delta, m_MouseSnapLeftover, true, true);
				m_MouseTotalDelta += Delta;
			}

			gMainWindow->UpdateFocusObject(GetFocusObject());
			if (Redraw)
				gMainWindow->UpdateAllViews();
		}
		break;
#endif
	case LC_TOOL_ERASER:
		break;

	case LC_TOOL_PAINT:
		break;

	case LC_TOOL_ZOOM:
		{
			float Distance = 2.0f * MouseSensitivity * (mInputState.y - mMouseDownY);

			mProject->mActiveModel->UpdateEditCameraTool(LC_ACTION_ZOOM_CAMERA, Distance, 0.0f, gMainWindow->mAddKeys);
		}
		break;

	case LC_TOOL_PAN:
		{
			float DistanceX = 2.0f * MouseSensitivity * (mInputState.x - mMouseDownX);
			float DistanceY = -2.0f * MouseSensitivity * (mInputState.y - mMouseDownY);

			mProject->mActiveModel->UpdateEditCameraTool(LC_ACTION_PAN_CAMERA, DistanceX, DistanceY, gMainWindow->mAddKeys);
		}
		break;

	case LC_TOOL_ROTATE_VIEW:
		{
			if (mTrackTool == LC_TRACKTOOL_ROTATE_VIEW_Z)
			{
				float Angle = -2.0f * MouseSensitivity * (mInputState.x - mMouseDownX) * LC_DTOR;

				mProject->mActiveModel->UpdateEditCameraTool(LC_ACTION_ROLL_CAMERA, Angle, 0.0f, gMainWindow->mAddKeys);
			}
			else
			{
				float AngleX = (mTrackTool == LC_TRACKTOOL_ROTATE_VIEW_Y) ? 0.0f : -0.1f * MouseSensitivity * (mInputState.x - mMouseDownX);
				float AngleY = (mTrackTool == LC_TRACKTOOL_ROTATE_VIEW_X) ? 0.0f : -0.1f * MouseSensitivity * (mInputState.y - mMouseDownY);

				mProject->mActiveModel->UpdateEditCameraTool(LC_ACTION_ORBIT_CAMERA, AngleX, AngleY, gMainWindow->mAddKeys);
			}
		}
		break;

	case LC_TOOL_ROLL:
		{
			float Angle = -2.0f * MouseSensitivity * (mInputState.x - mMouseDownX) * LC_DTOR;

			mProject->mActiveModel->UpdateEditCameraTool(LC_ACTION_ROLL_CAMERA, Angle, 0.0f, gMainWindow->mAddKeys);
		}
		break;

	case LC_TOOL_ZOOM_REGION:
		Redraw();
		break;
	}
}

void View::OnMouseWheel(float Direction)
{
	gMainWindow->SetActiveView(this);

	mProject->ZoomActiveView((int)(10 * Direction));
}

void View::DrawMouseTracking()
{
	lcTool CurrentTool = gMainWindow->GetCurrentTool();

/*
	if (mDropPiece)
		return;
*/

	if ((CurrentTool == LC_TOOL_SELECT || CurrentTool == LC_TOOL_ZOOM_REGION) && mMouseTrack == LC_MOUSETRACK_LEFT)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375f, 0.375f, 0.0f);

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
		Right = lcMin(Right, mWidth - 1);
		Bottom = lcMax(Bottom, 0.0f);
		Top = lcMin(Top, mHeight - 1);

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

		glVertexPointer(2, GL_FLOAT, 0, Verts);

		glColor4f(0.25f, 0.25f, 1.0f, 1.0f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

		glColor4f(0.25f, 0.25f, 1.0f, 0.25f);
		glDrawArrays(GL_TRIANGLE_STRIP, 10, 4);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		return;
	}

	if (CurrentTool == LC_TOOL_ROTATE_VIEW)
	{
		int x, y, w, h;

		x = 0;
		y = 0;
		w = mWidth;
		h = mHeight;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375f, 0.375f, 0.0f);

		glDisable(GL_DEPTH_TEST);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

		// Draw circle.
		float verts[32][2];

		float r = lcMin(w, h) * 0.35f;
		float cx = x + w / 2.0f;
		float cy = y + h / 2.0f;

		for (int i = 0; i < 32; i++)
		{
			verts[i][0] = cosf((float)i / 32.0f * (2.0f * LC_PI)) * r + cx;
			verts[i][1] = sinf((float)i / 32.0f * (2.0f * LC_PI)) * r + cy;
		}

		glVertexPointer(2, GL_FLOAT, 0, verts);
		glDrawArrays(GL_LINE_LOOP, 0, 32);

		const float OverlayCameraSquareSize = lcMax(8.0f, (w+h)/200);

		float Squares[16][2] =
		{
			{ cx + OverlayCameraSquareSize, cy + r + OverlayCameraSquareSize },
			{ cx - OverlayCameraSquareSize, cy + r + OverlayCameraSquareSize },
			{ cx - OverlayCameraSquareSize, cy + r - OverlayCameraSquareSize },
			{ cx + OverlayCameraSquareSize, cy + r - OverlayCameraSquareSize },
			{ cx + OverlayCameraSquareSize, cy - r + OverlayCameraSquareSize },
			{ cx - OverlayCameraSquareSize, cy - r + OverlayCameraSquareSize },
			{ cx - OverlayCameraSquareSize, cy - r - OverlayCameraSquareSize },
			{ cx + OverlayCameraSquareSize, cy - r - OverlayCameraSquareSize },
			{ cx + r + OverlayCameraSquareSize, cy + OverlayCameraSquareSize },
			{ cx + r - OverlayCameraSquareSize, cy + OverlayCameraSquareSize },
			{ cx + r - OverlayCameraSquareSize, cy - OverlayCameraSquareSize },
			{ cx + r + OverlayCameraSquareSize, cy - OverlayCameraSquareSize },
			{ cx - r + OverlayCameraSquareSize, cy + OverlayCameraSquareSize },
			{ cx - r - OverlayCameraSquareSize, cy + OverlayCameraSquareSize },
			{ cx - r - OverlayCameraSquareSize, cy - OverlayCameraSquareSize },
			{ cx - r + OverlayCameraSquareSize, cy - OverlayCameraSquareSize }
		};

		glVertexPointer(2, GL_FLOAT, 0, Squares);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		glDrawArrays(GL_LINE_LOOP, 4, 4);
		glDrawArrays(GL_LINE_LOOP, 8, 4);
		glDrawArrays(GL_LINE_LOOP, 12, 4);

		glEnable(GL_DEPTH_TEST);

		return;
	}

	if (CurrentTool != LC_TOOL_MOVE && CurrentTool != LC_TOOL_SELECT && CurrentTool != LC_TOOL_ROTATE)
		return;

	if (!mProject->mActiveModel->GetSelectedObjects().GetSize())
		return;

	float OverlayScale;
	lcVector3 OverlayCenter;

	lcTrackTool TrackTool = GetTrackTool(&OverlayScale, &OverlayCenter);

	if (mTrackTool != LC_TRACKTOOL_NONE)
		TrackTool = mTrackTool;

	if (CurrentTool == LC_TOOL_MOVE || CurrentTool == LC_TOOL_SELECT)
	{
		const float OverlayMovePlaneSize = 0.5f * OverlayScale;
		const float OverlayMoveArrowSize = 1.5f * OverlayScale;
		const float OverlayMoveArrowCapSize = 0.9f * OverlayScale;
		const float OverlayMoveArrowCapRadius = 0.1f * OverlayScale;
		const float OverlayMoveArrowBodyRadius = 0.05f * OverlayScale;
		const float OverlayRotateArrowStart = 1.0f * OverlayScale;
		const float OverlayRotateArrowEnd = 1.5f * OverlayScale;
		const float OverlayRotateArrowCenter = 1.2f * OverlayScale;

		glDisable(GL_DEPTH_TEST);

		// Find the rotation from the focused piece if relative snap is enabled.
		class Object* Focus = NULL;
		lcVector4 Rot(0, 0, 1, 0);
/*
		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPiece())
				Rot = ((Piece*)Focus)->mRotation;
			else
				Focus = NULL;
		}
*/
		// Plane translation quad.
		if ((TrackTool == LC_TRACKTOOL_MOVE_XY) || (TrackTool == LC_TRACKTOOL_MOVE_XZ) || (TrackTool == LC_TRACKTOOL_MOVE_YZ))
		{
			glPushMatrix();
			glTranslatef(OverlayCenter[0], OverlayCenter[1], OverlayCenter[2]);

			if (Focus)
				glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

			if (TrackTool == LC_TRACKTOOL_MOVE_XZ)
				glRotatef(90.0f, 0.0f, 0.0f, -1.0f);
			else if (TrackTool == LC_TRACKTOOL_MOVE_XY)
				glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);

			glColor4f(0.8f, 0.8f, 0.0f, 0.3f);

			float Verts[4][3] =
			{
				{ 0.0f, 0.0f, 0.0f },
				{ 0.0f, OverlayMovePlaneSize, 0.0f },
				{ 0.0f, OverlayMovePlaneSize, OverlayMovePlaneSize },
				{ 0.0f, 0.0f, OverlayMovePlaneSize }
			};

			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glDisable(GL_BLEND);

			glPopMatrix();
		}

		// Translation arrows.
		lcVector3 MoveArrowVerts[11];

		MoveArrowVerts[0] = lcVector3(0.0f, 0.0f, 0.0f);
		MoveArrowVerts[1] = lcVector3(OverlayMoveArrowSize, 0.0f, 0.0f);

		for (int j = 0; j < 9; j++)
		{
			float y = cosf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
			float z = sinf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
			MoveArrowVerts[j + 2] = lcVector3(OverlayMoveArrowCapSize, y, z);
		}

		for (int i = 0; i < 3; i++)
		{
			switch (i)
			{
			case 0:
				if ((TrackTool == LC_TRACKTOOL_MOVE_X) || (TrackTool == LC_TRACKTOOL_MOVE_XY) || (TrackTool == LC_TRACKTOOL_MOVE_XZ))
					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				else if (TrackTool == LC_TRACKTOOL_NONE || mMouseTrack == LC_MOUSETRACK_NONE)
					glColor4f(0.8f, 0.0f, 0.0f, 1.0f);
				else
					continue;
				break;
			case 1:
				if ((TrackTool == LC_TRACKTOOL_MOVE_Y) || (TrackTool == LC_TRACKTOOL_MOVE_XY) || (TrackTool == LC_TRACKTOOL_MOVE_YZ))
					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				else if (TrackTool == LC_TRACKTOOL_NONE || mMouseTrack == LC_MOUSETRACK_NONE)
					glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
				else
					continue;
				break;
			case 2:
				if ((TrackTool == LC_TRACKTOOL_MOVE_Z) || (TrackTool == LC_TRACKTOOL_MOVE_XZ) || (TrackTool == LC_TRACKTOOL_MOVE_YZ))
					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				else if (TrackTool == LC_TRACKTOOL_NONE || mMouseTrack == LC_MOUSETRACK_NONE)
					glColor4f(0.0f, 0.0f, 0.8f, 1.0f);
				else
					continue;
				break;
			}

			glPushMatrix();
			glTranslatef(OverlayCenter[0], OverlayCenter[1], OverlayCenter[2]);

			if (Focus)
				glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

			if (i == 1)
				glMultMatrixf(lcMatrix44(lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 1, 0), lcVector4(0, 0, 0, 1)));
			else if (i == 2)
				glMultMatrixf(lcMatrix44(lcVector4(0, 0, 1, 0), lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 0, 1)));

			glVertexPointer(3, GL_FLOAT, 0, MoveArrowVerts);
			glDrawArrays(GL_LINES, 0, 2);
			glDrawArrays(GL_TRIANGLE_FAN, 1, 10);

			glPopMatrix();
		}

		// Rotation arrows.
		if (CurrentTool == LC_TOOL_SELECT && mMouseTrack == LC_MOUSETRACK_NONE)
		{
			for (int i = 0; i < 3; i++)
			{
				glPushMatrix();
				glTranslatef(OverlayCenter[0], OverlayCenter[1], OverlayCenter[2]);

				if (Focus)
					glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

				if (i == 1)
					glMultMatrixf(lcMatrix44(lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 1, 0), lcVector4(0, 0, 0, 1)));
				else if (i == 2)
					glMultMatrixf(lcMatrix44(lcVector4(0, 0, 1, 0), lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 0, 1)));

				switch (i)
				{
				case 0:
					if (TrackTool == LC_TRACKTOOL_ROTATE_X)
						glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
					else
						glColor4f(0.8f, 0.0f, 0.0f, 1.0f);
					break;
				case 1:
					if (TrackTool == LC_TRACKTOOL_ROTATE_Y)
						glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
					else
						glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
					break;
				case 2:
					if (TrackTool == LC_TRACKTOOL_ROTATE_Z)
						glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
					else
						glColor4f(0.0f, 0.0f, 0.8f, 1.0f);
					break;
				}

				lcVector3 Verts[18];
				glVertexPointer(3, GL_FLOAT, 0, Verts);

				for (int j = 0; j < 9; j++)
				{
					const float Radius1 = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter - OverlayMoveArrowBodyRadius;
					const float Radius2 = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter + OverlayMoveArrowBodyRadius;
					float x = cosf(LC_2PI / 4 * j / 8);
					float y = sinf(LC_2PI / 4 * j / 8);

					Verts[j * 2 + 0] = lcVector3(0.0f, OverlayRotateArrowCenter + x * Radius1, OverlayRotateArrowCenter + y * Radius1);
					Verts[j * 2 + 1] = lcVector3(0.0f, OverlayRotateArrowCenter + x * Radius2, OverlayRotateArrowCenter + y * Radius2);
				}

				glDrawArrays(GL_TRIANGLE_STRIP, 0, 18);

				for (int j = 0; j < 9; j++)
				{
					const float Radius = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter;
					float x = cosf(LC_2PI / 4 * j / 8);
					float y = sinf(LC_2PI / 4 * j / 8);

					Verts[j * 2 + 0] = lcVector3(-OverlayMoveArrowBodyRadius, OverlayRotateArrowCenter + x * Radius, OverlayRotateArrowCenter + y * Radius);
					Verts[j * 2 + 1] = lcVector3( OverlayMoveArrowBodyRadius, OverlayRotateArrowCenter + x * Radius, OverlayRotateArrowCenter + y * Radius);
				}

				glDrawArrays(GL_TRIANGLE_STRIP, 0, 18);

				Verts[0] = lcVector3(0.0f, OverlayRotateArrowEnd - OverlayMoveArrowCapRadius, OverlayRotateArrowStart);

				for (int j = 0; j < 9; j++)
				{
					float x = cosf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
					float y = sinf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
					Verts[j + 1] = lcVector3(x, OverlayRotateArrowEnd - OverlayMoveArrowCapRadius + y, OverlayRotateArrowCenter);
				}

				glDrawArrays(GL_TRIANGLE_FAN, 0, 10);

				Verts[0] = lcVector3(0.0f, OverlayRotateArrowStart, OverlayRotateArrowEnd - OverlayMoveArrowCapRadius);

				for (int j = 0; j < 9; j++)
				{
					float x = cosf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
					float y = sinf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
					Verts[j + 1] = lcVector3(x, OverlayRotateArrowCenter, OverlayRotateArrowEnd - OverlayMoveArrowCapRadius + y);
				}

				glDrawArrays(GL_TRIANGLE_FAN, 0, 10);

				glPopMatrix();
			}
		}

		glEnable(GL_DEPTH_TEST);
	}
/*
	if (m_nCurAction == LC_TOOL_ROTATE || (m_nCurAction == LC_TOOL_SELECT && m_nTracking != LC_TRACK_NONE && m_OverlayMode >= LC_OVERLAY_ROTATE_X && m_OverlayMode <= LC_OVERLAY_ROTATE_XYZ))
	{
		const float OverlayRotateRadius = 2.0f;

		glDisable(GL_DEPTH_TEST);

		lcCamera* Camera = view->mCamera;
		int j;

		// Find the rotation from the focused piece if relative snap is enabled.
		Object* Focus = NULL;
		lcVector4 Rot(0, 0, 1, 0);

		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPiece())
				Rot = ((Piece*)Focus)->mRotation;
			else
				Focus = NULL;
		}

		bool HasAngle = false;

		// Draw a disc showing the rotation amount.
		if (m_MouseTotalDelta.LengthSquared() != 0.0f && (m_nTracking != LC_TRACK_NONE))
		{
			lcVector4 Rotation;
			float Angle, Step;

			HasAngle = true;

			switch (m_OverlayMode)
			{
			case LC_OVERLAY_ROTATE_X:
				glColor4f(0.8f, 0.0f, 0.0f, 0.3f);
				Angle = m_MouseTotalDelta[0];
				Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
				break;
			case LC_OVERLAY_ROTATE_Y:
				glColor4f(0.0f, 0.8f, 0.0f, 0.3f);
				Angle = m_MouseTotalDelta[1];
				Rotation = lcVector4(90.0f, 0.0f, 0.0f, 1.0f);
				break;
			case LC_OVERLAY_ROTATE_Z:
				glColor4f(0.0f, 0.0f, 0.8f, 0.3f);
				Angle = m_MouseTotalDelta[2];
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
				glPushMatrix();
				glTranslatef(OverlayCenter[0], OverlayCenter[1], OverlayCenter[2]);

				if (Focus)
					glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

				glRotatef(Rotation[0], Rotation[1], Rotation[2], Rotation[3]);

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

				glPopMatrix();
			}
		}

		glPushMatrix();

		lcMatrix44 Mat = lcMatrix44AffineInverse(Camera->mWorldView);
		Mat.SetTranslation(OverlayCenter);

		// Draw the circles.
		if (m_nCurAction == LC_TOOL_ROTATE && !HasAngle && m_nTracking == LC_TRACK_NONE)
		{
			lcVector3 Verts[32];

			for (j = 0; j < 32; j++)
			{
				lcVector3 Pt;

				Pt[0] = cosf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
				Pt[1] = sinf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
				Pt[2] = 0.0f;

				Verts[j] = lcMul31(Pt, Mat);
			}

			glColor4f(0.1f, 0.1f, 0.1f, 1.0f);

			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINE_LOOP, 0, 32);
		}

		lcVector3 ViewDir = Camera->mTargetPosition - Camera->mPosition;
		ViewDir.Normalize();

		// Transform ViewDir to local space.
		if (Focus)
		{
			lcMatrix44 RotMat = lcMatrix44FromAxisAngle(lcVector3(Rot[0], Rot[1], Rot[2]), -Rot[3] * LC_DTOR);

			ViewDir = lcMul30(ViewDir, RotMat);
		}

		glTranslatef(OverlayCenter[0], OverlayCenter[1], OverlayCenter[2]);

		if (Focus)
			glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

		// Draw each axis circle.
		for (int i = 0; i < 3; i++)
		{
			if (m_OverlayMode == LC_OVERLAY_ROTATE_X + i)
			{
				glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
			}
			else
			{
				if (m_nCurAction != LC_TOOL_ROTATE || HasAngle || m_nTracking != LC_TRACK_NONE)
					continue;

				switch (i)
				{
				case 0:
					glColor4f(0.8f, 0.0f, 0.0f, 1.0f);
					break;
				case 1:
					glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
					break;
				case 2:
					glColor4f(0.0f, 0.0f, 0.8f, 1.0f);
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

				if (m_nCurAction != LC_TOOL_ROTATE || HasAngle || m_nTracking != LC_TRACK_NONE || lcDot(ViewDir, v1 + v2) <= 0.0f)
				{
					Verts[NumVerts++] = v1 * (OverlayRotateRadius * OverlayScale);
					Verts[NumVerts++] = v2 * (OverlayRotateRadius * OverlayScale);
				}
			}

			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINES, 0, NumVerts);
		}

		glPopMatrix();

		// Draw tangent vector.
		if (m_nTracking != LC_TRACK_NONE)
		{
			if ((m_OverlayMode == LC_OVERLAY_ROTATE_X) || (m_OverlayMode == LC_OVERLAY_ROTATE_Y) || (m_OverlayMode == LC_OVERLAY_ROTATE_Z))
			{
				const float OverlayRotateArrowSize = 1.5f;
				const float OverlayRotateArrowCapSize = 0.25f;

				lcVector4 Rotation;
				float Angle;

				switch (m_OverlayMode)
				{
				case LC_OVERLAY_ROTATE_X:
					Angle = m_MouseTotalDelta[0];
					Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
					break;
				case LC_OVERLAY_ROTATE_Y:
					Angle = m_MouseTotalDelta[1];
					Rotation = lcVector4(90.0f, 0.0f, 0.0f, 1.0f);
					break;
				case LC_OVERLAY_ROTATE_Z:
					Angle = m_MouseTotalDelta[2];
					Rotation = lcVector4(90.0f, 0.0f, -1.0f, 0.0f);
					break;
				default:
					Angle = 0.0f;
					Rotation = lcVector4(0.0f, 0.0f, 1.0f, 0.0f);
					break;
				};

				glPushMatrix();
				glTranslatef(OverlayCenter[0], OverlayCenter[1], OverlayCenter[2]);

				if (Focus)
					glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

				glRotatef(Rotation[0], Rotation[1], Rotation[2], Rotation[3]);

				glColor4f(0.8f, 0.8f, 0.0f, 1.0f);

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

				glPopMatrix();

				// Draw text.
				int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
				float Aspect = (float)Viewport[2]/(float)Viewport[3];

				const lcMatrix44& ModelView = Camera->mWorldView;
				lcMatrix44 Projection = lcMatrix44Perspective(Camera->mFOV, Aspect, Camera->mNear, Camera->mFar);

				lcVector3 ScreenPos = lcProjectPoint(m_OverlayCenter, ModelView, Projection, Viewport);

				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glOrtho(0, Viewport[2], 0, Viewport[3], -1, 1);
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
				glTranslatef(0.375, 0.375, 0.0);

				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				m_pScreenFont->MakeCurrent();
				glEnable(GL_TEXTURE_2D);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);

				char buf[32];
				sprintf(buf, "[%.2f]", fabsf(Angle));

				int cx, cy;
				m_pScreenFont->GetStringDimensions(&cx, &cy, buf);

				glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				m_pScreenFont->PrintText(ScreenPos[0] - Viewport[0] - (cx / 2), ScreenPos[1] - Viewport[1] + (cy / 2), 0.0f, buf);

				glDisable(GL_BLEND);
				glDisable(GL_TEXTURE_2D);

				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}

		glEnable(GL_DEPTH_TEST);
	}
*/
}

void View::DrawViewport()
{
	const char* CameraName = mCamera->mName;

	if (!CameraName[0])
		return;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0f);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	mDefaultFont->MakeCurrent();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	mDefaultFont->PrintText(3.0f, (float)mHeight - 1.0f - 6.0f, 0.0f, CameraName);

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
}

lcObjectSection View::FindClosestObject(bool PiecesOnly) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];

	const lcMatrix44& ModelView = mCamera->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(mCamera->mFOV, Aspect, mCamera->mNear, mCamera->mFar);

	float x = (float)mInputState.x;
	float y = (float)mInputState.y;

	lcObjectHitTest HitTest;

	HitTest.PiecesOnly = PiecesOnly;
	HitTest.Camera = mCamera;
	HitTest.Start = lcUnprojectPoint(lcVector3(x, y, 0.0f), ModelView, Projection, Viewport);
	HitTest.End = lcUnprojectPoint(lcVector3(x, y, 1.0f), ModelView, Projection, Viewport);
	HitTest.Distance = FLT_MAX;
	HitTest.ObjectSection.Object = NULL;
	HitTest.ObjectSection.Section = 0;

	mProject->mActiveModel->FindClosestObject(HitTest);

	return HitTest.ObjectSection;
}

void View::FindObjectsInRectangle(float x1, float y1, float x2, float y2, lcArray<lcObjectSection>& Objects) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];

	const lcMatrix44& ModelView = mCamera->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(mCamera->mFOV, Aspect, mCamera->mNear, mCamera->mFar);

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
		lcVector3(Left, Top, 0), lcVector3(Left, Bottom, 0), lcVector3(Right, Bottom, 0),
		lcVector3(Right, Top, 0), lcVector3(Left, Top, 1), lcVector3(Right, Bottom, 1)
	};

	lcUnprojectPoints(Corners, 6, ModelView, Projection, Viewport);

	lcVector3 PlaneNormals[6];

	PlaneNormals[0] = lcNormalize(lcCross(Corners[4] - Corners[0], Corners[1] - Corners[0])); // Left
	PlaneNormals[1] = lcNormalize(lcCross(Corners[5] - Corners[2], Corners[3] - Corners[2])); // Right
	PlaneNormals[2] = lcNormalize(lcCross(Corners[3] - Corners[0], Corners[4] - Corners[0])); // Top
	PlaneNormals[3] = lcNormalize(lcCross(Corners[1] - Corners[2], Corners[5] - Corners[2])); // Bottom
	PlaneNormals[4] = lcNormalize(lcCross(Corners[1] - Corners[0], Corners[3] - Corners[0])); // Front
	PlaneNormals[5] = lcNormalize(lcCross(Corners[1] - Corners[2], Corners[3] - Corners[2])); // Back

	lcObjectBoxTest BoxTest;

	BoxTest.Camera = mCamera;
	BoxTest.Planes[0] = lcVector4(PlaneNormals[0], -lcDot(PlaneNormals[0], Corners[0]));
	BoxTest.Planes[1] = lcVector4(PlaneNormals[1], -lcDot(PlaneNormals[1], Corners[5]));
	BoxTest.Planes[2] = lcVector4(PlaneNormals[2], -lcDot(PlaneNormals[2], Corners[0]));
	BoxTest.Planes[3] = lcVector4(PlaneNormals[3], -lcDot(PlaneNormals[3], Corners[5]));
	BoxTest.Planes[4] = lcVector4(PlaneNormals[4], -lcDot(PlaneNormals[4], Corners[0]));
	BoxTest.Planes[5] = lcVector4(PlaneNormals[5], -lcDot(PlaneNormals[5], Corners[5]));

	mProject->mActiveModel->FindObjectsInBox(BoxTest);
}

void View::GetPieceInsertPosition(lcVector3* Position, lcVector4* AxisAngle)
{
	lcObjectSection ObjectSection = FindClosestObject(true);
	lcPiece* Piece = NULL;

	if (ObjectSection.Object)
		Piece = ObjectSection.Object->GetPiece(ObjectSection.Section);

	if (Piece)
	{
		lcVector3 Dist(0, 0, Piece->mPieceInfo->m_fDimensions[2] - mProject->m_pCurPiece->m_fDimensions[5]);
		Dist = mProject->SnapVector(Dist);

		*Position = lcMul31(Dist, Piece->mModelWorld);
		*AxisAngle = Piece->mAxisAngle;

		return;
	}

	int Viewport[4] = { 0, 0, mWidth, mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];

	const lcMatrix44& ModelView = mCamera->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(mCamera->mFOV, Aspect, mCamera->mNear, mCamera->mFar);

	lcVector3 ClickPoints[2] = { lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f), lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f) };
	lcUnprojectPoints(ClickPoints, 2, ModelView, Projection, Viewport);

	lcVector3 Intersection;
	if (lcLinePlaneIntersection(&Intersection, ClickPoints[0], ClickPoints[1], lcVector4(0, 0, 1, mProject->m_pCurPiece->m_fDimensions[5])))
	{
		Intersection = mProject->SnapVector(Intersection);
		*Position = Intersection;
		*AxisAngle = lcVector4(0, 0, 1, 0);
		return;
	}

	*Position = lcUnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f), ModelView, Projection, Viewport);
	*AxisAngle = lcVector4(0, 0, 1, 0);
}

lcTrackTool View::GetTrackTool(float* InterfaceScale, lcVector3* InterfaceCenter) const
{
	lcTool CurrentTool = gMainWindow->GetCurrentTool();
	int x = mInputState.x;
	int y = mInputState.y;

	if (CurrentTool == LC_TOOL_ROTATE_VIEW)
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

		const float SquareSize = lcMax(8.0f, (vw + vh) / 200);

		if ((d < r + SquareSize) && (d > r - SquareSize))
		{
			if ((cx - x < SquareSize) && (cx - x > -SquareSize))
				return LC_TRACKTOOL_ROTATE_VIEW_Y;

			if ((cy - y < SquareSize) && (cy - y > -SquareSize))
				return LC_TRACKTOOL_ROTATE_VIEW_X;
		}
		else
		{
			if (d < r)
				return LC_TRACKTOOL_ROTATE_VIEW;
			else
				return LC_TRACKTOOL_ROTATE_VIEW_Z;
		}
	}

	if (CurrentTool != LC_TOOL_SELECT && CurrentTool != LC_TOOL_MOVE && CurrentTool != LC_TOOL_ROTATE)
		return LC_TRACKTOOL_NONE;

	int Viewport[4] = { 0, 0, mWidth, mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];

	const lcMatrix44& ModelView = mCamera->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(mCamera->mFOV, Aspect, mCamera->mNear, mCamera->mFar);

	lcVector3 OverlayCenter = mProject->mActiveModel->GetFocusOrSelectionCenter();

	lcVector3 ScreenPos = lcProjectPoint(OverlayCenter, ModelView, Projection, Viewport);
	ScreenPos[0] += 10.0f;
	lcVector3 Point = lcUnprojectPoint(ScreenPos, ModelView, Projection, Viewport);

	lcVector3 Dist(Point - OverlayCenter);
	float OverlayScale = Dist.Length() * 5.0f;

	if (InterfaceScale)
		*InterfaceScale = OverlayScale;

	if (InterfaceCenter)
		*InterfaceCenter = OverlayCenter;

	if (CurrentTool == LC_TOOL_SELECT || CurrentTool == LC_TOOL_MOVE)
	{
		const float OverlayMovePlaneSize = 0.5f * OverlayScale;
		const float OverlayMoveArrowSize = 1.5f * OverlayScale;
		const float OverlayMoveArrowCapRadius = 0.1f * OverlayScale;
		const float OverlayRotateArrowStart = 1.0f * OverlayScale;
		const float OverlayRotateArrowEnd = 1.5f * OverlayScale;

		// Intersect the mouse with the 3 planes.
		lcVector3 PlaneNormals[3] =
		{
			lcVector3(1.0f, 0.0f, 0.0f),
			lcVector3(0.0f, 1.0f, 0.0f),
			lcVector3(0.0f, 0.0f, 1.0f),
		};
/*
		// Find the rotation from the focused piece if relative snap is enabled.
		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Object* Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPiece())
			{
				const lcMatrix44& RotMat = ((Piece*)Focus)->mModelWorld;

				for (int i = 0; i < 3; i++)
					PlaneNormals[i] = lcMul30(PlaneNormals[i], RotMat);
			}
		}
*/
		lcVector3 Start = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.0f), ModelView, Projection, Viewport);
		lcVector3 End = lcUnprojectPoint(lcVector3((float)x, (float)y, 1.0f), ModelView, Projection, Viewport);
		float ClosestIntersectionDistance = FLT_MAX;
		lcTrackTool TrackTool = LC_TRACKTOOL_NONE;

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

				TrackTool = PlaneModes[AxisIndex];

				ClosestIntersectionDistance = IntersectionDistance;
			}

			if (fabs(Proj1) < OverlayMoveArrowCapRadius && Proj2 > 0.0f && Proj2 < OverlayMoveArrowSize)
			{
				lcTrackTool DirModes[] = { LC_TRACKTOOL_MOVE_Z, LC_TRACKTOOL_MOVE_X, LC_TRACKTOOL_MOVE_Y };

				TrackTool = DirModes[AxisIndex];

				ClosestIntersectionDistance = IntersectionDistance;
			}

			if (fabs(Proj2) < OverlayMoveArrowCapRadius && Proj1 > 0.0f && Proj1 < OverlayMoveArrowSize)
			{
				lcTrackTool DirModes[] = { LC_TRACKTOOL_MOVE_Y, LC_TRACKTOOL_MOVE_Z, LC_TRACKTOOL_MOVE_X };

				TrackTool = DirModes[AxisIndex];

				ClosestIntersectionDistance = IntersectionDistance;
			}

			if (CurrentTool == LC_TOOL_MOVE)
				continue;

			if (Proj1 > OverlayRotateArrowStart && Proj1 < OverlayRotateArrowEnd && Proj2 > OverlayRotateArrowStart && Proj2 < OverlayRotateArrowEnd)
			{
				lcTrackTool PlaneModes[] = { LC_TRACKTOOL_ROTATE_X, LC_TRACKTOOL_ROTATE_Y, LC_TRACKTOOL_ROTATE_Z };

				TrackTool = PlaneModes[AxisIndex];

				ClosestIntersectionDistance = IntersectionDistance;
			}
		}

		return TrackTool;
	}

	if (CurrentTool == LC_TOOL_ROTATE)
	{
		const float OverlayRotateRadius = 2.0f;

		// Calculate the distance from the mouse pointer to the center of the sphere.
		lcVector3 SegStart = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.0f), ModelView, Projection, Viewport);
		lcVector3 SegEnd = lcUnprojectPoint(lcVector3((float)x, (float)y, 1.0f), ModelView, Projection, Viewport);
		lcTrackTool TrackTool = LC_TRACKTOOL_NONE;

		const lcVector3& Center = OverlayCenter;

		lcVector3 Line = SegEnd - SegStart;
		lcVector3 Vec = Center - SegStart;

		float u = lcDot(Vec, Line) / Line.LengthSquared();

		// Closest point in the line to the mouse.
		lcVector3 Closest = SegStart + Line * u;

		float Distance = (Closest - Center).Length();
		const float Epsilon = 0.25f * OverlayScale;

		if (Distance > (OverlayRotateRadius * OverlayScale + Epsilon))
		{
			TrackTool = LC_TRACKTOOL_NONE;
		}
		else if (Distance < (OverlayRotateRadius * OverlayScale + Epsilon))
		{
			// 3D rotation unless we're over one of the axis circles.
			TrackTool = LC_TRACKTOOL_NONE;

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
					lcVector3 Dist = Intersections[i] - Center;

					if (lcDot(ViewDir, Dist) > 0.0f)
						continue;
/*
					// Find the rotation from the focused piece if relative snap is enabled.
					if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
					{
						Object* Focus = GetFocusObject();

						if ((Focus != NULL) && Focus->IsPiece())
						{
							const lcVector4& Rot = ((Piece*)Focus)->mRotation;

							lcMatrix44 RotMat = lcMatrix44FromAxisAngle(lcVector3(Rot[0], Rot[1], Rot[2]), -Rot[3] * LC_DTOR);

							Dist = lcMul30(Dist, RotMat);
						}
					}
*/
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
								TrackTool = LC_TRACKTOOL_ROTATE_X;
						}
						else
						{
							if (dz < Epsilon)
								TrackTool = LC_TRACKTOOL_ROTATE_Z;
						}
					}
					else
					{
						if (dy < dz)
						{
							if (dy < Epsilon)
								TrackTool = LC_TRACKTOOL_ROTATE_Y;
						}
						else
						{
							if (dz < Epsilon)
								TrackTool = LC_TRACKTOOL_ROTATE_Z;
						}
					}

					if (TrackTool != LC_TRACKTOOL_NONE)
					{
						switch (TrackTool)
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

//						m_OverlayTrackStart = Center + Dist;

						break;
					}
				}
			}
		}

		return TrackTool;
	}

	return LC_TRACKTOOL_NONE;
}

lcTool View::GetMouseTool(lcTrackTool TrackTool) const
{
	switch (TrackTool)
	{
	case LC_TRACKTOOL_NONE:
		break;

	case LC_TRACKTOOL_MOVE_X:
	case LC_TRACKTOOL_MOVE_Y:
	case LC_TRACKTOOL_MOVE_Z:
	case LC_TRACKTOOL_MOVE_XY:
	case LC_TRACKTOOL_MOVE_XZ:
	case LC_TRACKTOOL_MOVE_YZ:
		return LC_TOOL_MOVE;
		break;

	case LC_TRACKTOOL_ROTATE_X:
	case LC_TRACKTOOL_ROTATE_Y:
	case LC_TRACKTOOL_ROTATE_Z:
		return LC_TOOL_ROTATE;
		break;

	case LC_TRACKTOOL_ZOOM:
		return LC_TOOL_ZOOM;
		break;

	case LC_TRACKTOOL_PAN:
		return LC_TOOL_PAN;
		break;

	case LC_TRACKTOOL_ROTATE_VIEW_X:
	case LC_TRACKTOOL_ROTATE_VIEW_Y:
	case LC_TRACKTOOL_ROTATE_VIEW_Z:
	case LC_TRACKTOOL_ROTATE_VIEW:
		return LC_TOOL_ROTATE_VIEW;
		break;
	}

	return gMainWindow->GetCurrentTool();
}

void View::StartTracking(lcMouseTrack MouseTrack, lcTrackTool TrackTool)
{
	LC_ASSERT(mMouseTrack == LC_MOUSETRACK_NONE);

	mMouseTrack = MouseTrack;
	mTrackTool = TrackTool;
	mMouseDownX = mInputState.x;
	mMouseDownY = mInputState.y;

	OnUpdateCursor();
}

void View::StopTracking(bool Accept)
{
	if (mMouseTrack == LC_MOUSETRACK_NONE)
		return;

	lcTool Tool = GetMouseTool(mTrackTool);

	mMouseTrack = LC_MOUSETRACK_NONE;
	mTrackTool = LC_TRACKTOOL_NONE;

	switch (Tool)
	{
	case LC_TOOL_INSERT:
		break;

	case LC_TOOL_LIGHT:
		break;

	case LC_TOOL_CAMERA:
		mProject->mActiveModel->EndCreateCameraTool(Accept);
		break;

	case LC_TOOL_SELECT:
		if (Accept)
		{
			float pt1x = lcClamp((float)mMouseDownX, 0.0f, mWidth - 1.0f);
			float pt1y = lcClamp((float)mMouseDownY, 0.0f, mHeight - 1.0f);
			float pt2x = lcClamp((float)mInputState.x, 0.0f, mWidth - 1.0f);
			float pt2y = lcClamp((float)mInputState.y, 0.0f, mHeight - 1.0f);

			if (pt1x != pt2x && pt1y != pt2y)
			{
				lcArray<lcObjectSection> ObjectSections;
				View* View = gMainWindow->mActiveView;

				FindObjectsInRectangle(pt1x, pt1y, pt2x, pt2y, ObjectSections);

				if (View->mInputState.Control)
					mProject->mActiveModel->AddToSelection(ObjectSections);
				else
					mProject->mActiveModel->SetSelection(ObjectSections);
			}
		}
		break;

	case LC_TOOL_MOVE:
		mProject->mActiveModel->EndMoveTool(Accept);
		break;

	case LC_TOOL_ERASER:
		break;

	case LC_TOOL_PAINT:
		break;

	case LC_TOOL_ZOOM:
		mProject->mActiveModel->EndEditCameraTool(LC_ACTION_ZOOM_CAMERA, Accept);
		break;

	case LC_TOOL_PAN:
		mProject->mActiveModel->EndEditCameraTool(LC_ACTION_PAN_CAMERA, Accept);
		break;

	case LC_TOOL_ROTATE_VIEW:
		mProject->mActiveModel->EndEditCameraTool(LC_ACTION_ORBIT_CAMERA, Accept);
		break;

	case LC_TOOL_ROLL:
		mProject->mActiveModel->EndEditCameraTool(LC_ACTION_ROLL_CAMERA, Accept);
		break;

	case LC_TOOL_ZOOM_REGION:
		{
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
			Right = lcMin(Right, mWidth - 1);
			Bottom = lcMax(Bottom, 0.0f);
			Top = lcMin(Top, mHeight - 1);

			mProject->mActiveModel->ZoomRegion(gMainWindow->mActiveView, Left, Right, Bottom, Top, gMainWindow->mAddKeys);
		}
		break;
	}

	gMainWindow->UpdateAllViews();
	OnUpdateCursor();


/*
	if (Accept)
	{
		if (mDropPiece)
		{
			int x = m_nDownX;
			int y = m_nDownY;

			if ((x > 0) && (x < ActiveView->mWidth) && (y > 0) && (y < ActiveView->mHeight))
			{
				lcVector3 Pos;
				lcVector4 Rot;

				GetPieceInsertPosition(ActiveView, x, y, Pos, Rot);

				Piece* pPiece = new Piece(mDropPiece);
				pPiece->Initialize(Pos[0], Pos[1], Pos[2], m_nCurStep);
				pPiece->SetColorIndex(gMainWindow->mColorIndex);

				pPiece->ChangeKey(m_nCurStep, false, false, Rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_nCurStep, m_bAnimation);

				SelectAndFocusNone(false);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				SystemPieceComboAdd(mDropPiece->m_strDescription);
				pPiece->Select (true, true, false);

				if (mDropPiece)
				{
					mDropPiece->Release();
					mDropPiece = NULL;
				}

				UpdateSelection();
				gMainWindow->UpdateAllViews();
				gMainWindow->UpdateFocusObject(pPiece);

				SetModifiedFlag(true);
				CheckPoint("Inserting");
			}
		}
	}

	if (mDropPiece)
	{
		mDropPiece->Release();
		mDropPiece = NULL;
	}

	return true;
*/
}

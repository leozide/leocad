#include "lc_global.h"
#include <stdlib.h>
#include "lc_mainwindow.h"
#include "project.h"
#include "camera.h"
#include "view.h"
#include "system.h"
#include "tr.h"
#include "texfont.h"
#include "lc_texture.h"

View::View(Project *project)
{
	mProject = project;
	mCamera = NULL;

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
	if (gMainWindow)
		gMainWindow->RemoveView(this);

	if (mCamera && mCamera->IsSimple())
		delete mCamera;
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

void View::SetDefaultCamera()
{
	if (!mCamera || !mCamera->IsSimple())
		mCamera = new lcCamera(true);

	mCamera->SetViewpoint(LC_VIEWPOINT_HOME, 1, false);
}

lcMatrix44 View::GetProjectionMatrix() const
{
	float AspectRatio = (float)mWidth / (float)mHeight;

	if (mCamera->m_pTR)
		mCamera->m_pTR->BeginTile();

	if (mCamera->IsOrtho())
	{
		// Compute the FOV/plane intersection radius.
		//                d               d
		//   a = 2 atan(------) => ~ a = --- => d = af
		//                2f              f
		float f = (mCamera->mPosition - mCamera->mOrthoTarget).Length();
		float d = (mCamera->m_fovy * f) * (LC_PI / 180.0f);
		float r = d / 2;

		float right = r * AspectRatio;
		return lcMatrix44Ortho(-right, right, -r, r, mCamera->m_zNear, mCamera->m_zFar * 4);
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

	mProject->RayTest(ObjectRayTest);

	return ObjectRayTest.ObjectSection;
}

lcArray<lcObjectSection> View::FindObjectsInBox(float x1, float y1, float x2, float y2) const
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

	mProject->BoxTest(ObjectBoxTest);

	return ObjectBoxTest.ObjectSections;
}

void View::OnDraw()
{
	mProject->Render(this, false);

	if (mWidget)
	{
		const lcPreferences& Preferences = lcGetPreferences();

		if (Preferences.mDrawGridStuds || Preferences.mDrawGridLines)
			DrawGrid();

		if (Preferences.mDrawAxes)
			DrawAxes();

		lcTool Tool = gMainWindow->GetTool();

		if ((Tool == LC_TOOL_SELECT || Tool == LC_TOOL_MOVE) && mTrackButton == LC_TRACKBUTTON_NONE && mProject->AnyObjectsSelected(false))
			DrawSelectMoveOverlay();
		else if (GetCurrentTool() == LC_TOOL_MOVE && mTrackButton != LC_TRACKBUTTON_NONE)
			DrawSelectMoveOverlay();
		else if ((Tool == LC_TOOL_ROTATE || (Tool == LC_TOOL_SELECT && mTrackButton != LC_TRACKBUTTON_NONE && mTrackTool >= LC_TRACKTOOL_ROTATE_X && mTrackTool <= LC_TRACKTOOL_ROTATE_XYZ)) && mProject->AnyObjectsSelected(true))
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
	const float OverlayScale = GetOverlayScale();
	const lcMatrix44& ViewMatrix = mCamera->mWorldView;

	const float OverlayMovePlaneSize = 0.5f * OverlayScale;
	const float OverlayMoveArrowSize = 1.5f * OverlayScale;
	const float OverlayMoveArrowCapSize = 0.9f * OverlayScale;
	const float OverlayMoveArrowCapRadius = 0.1f * OverlayScale;
	const float OverlayMoveArrowBodyRadius = 0.05f * OverlayScale;
	const float OverlayRotateArrowStart = 1.0f * OverlayScale;
	const float OverlayRotateArrowEnd = 1.5f * OverlayScale;
	const float OverlayRotateArrowCenter = 1.2f * OverlayScale;

	mContext->SetProjectionMatrix(GetProjectionMatrix());

	glDisable(GL_DEPTH_TEST);

	lcMatrix44 RelativeRotation = mProject->GetRelativeRotation();
	lcVector3 OverlayCenter = mProject->GetFocusOrSelectionCenter();
	bool AnyPiecesSelected = mProject->AnyObjectsSelected(true);

	// Draw the arrows.
	for (int i = 0; i < 3; i++)
	{
		switch (i)
		{
		case 0:
			if ((mTrackTool == LC_TRACKTOOL_MOVE_X) || (mTrackTool == LC_TRACKTOOL_MOVE_XY) || (mTrackTool == LC_TRACKTOOL_MOVE_XZ))
				glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
			else if (mTrackButton == LC_TRACKBUTTON_NONE)
				glColor4f(0.8f, 0.0f, 0.0f, 1.0f);
			else
				continue;
			break;
		case 1:
			if ((mTrackTool == LC_TRACKTOOL_MOVE_Y) || (mTrackTool == LC_TRACKTOOL_MOVE_XY) || (mTrackTool == LC_TRACKTOOL_MOVE_YZ))
				glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
			else if (mTrackButton == LC_TRACKBUTTON_NONE)
				glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
			else
				continue;
			break;
		case 2:
			if ((mTrackTool == LC_TRACKTOOL_MOVE_Z) || (mTrackTool == LC_TRACKTOOL_MOVE_XZ) || (mTrackTool == LC_TRACKTOOL_MOVE_YZ))
				glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
			else if (mTrackButton == LC_TRACKBUTTON_NONE)
				glColor4f(0.0f, 0.0f, 0.8f, 1.0f);
			else
				continue;
			break;
		}

		lcMatrix44 WorldViewMatrix = lcMul(lcMatrix44Translation(OverlayCenter), ViewMatrix);
		WorldViewMatrix = lcMul(RelativeRotation, WorldViewMatrix);

		if (i == 1)
			WorldViewMatrix = lcMul(lcMatrix44(lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 1, 0), lcVector4(0, 0, 0, 1)), WorldViewMatrix);
		else if (i == 2)
			WorldViewMatrix = lcMul(lcMatrix44(lcVector4(0, 0, 1, 0), lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 0, 1)), WorldViewMatrix);

		mContext->SetWorldViewMatrix(WorldViewMatrix);

		// Translation arrows.
		if (mTrackButton == LC_TRACKBUTTON_NONE || (mTrackTool >= LC_TRACKTOOL_MOVE_X && mTrackTool <= LC_TRACKTOOL_MOVE_XYZ))
		{
			lcVector3 Verts[11];

			Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);
			Verts[1] = lcVector3(OverlayMoveArrowSize, 0.0f, 0.0f);

			for (int j = 0; j < 9; j++)
			{
				float y = cosf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
				float z = sinf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
				Verts[j + 2] = lcVector3(OverlayMoveArrowCapSize, y, z);
			}

			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINES, 0, 2);
			glDrawArrays(GL_TRIANGLE_FAN, 1, 10);
		}

		// Rotation arrows.
		if (gMainWindow->GetTool() == LC_TOOL_SELECT && mTrackButton == LC_TRACKBUTTON_NONE && AnyPiecesSelected)
		{
			switch (i)
			{
			case 0:
				if (mTrackTool == LC_TRACKTOOL_ROTATE_X)
					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				else
					glColor4f(0.8f, 0.0f, 0.0f, 1.0f);
				break;
			case 1:
				if (mTrackTool == LC_TRACKTOOL_ROTATE_Y)
					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				else
					glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
				break;
			case 2:
				if (mTrackTool == LC_TRACKTOOL_ROTATE_Z)
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
		}
	}

	// Draw a quad if we're moving on a plane.
	if ((mTrackTool == LC_TRACKTOOL_MOVE_XY) || (mTrackTool == LC_TRACKTOOL_MOVE_XZ) || (mTrackTool == LC_TRACKTOOL_MOVE_YZ))
	{
		lcMatrix44 WorldViewMatrix = lcMul(lcMatrix44Translation(OverlayCenter), ViewMatrix);
		WorldViewMatrix = lcMul(RelativeRotation, WorldViewMatrix);

		if (mTrackTool == LC_TRACKTOOL_MOVE_XZ)
			WorldViewMatrix = lcMul(lcMatrix44RotationZ(-LC_PI / 2), WorldViewMatrix);
		else if (mTrackTool == LC_TRACKTOOL_MOVE_XY)
			WorldViewMatrix = lcMul(lcMatrix44RotationY(LC_PI / 2), WorldViewMatrix);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glColor4f(0.8f, 0.8f, 0.0f, 0.3f);
		mContext->SetWorldViewMatrix(WorldViewMatrix);

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
	}

	glEnable(GL_DEPTH_TEST);
}

void View::DrawRotateOverlay()
{
	const float OverlayScale = GetOverlayScale();
	const lcMatrix44& ViewMatrix = mCamera->mWorldView;

	const float OverlayRotateRadius = 2.0f;

	mContext->SetProjectionMatrix(GetProjectionMatrix());

	glDisable(GL_DEPTH_TEST);

	int j;

	lcMatrix44 RelativeRotation = mProject->GetRelativeRotation();
	lcVector3 OverlayCenter = mProject->GetFocusOrSelectionCenter();
	const lcVector3& MouseToolDistance = mProject->GetMouseToolDistance();
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
			glColor4f(0.8f, 0.0f, 0.0f, 0.3f);
			Angle = MouseToolDistance[0];
			Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
			break;
		case LC_TRACKTOOL_ROTATE_Y:
			glColor4f(0.0f, 0.8f, 0.0f, 0.3f);
			Angle = MouseToolDistance[1];
			Rotation = lcVector4(90.0f, 0.0f, 0.0f, 1.0f);
			break;
		case LC_TRACKTOOL_ROTATE_Z:
			glColor4f(0.0f, 0.0f, 0.8f, 0.3f);
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
			lcMatrix44 WorldViewMatrix = lcMul(lcMatrix44Translation(OverlayCenter), ViewMatrix);
			WorldViewMatrix = lcMul(RelativeRotation, WorldViewMatrix);
			WorldViewMatrix = lcMul(lcMatrix44FromAxisAngle(lcVector3(Rotation[1], Rotation[2], Rotation[3]), Rotation[0] * LC_DTOR), WorldViewMatrix);

			mContext->SetWorldViewMatrix(WorldViewMatrix);

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

	lcMatrix44 Mat = lcMatrix44AffineInverse(mCamera->mWorldView);
	Mat.SetTranslation(OverlayCenter);

	// Draw the circles.
	if (gMainWindow->GetTool() == LC_TOOL_ROTATE && !HasAngle && mTrackButton == LC_TRACKBUTTON_NONE)
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
		mContext->SetWorldViewMatrix(ViewMatrix);

		glVertexPointer(3, GL_FLOAT, 0, Verts);
		glDrawArrays(GL_LINE_LOOP, 0, 32);
	}

	lcVector3 ViewDir = mCamera->mTargetPosition - mCamera->mPosition;
	ViewDir.Normalize();

	// Transform ViewDir to local space.
	ViewDir = lcMul30(ViewDir, lcMatrix44AffineInverse(RelativeRotation));

	lcMatrix44 WorldViewMatrix = lcMul(lcMatrix44Translation(OverlayCenter), ViewMatrix);
	WorldViewMatrix = lcMul(RelativeRotation, WorldViewMatrix);
	mContext->SetWorldViewMatrix(WorldViewMatrix);

	// Draw each axis circle.
	for (int i = 0; i < 3; i++)
	{
		if (mTrackTool == LC_TRACKTOOL_ROTATE_X + i)
		{
			glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
		}
		else
		{
			if (gMainWindow->GetTool() != LC_TOOL_ROTATE || HasAngle || mTrackButton != LC_TRACKBUTTON_NONE)
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

		lcMatrix44 WorldViewMatrix = lcMul(lcMatrix44Translation(OverlayCenter), ViewMatrix);
		WorldViewMatrix = lcMul(RelativeRotation, WorldViewMatrix);
		WorldViewMatrix = lcMul(lcMatrix44FromAxisAngle(lcVector3(Rotation[1], Rotation[2], Rotation[3]), Rotation[0] * LC_DTOR), WorldViewMatrix);
		mContext->SetWorldViewMatrix(WorldViewMatrix);

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

		// Draw text.
		lcVector3 ScreenPos = ProjectPoint(OverlayCenter);

		mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));
		mContext->SetWorldViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		gTexFont.MakeCurrent();
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		char buf[32];
		sprintf(buf, "[%.2f]", fabsf(Angle));

		int cx, cy;
		gTexFont.GetStringDimensions(&cx, &cy, buf);

		glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
		gTexFont.PrintText(ScreenPos[0] - (cx / 2), ScreenPos[1] + (cy / 2), 0.0f, buf);

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
	}

	glEnable(GL_DEPTH_TEST);
}

void View::DrawSelectZoomRegionOverlay()
{
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));
	mContext->SetWorldViewMatrix(lcMatrix44Translation(lcVector3(0.375f, 0.375f, 0.0f)));

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
}

void View::DrawRotateViewOverlay()
{
	int x, y, w, h;

	x = 0;
	y = 0;
	w = mWidth;
	h = mHeight;

	mContext->SetProjectionMatrix(lcMatrix44Ortho(0, w, 0, h, -1, 1));
	mContext->SetWorldViewMatrix(lcMatrix44Translation(lcVector3(0.375f, 0.375f, 0.0f)));

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

	// Draw squares.
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
}

void View::DrawGrid()
{
	const lcPreferences& Preferences = lcGetPreferences();

	mContext->SetWorldViewMatrix(mCamera->mWorldView);

	const int Spacing = lcMax(Preferences.mGridLineSpacing, 1);
	int MinX, MaxX, MinY, MaxY;
	float BoundingBox[6];

	if (mProject->GetPiecesBoundingBox(this, BoundingBox))
	{
		MinX = (int)(floorf(BoundingBox[0] / (0.8f * Spacing))) - 1;
		MinY = (int)(floorf(BoundingBox[1] / (0.8f * Spacing))) - 1;
		MaxX = (int)(ceilf(BoundingBox[3] / (0.8f * Spacing))) + 1;
		MaxY = (int)(ceilf(BoundingBox[4] / (0.8f * Spacing))) + 1;

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

	if (Preferences.mDrawGridStuds)
	{
		float Left = MinX * 0.8f * Spacing;
		float Right = MaxX * 0.8f * Spacing;
		float Top = MinY * 0.8f * Spacing;
		float Bottom = MaxY * 0.8f * Spacing;
		float Z = 0;
		float U = (MaxX - MinX) * Spacing;
		float V = (MaxY - MinY) * Spacing;

		float Verts[4 * 5];
		float* CurVert = Verts;

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

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBindTexture(GL_TEXTURE_2D, gGridTexture->mTexture);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glColor4fv(lcVector4FromColor(Preferences.mGridStudColor));

		glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), Verts);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), Verts + 3);

		glDrawArrays(GL_QUADS, 0, 4);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
	}

	if (Preferences.mDrawGridLines)
	{
		mContext->SetLineWidth(1.0f);
		glColor4fv(lcVector4FromColor(Preferences.mGridLineColor));

		int NumVerts = 2 * (MaxX - MinX + MaxY - MinY + 2);
		float* Verts = (float*)malloc(NumVerts * sizeof(float[3]));
		float* Vert = Verts;
		float LineSpacing = Spacing * 0.8f;

		for (int Step = MinX; Step < MaxX + 1; Step++)
		{
			*Vert++ = Step * LineSpacing;
			*Vert++ = MinY * LineSpacing;
			*Vert++ = 0.0f;
			*Vert++ = Step * LineSpacing;
			*Vert++ = MaxY * LineSpacing;
			*Vert++ = 0.0f;
		}

		for (int Step = MinY; Step < MaxY + 1; Step++)
		{
			*Vert++ = MinX * LineSpacing;
			*Vert++ = Step * LineSpacing;
			*Vert++ = 0.0f;
			*Vert++ = MaxX * LineSpacing;
			*Vert++ = Step * LineSpacing;
			*Vert++ = 0.0f;
		}

		glVertexPointer(3, GL_FLOAT, 0, Verts);
		glDrawArrays(GL_LINES, 0, NumVerts);
		free(Verts);
	}
}

void View::DrawAxes()
{
//	glClear(GL_DEPTH_BUFFER_BIT);

	lcMatrix44 Mats[3];
	Mats[0] = mCamera->mWorldView;
	Mats[0].SetTranslation(lcVector3(0, 0, 0));
	Mats[1] = lcMul(lcMatrix44(lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 1, 0), lcVector4(0, 0, 0, 1)), Mats[0]);
	Mats[2] = lcMul(lcMatrix44(lcVector4(0, 0, 1, 0), lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 0, 1)), Mats[0]);

	lcVector3 pts[3] =
	{
		lcMul30(lcVector3(20, 0, 0), Mats[0]),
		lcMul30(lcVector3(0, 20, 0), Mats[0]),
		lcMul30(lcVector3(0, 0, 20), Mats[0]),
	};

	const lcVector4 Colors[3] =
	{
		lcVector4(0.8f, 0.0f, 0.0f, 1.0f),
		lcVector4(0.0f, 0.8f, 0.0f, 1.0f),
		lcVector4(0.0f, 0.0f, 0.8f, 1.0f)
	};

	mContext->SetProjectionMatrix(lcMatrix44Ortho(0, mWidth, 0, mHeight, -50, 50));
	mContext->SetWorldViewMatrix(lcMatrix44Translation(lcVector3(25.375f, 25.375f, 0.0f)));

	// Draw the arrows.
	lcVector3 Verts[11];
	Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);

	glVertexPointer(3, GL_FLOAT, 0, Verts);

	for (int i = 0; i < 3; i++)
	{
		glColor4fv(Colors[i]);
		Verts[1] = pts[i];

		for (int j = 0; j < 9; j++)
			Verts[j+2] = lcMul30(lcVector3(12.0f, cosf(LC_2PI * j / 8) * 3.0f, sinf(LC_2PI * j / 8) * 3.0f), Mats[i]);

		glDrawArrays(GL_LINES, 0, 2);
		glDrawArrays(GL_TRIANGLE_FAN, 1, 10);
	}

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	gTexFont.MakeCurrent();
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	gTexFont.PrintText(pts[0][0], pts[0][1], 40.0f, "X");
	gTexFont.PrintText(pts[1][0], pts[1][1], 40.0f, "Y");
	gTexFont.PrintText(pts[2][0], pts[2][1], 40.0f, "Z");

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

void View::DrawViewport()
{
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));
	mContext->SetWorldViewMatrix(lcMatrix44Translation(lcVector3(0.375f, 0.375f, 0.0f)));

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	if (gMainWindow->GetActiveView() == this)
	{
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
		float Verts[8] = { 0.0f, 0.0f, mWidth - 1, 0.0f, mWidth - 1, mHeight - 1, 0.0f, mHeight - 1 };

		glVertexPointer(2, GL_FLOAT, 0, Verts);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}

	const char* CameraName = mCamera->GetName();

	if (CameraName[0])
	{
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		gTexFont.MakeCurrent();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		gTexFont.PrintText(3.0f, (float)mHeight - 1.0f - 6.0f, 0.0f, CameraName);

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
	lcVector3 OverlayCenter = mProject->GetFocusOrSelectionCenter();

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
	{
		lcVector3 Position;
		lcVector4 Rotation;
		mProject->GetPieceInsertPosition(this, Position, Rotation);
		mProject->InsertPieceToolClicked(Position, Rotation);
	}

	mDragState = LC_DRAGSTATE_NONE;
	UpdateTrackTool();
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

			// Intersect the mouse with the 3 planes.
			lcVector3 PlaneNormals[3] =
			{
				lcVector3(1.0f, 0.0f, 0.0f),
				lcVector3(0.0f, 1.0f, 0.0f),
				lcVector3(0.0f, 0.0f, 1.0f),
			};

			lcMatrix44 RelativeRotation = mProject->GetRelativeRotation();
			lcVector3 OverlayCenter = mProject->GetFocusOrSelectionCenter();

			for (int i = 0; i < 3; i++)
				PlaneNormals[i] = lcMul30(PlaneNormals[i], RelativeRotation);

			NewTrackTool = (CurrentTool == LC_TOOL_MOVE) ? LC_TRACKTOOL_MOVE_XYZ : LC_TRACKTOOL_SELECT;
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

				if (CurrentTool == LC_TOOL_SELECT && Proj1 > OverlayRotateArrowStart && Proj1 < OverlayRotateArrowEnd && Proj2 > OverlayRotateArrowStart && Proj2 < OverlayRotateArrowEnd)
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

			// Calculate the distance from the mouse pointer to the center of the sphere.
			lcVector3 StartEnd[2] = { lcVector3((float)x, (float)y, 0.0f), lcVector3((float)x, (float)y, 1.0f) };
			UnprojectPoints(StartEnd, 2);
			const lcVector3& SegStart = StartEnd[0];
			const lcVector3& SegEnd = StartEnd[1];
			NewTrackTool = LC_TRACKTOOL_ROTATE_XYZ;

			lcVector3 OverlayCenter = mProject->GetFocusOrSelectionCenter();
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

					lcMatrix44 RelativeRotation = mProject->GetRelativeRotation();

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

			const float SquareSize = lcMax(8.0f, (vw + vh) / 200);

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
			mProject->BeginSpotLightTool(PositionTarget[0], PositionTarget[1]);
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
			mProject->BeginCameraTool(PositionTarget[0], PositionTarget[1]);
		}
		break;

	case LC_TOOL_SELECT:
		break;

	case LC_TOOL_MOVE:
	case LC_TOOL_ROTATE:
		mProject->BeginMouseTool();
		break;

	case LC_TOOL_ERASER:
	case LC_TOOL_PAINT:
		break;

	case LC_TOOL_ZOOM:
	case LC_TOOL_PAN:
	case LC_TOOL_ROTATE_VIEW:
	case LC_TOOL_ROLL:
		mProject->BeginMouseTool();
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
		mProject->EndMouseTool(Tool, Accept);
		break;

	case LC_TOOL_SELECT:
		if (Accept && mMouseDownX != mInputState.x && mMouseDownY != mInputState.y)
		{
			lcArray<lcObjectSection> ObjectSections = FindObjectsInBox(mMouseDownX, mMouseDownY, mInputState.x, mInputState.y);

			if (mInputState.Control)
				mProject->AddToSelection(ObjectSections);
			else
				mProject->SetSelection(ObjectSections);
		}
		break;

	case LC_TOOL_MOVE:
	case LC_TOOL_ROTATE:
		mProject->EndMouseTool(Tool, Accept);
		break;

	case LC_TOOL_ERASER:
	case LC_TOOL_PAINT:
		break;

	case LC_TOOL_ZOOM:
	case LC_TOOL_PAN:
	case LC_TOOL_ROTATE_VIEW:
	case LC_TOOL_ROLL:
		mProject->EndMouseTool(Tool, Accept);
		break;

	case LC_TOOL_ZOOM_REGION:
		{
			lcVector3 Points[3] =
			{
				lcVector3((mMouseDownX + lcMin(mInputState.x, mWidth - 1)) / 2, (mMouseDownY + lcMin(mInputState.y, mHeight - 1)) / 2, 0.9f),
				lcVector3((float)mWidth / 2.0f, (float)mHeight / 2.0f, 0.9f),
				lcVector3((float)mWidth / 2.0f, (float)mHeight / 2.0f, 0.1f),
			};

			UnprojectPoints(Points, 3);

			float RatioX = (mMouseDownX - mInputState.x) / mWidth;
			float RatioY = (mMouseDownY - mInputState.y) / mHeight;

			mProject->ZoomRegionToolClicked(mCamera, Points, fabsf(RatioX), fabsf(RatioY));
		}
		break;
	}

	mTrackButton = LC_TRACKBUTTON_NONE;
	UpdateTrackTool();
	gMainWindow->UpdateAllViews();
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
			lcVector3 Position;
			lcVector4 Rotation;
			mProject->GetPieceInsertPosition(this, Position, Rotation);
			mProject->InsertPieceToolClicked(Position, Rotation);

			if (!mInputState.Control)
				gMainWindow->SetTool(LC_TOOL_SELECT);

			UpdateTrackTool();
		}
		break;

	case LC_TRACKTOOL_POINTLIGHT:
		{
			mProject->PointLightToolClicked(UnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f)));

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
				mProject->FocusOrDeselectObject(ObjectSection);
			else
				mProject->ClearSelectionAndSetFocus(ObjectSection);

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
		if (mProject->AnyObjectsSelected(false))
			StartTracking(LC_TRACKBUTTON_LEFT);
		break;

	case LC_TRACKTOOL_ROTATE_X:
	case LC_TRACKTOOL_ROTATE_Y:
	case LC_TRACKTOOL_ROTATE_Z:
	case LC_TRACKTOOL_ROTATE_XY:
	case LC_TRACKTOOL_ROTATE_XYZ:
		if (mProject->AnyObjectsSelected(true))
			StartTracking(LC_TRACKBUTTON_LEFT);
		break;

	case LC_TRACKTOOL_ERASER:
		mProject->EraserToolClicked(FindObjectUnderPointer(false).Object);
		break;

	case LC_TRACKTOOL_PAINT:
		mProject->PaintToolClicked(FindObjectUnderPointer(true).Object);
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
		mProject->FocusOrDeselectObject(ObjectSection);
	else
		mProject->ClearSelectionAndSetFocus(ObjectSection);
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
		if (mProject->AnyObjectsSelected(false))
			StartTracking(LC_TRACKBUTTON_RIGHT);
		break;

	case LC_TRACKTOOL_ROTATE_X:
	case LC_TRACKTOOL_ROTATE_Y:
	case LC_TRACKTOOL_ROTATE_Z:
	case LC_TRACKTOOL_ROTATE_XY:
	case LC_TRACKTOOL_ROTATE_XYZ:
		if (mProject->AnyObjectsSelected(true))
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
		{
/*			lcVector3 Position;
			lcVector4 AxisAngle;

			GetPieceInsertPosition(&Position, &AxisAngle);
			mProject->mActiveModel->SetPreviewTransform(Position, AxisAngle);

*/			gMainWindow->UpdateAllViews();
		}

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
		mProject->UpdateSpotLightTool(UnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f)));
		break;

	case LC_TRACKTOOL_CAMERA:
		mProject->UpdateCameraTool(UnprojectPoint(lcVector3((float)mInputState.x, (float)mInputState.y, 0.9f)));
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
			lcVector3 Center = mProject->GetFocusOrSelectionCenter();

			if (mTrackTool == LC_TRACKTOOL_MOVE_X || mTrackTool == LC_TRACKTOOL_MOVE_Y || mTrackTool == LC_TRACKTOOL_MOVE_Z)
			{
				lcVector3 Direction;
				if (mTrackTool == LC_TRACKTOOL_MOVE_X)
					Direction = lcVector3(1.0, 0.0f, 0.0f);
				else if (mTrackTool == LC_TRACKTOOL_MOVE_Y)
					Direction = lcVector3(0.0, 1.0f, 0.0f);
				else
					Direction = lcVector3(0.0, 0.0f, 1.0f);

				lcMatrix44 RelativeRotation = mProject->GetRelativeRotation();
				Direction = lcMul30(Direction, RelativeRotation);

				lcVector3 Intersection;
				lcClosestPointsBetweenLines(Center, Center + Direction, CurrentStart, CurrentEnd, &Intersection, NULL);

				lcVector3 MoveStart;
				lcClosestPointsBetweenLines(Center, Center + Direction, MouseDownStart, MouseDownEnd, &MoveStart, NULL);

				lcVector3 Distance = Intersection - MoveStart;
				Distance = lcMul30(Distance, lcMatrix44AffineInverse(RelativeRotation));
				mProject->UpdateMoveTool(Distance);
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

				lcMatrix44 RelativeRotation = mProject->GetRelativeRotation();
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
						mProject->UpdateMoveTool(Distance);
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
						mProject->UpdateMoveTool(Distance);
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

			mProject->UpdateRotateTool(MoveX + MoveY);
		}
		break;

	case LC_TRACKTOOL_ROTATE_XY:
		{
			lcVector3 ScreenZ = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);
			lcVector3 ScreenX = lcCross(ScreenZ, mCamera->mUpVector);
			lcVector3 ScreenY = mCamera->mUpVector;

			lcVector3 MoveX = 36.0f * (float)(mInputState.x - mMouseDownX) * MouseSensitivity * ScreenX;
			lcVector3 MoveY = 36.0f * (float)(mInputState.y - mMouseDownY) * MouseSensitivity * ScreenY;
			mProject->UpdateRotateTool(MoveX + MoveY);
		}
		break;

	case LC_TRACKTOOL_ROTATE_XYZ:
		{
			lcVector3 ScreenZ = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);

			mProject->UpdateRotateTool(36.0f * (float)(mInputState.y - mMouseDownY) * MouseSensitivity * ScreenZ);
		}
		break;

	case LC_TRACKTOOL_ERASER:
	case LC_TRACKTOOL_PAINT:
		break;

	case LC_TRACKTOOL_ZOOM:
		mProject->UpdateZoomTool(mCamera, 2.0f * MouseSensitivity * (mInputState.y - mMouseDownY));
		break;

	case LC_TRACKTOOL_PAN:
		mProject->UpdatePanTool(mCamera, 2.0f * MouseSensitivity * (mInputState.x - mMouseDownX), 2.0f * MouseSensitivity * (mInputState.y - mMouseDownY));
		break;

	case LC_TRACKTOOL_ORBIT_X:
		mProject->UpdateOrbitTool(mCamera, 0.1f * MouseSensitivity * (mInputState.x - mMouseDownX), 0.0f);
		break;

	case LC_TRACKTOOL_ORBIT_Y:
		mProject->UpdateOrbitTool(mCamera, 0.0f, 0.1f * MouseSensitivity * (mInputState.y - mMouseDownY));
		break;

	case LC_TRACKTOOL_ORBIT_XY:
		mProject->UpdateOrbitTool(mCamera, 0.1f * MouseSensitivity * (mInputState.x - mMouseDownX), 0.1f * MouseSensitivity * (mInputState.y - mMouseDownY));
		break;

	case LC_TRACKTOOL_ROLL:
		mProject->UpdateRollTool(mCamera, 2.0f * MouseSensitivity * (mInputState.x - mMouseDownX) * LC_DTOR);
		break;

	case LC_TRACKTOOL_ZOOM_REGION:
		Redraw();
		break;
	}
}

void View::OnMouseWheel(float Direction)
{
	mProject->ZoomActiveView((int)((mInputState.Control ? 100 : 10) * Direction));
}

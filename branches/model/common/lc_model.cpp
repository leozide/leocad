#include "lc_global.h"
#include "lc_model.h"
#include "lc_mainwindow.h"
#include "view.h"
#include "lc_object.h"
#include "lc_part.h"
#include "lc_camera.h"
#include "lc_light.h"
#include "lc_file.h"

class lcCheckpoint
{
public:
	lcCheckpoint(const char* Name)
	{
		strcpy(mName, Name);
	}

	char mName[64];
	lcMemFile mAction;
};

lcModel::lcModel()
{
	mCurrentCheckpoint = NULL;
	mFocusObject = NULL;
}

lcModel::~lcModel()
{
	DeleteContents();
}

void lcModel::DeleteContents()
{
	delete mCurrentCheckpoint;
	mCurrentCheckpoint = NULL;
	mUndoCheckpoints.DeleteAll();
	mRedoCheckpoints.DeleteAll();

	mFocusObject = NULL;
	mSelectedObjects.RemoveAll();
	mObjects.DeleteAll();

//	mParts.DeleteAll();
//	mCameras.DeleteAll();
//	mLights.DeleteAll();
}

void lcModel::Update(lcTime Time)
{
//	for (int PartIdx = 0; PartIdx < mParts.GetSize(); PartIdx++)
//		mParts[PartIdx]->Update(Time);

//	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
//		mCameras[CameraIdx]->Update(Time);

//	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
//		mLights[LightIdx]->Update(Time);
}

void lcModel::BeginCheckpoint(const char* Name)
{
	LC_ASSERT(!mCurrentCheckpoint);
	mCurrentCheckpoint = new lcCheckpoint(Name);
}

void lcModel::EndCheckpoint(bool Accept)
{
	LC_ASSERT(mCurrentCheckpoint);

	if (Accept)
	{
		mUndoCheckpoints.Add(mCurrentCheckpoint);
		mRedoCheckpoints.DeleteAll();
	}
	else
	{
		RevertCheckpoint(mCurrentCheckpoint);
		delete mCurrentCheckpoint;
	}

	mCurrentCheckpoint = NULL;
}

void lcModel::ToggleSelection(const lcObjectSection& ObjectSection)
{
	if (!ObjectSection.Object)
		return;

	bool WasSelected = ObjectSection.Object->IsSelected();

	ObjectSection.Object->ToggleSelection(ObjectSection.Section);

	if (mFocusObject && !mFocusObject->IsFocused())
		mFocusObject = NULL;

	bool IsSelected  = ObjectSection.Object->IsSelected();

	if (WasSelected && !IsSelected)
		mSelectedObjects.Remove(ObjectSection.Object);
	else if (!WasSelected && IsSelected)
		mSelectedObjects.Add(ObjectSection.Object);

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
	gMainWindow->UpdateFocusObject();
}

void lcModel::SetFocus(const lcObjectSection& ObjectSection)
{
	for (int ObjectIdx = 0; ObjectIdx < mSelectedObjects.GetSize(); ObjectIdx++)
		mSelectedObjects[ObjectIdx]->ClearSelection();
	mSelectedObjects.RemoveAll();
	mFocusObject = NULL;

	if (ObjectSection.Object)
	{
		mSelectedObjects.Add(ObjectSection.Object);
		ObjectSection.Object->SetFocus(ObjectSection.Section, true);
		mFocusObject = ObjectSection.Object;
	}

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
	gMainWindow->UpdateFocusObject();
}

void lcModel::SetCurrentTime(lcTime Time)
{
}

//void lcModel::MoveSelectedObjects(const lcVector3& Distance)
//{
//}

void lcModel::BeginCameraTool(const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3& UpVector)
{
	BeginCheckpoint("Create camera");
	lcMemFile& Action = mCurrentCheckpoint->mAction;

	Action.WriteU32(LC_ACTION_CREATE_CAMERA);
	Action.WriteFloats(Position, 3);
	Action.WriteFloats(TargetPosition, 3);
	Action.WriteFloats(UpVector, 3);

	ApplyCheckpoint(mCurrentCheckpoint);
}

void lcModel::UpdateCameraTool(const lcVector3& Distance)
{
	lcMemFile& Action = mCurrentCheckpoint->mAction;

	lcCamera* NewCamera = (lcCamera*)mFocusObject;

	NewCamera->mTargetPosition += Distance;
	NewCamera->Update();

	Action.Seek(0, SEEK_SET);
	LC_ASSERT(Action.ReadU32() == LC_ACTION_CREATE_CAMERA);

	Action.Seek(3 * sizeof(float), SEEK_CUR);
	Action.WriteFloats(NewCamera->mTargetPosition, 3);

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateFocusObject();
}

void lcModel::EndCameraTool(bool Accept)
{
	EndCheckpoint(Accept);
	if (Accept)
	{
//		gMainWindow->UpdateCameraMenu(mCameras);
//		SetModifiedFlag(true);
	}
}

void lcModel::ApplyCheckpoint(lcCheckpoint* Checkpoint)
{
	lcMemFile& Action = Checkpoint->mAction;

	Action.Seek(0, SEEK_SET);
	lcActionType ActionType = (lcActionType)Action.ReadU32();

	switch (ActionType)
	{
	case LC_ACTION_CREATE_CAMERA:
		{
			lcVector3 Position, TargetPosition, UpVector;
			Action.ReadFloats(Position, 3);
			Action.ReadFloats(TargetPosition, 3);
			Action.ReadFloats(UpVector, 3);

			const char* Prefix = "Camera ";
			const int PrefixLength = strlen(Prefix);
			int Index, MaxIndex = 0;

			for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
			{
				lcObject* Object = mObjects[ObjectIdx];

				if (!Object->IsCamera())
					continue;

				lcCamera* Camera = (lcCamera*)Object;

				if (strncmp(Camera->mName, Prefix, PrefixLength))
					continue;

				if (sscanf(Camera->mName + PrefixLength, " %d", &Index) != 1)
					continue;

				if (Index > MaxIndex)
					MaxIndex = Index;
			}

			lcCamera* NewCamera = new lcCamera(false);
			mObjects.Add(NewCamera);

			NewCamera->mPosition = Position;
			NewCamera->mTargetPosition = TargetPosition;
			NewCamera->mUpVector = UpVector;
			sprintf(NewCamera->mName, "%s %d", Prefix, MaxIndex + 1);

			NewCamera->Update();

			lcObjectSection ObjectSection;
			ObjectSection.Object = NewCamera;
			ObjectSection.Section = LC_CAMERA_TARGET;
			SetFocus(ObjectSection);
		}
		break;
	}
}

void lcModel::RevertCheckpoint(lcCheckpoint* Checkpoint)
{
	lcMemFile& Action = Checkpoint->mAction;

	Action.Seek(0, SEEK_SET);
	lcActionType ActionType = (lcActionType)Action.ReadU32();

	switch (ActionType)
	{
	case LC_ACTION_CREATE_CAMERA:
		{
			lcCamera* Camera = (lcCamera*)mObjects[mObjects.GetSize() - 1];

			if (mFocusObject == Camera)
			{
				mFocusObject = NULL;
				gMainWindow->UpdateFocusObject();
			}

			int SelectedIndex = mSelectedObjects.FindIndex(Camera);
			if (SelectedIndex != -1)
			{
				mSelectedObjects.RemoveIndex(SelectedIndex);
				gMainWindow->UpdateSelection();
			}

			delete Camera;
			mObjects.RemoveIndex(mObjects.GetSize() - 1);

			gMainWindow->UpdateAllViews();
//			gMainWindow->UpdateCameraMenu();
		}
		break;
	}
}

void lcModel::RenderBackground(View* View) const
{
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*
	if ((m_nScene & (LC_SCENE_GRADIENT | LC_SCENE_BG)) == 0)
	{
		glClearColor(m_fBackground[0], m_fBackground[1], m_fBackground[2], 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	float ViewWidth = (float)view->mWidth;
	float ViewHeight = (float)view->mHeight;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, ViewWidth, 0.0f, ViewHeight, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0f);

	// Draw gradient quad.
	if (m_nScene & LC_SCENE_GRADIENT)
	{
		glShadeModel(GL_SMOOTH);

		float Verts[4][2];
		float Colors[4][4];

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, Verts);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 0, Colors);

		Colors[0][0] = m_fGradient1[0]; Colors[0][1] = m_fGradient1[1]; Colors[0][2] = m_fGradient1[2]; Colors[0][3] = 1.0f;
		Verts[0][0] = ViewWidth; Verts[0][1] = ViewHeight;
		Colors[1][0] = m_fGradient1[0]; Colors[1][1] = m_fGradient1[1]; Colors[1][2] = m_fGradient1[2]; Colors[1][3] = 1.0f;
		Verts[1][0] = 0; Verts[1][1] = ViewHeight;
		Colors[2][0] = m_fGradient2[0]; Colors[2][1] = m_fGradient2[1]; Colors[2][2] = m_fGradient2[2]; Colors[2][3] = 1.0f;
		Verts[2][0] = 0; Verts[2][1] = 0;
		Colors[3][0] = m_fGradient2[0]; Colors[3][1] = m_fGradient2[1]; Colors[3][2] = m_fGradient2[2]; Colors[3][3] = 1.0f;
		Verts[3][0] = ViewWidth; Verts[3][1] = 0;

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		glShadeModel(GL_FLAT);
	}

	// Draw the background picture.
	if (m_nScene & LC_SCENE_BG)
	{
		glEnable(GL_TEXTURE_2D);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBindTexture(GL_TEXTURE_2D, m_pBackground->mTexture);

		float Verts[4][2];
		float Coords[4][2];

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, Verts);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, Coords);

		float tw = 1.0f, th = 1.0f;
		if (m_nScene & LC_SCENE_BG_TILE)
		{
			tw = ViewWidth / m_pBackground->mWidth;
			th = ViewHeight / m_pBackground->mHeight;
		}

		Coords[0][0] = 0; Coords[0][1] = 0;
		Verts[0][0] = 0; Verts[0][1] = ViewHeight;
		Coords[1][0] = tw; Coords[1][1] = 0;
		Verts[1][0] = ViewWidth; Verts[1][1] = ViewHeight;
		Coords[2][0] = tw; Coords[2][1] = th;
		Verts[2][0] = ViewWidth; Verts[2][1] = 0;
		Coords[3][0] = 0; Coords[3][1] = th;
		Verts[3][0] = 0; Verts[3][1] = 0;

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glDisable(GL_TEXTURE_2D);
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	*/
}

//void lcModel::GetRenderMeshes(View* View, bool PartsOnly, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes) const
//{
//	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
//		mObjects[ObjectIdx]->GetRenderMeshes(View, PartsOnly, OpaqueMeshes, TranslucentMeshes);
//}

void lcModel::RenderObjects(View* View) const
{
	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
		mObjects[ObjectIdx]->RenderExtra(View);

	/*
#ifdef LC_DEBUG
	RenderDebugPrimitives();
#endif

	// Draw cameras & lights
	if (m_nCurAction == LC_TOOL_INSERT || mDropPiece)
	{
		lcVector3 Pos;
		lcVector4 Rot;
		GetPieceInsertPosition(view, m_nDownX, m_nDownY, Pos, Rot);
		PieceInfo* PreviewPiece = mDropPiece ? mDropPiece : m_pCurPiece;

		glPushMatrix();
		glTranslatef(Pos[0], Pos[1], Pos[2]);
		glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);
		glLineWidth(2*m_fLineWidth);
		PreviewPiece->RenderPiece(gMainWindow->mColorIndex);
		glLineWidth(m_fLineWidth);
		glPopMatrix();
	}

	if (m_nDetail & LC_DET_LIGHTING)
	{
		glDisable (GL_LIGHTING);
		int index = 0;
		Light *pLight;

		for (pLight = m_pLights; pLight; pLight = pLight->m_pNext, index++)
			glDisable ((GLenum)(GL_LIGHT0+index));
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = mCameras[CameraIdx];

//		if ((Camera == View->mCamera) || !Camera->IsVisible())
//			continue;

		Camera->Render();
	}

	for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		if (pLight->IsVisible ())
			pLight->Render(m_fLineWidth);

	// Draw axis icon
	if (m_nSnap & LC_DRAW_AXIS)
	{
//		glClear(GL_DEPTH_BUFFER_BIT);

		lcMatrix44 Mats[3];
		Mats[0] = view->mCamera->mWorldView;
		Mats[0].SetTranslation(lcVector3(0, 0, 0));
		Mats[1] = lcMul(lcMatrix44(lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 1, 0), lcVector4(0, 0, 0, 1)), Mats[0]);
		Mats[2] = lcMul(lcMatrix44(lcVector4(0, 0, 1, 0), lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 0, 1)), Mats[0]);

		lcVector3 pts[3] =
		{
			lcMul30(lcVector3(20, 0, 0), Mats[0]),
			lcMul30(lcVector3(0, 20, 0), Mats[0]),
			lcMul30(lcVector3(0, 0, 20), Mats[0]),
		};

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, view->mWidth, 0, view->mHeight, -50, 50);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(25.375f, 25.375f, 0.0f);

		// Draw the arrows.
		lcVector3 Verts[11];
		Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, Verts);

		for (int i = 0; i < 3; i++)
		{
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

			Verts[1] = pts[i];

			for (int j = 0; j < 9; j++)
				Verts[j+2] = lcMul30(lcVector3(12.0f, cosf(LC_2PI * j / 8) * 3.0f, sinf(LC_2PI * j / 8) * 3.0f), Mats[i]);

			glDrawArrays(GL_LINES, 0, 2);
			glDrawArrays(GL_TRIANGLE_FAN, 1, 10);
		}

		glDisableClientState(GL_VERTEX_ARRAY);

		// Draw the text.
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		m_pScreenFont->MakeCurrent();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);

		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		m_pScreenFont->PrintText(pts[0][0], pts[0][1], 40.0f, "X");
		m_pScreenFont->PrintText(pts[1][0], pts[1][1], 40.0f, "Y");
		m_pScreenFont->PrintText(pts[2][0], pts[2][1], 40.0f, "Z");

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
	}
*/
}

void lcModel::FindClosestObject(lcObjectHitTest& HitTest) const
{
	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
	{
//		if (visible)
//		if (camera != viewcamera)
		mObjects[ObjectIdx]->ClosestHitTest(HitTest);
	}
}

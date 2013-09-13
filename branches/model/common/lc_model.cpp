#include "lc_global.h"
#include "lc_model.h"
#include "lc_mainwindow.h"
#include "view.h"
#include "lc_object.h"
#include "lc_piece.h"
#include "lc_camera.h"
#include "lc_light.h"
#include "lc_mesh.h"
#include "lc_file.h"

class lcCheckpoint
{
public:
	lcCheckpoint(lcActionType ActionType)
	{
		mActionType = ActionType;
	}

	lcActionType mActionType;
	lcMemFile mApply;
	lcMemFile mRevert;
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
}

lcCamera* lcModel::GetCamera(int CameraIndex)
{
	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
		if (mObjects[ObjectIdx]->IsCamera())
			if (CameraIndex-- == 0)
				return (lcCamera*)mObjects[ObjectIdx];

	return NULL;
}

void lcModel::GetCameras(lcArray<lcCamera*>& Cameras)
{
	Cameras.RemoveAll();

	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
		if (mObjects[ObjectIdx]->IsCamera())
			Cameras.Add((lcCamera*)mObjects[ObjectIdx]);
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

void lcModel::BeginCheckpoint(lcActionType ActionType)
{
	LC_ASSERT(!mCurrentCheckpoint);
	mCurrentCheckpoint = new lcCheckpoint(ActionType);
}

void lcModel::EndCheckpoint(bool Accept, bool SaveCheckpoint)
{
	LC_ASSERT(mCurrentCheckpoint);

	if (Accept && SaveCheckpoint)
	{
		mUndoCheckpoints.Add(mCurrentCheckpoint);
		mRedoCheckpoints.DeleteAll();
	}
	else
	{
		if (!Accept)
			RevertCheckpoint(mCurrentCheckpoint);
		delete mCurrentCheckpoint;
	}

	mCurrentCheckpoint = NULL;
}

void lcModel::ToggleSelection(const lcObjectSection& ObjectSection)
{
	lcObject* Object = ObjectSection.Object;
	lcuint32 Section = ObjectSection.Section;

	if (!Object)
		return;

	bool WasSelected = Object->IsSelected();

	Object->ToggleSelection(Section);

	if (mFocusObject && !mFocusObject->IsFocused())
		mFocusObject = NULL;

	bool IsSelected = Object->IsSelected();

	if (WasSelected && !IsSelected)
		mSelectedObjects.Remove(Object);
	else if (!WasSelected && IsSelected)
		mSelectedObjects.Add(Object);

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
	gMainWindow->UpdateFocusObject();
}

void lcModel::AddToSelection(const lcArray<lcObjectSection>& ObjectSections)
{
	for (int ObjectIdx = 0; ObjectIdx < ObjectSections.GetSize(); ObjectIdx++)
	{
		lcObject* Object = ObjectSections[ObjectIdx].Object;

		bool WasSelected = Object->IsSelected();

		Object->SetSelection(ObjectSections[ObjectIdx].Section, true);

		if (!WasSelected)
			mSelectedObjects.Add(Object);
	}

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
}

void lcModel::SetSelection(const lcArray<lcObjectSection>& ObjectSections)
{
	for (int ObjectIdx = 0; ObjectIdx < mSelectedObjects.GetSize(); ObjectIdx++)
		mSelectedObjects[ObjectIdx]->ClearSelection();
	mSelectedObjects.RemoveAll();

	AddToSelection(ObjectSections);

	if (mFocusObject)
	{
		mFocusObject = NULL;
		gMainWindow->UpdateFocusObject();
	}
}

void lcModel::ToggleFocus(const lcObjectSection& ObjectSection)
{
	lcObject* Object = ObjectSection.Object;
	lcuint32 Section = ObjectSection.Section;

	if (!Object)
		return;

	bool WasSelected = Object->IsSelected();

	if (Object->IsFocused(Section))
	{
		Object->SetFocus(Section, false);
		mFocusObject = NULL;
	}
	else
	{
		if (mFocusObject)
			mFocusObject->ClearFocus();

		Object->SetFocus(Section, true);
		mFocusObject = Object;
	}

	bool IsSelected = Object->IsSelected();

	if (!WasSelected && IsSelected)
		mSelectedObjects.Add(Object);

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
	gMainWindow->UpdateFocusObject();
}

void lcModel::SetFocus(const lcObjectSection& ObjectSection)
{
	lcObject* Object = ObjectSection.Object;
	lcuint32 Section = ObjectSection.Section;

	for (int ObjectIdx = 0; ObjectIdx < mSelectedObjects.GetSize(); ObjectIdx++)
		mSelectedObjects[ObjectIdx]->ClearSelection();
	mSelectedObjects.RemoveAll();
	mFocusObject = NULL;

	if (Object)
	{
		mSelectedObjects.Add(Object);
		Object->SetFocus(Section, true);
		mFocusObject = Object;
	}

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
	gMainWindow->UpdateFocusObject();
}

void lcModel::BeginCameraTool(const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3& UpVector)
{
	BeginCheckpoint(LC_ACTION_CREATE_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.WriteFloats(lcVector3(0.0f, 0.0f, 0.0f), 3);

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
	sprintf(NewCamera->mName, "%s%d", Prefix, MaxIndex + 1);

	NewCamera->Update();

	lcObjectSection ObjectSection;
	ObjectSection.Object = NewCamera;
	ObjectSection.Section = LC_CAMERA_TARGET;
	SetFocus(ObjectSection);

	gMainWindow->UpdateCameraMenu();
}

void lcModel::UpdateCameraTool(const lcVector3& Distance, lcTime Time, bool AddKeys)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_CREATE_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	lcVector3 PreviousDistance;
	Apply.ReadFloats(PreviousDistance, 3);

	if (PreviousDistance == Distance)
		return;

	Apply.Seek(0, SEEK_SET);
	Apply.WriteFloats(Distance, 3);

	lcCamera* Camera = (lcCamera*)mObjects[mObjects.GetSize() - 1];
	Camera->Move(Distance, Time, AddKeys);
	Camera->Update();

	gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();
}

void lcModel::EndCameraTool(bool Accept)
{
	if (Accept)
	{
		lcMemFile& Apply = mCurrentCheckpoint->mApply;
		Apply.Seek(0, SEEK_SET);

		lcCamera* Camera = (lcCamera*)mObjects[mObjects.GetSize() - 1];
		Camera->Save(Apply);

//		SetModifiedFlag(true);
	}

	EndCheckpoint(Accept, true);
}

void lcModel::BeginMoveTool()
{
	BeginCheckpoint(LC_ACTION_MOVE_OBJECTS);
	lcMemFile& Revert = mCurrentCheckpoint->mRevert;

	Revert.WriteS32(mSelectedObjects.GetSize());
	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
	{
		if (!mObjects[ObjectIdx]->IsSelected())
			continue;

		Revert.WriteS32(ObjectIdx);
		mObjects[ObjectIdx]->Save(Revert);
	}

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.WriteFloats(lcVector3(0.0f, 0.0f, 0.0f), 3);
}

void lcModel::UpdateMoveTool(const lcVector3& Distance, lcTime Time, bool AddKeys)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_MOVE_OBJECTS);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	lcVector3 PreviousDistance;
	Apply.ReadFloats(PreviousDistance, 3);

	if (PreviousDistance == Distance)
		return;

	Apply.Seek(0, SEEK_SET);
	Apply.WriteFloats(Distance, 3);

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.Seek(0, SEEK_SET);

	lcint32 NumObjects = Revert.ReadS32();
	while (NumObjects--)
	{
		lcint32 ObjectIndex = Revert.ReadS32();
		mObjects[ObjectIndex]->Load(Revert);
		mObjects[ObjectIndex]->Move(Distance, Time, AddKeys);
		mObjects[ObjectIndex]->Update();
	}

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();
}

void lcModel::EndMoveTool(bool Accept)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_MOVE_OBJECTS);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	lcVector3 Distance;
	Apply.ReadFloats(Distance, 3);

	if (fabsf(Distance[0]) < 0.0001f && fabsf(Distance[1]) < 0.0001f && fabsf(Distance[2]) < 0.0001f)
		Accept = false;

	if (!mSelectedObjects.GetSize())
		Accept = false;

	if (Accept)
	{
		Apply.Seek(0, SEEK_SET);

		Apply.WriteS32(mSelectedObjects.GetSize());
		for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
		{
			if (!mObjects[ObjectIdx]->IsSelected())
				continue;

			Apply.WriteS32(ObjectIdx);
			mObjects[ObjectIdx]->Save(Apply);
		}

//		SetModifiedFlag(true);
	}

	EndCheckpoint(Accept, true);
}

void lcModel::BeginZoomTool()
{
	BeginCheckpoint(LC_ACTION_ZOOM_CAMERA);

	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.WriteS32(mObjects.FindIndex(Camera));
	Camera->Save(Revert);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.WriteFloat(0.0f);
}

void lcModel::UpdateZoomTool(float Distance, lcTime Time, bool AddKeys)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_ZOOM_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	float PreviousDistance = Apply.ReadFloat();

	if (PreviousDistance == Distance)
		return;

	Apply.Seek(0, SEEK_SET);
	Apply.WriteFloat(Distance);

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.Seek(0, SEEK_SET);

	Revert.ReadS32();
	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	Camera->Load(Revert);
	Camera->Zoom(Distance, Time, AddKeys);
	Camera->Update();

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();
}

void lcModel::EndZoomTool(bool Accept)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_ZOOM_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	float Distance = Apply.ReadFloat();

	if (fabsf(Distance) < 0.0001f)
		Accept = false;

	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	if (Accept)
	{
		Apply.Seek(0, SEEK_SET);

		Apply.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Apply);

//		SetModifiedFlag(true);
	}

	EndCheckpoint(Accept, !Camera->IsSimple());
}

void lcModel::BeginPanTool()
{
	BeginCheckpoint(LC_ACTION_PAN_CAMERA);

	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.WriteS32(mObjects.FindIndex(Camera));
	Camera->Save(Revert);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.WriteFloat(0.0f);
	Apply.WriteFloat(0.0f);
}

void lcModel::UpdatePanTool(float DistanceX, float DistanceY, lcTime Time, bool AddKeys)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_PAN_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	float PreviousDistanceX = Apply.ReadFloat();
	float PreviousDistanceY = Apply.ReadFloat();

	if (PreviousDistanceX == DistanceX && PreviousDistanceY == DistanceY)
		return;

	Apply.Seek(0, SEEK_SET);
	Apply.WriteFloat(DistanceX);
	Apply.WriteFloat(DistanceY);

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.Seek(0, SEEK_SET);

	Revert.ReadS32();
	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	Camera->Load(Revert);
	Camera->Pan(DistanceX, DistanceY, Time, AddKeys);
	Camera->Update();

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();
}

void lcModel::EndPanTool(bool Accept)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_PAN_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	float DistanceX = Apply.ReadFloat();
	float DistanceY = Apply.ReadFloat();

	if (fabsf(DistanceX) < 0.0001f && fabsf(DistanceY) < 0.0001f)
		Accept = false;

	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	if (Accept)
	{
		Apply.Seek(0, SEEK_SET);

		Apply.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Apply);

//		SetModifiedFlag(true);
	}

	EndCheckpoint(Accept, !Camera->IsSimple());
}

void lcModel::BeginOrbitTool(const lcVector3& Center)
{
	BeginCheckpoint(LC_ACTION_ORBIT_CAMERA);

	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.WriteS32(mObjects.FindIndex(Camera));
	Camera->Save(Revert);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.WriteFloat(0.0f);
	Apply.WriteFloat(0.0f);
	Apply.WriteFloats(Center, 3);
}

void lcModel::UpdateOrbitTool(float AngleX, float AngleY, lcTime Time, bool AddKeys)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_ORBIT_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	float PreviousAngleX = Apply.ReadFloat();
	float PreviousAngleY = Apply.ReadFloat();

	if (PreviousAngleX == AngleX && PreviousAngleY == AngleY)
		return;

	Apply.Seek(0, SEEK_SET);
	Apply.WriteFloat(AngleX);
	Apply.WriteFloat(AngleY);

	lcVector3 Center;
	Apply.ReadFloats(Center, 3);

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.Seek(0, SEEK_SET);

	Revert.ReadS32();
	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	Camera->Load(Revert);
	Camera->Orbit(AngleX, AngleY, Center, Time, AddKeys);
	Camera->Update();

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();
}

void lcModel::EndOrbitTool(bool Accept)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_ORBIT_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	float DistanceX = Apply.ReadFloat();
	float DistanceY = Apply.ReadFloat();

	if (fabsf(DistanceX) < 0.0001f && fabsf(DistanceY) < 0.0001f)
		Accept = false;

	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	if (Accept)
	{
		Apply.Seek(0, SEEK_SET);

		Apply.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Apply);

//		SetModifiedFlag(true);
	}

	EndCheckpoint(Accept, !Camera->IsSimple());
}

void lcModel::BeginRollTool()
{
	BeginCheckpoint(LC_ACTION_ROLL_CAMERA);

	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.WriteS32(mObjects.FindIndex(Camera));
	Camera->Save(Revert);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.WriteFloat(0.0f);
}

void lcModel::UpdateRollTool(float Angle, lcTime Time, bool AddKeys)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_ROLL_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	float PreviousAngle = Apply.ReadFloat();

	if (PreviousAngle == Angle)
		return;

	Apply.Seek(0, SEEK_SET);
	Apply.WriteFloat(Angle);

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.Seek(0, SEEK_SET);

	Revert.ReadS32();
	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	Camera->Load(Revert);
	Camera->Roll(Angle, Time, AddKeys);
	Camera->Update();

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();
}

void lcModel::EndRollTool(bool Accept)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_ROLL_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	float Angle = Apply.ReadFloat();

	if (fabsf(Angle) < 0.0001f)
		Accept = false;

	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	if (Accept)
	{
		Apply.Seek(0, SEEK_SET);

		Apply.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Apply);

//		SetModifiedFlag(true);
	}

	EndCheckpoint(Accept, !Camera->IsSimple());
}

void lcModel::ZoomExtents(View* View, const lcVector3& Center, const lcVector3* Points, lcTime Time, bool AddKeys)
{
	lcCamera* Camera = View->mCamera;

	if (!Camera->IsSimple())
	{
		BeginCheckpoint(LC_ACTION_ZOOM_EXTENTS);
		lcMemFile& Revert = mCurrentCheckpoint->mRevert;

		Revert.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Revert);
	}

	Camera->ZoomExtents(View, Center, Points, Time, AddKeys);
	Camera->Update();

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
	{
		lcMemFile& Apply = mCurrentCheckpoint->mApply;

		Apply.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Apply);

		EndCheckpoint(true, true);
	}
}

void lcModel::ZoomRegion(View* View, float Left, float Right, float Bottom, float Top, lcTime Time, bool AddKeys)
{
	lcCamera* Camera = View->mCamera;

	if (!Camera->IsSimple())
	{
		BeginCheckpoint(LC_ACTION_ZOOM_REGION);
		lcMemFile& Revert = mCurrentCheckpoint->mRevert;

		Revert.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Revert);
	}

	Camera->ZoomRegion(View, Left, Right, Bottom, Top, Time, AddKeys);
	Camera->Update();

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
	{
		lcMemFile& Apply = mCurrentCheckpoint->mApply;

		Apply.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Apply);

		EndCheckpoint(true, true);
	}
}

void lcModel::ApplyCheckpoint(lcCheckpoint* Checkpoint)
{
	lcActionType ActionType = Checkpoint->mActionType;
	lcMemFile& Apply = Checkpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	switch (ActionType)
	{
	case LC_ACTION_CREATE_CAMERA:
		{
			lcCamera* Camera = new lcCamera(false);
			Camera->Load(Apply);

			gMainWindow->UpdateAllViews();
			gMainWindow->UpdateCameraMenu();
		}
		break;

	case LC_ACTION_MOVE_OBJECTS:
		{
			lcint32 NumObjects = Apply.ReadS32();

			while (NumObjects--)
			{
				lcint32 ObjectIndex = Apply.ReadS32();
				mObjects[ObjectIndex]->Load(Apply);
				mObjects[ObjectIndex]->Update();
			}

			if (mFocusObject)
				gMainWindow->UpdateFocusObject();
			gMainWindow->UpdateAllViews();
		}
		break;

	case LC_ACTION_ZOOM_CAMERA:
	case LC_ACTION_PAN_CAMERA:
	case LC_ACTION_ORBIT_CAMERA:
	case LC_ACTION_ROLL_CAMERA:
	case LC_ACTION_ZOOM_EXTENTS:
	case LC_ACTION_ZOOM_REGION:
		{
			lcint32 ObjectIndex = Apply.ReadS32();
			lcCamera* Camera = (lcCamera*)mObjects[ObjectIndex];

			Camera->Load(Apply);
			Camera->Update();

			if (mFocusObject)
				gMainWindow->UpdateFocusObject();
			gMainWindow->UpdateAllViews();
		}
		break;
	}
}

void lcModel::RevertCheckpoint(lcCheckpoint* Checkpoint)
{
	lcActionType ActionType = Checkpoint->mActionType;
	lcMemFile& Revert = Checkpoint->mRevert;
	Revert.Seek(0, SEEK_SET);

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
			gMainWindow->UpdateCameraMenu();
		}
		break;

	case LC_ACTION_MOVE_OBJECTS:
		{
			lcint32 NumObjects = Revert.ReadS32();

			while (NumObjects--)
			{
				lcint32 ObjectIndex = Revert.ReadS32();
				mObjects[ObjectIndex]->Load(Revert);
				mObjects[ObjectIndex]->Update();
			}

			if (mFocusObject)
				gMainWindow->UpdateFocusObject();
			gMainWindow->UpdateAllViews();
		}
		break;

	case LC_ACTION_ZOOM_CAMERA:
	case LC_ACTION_PAN_CAMERA:
	case LC_ACTION_ORBIT_CAMERA:
	case LC_ACTION_ROLL_CAMERA:
	case LC_ACTION_ZOOM_EXTENTS:
	case LC_ACTION_ZOOM_REGION:
		{
			lcint32 ObjectIndex = Revert.ReadS32();
			lcCamera* Camera = ObjectIndex == -1 ? gMainWindow->mActiveView->mCamera : (lcCamera*)mObjects[ObjectIndex];

			Camera->Load(Revert);
			Camera->Update();

			if (mFocusObject)
				gMainWindow->UpdateFocusObject();
			gMainWindow->UpdateAllViews();
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

void lcModel::GetRenderMeshes(View* View, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes, lcArray<lcObject*> InterfaceObjects) const
{
	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
	{
// if visible
		mObjects[ObjectIdx]->GetRenderMeshes(View, OpaqueMeshes, TranslucentMeshes, InterfaceObjects);
	}
}

void lcModel::RenderScene(View* View, bool RenderInterface) const
{
	/*
	if (m_nDetail & LC_DET_LIGHTING)
	{
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
		glEnable(GL_COLOR_MATERIAL);
		glShadeModel(GL_SMOOTH);

		glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 64);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, lcVector4(0.8f, 0.8f, 0.8f, 1.0f));
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lcVector4(0.8f, 0.8f, 0.8f, 1.0f));

		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_fAmbient);

		int index = 0;
		Light *pLight;

		for (pLight = m_pLights; pLight; pLight = pLight->m_pNext, index++)
			pLight->Setup(index);

		glEnable(GL_LIGHTING);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (m_nScene & LC_SCENE_FOG)
	{
		glFogi(GL_FOG_MODE, GL_EXP);
		glFogf(GL_FOG_DENSITY, m_fFogDensity);
		glFogfv(GL_FOG_COLOR, m_fFogColor);
		glEnable(GL_FOG);
	}

	if (m_nScene & LC_SCENE_FLOOR)
		m_pTerrain->Render(View->mCamera, AspectRatio);

	glLineWidth(m_fLineWidth);
*/
	lcArray<lcRenderMesh> OpaqueMeshes;
	lcArray<lcRenderMesh> TranslucentMeshes;
	lcArray<lcObject*> InterfaceObjects;

	GetRenderMeshes(View, OpaqueMeshes, TranslucentMeshes, InterfaceObjects);

	OpaqueMeshes.Sort(lcOpaqueRenderMeshCompare);
	TranslucentMeshes.Sort(lcTranslucentRenderMeshCompare);

	lcRenderOpaqueMeshes(OpaqueMeshes);

	if (RenderInterface)
	{
		/*
		if (m_nDetail & LC_DET_LIGHTING)
		{
			glDisable(GL_LIGHTING);
			glDisable(GL_COLOR_MATERIAL);
			glShadeModel(GL_FLAT);
		}

		if (m_nScene & LC_SCENE_FOG)
			glDisable(GL_FOG);
		*/

// render grid
	}

	lcRenderTranslucentMeshes(TranslucentMeshes);
/*
	if (m_nDetail & LC_DET_LIGHTING)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (m_nScene & LC_SCENE_FOG)
		glDisable(GL_FOG);
*/

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

void lcModel::FindObjectsInBox(const lcVector4* BoxPlanes, lcArray<lcObjectSection>& ObjectSections) const
{
	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
	{
//		if (visible)
//		if (camera != viewcamera)
		mObjects[ObjectIdx]->BoxTest(BoxPlanes, ObjectSections);
	}
}

void lcModel::AddPiece(PieceInfo* Part, int ColorIndex, const lcVector3& Position, const lcVector4& AxisAngle, lcTime Time)
{
	lcPiece* Piece = new lcPiece(Part, ColorIndex, Position, AxisAngle, Time);
	Piece->Update();
	mObjects.Add(Piece);

	BeginCheckpoint(LC_ACTION_CREATE_PIECE);
	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Piece->Save(Apply);
	EndCheckpoint(true, true);

	lcObjectSection ObjectSection;
	ObjectSection.Object = Piece;
	ObjectSection.Section = 0;
	SetFocus(ObjectSection);

//	SystemPieceComboAdd(m_pCurPiece->m_strDescription);
//	SetModifiedFlag(true);
}

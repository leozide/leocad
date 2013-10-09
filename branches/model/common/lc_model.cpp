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
#include "lc_profile.h"
#include "lc_application.h"
#include "pieceinf.h"

enum
{
	LC_CHECKPOINT_CREATE_OBJECTS,
	LC_CHECKPOINT_REMOVE_OBJECTS,
	LC_CHECKPOINT_LOAD_OBJECTS
};

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

void lcModelProperties::LoadDefaults()
{
	mAuthor = lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME);

	mBackgroundType = (lcBackgroundType)lcGetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TILE);
	mBackgroundSolidColor = lcGetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_COLOR);
	mBackgroundGradientColor1 = lcGetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR1);
	mBackgroundGradientColor2 = lcGetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR2);
	mBackgroundImage = lcGetProfileString(LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE);
	mBackgroundImageTile = lcGetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TILE);

	mFogEnabled = lcGetProfileInt(LC_PROFILE_DEFAULT_FOG_ENABLED);
	mFogDensity = lcGetProfileFloat(LC_PROFILE_DEFAULT_FOG_DENSITY);
	mFogColor = lcGetProfileInt(LC_PROFILE_DEFAULT_FOG_COLOR);
	mAmbientColor = lcGetProfileInt(LC_PROFILE_DEFAULT_AMBIENT_COLOR);
}

void lcModelProperties::SaveDefaults()
{
	lcSetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TYPE, mBackgroundType);
	lcSetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_COLOR, mBackgroundSolidColor);
	lcSetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR1, mBackgroundGradientColor1);
	lcSetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR2, mBackgroundGradientColor2);
	lcSetProfileString(LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE, mBackgroundImage);
	lcSetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TILE, mBackgroundImageTile);

	lcSetProfileInt(LC_PROFILE_DEFAULT_FOG_ENABLED, mFogEnabled);
	lcSetProfileFloat(LC_PROFILE_DEFAULT_FOG_DENSITY, mFogDensity);
	lcSetProfileInt(LC_PROFILE_DEFAULT_FOG_COLOR, mFogColor);
	lcSetProfileInt(LC_PROFILE_DEFAULT_AMBIENT_COLOR, mAmbientColor);
}

lcModel::lcModel()
{
	mCurrentTime = 1;
	mCurrentCheckpoint = NULL;
	mFocusObject = NULL;
	mProperties.LoadDefaults();
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

void lcModel::ShowPropertiesDialog()
{
	lcPropertiesDialogOptions Options;

	Options.Properties = mProperties;
	Options.SetDefault = false;

	GetPartsUsed(Options.PartsUsed);

	if (!gMainWindow->DoDialog(LC_DIALOG_PROPERTIES, &Options))
		return;

	if (Options.SetDefault)
		Options.Properties.SaveDefaults();

	if (mProperties == Options.Properties)
		return;

// if model name changed update menu

	mProperties = Options.Properties;

// reload background

//	SetModifiedFlag(true);
//	CheckPoint("Properties");
}

void lcModel::GetPartsUsed(lcArray<lcObjectParts>& PartsUsed) const
{
	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
		mObjects[ObjectIdx]->GetPartsUsed(PartsUsed);
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

lcVector3 lcModel::GetFocusOrSelectionCenter() const
{
	if (mFocusObject)
		return mFocusObject->GetFocusPosition();

//	for (int ObjectIdx = 0; ObjectIdx < mSelectedObjects.GetSize(); ObjectIdx++)
//		mSelectedObjects[ObjectIdx]->get

	return lcVector3(0.0f, 0.0f, 0.0f);
}

void lcModel::UndoCheckpoint()
{
	int UndoCount = mUndoCheckpoints.GetSize();

	if (!UndoCount)
		return;

	lcCheckpoint* Checkpoint = mUndoCheckpoints[UndoCount - 1];

	ApplyCheckpoint(Checkpoint->mRevert);

	mUndoCheckpoints.RemoveIndex(UndoCount - 1);
	mRedoCheckpoints.Add(Checkpoint);

	gMainWindow->UpdateCheckpoint();
}

void lcModel::RedoCheckpoint()
{
	int RedoCount = mRedoCheckpoints.GetSize();

	if (!RedoCount)
		return;

	lcCheckpoint* Checkpoint = mRedoCheckpoints[RedoCount - 1];

	ApplyCheckpoint(Checkpoint->mApply);

	mRedoCheckpoints.RemoveIndex(RedoCount - 1);
	mUndoCheckpoints.Add(Checkpoint);

	gMainWindow->UpdateCheckpoint();
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
		gMainWindow->UpdateCheckpoint();
	}
	else
	{
		if (!Accept)
			ApplyCheckpoint(mCurrentCheckpoint->mRevert);
		delete mCurrentCheckpoint;
	}

	mCurrentCheckpoint = NULL;
}

void lcModel::InvertSelection()
{
	mSelectedObjects.RemoveAll();

	if (mFocusObject)
	{
		mFocusObject->ClearFocus();
		mFocusObject = NULL;
	}

	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
	{
		lcObject* Object = mObjects[ObjectIdx];
		Object->InvertSelection();

		if (Object->IsSelected())
			mSelectedObjects.Add(Object);
	}

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
	gMainWindow->UpdateFocusObject();
}

void lcModel::InvertSelection(const lcObjectSection& ObjectSection)
{
	lcObject* Object = ObjectSection.Object;
	lcuintptr Section = ObjectSection.Section;

	if (!Object)
		return;

	bool WasSelected = Object->IsSelected();

	Object->InvertSelection(Section);

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
		mSelectedObjects[ObjectIdx]->SetSelection(false);
	mSelectedObjects.RemoveAll();

	AddToSelection(ObjectSections);

	if (mFocusObject)
	{
		mFocusObject = NULL;
		gMainWindow->UpdateFocusObject();
	}
}

void lcModel::SelectAllObjects()
{
	mSelectedObjects = mObjects;
	for (int ObjectIdx = 0; ObjectIdx < mSelectedObjects.GetSize(); ObjectIdx++)
		mSelectedObjects[ObjectIdx]->SetSelection(true);

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
}

void lcModel::ClearSelection()
{
	for (int ObjectIdx = 0; ObjectIdx < mSelectedObjects.GetSize(); ObjectIdx++)
		mSelectedObjects[ObjectIdx]->SetSelection(false);
	mSelectedObjects.RemoveAll();

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();

	if (mFocusObject)
	{
		mFocusObject = NULL;
		gMainWindow->UpdateFocusObject();
	}
}

void lcModel::ClearSelectionOrSetFocus(const lcObjectSection& ObjectSection)
{
	lcObject* Object = ObjectSection.Object;
	lcuintptr Section = ObjectSection.Section;

	if (!Object)
		return;

	bool WasSelected = Object->IsSelected();

	if (!Object->IsFocused(Section))
	{
		if (mFocusObject)
			mFocusObject->ClearFocus();

		Object->SetFocus(Section, true);
		mFocusObject = Object;
	}
	else
		Object->SetSelection(Section, false);

	bool IsSelected = Object->IsSelected();

	if (!WasSelected && IsSelected)
		mSelectedObjects.Add(Object);
	else if (WasSelected && !IsSelected)
		mSelectedObjects.Remove(Object);

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
	gMainWindow->UpdateFocusObject();
}

void lcModel::SetFocus(const lcObjectSection& ObjectSection)
{
	lcObject* Object = ObjectSection.Object;
	lcuintptr Section = ObjectSection.Section;

	for (int ObjectIdx = 0; ObjectIdx < mSelectedObjects.GetSize(); ObjectIdx++)
		mSelectedObjects[ObjectIdx]->SetSelection(false);
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

lcTime lcModel::GetTotalTime() const
{
// todo: total time
	return 1000;
}

void lcModel::SetCurrentTime(lcTime Time)
{
	Time = lcClamp(Time, (lcTime)1, (lcTime)~0 - 1);

	if (mCurrentTime == Time)
		return;

	mCurrentTime = Time;

	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
	{
		lcObject* Object = mObjects[ObjectIdx];

		Object->SetCurrentTime(mCurrentTime);

		if (!Object->IsSelected() || Object->IsVisible())
			continue;

		Object->SetSelection(false);

		if (mFocusObject == Object)
			mFocusObject = NULL;

		mSelectedObjects.Remove(Object);
	}

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
	gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateCurrentTime();
}

void lcModel::HideSelectedObjects()
{
	for (int ObjectIdx = 0; ObjectIdx < mSelectedObjects.GetSize(); ObjectIdx++)
	{
		lcObject* Object = mSelectedObjects[ObjectIdx];

		Object->SetSelection(false);
		Object->SetVisible(false);
	}
	mSelectedObjects.RemoveAll();
	mFocusObject = NULL;

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
	gMainWindow->UpdateFocusObject();
}

void lcModel::HideUnselectedObjects()
{
	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
	{
		lcObject* Object = mObjects[ObjectIdx];
		if (!Object->IsSelected())
			Object->SetVisible(false);
	}

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
}

void lcModel::UnhideAllObjects()
{
	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
		mObjects[ObjectIdx]->SetVisible(true);

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();
}

void lcModel::BeginCreateCameraTool(const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3& UpVector)
{
	BeginCheckpoint(LC_ACTION_CREATE_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.WriteFloats(TargetPosition, 3);

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

void lcModel::UpdateCreateCameraTool(const lcVector3& TargetPosition, bool AddKeys)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_CREATE_CAMERA);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	lcVector3 PreviousTargetPosition;
	Apply.ReadFloats(PreviousTargetPosition, 3);

	if (PreviousTargetPosition == TargetPosition)
		return;

	Apply.Seek(0, SEEK_SET);
	Apply.WriteFloats(TargetPosition, 3);

	lcCamera* Camera = (lcCamera*)mObjects[mObjects.GetSize() - 1];
	Camera->Move(TargetPosition - Camera->mTargetPosition, mCurrentTime, AddKeys);
	Camera->Update();

	gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();
}

void lcModel::EndCreateCameraTool(bool Accept)
{
	if (Accept)
	{
		lcMemFile& Apply = mCurrentCheckpoint->mApply;
		Apply.Seek(0, SEEK_SET);
		lcint32 ObjectIndex = mObjects.GetSize() - 1;

		Apply.WriteU32(LC_CHECKPOINT_CREATE_OBJECTS);
		Apply.WriteS32(1);
		Apply.WriteS32(ObjectIndex);
		Apply.WriteU32(LC_OBJECT_TYPE_CAMERA);
		mObjects[ObjectIndex]->Save(Apply);

		lcMemFile& Revert = mCurrentCheckpoint->mRevert;
		Revert.Seek(0, SEEK_SET);
		Revert.WriteU32(LC_CHECKPOINT_REMOVE_OBJECTS);
		Revert.WriteU32(1);
		Revert.WriteU32(ObjectIndex);

//		SetModifiedFlag(true);
	}

	EndCheckpoint(Accept, true);
}

void lcModel::BeginMoveTool()
{
	BeginCheckpoint(LC_ACTION_MOVE_OBJECTS);
	lcMemFile& Revert = mCurrentCheckpoint->mRevert;

	Revert.WriteU32(LC_CHECKPOINT_LOAD_OBJECTS);
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

void lcModel::UpdateMoveTool(const lcVector3& Distance, bool AddKeys)
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

	Revert.ReadU32();
	lcint32 NumObjects = Revert.ReadS32();
	while (NumObjects--)
	{
		lcint32 ObjectIndex = Revert.ReadS32();
		mObjects[ObjectIndex]->Load(Revert);
		mObjects[ObjectIndex]->Move(Distance, mCurrentTime, AddKeys);
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

		Apply.WriteU32(LC_CHECKPOINT_LOAD_OBJECTS);
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

void lcModel::BeginRotateTool()
{
	BeginCheckpoint(LC_ACTION_ROTATE_OBJECTS);
	lcMemFile& Revert = mCurrentCheckpoint->mRevert;

	Revert.WriteU32(LC_CHECKPOINT_LOAD_OBJECTS);
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

void lcModel::UpdateRotateTool(const lcVector3& Angles, bool AddKeys)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_ROTATE_OBJECTS);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	lcVector3 PreviousAngles;
	Apply.ReadFloats(PreviousAngles, 3);

	if (PreviousAngles == Angles)
		return;

	Apply.Seek(0, SEEK_SET);
	Apply.WriteFloats(Angles, 3);

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.Seek(0, SEEK_SET);

	Revert.ReadU32();
	lcint32 NumObjects = Revert.ReadS32();
	while (NumObjects--)
	{
		lcint32 ObjectIndex = Revert.ReadS32();
		mObjects[ObjectIndex]->Load(Revert);
//		mObjects[ObjectIndex]->Rotate(Angles, Time, AddKeys);
		mObjects[ObjectIndex]->Update();
	}

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();
}

void lcModel::EndRotateTool(bool Accept)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == LC_ACTION_MOVE_OBJECTS);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	lcVector3 Angles;
	Apply.ReadFloats(Angles, 3);

	if (fabsf(Angles[0]) < 0.0001f && fabsf(Angles[1]) < 0.0001f && fabsf(Angles[2]) < 0.0001f)
		Accept = false;

	if (!mSelectedObjects.GetSize())
		Accept = false;

	if (Accept)
	{
		Apply.Seek(0, SEEK_SET);

		Apply.WriteU32(LC_CHECKPOINT_LOAD_OBJECTS);
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

void lcModel::BeginEditCameraTool(lcActionType ActionType, const lcVector3& Center)
{
	BeginCheckpoint(ActionType);

	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.WriteU32(LC_CHECKPOINT_LOAD_OBJECTS);
	Revert.WriteS32(1);
	Revert.WriteS32(mObjects.FindIndex(Camera));
	Camera->Save(Revert);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.WriteFloat(0.0f);
	Apply.WriteFloat(0.0f);
	Apply.WriteFloats(Center, 3);
}

void lcModel::UpdateEditCameraTool(lcActionType ActionType, float ValueX, float ValueY, bool AddKeys)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == ActionType);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	float PreviousValueX = Apply.ReadFloat();
	float PreviousValueY = Apply.ReadFloat();

	if (PreviousValueX == ValueX && PreviousValueY == ValueY)
		return;

	lcVector3 Center;
	Apply.ReadFloats(Center, 3);

	Apply.Seek(0, SEEK_SET);
	Apply.WriteFloat(ValueX);
	Apply.WriteFloat(ValueY);

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.Seek(0, SEEK_SET);

	Revert.ReadS32();
	Revert.ReadS32();
	Revert.ReadS32();
	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	Camera->Load(Revert);
	switch (ActionType)
	{
	case LC_ACTION_ZOOM_CAMERA:
		Camera->Zoom(ValueX, mCurrentTime, AddKeys);
		break;

	case LC_ACTION_PAN_CAMERA:
		Camera->Pan(ValueX, ValueY, mCurrentTime, AddKeys);
		break;

	case LC_ACTION_ORBIT_CAMERA:
		Camera->Orbit(ValueX, ValueY, Center, mCurrentTime, AddKeys);
		break;

	case LC_ACTION_ROLL_CAMERA:
		Camera->Roll(ValueX, mCurrentTime, AddKeys);
		break;

	default:
		LC_ASSERT(0);
		break;
	}
	Camera->Update();

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();
}

void lcModel::EndEditCameraTool(lcActionType ActionType, bool Accept)
{
	LC_ASSERT(mCurrentCheckpoint->mActionType == ActionType);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;
	Apply.Seek(0, SEEK_SET);

	float ValueX = Apply.ReadFloat();
	float ValueY = Apply.ReadFloat();

	if (fabsf(ValueX) < 0.0001f && fabsf(ValueY) < 0.0001f)
		Accept = false;

	lcCamera* Camera = gMainWindow->mActiveView->mCamera;

	if (Accept)
	{
		Apply.Seek(0, SEEK_SET);

		Apply.WriteU32(LC_CHECKPOINT_LOAD_OBJECTS);
		Apply.WriteS32(1);
		Apply.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Apply);

//		SetModifiedFlag(true);
	}

	EndCheckpoint(Accept, !Camera->IsSimple());
}

void lcModel::ZoomExtents(View* View, const lcVector3& Center, const lcVector3* Points, bool AddKeys)
{
	lcCamera* Camera = View->mCamera;

	if (!Camera->IsSimple())
	{
		BeginCheckpoint(LC_ACTION_ZOOM_EXTENTS);
		lcMemFile& Revert = mCurrentCheckpoint->mRevert;

		Revert.WriteU32(LC_CHECKPOINT_LOAD_OBJECTS);
		Revert.WriteS32(1);
		Revert.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Revert);
	}

	Camera->ZoomExtents(View, Center, Points, mCurrentTime, AddKeys);
	Camera->Update();

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
	{
		lcMemFile& Apply = mCurrentCheckpoint->mApply;

		Apply.WriteU32(LC_CHECKPOINT_LOAD_OBJECTS);
		Apply.WriteS32(1);
		Apply.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Apply);

		EndCheckpoint(true, true);
	}
}

void lcModel::ZoomRegion(View* View, float Left, float Right, float Bottom, float Top, bool AddKeys)
{
	lcCamera* Camera = View->mCamera;

	if (!Camera->IsSimple())
	{
		BeginCheckpoint(LC_ACTION_ZOOM_REGION);
		lcMemFile& Revert = mCurrentCheckpoint->mRevert;

		Revert.WriteU32(LC_CHECKPOINT_LOAD_OBJECTS);
		Revert.WriteS32(1);
		Revert.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Revert);
	}

	Camera->ZoomRegion(View, Left, Right, Bottom, Top, mCurrentTime, AddKeys);
	Camera->Update();

	if (mFocusObject)
		gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
	{
		lcMemFile& Apply = mCurrentCheckpoint->mApply;

		Apply.WriteU32(LC_CHECKPOINT_LOAD_OBJECTS);
		Apply.WriteS32(1);
		Apply.WriteS32(mObjects.FindIndex(Camera));
		Camera->Save(Apply);

		EndCheckpoint(true, true);
	}
}

void lcModel::ApplyCheckpoint(lcMemFile& File)
{
	File.Seek(0, SEEK_SET);
	lcuint32 CheckpointType = File.ReadU32();

	switch (CheckpointType)
	{
	case LC_CHECKPOINT_CREATE_OBJECTS:
		{
			lcint32 ObjectCount = File.ReadS32();
			bool UpdateCameraMenu = false;

			while (ObjectCount--)
			{
				lcObjectType ObjectType = (lcObjectType)File.ReadS32();
				lcint32 ObjectIndex = File.ReadS32();
				lcObject* Object = NULL;

				switch (ObjectType)
				{
				case LC_OBJECT_TYPE_PIECE:
					Object = new lcPiece();
					break;

				case LC_OBJECT_TYPE_CAMERA:
					Object = new lcCamera(false);
					break;
				}

				Object->Load(File);
				Object->Update();
				mObjects.InsertAt(ObjectIndex, Object);
			}

			if (UpdateCameraMenu)
				gMainWindow->UpdateCameraMenu();

			gMainWindow->UpdateAllViews();
		}
		break;

	case LC_CHECKPOINT_REMOVE_OBJECTS:
		{
			lcint32 ObjectCount = File.ReadS32();
			bool UpdateSelection = false;
			bool UpdateCameraMenu = false;
			int ObjectsRemoved = 0;

			while (ObjectCount--)
			{
				lcint32 ObjectIndex = File.ReadS32();
				lcObject* Object = mObjects[ObjectIndex - ObjectsRemoved];

				if (mFocusObject == Object)
				{
					mFocusObject = NULL;
					gMainWindow->UpdateFocusObject();
				}

				if (Object->IsSelected())
				{
					mSelectedObjects.Remove(Object);
					UpdateSelection = true;
				}
// if view camera fix view

				if (Object->IsCamera())
					UpdateCameraMenu = true;

				ObjectsRemoved++;
				mObjects.RemoveIndex(ObjectIndex);
				delete Object;
			}

			if (UpdateSelection)
				gMainWindow->UpdateSelection();

			if (UpdateCameraMenu)
				gMainWindow->UpdateCameraMenu();

			gMainWindow->UpdateAllViews();
		}
		break;

	case LC_CHECKPOINT_LOAD_OBJECTS:
		{
			lcint32 NumObjects = File.ReadS32();

			while (NumObjects--)
			{
				lcint32 ObjectIndex = File.ReadS32();
				lcObject* Object = ObjectIndex == -1 ? gMainWindow->mActiveView->mCamera : mObjects[ObjectIndex];

				Object->Load(File);
				Object->Update();
			}

			if (mFocusObject)
				gMainWindow->UpdateFocusObject();
			gMainWindow->UpdateAllViews();
		}
		break;
	}
}

void lcModel::DrawBackground(lcGLWidget* Widget) const
{
	if (mProperties.mBackgroundType == LC_BACKGROUND_SOLID)
	{
		lcVector4 Color = lcVector4FromColor(mProperties.mBackgroundSolidColor);
		glClearColor(Color[0], Color[1], Color[2], 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}

	glClear(GL_DEPTH_BUFFER_BIT);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	float ViewWidth = (float)Widget->mWidth;
	float ViewHeight = (float)Widget->mHeight;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, ViewWidth, 0.0f, ViewHeight, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0f);

	float Verts[4][2] = { { ViewWidth, ViewHeight }, { 0.0f, ViewHeight }, { 0.0f, 0.0f }, { ViewWidth, 0.0f } };
	glVertexPointer(2, GL_FLOAT, 0, Verts);

	if (mProperties.mBackgroundType == LC_BACKGROUND_GRADIENT)
	{
		glShadeModel(GL_SMOOTH);

		lcVector4 Colors[4];

		lcVector4 TopColor = lcVector4FromColor(mProperties.mBackgroundGradientColor1);
		lcVector4 BottomColor = lcVector4FromColor(mProperties.mBackgroundGradientColor2);

		Colors[0] = TopColor;
		Colors[1] = TopColor;
		Colors[2] = BottomColor;
		Colors[3] = BottomColor;

		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 0, Colors);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_COLOR_ARRAY);

		glShadeModel(GL_FLAT);
	}
	else
	{
		/*
		glEnable(GL_TEXTURE_2D);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBindTexture(GL_TEXTURE_2D, m_pBackground->mTexture);

		float Coords[4][2];

		float tw = 1.0f, th = 1.0f;
		if (mProperties.mBackgroundImageTile)
		{
			tw = ViewWidth / m_pBackground->mWidth;
			th = ViewHeight / m_pBackground->mHeight;
		}

		Coords[0][0] = 0; Coords[0][1] = 0;
		Coords[1][0] = tw; Coords[1][1] = 0;
		Coords[2][0] = tw; Coords[2][1] = th;
		Coords[3][0] = 0; Coords[3][1] = th;

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, Coords);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glDisable(GL_TEXTURE_2D);
		*/
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}

void lcModel::DrawScene(View* View, bool DrawInterface) const
{
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	float AspectRatio = (float)View->mWidth / (float)View->mHeight;
	View->mCamera->LoadProjection(AspectRatio);

	lcPreferences* Preferences = lcGetPreferences();

	if (Preferences->mLightingMode == LC_LIGHTING_FULL)
	{
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
		glEnable(GL_COLOR_MATERIAL);
		glShadeModel(GL_SMOOTH);

		glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 64);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, lcVector4(0.8f, 0.8f, 0.8f, 1.0f));
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lcVector4(0.8f, 0.8f, 0.8f, 1.0f));

		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lcVector4FromColor(mProperties.mAmbientColor));

		/*
		int index = 0;
		Light *pLight;

		for (pLight = m_pLights; pLight; pLight = pLight->m_pNext, index++)
			pLight->Setup(index);
		*/

		glEnable(GL_LIGHTING);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (mProperties.mFogEnabled)
	{
		glFogi(GL_FOG_MODE, GL_EXP);
		glFogf(GL_FOG_DENSITY, mProperties.mFogDensity);
		glFogfv(GL_FOG_COLOR, lcVector4FromColor(mProperties.mFogColor));
		glEnable(GL_FOG);
	}

	glLineWidth(Preferences->mLineWidth);

	lcArray<lcRenderMesh> OpaqueMeshes(mObjects.GetSize());
	lcArray<lcRenderMesh> TranslucentMeshes;
	lcArray<lcObject*> InterfaceObjects;

	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
		mObjects[ObjectIdx]->GetRenderMeshes(View, OpaqueMeshes, TranslucentMeshes, InterfaceObjects);

	OpaqueMeshes.Sort(lcOpaqueRenderMeshCompare);
	TranslucentMeshes.Sort(lcTranslucentRenderMeshCompare);

	lcRenderOpaqueMeshes(OpaqueMeshes);

//	if (m_nScene & LC_SCENE_FLOOR)
//		m_pTerrain->Render(View->mCamera, AspectRatio);

	if (DrawInterface && (InterfaceObjects.GetSize() || Preferences->mDrawGridLines || Preferences->mDrawGridStuds))
	{
		if (Preferences->mLightingMode != LC_LIGHTING_FLAT)
		{
			glDisable(GL_LIGHTING);
			glDisable(GL_COLOR_MATERIAL);
			glShadeModel(GL_FLAT);
		}

		if (mProperties.mFogEnabled)
			glDisable(GL_FOG);

		for (int ObjectIdx = 0; ObjectIdx < InterfaceObjects.GetSize(); ObjectIdx++)
			InterfaceObjects[ObjectIdx]->RenderInterface(View);

		if (Preferences->mDrawGridLines || Preferences->mDrawGridStuds)
			DrawGrid();

		if (Preferences->mLightingMode != LC_LIGHTING_FLAT)
		{
			glEnable(GL_LIGHTING);
			glEnable(GL_COLOR_MATERIAL);
			glShadeModel(GL_SMOOTH);
		}

		if (mProperties.mFogEnabled)
			glEnable(GL_FOG);
	}

	lcRenderTranslucentMeshes(TranslucentMeshes);

	if (Preferences->mLightingMode != LC_LIGHTING_FLAT)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (mProperties.mFogEnabled)
		glDisable(GL_FOG);

/*
#ifdef LC_DEBUG
	RenderDebugPrimitives();
#endif

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
*/
}

void lcModel::DrawGrid() const
{
	lcPreferences* Preferences = lcGetPreferences();

	const int Spacing = lcMax(Preferences->mGridLineSpacing, 1);
	int MinX = 0, MaxX = 0, MinY = 0, MaxY = 0;
/*
	if (m_pPieces || (m_nCurAction == LC_TOOL_INSERT || mDropPiece))
	{
		float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };

		for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			if (pPiece->IsVisible(m_nCurStep, m_bAnimation))
				pPiece->CompareBoundingBox(bs);

		if (m_nCurAction == LC_TOOL_INSERT || mDropPiece)
		{
			lcVector3 Position;
			lcVector4 Rotation;
			GetPieceInsertPosition(view, m_nDownX, m_nDownY, Position, Rotation);

			PieceInfo* PreviewPiece = mDropPiece ? mDropPiece : m_pCurPiece;

			lcVector3 Points[8] =
			{
				lcVector3(PreviewPiece->m_fDimensions[0],PreviewPiece->m_fDimensions[1], PreviewPiece->m_fDimensions[5]),
				lcVector3(PreviewPiece->m_fDimensions[3],PreviewPiece->m_fDimensions[1], PreviewPiece->m_fDimensions[5]),
				lcVector3(PreviewPiece->m_fDimensions[0],PreviewPiece->m_fDimensions[1], PreviewPiece->m_fDimensions[2]),
				lcVector3(PreviewPiece->m_fDimensions[3],PreviewPiece->m_fDimensions[4], PreviewPiece->m_fDimensions[5]),
				lcVector3(PreviewPiece->m_fDimensions[3],PreviewPiece->m_fDimensions[4], PreviewPiece->m_fDimensions[2]),
				lcVector3(PreviewPiece->m_fDimensions[0],PreviewPiece->m_fDimensions[4], PreviewPiece->m_fDimensions[2]),
				lcVector3(PreviewPiece->m_fDimensions[0],PreviewPiece->m_fDimensions[4], PreviewPiece->m_fDimensions[5]),
				lcVector3(PreviewPiece->m_fDimensions[3],PreviewPiece->m_fDimensions[1], PreviewPiece->m_fDimensions[2])
			};

			lcMatrix44 ModelWorld = lcMatrix44FromAxisAngle(lcVector3(Rotation[0], Rotation[1], Rotation[2]), Rotation[3] * LC_DTOR);
			ModelWorld.SetTranslation(Position);

			for (int i = 0; i < 8; i++)
			{
				lcVector3 Point = lcMul31(Points[i], ModelWorld);

				if (Point[0] < bs[0]) bs[0] = Point[0];
				if (Point[1] < bs[1]) bs[1] = Point[1];
				if (Point[2] < bs[2]) bs[2] = Point[2];
				if (Point[0] > bs[3]) bs[3] = Point[0];
				if (Point[1] > bs[4]) bs[4] = Point[1];
				if (Point[2] > bs[5]) bs[5] = Point[2];
			}
		}

		MinX = (int)(floorf(bs[0] / (0.8f * Spacing))) - 1;
		MinY = (int)(floorf(bs[1] / (0.8f * Spacing))) - 1;
		MaxX = (int)(ceilf(bs[3] / (0.8f * Spacing))) + 1;
		MaxY = (int)(ceilf(bs[4] / (0.8f * Spacing))) + 1;
	}
*/
	MinX = lcMin(MinX, -2);
	MinY = lcMin(MinY, -2);
	MaxX = lcMax(MaxX, 2);
	MaxY = lcMax(MaxY, 2);
/*
	if (mGridStuds)
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
		glBindTexture(GL_TEXTURE_2D, mGridTexture->mTexture);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glColor4fv(lcVector4FromColor(mGridStudColor));

		glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), Verts);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), Verts + 3);

		glDrawArrays(GL_QUADS, 0, 4);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
	}
*/
	if (Preferences->mDrawGridLines)
	{
		glColor4fv(lcVector4FromColor(Preferences->mGridLineColor));

		int NumVerts = 2 * (MaxX - MinX + MaxY - MinY + 2);
		float* VertexBuffer = new float[NumVerts * 2];
		float* Vert = VertexBuffer;
		float LineSpacing = Spacing * 0.8f;

		for (int Step = MinX; Step < MaxX + 1; Step++)
		{
			*Vert++ = Step * LineSpacing;
			*Vert++ = MinY * LineSpacing;
			*Vert++ = Step * LineSpacing;
			*Vert++ = MaxY * LineSpacing;
		}

		for (int Step = MinY; Step < MaxY + 1; Step++)
		{
			*Vert++ = MinX * LineSpacing;
			*Vert++ = Step * LineSpacing;
			*Vert++ = MaxX * LineSpacing;
			*Vert++ = Step * LineSpacing;
		}

		glVertexPointer(2, GL_FLOAT, 0, VertexBuffer);
		glDrawArrays(GL_LINES, 0, NumVerts);
		delete[] VertexBuffer;
	}
}

void lcModel::FindClosestObject(lcObjectHitTest& HitTest, bool PiecesOnly) const
{
	for (int ObjectIdx = 0; ObjectIdx < mObjects.GetSize(); ObjectIdx++)
	{
//		if (visible)
//		if (camera != viewcamera)
		mObjects[ObjectIdx]->ClosestHitTest(HitTest, PiecesOnly);
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

void lcModel::AddPiece(PieceInfo* Part, int ColorIndex)
{
	lcVector3 Position(0.0f, 0.0f, -Part->m_fDimensions[5]);
	lcVector4 AxisAngle(0.0f, 0.0f, 1.0f, 0.0f);

	if (mFocusObject)
	{
		lcPiece* Piece = mFocusObject->GetPiece(mFocusObject->GetFocusSection());

		if (Piece)
		{
			lcVector3 Dist(0, 0, Piece->mPieceInfo->m_fDimensions[2] - Part->m_fDimensions[5]);
			Dist = lcGetActiveProject()->SnapVector(Dist);

			Position = lcMul31(Dist, Piece->mModelWorld);
			AxisAngle = Piece->mAxisAngle;
		}
	}

	AddPiece(Part, ColorIndex, Position, AxisAngle);
}

void lcModel::AddPiece(PieceInfo* Part, int ColorIndex, const lcVector3& Position, const lcVector4& AxisAngle)
{
	lcPiece* Piece = new lcPiece(Part, ColorIndex, Position, AxisAngle, mCurrentTime);
	Piece->Update();
	mObjects.Add(Piece);

	BeginCheckpoint(LC_ACTION_CREATE_PIECE);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;

	Apply.WriteU32(LC_CHECKPOINT_CREATE_OBJECTS);
	Apply.WriteS32(1);
	Apply.WriteS32(mObjects.GetSize() - 1);
	Apply.WriteS32(LC_OBJECT_TYPE_PIECE);
	Piece->Save(Apply);

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;
	Revert.WriteU32(LC_CHECKPOINT_REMOVE_OBJECTS);
	Revert.WriteS32(1);
	Revert.WriteS32(mObjects.GetSize() - 1);

	EndCheckpoint(true, true);

	lcObjectSection ObjectSection;
	ObjectSection.Object = Piece;
	ObjectSection.Section = 0;
	SetFocus(ObjectSection);

//	SystemPieceComboAdd(m_pCurPiece->m_strDescription);
//	SetModifiedFlag(true);
}

void lcModel::RemoveObject(lcObject* Object)
{
	if (!Object)
		return;

	lcArray<lcObject*> Objects;
	Objects.Add(Object);

	RemoveObjects(Objects);
}

void lcModel::RemoveObjects(const lcArray<lcObject*>& Objects)
{
	if (!Objects.GetSize())
		return;

	BeginCheckpoint(LC_ACTION_REMOVE_OBJECT);

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;

	Revert.WriteU32(LC_CHECKPOINT_CREATE_OBJECTS);
	Revert.WriteS32(Objects.GetSize());

	lcMemFile& Apply = mCurrentCheckpoint->mApply;

	Apply.WriteU32(LC_CHECKPOINT_REMOVE_OBJECTS);
	Apply.WriteS32(Objects.GetSize());

	for (int ObjectIdx = 0; ObjectIdx < Objects.GetSize(); ObjectIdx++)
	{
		lcObject* Object = Objects[ObjectIdx];
		int ObjectIndex = mObjects.FindIndex(Object);
		LC_ASSERT(ObjectIndex != -1);

		Revert.WriteS32(ObjectIndex);
		Revert.WriteU32(Object->ObjectType());
		Object->Save(Revert);

		Apply.WriteS32(ObjectIndex);
	}

	EndCheckpoint(true, true);

	ApplyCheckpoint(Apply);

//	SetModifiedFlag(true);
}

void lcModel::CopyToClipboard()
{
	if (!mSelectedObjects.GetSize())
		return;

	lcMemFile* Clipboard = new lcMemFile();

	Clipboard->WriteS32(mSelectedObjects.GetSize());

	for (int ObjectIdx = 0; ObjectIdx < mSelectedObjects.GetSize(); ObjectIdx++)
	{
		lcObject* Object = mSelectedObjects[ObjectIdx];

		Clipboard->WriteU32(Object->ObjectType());
		Object->Save(*Clipboard);
	}

	g_App->ExportClipboard(Clipboard);
}

void lcModel::PasteFromClipboard()
{
	lcFile* Clipboard = g_App->mClipboard;
	if (!Clipboard)
		return;

	for (int ObjectIdx = 0; ObjectIdx < mSelectedObjects.GetSize(); ObjectIdx++)
		mSelectedObjects[ObjectIdx]->SetSelection(false);
	mSelectedObjects.RemoveAll();

	if (mFocusObject)
	{
		mFocusObject = NULL;
		gMainWindow->UpdateFocusObject();
	}

	Clipboard->Seek(0, SEEK_SET);

	lcint32 ObjectCount = Clipboard->ReadS32();
	bool UpdateCameraMenu = false;

	BeginCheckpoint(LC_ACTION_PASTE_OBJECTS);

	lcMemFile& Revert = mCurrentCheckpoint->mRevert;

	Revert.WriteU32(LC_CHECKPOINT_REMOVE_OBJECTS);
	Revert.WriteS32(ObjectCount);

	lcMemFile& Apply = mCurrentCheckpoint->mApply;

	Apply.WriteU32(LC_CHECKPOINT_CREATE_OBJECTS);
	Apply.WriteU32(ObjectCount);

	while (ObjectCount--)
	{
		lcObjectType ObjectType = (lcObjectType)Clipboard->ReadS32();
		lcObject* Object = NULL;

		switch (ObjectType)
		{
		case LC_OBJECT_TYPE_PIECE:
			Object = new lcPiece();
			break;

		case LC_OBJECT_TYPE_CAMERA:
			Object = new lcCamera(false);
			break;
		}

		Object->Load(*Clipboard);
// set step show to current step
		Object->Update();
		mObjects.Add(Object);

		Object->SetSelection(true);
		mSelectedObjects.Add(Object);

		Apply.WriteS32(mObjects.GetSize() - 1);
		Apply.WriteS32(ObjectType);
		Object->Save(Apply);

		Revert.WriteS32(mObjects.GetSize() - 1);
	}

	EndCheckpoint(true, true);

	if (UpdateCameraMenu)
		gMainWindow->UpdateCameraMenu();

	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateSelection();

//	SetModifiedFlag(true);
}

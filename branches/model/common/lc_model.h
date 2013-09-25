#ifndef LC_MODEL_H
#define LC_MODEL_H

#include "lc_array.h"
#include "lc_math.h"
#include "str.h"

class View;
class PieceInfo;
class lcCheckpoint;
struct lcObjectHitTest;
struct lcObjectSection;
struct lcObjectParts;
struct lcRenderMesh;

enum lcBackgroundType
{
	LC_BACKGROUND_SOLID,
	LC_BACKGROUND_GRADIENT,
	LC_BACKGROUND_IMAGE
};

class lcModelProperties
{
public:
	void LoadDefaults();
	void SaveDefaults();

	lcModelProperties& operator=(const lcModelProperties& Properties)
	{
		mName = Properties.mName;
		mAuthor = Properties.mAuthor;
		mDescription = Properties.mDescription;
		mComments = Properties.mComments;

		mBackgroundType = Properties.mBackgroundType;
		mBackgroundSolidColor = Properties.mBackgroundSolidColor;
		mBackgroundGradientColor1 = Properties.mBackgroundGradientColor1;
		mBackgroundGradientColor2 = Properties.mBackgroundGradientColor2;
		mBackgroundImage = Properties.mBackgroundImage;
		mBackgroundImageTile = Properties.mBackgroundImageTile;

		mFogEnabled = Properties.mFogEnabled;
		mFogDensity = Properties.mFogDensity;
		mFogColor = Properties.mFogColor;
		mAmbientColor = Properties.mAmbientColor;

		return *this;
	}

	bool operator==(const lcModelProperties& Properties)
	{
		if (mName != Properties.mName || mAuthor != Properties.mAuthor ||
			mDescription != Properties.mDescription || mComments != Properties.mComments)
			return false;

		if (mBackgroundType != Properties.mBackgroundType || mBackgroundSolidColor != Properties.mBackgroundSolidColor ||
			mBackgroundGradientColor1 != Properties.mBackgroundGradientColor1 || mBackgroundGradientColor2 != Properties.mBackgroundGradientColor2 ||
			mBackgroundImage != Properties.mBackgroundImage || mBackgroundImageTile != Properties.mBackgroundImageTile)
			return false;

		if (mFogEnabled != Properties.mFogEnabled || mFogDensity != Properties.mFogDensity ||
			mFogColor != Properties.mFogColor || mAmbientColor != Properties.mAmbientColor)
			return false;

		return true;
	}

	String mName;
	String mAuthor;
	String mDescription;
	String mComments;

	lcBackgroundType mBackgroundType;
	lcuint32 mBackgroundSolidColor;
	lcuint32 mBackgroundGradientColor1;
	lcuint32 mBackgroundGradientColor2;
	String mBackgroundImage;
	bool mBackgroundImageTile;

	bool mFogEnabled;
	float mFogDensity;
	lcuint32 mFogColor;
	lcuint32 mAmbientColor;
};

enum lcActionType
{
	LC_ACTION_REMOVE_OBJECT,
	LC_ACTION_PASTE_OBJECTS,
	LC_ACTION_CREATE_PIECE,
	LC_ACTION_CREATE_CAMERA,
	LC_ACTION_MOVE_OBJECTS,
	LC_ACTION_ROTATE_OBJECTS,
	LC_ACTION_ZOOM_CAMERA,
	LC_ACTION_PAN_CAMERA,
	LC_ACTION_ORBIT_CAMERA,
	LC_ACTION_ROLL_CAMERA,
	LC_ACTION_ZOOM_EXTENTS,
	LC_ACTION_ZOOM_REGION
};

class lcModel
{
public:
	lcModel();
	~lcModel();

	void ShowPropertiesDialog();

	lcObject* GetFocusObject() const
	{
		return mFocusObject;
	}

	const lcArray<lcObject*>& GetSelectedObjects() const
	{
		return mSelectedObjects;
	}

	void GetPartsUsed(lcArray<lcObjectParts>& PartsUsed) const;
	lcCamera* GetCamera(int CameraIndex);
	void GetCameras(lcArray<lcCamera*>& Cameras);

	void UndoCheckpoint();
	void RedoCheckpoint();

	void RenderBackground(View* View) const;
	void RenderScene(View* View, bool RenderInterface) const;
	void GetRenderMeshes(View* View, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes, lcArray<lcObject*>& InterfaceObjects) const;

	void InvertSelection();
	void InvertSelection(const lcObjectSection& ObjectSection);
	void AddToSelection(const lcArray<lcObjectSection>& ObjectSections);
	void SetSelection(const lcArray<lcObjectSection>& ObjectSections);
	void ClearSelectionOrSetFocus(const lcObjectSection& ObjectSection);
	void SetFocus(const lcObjectSection& ObjectSection);

	void HideSelectedObjects();
	void HideUnselectedObjects();
	void UnhideAllObjects();

	void FindClosestObject(lcObjectHitTest& HitTest) const;
	void FindObjectsInBox(const lcVector4* BoxPlanes, lcArray<lcObjectSection>& ObjectSections) const;

	void AddPiece(PieceInfo* Part, int ColorIndex, const lcVector3& Position, const lcVector4& AxisAngle, lcTime Time);
	void RemoveObject(lcObject* Object);
	void RemoveObjects(const lcArray<lcObject*>& Objects);
	void CopyToClipboard();
	void PasteFromClipboard();

	void BeginCreateCameraTool(const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3& UpVector);
	void UpdateCreateCameraTool(const lcVector3& Distance, lcTime Time, bool AddKeys);
	void EndCreateCameraTool(bool Accept);

	void BeginMoveTool();
	void UpdateMoveTool(const lcVector3& Distance, lcTime Time, bool AddKeys);
	void EndMoveTool(bool Accept);

	void BeginRotateTool();
	void UpdateRotateTool(const lcVector3& Angles, lcTime Time, bool AddKeys);
	void EndRotateTool(bool Accept);

	void BeginEditCameraTool(lcActionType ActionType, const lcVector3& Center);
	void UpdateEditCameraTool(lcActionType ActionType, float ValueX, float ValueY, lcTime Time, bool AddKeys);
	void EndEditCameraTool(lcActionType ActionType, bool Accept);

	void ZoomExtents(View* View, const lcVector3& Center, const lcVector3* Points, lcTime Time, bool AddKeys);
	void ZoomRegion(View* View, float Left, float Right, float Bottom, float Top, lcTime Time, bool AddKeys);

protected:
	void DeleteContents();
	void Update(lcTime Time);

	void BeginCheckpoint(lcActionType ActionType);
	void EndCheckpoint(bool Accept, bool SaveCheckpoint);
	void ApplyCheckpoint(lcMemFile& File);

	lcCheckpoint* mCurrentCheckpoint;
	lcArray<lcCheckpoint*> mUndoCheckpoints;
	lcArray<lcCheckpoint*> mRedoCheckpoints;

	lcObject* mFocusObject;
	lcArray<lcObject*> mSelectedObjects;

	lcArray<lcObject*> mObjects;

	lcModelProperties mProperties;
};

#endif // LC_MODEL_H

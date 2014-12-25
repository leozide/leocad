#ifndef _LC_MODEL_H_
#define _LC_MODEL_H_

#include "lc_file.h"
#include "lc_math.h"
#include "str.h"
#include "object.h"

#define LC_SEL_NO_PIECES     0x01 // No pieces in model
#define LC_SEL_PIECE         0x02 // At last 1 piece selected
#define LC_SEL_SELECTED      0x04 // At last 1 object selected
#define LC_SEL_UNSELECTED    0x08 // At least 1 piece unselected
#define LC_SEL_HIDDEN        0x10 // At least one piece hidden
#define LC_SEL_GROUPED       0x20 // At least one piece selected is grouped
#define LC_SEL_FOCUS_GROUPED 0x40 // Focused piece is grouped
#define LC_SEL_CAN_GROUP     0x80 // Can make a new group

enum lcTransformType
{
	LC_TRANSFORM_ABSOLUTE_TRANSLATION,
	LC_TRANSFORM_RELATIVE_TRANSLATION,
	LC_TRANSFORM_ABSOLUTE_ROTATION,
	LC_TRANSFORM_RELATIVE_ROTATION
};

enum lcBackgroundType
{
	LC_BACKGROUND_SOLID,
	LC_BACKGROUND_GRADIENT,
	LC_BACKGROUND_IMAGE,
	LC_NUM_BACKGROUND_TYPES
};

class lcModelProperties
{
public:
	void LoadDefaults();
	void SaveDefaults();

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

	void SaveLDraw(QTextStream& Stream) const;
	void ParseLDrawLine(QTextStream& Stream);

	QString mName;
	QString mAuthor;
	QString mDescription;
	QString mComments;

	lcBackgroundType mBackgroundType;
	lcVector3 mBackgroundSolidColor;
	lcVector3 mBackgroundGradientColor1;
	lcVector3 mBackgroundGradientColor2;
	QString mBackgroundImage;
	bool mBackgroundImageTile;

	bool mFogEnabled;
	float mFogDensity;
	lcVector3 mFogColor;
	lcVector3 mAmbientColor;
};

enum lcTool
{
	LC_TOOL_INSERT,
	LC_TOOL_LIGHT,
	LC_TOOL_SPOTLIGHT,
	LC_TOOL_CAMERA,
	LC_TOOL_SELECT,
	LC_TOOL_MOVE,
	LC_TOOL_ROTATE,
	LC_TOOL_ERASER,
	LC_TOOL_PAINT,
	LC_TOOL_ZOOM,
	LC_TOOL_PAN,
	LC_TOOL_ROTATE_VIEW,
	LC_TOOL_ROLL,
	LC_TOOL_ZOOM_REGION
};

struct lcModelHistoryEntry
{
	QByteArray File;
	QString Description;
};

struct lcPartsListEntry
{
	PieceInfo* Info;
	int ColorIndex;
	int Count;
};

class lcModel
{
public:
	lcModel(const QString& Name);
	~lcModel();

	bool IsModified() const
	{
		return mSavedHistory != mUndoHistory[0];
	}

	bool IncludesModel(const lcModel* Model) const;
	void CreatePieceInfo();

	PieceInfo* GetPieceInfo() const
	{
		return mPieceInfo;
	}

	const lcArray<lcPiece*>& GetPieces() const
	{
		return mPieces;
	}

	const lcArray<lcCamera*>& GetCameras() const
	{
		return mCameras;
	}

	const lcArray<lcLight*>& GetLights() const
	{
		return mLights;
	}

	const lcArray<lcGroup*>& GetGroups() const
	{
		return mGroups;
	}

	const lcModelProperties& GetProperties() const
	{
		return mProperties;
	}

	void SetName(const QString& Name)
	{
		mProperties.mName = Name;
	}

	lcStep GetLastStep() const;

	lcStep GetCurrentStep() const
	{
		return mCurrentStep;
	}

	void CalculateStep(lcStep Step);
	void SetCurrentStep(lcStep Step)
	{
		mCurrentStep = Step;
		CalculateStep(Step);
	}

	void ShowFirstStep();
	void ShowLastStep();
	void ShowPreviousStep();
	void ShowNextStep();
	void InsertStep();
	void RemoveStep();

	void AddPiece();
	void DeleteSelectedObjects();
	void ShowSelectedPiecesEarlier();
	void ShowSelectedPiecesLater();

	lcGroup* AddGroup(const char* Prefix, lcGroup* Parent);
	lcGroup* GetGroup(const char* Name, bool CreateIfMissing);
	void RemoveGroup(lcGroup* Group);
	void GroupSelection();
	void UngroupSelection();
	void AddSelectedPiecesToGroup();
	void RemoveFocusPieceFromGroup();
	void ShowEditGroupsDialog();

	void SaveLDraw(QTextStream& Stream, bool SelectedOnly) const;
	void LoadLDraw(QTextStream& Stream);
	bool LoadBinary(lcFile* File);
	void Merge(lcModel* Other);

	void SetSaved()
	{
		if (mUndoHistory.IsEmpty())
			SaveCheckpoint(QString());

		mSavedHistory = mUndoHistory[0];
	}

	void Cut();
	void Copy();
	void Paste();

	void GetScene(lcScene& Scene, lcCamera* ViewCamera, bool DrawInterface) const;
	void AddRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int DefaultColorIndex, bool Focused, bool Selected) const;
	void DrawBackground(lcContext* Context);

	void RayTest(lcObjectRayTest& ObjectRayTest) const;
	void BoxTest(lcObjectBoxTest& ObjectBoxTest) const;

	bool AnyPiecesSelected() const;
	bool AnyObjectsSelected() const;
	bool GetPieceFocusOrSelectionCenter(lcVector3& Center) const;
	lcVector3 GetFocusOrSelectionCenter() const;
	bool GetFocusPosition(lcVector3& Position) const;
	lcObject* GetFocusObject() const;
	bool GetSelectionCenter(lcVector3& Center) const;
	bool GetPiecesBoundingBox(float BoundingBox[6]) const;
	void GetPartsList(lcArray<lcPartsListEntry>& PartsList) const;

	void FocusOrDeselectObject(const lcObjectSection& ObjectSection);
	void ClearSelection(bool UpdateInterface);
	void ClearSelectionAndSetFocus(lcObject* Object, lcuint32 Section);
	void ClearSelectionAndSetFocus(const lcObjectSection& ObjectSection);
	void SetSelection(const lcArray<lcObject*>& Objects);
	void AddToSelection(const lcArray<lcObject*>& Objects);
	void SelectAllPieces();
	void InvertSelection();

	void HideSelectedPieces();
	void HideUnselectedPieces();
	void UnhideAllPieces();

	void FindPiece(bool FindFirst, bool SearchForward);

	void UndoAction();
	void RedoAction();

	lcVector3 LockVector(const lcVector3& Vector) const;
	lcVector3 SnapPosition(const lcVector3& Delta) const;
	lcVector3 SnapRotation(const lcVector3& Delta) const;
	lcMatrix44 GetRelativeRotation() const;

	const lcVector3& GetMouseToolDistance() const
	{
		return mMouseToolDistance;
	}

	void BeginMouseTool();
	void EndMouseTool(lcTool Tool, bool Accept);
	void InsertPieceToolClicked(const lcVector3& Position, const lcVector4& Rotation);
	void PointLightToolClicked(const lcVector3& Position);
	void BeginSpotLightTool(const lcVector3& Position, const lcVector3& Target);
	void UpdateSpotLightTool(const lcVector3& Position);
	void BeginCameraTool(const lcVector3& Position, const lcVector3& Target);
	void UpdateCameraTool(const lcVector3& Position);
	void UpdateMoveTool(const lcVector3& Distance);
	void UpdateRotateTool(const lcVector3& Angles);
	void EraserToolClicked(lcObject* Object);
	void PaintToolClicked(lcObject* Object);
	void UpdateZoomTool(lcCamera* Camera, float Mouse);
	void UpdatePanTool(lcCamera* Camera, float MouseX, float MouseY);
	void UpdateOrbitTool(lcCamera* Camera, float MouseX, float MouseY);
	void UpdateRollTool(lcCamera* Camera, float Mouse);
	void ZoomRegionToolClicked(lcCamera* Camera, const lcVector3* Points, float RatioX, float RatioY);
	void LookAt(lcCamera* Camera);
	void ZoomExtents(lcCamera* Camera, float Aspect);
	void Zoom(lcCamera* Camera, float Amount);

	void MoveSelectedObjects(const lcVector3& Distance, bool Update)
	{
		MoveSelectedObjects(Distance, Distance, Update);
	}

	void MoveSelectedObjects(const lcVector3& PieceDistance, const lcVector3& ObjectDistance, bool Update);
	void RotateSelectedPieces(const lcVector3& Angles, bool Update);
	void TransformSelectedObjects(lcTransformType TransformType, const lcVector3& Transform);
	void SetObjectProperty(lcObject* Object, lcObjectPropertyType ObjectPropertyType, const void* Value);

	void ShowPropertiesDialog();
	void ShowSelectByNameDialog();
	void ShowArrayDialog();
	void ShowMinifigDialog();
	void UpdateInterface();

	void Export3DStudio() const;
	void ExportBrickLink() const;
	void ExportCSV() const;
	void ExportPOVRay() const;
	void ExportWavefront() const;

protected:
	void DeleteModel();
	void DeleteHistory();
	void SaveCheckpoint(const QString& Description);
	void LoadCheckPoint(lcModelHistoryEntry* CheckPoint);

	void GetGroupName(const char* Prefix, char* GroupName);
	void RemoveEmptyGroups();
	bool RemoveSelectedObjects();

	void UpdateBackgroundTexture();

	void UpdateSelection() const;
	void SelectGroup(lcGroup* TopGroup, bool Select);

	lcModelProperties mProperties;
	PieceInfo* mPieceInfo;

	lcStep mCurrentStep;
	lcVector3 mMouseToolDistance;
	lcTexture* mBackgroundTexture;

	lcArray<lcPiece*> mPieces;
	lcArray<lcCamera*> mCameras;
	lcArray<lcLight*> mLights;
	lcArray<lcGroup*> mGroups;

	lcModelHistoryEntry* mSavedHistory;
	lcArray<lcModelHistoryEntry*> mUndoHistory;
	lcArray<lcModelHistoryEntry*> mRedoHistory;

	Q_DECLARE_TR_FUNCTIONS(lcModel);
};

#endif // _LC_MODEL_H_

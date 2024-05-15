#pragma once

#include "lc_math.h"
#include "lc_commands.h"
#include "lc_array.h"

enum class lcObjectPropertyId;

#define LC_SEL_NO_PIECES                0x0001 // No pieces in model
#define LC_SEL_PIECE                    0x0002 // At least 1 piece selected
#define LC_SEL_CAMERA                   0x0004 // At least 1 camera selected
#define LC_SEL_LIGHT                    0x0008 // At least 1 light selected
#define LC_SEL_SELECTED                 0x0010 // At least 1 object selected
#define LC_SEL_UNSELECTED               0x0020 // At least 1 piece unselected
#define LC_SEL_HIDDEN                   0x0040 // At least one piece hidden
#define LC_SEL_HIDDEN_SELECTED          0x0080 // At least one piece selected is hidden
#define LC_SEL_VISIBLE_SELECTED         0x0100 // At least one piece selected is not hidden
#define LC_SEL_GROUPED                  0x0200 // At least one piece selected is grouped
#define LC_SEL_FOCUS_GROUPED            0x0400 // Focused piece is grouped
#define LC_SEL_CAN_GROUP                0x0800 // Can make a new group
#define LC_SEL_MODEL_SELECTED           0x1000 // At least one model reference is selected
#define LC_SEL_CAN_ADD_CONTROL_POINT    0x2000 // Can add control points to focused piece
#define LC_SEL_CAN_REMOVE_CONTROL_POINT 0x4000 // Can remove control points from focused piece

enum class lcSelectionMode
{
	Single,
	Piece,
	Color,
	PieceColor
};

enum class lcTransformType
{
	First,
	AbsoluteTranslation = First,
	RelativeTranslation,
	AbsoluteRotation,
	RelativeRotation,
	Count
};

class lcModelProperties
{
public:
	void LoadDefaults();
	void SaveDefaults();

	bool operator==(const lcModelProperties& Properties) const
	{
		if (mFileName != Properties.mFileName || mModelName != Properties.mModelName || mAuthor != Properties.mAuthor ||
			mDescription != Properties.mDescription || mComments != Properties.mComments)
			return false;

		if (mAmbientColor != Properties.mAmbientColor)
			return false;

		return true;
	}

	void SaveLDraw(QTextStream& Stream) const;
	bool ParseLDrawHeader(QString Line, bool FirstLine);
	void ParseLDrawLine(QTextStream& Stream);

	QString mFileName;
	QString mDescription;
	QString mModelName;
	QString mAuthor;
	QString mComments;

	lcVector3 mAmbientColor;
};

class lcPOVRayOptions
{
public:
	lcPOVRayOptions();
	void ParseLDrawLine(QTextStream& LineStream);
	void SaveLDraw(QTextStream& Stream) const;

	bool UseLGEO;
	bool ExcludeFloor;
	bool ExcludeBackground;
	bool NoReflection;
	bool NoShadow;
	int FloorAxis;
	float FloorAmbient;
	float FloorDiffuse;
	lcVector3 FloorColor;
	QString HeaderIncludeFile;
	QString FooterIncludeFile;
};

struct lcModelHistoryEntry
{
	QByteArray File;
	QString Description;
};

class lcModel
{
public:
	lcModel(const QString& FileName, Project* Project, bool Preview);
	~lcModel();

	lcModel(const lcModel&) = delete;
	lcModel(lcModel&&) = delete;
	lcModel& operator=(const lcModel&) = delete;
	lcModel& operator=(lcModel&&) = delete;

	Project* GetProject() const
	{
		return mProject;
	}

	bool IsModified() const
	{
		return mSavedHistory != mUndoHistory[0];
	}

	bool IsActive() const
	{
		return mActive;
	}

	bool GetPieceWorldMatrix(lcPiece* Piece, lcMatrix44& ParentWorldMatrix) const;
	bool IncludesModel(const lcModel* Model) const;
	void CreatePieceInfo(Project* Project);
	void UpdatePieceInfo(std::vector<lcModel*>& UpdatedModels);
	void UpdateMesh();
	void UpdateAllViews() const;

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

	const lcPOVRayOptions& GetPOVRayOptions() const
	{
		return mPOVRayOptions;
	}

	void SetFileName(const QString& FileName)
	{
		if (mProperties.mModelName == mProperties.mFileName)
			mProperties.mModelName = FileName;
		mProperties.mFileName = FileName;
	}

	const QString& GetFileName() const
	{
		return mProperties.mFileName;
	}

	void SetDescription(const QString& Description)
	{
		mProperties.mDescription = Description;
	}

	const QStringList& GetFileLines() const
	{
		return mFileLines;
	}

	lcStep GetLastStep() const;

	lcStep GetCurrentStep() const
	{
		return mCurrentStep;
	}

	void SetActive(bool Active);
	void CalculateStep(lcStep Step);
	void SetCurrentStep(lcStep Step);
	void SetTemporaryStep(lcStep Step)
	{
		mCurrentStep = Step;
		CalculateStep(Step);
	}

	void ShowFirstStep();
	void ShowLastStep();
	void ShowPreviousStep();
	void ShowNextStep();
	void InsertStep(lcStep Step);
	void RemoveStep(lcStep Step);

	void AddPiece();
	void DeleteAllCameras();
	void DeleteSelectedObjects();
	void ResetSelectedPiecesPivotPoint();
	void RemoveSelectedPiecesKeyFrames();
	void InsertControlPoint();
	void RemoveFocusedControlPoint();
	void ShowSelectedPiecesEarlier();
	void ShowSelectedPiecesLater();
	void SetPieceSteps(const QList<QPair<lcPiece*, lcStep>>& PieceSteps);
	void RenamePiece(PieceInfo* Info);

	void MoveSelectionToModel(lcModel* Model);
	void InlineSelectedModels();

	lcGroup* AddGroup(const QString& Prefix, lcGroup* Parent);
	lcGroup* GetGroup(const QString& Name, bool CreateIfMissing);
	void RemoveGroup(lcGroup* Group);
	void GroupSelection();
	void UngroupSelection();
	void AddSelectedPiecesToGroup();
	void RemoveFocusPieceFromGroup();
	void ShowEditGroupsDialog();

	void SaveLDraw(QTextStream& Stream, bool SelectedOnly, lcStep LastStep) const;
	void LoadLDraw(QIODevice& Device, Project* Project);
	bool LoadBinary(lcFile* File);
	bool LoadLDD(const QString& FileData);
	bool LoadInventory(const QByteArray& Inventory);
	int SplitMPD(QIODevice& Device);
	void Merge(lcModel* Other);

	void SetSaved()
	{
		if (mUndoHistory.empty())
			SaveCheckpoint(QString());

		if (!mIsPreview)
			mSavedHistory = mUndoHistory[0];
	}

	void SetMinifig(const lcMinifig& Minifig);
	void SetPreviewPieceInfo(PieceInfo* Info, int ColorIndex);

	void Cut();
	void Copy();
	void Paste(bool PasteToCurrentStep);
	void DuplicateSelectedPieces();
	void PaintSelectedPieces();

	void GetScene(lcScene* Scene, const lcCamera* ViewCamera, bool AllowHighlight, bool AllowFade) const;
	void AddSubModelRenderMeshes(lcScene* Scene, const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const;
	QImage GetStepImage(bool Zoom, int Width, int Height, lcStep Step);
	QImage GetPartsListImage(int MaxWidth, lcStep Step, quint32 BackgroundColor, QFont Font, QColor TextColor) const;
	void SaveStepImages(const QString& BaseName, bool AddStepSuffix, bool Zoom, int Width, int Height, lcStep Start, lcStep End);

	void RayTest(lcObjectRayTest& ObjectRayTest) const;
	void BoxTest(lcObjectBoxTest& ObjectBoxTest) const;
	bool SubModelMinIntersectDist(const lcVector3& WorldStart, const lcVector3& WorldEnd, float& MinDistance, lcPieceInfoRayTest& PieceInfoRayTest) const;
	bool SubModelBoxTest(const lcVector4 Planes[6]) const;
	void SubModelCompareBoundingBox(const lcMatrix44& WorldMatrix, lcVector3& Min, lcVector3& Max) const;
	void SubModelAddBoundingBoxPoints(const lcMatrix44& WorldMatrix, std::vector<lcVector3>& Points) const;

	bool HasPieces() const
	{
		return !mPieces.IsEmpty();
	}

	bool AnyPiecesSelected() const;
	bool AnyObjectsSelected() const;
	lcModel* GetFirstSelectedSubmodel() const;
	void GetSubModels(lcArray<lcModel*>& SubModels) const;
	bool GetMoveRotateTransform(lcVector3& Center, lcMatrix33& RelativeRotation) const;
	bool CanRotateSelection() const;
	bool GetPieceFocusOrSelectionCenter(lcVector3& Center) const;
	lcVector3 GetSelectionOrModelCenter() const;
	bool GetFocusPosition(lcVector3& Position) const;
	lcObject* GetFocusObject() const;
	bool GetSelectionCenter(lcVector3& Center) const;
	lcBoundingBox GetAllPiecesBoundingBox() const;
	bool GetVisiblePiecesBoundingBox(lcVector3& Min, lcVector3& Max) const;
	std::vector<lcVector3> GetPiecesBoundingBoxPoints() const;
	void GetPartsList(int DefaultColorIndex, bool ScanSubModels, bool AddSubModels, lcPartsList& PartsList) const;
	void GetPartsListForStep(lcStep Step, int DefaultColorIndex, lcPartsList& PartsList, bool Cumulative) const;
	void GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const;
	void GetSelectionInformation(int* Flags, std::vector<lcObject*>& Selection, lcObject** Focus) const;
	lcArray<lcObject*> GetSelectionModePieces(const lcPiece* SelectedPiece) const;

	void FocusOrDeselectObject(const lcObjectSection& ObjectSection);
	void ClearSelection(bool UpdateInterface);
	void ClearSelectionAndSetFocus(lcObject* Object, quint32 Section, bool EnableSelectionMode);
	void ClearSelectionAndSetFocus(const lcObjectSection& ObjectSection, bool EnableSelectionMode);
	void SetSelectionAndFocus(const lcArray<lcObject*>& Selection, lcObject* Focus, quint32 Section, bool EnableSelectionMode);
	void AddToSelection(const lcArray<lcObject*>& Objects, bool EnableSelectionMode, bool UpdateInterface);
	void RemoveFromSelection(const lcArray<lcObject*>& Objects);
	void RemoveFromSelection(const lcObjectSection& ObjectSection);
	void SelectAllPieces();
	void InvertSelection();

	void HideSelectedPieces();
	void HideUnselectedPieces();
	void UnhideSelectedPieces();
	void UnhideAllPieces();

	void FindReplacePiece(bool SearchForward, bool FindAll, bool Replace);

	void UndoAction();
	void RedoAction();

	lcVector3 SnapPosition(const lcVector3& Delta) const;
	lcVector3 SnapRotation(const lcVector3& Delta) const;
	lcMatrix33 GetRelativeRotation() const;

	const lcVector3& GetMouseToolDistance() const
	{
		return mMouseToolDistance;
	}

	void BeginMouseTool();
	void EndMouseTool(lcTool Tool, bool Accept);
	void InsertPieceToolClicked(const lcMatrix44& WorldMatrix);
	void InsertLightToolClicked(const lcVector3& Position, lcLightType LightType);
	void BeginCameraTool(const lcVector3& Position, const lcVector3& Target);
	void UpdateCameraTool(const lcVector3& Position);
	void UpdateMoveTool(const lcVector3& Distance, bool AllowRelative, bool AlternateButtonDrag);
	void UpdateRotateTool(const lcVector3& Angles, bool AlternateButtonDrag);
	void UpdateScaleTool(const float Scale);
	void EraserToolClicked(lcObject* Object);
	void PaintToolClicked(lcObject* Object);
	void ColorPickerToolClicked(const lcObject* Object);
	void UpdateZoomTool(lcCamera* Camera, float Mouse);
	void UpdatePanTool(lcCamera* Camera, const lcVector3& Distance);
	void UpdateOrbitTool(lcCamera* Camera, float MouseX, float MouseY);
	void UpdateRollTool(lcCamera* Camera, float Mouse);
	void ZoomRegionToolClicked(lcCamera* Camera, float AspectRatio, const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3* Corners);
	void LookAt(lcCamera* Camera);
	void MoveCamera(lcCamera* Camera, const lcVector3& Direction);
	void ZoomExtents(lcCamera* Camera, float Aspect);
	void Zoom(lcCamera* Camera, float Amount);

	void MoveSelectedObjects(const lcVector3& Distance, bool AllowRelative, bool AlternateButtonDrag, bool Update, bool Checkpoint, bool FirstMove)
	{
		MoveSelectedObjects(Distance, Distance, AllowRelative, AlternateButtonDrag, Update, Checkpoint, FirstMove);
	}

	void MoveSelectedObjects(const lcVector3& PieceDistance, const lcVector3& ObjectDistance, bool AllowRelative, bool AlternateButtonDrag, bool Update, bool Checkpoint, bool FirstMove);
	void RotateSelectedObjects(const lcVector3& Angles, bool Relative, bool RotatePivotPoint, bool Update, bool Checkpoint);
	void ScaleSelectedPieces(const float Scale, bool Update, bool Checkpoint);
	void TransformSelectedObjects(lcTransformType TransformType, const lcVector3& Transform);
	void SetObjectsKeyFrame(const std::vector<lcObject*>& Objects, lcObjectPropertyId PropertyId, bool KeyFrame);
	void SetSelectedPiecesColorIndex(int ColorIndex);
	void SetSelectedPiecesStepShow(lcStep Step);
	void SetSelectedPiecesStepHide(lcStep Step);

	void SetObjectsProperty(const std::vector<lcObject*>& Objects, lcObjectPropertyId PropertyId, QVariant Value);

	void SetCameraOrthographic(lcCamera* Camera, bool Ortho);
	void SetCameraFOV(lcCamera* Camera, float FOV);
	void SetCameraZNear(lcCamera* Camera, float ZNear);
	void SetCameraZFar(lcCamera* Camera, float ZFar);

	void ShowPropertiesDialog();
	void ShowSelectByNameDialog();
	void ShowArrayDialog();
	void ShowMinifigDialog();
	void UpdateInterface();

protected:
	void DeleteModel();
	void DeleteHistory();
	void SaveCheckpoint(const QString& Description);
	void LoadCheckPoint(lcModelHistoryEntry* CheckPoint);

	QString GetGroupName(const QString& Prefix);
	void RemoveEmptyGroups();
	bool RemoveSelectedObjects();

	void SelectGroup(lcGroup* TopGroup, bool Select);

	void AddPiece(lcPiece* Piece);
	void InsertPiece(lcPiece* Piece, int Index);

	lcPOVRayOptions mPOVRayOptions;
	lcModelProperties mProperties;
	Project* const mProject;
	PieceInfo* mPieceInfo;

	bool mIsPreview;
	bool mActive;
	lcStep mCurrentStep;
	lcVector3 mMouseToolDistance;
	bool mMouseToolFirstMove;

	lcArray<lcPiece*> mPieces;
	lcArray<lcCamera*> mCameras;
	lcArray<lcLight*> mLights;
	lcArray<lcGroup*> mGroups;
	QStringList mFileLines;

	lcModelHistoryEntry* mSavedHistory;
	std::vector<lcModelHistoryEntry*> mUndoHistory;
	std::vector<lcModelHistoryEntry*> mRedoHistory;

	Q_DECLARE_TR_FUNCTIONS(lcModel);
};

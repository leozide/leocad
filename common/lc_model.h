#pragma once

#include "lc_math.h"
#include "object.h"
#include "lc_commands.h"

#define LC_SEL_NO_PIECES                0x0001 // No pieces in model
#define LC_SEL_PIECE                    0x0002 // At last 1 piece selected
#define LC_SEL_SELECTED                 0x0004 // At last 1 object selected
#define LC_SEL_UNSELECTED               0x0008 // At least 1 piece unselected
#define LC_SEL_HIDDEN                   0x0010 // At least one piece hidden
#define LC_SEL_HIDDEN_SELECTED          0x0020 // At least one piece selected is hidden
#define LC_SEL_VISIBLE_SELECTED         0x0040 // At least one piece selected is not hidden
#define LC_SEL_GROUPED                  0x0080 // At least one piece selected is grouped
#define LC_SEL_FOCUS_GROUPED            0x0100 // Focused piece is grouped
#define LC_SEL_CAN_GROUP                0x0200 // Can make a new group
#define LC_SEL_MODEL_SELECTED           0x0400 // At least one model reference is selected
#define LC_SEL_CAN_ADD_CONTROL_POINT    0x0800 // Can add control points to focused piece
#define LC_SEL_CAN_REMOVE_CONTROL_POINT 0x1000 // Can remove control points from focused piece

class lcGLWidget;

enum class lcSelectionMode
{
	SINGLE,
	PIECE,
	COLOR,
	PIECE_COLOR
};

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
	LC_BACKGROUND_IMAGE
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

		if (mAmbientColor != Properties.mAmbientColor)
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

	lcVector3 mAmbientColor;
};

struct lcModelHistoryEntry
{
	QByteArray File;
	QString Description;
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

	bool GetPieceWorldMatrix(lcPiece* Piece, lcMatrix44& ParentWorldMatrix) const;
	bool IncludesModel(const lcModel* Model) const;
	void CreatePieceInfo(Project* Project);
	void UpdatePieceInfo(std::vector<lcModel*>& UpdatedModels);
	void UpdateMesh();

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

	const QString& GetName()
	{
		return mProperties.mName;
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

	void SaveLDraw(QTextStream& Stream, bool SelectedOnly) const;
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

		mSavedHistory = mUndoHistory[0];
	}

	void Cut();
	void Copy();
	void Paste();
	void DuplicateSelectedPieces();

	void GetScene(lcScene& Scene, lcCamera* ViewCamera, bool AllowHighlight, bool AllowFade) const;
	void AddSubModelRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const;
	void DrawBackground(lcGLWidget* Widget);
	QImage GetStepImage(bool Zoom, bool Highlight, int Width, int Height, lcStep Step);
	QImage GetPartsListImage(int MaxWidth, lcStep Step) const;
	void SaveStepImages(const QString& BaseName, bool AddStepSuffix, bool Zoom, bool Highlight, int Width, int Height, lcStep Start, lcStep End);
	std::vector<std::pair<lcModel*, lcStep>> GetPageLayouts(std::vector<const lcModel*>& AddedModels);

	void RayTest(lcObjectRayTest& ObjectRayTest) const;
	void BoxTest(lcObjectBoxTest& ObjectBoxTest) const;
	bool SubModelMinIntersectDist(const lcVector3& WorldStart, const lcVector3& WorldEnd, float& MinDistance) const;
	bool SubModelBoxTest(const lcVector4 Planes[6]) const;

	bool HasPieces() const
	{
		return !mPieces.IsEmpty();
	}

	bool AnyPiecesSelected() const;
	bool AnyObjectsSelected() const;
	lcModel* GetFirstSelectedSubmodel() const;
	void GetSubModels(lcArray<lcModel*>& SubModels) const;
	bool GetMoveRotateTransform(lcVector3& Center, lcMatrix33& RelativeRotation) const;
	bool GetPieceFocusOrSelectionCenter(lcVector3& Center) const;
	lcVector3 GetSelectionOrModelCenter() const;
	bool GetFocusPosition(lcVector3& Position) const;
	lcObject* GetFocusObject() const;
	bool GetSelectionCenter(lcVector3& Center) const;
	bool GetPiecesBoundingBox(lcVector3& Min, lcVector3& Max) const;
	void GetPartsList(int DefaultColorIndex, bool ScanSubModels, bool AddSubModels, lcPartsList& PartsList) const;
	void GetPartsListForStep(lcStep Step, int DefaultColorIndex, lcPartsList& PartsList) const;
	void GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const;
	void GetSelectionInformation(int* Flags, lcArray<lcObject*>& Selection, lcObject** Focus) const;
	lcArray<lcObject*> GetSelectionModePieces(lcPiece* SelectedPiece) const;

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

	void FindPiece(bool FindFirst, bool SearchForward);

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
	void PointLightToolClicked(const lcVector3& Position);
	void BeginSpotLightTool(const lcVector3& Position, const lcVector3& Target);
	void UpdateSpotLightTool(const lcVector3& Position);
	void BeginCameraTool(const lcVector3& Position, const lcVector3& Target);
	void UpdateCameraTool(const lcVector3& Position);
	void UpdateMoveTool(const lcVector3& Distance, bool AlternateButtonDrag);
	void UpdateRotateTool(const lcVector3& Angles, bool AlternateButtonDrag);
	void UpdateScaleTool(const float Scale);
	void EraserToolClicked(lcObject* Object);
	void PaintToolClicked(lcObject* Object);
    void PipetteToolClicked(lcObject* Object);
	void UpdateZoomTool(lcCamera* Camera, float Mouse);
	void UpdatePanTool(lcCamera* Camera, const lcVector3& Distance);
	void UpdateOrbitTool(lcCamera* Camera, float MouseX, float MouseY);
	void UpdateRollTool(lcCamera* Camera, float Mouse);
	void ZoomRegionToolClicked(lcCamera* Camera, float AspectRatio, const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3* Corners);
	void LookAt(lcCamera* Camera);
	void MoveCamera(lcCamera* Camera, const lcVector3& Direction);
	void ZoomExtents(lcCamera* Camera, float Aspect);
	void Zoom(lcCamera* Camera, float Amount);

	void MoveSelectedObjects(const lcVector3& Distance, bool Relative, bool AlternateButtonDrag, bool Update, bool Checkpoint)
	{
		MoveSelectedObjects(Distance, Distance, Relative, AlternateButtonDrag, Update, Checkpoint);
	}

	void MoveSelectedObjects(const lcVector3& PieceDistance, const lcVector3& ObjectDistance, bool Relative, bool AlternateButtonDrag, bool Update, bool Checkpoint);
	void RotateSelectedPieces(const lcVector3& Angles, bool Relative, bool AlternateButtonDrag, bool Update, bool Checkpoint);
	void ScaleSelectedPieces(const float Scale, bool Update, bool Checkpoint);
	void TransformSelectedObjects(lcTransformType TransformType, const lcVector3& Transform);
	void SetSelectedPiecesColorIndex(int ColorIndex);
	void SetSelectedPiecesPieceInfo(PieceInfo* Info);
	void SetSelectedPiecesStepShow(lcStep Step);
	void SetSelectedPiecesStepHide(lcStep Step);

	void SetCameraOrthographic(lcCamera* Camera, bool Ortho);
	void SetCameraFOV(lcCamera* Camera, float FOV);
	void SetCameraZNear(lcCamera* Camera, float ZNear);
	void SetCameraZFar(lcCamera* Camera, float ZFar);
	void SetCameraName(lcCamera* Camera, const char* Name);

	void ShowPropertiesDialog();
	void ShowSelectByNameDialog();
	void ShowSelectByColorDialog();
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

	void UpdateBackgroundTexture();

	void SelectGroup(lcGroup* TopGroup, bool Select);

	void AddPiece(lcPiece* Piece);
	void InsertPiece(lcPiece* Piece, int Index);

	lcModelProperties mProperties;
	PieceInfo* mPieceInfo;

	bool mActive;
	lcStep mCurrentStep;
	lcVector3 mMouseToolDistance;
	lcTexture* mBackgroundTexture;

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


#pragma once

enum class lcObjectType;
enum class lcSelectionMode;

class lcModelAction
{
public:
	lcModelAction() = default;
	virtual ~lcModelAction() = default;

protected:
	bool SaveHistoryBuffer(QByteArray& Buffer, const lcModel* Model);
	bool LoadHistoryBuffer(const QByteArray& Buffer, lcModel* Model, bool CreateObjects) const;
	
	std::vector<size_t> mGroupIndices;
	std::vector<size_t> mPieceIndices;
	std::vector<size_t> mCameraIndices;
	std::vector<size_t> mLightIndices;
};

enum class lcModelActionSelectionMode
{
	ClearSelection,
	ClearSelectionAndSetFocus,
	SetFocus,
	SetSelectionAndFocus,
	SelectAllPieces,
	InvertPieceSelection,
	AddToSelection,
	RemoveFromSelection,
	Set,
	Save,
	Restore
};

struct lcModelActionSelectionState
{
	std::vector<bool> SelectedPieces;
	std::vector<bool> SelectedCameras;
	std::vector<bool> SelectedLights;
	size_t FocusIndex = SIZE_MAX;
	uint32_t FocusSection = 0;
	lcObjectType FocusObjectType = static_cast<lcObjectType>(~0);
};

class lcModelActionSelection : public lcModelAction
{
public:
	lcModelActionSelection(lcModelActionSelectionMode Mode, lcStep Step);
	virtual ~lcModelActionSelection() = default;
	
	bool Initialize(const lcModel* Model, const std::vector<lcObject*>& Objects, lcObject* FocusObject, uint32_t FocusSection, lcSelectionMode SelectionMode);
	
	lcModelActionSelectionMode GetMode() const
	{
		return mMode;
	}
	
	lcStep GetStep() const
	{
		return mStep;
	}
	
	lcSelectionMode GetSelectionMode() const
	{
		return mSelectionMode;
	}
	
	uint32_t GetNewFocusSection() const
	{
		return mNewFocusSection;
	}
	
	std::vector<lcObject*> GetNewObjects(const lcModel* Model) const;
	lcObject* GetNewFocusObject(const lcModel* Model) const;
	std::tuple<std::vector<lcObject*>, lcObject*, uint32_t> GetPreviousSelection(const lcModel* Model) const;

protected:
	static void SaveState(lcModelActionSelectionState& State, const lcModel* Model);
	static size_t GetObjectIndex(const lcObject* Object, const lcModel* Model);
	bool HasChanges(const lcModel* Model, const std::vector<lcObject*>& Objects, lcObject* FocusObject, uint32_t FocusSection) const;
	void SaveNewObjects(const std::vector<lcObject*>& Objects, const lcModel* Model);
	void SaveNewFocusObject(const lcModel* Model, lcObject* FocusObject, uint32_t FocusSection);
	
	lcModelActionSelectionState mStartState;
	
	std::vector<std::pair<size_t, lcObjectType>> mNewObjects;
	size_t mNewFocusIndex = SIZE_MAX;
	uint32_t mNewFocusSection = 0;
	lcObjectType mNewFocusObjectType = (lcObjectType)0;
	lcSelectionMode mSelectionMode = (lcSelectionMode)0;

	lcModelActionSelectionMode mMode;
	lcStep mStep;
};

enum class lcModelActionObjectEditMode
{
	EditAllObjects,
	EditAllPieces,
	EditSelectedObjects,
	EditSelectedPieces,
	EditUnselectedPieces,
	EditCamera,
	CreatePieces,
	CreateCamera,
	CreateLight
};

class lcModelActionObjectEdit: public lcModelAction
{
public:
	lcModelActionObjectEdit(lcModelActionObjectEditMode Mode);
	virtual ~lcModelActionObjectEdit() = default;
	
	bool SaveStartState(const lcModel* Model, const lcCamera* Camera);
	bool SaveEndState(const lcModel* Model, std::vector<size_t>&& ObjectIndices, std::vector<size_t>&& GroupIndices);
	void LoadStartState(lcModel* Model) const;
	void LoadEndState(lcModel* Model) const;

	lcModelActionObjectEditMode GetMode() const
	{
		return mMode;
	}

protected:
	lcModelActionObjectEditMode mMode;
	QByteArray mStartBuffer;
	QByteArray mEndBuffer;
};

enum class lcModelActionGroupPiecesMode
{
	Group,
	Ungroup
};

class lcModelActionGroupPieces : public lcModelAction
{
public:
	lcModelActionGroupPieces(lcModelActionGroupPiecesMode Mode, const QString& GroupName);
	virtual ~lcModelActionGroupPieces() = default;

	lcModelActionGroupPiecesMode GetMode() const
	{
		return mMode;
	}

	const QString& GetGroupName() const
	{
		return mGroupName;
	}

protected:
	lcModelActionGroupPiecesMode mMode;
	QString mGroupName;
};

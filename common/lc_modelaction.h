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

struct lcModelActionSelectionState
{
	std::vector<bool> PieceSelection;
	std::vector<bool> CameraSelection;
	std::vector<bool> LightSelection;
	size_t FocusIndex = SIZE_MAX;
	uint32_t FocusSection = 0;
	lcObjectType FocusObjectType = static_cast<lcObjectType>(~0);

	bool operator!=(const lcModelActionSelectionState& Other) const
	{
		return PieceSelection != Other.PieceSelection || CameraSelection != Other.CameraSelection || LightSelection != Other.LightSelection ||
			FocusIndex != Other.FocusIndex || FocusSection != Other.FocusSection || FocusObjectType != Other.FocusObjectType;
	}
};

class lcModelActionSelection : public lcModelAction
{
public:
	lcModelActionSelection() = default;
	virtual ~lcModelActionSelection() = default;
	
	void SaveStartState(const lcModel* Model);
	void SaveEndState(const lcModel* Model);
	void LoadStartState(lcModel* Model) const;
	void LoadEndState(lcModel* Model) const;

	bool StateChanged() const;

protected:
	static void SaveState(lcModelActionSelectionState& State, const lcModel* Model);
	static void LoadState(const lcModelActionSelectionState& State, lcModel* Model);
	
	lcModelActionSelectionState mStartState;
	lcModelActionSelectionState mEndState;
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

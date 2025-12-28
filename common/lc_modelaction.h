#pragma once

enum class lcObjectType;

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
	Clear,
	Set,
	Save,
	Restore
};

class lcModelActionSelection : public lcModelAction
{
public:
	lcModelActionSelection(lcModelActionSelectionMode Mode);
	virtual ~lcModelActionSelection() = default;

	lcModelActionSelectionMode GetMode() const
	{
		return mMode;
	}

	void SetSelection(const std::vector<std::unique_ptr<lcPiece>>& Pieces, const std::vector<std::unique_ptr<lcCamera>>& Cameras, const std::vector<std::unique_ptr<lcLight>>& Lights);
	std::tuple<std::vector<lcObject*>, lcObject*, uint32_t> GetSelection(const std::vector<std::unique_ptr<lcPiece>>& Pieces, const std::vector<std::unique_ptr<lcCamera>>& Cameras, const std::vector<std::unique_ptr<lcLight>>& Lights) const;

protected:
	std::vector<size_t> mSelectedPieces;
	std::vector<size_t> mSelectedCameras;
	std::vector<size_t> mSelectedLights;
	size_t mFocusIndex = SIZE_MAX;
	uint32_t mFocusSection = 0;
	lcObjectType mFocusType = (lcObjectType)0;
	lcModelActionSelectionMode mMode = lcModelActionSelectionMode::Clear;
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
	void SaveState(QByteArray& Buffer);
	
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

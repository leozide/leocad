#pragma once

#include "lc_model.h"

enum class lcObjectType;
enum class lcSelectionMode;

class lcModelHistory
{
public:
	lcModelHistory() = default;
	virtual ~lcModelHistory() = default;

	virtual void SaveStartState(const lcModel* Model) = 0;
	virtual void SaveEndState(const lcModel* Model) = 0;
	virtual void LoadStartState(lcModel* Model) const = 0;
	virtual void LoadEndState(lcModel* Model) const = 0;
	virtual bool StateChanged() const = 0;

	virtual bool CanMergeWith(const lcModelHistory* Other) const
	{
		Q_UNUSED(Other);

		return false;
	}

	virtual void MergeWith(lcModelHistory* Other)
	{
		Q_UNUSED(Other);
	}
};

struct lcModelHistorySelectState
{
	std::vector<bool> PieceSelection;
	std::vector<bool> CameraSelection;
	std::vector<bool> LightSelection;
	size_t FocusIndex = SIZE_MAX;
	uint32_t FocusSection = ~0U;
	lcObjectType FocusObjectType = static_cast<lcObjectType>(~0);

	bool operator!=(const lcModelHistorySelectState& Other) const
	{
		return PieceSelection != Other.PieceSelection || CameraSelection != Other.CameraSelection || LightSelection != Other.LightSelection ||
			FocusIndex != Other.FocusIndex || FocusSection != Other.FocusSection || FocusObjectType != Other.FocusObjectType;
	}
};

class lcModelHistorySelect : public lcModelHistory
{
public:
	lcModelHistorySelect() = default;
	virtual ~lcModelHistorySelect() = default;

	void SaveStartState(const lcModel* Model) override;
	void SaveEndState(const lcModel* Model) override;
	void LoadStartState(lcModel* Model) const override;
	void LoadEndState(lcModel* Model) const override;
	bool StateChanged() const override;

protected:
	static void SaveState(lcModelHistorySelectState& State, const lcModel* Model);
	static void LoadState(const lcModelHistorySelectState& State, lcModel* Model);

	lcModelHistorySelectState mStartState;
	lcModelHistorySelectState mEndState;
};

struct lcGroupHistoryState;
struct lcPieceHistoryState;
struct lcCameraHistoryState;
struct lcLightHistoryState;

struct lcModelHistoryEditState
{
	std::vector<lcGroupHistoryState> Groups;
	std::vector<lcPieceHistoryState> Pieces;
	std::vector<lcCameraHistoryState> Cameras;
	std::vector<lcLightHistoryState> Lights;
};

bool operator!=(const lcModelHistoryEditState& a, const lcModelHistoryEditState& b);

enum class lcModelHistoryEditMerge
{
	None,
	KeyboardMove,
	KeyboardRotate,
	KeyboardZoom,
	KeyboardMoveCamera,
	PropertiesMove,
	PropertiesRotate,
	PropertiesEdit = 0x40000000
};

class lcModelHistoryEdit : public lcModelHistory
{
public:
	lcModelHistoryEdit(lcModelHistoryEditMerge ModelHistoryEditMerge);
	virtual ~lcModelHistoryEdit();

	void SaveStartState(const lcModel* Model) override;
	void SaveEndState(const lcModel* Model) override;
	void LoadStartState(lcModel* Model) const override;
	void LoadEndState(lcModel* Model) const override;
	bool StateChanged() const override;

	bool CanMergeWith(const lcModelHistory* Other) const override;
	void MergeWith(lcModelHistory* Other) override;

protected:
	static void SaveState(lcModelHistoryEditState& State, const lcModel* Model);
	static void LoadState(const lcModelHistoryEditState& State, lcModel* Model);

	lcModelHistoryEditState mStartState;
	lcModelHistoryEditState mEndState;
	lcModelHistoryEditMerge mMerge;
};

class lcModelHistoryProperties : public lcModelHistory
{
public:
	lcModelHistoryProperties() = default;
	virtual ~lcModelHistoryProperties() = default;

	void SaveStartState(const lcModel* Model) override;
	void SaveEndState(const lcModel* Model) override;
	void LoadStartState(lcModel* Model) const override;
	void LoadEndState(lcModel* Model) const override;
	bool StateChanged() const override;

protected:
	static void SaveState(lcModelProperties& State, const lcModel* Model);
	static void LoadState(const lcModelProperties& State, lcModel* Model);

	lcModelProperties mStartState;
	lcModelProperties mEndState;
};

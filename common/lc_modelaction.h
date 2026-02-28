#pragma once

#include "lc_model.h"

enum class lcObjectType;
enum class lcSelectionMode;

class lcModelAction
{
public:
	lcModelAction() = default;
	virtual ~lcModelAction() = default;
	
	virtual void SaveStartState(const lcModel* Model) = 0;
	virtual void SaveEndState(const lcModel* Model) = 0;
	virtual void LoadStartState(lcModel* Model) const = 0;
	virtual void LoadEndState(lcModel* Model) const = 0;
};

struct lcModelActionSelectionState
{
	std::vector<bool> PieceSelection;
	std::vector<bool> CameraSelection;
	std::vector<bool> LightSelection;
	size_t FocusIndex = SIZE_MAX;
	uint32_t FocusSection = ~0U;
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
	
	void SaveStartState(const lcModel* Model) override;
	void SaveEndState(const lcModel* Model) override;
	void LoadStartState(lcModel* Model) const override;
	void LoadEndState(lcModel* Model) const override;

	bool StateChanged() const;

protected:
	static void SaveState(lcModelActionSelectionState& State, const lcModel* Model);
	static void LoadState(const lcModelActionSelectionState& State, lcModel* Model);
	
	lcModelActionSelectionState mStartState;
	lcModelActionSelectionState mEndState;
};

struct lcGroupHistoryState;
struct lcPieceHistoryState;
struct lcCameraHistoryState;
struct lcLightHistoryState;

struct lcModelHistoryState
{
	std::vector<lcGroupHistoryState> Groups;
	std::vector<lcPieceHistoryState> Pieces;
	std::vector<lcCameraHistoryState> Cameras;
	std::vector<lcLightHistoryState> Lights;
	
	bool operator!=(const lcModelHistoryState& Other) const
	{
		return Groups != Other.Groups || Pieces != Other.Pieces || Cameras != Other.Cameras || Lights != Other.Lights;
	}
};

class lcModelActionObjectEdit: public lcModelAction
{
public:
	lcModelActionObjectEdit() = default;
	virtual ~lcModelActionObjectEdit() = default;
	
	void SaveStartState(const lcModel* Model) override;
	void SaveEndState(const lcModel* Model) override;
	void LoadStartState(lcModel* Model) const override;
	void LoadEndState(lcModel* Model) const override;
	
	bool StateChanged() const;

protected:
	static void SaveState(lcModelHistoryState& State, const lcModel* Model);
	static void LoadState(const lcModelHistoryState& State, lcModel* Model);
	
	lcModelHistoryState mStartState;
	lcModelHistoryState mEndState;
};

class lcModelActionProperties : public lcModelAction
{
public:
	lcModelActionProperties() = default;
	virtual ~lcModelActionProperties() = default;

	void SaveStartState(const lcModel* Model) override;
	void SaveEndState(const lcModel* Model) override;
	void LoadStartState(lcModel* Model) const override;
	void LoadEndState(lcModel* Model) const override;

	bool StateChanged() const;

protected:
	static void SaveState(lcModelProperties& State, const lcModel* Model);
	static void LoadState(const lcModelProperties& State, lcModel* Model);

	lcModelProperties mStartState;
	lcModelProperties mEndState;
};

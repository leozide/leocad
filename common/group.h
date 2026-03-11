#pragma once

#define LC_MAX_GROUP_NAME 64

enum class lcGroupId : uint32_t;

struct lcGroupHistoryState
{
	lcGroupId Id;
	uint32_t ParentIndex;
	QString Name;

	bool operator==(const lcGroupHistoryState& Other) const
	{
		return Id == Other.Id && ParentIndex == Other.ParentIndex && Name == Other.Name;
	}
};

class lcGroup
{
public:
	lcGroup();

	lcGroup* GetTopGroup()
	{
		return mGroup ? mGroup->GetTopGroup() : this;
	}

	lcGroupId GetId() const
	{
		return mId;
	}

	void FileLoad(lcFile* File);
	void CreateName(const std::vector<std::unique_ptr<lcGroup>>& Groups);

	lcGroupHistoryState GetHistoryState(const lcModel* Model) const;
	void SetHistoryState(const lcGroupHistoryState& State, const lcModel* Model);

	lcGroup* mGroup = nullptr;
	QString mName;

protected:
	lcGroupId mId = static_cast<lcGroupId>(0);

	static lcGroupId mNextId;
};

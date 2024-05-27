#pragma once

#define LC_MAX_GROUP_NAME 64

class lcGroup
{
public:
	lcGroup();

	lcGroup* GetTopGroup()
	{
		return mGroup ? mGroup->GetTopGroup() : this;
	}

	void FileLoad(lcFile* File);
	void CreateName(const std::vector<std::unique_ptr<lcGroup>>& Groups);

	lcGroup* mGroup;
	QString mName;
};


#pragma once

#include "lc_array.h"

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
	void CreateName(const lcArray<lcGroup*>& Groups);

	lcGroup* mGroup;
	QString mName;
};


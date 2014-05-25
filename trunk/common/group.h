#ifndef _GROUP_H_
#define _GROUP_H_

#include "lc_array.h"

#define LC_MAX_GROUP_NAME 64

class lcGroup
{
public:
	lcGroup();
	~lcGroup();

	lcGroup* GetTopGroup()
	{
		return mGroup ? mGroup->GetTopGroup() : this;
	}

	void FileLoad(lcFile* File);
	void FileSave(lcFile* File, const lcArray<lcGroup*>& Groups);

	lcGroup* mGroup;
	char m_strName[LC_MAX_GROUP_NAME + 1];
};

#endif // _GROUP_H_

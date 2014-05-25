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

	void SetGroup(lcGroup* Group);
	void UnGroup(lcGroup* Group);

	lcGroup* mGroup;

	void FileLoad(lcFile* file);
	void FileSave(lcFile* file, const lcArray<lcGroup*>& Groups);

	char m_strName[LC_MAX_GROUP_NAME + 1];
	float m_fCenter[3];
};

#endif // _GROUP_H_

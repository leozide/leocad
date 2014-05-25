#include "lc_global.h"
#include <stdlib.h>
#include "group.h"
#include "lc_file.h"

lcGroup::lcGroup()
{
	mGroup = NULL;
}

lcGroup::~lcGroup()
{

}

void lcGroup::SetGroup(lcGroup* Group)
{
	if (Group == this)
		return;

	if (mGroup != NULL && mGroup != (lcGroup*)-1)
		mGroup->SetGroup(Group);
	else
		mGroup = Group;
}

void lcGroup::UnGroup(lcGroup* Group)
{
	if (mGroup == Group)
		mGroup = NULL;
	else if (mGroup != NULL)
		mGroup->UnGroup(Group);
}

void lcGroup::FileLoad(lcFile* file)
{
	lcuint8 version;
	lcint32 i;

	file->ReadU8(&version, 1);
	file->ReadBuffer(m_strName, 65);
	file->ReadFloats(m_fCenter, 3);
	file->ReadS32(&i, 1);
	mGroup = (lcGroup*)(long)i;
}

void lcGroup::FileSave(lcFile* file, const lcArray<lcGroup*>& Groups)
{
	lcuint8 version = 1; // LeoCAD 0.60

	file->WriteU8(&version, 1);
	file->WriteBuffer(m_strName, 65);
	file->WriteFloats(m_fCenter, 3);

	lcint32 GroupIndex = Groups.FindIndex(mGroup);
	file->WriteS32(&GroupIndex, 1);
}

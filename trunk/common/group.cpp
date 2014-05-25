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

void lcGroup::FileLoad(lcFile* File)
{
	lcuint8 Version;
	lcint32 GroupIndex;

	Version = File->ReadU8();
	File->ReadBuffer(m_strName, 65);
	File->ReadVector3();
	File->ReadS32(&GroupIndex, 1);
	mGroup = (lcGroup*)(long)GroupIndex;
}

void lcGroup::FileSave(lcFile* File, const lcArray<lcGroup*>& Groups)
{
	lcuint8 Version = 1; // LeoCAD 0.60

	File->WriteU8(Version);
	File->WriteBuffer(m_strName, 65);
	File->WriteVector3(lcVector3(0.0f, 0.0f, 0.0f));

	lcint32 GroupIndex = Groups.FindIndex(mGroup);
	File->WriteS32(&GroupIndex, 1);
}

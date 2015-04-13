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
	lcint32 GroupIndex;

	File->ReadU8();
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

void lcGroup::CreateName(const lcArray<lcGroup*>& Groups)
{
	if (m_strName[0])
	{
		bool Found = false;
		for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
		{
			if (!strcmp(Groups[GroupIdx]->m_strName, m_strName))
			{
				Found = true;
				break;
			}
		}

		if (!Found)
			return;
	}

	int i, max = 0;
	const char* Prefix = "Group ";

	for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
		if (strncmp(Groups[GroupIdx]->m_strName, Prefix, strlen(Prefix)) == 0)
			if (sscanf(Groups[GroupIdx]->m_strName + strlen(Prefix), " %d", &i) == 1)
				if (i > max)
					max = i;

	sprintf(m_strName, "%s %d", Prefix, max+1);
}

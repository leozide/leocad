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
	char Name[LC_MAX_GROUP_NAME + 1];

	File->ReadU8();
	File->ReadBuffer(Name, sizeof(Name));
	mName = QString::fromUtf8(Name);
	File->ReadVector3();
	File->ReadS32(&GroupIndex, 1);
	mGroup = (lcGroup*)(quintptr)GroupIndex;
}

void lcGroup::CreateName(const lcArray<lcGroup*>& Groups)
{
	if (!mName.isEmpty())
	{
		bool Found = false;
		for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
		{
			if (Groups[GroupIdx]->mName == mName)
			{
				Found = true;
				break;
			}
		}

		if (!Found)
			return;
	}

	int Max = 0;
	QString Prefix = QApplication::tr("Group #");
	int Length = Prefix.length();

	for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
	{
		const QString& Name = Groups[GroupIdx]->mName;

		if (Name.startsWith(Prefix))
		{
			bool Ok = false;
			int GroupNumber = Name.mid(Length).toInt(&Ok);
			if (Ok && GroupNumber > Max)
				Max = GroupNumber;
		}
	}

	mName = Prefix + QString::number(Max + 1);
}

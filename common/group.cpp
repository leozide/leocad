#include "lc_global.h"
#include <stdlib.h>
#include "group.h"
#include "lc_file.h"

lcGroup::lcGroup()
{
	mGroup = nullptr;
}

void lcGroup::FileLoad(lcFile* File)
{
	qint32 GroupIndex;
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
		for (lcGroup* Group : Groups)
		{
			if (Group->mName == mName)
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

	for (lcGroup* Group : Groups)
	{
		const QString& Name = Group->mName;

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

#include "lc_global.h"
#include <stdlib.h>
#include "group.h"
#include "lc_model.h"
#include "lc_file.h"

lcGroupId lcGroup::mNextId;

lcGroup::lcGroup()
    : mId(mNextId)
{
	mNextId = static_cast<lcGroupId>(static_cast<uint32_t>(mNextId) + 1);
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

void lcGroup::CreateName(const std::vector<std::unique_ptr<lcGroup>>& Groups)
{
	if (!mName.isEmpty())
	{
		bool Found = false;
		for (const std::unique_ptr<lcGroup>& Group : Groups)
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
	const qsizetype Length = Prefix.length();

	for (const std::unique_ptr<lcGroup>& Group : Groups)
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

lcGroupHistoryState lcGroup::GetHistoryState(const lcModel* Model) const
{
	lcGroupHistoryState State;

	State.Id = mId;
	State.ParentIndex = ~0U;
	State.Name = mName;

	if (mGroup)
	{
		const std::vector<std::unique_ptr<lcGroup>>& Groups = Model->GetGroups();

		for (size_t GroupIndex = 0; GroupIndex < Groups.size(); GroupIndex++)
		{
			if (Groups[GroupIndex].get() == mGroup)
			{
				State.ParentIndex = static_cast<uint32_t>(GroupIndex);
				break;
			}
		}
	}

	return State;
}

void lcGroup::SetHistoryState(const lcGroupHistoryState& State, const lcModel* Model)
{
	const std::vector<std::unique_ptr<lcGroup>>& Groups = Model->GetGroups();

	mId = State.Id;
	mName = State.Name;
	mGroup = (State.ParentIndex < Groups.size()) ? Groups[State.ParentIndex].get() : nullptr;
}

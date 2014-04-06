// Piece group
//

#include "lc_global.h"
#include <stdlib.h>
#include "group.h"
#include "lc_file.h"

/////////////////////////////////////////////////////////////////////////////
// Group construction/destruction

Group::Group()
{
	m_pGroup = NULL;
	m_pNext = NULL;
}

Group::~Group()
{

}

Group* Group::GetTopGroup()
{
	return m_pGroup ? m_pGroup->GetTopGroup() : this;
}

void Group::SetGroup(Group* pGroup)
{
	if (pGroup == this)
		return;

	if (m_pGroup != NULL && m_pGroup != (Group*)-1)
		m_pGroup->SetGroup(pGroup);
	else
		m_pGroup = pGroup;
}

void Group::UnGroup(Group* pGroup)
{
	if (m_pGroup == pGroup)
		m_pGroup = NULL;
	else
		if (m_pGroup != NULL)
			m_pGroup->UnGroup(pGroup);
}

void Group::FileLoad(lcFile* file)
{
	lcuint8 version;
	lcint32 i;

	file->ReadU8(&version, 1);
	file->ReadBuffer(m_strName, 65);
	file->ReadFloats(m_fCenter, 3);
	file->ReadS32(&i, 1);
	m_pGroup = (Group*)(long)i;
}

void Group::FileSave(lcFile* file, Group* pGroups)
{
	lcuint8 version = 1; // LeoCAD 0.60
	lcint32 i = 0;

	file->WriteU8(&version, 1);
	file->WriteBuffer(m_strName, 65);
	file->WriteFloats(m_fCenter, 3);

	if (m_pGroup == NULL)
		i = -1;
	else
	{
		for (; pGroups; pGroups = pGroups->m_pNext)
			if (pGroups == m_pGroup)
				break;
			else
				i++;
	}

	file->WriteS32(&i, 1);
}

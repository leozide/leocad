// Piece group
//

#include "lc_global.h"
#include <stdlib.h>
#include "group.h"
#include "file.h"

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

void Group::FileLoad(File* file)
{
	unsigned char version;
	int i;

	file->Read(&version, 1);
	file->Read(m_strName, 65);
	file->Read(m_fCenter, 12);
	file->ReadLong(&i, 1);
	m_pGroup = (Group*)i;
}

void Group::FileSave(File* file, Group* pGroups)
{
	unsigned char version = 1; // LeoCAD 0.60

	file->Write(&version, 1);
	file->Write(m_strName, 65);
	file->Write(m_fCenter, 12);

	int i = 0;
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
	file->WriteLong(&i, 1);
}

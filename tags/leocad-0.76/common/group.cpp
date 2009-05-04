// Piece group
//

#include "lc_global.h"
#include <stdlib.h>
#include "group.h"
#include "file.h"

/////////////////////////////////////////////////////////////////////////////
// Group construction/destruction

lcGroup::lcGroup()
{
	m_Group = NULL;
	m_Next = NULL;
}

lcGroup::~lcGroup()
{

}

lcGroup* lcGroup::GetTopGroup()
{
	return m_Group ? m_Group->GetTopGroup() : this;
}

void lcGroup::SetGroup(lcGroup* Group)
{
	if (Group == this)
		return;

	if (m_Group != NULL && m_Group != (lcGroup*)-1)
		m_Group->SetGroup(Group);
	else
		m_Group = Group;
}

void lcGroup::UnGroup(lcGroup* Group)
{
	if (m_Group == Group)
		m_Group = NULL;
	else
		if (m_Group != NULL)
			m_Group->UnGroup(Group);
}

void lcGroup::FileLoad(File* file)
{
	unsigned char version;
	int i;

	file->Read(&version, 1);
	file->Read(m_strName, 65);
	file->Read(m_fCenter, 12);
	file->ReadLong(&i, 1);
	m_Group = (lcGroup*)i;
}

void lcGroup::FileSave(File* file, lcGroup* Groups)
{
	unsigned char version = 1; // LeoCAD 0.60

	file->Write(&version, 1);
	file->Write(m_strName, 65);
	file->Write(m_fCenter, 12);

	int i = 0;
	if (m_Group == NULL)
		i = -1;
	else
	{
		for (; Groups; Groups = Groups->m_Next)
			if (Groups == m_Group)
				break;
			else
				i++;
	}
	file->WriteLong(&i, 1);
}

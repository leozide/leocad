#ifndef _GROUP_H
#define _GROUP_H

class lcFile;

class lcGroup
{
public:
	lcGroup();
	~lcGroup();

	void SetGroup(lcGroup* Group);
	void UnGroup(lcGroup* Group);
	lcGroup* GetTopGroup();

	lcGroup* m_Next;
	lcGroup* m_Group;

	void FileLoad(lcFile* file);
	void FileSave(lcFile* file, lcGroup* Groups);

	char m_strName[65];
	float m_fCenter[3];
};

#endif // _GROUP_H

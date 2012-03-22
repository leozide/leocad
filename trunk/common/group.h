#ifndef _GROUP_H_
#define _GROUP_H_

class Group
{
public:
//	void DoSaveLoad(CArchive& ar, CCADDoc* pDoc);
	Group();
	~Group();

	void SetGroup(Group* pGroup);
	void UnGroup(Group* pGroup);
	Group* GetTopGroup();

	Group* m_pNext;
	Group* m_pGroup;

	void FileLoad(lcFile* file);
	void FileSave(lcFile* file, Group* pGroups);

	char m_strName[65];
	float m_fCenter[3];
};

#endif // _GROUP_H_

#ifndef _GROUP_H_
#define _GROUP_H_

#define LC_MAX_GROUP_NAME 64

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

	char m_strName[LC_MAX_GROUP_NAME + 1];
	float m_fCenter[3];
};

#endif // _GROUP_H_

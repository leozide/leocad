#ifndef _PIECE_H_
#define _PIECE_H_

class Piece;
class Group;
class PieceInfo;

#include "object.h"
#include "globals.h"
#include "typedefs.h"

#define LC_PIECE_HIDDEN		0x01
#define LC_PIECE_SELECTED	0x02
#define LC_PIECE_FOCUSED	0x04

typedef enum
{
	LC_PK_POSITION,
	LC_PK_ROTATION,
	LC_PK_COUNT
} LC_PK_TYPES;

class Piece : public Object
{
public:
	Piece (PieceInfo* pPieceInfo);
	~Piece ();

	void Select (bool bSelecting, bool bFocus, bool bMultiple);
	virtual void InsertTime (unsigned short start, bool animation, unsigned short time);
	virtual void RemoveTime (unsigned short start, bool animation, unsigned short time);
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes);





	Piece* m_pNext;
	Piece* m_pLink;

	void Hide()
		{ m_nState = LC_PIECE_HIDDEN; }
	void UnHide()
		{ m_nState &= ~LC_PIECE_HIDDEN; }
	bool IsHidden()
		{ return (m_nState & LC_PIECE_HIDDEN) != 0; }
	bool IsSelected()
		{ return (m_nState & LC_PIECE_SELECTED) != 0; }
	bool IsFocused()
		{ return (m_nState & LC_PIECE_FOCUSED) != 0; }

	const char* GetName() const
	{ return m_strName; };

	void MinIntersectDist(LC_CLICKLINE* pLine);
	bool IsVisible(unsigned short nTime, bool bAnimation);
	void Initialize(float x, float y, float z, unsigned char nStep, unsigned short nFrame, unsigned char nColor);
	void CreateName(Piece* pPiece);
	void AddConnections(CONNECTION_TYPE* pConnections);
	void RemoveConnections(CONNECTION_TYPE* pConnections);
	void CompareBoundingBox(float box[6]);
	void SetPieceInfo(PieceInfo* pPieceInfo);
	bool FileLoad(lcFile& file, char* name);
	void FileSave(lcFile& file, Group* pGroups);

	void CalculateConnections(CONNECTION_TYPE* pConnections, unsigned short nTime, bool bAnimation, bool bForceRebuild, bool bFixOthers);
	void UpdatePosition(unsigned short nTime, bool bAnimation);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz);

	void DoGroup(Group* pGroup);
	void UnGroup(Group* pGroup);
	Group* GetTopGroup();
	void SetGroup(Group* pGroup)
		{ m_pGroup = pGroup; }
	Group* GetGroup()
		{ return m_pGroup; }
	void SetName(char* name)
		{ strcpy(m_strName, name); }
	const char* GetName()
		{ return m_strName; }
	const unsigned char GetColor()
		{ return m_nColor; }
	void SetColor(unsigned char color)
		{ m_nColor = color; }
	PieceInfo* GetPieceInfo()
		{ return m_pPieceInfo; }
	void SetStepShow(unsigned char step)
		{ m_nStepShow = step; }
	const unsigned char GetStepShow()
		{ return m_nStepShow; }
	void SetStepHide(unsigned char step)
		{ m_nStepHide = step; }
	const unsigned char GetStepHide()
		{ return m_nStepHide; }
	void SetFrameShow(unsigned short frame)
		{ m_nFrameShow = frame; }
	const unsigned short GetFrameShow()
		{ return m_nFrameShow; }
	void SetFrameHide(unsigned short frame)
		{ m_nFrameHide = frame; }
	const unsigned short GetFrameHide()
		{ return m_nFrameHide; }
	const float* GetConstPosition()
		{ return m_fPosition; }
	inline Vector3 GetPosition() const
		{ return Vector3(m_fPosition[0], m_fPosition[1], m_fPosition[2]); }
	void GetPosition (float* position)
		{ memcpy(position, m_fPosition, sizeof(m_fPosition)); }
	void GetRotation (float* rotation)
		{ memcpy(rotation, m_fRotation, sizeof(m_fRotation)); }

	void Render(bool bLighting, bool bEdges);
	void RenderBox(bool bHilite, float fLineWidth);

	inline bool IsTransparent()
	{
		if (m_nColor < 14) return false;
		if (m_nColor > 21) return false;
		return true;
	};

/*
	inline void UseTransform()
	{
		glTranslatef(m_fPosition[0], m_fPosition[1], m_fPosition[2]);
		glRotatef(m_fRotation[3], m_fRotation[0], m_fRotation[1], m_fRotation[2]);
	}
*/
protected:
	void BuildDrawInfo();

	// Atributes
	PieceInfo* m_pPieceInfo;
	Group* m_pGroup;

	unsigned short m_nFrameShow;
	unsigned short m_nFrameHide;
	unsigned char m_nStepShow;
	unsigned char m_nStepHide;

	unsigned char m_nColor;
	unsigned char m_nState;
	char m_strName[81];

	// Temporary variables
	float m_fPosition[3];
	float m_fRotation[4];
	CONNECTION* m_pConnections;
	void* m_pDrawInfo;
};


#endif // _PIECE_H

//
//	piece.h
////////////////////////////////////////////////////

#ifndef _PIECE_H_
#define _PIECE_H_

class File;
class Piece;
class Group;
class PieceInfo;

#include "boundbox.h"
#include "globals.h"
#include "typedefs.h"
#include "defines.h"

#define LC_PIECE_HIDDEN		0x01
#define LC_PIECE_SELECTED	0x02
#define LC_PIECE_FOCUSED	0x04

typedef enum {	PK_POSITION, PK_ROTATION } PK_TYPES;

typedef struct PIECE_KEY {
	unsigned short	time;
	float			param[4];
	unsigned char	type;
	PIECE_KEY*		next;
} PIECE_KEY;

class Piece
{
public:
	Piece(PieceInfo* pPieceInfo);
	~Piece();

	Piece* m_pNext;
	Piece* m_pLink;

	void Hide()
		{ m_nState = LC_PIECE_HIDDEN; }
	void UnHide()
		{ m_nState &= ~LC_PIECE_HIDDEN; }
	bool IsHidden()
		{ return (m_nState & LC_PIECE_HIDDEN) != 0; }
	void Select()
		{ m_nState |= LC_PIECE_SELECTED; } 
	void UnSelect()
		{ m_nState &= ~(LC_PIECE_SELECTED|LC_PIECE_FOCUSED); } 
	bool IsSelected()
		{ return (m_nState & LC_PIECE_SELECTED) != 0; }
	void Focus()
		{ m_nState |= LC_PIECE_FOCUSED|LC_PIECE_SELECTED; } 
	void UnFocus()
		{ m_nState &= ~LC_PIECE_FOCUSED; } 
	bool IsFocused()
		{ return (m_nState & LC_PIECE_FOCUSED) != 0; }

	void MinIntersectDist(CLICKLINE* pLine);
	bool IsVisible(unsigned short nTime, bool bAnimation);
	void Initialize(float x, float y, float z, unsigned char nStep, unsigned short nFrame, unsigned char nColor);
	void CreateName(Piece* pPiece);
	void AddConnections(CONNECTION_TYPE* pConnections);
	void RemoveConnections(CONNECTION_TYPE* pConnections);
	void CompareBoundingBox(float box[6]);
	void SetPieceInfo(PieceInfo* pPieceInfo);
	void FileLoad(File* file, char* name);
	void FileSave(File* file, Group* pGroups);

	void CalculateConnections(CONNECTION_TYPE* pConnections, unsigned short nTime, bool bAnimation, bool bForceRebuild, bool bFixOthers);
	void UpdatePosition(unsigned short nTime, bool bAnimation);
	bool CalculatePositionRotation(unsigned short nTime, bool bAnimation, float pos[3], float rot[4]);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z);
	void ChangeKey(unsigned short nTime, bool bAnimation, bool bAddKey, float* param, unsigned char nKeyType);
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
	void GetPosition (float* position)
		{ memcpy(position, m_fPosition, sizeof(m_fPosition)); }
	void GetRotation (float* rotation)
		{ memcpy(rotation, m_fRotation, sizeof(m_fRotation)); }

	void Render(bool bLighting, bool bNoAlpha, bool bEdges, unsigned char* nLastColor, bool* bTrans);
	inline void RenderBox(bool bHilite, float fLineWidth)
	{
		glPushMatrix();
		glTranslatef(m_fPosition[0], m_fPosition[1], m_fPosition[2]);
		glRotatef(m_fRotation[3], m_fRotation[0], m_fRotation[1], m_fRotation[2]);

		if (bHilite && ((m_nState & LC_PIECE_SELECTED) != 0))
		{
			glColor3ubv(FlatColorArray[m_nState & LC_PIECE_FOCUSED ? LC_COL_FOCUSED : LC_COL_SELECTED]);
			glLineWidth(2*fLineWidth);
			glPushAttrib(GL_POLYGON_BIT);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glCallList(m_nBoxList);
			glPopAttrib();
			glLineWidth(fLineWidth);
		}
		else
		{
			glColor3ubv(FlatColorArray[m_nColor]);
			glCallList(m_nBoxList);
		}
		glPopMatrix();
	}

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
	void LineFacet(float* p1, float* p2, float* p3, float* p4, CLICKLINE* pLine);
	void RemoveKeys();
	void BuildDrawInfo();

	// Position
	PIECE_KEY* m_pAnimationKeys;
	PIECE_KEY* m_pInstructionKeys;

	// Atributes
	PieceInfo* m_pPieceInfo;
	BoundingBox m_BoundingBox;
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
	GLuint m_nBoxList;
	CONNECTION* m_pConnections;
	void* m_pDrawInfo;
};


#endif // _PIECE_H

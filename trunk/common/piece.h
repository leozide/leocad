#ifndef _PIECE_H_
#define _PIECE_H_

class File;
class Piece;
class Group;
class PieceInfo;
class lcMesh;

#include "object.h"
#include "globals.h"
#include "typedefs.h"
#include "defines.h"
#include "config.h"

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

	void Select (bool bSelecting, bool bFocus);
	virtual void InsertTime(u32 start, u32 time);
	virtual void RemoveTime(u32 start, u32 time);
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes);





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

	lcMesh* GetMesh() const
	{ return m_Mesh; }

	const Matrix44& GetModelWorld() const
	{ return m_ModelWorld; }

	void MinIntersectDist(LC_CLICKLINE* pLine);
	bool IsVisible(u32 Time);
	void Initialize(float x, float y, float z, u32 Time, unsigned char nColor);
	void CreateName(Piece* pPiece);
	void AddConnections(CONNECTION_TYPE* pConnections);
	void RemoveConnections(CONNECTION_TYPE* pConnections);
	void CompareBoundingBox(float box[6]);
	void GetBoundingBox(Vector3 Verts[8]);
	void SetPieceInfo(PieceInfo* pPieceInfo);
	bool FileLoad(File& file, char* name);
	void FileSave(File& file, Group* pGroups);

	void CalculateConnections(CONNECTION_TYPE* pConnections, unsigned short nTime, bool bForceRebuild, bool bFixOthers);
	void UpdatePosition(unsigned short nTime);
	void Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz);

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
	void SetTimeShow(int Time)
	{ m_TimeShow = Time; }
	const unsigned char GetTimeShow()
	{ return m_TimeShow; }
	void SetTimeHide(unsigned char Time)
	{ m_TimeHide = Time; }
	const unsigned char GetTimeHide()
	{ return m_TimeHide; }
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

	u32 m_TimeShow;
	u32 m_TimeHide;

	unsigned char m_nColor;
	unsigned char m_nState;
	char m_strName[81];

	// Temporary variables
	Matrix44 m_ModelWorld;
	float m_fPosition[3];
	float m_fRotation[4];
	CONNECTION* m_pConnections;
	lcMesh* m_Mesh;

	// Workaround for MSVC6 poor template support.
	template <class T>
	struct TypeToType
	{
		typedef T type;
	};

	template<class T>
	void BuildMesh(TypeToType<T>);
};


#endif // _PIECE_H

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "str.h"
#include "algebra.h"

class lcFile;
class Matrix;
class lcObject;
/*
#define LC_OBJECT_NAME_LEN         80
#define LC_OBJECT_HIDDEN         0x01
#define LC_OBJECT_SELECTED       0x02
#define LC_OBJECT_FOCUSED        0x04
*/

#define LC_OBJECT_TIME_MAX 0xffffffff

typedef enum
{
	LC_OBJECT_PIECE,
	LC_OBJECT_CAMERA,
	LC_OBJECT_CAMERA_TARGET,
	LC_OBJECT_LIGHT,
	LC_OBJECT_LIGHT_TARGET,
//	LC_OBJECT_CURVE,
//	LC_OBJECT_CURVE_POINT,
//	LC_OBJECT_GROUP,
//	LC_OBJECT_GROUP_PIVOT,
} LC_OBJECT_TYPE;

// key handling
struct LC_OBJECT_KEY
{
	u32 Time;
	float Value[4];
	int Type;
	LC_OBJECT_KEY* Next;
};

struct LC_OBJECT_KEY_INFO
{
	const char* Description;
	int Size; // number of floats
	int Type;
};

struct lcClickLine
{
	Vector3 Start;
	Vector3 End;
	float Dist;
	const lcObject* Object;
};

class lcObject
{
public:
	lcObject(LC_OBJECT_TYPE nType);
	virtual ~lcObject();

public:
	// Move the object.
	virtual void Move(u32 Time, bool AddKey, const Vector3& Delta) = 0;

	// bSelecting is the action (add/remove), bFocus means "add focus if selecting"
	// or "remove focus only if deselecting", bMultiple = Ctrl key is down
	virtual void Select(bool bSelecting, bool bFocus, bool bMultiple) = 0;



	// Creates a unique name for this object starting with an existing prefix.
	void SetUniqueName(lcObject* List, const String& Prefix);

	// Check if this object intersects a ray at a point closer to its start.
	virtual void ClosestLineIntersect(lcClickLine& ClickLine) const = 0;

	// Check if this object intersects the volume specified by a given set of planes.
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const = 0;



  /*
  virtual void UpdatePosition (u32 Time) = 0;
  virtual void CompareBoundingBox (float *box) { };
  virtual void Render (LC_RENDER_INFO* pInfo) = 0;

  // Query functions
  virtual bool IsSelected() const
    { return (m_nState & LC_OBJECT_SELECTED) != 0; };
  virtual bool IsFocused() const
    { return (m_nState & LC_OBJECT_FOCUSED) != 0; };
  virtual bool IsVisible(u32 Time) const
    { return (m_nState & LC_OBJECT_HIDDEN) == 0; }


  // State change, most classes will have to replace these functions
  virtual void SetSelection(bool bSelect, void *pParam = NULL)
    {
      if (bSelect)
	m_nState |= LC_OBJECT_SELECTED;
      else
	m_nState &= ~(LC_OBJECT_SELECTED | LC_OBJECT_FOCUSED);
    };
  virtual void SetFocus(bool bFocus, void *pParam = NULL)
    {
      if (bFocus)
	m_nState |= (LC_OBJECT_SELECTED | LC_OBJECT_FOCUSED);
      else
	m_nState &= ~LC_OBJECT_FOCUSED;
    };
  virtual void SetVisible(bool bVisible)
    {
      if (bVisible)
	m_nState &= ~LC_OBJECT_HIDDEN;
      else
      {
	m_nState |= LC_OBJECT_HIDDEN;
	SetSelection (false, NULL);
      }
    }
  */

	// determine the object type
	bool IsPiece () const
	{ return m_nObjectType == LC_OBJECT_PIECE; }
	bool IsCamera () const
	{ return m_nObjectType == LC_OBJECT_CAMERA; }
	bool IsLight () const
	{ return m_nObjectType == LC_OBJECT_LIGHT; }
//	bool IsCurve () const
//	{ return m_nObjectType == LC_OBJECT_CURVE; }

	LC_OBJECT_TYPE GetType () const
	{ return m_nObjectType; }

protected:
	virtual bool FileLoad(lcFile& file);
	virtual void FileSave(lcFile& file) const;


	// Key handling stuff
public:
	void CalculateSingleKey(u32 Time, bool Animation, int KeyType, float* Value) const;
	void ChangeKey(u32 Time, bool AddKey, const float* Value, int KeyType);
	virtual void InsertTime(u32 Start, u32 Time);
	virtual void RemoveTime(u32 Start, u32 Time);

	int GetKeyTypeCount () const
	{ return m_nKeyInfoCount; }
	const LC_OBJECT_KEY_INFO* GetKeyTypeInfo (int index) const
	{ return &m_pKeyInfo[index]; };
	const float* GetKeyTypeValue (int index) const
	{ return m_pKeyValues[index]; };

protected:
	void RegisterKeys(float *values[], LC_OBJECT_KEY_INFO* info, int count);
	void CalculateKeys(u32 Time);
	void RemoveKeys();

	LC_OBJECT_KEY* m_Keys;
	float** m_pKeyValues;

	LC_OBJECT_KEY_INFO *m_pKeyInfo;
	int m_nKeyInfoCount;


public:
	lcObject* m_Next;

	String m_Name;

private:
	// Object type
	LC_OBJECT_TYPE m_nObjectType;
};

#endif

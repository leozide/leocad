#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "lc_math.h"

class Object;

enum LC_OBJECT_TYPE
{
	LC_OBJECT_PIECE,
	LC_OBJECT_CAMERA,
	LC_OBJECT_CAMERA_TARGET,
	LC_OBJECT_LIGHT,
	LC_OBJECT_LIGHT_TARGET,
//	LC_OBJECT_GROUP,
//	LC_OBJECT_GROUP_PIVOT,
};

// key handling
struct LC_OBJECT_KEY
{
	unsigned short  time;
	float           param[4];
	unsigned char   type;
	LC_OBJECT_KEY*  next;
};

struct LC_OBJECT_KEY_INFO
{
	const char *description;
	unsigned char size; // number of floats
	unsigned char type;
};

struct lcClickLine
{
	lcVector3 Start;
	lcVector3 End;
	float MinDist;
	Object* Closest;
};

class Object
{
public:
	Object(LC_OBJECT_TYPE nType);
	virtual ~Object();

public:
	// Move the object.
	virtual void Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz) = 0;

	// Check if the object intersects the ray.
	virtual void MinIntersectDist(lcClickLine* ClickLine) = 0;

	// bSelecting is the action (add/remove), bFocus means "add focus if selecting"
	// or "remove focus only if deselecting", bMultiple = Ctrl key is down
	virtual void Select(bool bSelecting, bool bFocus, bool bMultiple) = 0;

	// Check if the object intersects the volume specified by a given set of planes.
	virtual bool IntersectsVolume(const lcVector4 Planes[6]) const = 0;


  /*
  virtual void UpdatePosition(unsigned short nTime, bool bAnimation) = 0;
  virtual void CompareBoundingBox(float *box) { };
  virtual void Render(LC_RENDER_INFO* pInfo) = 0;

  // Query functions
  virtual bool IsSelected() const
    { return (m_nState & LC_OBJECT_SELECTED) != 0; };
  virtual bool IsFocused() const
    { return (m_nState & LC_OBJECT_FOCUSED) != 0; };
  virtual bool IsVisible(unsigned short nTime, bool bAnimation) const
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
  virtual bool SetColor(int nColor)
    { return false; };
  */

	bool IsPiece() const
	{
		return m_nObjectType == LC_OBJECT_PIECE;
	}

	bool IsCamera() const
	{
		return m_nObjectType == LC_OBJECT_CAMERA;
	}

	bool IsLight() const
	{
		return m_nObjectType == LC_OBJECT_LIGHT;
	}

	LC_OBJECT_TYPE GetType() const
	{
		return m_nObjectType;
	}

	virtual const char* GetName() const = 0;

protected:
	virtual bool FileLoad(lcFile& file);
	virtual void FileSave(lcFile& file) const;


public:
	void CalculateSingleKey(unsigned short nTime, int keytype, float *value) const;
	void ChangeKey(unsigned short time, bool addkey, const float *param, unsigned char keytype);
	virtual void InsertTime(unsigned short start, unsigned short time);
	virtual void RemoveTime(unsigned short start, unsigned short time);

	int GetKeyTypeCount() const
	{
		return m_nKeyInfoCount;
	}

	const LC_OBJECT_KEY_INFO* GetKeyTypeInfo(int index) const
	{
		return &m_pKeyInfo[index];
	}

	const float* GetKeyTypeValue(int index) const
	{
		return m_pKeyValues[index];
	}

protected:
	void RegisterKeys(float *values[], LC_OBJECT_KEY_INFO* info, int count);
	void CalculateKeys(unsigned short nTime);

private:
	void RemoveKeys();

	LC_OBJECT_KEY* m_pInstructionKeys;
	float **m_pKeyValues;

	LC_OBJECT_KEY_INFO *m_pKeyInfo;
	int m_nKeyInfoCount;

private:
	LC_OBJECT_TYPE m_nObjectType;
};

#endif

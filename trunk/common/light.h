#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "object.h"

#define LC_LIGHT_HIDDEN				0x01
#define LC_LIGHT_SELECTED			0x02
#define LC_LIGHT_FOCUSED			0x04
#define LC_LIGHT_TARGET_SELECTED	0x08
#define LC_LIGHT_TARGET_FOCUSED		0x10
#define LC_LIGHT_ENABLED			0x20

class Light;
class LightTarget;

class LightTarget : public Object
{
 public:
  LightTarget (Light *pParent);
  ~LightTarget ();

 public:
  void MinIntersectDist (LC_CLICKLINE* pLine);

  Light* GetParent () const
    { return m_pParent; }

 protected:
  Light* m_pParent;

  friend class Light; // FIXME: needed for BoundingBoxCalculate ()
  // remove and use UpdatePosition instead
};

class Light : public Object
{
public:
	Light();
	~Light();

	Light* m_pNext;

	bool IsVisible()
		{ return (m_nState & LC_LIGHT_HIDDEN) == 0; } 
	bool IsSelected()
		{ return (m_nState & (LC_LIGHT_SELECTED|LC_LIGHT_TARGET_SELECTED)) != 0; } 
	void Select()
		{ m_nState |= (LC_LIGHT_SELECTED|LC_LIGHT_TARGET_SELECTED); } 
	void UnSelect()
		{ m_nState &= ~(LC_LIGHT_SELECTED|LC_LIGHT_FOCUSED|LC_LIGHT_TARGET_SELECTED|LC_LIGHT_TARGET_FOCUSED); } 
	void UnFocus()
		{ m_nState &= ~(LC_LIGHT_FOCUSED|LC_LIGHT_TARGET_FOCUSED); } 
	void FocusEye()
		{ m_nState |= (LC_LIGHT_FOCUSED|LC_LIGHT_SELECTED); } 
	void FocusTarget()
		{ m_nState |= (LC_LIGHT_TARGET_FOCUSED|LC_LIGHT_TARGET_SELECTED); } 
	const char* GetName()
		{ return m_strName; }

	void MinIntersectDist(LC_CLICKLINE* Line);
	void UpdatePosition(unsigned short nTime, bool bAnimation);

protected:
	void RemoveKeys();

	//	BoundingBox m_BoundingBox;
	//	BoundingBox m_TargetBoundingBox;

	unsigned char m_nState;
	char m_strName[81];
};

#endif // _LIGHT_H_

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "opengl.h"
#include "object.h"

#define LC_LIGHT_HIDDEN			0x01
#define LC_LIGHT_SELECTED		0x02
#define LC_LIGHT_FOCUSED		0x04
#define LC_LIGHT_TARGET_SELECTED	0x08
#define LC_LIGHT_TARGET_FOCUSED		0x10
#define LC_LIGHT_ENABLED		0x20

class Light;
class LightTarget;

typedef enum { LK_POSITION, LK_TARGET, LK_COLOR } LK_TYPES;

typedef struct LC_LIGHT_KEY
{
  unsigned short  time;
  float	          param[3];
  unsigned char   type;
  LC_LIGHT_KEY*  next;
} LC_LIGHT_KEY;

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
  Light (float px, float py, float pz);
  Light (float px, float py, float pz, float tx, float ty, float tz);
  virtual ~Light ();





 public:
  Light* m_pNext;

  bool IsVisible()
    { return (m_nState & LC_LIGHT_HIDDEN) == 0; } 
  bool IsSelected()
    { return (m_nState & (LC_LIGHT_SELECTED|LC_LIGHT_TARGET_SELECTED)) != 0; } 
  bool IsEyeSelected()
    { return (m_nState & LC_LIGHT_SELECTED) != 0; } 
  bool IsTargetSelected()
    { return (m_nState & LC_LIGHT_TARGET_SELECTED) != 0; } 
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
  void GetTargetPos (float *pos) const
    { memcpy (pos, m_fTarget, sizeof (float[3])); }

  void Render (float fLineWidth);
  void MinIntersectDist (LC_CLICKLINE* Line);
  void UpdatePosition (unsigned short nTime, bool bAnimation);
  void CalculatePosition (unsigned short nTime, bool bAnimation, float pos[3], float target[3], float color[3]);
  void Move (unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz);
  void ChangeKey (unsigned short nTime, bool bAnimation, bool bAddKey, float param[3], unsigned char nKeyType);
  void Setup (int index);

protected:
  void RemoveKeys ();
  void Initialize ();

  // Camera target
  LightTarget* m_pTarget;

  // Position
  LC_LIGHT_KEY* m_pAnimationKeys;
  LC_LIGHT_KEY* m_pInstructionKeys;

  // Attributes
  float m_fCone;
  unsigned char m_nState;
  char m_strName[81];
  bool m_bEnabled;

  GLuint m_nList;
  static GLuint m_nSphereList;
  static GLuint m_nTargetList;

  // Temporary position
  float m_fPos[4];
  float m_fTarget[4];
  float m_fColor[4];
};

#endif // _LIGHT_H_

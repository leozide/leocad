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

typedef enum
{
  LC_LK_POSITION, LC_LK_TARGET, // position
  LC_LK_AMBIENT, LC_LK_DIFFUSE, LC_LK_SPECULAR, // color
  LC_LK_CONSTANT, LC_LK_LINEAR, LC_LK_QUADRATIC, // attenuation
  LC_LK_CUTOFF, LC_LK_EXPONENT, // spot
  LC_LK_COUNT
} LC_LK_TYPES;

class LightTarget : public Object
{
 public:
  LightTarget (Light *pParent);
  ~LightTarget ();

 public:
  void MinIntersectDist (LC_CLICKLINE* pLine);
  void Select (bool bSelecting, bool bFocus, bool bMultiple);
  void Move (unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z)
    {
      // FIXME: move the position handling to the light target
    }

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

  void Select (bool bSelecting, bool bFocus, bool bMultiple);



        void SelectTarget (bool bSelecting, bool bFocus, bool bMultiple);

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
  LightTarget* GetTarget () const
    { return m_pTarget; }


  void Render (float fLineWidth);
  void MinIntersectDist (LC_CLICKLINE* Line);
  void UpdatePosition (unsigned short nTime, bool bAnimation);
  void Move (unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz);
  void Setup (int index);

protected:
  void Initialize ();

  // Camera target
  LightTarget* m_pTarget;

  // Attributes
  float m_fCone;
  unsigned char m_nState;
  char m_strName[81];
  bool m_bEnabled;

  GLuint m_nList;
  static GLuint m_nSphereList;
  static GLuint m_nTargetList;

  // Temporary parameters
  float m_fPos[4];
  float m_fTarget[3];
  float m_fAmbient[4];
  float m_fDiffuse[4];
  float m_fSpecular[4];
  float m_fConstant;
  float m_fLinear;
  float m_fQuadratic;
  float m_fCutoff;
  float m_fExponent;
};

#endif // _LIGHT_H_

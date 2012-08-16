#ifndef _CURVE_H_
#define _CURVE_H_

#include "object.h"
#include "opengl.h"
#include "array.h"

class Curve;
class CurvePoint;
class PieceInfo;

//#define LC_CURVE_POINT_HIDDEN             0x01
#define LC_CURVE_POINT_SELECTED           0x02
#define LC_CURVE_POINT_FOCUSED            0x04
#define LC_CURVE_POINT_ARROW1_FOCUSED     0x08
#define LC_CURVE_POINT_ARROW2_FOCUSED     0x10
#define LC_CURVE_POINT_CONTINUOUS         0x20

typedef enum
{
  LC_CURVE_POINT_KEY_POSITION,
  LC_CURVE_POINT_KEY_DIRECTION1,
  LC_CURVE_POINT_KEY_DIRECTION2,
  LC_CURVE_POINT_KEY_ANGLE,
  LC_CURVE_POINT_KEY_COUNT
} LC_CURVE_POINT_KEY_TYPES;

class CurvePoint : public Object
{
 public:
  // constructors / destructor
  CurvePoint(Curve *pParent);
  CurvePoint(Curve *pParent, const float *pos, const float *dir);
  virtual ~CurvePoint();

  // object functions
  bool FileLoad(lcFile& file);
  void FileSave(lcFile& file) const;
  void MinIntersectDist(lcClickLine* ClickLine);
  bool IntersectsVolume(const lcVector4 Planes[6])
  { return false; }
  void UpdatePosition(unsigned short nTime, bool bAnimation);
  void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz);
  void Render();
  void Select(bool bSelecting, bool bFocus, bool bMultiple);

  // query functions
  Curve* GetParent() const
    { return m_pParent; }
  const float* GetPosition() const
    { return m_fPos; }
  const float* GetDirection1() const
    { return m_fDir1; }
  const float* GetDirection2() const
    { return m_fDir2; }
  float GetAngle () const
    { return m_fAngle; }

 protected:
  void Initialize ();

  Curve* m_pParent;
  static GLuint m_nArrowList;
  static GLuint m_nSphereList;

  unsigned char m_nState;

  // temporary
  float m_fPos[3];
  float m_fDir1[3];
  float m_fDir2[3];
  float m_fAngle;

  unsigned char m_nLastHit; // FIXME: create arrow objects, ugly hack for now
};

// =============================================================================

#define LC_CURVE_HIDDEN         0x01
#define LC_CURVE_SELECTED       0x02
#define LC_CURVE_FOCUSED        0x04
#define LC_CURVE_LOOP           0x10
#define LC_CURVE_FIXED_SIZE     0x20

// all the different types of curved objects
typedef enum
{
  LC_CURVE_TYPE_HOSE
} LC_CURVE_TYPE;

class Curve : public Object
{
 public:
  // constructors / destructor
  Curve ();
  Curve (PieceInfo *pInfo, const float *pos, unsigned char color);
  virtual ~Curve ();

  // object functions
  bool FileLoad(lcFile& file);
  void FileSave(lcFile& file) const;
  void MinIntersectDist(lcClickLine* ClickLine);
  void UpdatePosition(unsigned short nTime, bool bAnimation);
  void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz);
  void Render();
  void Select(bool bSelecting, bool bFocus, bool bMultiple);

  // implementation
  void DeselectOtherPoints(CurvePoint *pSender, bool bFocusOnly);

 protected:
  void Initialize ();
  void TesselateHose ();

  LC_CURVE_TYPE m_nCurveType;
  unsigned char m_nState;
  unsigned char m_nColor;
  float m_fLength;

  float m_fUp0[3];

  GLuint m_nDisplayList;

  PtrArray<CurvePoint> m_Points;
};

#endif // _CURVE_H_

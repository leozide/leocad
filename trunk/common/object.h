#ifndef _OBJECT_H_
#define _OBJECT_H_

class File;
class Matrix;
class Object;
/*
#define LC_OBJECT_NAME_LEN         80
#define LC_OBJECT_HIDDEN         0x01
#define LC_OBJECT_SELECTED       0x02
#define LC_OBJECT_FOCUSED        0x04
*/
typedef enum
{
  LC_OBJECT_PIECE,
  LC_OBJECT_CAMERA,
  LC_OBJECT_CAMERA_TARGET,
  LC_OBJECT_LIGHT,
  LC_OBJECT_LIGHT_TARGET,
  //  LC_OBJECT_GROUP,
  //  LC_OBJECT_GROUP_PIVOT,
  //  LC_OBJECT_CURVE
} LC_OBJECT_TYPE;

// rendering parameters
typedef struct
{
  bool lighting;
  bool stipple;
  bool edges;
  float fLineWidth;

  unsigned char lastcolor;
  bool transparent;
} LC_RENDER_INFO;

// Callback "closure" struct, used to make the necessary parameters known to
// the callback function.
typedef struct LC_CLICKLINE
{
  double a1, b1, c1;
  double a2, b2, c2;
  double mindist;
  Object *pClosest;
  //  void *pParam;

  double PointDistance (float *point);

} LC_CLICKLINE;

class Object
{
 public:
  Object (LC_OBJECT_TYPE nType);
  virtual ~Object ();


  /*

  // These functions must be implemented for each class
  virtual void UpdatePosition (unsigned short nTime, bool bAnimation) = 0;
  virtual void CompareBoundingBox (float *box) { };
  virtual void Move (unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz) = 0;
  virtual void FileSave (File* file) = 0;
  virtual void FileLoad (File* file) = 0;
  virtual void Render (LC_RENDER_INFO* pInfo) = 0;

  // Query functions
  virtual bool IsSelected () const
    { return (m_nState & LC_OBJECT_SELECTED) != 0; };
  virtual bool IsFocused () const
    { return (m_nState & LC_OBJECT_FOCUSED) != 0; };
  virtual bool IsVisible (unsigned short nTime, bool bAnimation) const
    { return (m_nState & LC_OBJECT_HIDDEN) == 0; }
  const char* GetName() const
    { return m_strName; }

  // State change, most classes will have to replace these functions
  virtual void SetSelection (bool bSelect, void *pParam = NULL)
    {
      if (bSelect)
	m_nState |= LC_OBJECT_SELECTED;
      else
	m_nState &= ~(LC_OBJECT_SELECTED | LC_OBJECT_FOCUSED);
    };
  virtual void SetFocus (bool bFocus, void *pParam = NULL)
    {
      if (bFocus)
	m_nState |= (LC_OBJECT_SELECTED | LC_OBJECT_FOCUSED);
      else
	m_nState &= ~LC_OBJECT_FOCUSED;
    };
  virtual void SetVisible (bool bVisible)
    {
      if (bVisible)
	m_nState &= ~LC_OBJECT_HIDDEN;
      else
      {
	m_nState |= LC_OBJECT_HIDDEN;
	SetSelection (false, NULL);
      }
    }
  virtual bool SetColor (int nColor)
    { return false; };

  // collision detection
  virtual void MinIntersectDist (LC_CLICKLINE* pLine) = 0;
  //  void BoxMinIntersectDist (LC_CLICKLINE* pLine);
  //  void SphereMinIntersectDist (LC_CLICKLINE* pLine);
  */
  // determine the object type
  bool IsPiece () const
    { return m_nObjectType == LC_OBJECT_PIECE; }
  bool IsCamera () const
    { return m_nObjectType == LC_OBJECT_CAMERA; }
  bool IsLight () const
    { return m_nObjectType == LC_OBJECT_LIGHT; }
  /*
  bool IsGroup () const
    { return m_nObjectType == LC_OBJECT_GROUP; }
  bool IsCurve () const
    { return m_nObjectType == LC_OBJECT_CURVE; }
  */
  LC_OBJECT_TYPE GetType () const
    { return m_nObjectType; }
  /*
  // For linked lists
  Object* m_pNext;
  Object* m_pNextRender;
  Object* m_pParent;

  Object* GetTopAncestor ()
    { return m_pParent ? m_pParent->GetTopAncestor () : this; }
  */
 protected:
  //  char m_strName[LC_OBJECT_NAME_LEN+1];
  //  unsigned char m_nState;

  // Bounding box stuff
  double BoundingBoxIntersectDist (LC_CLICKLINE* pLine) const;
  void BoundingBoxCalculate (float pos[3]);
  void BoundingBoxCalculate (Matrix *mat);
  void BoundingBoxCalculate (Matrix *mat, float Dimensions[6]);

 private:
  bool BoundingBoxIntersectionbyLine (double a1, double b1, double c1, double a2, double b2, double c2,
				      double *x, double *y, double *z) const;
  bool BoundingBoxPointInside (double x, double y, double z) const;
  float m_fBoxPlanes[4][6];

  LC_OBJECT_TYPE m_nObjectType;
};

#endif

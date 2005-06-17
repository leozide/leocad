#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "opengl.h"
#include "object.h"
#include "algebra.h"

#define LC_CAMERA_HIDDEN            0x01
#define LC_CAMERA_SELECTED          0x02
#define LC_CAMERA_FOCUSED           0x04
#define LC_CAMERA_TARGET_SELECTED   0x08
#define LC_CAMERA_TARGET_FOCUSED    0x10

class Camera;
class CameraTarget;
class File;
class TiledRender;

typedef enum
{
	LC_CAMERA_FRONT,LC_CAMERA_BACK,
	LC_CAMERA_TOP,  LC_CAMERA_UNDER,
	LC_CAMERA_LEFT, LC_CAMERA_RIGHT,
	LC_CAMERA_MAIN, LC_CAMERA_USER
} LC_CAMERA_TYPES;

typedef enum
{
	LC_CK_EYE,
	LC_CK_TARGET,
	LC_CK_UP,
	LC_CK_COUNT
} LC_CK_TYPES;

class CameraTarget : public Object
{
public:
	CameraTarget (Camera *pParent);
	virtual ~CameraTarget ();

 public:
  void MinIntersectDist (LC_CLICKLINE* pLine);
  void Select (bool bSelecting, bool bFocus, bool bMultiple);
  void Move (unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z)
    {
      // FIXME: move the position handling to the camera target
    }

  Camera* GetParent () const
    { return m_pParent; }

 protected:
  Camera* m_pParent;

  friend class Camera; // FIXME: needed for BoundingBoxCalculate ()
  // remove and use UpdatePosition instead
};

class Camera : public Object
{
public:
	Camera ();
	Camera (unsigned char nType, Camera* pPrev);
	Camera (float ex, float ey, float ez, float tx, float ty, float tz, Camera* pCamera);
	Camera (const float *eye, const float *target, const float *up, Camera* pCamera);
	virtual ~Camera ();

	// Query functions.
	inline Point3 GetEyePosition() const
	{ return Point3(m_fEye[0], m_fEye[1], m_fEye[2]); };
	inline Point3 GetTargetPosition() const
	{ return Point3(m_fTarget[0], m_fTarget[1], m_fTarget[2]); };
	inline Vector3 GetUpVector() const
	{ return Vector3(m_fUp[0], m_fUp[1], m_fUp[2]); };

	CameraTarget* GetTarget () const
		{ return m_pTarget; }


	// Deprecated functions:
	const float* GetEyePos () const
		{ return m_fEye; };
	void GetEyePos (float* eye) const
		{ memcpy(eye, m_fEye, sizeof(m_fEye)); };
	const float* GetTargetPos () const
		{ return m_fTarget; };
	void GetTargetPos (float* target) const
		{ memcpy(target, m_fTarget, sizeof(m_fTarget)); };
	const float* GetUpVec () const
		{ return m_fUp; };
	void GetUpVec (float* up) const
		{ memcpy(up, m_fUp, sizeof(m_fUp)); };






public:
	Camera* m_pNext;
	void Hide()
		{ m_nState = LC_CAMERA_HIDDEN; }
	void UnHide()
		{ m_nState &= ~LC_CAMERA_HIDDEN; }
	char* GetName()
		{ return m_strName; }
	bool IsSide()
		{ return m_nType < LC_CAMERA_MAIN; }
	bool IsUser()
		{ return m_nType == LC_CAMERA_USER; }
	bool IsVisible()
		{ return (m_nState & LC_CAMERA_HIDDEN) == 0; }
	bool IsSelected()
		{ return (m_nState & (LC_CAMERA_SELECTED|LC_CAMERA_TARGET_SELECTED)) != 0; } 
	bool IsEyeSelected()
		{ return (m_nState & LC_CAMERA_SELECTED) != 0; } 
	bool IsTargetSelected()
		{ return (m_nState & LC_CAMERA_TARGET_SELECTED) != 0; } 
	bool IsEyeFocused()
		{ return (m_nState & LC_CAMERA_FOCUSED) != 0; } 
	bool IsTargetFocused()
		{ return (m_nState & LC_CAMERA_TARGET_FOCUSED) != 0; } 

	/*
	void Select()
		{ m_nState |= (LC_CAMERA_SELECTED|LC_CAMERA_TARGET_SELECTED); } 
	void UnSelect()
		{ m_nState &= ~(LC_CAMERA_SELECTED|LC_CAMERA_FOCUSED|LC_CAMERA_TARGET_SELECTED|LC_CAMERA_TARGET_FOCUSED); } 
	void UnFocus()
		{ m_nState &= ~(LC_CAMERA_FOCUSED|LC_CAMERA_TARGET_FOCUSED); } 
	void FocusEye()
		{ m_nState |= (LC_CAMERA_FOCUSED|LC_CAMERA_SELECTED); } 
	void FocusTarget()
		{ m_nState |= (LC_CAMERA_TARGET_FOCUSED|LC_CAMERA_TARGET_SELECTED); } 
	*/

	void SelectTarget (bool bSelecting, bool bFocus, bool bMultiple);

public:
	bool FileLoad (File& file);
	void FileSave (File& file) const;
	void MinIntersectDist (LC_CLICKLINE* pLine);
	void Select (bool bSelecting, bool bFocus, bool bMultiple);


	void UpdatePosition(unsigned short nTime, bool bAnimation);
	void Render(float fLineWidth);
	void LoadProjection(float fAspect);

	void DoZoom(int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey, float* center);
	void DoRoll(int dx, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z);

	void StartTiledRendering(int tw, int th, int iw, int ih, float fAspect);
	void GetTileInfo(int* row, int* col, int* width, int* height);
	bool EndTile();

	float m_fovy;
	float m_zNear;
	float m_zFar;

protected:
	void Initialize();

	// Camera target
	CameraTarget* m_pTarget;

	// Attributes
	char m_strName[81];
	unsigned char m_nState;
	unsigned char m_nType;
	GLuint m_nList;
	static GLuint m_nTargetList;

	// Current position and orientation.
	float m_fEye[3];
	float m_fTarget[3];
	float m_fUp[3];

	TiledRender* m_pTR;
};

#endif // _CAMERA_H_

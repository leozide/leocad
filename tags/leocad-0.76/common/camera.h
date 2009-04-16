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
#define LC_CAMERA_ORTHOGRAPHIC      0x20
#define LC_CAMERA_SHOW_CONE         0x40

class lcCamera;
class CameraTarget;
class File;
class TiledRender;

typedef enum
{
	LC_CAMERA_FRONT,
	LC_CAMERA_BACK,
	LC_CAMERA_TOP,
	LC_CAMERA_UNDER,
	LC_CAMERA_LEFT,
	LC_CAMERA_RIGHT,
	LC_CAMERA_MAIN,
	LC_CAMERA_USER
} LC_CAMERA_TYPES;

typedef enum
{
	LC_CK_EYE,
	LC_CK_TARGET,
	LC_CK_UP,
	LC_CK_COUNT
} LC_CK_TYPES;

class CameraTarget : public lcObject
{
public:
	CameraTarget(lcCamera* Parent);
	virtual ~CameraTarget();

	// Base class implementation.
	virtual void ClosestLineIntersect(lcClickLine& ClickLine) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;


	void Select(bool bSelecting, bool bFocus, bool bMultiple);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z)
	{
		// FIXME: move the position handling to the camera target
	}

	lcCamera* m_Parent;
};

class lcCamera : public lcObject
{
public:
	lcCamera();
	lcCamera(unsigned char nType, lcCamera* pPrev);
	lcCamera(float ex, float ey, float ez, float tx, float ty, float tz, lcObject* pCamera);
	lcCamera(const float *eye, const float *target, const float *up, lcObject* pCamera);
	virtual ~lcCamera();

	// Base class implementation.
	virtual void ClosestLineIntersect(lcClickLine& ClickLine) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;


	// Deprecated functions:
	CameraTarget* GetTarget() const
		{ return m_Target; }
	const float* GetUpVec () const
		{ return m_fUp; };
	void GetUpVec (float* up) const
		{ memcpy(up, m_fUp, sizeof(m_fUp)); };
	inline Vector3 GetUpVector() const
	{ return Vector3(m_fUp[0], m_fUp[1], m_fUp[2]); };






public:
	void Hide()
		{ m_nState = LC_CAMERA_HIDDEN; }
	void UnHide()
		{ m_nState &= ~LC_CAMERA_HIDDEN; }
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
	bool IsOrtho() const
		{ return (m_nState & LC_CAMERA_ORTHOGRAPHIC) != 0; }

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

	void SelectTarget(bool bSelecting, bool bFocus, bool bMultiple);

public:
	bool FileLoad(File& file);
	void FileSave(File& file) const;
	void Select(bool bSelecting, bool bFocus, bool bMultiple);


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

public:
	// Camera properties.
	float m_NearDist;
	float m_FarDist;
	float m_FOV;

	// Temporary values.
	Matrix44 m_WorldView;
	Matrix44 m_ViewWorld;

	// Current position.
	Vector3 m_Position;
	Vector3 m_TargetPosition;

protected:
	void Initialize();

	// Camera target
	CameraTarget* m_Target;

	// Attributes
	unsigned char m_nState;
	unsigned char m_nType;

	float m_fUp[3];

	TiledRender* m_pTR;
};

#endif // _CAMERA_H_

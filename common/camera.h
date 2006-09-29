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
#define LC_CAMERA_AUTO_CLIP         0x80

class Camera;
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

class CameraTarget : public Object
{
public:
	CameraTarget(Camera *pParent);
	virtual ~CameraTarget();

public:
	void MinIntersectDist(LC_CLICKLINE* pLine);
	bool IntersectsVolume(const Vector4* Planes, int NumPlanes)
	{ return false; }
	void Select(bool bSelecting, bool bFocus, bool bMultiple);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z)
	{
		// FIXME: move the position handling to the camera target
	}

	const char* GetName() const;

	Camera* GetParent() const
	{ return m_pParent; }

protected:
	Camera* m_pParent;

	friend class Camera; // FIXME: needed for BoundingBoxCalculate()
	// remove and use UpdatePosition instead
};

class Camera : public Object
{
public:
	Camera();
	Camera(unsigned char nType, Camera* pPrev);
	Camera(float ex, float ey, float ez, float tx, float ty, float tz, Camera* pCamera);
	Camera(const float *eye, const float *target, const float *up, Camera* pCamera);
	virtual ~Camera();

	// Query functions.
	inline const Vector3& GetEyePosition() const
	{ return m_Eye; };
	inline const Vector3& GetTargetPosition() const
	{ return m_Target; };
	inline const Vector3& GetUpVector() const
	{ return m_Up; }
	inline const Matrix44& GetWorldViewMatrix() const
	{ return m_WorldView; }

	const char* GetName() const
	{ return m_strName; };

	CameraTarget* GetTarget() const
		{ return m_pTarget; }

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
	bool IsOrtho() const
		{ return (m_nState & LC_CAMERA_ORTHOGRAPHIC) != 0; }
	void SetOrtho(bool ortho)
	{
		if (ortho)
			m_nState |= LC_CAMERA_ORTHOGRAPHIC;
		else
			m_nState &= ~LC_CAMERA_ORTHOGRAPHIC;
	}

	void SetAutoClip(bool clip)
	{
		if (clip)
			m_nState |= LC_CAMERA_AUTO_CLIP;
		else
			m_nState &= ~LC_CAMERA_AUTO_CLIP;
	}

	void SetAlwaysShowCone(bool cone)
	{
		if (cone)
			m_nState |= LC_CAMERA_SHOW_CONE;
		else
			m_nState &= ~LC_CAMERA_SHOW_CONE;
	}

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
	void MinIntersectDist(LC_CLICKLINE* pLine);
	void Select(bool bSelecting, bool bFocus, bool bMultiple);
	bool IntersectsVolume(const Vector4* Planes, int NumPlanes)
	{ return false; }


	void UpdatePosition(unsigned short nTime, bool bAnimation);
	void Render(float fLineWidth);
	void LoadProjection(float fAspect);
	void GetFrustumPlanes(float Aspect, Vector4 Planes[6]) const;
	float GetRoll() const;

	void DoZoom(int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey, float* center);
	void DoRoll(int dx, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void SetRoll(float Roll, unsigned short nTime, bool bAnimation, bool bAddKey);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z);

	void StartTiledRendering(int tw, int th, int iw, int ih, float fAspect);
	void GetTileInfo(int* row, int* col, int* width, int* height);
	bool EndTile();

	float m_fovy;
	float m_zNear;
	float m_zFar;
	unsigned char m_nState;

protected:
	void Initialize();
	void UpdateBoundingBox();

	// Camera target
	CameraTarget* m_pTarget;

	// Attributes
	char m_strName[81];
	unsigned char m_nType;
	GLuint m_nList;
	static GLuint m_nTargetList;

	// Current position and orientation.
	Vector3 m_Eye;
	Vector3 m_Target;
	Vector3 m_Up;
	Matrix44 m_WorldView;

	TiledRender* m_pTR;
};

#endif // _CAMERA_H_

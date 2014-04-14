#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "object.h"
#include "lc_math.h"
#include "lc_array.h"
#include "lc_projection.h"

#define LC_CAMERA_HIDDEN            0x01
#define LC_CAMERA_SELECTED          0x02
#define LC_CAMERA_FOCUSED           0x04
#define LC_CAMERA_TARGET_SELECTED   0x08
#define LC_CAMERA_TARGET_FOCUSED    0x10
#define LC_CAMERA_SIMPLE            0x20

class Camera;
class CameraTarget;
class TiledRender;
class View;

enum LC_VIEWPOINT
{
	LC_VIEWPOINT_FRONT,
	LC_VIEWPOINT_BACK,
	LC_VIEWPOINT_TOP,
	LC_VIEWPOINT_BOTTOM,
	LC_VIEWPOINT_LEFT,
	LC_VIEWPOINT_RIGHT,
	LC_VIEWPOINT_HOME
};

typedef enum
{
	LC_CAMERA_FRONT,LC_CAMERA_BACK,
	LC_CAMERA_TOP,  LC_CAMERA_UNDER,
	LC_CAMERA_LEFT, LC_CAMERA_RIGHT,
	LC_CAMERA_MAIN, LC_CAMERA_USER
} LC_CAMERA_TYPES;

enum LC_CK_TYPES
{
	LC_CK_EYE,
	LC_CK_TARGET,
	LC_CK_UP,
	LC_CK_COUNT
};

class CameraTarget : public Object
{
public:
	CameraTarget(Camera *pParent);
	virtual ~CameraTarget();

public:
	virtual void MinIntersectDist(lcClickLine* ClickLine);
	virtual bool IntersectsVolume(const lcVector4 Planes[6]) const;
	void Select(bool bSelecting, bool bFocus, bool bMultiple);
	void Move(unsigned short nTime, bool bAddKey, float x, float y, float z)
	{
		// FIXME: move the position handling to the camera target
	}

	const char* GetName() const;

	Camera* GetParent() const
	{ return m_pParent; }

protected:
	Camera* m_pParent;
};

class Camera : public Object
{
public:
	Camera(bool Simple);
	Camera(float ex, float ey, float ez, float tx, float ty, float tz);
	virtual ~Camera();

	const char* GetName() const
	{
		return m_strName;
	}

	void CreateName(const lcArray<Camera*>& Cameras);

	CameraTarget* GetTarget() const
	{
		return m_pTarget;
	}

	bool IsSimple() const
	{
		return (m_nState & LC_CAMERA_SIMPLE) != 0;
	}




public:
	void Hide()
		{ m_nState = LC_CAMERA_HIDDEN; }
	void UnHide()
		{ m_nState &= ~LC_CAMERA_HIDDEN; }
	char* GetName()
		{ return m_strName; }
	bool IsSide()
		{ return m_nType < LC_CAMERA_MAIN; }
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

	void SelectTarget(bool bSelecting, bool bFocus, bool bMultiple);

public:
	bool FileLoad(lcFile& file);
	void FileSave(lcFile& file) const;
	virtual void MinIntersectDist(lcClickLine* ClickLine);
	virtual bool IntersectsVolume(const lcVector4 Planes[6]) const;
	void Select(bool bSelecting, bool bFocus, bool bMultiple);


	void UpdatePosition(unsigned short nTime);
	void CopyPosition(const Camera* camera);
	void Render(View* View);
	void LoadProjection(const lcProjection& projection);

	void ZoomExtents(View* view, const lcVector3& Center, const lcVector3* Points, int NumPoints, unsigned short nTime, bool bAddKey);
	void ZoomRegion(View* view, float Left, float Right, float Bottom, float Top, unsigned short nTime, bool bAddKey);
	void DoZoom(int dy, int mouse, unsigned short nTime, bool bAddKey);
	void DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAddKey);
	void DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAddKey, float* center);
	void DoRoll(int dx, int mouse, unsigned short nTime, bool bAddKey);
	void DoCenter(lcVector3& point, unsigned short nTime, bool bAddKey);
	void Move(unsigned short nTime, bool bAddKey, float x, float y, float z);
	void SetViewpoint(LC_VIEWPOINT Viewpoint, unsigned short nTime, bool bAddKey);
	void SetFocalPoint(const lcVector3& focus, unsigned short nTime, bool bAddKey);

	void StartTiledRendering(int tw, int th, int iw, int ih, float fAspect);
	void GetTileInfo(int* row, int* col, int* width, int* height);
	bool EndTile();

	char m_strName[81];

	float m_fovy;
	float m_zNear;
	float m_zFar;

	lcMatrix44 mWorldView;
	lcVector3 mPosition;
	lcVector3 mTargetPosition;
	lcVector3 mUpVector;
	lcVector3 mOrthoTarget;
	lcProjection mProjection;

protected:
	void Initialize();

	// Camera target
	CameraTarget* m_pTarget;

	// Attributes
	unsigned char m_nState;
	unsigned char m_nType;

	TiledRender* m_pTR;
};

#endif // _CAMERA_H_

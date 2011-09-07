#ifndef _CAMERA_H_
#define _CAMERA_H_

#if 0

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

class Camera : public Object
{
public:
	Camera();
	Camera(unsigned char nType, Camera* pPrev);
	Camera(float ex, float ey, float ez, float tx, float ty, float tz, Camera* pCamera);
	Camera(const float *eye, const float *target, const float *up, Camera* pCamera);
	virtual ~Camera();

public:
	bool IsSide()
		{ return m_nType < LC_CAMERA_MAIN; }
	bool IsUser()
		{ return m_nType == LC_CAMERA_USER; }
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

	void SelectTarget(bool bSelecting, bool bFocus);

public:
	bool FileLoad(File& file);
	void FileSave(File& file) const;
	void MinIntersectDist(LC_CLICKLINE* pLine);
	void Select(bool bSelecting, bool bFocus);
	bool IntersectsVolume(const Vector4* Planes, int NumPlanes)
	{ return false; }


	void UpdatePosition(unsigned short nTime);
	void Render(float fLineWidth);
	float GetRoll() const;

	void DoZoom(int dy, int mouse, unsigned short nTime, bool bAddKey);
	void DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAddKey);
	void DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAddKey, float* center);
	void DoRoll(int dx, int mouse, unsigned short nTime, bool bAddKey);
	void SetRoll(float Roll, unsigned short nTime, bool bAddKey);
	void Move(unsigned short nTime, bool bAddKey, float x, float y, float z);

	void StartTiledRendering(int tw, int th, int iw, int ih, float fAspect);
	void GetTileInfo(int* row, int* col, int* width, int* height);
	bool EndTile();

protected:
	void Initialize();
	void UpdateBoundingBox();

	TiledRender* m_pTR;
};

#endif

#endif // _CAMERA_H_

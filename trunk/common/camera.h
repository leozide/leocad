#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "object.h"
#include "lc_math.h"
#include "lc_array.h"
#include "lc_projection.h"

class Camera;
class TiledRender;
class View;

#define LC_CAMERA_HIDDEN            0x0001
#define LC_CAMERA_SIMPLE            0x0002
#define LC_CAMERA_POSITION_SELECTED 0x0010
#define LC_CAMERA_POSITION_FOCUSED  0x0020
#define LC_CAMERA_TARGET_SELECTED   0x0040
#define LC_CAMERA_TARGET_FOCUSED    0x0080
#define LC_CAMERA_UPVECTOR_SELECTED 0x0100
#define LC_CAMERA_UPVECTOR_FOCUSED  0x0200

#define LC_CAMERA_SELECTION_MASK    (LC_CAMERA_POSITION_SELECTED | LC_CAMERA_TARGET_SELECTED | LC_CAMERA_UPVECTOR_SELECTED)
#define LC_CAMERA_FOCUS_MASK        (LC_CAMERA_POSITION_FOCUSED | LC_CAMERA_TARGET_FOCUSED | LC_CAMERA_UPVECTOR_FOCUSED)

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

enum lcCameraSection
{
	LC_CAMERA_SECTION_POSITION,
	LC_CAMERA_SECTION_TARGET,
	LC_CAMERA_SECTION_UPVECTOR
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

	bool IsSimple() const
	{
		return (mState & LC_CAMERA_SIMPLE) != 0;
	}

	virtual bool IsSelected() const
	{
		return (mState & LC_CAMERA_SELECTION_MASK) != 0;
	}

	virtual bool IsSelected(lcuintptr Section) const
	{
		switch (Section)
		{
		case LC_CAMERA_SECTION_POSITION:
			return (mState & LC_CAMERA_POSITION_SELECTED) != 0;
			break;

		case LC_CAMERA_SECTION_TARGET:
			return (mState & LC_CAMERA_TARGET_SELECTED) != 0;
			break;

		case LC_CAMERA_SECTION_UPVECTOR:
			return (mState & LC_CAMERA_UPVECTOR_SELECTED) != 0;
			break;
		}
		return false;
	}

	virtual void SetSelected(bool Selected)
	{
		if (Selected)
			mState |= LC_CAMERA_SELECTION_MASK;
		else
			mState &= ~(LC_CAMERA_SELECTION_MASK | LC_CAMERA_FOCUS_MASK);
	}

	virtual void SetSelected(lcuintptr Section, bool Selected)
	{
		switch (Section)
		{
		case LC_CAMERA_SECTION_POSITION:
			if (Selected)
				mState |= LC_CAMERA_POSITION_SELECTED;
			else
				mState &= ~(LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED);
			break;

		case LC_CAMERA_SECTION_TARGET:
			if (Selected)
				mState |= LC_CAMERA_TARGET_SELECTED;
			else
				mState &= ~(LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED);
			break;

		case LC_CAMERA_SECTION_UPVECTOR:
			if (Selected)
				mState |= LC_CAMERA_UPVECTOR_SELECTED;
			else
				mState &= ~(LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED);
			break;
		}
	}

	virtual bool IsFocused() const
	{
		return (mState & LC_CAMERA_FOCUS_MASK) != 0;
	}

	virtual bool IsFocused(lcuintptr Section) const
	{
		switch (Section)
		{
		case LC_CAMERA_SECTION_POSITION:
			return (mState & LC_CAMERA_POSITION_FOCUSED) != 0;
			break;

		case LC_CAMERA_SECTION_TARGET:
			return (mState & LC_CAMERA_TARGET_FOCUSED) != 0;
			break;

		case LC_CAMERA_SECTION_UPVECTOR:
			return (mState & LC_CAMERA_UPVECTOR_FOCUSED) != 0;
			break;
		}
		return false;
	}

	virtual void SetFocused(lcuintptr Section, bool Focus)
	{
		switch (Section)
		{
		case LC_CAMERA_SECTION_POSITION:
			if (Focus)
				mState |= LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED;
			else
				mState &= ~(LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED);
			break;

		case LC_CAMERA_SECTION_TARGET:
			if (Focus)
				mState |= LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED;
			else
				mState &= ~(LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED);
			break;

		case LC_CAMERA_SECTION_UPVECTOR:
			if (Focus)
				mState |= LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED;
			else
				mState &= ~(LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED);
			break;
		}
	}

	virtual lcuintptr GetFocusSection() const
	{
		if (mState & LC_CAMERA_POSITION_FOCUSED)
			return LC_CAMERA_SECTION_POSITION;

		if (mState & LC_CAMERA_TARGET_FOCUSED)
			return LC_CAMERA_SECTION_TARGET;

		if (mState & LC_CAMERA_UPVECTOR_FOCUSED)
			return LC_CAMERA_SECTION_UPVECTOR;

		return ~0;
	}

	virtual lcVector3 GetSectionPosition(lcuintptr Section) const
	{
		switch (Section)
		{
		case LC_CAMERA_SECTION_POSITION:
			return mPosition;

		case LC_CAMERA_SECTION_TARGET:
			return mTargetPosition;

		case LC_CAMERA_SECTION_UPVECTOR:
			return lcMul31(lcVector3(0, 1, 0), lcMatrix44AffineInverse(mWorldView));
		}

		return lcVector3(0.0f, 0.0f, 0.0f);
	}




public:
//	void Hide()
//		{ mState = LC_CAMERA_HIDDEN; }
//	void UnHide()
//		{ mState &= ~LC_CAMERA_HIDDEN; }
	char* GetName()
		{ return m_strName; }
	bool IsSide()
		{ return m_nType < LC_CAMERA_MAIN; }
	bool IsVisible() const
		{ return (mState & LC_CAMERA_HIDDEN) == 0; }

public:
	virtual void RayTest(lcObjectRayTest& ObjectRayTest) const;
	virtual void BoxTest(lcObjectBoxTest& ObjectBoxTest) const;

	bool FileLoad(lcFile& file);
	void FileSave(lcFile& file) const;
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

	lcuint32 mState;
	unsigned char m_nType;

	TiledRender* m_pTR;
};

#endif // _CAMERA_H_

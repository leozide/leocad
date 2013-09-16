#ifndef LC_CAMERA_H
#define LC_CAMERA_H

#include "lc_object.h"
#include "lc_math.h"
#include "lc_file.h"

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

enum lcViewpoint
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
	LC_CAMERA_POSITION,
	LC_CAMERA_TARGET,
	LC_CAMERA_UPVECTOR
};

class lcCamera : public lcObject
{
public:
	lcCamera(bool Simple);
	virtual ~lcCamera();

	bool IsSimple() const
	{
		return (mState & LC_CAMERA_SIMPLE) != 0;
	}

	virtual bool IsVisible() const
	{
		return (mState & LC_CAMERA_HIDDEN) == 0;
	}

	virtual void SetVisible(bool Visible)
	{
		if (Visible)
			mState &= ~LC_CAMERA_HIDDEN;
		else
		{
			mState |= LC_CAMERA_HIDDEN;
			mState &= ~(LC_CAMERA_SELECTION_MASK | LC_CAMERA_FOCUS_MASK);
		}
	}

	virtual bool IsSelected() const
	{
		return (mState & LC_CAMERA_SELECTION_MASK) != 0;
	}

	virtual bool IsSelected(lcuint32 Section) const
	{
		switch (Section)
		{
		case LC_CAMERA_POSITION:
			return (mState & LC_CAMERA_POSITION_SELECTED) != 0;
			break;

		case LC_CAMERA_TARGET:
			return (mState & LC_CAMERA_TARGET_SELECTED) != 0;
			break;

		case LC_CAMERA_UPVECTOR:
			return (mState & LC_CAMERA_UPVECTOR_SELECTED) != 0;
			break;
		}
		return false;
	}

	virtual bool IsFocused() const
	{
		return (mState & LC_CAMERA_FOCUS_MASK) != 0;
	}

	virtual bool IsFocused(lcuint32 Section) const
	{
		switch (Section)
		{
		case LC_CAMERA_POSITION:
			return (mState & LC_CAMERA_POSITION_FOCUSED) != 0;
			break;

		case LC_CAMERA_TARGET:
			return (mState & LC_CAMERA_TARGET_FOCUSED) != 0;
			break;

		case LC_CAMERA_UPVECTOR:
			return (mState & LC_CAMERA_UPVECTOR_FOCUSED) != 0;
			break;
		}
		return false;
	}

	virtual void ClearSelection()
	{
		mState &= ~(LC_CAMERA_SELECTION_MASK | LC_CAMERA_FOCUS_MASK);
	}

	virtual void ClearFocus()
	{
		mState &= ~LC_CAMERA_FOCUS_MASK;
	}

	virtual void SetSelection(lcuint32 Section, bool Selection)
	{
		switch (Section)
		{
		case LC_CAMERA_POSITION:
			if (Selection)
				mState |= LC_CAMERA_POSITION_SELECTED;
			else
				mState &= ~(LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED);
			break;

		case LC_CAMERA_TARGET:
			if (Selection)
				mState |= LC_CAMERA_TARGET_SELECTED;
			else
				mState &= ~(LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED);
			break;

		case LC_CAMERA_UPVECTOR:
			if (Selection)
				mState |= LC_CAMERA_UPVECTOR_SELECTED;
			else
				mState &= ~(LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED);
			break;
		}
	}

	virtual void SetFocus(lcuint32 Section, bool Focus)
	{
		switch (Section)
		{
		case LC_CAMERA_POSITION:
			if (Focus)
				mState |= LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED;
			else
				mState &= ~(LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED);
			break;

		case LC_CAMERA_TARGET:
			if (Focus)
				mState |= LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED;
			else
				mState &= ~(LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED);
			break;

		case LC_CAMERA_UPVECTOR:
			if (Focus)
				mState |= LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED;
			else
				mState &= ~(LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED);
			break;
		}
	}

	virtual void InvertSelection()
	{
		if (mState & LC_CAMERA_POSITION_SELECTED)
			mState &= ~(LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED);
		else
			mState |= LC_CAMERA_POSITION_SELECTED;

		if (mState & LC_CAMERA_TARGET_SELECTED)
			mState &= ~(LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED);
		else
			mState |= LC_CAMERA_TARGET_SELECTED;

		if (mState & LC_CAMERA_UPVECTOR_SELECTED)
			mState &= ~(LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED);
		else
			mState |= LC_CAMERA_UPVECTOR_SELECTED;
	}

	virtual void InvertSelection(lcuint32 Section)
	{
		switch (Section)
		{
		case LC_CAMERA_POSITION:
			if (mState & LC_CAMERA_POSITION_SELECTED)
				mState &= ~(LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED);
			else
				mState |= LC_CAMERA_POSITION_SELECTED;
			break;

		case LC_CAMERA_TARGET:
			if (mState & LC_CAMERA_TARGET_SELECTED)
				mState &= ~(LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED);
			else
				mState |= LC_CAMERA_TARGET_SELECTED;
			break;

		case LC_CAMERA_UPVECTOR:
			if (mState & LC_CAMERA_UPVECTOR_SELECTED)
				mState &= ~(LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED);
			else
				mState |= LC_CAMERA_UPVECTOR_SELECTED;
			break;
		}
	}

	virtual void SetCurrentTime(lcTime Time)
	{
		if (mPositionKeys.GetSize())
			mPosition = CalculateKey(mPositionKeys, Time);

		if (mTargetPositionKeys.GetSize())
			mTargetPosition = CalculateKey(mTargetPositionKeys, Time);

		if (mUpVectorKeys.GetSize())
			mUpVector = CalculateKey(mUpVectorKeys, Time);

		Update();
	}

	virtual void Save(lcFile& File);
	virtual void Load(lcFile& File);
	virtual void Update();

	virtual void ClosestHitTest(lcObjectHitTest& HitTest);
	virtual void BoxTest(const lcVector4* BoxPlanes, lcArray<lcObjectSection>& ObjectSections);

	virtual void GetRenderMeshes(View* View, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes, lcArray<lcObject*>& InterfaceObjects);
	virtual void RenderInterface(View* View) const;

	virtual void Move(const lcVector3& Distance, lcTime Time, bool AddKey);

	void Zoom(float Distance, lcTime Time, bool AddKeys);
	void Pan(float DistanceX, float DistanceY, lcTime Time, bool AddKeys);
	void Orbit(float AngleX, float AngleY, const lcVector3& Center, lcTime Time, bool AddKeys);
	void Roll(float Angle, lcTime Time, bool AddKeys);
	void ZoomExtents(View* View, const lcVector3& Center, const lcVector3* Points, lcTime Time, bool AddKeys);
	void ZoomRegion(View* View, float Left, float Right, float Bottom, float Top, lcTime Time, bool AddKeys);

	void CopySettings(const lcCamera* Camera);

	void SetViewpoint(lcViewpoint Viewpoint);
	void LoadProjection(float Aspect);

	void StartTiledRendering(int tw, int th, int iw, int ih, float fAspect);
	void GetTileInfo(int* row, int* col, int* width, int* height);
	bool EndTile();

	lcMatrix44 mWorldView;
	lcVector3 mPosition;
	lcVector3 mTargetPosition;
	lcVector3 mUpVector;

	float mFOV;
	float mNear;
	float mFar;

	lcuint32 mState;
	char mName[81];

protected:
	lcArray<lcObjectVector3Key> mPositionKeys;
	lcArray<lcObjectVector3Key> mTargetPositionKeys;
	lcArray<lcObjectVector3Key> mUpVectorKeys;

	class TiledRender* mTR; // TODO: move tiled render to view class and remove tr.cpp
};


/*
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





public:
	void Hide()
		{ m_nState = LC_CAMERA_HIDDEN; }
	void UnHide()
		{ m_nState &= ~LC_CAMERA_HIDDEN; }
	char* GetName()
		{ return m_strName; }
	bool IsSide()
		{ return m_nType < LC_CAMERA_MAIN; }
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

	void SelectTarget(bool bSelecting, bool bFocus, bool bMultiple);

public:
	bool FileLoad(lcFile& file);
	void FileSave(lcFile& file) const;
	virtual void MinIntersectDist(lcClickLine* ClickLine);
	virtual bool IntersectsVolume(const lcVector4 Planes[6]) const;
	void Select(bool bSelecting, bool bFocus, bool bMultiple);


	void UpdatePosition(unsigned short nTime, bool bAnimation);
	void CopyPosition(const Camera* camera);
	void LoadProjection(float fAspect);

	void ZoomExtents(View* view, const lcVector3& Center, const lcVector3* Points, int NumPoints, unsigned short nTime, bool bAnimation, bool bAddKey);
	void ZoomRegion(View* view, float Left, float Right, float Bottom, float Top, unsigned short nTime, bool bAnimation, bool bAddKey);
	void DoZoom(int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey, float* center);
	void DoRoll(int dx, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z);
	void SetViewpoint(LC_VIEWPOINT Viewpoint, unsigned short nTime, bool bAnimation, bool bAddKey);

	void StartTiledRendering(int tw, int th, int iw, int ih, float fAspect);
	void GetTileInfo(int* row, int* col, int* width, int* height);
	bool EndTile();

protected:
	void Initialize();

	// Attributes
	unsigned char m_nType;

	TiledRender* m_pTR;
};
  */

#endif // LC_CAMERA_H

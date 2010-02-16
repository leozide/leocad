#ifndef _VIEW_H_
#define _VIEW_H_

#include "glwindow.h"
#include "typedefs.h"
#include "camera.h"
#include "lc_viewpoint.h"

class Project;
class lcCamera;

class View : public GLWindow
{
public:
	View(Project *pProject, GLWindow *share);
	virtual ~View();

	void OnDraw();
	void OnInitialUpdate();
	void OnSize(int cx, int cy);
	void OnLeftButtonDown(int x, int y, bool bControl, bool bShift);
	void OnLeftButtonUp(int x, int y, bool bControl, bool bShift);
	void OnLeftButtonDoubleClick(int x, int y, bool bControl, bool bShift);
	void OnRightButtonDown(int x, int y, bool bControl, bool bShift);
	void OnRightButtonUp(int x, int y, bool bControl, bool bShift);
	void OnMouseMove(int x, int y, bool bControl, bool bShift);

	LC_CURSOR_TYPE GetCursor(int x, int y) const;

	void DrawViewCube();
	int ViewCubeHitTest(int x, int y);
	void ViewCubeClick();

	void UpdateOverlayScale();

	lcCamera* GetCamera1() const
	{
		return mCamera;
	}

	void SetCamera1(lcCamera* Camera)
	{
		if (mCamera && !Camera)
			mViewpoint = *mCamera;

		mCamera = Camera;
	}

	lcViewpoint* GetViewpoint()
	{
		if (mCamera)
			return mCamera;
		else
			return &mViewpoint;
	}

	const lcViewpoint* GetViewpoint() const
	{
		if (mCamera)
			return mCamera;
		else
			return &mViewpoint;
	}

	void SetViewpoint(const lcViewpoint* Viewpoint)
	{
		mViewpoint = *Viewpoint;
	}

	void StartViewpointTransition(const lcViewpoint* Viewpoint);
	void UpdateViewpointTransition();

	void UpdateCamera() { };
	Matrix44 GetProjectionMatrix() const;

public:
	float m_OverlayScale; // TODO: Remove m_OverlayScale

	bool m_ViewCubeTrack;
	int m_ViewCubeHover;

	int mViewport[4];
	lcCamera* mCamera;
	lcViewpoint mViewpoint;

	// Transition.
	bool mTransitionActive;
	u64 mTransitionStart;
	lcViewpoint mViewpointStart;
	lcViewpoint mViewpointEnd;

	Project* m_Project; // TODO: remove m_Project.
};

#endif // _VIEW_H_

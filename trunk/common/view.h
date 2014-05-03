#ifndef _VIEW_H_
#define _VIEW_H_

#include "lc_glwidget.h"

class Project;

class View : public lcGLWidget
{
public:
	View(Project *project);
	virtual ~View();

	void OnDraw();
	void OnInitialUpdate();
	void OnUpdateCursor();
	void OnLeftButtonDown();
	void OnLeftButtonUp();
	void OnLeftButtonDoubleClick();
	void OnMiddleButtonDown();
	void OnMiddleButtonUp();
	void OnRightButtonDown();
	void OnRightButtonUp();
	void OnMouseMove();
	void OnMouseWheel(float Direction);

	void SetCamera(Camera* camera, bool ForceCopy);
	void SetDefaultCamera();
	lcMatrix44 GetProjectionMatrix() const;
	LC_CURSOR_TYPE GetCursor() const;

	lcObjectSection FindObjectUnderPointer(bool PiecesOnly) const;
	lcArray<lcObjectSection> FindObjectsInBox(float x1, float y1, float x2, float y2) const;

	Project* m_Project;
	Camera* mCamera;
	float m_OverlayScale;

	lcVector3 ProjectPoint(const lcVector3& Point) const
	{
		int Viewport[4] = { 0, 0, mWidth, mHeight };
		return lcProjectPoint(Point, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
	}

	lcVector3 UnprojectPoint(const lcVector3& Point) const
	{
		int Viewport[4] = { 0, 0, mWidth, mHeight };
		return lcUnprojectPoint(Point, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
	}

	void UnprojectPoints(lcVector3* Points, int NumPoints) const
	{
		int Viewport[4] = { 0, 0, mWidth, mHeight };
		lcUnprojectPoints(Points, NumPoints, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
	}
};

#endif // _VIEW_H_

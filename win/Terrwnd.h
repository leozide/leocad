#ifndef _TERRWND_H_
#define _TERRWND_H_

#include "glwindow.h"

class Terrain;
class Camera;

class lcTerrainView : public GLWindow
{
public:
	lcTerrainView(GLWindow *share, Terrain* pTerrain);
	virtual ~lcTerrainView();

	virtual void OnDraw();
	virtual void OnInitialUpdate();
	virtual void OnLeftButtonDown(int x, int y, bool Control, bool Shift);
	virtual void OnLeftButtonUp(int x, int y, bool Control, bool Shift);
	virtual void OnMouseMove(int x, int y, bool Control, bool Shift);

	void ResetCamera();
	void LoadTexture();
	void SetAction(int Action);

	Terrain* mTerrain;
	Camera* mCamera;
	int mAction;
	int mMouseX;
	int mMouseY;
	bool mMouseDown;

	enum TERRAIN_ACTIONS { TERRAIN_ZOOM, TERRAIN_PAN, TERRAIN_ROTATE };
};

#endif // _TERRWND_H_

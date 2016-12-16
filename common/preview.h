#ifndef _PREVIEW_H_
#define _PREVIEW_H_

#include "lc_glwidget.h"

class PieceInfo;

class PiecePreview : public lcGLWidget
{
public:
	PiecePreview();
	virtual ~PiecePreview();

	void OnDraw();
	void OnLeftButtonDown();
	void OnLeftButtonUp();
	void OnLeftButtonDoubleClick();
	void OnRightButtonDown();
	void OnRightButtonUp();
	void OnMouseMove();

	void SetCurrentPiece(PieceInfo* Info);
	void SetDefaultPiece();

protected:
	// Mouse tracking.
	int m_Tracking;
	int m_DownX;
	int m_DownY;

	// Current camera settings.
	float m_Distance;
	float m_RotateX;
	float m_RotateZ;
	bool m_AutoZoom;
};

#endif // _PREVIEW_H_


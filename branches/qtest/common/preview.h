#ifndef _PREVIEW_H_
#define _PREVIEW_H_

#include "glwindow.h"

class PieceInfo;

class PiecePreview : public GLWindow
{
public:
	PiecePreview();
	virtual ~PiecePreview();

	void OnDraw();
	void OnLeftButtonDown(int x, int y, bool Control, bool Shift, bool Alt);
	void OnLeftButtonUp(int x, int y, bool Control, bool Shift, bool Alt);
	void OnLeftButtonDoubleClick(int x, int y, bool Control, bool Shift, bool Alt);
	void OnRightButtonDown(int x, int y, bool Control, bool Shift, bool Alt);
	void OnRightButtonUp(int x, int y, bool Control, bool Shift, bool Alt);
	void OnMouseMove(int x, int y, bool Control, bool Shift, bool Alt);

	PieceInfo* GetCurrentPiece() const
	{ return m_PieceInfo; }
	void SetCurrentPiece(PieceInfo* Info);

protected:
	PieceInfo* m_PieceInfo;

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


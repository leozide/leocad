#ifndef _PREVIEW_H_
#define _PREVIEW_H_

#include "glwindow.h"

class PieceInfo;

class PiecePreview : public GLWindow
{
public:
	PiecePreview(GLWindow *share);
	virtual ~PiecePreview();

	void OnDraw();
  void OnLeftButtonDown(int x, int y, bool Control, bool Shift);
  void OnLeftButtonUp(int x, int y, bool Control, bool Shift);
  void OnLeftButtonDoubleClick(int x, int y, bool Control, bool Shift);
  void OnRightButtonDown(int x, int y, bool Control, bool Shift);
  void OnRightButtonUp(int x, int y, bool Control, bool Shift);
  void OnMouseMove(int x, int y, bool Control, bool Shift);

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


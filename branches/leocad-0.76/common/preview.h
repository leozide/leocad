#ifndef _PREVIEW_H_
#define _PREVIEW_H_

#include "glwindow.h"
#include "lc_message.h"

class PieceInfo;

class PiecePreview : public GLWindow, public lcListener
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

	// Call this function when the pieces list selection changes.
	void SetSelection(void* Selection);

	void ProcessMessage(lcMessageType Message, void* Data);

	// Currently selected model or piece, to change the selection use SetSelection().
	PieceInfo* m_Selection;

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


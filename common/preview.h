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

	PieceInfo* GetCurrentPiece() const
	{ return m_PieceInfo; }
	void SetCurrentPiece(PieceInfo* Info);

protected:
	PieceInfo* m_PieceInfo;
};

#endif // _PREVIEW_H_


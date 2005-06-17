#ifndef _PREVIEW_H_
#define _PREVIEW_H_

#include "glwindow.h"

class PieceInfo;

class PiecePreview : public GLWindow
{
 public:
  PiecePreview (GLWindow *share);
  virtual ~PiecePreview ();

  void OnDraw ();

  void SetCurrentPiece (PieceInfo *pInfo);

 private:
  PieceInfo* m_pPieceInfo;
};

#endif // _PREVIEW_H_


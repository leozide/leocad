#ifndef _VIEW_H_
#define _VIEW_H_

#include "glwindow.h"

class Project;

class View : public GLWindow
{
 public:
  View (Project *pProject, GLWindow *share);
  virtual ~View ();

  void OnDraw ();
  void OnInitialUpdate ();
  void OnLeftButtonDown (int x, int y, bool bControl, bool bShift);
  void OnLeftButtonUp (int x, int y, bool bControl, bool bShift);
  void OnLeftButtonDoubleClick (int x, int y, bool bControl, bool bShift);
  void OnRightButtonDown (int x, int y, bool bControl, bool bShift);
  void OnRightButtonUp (int x, int y, bool bControl, bool bShift);
  void OnMouseMove (int x, int y, bool bControl, bool bShift);

  Project* GetProject () const
    { return m_pProject; }

 protected:
  Project* m_pProject;

  //  virtual void OnInitialUpdate (); // called first time after construct    
};

#endif // _VIEW_H_

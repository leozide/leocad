//
// View the project contents
//

#include "view.h"
#include "project.h"

View::View (Project *pProject, GLWindow *share)
  : GLWindow (share)
{
  m_pProject = pProject;
}

View::~View ()
{
  if (m_pProject != NULL)
    m_pProject->RemoveView (this);
}

void View::OnDraw ()
{
  MakeCurrent ();

  m_pProject->SetViewSize (m_nWidth, m_nHeight);
  m_pProject->Render (false);

  SwapBuffers ();
}

void View::OnInitialUpdate ()
{
  GLWindow::OnInitialUpdate ();
  m_pProject->AddView (this);
}

void View::OnLeftButtonDown (int x, int y, bool bControl, bool bShift)
{
  m_pProject->SetViewSize (m_nWidth, m_nHeight);
  m_pProject->OnLeftButtonDown (x, y, bControl, bShift);
}

void View::OnLeftButtonUp (int x, int y, bool bControl, bool bShift)
{
  m_pProject->SetViewSize (m_nWidth, m_nHeight);
  m_pProject->OnLeftButtonUp (x, y, bControl, bShift);
}

void View::OnLeftButtonDoubleClick (int x, int y, bool bControl, bool bShift)
{
  m_pProject->SetViewSize (m_nWidth, m_nHeight);
  m_pProject->OnLeftButtonDoubleClick (x, y, bControl, bShift);
}

void View::OnRightButtonDown (int x, int y, bool bControl, bool bShift)
{
  m_pProject->SetViewSize (m_nWidth, m_nHeight);
  m_pProject->OnRightButtonDown (x, y, bControl, bShift);
}

void View::OnRightButtonUp (int x, int y, bool bControl, bool bShift)
{
  m_pProject->SetViewSize (m_nWidth, m_nHeight);
  m_pProject->OnRightButtonUp (x, y, bControl, bShift);
}

void View::OnMouseMove (int x, int y, bool bControl, bool bShift)
{
  m_pProject->SetViewSize (m_nWidth, m_nHeight);
  m_pProject->OnMouseMove (x, y, bControl, bShift);
}

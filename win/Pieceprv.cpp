// PiecePrv.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "PiecePrv.h"
#include "Tools.h"
#include "pieceinf.h"
#include "globals.h"
#include "project.h"
#include "preview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPiecePreview

CPiecePreview::CPiecePreview()
{
	m_Preview = NULL;
}

CPiecePreview::~CPiecePreview()
{
	delete m_Preview;
}


BEGIN_MESSAGE_MAP(CPiecePreview, CWnd)
	//{{AFX_MSG_MAP(CPiecePreview)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPiecePreview message handlers

int CPiecePreview::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Preview = new PiecePreview(NULL);
	m_Preview->Create(m_hWnd);
	m_Preview->MakeCurrent();

	return 0;
}

void CPiecePreview::OnDestroy() 
{
	if (m_Preview)
	{
		m_Preview->DestroyContext();
		delete m_Preview;
		m_Preview = NULL;
	}

	CWnd::OnDestroy();
}

void CPiecePreview::SetPieceInfo(PieceInfo* pInfo)
{
	m_Preview->SetCurrentPiece(pInfo);
}

PieceInfo* CPiecePreview::GetPieceInfo() const
{
	return m_Preview->GetCurrentPiece();
}

BOOL GLWindowPreTranslateMessage (GLWindow *wnd, MSG *pMsg);
LRESULT CPiecePreview::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_Preview)
	{
		MSG msg;
		msg.message = message;
		msg.wParam = wParam;
		msg.lParam = lParam;

		if (GLWindowPreTranslateMessage(m_Preview, &msg))
			return TRUE;
	}

	return CWnd::WindowProc(message, wParam, lParam);
}

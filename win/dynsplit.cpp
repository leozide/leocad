//
// CDynamicSplitterWnd is a helper class built on top of the standard CSplitterWnd class
// to make it easier to dynamically split windows at run time.
//

#include "stdafx.h"
#include "dynsplit.h"

BOOL CDynamicSplitterWnd::AttachWindow(CWnd* Wnd, int Row, int Col)
{
	// Make sure the splitter window was created.
	if (!IsWindow(m_hWnd))
	{
		ASSERT(0);
		TRACE(_T("Create splitter before attaching windows to panes"));
		return FALSE;
	}

	// Make sure the row and col indices are within bounds.
	if (Row >= GetRowCount() || Col >= GetColumnCount())
	{
		ASSERT(0);
		return FALSE;
	}

	// Is the window to be attached a valid one.
	if (Wnd == NULL || (!IsWindow(Wnd->m_hWnd)))
	{
		ASSERT(0);
		return FALSE;
	}

	Wnd->SetDlgCtrlID(IdFromRowCol(Row, Col));
	Wnd->SetParent(this);
	Wnd->ShowWindow(SW_SHOW);
	Wnd->InvalidateRect(NULL);

	return TRUE;
}

BOOL CDynamicSplitterWnd::DetachWindow(int Row, int Col)
{
	// Make sure the splitter window was created.
	if (!IsWindow(m_hWnd))
	{
		ASSERT(0);
		TRACE(_T("Create splitter before attaching windows to panes"));
		return FALSE;
	}

	// Make sure the row and col indices are within bounds.
	if (Row >= GetRowCount() || Col >= GetColumnCount())
	{
		ASSERT(0);
		return FALSE;
	}

	CWnd* pWnd = GetPane(Row, Col);
	if (pWnd == NULL || (!IsWindow(pWnd->m_hWnd)))
	{
		ASSERT(0);
		return FALSE;
	}

	// Set the parent window handle to NULL
	// so that this child window is not destroyed
	// when the parent (splitter) is destroyed
	pWnd->SetParent(NULL);

	//Hide the window
	pWnd->ShowWindow(SW_HIDE);
	pWnd->UpdateWindow();

	return TRUE;
}

void CDynamicSplitterWnd::GetViewRowCol(CWnd* Window, int* Row, int* Col)
{
	for (*Row = 0; *Row < GetRowCount(); (*Row)++)
	{
		for (*Col = 0; *Col < GetColumnCount(); (*Col)++)
		{
			if (GetPane(*Row, *Col) == Window)
			{
				return;
			}
		}
	}

	ASSERT(0);
}

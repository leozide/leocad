#include "lc_global.h"
#include "propertiesgridctrl.h"
#include "globals.h"

void CLeoCADMFCPropertyGridCtrl::UpdateColor(COLORREF color)
{
	ASSERT_VALID(this);
	ASSERT_VALID(m_pSel);

	CLeoCADMFCPropertyGridColorProperty* pColorProp = DYNAMIC_DOWNCAST(CLeoCADMFCPropertyGridColorProperty, m_pSel);
	if (pColorProp == NULL)
	{
		ASSERT(FALSE);
		return;
	}

	int ColorIdx;
	for (ColorIdx = 0; ColorIdx < LC_MAXCOLORS; ColorIdx++)
		if (color == RGB(FlatColorArray[ColorIdx][0], FlatColorArray[ColorIdx][1], FlatColorArray[ColorIdx][2]))
			break;

	if (ColorIdx == LC_MAXCOLORS)
		return;

	BOOL bChanged = ColorIdx != pColorProp->GetColor();
	pColorProp->SetColor(ColorIdx, false);

	if (bChanged)
	{
		OnPropertyChanged(pColorProp);
	}
}

void CLeoCADMFCPropertyGridCtrl::CloseColorPopup()
{
	ASSERT_VALID(this);
	ASSERT_VALID(m_pSel);

	CLeoCADMFCPropertyGridColorProperty* pColorProp = DYNAMIC_DOWNCAST(CLeoCADMFCPropertyGridColorProperty, m_pSel);
	if (pColorProp == NULL)
	{
		ASSERT(FALSE);
		return;
	}

	pColorProp->m_pPopup = NULL;

	pColorProp->m_bButtonIsDown = FALSE;
	pColorProp->Redraw();

	if (pColorProp->m_pWndInPlace != NULL)
	{
		pColorProp->m_pWndInPlace->SetFocus();
	}
}

IMPLEMENT_DYNAMIC(CLeoCADMFCPropertyGridColorProperty, CMFCPropertyGridProperty)

CLeoCADMFCPropertyGridColorProperty::CLeoCADMFCPropertyGridColorProperty(const CString& strName, LPCTSTR lpszDescr, DWORD_PTR dwData)
	: CMFCPropertyGridProperty(strName, COleVariant(), lpszDescr, dwData)
{
	m_Color = 0;
	m_ColorOrig = 0;

	m_varValue = (_variant_t)(UINT)m_Color;
	m_varValueOrig = (_variant_t)(UINT)m_ColorOrig;

	m_dwFlags = 1; // AFX_PROP_HAS_LIST
	m_pPopup = NULL;
}

CLeoCADMFCPropertyGridColorProperty::~CLeoCADMFCPropertyGridColorProperty()
{
}

void CLeoCADMFCPropertyGridColorProperty::OnDrawValue(CDC* pDC, CRect rect)
{
	CRect rectColor = rect;

	rect.left += rect.Height();
	CMFCPropertyGridProperty::OnDrawValue(pDC, rect);

	rectColor.right = rectColor.left + rectColor.Height();
	rectColor.DeflateRect(1, 1);
	rectColor.top++;
	rectColor.left++;

	CBrush br(RGB(FlatColorArray[m_Color][0], FlatColorArray[m_Color][1], FlatColorArray[m_Color][2]));
	pDC->FillRect(rectColor, &br);
	pDC->Draw3dRect(rectColor, 0, 0);

	if (m_Color > 13 && m_Color < 22)
	{
	    rectColor.DeflateRect(1, 1);
		rectColor.bottom -= 1;

		for (int x = rectColor.left; x < rectColor.right; x++)
		{
			for (int y = rectColor.top + x % 4; y < rectColor.bottom; y+=4)
				pDC->SetPixel(x, y, RGB(255,255,255));

			for (int y = rectColor.bottom - x % 4; y > rectColor.top; y-=4)
				pDC->SetPixel(x, y, RGB(255,255,255));
		}
	}
}

void CLeoCADMFCPropertyGridColorProperty::OnClickButton(CPoint /*point*/)
{
	ASSERT_VALID(this);
	ASSERT_VALID(m_pWndList);

	m_bButtonIsDown = TRUE;
	Redraw();

	CList<COLORREF,COLORREF> lstDocColors;
	CArray<COLORREF, COLORREF> Colors;

	COLORREF Color = RGB(FlatColorArray[m_Color][0], FlatColorArray[m_Color][1], FlatColorArray[m_Color][2]);

	for (int ColorIdx = 0; ColorIdx < LC_MAXCOLORS; ColorIdx++)
		Colors.Add(RGB(FlatColorArray[ColorIdx][0], FlatColorArray[ColorIdx][1], FlatColorArray[ColorIdx][2]));

	m_pPopup = new CMFCColorPopupMenu(NULL, Colors, Color, NULL, NULL, NULL, lstDocColors, 7, 0);
	m_pPopup->SetPropList(m_pWndList);

	CPoint pt(m_pWndList->GetListRect().left + m_pWndList->GetLeftColumnWidth() + 1, m_rectButton.bottom + 1);
	m_pWndList->ClientToScreen(&pt);

	if (!m_pPopup->Create(m_pWndList, pt.x, pt.y, NULL, FALSE))
	{
		ASSERT(FALSE);
		m_pPopup = NULL;
	}
	else
	{
		m_pPopup->GetMenuBar()->SetFocus();
	}
}

BOOL CLeoCADMFCPropertyGridColorProperty::OnEdit(LPPOINT /*lptClick*/)
{
	m_pWndInPlace = NULL;

	CRect rectEdit;
	CRect rectSpin;

	rectEdit.SetRectEmpty();
	rectSpin.SetRectEmpty();

	CMFCMaskedEdit* pWndEdit = new CMFCMaskedEdit;

	pWndEdit->Create(WS_CHILD, rectEdit, m_pWndList, AFX_PROPLIST_ID_INPLACE);
	m_pWndInPlace = pWndEdit;

	m_pWndInPlace->SetWindowText(FormatProperty());

	m_pWndInPlace->SetFocus();

	m_bInPlaceEdit = TRUE;

	return TRUE;
}

void CLeoCADMFCPropertyGridColorProperty::ResetOriginalValue()
{
	CMFCPropertyGridProperty::ResetOriginalValue();
	m_Color = m_ColorOrig;
}

CString CLeoCADMFCPropertyGridColorProperty::FormatProperty()
{
	ASSERT_VALID(this);

	return colornames[m_Color];
}

void CLeoCADMFCPropertyGridColorProperty::SetColor(int color, bool original)
{
	ASSERT_VALID(this);

	if (m_Color == color)
		return;

	m_Color = color;
	m_varValue = (_variant_t)(UINT)m_Color;

	if (original)
	{
		m_ColorOrig = color;
		m_varValueOrig = (_variant_t)(UINT)m_ColorOrig;
	}

	if (::IsWindow(m_pWndList->GetSafeHwnd()))
	{
		CRect rect = m_Rect;
		rect.DeflateRect(0, 1);

		m_pWndList->InvalidateRect(rect);
		m_pWndList->UpdateWindow();
	}
}

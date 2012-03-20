#include "lc_global.h"
#include <math.h>
#include "ClrPick.h"
#include "ClrPopup.h"
#include "resource.h"
#include "globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DEFAULT_BOX_VALUE -3
#define CUSTOM_BOX_VALUE  -2
#define INVALID_COLOUR    -1

#define MAX_COLOURS 40
#define NUM_COLORS 28

/////////////////////////////////////////////////////////////////////////////
// CColorPopup

CColorPopup::CColorPopup()
{
    Initialise();
}

COLORREF CColorPopup::GetColor(int nIndex)
{
	return RGB(FlatColorArray[nIndex][0], FlatColorArray[nIndex][1], FlatColorArray[nIndex][2]);
}

CColorPopup::CColorPopup(CPoint p, COLORREF crColor, CWnd* pParentWnd, BOOL bDefaultText, BOOL bCustomText)
{
    Initialise();

    m_crColor = m_crInitialColor = crColor;
    m_pParent = pParentWnd;
    m_bDefaultText = bDefaultText;
    m_bCustomText  = bCustomText;

    CColorPopup::Create(p, crColor, pParentWnd, bDefaultText, bCustomText);
}

void CColorPopup::Initialise()
{
//    m_nNumColors = sizeof(m_crColors)/sizeof(ColorTableEntry);
//    ASSERT(m_nNumColors <= MAX_COLOURS);
//    if (m_nNumColors > MAX_COLOURS)
//        m_nNumColors = MAX_COLOURS;

    m_nNumColumns       = 0;
    m_nNumRows          = 0;
    m_nBoxSize          = 18;
    m_nMargin           = ::GetSystemMetrics(SM_CXEDGE);
    m_nCurrentSel       = INVALID_COLOUR;
    m_nChosenColorSel  = INVALID_COLOUR;
    m_pParent           = NULL;
    m_crColor          = m_crInitialColor = RGB(0,0,0);

    // Idiot check: Make sure the colour square is at least 5 x 5;
    if (m_nBoxSize - 2*m_nMargin - 2 < 5) m_nBoxSize = 5 + 2*m_nMargin + 2;

    // Create the font
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    VERIFY(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0));
    m_Font.CreateFontIndirect(&(ncm.lfMessageFont));

    // Create the palette
    struct {
        LOGPALETTE    LogPalette;
        PALETTEENTRY  PalEntry[MAX_COLOURS];
    } pal;

    LOGPALETTE* pLogPalette = (LOGPALETTE*) &pal;
    pLogPalette->palVersion    = 0x300;
    pLogPalette->palNumEntries = (WORD) NUM_COLORS; 

    for (int i = 0; i < NUM_COLORS; i++)
    {
		pLogPalette->palPalEntry[i].peRed   = FlatColorArray[i][0];
		pLogPalette->palPalEntry[i].peGreen = FlatColorArray[i][1];
		pLogPalette->palPalEntry[i].peBlue  = FlatColorArray[i][2];
		pLogPalette->palPalEntry[i].peFlags = 0;
    }

    m_Palette.CreatePalette(pLogPalette);
}

CColorPopup::~CColorPopup()
{
    m_Font.DeleteObject();
    m_Palette.DeleteObject();
}

BOOL CColorPopup::Create(CPoint p, COLORREF crColor, CWnd* pParentWnd, BOOL bDefaultText, BOOL bCustomText)
{
    ASSERT(pParentWnd && ::IsWindow(pParentWnd->GetSafeHwnd()));
    ASSERT(pParentWnd->IsKindOf(RUNTIME_CLASS(CColorPicker)));

    m_pParent  = pParentWnd;
    m_crColor = m_crInitialColor = crColor;

    // Get the class name and create the window
    CString szClassName = AfxRegisterWndClass(CS_CLASSDC|CS_SAVEBITS|CS_HREDRAW|CS_VREDRAW,
                                              0, (HBRUSH)GetStockObject(LTGRAY_BRUSH),0);

    if (!CWnd::CreateEx(0, szClassName, _T(""), WS_VISIBLE|WS_POPUP, 
                        p.x, p.y, 100, 100, // size updated soon
                        pParentWnd->GetSafeHwnd(), 0, NULL))
        return FALSE;

	m_bCustomText = bCustomText;
	m_bDefaultText = bDefaultText;
        
    // Set the window size
    SetWindowSize();

    // Create the tooltips
    CreateToolTips();

    // Find which cell (if any) corresponds to the initial colour
    FindCellFromColor(crColor);

    // Capture all mouse events for the life of this window
    SetCapture();

    return TRUE;
}

BEGIN_MESSAGE_MAP(CColorPopup, CWnd)
    //{{AFX_MSG_MAP(CColorPopup)
    ON_WM_NCDESTROY()
    ON_WM_LBUTTONUP()
    ON_WM_PAINT()
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_WM_QUERYNEWPALETTE()
    ON_WM_PALETTECHANGED()
	ON_WM_KILLFOCUS()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorPopup message handlers

// For tooltips
BOOL CColorPopup::PreTranslateMessage(MSG* pMsg) 
{
    m_ToolTip.RelayEvent(pMsg);
    return CWnd::PreTranslateMessage(pMsg);
}

// If an arrow key is pressed, then move the selection
void CColorPopup::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    int row = GetRow(m_nCurrentSel),
        col = GetColumn(m_nCurrentSel);
    int nSelection = m_nCurrentSel;

    if (nChar == VK_DOWN) 
    {
        if (row == DEFAULT_BOX_VALUE) 
            row = col = 0; 
        else if (row == CUSTOM_BOX_VALUE)
        {
            if (m_bDefaultText)
                row = col = DEFAULT_BOX_VALUE;
            else
                row = col = 0;
        }
        else
        {
            row++;
            if (GetIndex(row,col) < 0)
            {
                if (m_bCustomText)
                    row = col = CUSTOM_BOX_VALUE;
                else if (m_bDefaultText)
                    row = col = DEFAULT_BOX_VALUE;
                else
                    row = col = 0;
            }
        }
        ChangeSelection(GetIndex(row, col));
    }

    if (nChar == VK_UP) 
    {
        if (row == DEFAULT_BOX_VALUE)
        {
            if (m_bCustomText)
                row = col = CUSTOM_BOX_VALUE;
            else
           { 
                row = GetRow(NUM_COLORS-1); 
                col = GetColumn(NUM_COLORS-1); 
            }
        }
        else if (row == CUSTOM_BOX_VALUE)
        { 
            row = GetRow(NUM_COLORS-1); 
            col = GetColumn(NUM_COLORS-1); 
        }
        else if (row > 0) row--;
        else /* row == 0 */
        {
            if (m_bDefaultText)
                row = col = DEFAULT_BOX_VALUE;
            else if (m_bCustomText)
                row = col = CUSTOM_BOX_VALUE;
            else
            { 
                row = GetRow(NUM_COLORS-1); 
                col = GetColumn(NUM_COLORS-1); 
            }
        }
        ChangeSelection(GetIndex(row, col));
    }

    if (nChar == VK_RIGHT) 
    {
        if (row == DEFAULT_BOX_VALUE) 
            row = col = 0; 
        else if (row == CUSTOM_BOX_VALUE)
        {
            if (m_bDefaultText)
                row = col = DEFAULT_BOX_VALUE;
            else
                row = col = 0;
        }
        else if (col < m_nNumColumns-1) 
            col++;
        else 
        { 
            col = 0; row++;
        }

        if (GetIndex(row,col) == INVALID_COLOUR)
        {
            if (m_bCustomText)
                row = col = CUSTOM_BOX_VALUE;
            else if (m_bDefaultText)
                row = col = DEFAULT_BOX_VALUE;
            else
                row = col = 0;
        }

        ChangeSelection(GetIndex(row, col));
    }

    if (nChar == VK_LEFT) 
    {
        if (row == DEFAULT_BOX_VALUE)
        {
            if (m_bCustomText)
                row = col = CUSTOM_BOX_VALUE;
            else
           { 
                row = GetRow(NUM_COLORS-1); 
                col = GetColumn(NUM_COLORS-1); 
            }
        }
        else if (row == CUSTOM_BOX_VALUE)
        { 
            row = GetRow(NUM_COLORS-1); 
            col = GetColumn(NUM_COLORS-1); 
        }
        else if (col > 0) col--;
        else /* col == 0 */
        {
            if (row > 0) { row--; col = m_nNumColumns-1; }
            else 
            {
                if (m_bDefaultText)
                    row = col = DEFAULT_BOX_VALUE;
                else if (m_bCustomText)
                    row = col = CUSTOM_BOX_VALUE;
                else
                { 
                    row = GetRow(NUM_COLORS-1); 
                    col = GetColumn(NUM_COLORS-1); 
                }
            }
        }
        ChangeSelection(GetIndex(row, col));
    }

    if (nChar == VK_ESCAPE) 
    {
        m_crColor = m_crInitialColor;
        EndSelection(CPN_SELENDCANCEL);
        return;
    }

    if (nChar == VK_RETURN || nChar == VK_SPACE)
    {
        EndSelection(CPN_SELENDOK);
        return;
    }

    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

// auto-deletion
void CColorPopup::OnNcDestroy() 
{
    CWnd::OnNcDestroy();
    delete this;
}

void CColorPopup::OnPaint() 
{
    CPaintDC dc(this); // device context for painting

    // Draw the Default Area text
    if (m_bDefaultText)
        DrawCell(&dc, DEFAULT_BOX_VALUE);
 
    // Draw colour cells
    for (int i = 0; i < NUM_COLORS; i++)
        DrawCell(&dc, i);
    
    // Draw custom text
    if (m_bCustomText)
        DrawCell(&dc, CUSTOM_BOX_VALUE);

    // Draw raised window edge (ex-window style WS_EX_WINDOWEDGE is sposed to do this,
    // but for some reason isn't
    CRect rect;
    GetClientRect(rect);
    dc.DrawEdge(rect, EDGE_RAISED, BF_RECT);
}

void CColorPopup::OnMouseMove(UINT nFlags, CPoint point) 
{
    int nNewSelection = INVALID_COLOUR;

    // Translate points to be relative raised window edge
    point.x -= m_nMargin;
    point.y -= m_nMargin;

    // First check we aren't in text box
    if (m_bCustomText && m_CustomTextRect.PtInRect(point))
        nNewSelection = CUSTOM_BOX_VALUE;
    else if (m_bDefaultText && m_DefaultTextRect.PtInRect(point))
        nNewSelection = DEFAULT_BOX_VALUE;
    else
    {
        // Take into account text box
        if (m_bDefaultText) 
            point.y -= m_DefaultTextRect.Height();  

        // Get the row and column
        nNewSelection = GetIndex(point.y / m_nBoxSize, point.x / m_nBoxSize);

        // In range? If not, default and exit
        if (nNewSelection < 0 || nNewSelection >= NUM_COLORS)
        {
            CWnd::OnMouseMove(nFlags, point);
            return;
        }
    }

    // OK - we have the row and column of the current selection (may be CUSTOM_BOX_VALUE)
    // Has the row/col selection changed? If yes, then redraw old and new cells.
    if (nNewSelection != m_nCurrentSel)
        ChangeSelection(nNewSelection);

    CWnd::OnMouseMove(nFlags, point);
}

// End selection on LButtonUp
void CColorPopup::OnLButtonUp(UINT nFlags, CPoint point) 
{    
    CWnd::OnLButtonUp(nFlags, point);

    DWORD pos = GetMessagePos();
    point = CPoint(LOWORD(pos), HIWORD(pos));

    if (m_WindowRect.PtInRect(point))
        EndSelection(CPN_SELENDOK);
    else
        EndSelection(CPN_SELENDCANCEL);
}

/////////////////////////////////////////////////////////////////////////////
// CColorPopup implementation

int CColorPopup::GetIndex(int row, int col) const
{ 
    if ((row == CUSTOM_BOX_VALUE || col == CUSTOM_BOX_VALUE) && m_bCustomText)
        return CUSTOM_BOX_VALUE;
    else if ((row == DEFAULT_BOX_VALUE || col == DEFAULT_BOX_VALUE) && m_bDefaultText)
        return DEFAULT_BOX_VALUE;
    else if (row < 0 || col < 0 || row >= m_nNumRows || col >= m_nNumColumns)
        return INVALID_COLOUR;
    else
    {
        if (row*m_nNumColumns + col >= NUM_COLORS)
            return INVALID_COLOUR;
        else
            return row*m_nNumColumns + col;
    }
}

int CColorPopup::GetRow(int nIndex) const               
{ 
    if (nIndex == CUSTOM_BOX_VALUE && m_bCustomText)
        return CUSTOM_BOX_VALUE;
    else if (nIndex == DEFAULT_BOX_VALUE && m_bDefaultText)
        return DEFAULT_BOX_VALUE;
    else if (nIndex < 0 || nIndex >= NUM_COLORS)
        return INVALID_COLOUR;
    else
        return nIndex / m_nNumColumns; 
}

int CColorPopup::GetColumn(int nIndex) const            
{ 
    if (nIndex == CUSTOM_BOX_VALUE && m_bCustomText)
        return CUSTOM_BOX_VALUE;
    else if (nIndex == DEFAULT_BOX_VALUE && m_bDefaultText)
        return DEFAULT_BOX_VALUE;
    else if (nIndex < 0 || nIndex >= NUM_COLORS)
        return INVALID_COLOUR;
    else
        return nIndex % m_nNumColumns; 
}

void CColorPopup::FindCellFromColor(COLORREF crColor)
{
    if (crColor == CLR_DEFAULT && m_bDefaultText)
    {
        m_nChosenColorSel = DEFAULT_BOX_VALUE;
        return;
    }

    for (int i = 0; i < NUM_COLORS; i++)
    {
        if (GetColor(i) == crColor)
        {
            m_nChosenColorSel = i;
            return;
        }
    }

    if (m_bCustomText)
        m_nChosenColorSel = CUSTOM_BOX_VALUE;
    else
        m_nChosenColorSel = INVALID_COLOUR;
}

// Gets the dimensions of the colour cell given by (row,col)
BOOL CColorPopup::GetCellRect(int nIndex, const LPRECT& rect)
{
    if (nIndex == CUSTOM_BOX_VALUE)
    {
        ::SetRect(rect, 
                  m_CustomTextRect.left,  m_CustomTextRect.top,
                  m_CustomTextRect.right, m_CustomTextRect.bottom);
        return TRUE;
    }
    else if (nIndex == DEFAULT_BOX_VALUE)
    {
        ::SetRect(rect, 
                  m_DefaultTextRect.left,  m_DefaultTextRect.top,
                  m_DefaultTextRect.right, m_DefaultTextRect.bottom);
        return TRUE;
    }

    if (nIndex < 0 || nIndex >= NUM_COLORS)
        return FALSE;

    rect->left = GetColumn(nIndex) * m_nBoxSize + m_nMargin;
    rect->top  = GetRow(nIndex) * m_nBoxSize + m_nMargin;

    // Move everything down if we are displaying a default text area
    if (m_bDefaultText) 
        rect->top += (m_nMargin + m_DefaultTextRect.Height());

    rect->right = rect->left + m_nBoxSize;
    rect->bottom = rect->top + m_nBoxSize;

    return TRUE;
}

// Works out an appropriate size and position of this window
void CColorPopup::SetWindowSize()
{
    CSize TextSize;

    // If we are showing a custom or default text area, get the font and text size.
    if (m_bCustomText || m_bDefaultText)
    {
        CClientDC dc(this);
        CFont* pOldFont = (CFont*) dc.SelectObject(&m_Font);

        // Get the size of the custom text (if there IS custom text)
        TextSize = CSize(0,0);
        if (m_bCustomText)
            TextSize = dc.GetTextExtent(_T("More Colors..."));

        // Get the size of the default text (if there IS default text)
        if (m_bDefaultText)
        {
            CSize DefaultSize = dc.GetTextExtent(_T("Automatic"));
            if (DefaultSize.cx > TextSize.cx) TextSize.cx = DefaultSize.cx;
            if (DefaultSize.cy > TextSize.cy) TextSize.cy = DefaultSize.cy;
        }

        dc.SelectObject(pOldFont);
        TextSize += CSize(2*m_nMargin,2*m_nMargin);

        // Add even more space to draw the horizontal line
        TextSize.cy += 2*m_nMargin + 2;
    }

    // Get the number of columns and rows
    //m_nNumColumns = (int) sqrt((double)m_nNumColors);    // for a square window (yuk)
    m_nNumColumns = 7;
    m_nNumRows = NUM_COLORS / m_nNumColumns;
    if (NUM_COLORS % m_nNumColumns) m_nNumRows++;

    // Get the current window position, and set the new size
    CRect rect;
    GetWindowRect(rect);

    m_WindowRect.SetRect(rect.left, rect.top, 
                         rect.left + m_nNumColumns*m_nBoxSize + 2*m_nMargin,
                         rect.top  + m_nNumRows*m_nBoxSize + 2*m_nMargin);

    // if custom text, then expand window if necessary, and set text width as
    // window width
    if (m_bDefaultText) 
    {
        if (TextSize.cx > m_WindowRect.Width())
            m_WindowRect.right = m_WindowRect.left + TextSize.cx;
        TextSize.cx = m_WindowRect.Width()-2*m_nMargin;

        // Work out the text area
        m_DefaultTextRect.SetRect(m_nMargin, m_nMargin, 
                                  m_nMargin+TextSize.cx, 2*m_nMargin+TextSize.cy);
        m_WindowRect.bottom += m_DefaultTextRect.Height() + 2*m_nMargin;
    }

    // if custom text, then expand window if necessary, and set text width as
    // window width
    if (m_bCustomText) 
    {
        if (TextSize.cx > m_WindowRect.Width())
            m_WindowRect.right = m_WindowRect.left + TextSize.cx;
        TextSize.cx = m_WindowRect.Width()-2*m_nMargin;

        // Work out the text area
        m_CustomTextRect.SetRect(m_nMargin, m_WindowRect.Height(), 
                                 m_nMargin+TextSize.cx, 
                                 m_WindowRect.Height()+m_nMargin+TextSize.cy);
        m_WindowRect.bottom += m_CustomTextRect.Height() + 2*m_nMargin;
   }

    // Need to check it'll fit on screen: Too far right?
    CSize ScreenSize(::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));
    if (m_WindowRect.right > ScreenSize.cx)
        m_WindowRect.OffsetRect(-(m_WindowRect.right - ScreenSize.cx), 0);

    // Too far left?
    if (m_WindowRect.left < 0)
        m_WindowRect.OffsetRect( -m_WindowRect.left, 0);

    // Bottom falling out of screen?
    if (m_WindowRect.bottom > ScreenSize.cy)
    {
        CRect ParentRect;
        m_pParent->GetWindowRect(ParentRect);
        m_WindowRect.OffsetRect(0, -(ParentRect.Height() + m_WindowRect.Height()));
    }

    // Set the window size and position
    MoveWindow(m_WindowRect, TRUE);
}

void CColorPopup::CreateToolTips()
{
    // Create the tool tip
    if (!m_ToolTip.Create(this)) return;

    // Add a tool for each cell
    for (int i = 0; i < NUM_COLORS; i++)
    {
        CRect rect;
        if (!GetCellRect(i, rect)) continue;
            m_ToolTip.AddTool(this, IDS_COLOR01+i, rect, 1);
    }
}

void CColorPopup::ChangeSelection(int nIndex)
{
    CClientDC dc(this);        // device context for drawing

    if (nIndex > NUM_COLORS)
        nIndex = CUSTOM_BOX_VALUE; 

    if ((m_nCurrentSel >= 0 && m_nCurrentSel < NUM_COLORS) ||
        m_nCurrentSel == CUSTOM_BOX_VALUE || m_nCurrentSel == DEFAULT_BOX_VALUE)
    {
        // Set Current selection as invalid and redraw old selection (this way
        // the old selection will be drawn unselected)
        int OldSel = m_nCurrentSel;
        m_nCurrentSel = INVALID_COLOUR;
        DrawCell(&dc, OldSel);
    }

    // Set the current selection as row/col and draw (it will be drawn selected)
    m_nCurrentSel = nIndex;
    DrawCell(&dc, m_nCurrentSel);

    // Store the current colour
//    if (m_nCurrentSel == CUSTOM_BOX_VALUE)
//        m_pParent->SendMessage(CPN_SELCHANGE, (WPARAM) m_crInitialColor, 0);
//    else
		if (m_nCurrentSel == DEFAULT_BOX_VALUE)
    {
        m_crColor = CLR_DEFAULT;
//        m_pParent->SendMessage(CPN_SELCHANGE, (WPARAM) CLR_DEFAULT, 0);
    }
    else
    {
        m_crColor = GetColor(m_nCurrentSel);
//        m_pParent->SendMessage(CPN_SELCHANGE, (WPARAM) m_crColor, 0);
    }
}

void CColorPopup::EndSelection(int nMessage)
{
    ReleaseCapture();

    // If custom text selected, perform a custom colour selection
    if (nMessage != CPN_SELENDCANCEL && m_nCurrentSel == CUSTOM_BOX_VALUE)
    {
        CColorDialog dlg(m_crInitialColor, CC_FULLOPEN | CC_ANYCOLOR, this);

        if (dlg.DoModal() == IDOK)
            m_crColor = dlg.GetColor();
        else
            m_crColor = m_crInitialColor;
   } 

    if (nMessage == CPN_SELENDCANCEL)
        m_crColor = m_crInitialColor;

    m_pParent->SendMessage(nMessage, (WPARAM)m_crColor, (LPARAM)m_nCurrentSel);
    
    DestroyWindow();
}

void CColorPopup::DrawCell(CDC* pDC, int nIndex)
{
    // For the Custom Text area
    if (m_bCustomText && nIndex == CUSTOM_BOX_VALUE)
    {
        // The extent of the actual text button
        CRect TextButtonRect = m_CustomTextRect;
        TextButtonRect.top += 2*m_nMargin;

        // Fill background
        pDC->FillSolidRect(TextButtonRect, ::GetSysColor(COLOR_3DFACE));

        // Draw horizontal line
        pDC->FillSolidRect(m_CustomTextRect.left+2*m_nMargin, m_CustomTextRect.top,
                           m_CustomTextRect.Width()-4*m_nMargin, 1, ::GetSysColor(COLOR_3DSHADOW));
        pDC->FillSolidRect(m_CustomTextRect.left+2*m_nMargin, m_CustomTextRect.top+1,
                           m_CustomTextRect.Width()-4*m_nMargin, 1, ::GetSysColor(COLOR_3DHILIGHT));

        TextButtonRect.DeflateRect(1,1);

        // fill background
        if (m_nChosenColorSel == nIndex && m_nCurrentSel != nIndex)
            pDC->FillSolidRect(TextButtonRect, ::GetSysColor(COLOR_3DLIGHT));
        else
            pDC->FillSolidRect(TextButtonRect, ::GetSysColor(COLOR_3DFACE));

        // Draw button
        if (m_nCurrentSel == nIndex) 
            pDC->DrawEdge(TextButtonRect, EDGE_RAISED, BF_RECT);
        else if (m_nChosenColorSel == nIndex)
            pDC->DrawEdge(TextButtonRect, EDGE_SUNKEN, BF_RECT);

        // Draw custom text
        CFont *pOldFont = (CFont*) pDC->SelectObject(&m_Font);
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(_T("More Colors..."), TextButtonRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        pDC->SelectObject(pOldFont);

        return;
    }        

    // For the Default Text area
    if (m_bDefaultText && nIndex == DEFAULT_BOX_VALUE)
    {
        // Fill background
        pDC->FillSolidRect(m_DefaultTextRect, ::GetSysColor(COLOR_3DFACE));

        // The extent of the actual text button
        CRect TextButtonRect = m_DefaultTextRect;
        TextButtonRect.DeflateRect(1,1);

        // fill background
        if (m_nChosenColorSel == nIndex && m_nCurrentSel != nIndex)
            pDC->FillSolidRect(TextButtonRect, ::GetSysColor(COLOR_3DLIGHT));
        else
            pDC->FillSolidRect(TextButtonRect, ::GetSysColor(COLOR_3DFACE));

        // Draw thin line around text
        CRect LineRect = TextButtonRect;
        LineRect.DeflateRect(2*m_nMargin,2*m_nMargin);
        CPen pen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));
        CPen* pOldPen = pDC->SelectObject(&pen);
        pDC->SelectStockObject(NULL_BRUSH);
        pDC->Rectangle(LineRect);
        pDC->SelectObject(pOldPen);

        // Draw button
        if (m_nCurrentSel == nIndex) 
            pDC->DrawEdge(TextButtonRect, EDGE_RAISED, BF_RECT);
        else if (m_nChosenColorSel == nIndex)
            pDC->DrawEdge(TextButtonRect, EDGE_SUNKEN, BF_RECT);

        // Draw custom text
        CFont *pOldFont = (CFont*) pDC->SelectObject(&m_Font);
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(_T("Automatic"), TextButtonRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        pDC->SelectObject(pOldFont);

        return;
    }        

    CRect rect;
    if (!GetCellRect(nIndex, rect)) return;

    // Select and realize the palette
    CPalette* pOldPalette;
    if (pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
    {
        pOldPalette = pDC->SelectPalette(&m_Palette, FALSE);
        pDC->RealizePalette();
    }

    // fill background
    if (m_nChosenColorSel == nIndex && m_nCurrentSel != nIndex)
        pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DHILIGHT));
    else
        pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));

    // Draw button
    if (m_nCurrentSel == nIndex) 
        pDC->DrawEdge(rect, EDGE_RAISED, BF_RECT);
    else if (m_nChosenColorSel == nIndex)
        pDC->DrawEdge(rect, EDGE_SUNKEN, BF_RECT);

    CBrush brush(PALETTERGB(GetRValue(GetColor(nIndex)), 
                            GetGValue(GetColor(nIndex)), 
                            GetBValue(GetColor(nIndex)) ));
    CPen   pen;
    pen.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));

    CBrush* pOldBrush = (CBrush*) pDC->SelectObject(&brush);
    CPen*   pOldPen   = (CPen*)   pDC->SelectObject(&pen);

    // Draw the cell colour
    rect.DeflateRect(m_nMargin+1, m_nMargin+1);
    pDC->Rectangle(rect);

    rect.DeflateRect(1, 1);
	rect.bottom -= 1;
	if (nIndex > 13 && nIndex < 22)
	{
		for (int x = rect.left; x < rect.right; x++)
		{
			for (int y = rect.top; y < rect.bottom; y+=4)
			{
				if (y == rect.top) y += x%4;
				pDC->SetPixel (x,y,RGB(255,255,255));
			}
			for (int y = rect.bottom; y > rect.top; y-=4)
			{
				if (y == rect.bottom) y-= x%4;
				pDC->SetPixel (x,y,RGB(255,255,255));
			}
		}
	}

    // restore DC and cleanup
    pDC->SelectObject(pOldBrush);
    pDC->SelectObject(pOldPen);
    brush.DeleteObject();
    pen.DeleteObject();

    if (pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
        pDC->SelectPalette(pOldPalette, FALSE);
}

BOOL CColorPopup::OnQueryNewPalette() 
{
    Invalidate();    
    return CWnd::OnQueryNewPalette();
}

void CColorPopup::OnPaletteChanged(CWnd* pFocusWnd) 
{
    CWnd::OnPaletteChanged(pFocusWnd);

    if (pFocusWnd->GetSafeHwnd() != GetSafeHwnd())
        Invalidate();
}

void CColorPopup::OnKillFocus(CWnd* pNewWnd) 
{
	CWnd::OnKillFocus(pNewWnd);

    ReleaseCapture();
    //DestroyWindow(); - causes crash when Custom colour dialog appears.
}

// KillFocus problem fix suggested by Paul Wilkerson.
void CColorPopup::OnActivateApp(BOOL bActive, DWORD hTask) 
{
	CWnd::OnActivateApp(bActive, hTask);

	// If Deactivating App, cancel this selection
	if (!bActive)
		 EndSelection(CPN_SELENDCANCEL);
}

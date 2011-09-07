///////////////////////////////////////
// Miscelaneous Helper Functions

#ifndef _TOOLS_H_
#define _TOOLS_H_

BOOL CreateRGBPalette(HDC hdc, CPalette **ppCPalette);
BOOL FolderBrowse(CString *strFolder, LPCSTR lpszTitle, HWND hWndOwner);
HANDLE MakeDib(HBITMAP hbitmap, UINT bits);
HBITMAP CreateColorBitmap (UINT cx, UINT cy, COLORREF cr);

UINT APIENTRY PrintHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

#endif // _TOOLS_H_

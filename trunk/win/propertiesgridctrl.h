#ifndef _PROPERTIESGRIDCTRL_H_
#define _PROPERTIESGRIDCTRL_H_

class CLeoCADMFCPropertyGridCtrl : public CMFCPropertyGridCtrl
{
public:
	virtual void CloseColorPopup();
	virtual void UpdateColor(COLORREF color);
};

class CLeoCADMFCPropertyGridColorProperty : public CMFCPropertyGridProperty
{
	friend class CLeoCADMFCPropertyGridCtrl;

	DECLARE_DYNAMIC(CLeoCADMFCPropertyGridColorProperty)

// Construction
public:
	CLeoCADMFCPropertyGridColorProperty(const CString& strName, LPCTSTR lpszDescr = NULL, DWORD_PTR dwData = 0);
	virtual ~CLeoCADMFCPropertyGridColorProperty();

// Overrides
public:
	virtual void OnDrawValue(CDC* pDC, CRect rect);
	virtual void OnClickButton(CPoint point);
	virtual BOOL OnEdit(LPPOINT lptClick);
	virtual CString FormatProperty();

protected:
	virtual BOOL OnKillFocus(CWnd* pNewWnd) { return pNewWnd->GetSafeHwnd() != m_pPopup->GetSafeHwnd(); }
	virtual BOOL OnEditKillFocus() { return m_pPopup == NULL; }
	virtual BOOL IsValueChanged() const { return m_Color != m_ColorOrig; }

	virtual void ResetOriginalValue();

// Attributes
public:
	int GetColor() const { return m_Color; }
	void SetColor(int color, bool original);

// Attributes
protected:
	int m_Color;
	int m_ColorOrig;

	CMFCColorPopupMenu* m_pPopup;
	CArray<COLORREF, COLORREF> m_Colors;
};

#endif // _PROPERTIESGRIDCTRL_H_

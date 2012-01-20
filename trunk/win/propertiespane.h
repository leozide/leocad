#pragma once

class Object;

class CPropertiesPane : public CDockablePane
{
public:
	CPropertiesPane();
	virtual ~CPropertiesPane();

	void Update(Object* Focus);

protected:
	CFont m_fntPropList;
	CMFCPropertyGridCtrl m_wndPropList;

	void AdjustLayout();

	void InitPropList();
	void SetPropListFont();

	void SetEmpty(bool Force = false);
	void SetPiece(Object* Focus);
	void SetCamera(Object* Focus);
	void SetLight(Object* Focus);

	void ModifyPiece();
	void ModifyCamera();
	void ModifyLight();

	Object* mObject;

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnPropertyChanged(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};



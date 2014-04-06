#ifndef _MINIFIG_H_
#define _MINIFIG_H_

#include "lc_glwidget.h"
#include "lc_math.h"
#include "lc_array.h"

class PieceInfo;

enum LC_MFW_TYPES
{
	LC_MFW_HATS,
	LC_MFW_HATS2,
	LC_MFW_HEAD,
	LC_MFW_NECK,
	LC_MFW_BODY,
	LC_MFW_BODY2,
	LC_MFW_BODY3,
	LC_MFW_RARM,
	LC_MFW_LARM,
	LC_MFW_RHAND,
	LC_MFW_LHAND,
	LC_MFW_RHANDA,
	LC_MFW_LHANDA,
	LC_MFW_RLEG,
	LC_MFW_LLEG,
	LC_MFW_RLEGA,
	LC_MFW_LLEGA,
	LC_MFW_NUMITEMS
};

struct lcMinifigPieceInfo
{
	char Description[128];
	PieceInfo* Info;
	lcMatrix44 Offset;
};

struct lcMinifig
{
	PieceInfo* Parts[LC_MFW_NUMITEMS];
	int Colors[LC_MFW_NUMITEMS];
	float Angles[LC_MFW_NUMITEMS];
	lcMatrix44 Matrices[LC_MFW_NUMITEMS];
};

class MinifigWizard : public lcGLWidget
{
public:
	MinifigWizard(lcMinifig* Minifig);
	~MinifigWizard ();

	void OnDraw ();
	void OnLeftButtonDown();
	void OnLeftButtonUp();
	void OnLeftButtonDoubleClick();
	void OnRightButtonDown();
	void OnRightButtonUp();
	void OnMouseMove();
	void OnInitialUpdate();

	void Calculate();
	int GetSelectionIndex(int Type) const;
	void SetSelectionIndex(int Type, int Index);
	void SetColor(int Type, int Color);
	void SetAngle(int Type, float Angle);

	void ParseSettings(lcFile& Settings);

	lcArray<lcMinifigPieceInfo> mSettings[LC_MFW_NUMITEMS];

	lcMinifig* mMinifig;

	int m_Tracking;
	int m_DownX;
	int m_DownY;

	float m_Distance;
	float m_RotateX;
	float m_RotateZ;
	bool m_AutoZoom;
};

#endif // _MINIFIG_H_

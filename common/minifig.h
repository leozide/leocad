#pragma once

#include "lc_glwidget.h"
#include "lc_math.h"

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

struct lcMinifigTemplate
{
	QString Parts[LC_MFW_NUMITEMS];
	int Colors[LC_MFW_NUMITEMS];
	float Angles[LC_MFW_NUMITEMS];
};

class MinifigWizard : public lcGLWidget
{
public:
	MinifigWizard();
	~MinifigWizard();

	const std::map<QString, lcMinifigTemplate>& GetTemplates() const
	{
		return mTemplates;
	}

	void SaveTemplate(const QString& TemplateName, const lcMinifigTemplate& Template);
	void DeleteTemplate(const QString& TemplateName);

	void OnDraw();
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

	std::vector<lcMinifigPieceInfo> mSettings[LC_MFW_NUMITEMS];

	lcMinifig mMinifig;

	int mTracking;
	int mDownX;
	int mDownY;

	float mDistance;
	float mRotateX;
	float mRotateZ;
	bool mAutoZoom;

protected:
	void LoadSettings();
	void LoadTemplates();
	void SaveTemplates();

	std::map<QString, lcMinifigTemplate> mTemplates;
	static const char* mSectionNames[LC_MFW_NUMITEMS];
};


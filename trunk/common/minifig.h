#ifndef _MINIFIG_H_
#define _MINIFIG_H_

#include "glwindow.h"
#include "lc_math.h"
#include "array.h"

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

class MinifigWizard : public GLWindow
{
public:
	MinifigWizard (GLWindow *share);
	~MinifigWizard ();

	void OnDraw ();
	void OnInitialUpdate();

	void Calculate();
	int GetSelectionIndex(int Type) const;
	void SetSelectionIndex(int Type, int Index);
	void SetColor(int Type, int Color);
	void SetAngle(int Type, float Angle);

	void GetMinifigNames (char ***names, int *count);
	void SaveMinifig (const char* name);
	bool LoadMinifig (const char* name);
	void DeleteMinifig (const char* name);

	void ParseSettings(lcFile& Settings);

	ObjArray<lcMinifigPieceInfo> mSettings[LC_MFW_NUMITEMS];

	PieceInfo* m_Info[LC_MFW_NUMITEMS];
	int m_Colors[LC_MFW_NUMITEMS];
	float m_Angles[LC_MFW_NUMITEMS];
	lcMatrix44 m_Matrices[LC_MFW_NUMITEMS];

protected:
	// saved minifig templates
	int  m_MinifigCount;
	char **m_MinifigNames;
	char **m_MinifigTemplates;
};

#endif // _MINIFIG_H_

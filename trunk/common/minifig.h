#ifndef _MINIFIG_H_
#define _MINIFIG_H_

#include "glwindow.h"

class PieceInfo;

typedef enum
{
	LC_MFW_HAT,
	LC_MFW_HEAD,
	LC_MFW_TORSO,
	LC_MFW_NECK,
	LC_MFW_LEFT_ARM,
	LC_MFW_RIGHT_ARM,
	LC_MFW_LEFT_HAND,
	LC_MFW_RIGHT_HAND,
	LC_MFW_LEFT_TOOL,
	LC_MFW_RIGHT_TOOL,
	LC_MFW_HIPS,
	LC_MFW_LEFT_LEG,
	LC_MFW_RIGHT_LEG,
	LC_MFW_LEFT_SHOE,
	LC_MFW_RIGHT_SHOE,
	LC_MFW_NUMITEMS
} LC_MFW_TYPES;

struct LC_MFW_PIECEINFO
{
	char name[9];
	char description[65];
	int type;
	float x, y, z;
	float rx, ry, rz;
};

class MinifigWizard : public GLWindow
{
public:
	MinifigWizard(GLWindow *share);
	~MinifigWizard();

	void OnDraw();

	void Calculate();
	void GetItems(int type, LC_MFW_PIECEINFO*** items, int *count);
	void GetSelections(const char** names);
	void ChangePiece(int type, LC_MFW_PIECEINFO* info);
	void ChangeColor(int type, int color);
	void ChangeAngle(int type, float angle);

	void GetMinifigNames(char*** names, int* count);
	void SaveMinifig(const char* name);
	bool LoadMinifig(const char* name);
	void DeleteMinifig(const char* name);

public:
	PieceInfo* m_Info[LC_MFW_NUMITEMS];
	int m_Colors[LC_MFW_NUMITEMS];
	float m_Angles[LC_MFW_NUMITEMS];
	float m_Position[LC_MFW_NUMITEMS][3];
	float m_Rotation[LC_MFW_NUMITEMS][4];

protected:
	// saved minifig templates
	int m_MinifigCount;
	char** m_MinifigNames;
	char** m_MinifigTemplates;
};

#endif // _MINIFIG_H_

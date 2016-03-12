#ifndef _LC_COLORS_H_
#define _LC_COLORS_H_

#include "lc_array.h"
#include "lc_math.h"

#define LC_MAX_COLOR_NAME 64
#define LC_COLOR_DIRECT 0x80000000

struct lcColor
{
	lcuint32 Code;
	bool Translucent;
	lcVector4 Value;
	lcVector4 Edge;
	char Name[LC_MAX_COLOR_NAME];
	char SafeName[LC_MAX_COLOR_NAME];
};

enum
{
	LC_COLORGROUP_SOLID,
	LC_COLORGROUP_TRANSLUCENT,
	LC_COLORGROUP_SPECIAL,
	LC_NUM_COLORGROUPS
};

struct lcColorGroup
{
	lcArray<int> Colors;
	QString Name;
};

enum lcInterfaceColor
{
	LC_COLOR_SELECTED,
	LC_COLOR_FOCUSED,
	LC_COLOR_CAMERA,
	LC_COLOR_LIGHT,
	LC_COLOR_CONTROL_POINT,
	LC_COLOR_CONTROL_POINT_FOCUSED,
	LC_NUM_INTERFACECOLORS
};

extern lcVector4 gInterfaceColors[LC_NUM_INTERFACECOLORS];
extern lcArray<lcColor> gColorList;
extern lcColorGroup gColorGroups[LC_NUM_COLORGROUPS];
extern int gNumUserColors;
extern int gEdgeColor;
extern int gDefaultColor;

void lcLoadDefaultColors();
bool lcLoadColorFile(lcFile& File);
int lcGetColorIndex(lcuint32 ColorCode);
int lcGetBrickLinkColor(int ColorIndex);

inline lcuint32 lcGetColorCodeFromExtendedColor(int Color)
{
	const int ConverstionTable[] = { 4, 12, 2, 10, 1, 9, 14, 15, 8, 0, 6, 13, 13, 334, 36, 44, 34, 42, 33, 41, 46, 47, 7, 382, 6, 13, 11, 383 };
	return ConverstionTable[Color];
}

inline lcuint32 lcGetColorCodeFromOriginalColor(int Color)
{
	const int ConverstionTable[] = { 0, 2, 4, 9, 7, 6, 22, 8, 10, 11, 14, 16, 18, 9, 21, 20, 22, 8, 10, 11 };
	return lcGetColorCodeFromExtendedColor(ConverstionTable[Color]);
}

inline lcuint32 lcGetColorCode(int ColorIndex)
{
	return gColorList[ColorIndex].Code;
}

inline bool lcIsColorTranslucent(int ColorIndex)
{
	return gColorList[ColorIndex].Translucent;
}

#endif // _LC_COLORS_H_

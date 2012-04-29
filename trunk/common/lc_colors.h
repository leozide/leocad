#ifndef _LC_COLORS_H_
#define _LC_COLORS_H_

#include "opengl.h"
#include "array.h"

#define LC_MAX_COLOR_NAME 64
#define LC_COLOR_DIRECT 0x80000000

struct lcColor
{
	lcuint32 Code;
	bool Translucent;
	float Value[4];
	float Edge[4];
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
	ObjArray<int> Colors;
	char Name[LC_MAX_COLOR_NAME];
};

extern ObjArray<lcColor> gColorList;
extern lcColorGroup gColorGroups[LC_NUM_COLORGROUPS];
extern int gNumUserColors;
extern int gEdgeColor;
extern int gDefaultColor;

void lcLoadDefaultColors();
int lcGetColorIndex(lcuint32 ColorCode);

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

inline void lcSetColor(int ColorIndex)
{
	glColor4fv(gColorList[ColorIndex].Value);
}

inline void lcSetEdgeColor(int ColorIndex)
{
	glColor4fv(gColorList[ColorIndex].Edge);
}

inline void lcSetColorFocused()
{
	glColor4f(0.4000f, 0.2980f, 0.8980f, 1.0000f);
}

inline void lcSetColorSelected()
{
	glColor4f(0.8980f, 0.2980f, 0.4000f, 1.0000f);
}

inline void lcSetColorCamera()
{
	glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
}

inline void lcSetColorLight()
{
	glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
}

#endif // _LC_COLORS_H_

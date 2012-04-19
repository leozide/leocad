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

extern ObjArray<lcColor> gColorList;
extern int gNumUserColors;
extern int gEdgeColor;
extern int gDefaultColor;

void lcLoadDefaultColors();
int lcGetColorIndex(lcuint32 ColorCode);

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

/*
void lcColorInit(const char* FileName);
void lcColorShutdown();

#define LC_COLOR_TRANSLUCENT(Color) g_ColorList[Color].Translucent
#define LC_COLOR_RGB(Color) RGB(g_ColorList[Color].Value[0]*255, g_ColorList[Color].Value[1]*255, g_ColorList[Color].Value[2]*255)

//#define LC_MAXCOLORS	28	// Number of colors supported
//#define LC_COL_EDGES	28	// Piece edges
//#define LC_COL_DEFAULT	31	// Default piece color

*/

#endif // _LC_COLORS_H_

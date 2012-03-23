#ifndef _LC_COLORS_H_
#define _LC_COLORS_H_

#define LC_MAX_COLOR_NAME 64

struct lcColor
{
	lcuint32 Code;
	bool Translucent;
	float Value[4];
	float Edge[4];
	char Name[LC_MAX_COLOR_NAME];
};

extern lcColor* gColorList;
extern int gNumColors;

/*
void lcColorInit(const char* FileName);
void lcColorShutdown();

#define LC_COLOR_TRANSLUCENT(Color) g_ColorList[Color].Translucent
#define LC_COLOR_RGB(Color) RGB(g_ColorList[Color].Value[0]*255, g_ColorList[Color].Value[1]*255, g_ColorList[Color].Value[2]*255)

//#define LC_MAXCOLORS	28	// Number of colors supported
//#define LC_COL_EDGES	28	// Piece edges
//#define LC_COL_SELECTED	29	// Selected object
//#define LC_COL_FOCUSED	30	// Focused object
//#define LC_COL_DEFAULT	31	// Default piece color

*/

#endif // _LC_COLORS_H_

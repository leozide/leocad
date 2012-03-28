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

int lcGetColorIndex(lcuint32 ColorCode);
int lcGetColorCode(int ColorIndex);

inline bool lcIsColorTranslucent(int ColorIndex)
{
	return (ColorIndex > 13 && ColorIndex < 22); // temp
}

inline void lcSetColor(int ColorIndex)
{
	float* Color = gColorList[ColorIndex].Value;
	glColor4f(Color[0], Color[1], Color[2], Color[3]);
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

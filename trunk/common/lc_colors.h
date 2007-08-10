#ifndef _LC_COLORS_H_
#define _LC_COLORS_H_

struct lcColor
{
	int Code;
	float Value[4];
	bool Translucent;
	const char* Name;
};

extern lcColor lcColorList[];
extern int lcNumUserColors;
extern int lcNumColors;

#define LC_COLOR_TRANSLUCENT(Color) lcColorList[Color].Translucent
#define LC_COLOR_RGB(Color) RGB(lcColorList[Color].Value[0]*255, lcColorList[Color].Value[1]*255, lcColorList[Color].Value[2]*255)

#define LC_COLOR_DEFAULT   (lcNumUserColors-1)
#define LC_COLOR_EDGE      (lcNumUserColors+0)
#define LC_COLOR_SELECTION (lcNumUserColors+1)
#define LC_COLOR_FOCUS     (lcNumUserColors+2)

//#define LC_MAXCOLORS	28	// Number of colors supported
//#define LC_COL_EDGES	28	// Piece edges
//#define LC_COL_SELECTED	29	// Selected object
//#define LC_COL_FOCUSED	30	// Focused object
//#define LC_COL_DEFAULT	31	// Default piece color

#endif // _LC_COLORS_H_

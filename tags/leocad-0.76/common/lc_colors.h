#ifndef _LC_COLORS_H_
#define _LC_COLORS_H_

#include "opengl.h"
#include "lc_array.h"
#include "str.h"
#include "file.h"

struct lcColor
{
	u32 Code;
	float Value[4];
	bool Translucent;
	String Name;
};

struct lcColorGroup
{
	String Name;
	lcObjArray<int> Colors;
};

extern lcColor* g_ColorList;
extern u32 lcNumUserColors;
extern u32 lcNumColors;

void lcColorInit(const char* FileName);
void lcColorShutdown();

#define LC_COLOR_TRANSLUCENT(Color) g_ColorList[Color].Translucent
#define LC_COLOR_RGB(Color) RGB(g_ColorList[Color].Value[0]*255, g_ColorList[Color].Value[1]*255, g_ColorList[Color].Value[2]*255)

#define LC_COLOR_DEFAULT   (lcNumUserColors-1)
#define LC_COLOR_EDGE      (lcNumUserColors+0)
#define LC_COLOR_SELECTION (lcNumUserColors+1)
#define LC_COLOR_FOCUS     (lcNumUserColors+2)

//#define LC_MAXCOLORS	28	// Number of colors supported
//#define LC_COL_EDGES	28	// Piece edges
//#define LC_COL_SELECTED	29	// Selected object
//#define LC_COL_FOCUSED	30	// Focused object
//#define LC_COL_DEFAULT	31	// Default piece color

inline void lcSetColor(u32 Index)
{
	glColor4f(g_ColorList[Index].Value[0], g_ColorList[Index].Value[1], g_ColorList[Index].Value[2], g_ColorList[Index].Value[3]);
}

// Convert a color from LDraw to LeoCAD.
inline int lcConvertLDrawColor(u32 Color)
{
	for (u32 i = 0; i < lcNumColors; i++)
		if (g_ColorList[i].Code == Color)
			return i;

	return 0; // black
}

class lcColorConfig
{
public:
	lcColorConfig() { };
	~lcColorConfig() { };

	void Load(lcFile& File);
	void LoadDefault();

	int GetNumUserColors() const
	{
		return mColors.GetSize() - 3;
	}

	lcObjArray<lcColor> mColors;
	lcObjArray<lcColorGroup> mColorGroups;
};

#endif // _LC_COLORS_H_

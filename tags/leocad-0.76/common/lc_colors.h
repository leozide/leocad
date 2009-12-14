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
	float Edge[4];
	bool Translucent;
	String Name;
};

inline bool operator!=(const lcColor& a, const lcColor& b)
{
	if (a.Code != b.Code)
		return true;

	for (int i = 0; i < 4; i++)
		if (a.Value[0] != b.Value[0])
			return true;

	for (int i = 0; i < 4; i++)
		if (a.Edge[0] != b.Edge[0])
			return true;

	if (a.Translucent != b.Translucent)
		return true;

	if (a.Name != b.Name)
		return true;

	return false;
}

struct lcColorGroup
{
	String Name;
	lcObjArray<int> Colors;
};

extern lcColor* g_ColorList;
extern int lcNumUserColors;
extern int lcNumColors;

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

inline void lcSetEdgeColor(u32 Index)
{
	glColor4f(g_ColorList[Index].Edge[0], g_ColorList[Index].Edge[1], g_ColorList[Index].Edge[2], g_ColorList[Index].Edge[3]);
}

// Convert a color from LDraw to LeoCAD.
inline int lcConvertLDrawColor(u32 Color)
{
	for (int i = 0; i < lcNumColors; i++)
		if (g_ColorList[i].Code == Color)
			return i;

	return 0; // black
}

class lcColorConfig
{
public:
	lcColorConfig() { };
	~lcColorConfig() { };

	void SaveConfig();
	void LoadConfig();
	void LoadDefaultConfig();

	void LoadColors(lcFile& File);
	void LoadDefaultColors();

	int GetNumUserColors() const
	{
		return mColors.GetSize() - 3;
	}

	lcObjArray<lcColor> mColors;
	lcObjArray<lcColorGroup> mColorGroups;
};

inline bool operator!=(const lcColorConfig& a, const lcColorConfig& b)
{
	if (a.mColors.GetSize() != b.mColors.GetSize())
		return true;

	if (a.mColorGroups.GetSize() != b.mColorGroups.GetSize())
		return true;

	for (int ColorIdx = 0; ColorIdx < a.mColors.GetSize(); ColorIdx++)
		if (a.mColors[ColorIdx] != b.mColors[ColorIdx])
			return true;

	for (int GroupIdx = 0; GroupIdx < a.mColorGroups.GetSize(); GroupIdx++)
	{
		lcColorGroup& ga = a.mColorGroups[GroupIdx];
		lcColorGroup& gb = b.mColorGroups[GroupIdx];

		if (ga.Colors.GetSize() != gb.Colors.GetSize())
			return true;

		if (memcmp(&ga.Colors[0], &gb.Colors[0], sizeof(ga.Colors[0]) * ga.Colors.GetSize()))
			return true;
	}

	return false;
}

#endif // _LC_COLORS_H_

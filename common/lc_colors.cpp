#include "lc_global.h"
#include "lc_colors.h"
#include "lc_file.h"
#include <float.h>

std::vector<lcColor> gColorList;
lcColorGroup gColorGroups[LC_NUM_COLORGROUPS];
int gNumUserColors;
int gEdgeColor;
int gDefaultColor;

lcVector4 gInterfaceColors[LC_NUM_INTERFACECOLORS] = // todo: make the colors configurable and include the grid and other hardcoded colors here as well.
{
	lcVector4(0.898f, 0.298f, 0.400f, 1.000f), // LC_COLOR_SELECTED
	lcVector4(0.400f, 0.298f, 0.898f, 1.000f), // LC_COLOR_FOCUSED
	lcVector4(0.500f, 0.800f, 0.500f, 1.000f), // LC_COLOR_CAMERA
	lcVector4(0.500f, 0.800f, 0.500f, 1.000f), // LC_COLOR_LIGHT
	lcVector4(0.500f, 0.800f, 0.500f, 0.500f), // LC_COLOR_CONTROL_POINT
	lcVector4(0.400f, 0.298f, 0.898f, 0.500f), // LC_COLOR_CONTROL_POINT_FOCUSED
};

static void GetToken(char*& Ptr, char* Token)
{
	while (*Ptr && *Ptr <= 32)
		Ptr++;

	while (*Ptr > 32)
		*Token++ = *Ptr++;

	*Token = 0;
}

int lcGetBrickLinkColor(int ColorIndex)
{
	struct lcBrickLinkEntry
	{
		int LDraw;
		int BrickLink;
	};

	lcBrickLinkEntry BrickLinkColors[] =
	{
		{   0,  11 }, // Black
		{   1,   7 }, // Blue
		{   2,   6 }, // Green
		{   3,  39 }, // Dark Turquoise
		{   4,   5 }, // Red
		{   5,  47 }, // Dark Pink
		{   6,   8 }, // Brown
		{   7,   9 }, // Light Gray
		{   8,  10 }, // Dark Gray
		{   9,  62 }, // Light Blue
		{  10,  36 }, // Bright Green
		{  11,  40 }, // Light Turquoise
		{  12,  25 }, // Salmon
		{  13,  23 }, // Pink
		{  14,   3 }, // Yellow
		{  15,   1 }, // White
		{  16,  -1 }, // None
		{  17,  38 }, // Light Green
		{  18,  33 }, // Light Yellow
		{  19,   2 }, // Tan
		{  20,  44 }, // Light Violet
		{  21,  46 }, // Glow in Dark Opaque
		{  22,  24 }, // Purple
		{  23, 109 }, // Dark Blue-Violet
		{  24,  -1 }, // None
		{  25,   4 }, // Orange
		{  26,  71 }, // Magenta
		{  27,  34 }, // Lime
		{  28,  69 }, // Dark Tan
		{  29, 104 }, // Bright Pink
		{  30, 157 }, // Medium Lavender
		{  31, 154 }, // Lavender
		{  32,  11 }, // Black
		{  33,  14 }, // Trans-Dark Blue
		{  34,  20 }, // Trans-Green
		{  35, 108 }, // Trans-Bright Green
		{  36,  17 }, // Trans-Red
		{  37,  50 }, // Trans-Dark Pink
		{  38,  18 }, // Trans-Neon Orange
		{  39, 113 }, // Trans-Very Lt Blue
		{  40,  13 }, // Trans-Black
		{  41,  74 }, // Trans-Medium Blue
		{  42,  16 }, // Trans-Neon Green
		{  43,  15 }, // Trans-Light Blue
		{  44, 114 }, // Trans-Light Purple
		{  45, 107 }, // Trans-Pink
		{  46,  19 }, // Trans-Yellow
		{  47,  12 }, // Trans-Clear
		{  52,  51 }, // Trans-Purple
		{  54, 121 }, // Trans-Neon Yellow
		{  57,  98 }, // Trans-Orange
		{  60,  57 }, // Chrome Antique Brass
		{  61,  52 }, // Chrome Blue
		{  62,  64 }, // Chrome Green
		{  63,  82 }, // Chrome Pink
		{  64, 122 }, // Chrome Black
		{  65,   3 }, // Yellow
		{  66,  19 }, // Trans-Yellow
		{  67,  12 }, // Trans-Clear
		{  68,  96 }, // Very Light Orange
		{  69,  93 }, // Light Purple
		{  70,  88 }, // Reddish Brown
		{  71,  86 }, // Light Bluish Gray
		{  72,  85 }, // Dark Bluish Gray
		{  73,  42 }, // Medium Blue
		{  74,  37 }, // Medium Green
		{  75, 116 }, // Speckle Black-Copper
		{  76, 117 }, // Speckle DBGray-Silver
		{  77,  56 }, // Light Pink
		{  78,  90 }, // Light Flesh
		{  79,  60 }, // Milky White
		{  80,  67 }, // Metallic Silver
		{  81,  70 }, // Metallic Green
		{  82,  65 }, // Metallic Gold
		{  83,  11 }, // Black
		{  84, 150 }, // Medium Dark Flesh
		{  85,  89 }, // Dark Purple
		{  86,  91 }, // Dark Flesh
		{  87,  77 }, // Pearl Dark Gray
		{  89,  97 }, // Blue - Violet
		{  92,  28 }, // Flesh
		{ 100,  26 }, // Light Salmon
		{ 110,  43 }, // Violet
		{ 112,  73 }, // Medium Violet
		{ 114, 100 }, // Glitter Trans-Dark Pink
		{ 115,  76 }, // Medium Lime
		{ 117, 101 }, // Glitter Trans-Clear
		{ 118,  41 }, // Aqua
		{ 120,  35 }, // Light Lime
		{ 125,  32 }, // Light Orange
		{ 128,  68 }, // Dark Orange
		{ 129, 102 }, // Glitter Trans-Purple
		{ 132, 111 }, // Speckle Black-Silver
		{ 133, 151 }, // Speckle Black-Gold
		{ 134,  84 }, // Copper
		{ 135,  66 }, // Pearl Light Gray
		{ 137,  78 }, // Metal Blue
		{ 142,  61 }, // Pearl Light Gold
		{ 148,  77 }, // Pearl Dark Gray
		{ 150, 119 }, // Pearl Very Light Gray
		{ 151,  99 }, // Very Light Bluish Gray
		{ 178,  81 }, // Flat Dark Gold
		{ 179,  95 }, // Flat Silver
		{ 183,  83 }, // Pearl White
		{ 191, 110 }, // Bright Light Orange
		{ 212, 105 }, // Bright Light Blue
		{ 216,  27 }, // Rust
		{ 226, 103 }, // Bright Light Yellow
		{ 232,  87 }, // Sky Blue
		{ 256,  11 }, // Black
		{ 272,  63 }, // Dark Blue
		{ 273,   7 }, // Blue
		{ 284, 114 }, // Trans-Light Purple
		{ 288,  80 }, // Dark Green
		{ 294, 118 }, // Glow In Dark Trans
		{ 297, 115 }, // Pearl Gold
		{ 308, 120 }, // Dark Brown
		{ 313,  72 }, // Maersk Blue
		{ 320,  59 }, // Dark Red
		{ 321, 153 }, // Dark Azure
		{ 322, 156 }, // Medium Azure
		{ 323, 152 }, // Light Aqua
		{ 324,   5 }, // Red
		{ 326, 158 }, // Yellowish Green
		{ 330, 155 }, // Olive Green
		{ 334,  21 }, // Chrome Gold
		{ 335,  58 }, // Sand Red
		{ 350,   4 }, // Orange
		{ 351,  94 }, // Medium Dark Pink
		{ 366,  29 }, // Earth Orange
		{ 373,  54 }, // Sand Purple
		{ 375,   9 }, // Light Gray
		{ 378,  48 }, // Sand Green
		{ 379,  55 }, // Sand Blue
		{ 383,  22 }, // Chrome Silver
		{ 406,  63 }, // Dark Blue
		{ 449,  24 }, // Purple
		{ 450, 106 }, // Fabuland Brown
		{ 462,  31 }, // Medium Orange
		{ 484,  68 }, // Dark Orange
		{ 490,  34 }, // Lime
		{ 493, -10 }, // Magnet
		{ 494, -11 }, // Electric_Contact_Alloy
		{ 495, -12 }, // Electric_Contact_Copper
		{ 496,  86 }, // Light Bluish Gray
		{ 503,  49 }, // Very Light Gray
		{ 504,  95 }, // Flat Silver
		{ 511,   1 }, // White
		{ 10001, 159 }, // Glow In Dark White
		{ 10002, 160 }, // Fabuland Orange
		{ 10003, 161 }, // Dark Yellow
		{ 10004, 162 }, // Glitter Trans-Light Blue
		{ 10005, 163 }, // Glitter Trans-Neon Green
		{ 10006, 164 }, // Trans-Light Orange
		{ 10007, 165 }, // Neon Orange
		{ 10008, 220 }, // Coral
		{ 10009, 166 }, // Neon Green
		{ 10010, 221 }  // Trans-Light Green
	};

	int ColorCode = gColorList[ColorIndex].Code;

	for (unsigned int Color = 0; Color < LC_ARRAY_COUNT(BrickLinkColors); Color++)
		if (BrickLinkColors[Color].LDraw == ColorCode)
			return BrickLinkColors[Color].BrickLink;

	return 0;
}

bool lcLoadColorFile(lcFile& File, bool UpdateColors)
{
	char Line[1024], Token[1024];
	std::vector<lcColor>& Colors = gColorList;
	lcColor Color, MainColor, EdgeColor;

	if (UpdateColors)
	{
		EdgeColor = Colors[lcGetColorIndex(24)];
		for (auto EdgeColorIt = Colors.begin(); EdgeColorIt != Colors.end() ; EdgeColorIt++)
		{
			if (EdgeColorIt->Code == 24)
			{
				Colors.erase(EdgeColorIt);
				break;
			}
		}

		MainColor = Colors[lcGetColorIndex(16)];
		for (auto MainColorIt = Colors.begin(); MainColorIt != Colors.end() ; MainColorIt++)
		{
			if (MainColorIt->Code == 16)
			{
				Colors.erase(MainColorIt);
				break;
			}
		}

		std::vector<int>& SolidColors = gColorGroups[LC_COLORGROUP_SOLID].Colors;
		std::vector<int>::iterator DefaultColorIt = std::find(SolidColors.begin(), SolidColors.end(), gDefaultColor);
		if (DefaultColorIt != SolidColors.end())
			SolidColors.erase(DefaultColorIt);
	}
	else
	{
		Colors.clear();

		for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
			gColorGroups[GroupIdx].Colors.clear();

		gColorGroups[0].Name = QApplication::tr("Solid", "Colors");
		gColorGroups[1].Name = QApplication::tr("Translucent", "Colors");
		gColorGroups[2].Name = QApplication::tr("Special", "Colors");

		MainColor.Code = 16;
		MainColor.Translucent = false;
		MainColor.Value[0] = 1.0f;
		MainColor.Value[1] = 1.0f;
		MainColor.Value[2] = 0.5f;
		MainColor.Value[3] = 1.0f;
		MainColor.Edge[0] = 0.2f;
		MainColor.Edge[1] = 0.2f;
		MainColor.Edge[2] = 0.2f;
		MainColor.Edge[3] = 1.0f;
		strcpy(MainColor.Name, "Main Color");
		strcpy(MainColor.SafeName, "Main_Color");

		EdgeColor.Code = 24;
		EdgeColor.Translucent = false;
		EdgeColor.Value[0] = 0.5f;
		EdgeColor.Value[1] = 0.5f;
		EdgeColor.Value[2] = 0.5f;
		EdgeColor.Value[3] = 1.0f;
		EdgeColor.Edge[0] = 0.2f;
		EdgeColor.Edge[1] = 0.2f;
		EdgeColor.Edge[2] = 0.2f;
		EdgeColor.Edge[3] = 1.0f;
		strcpy(EdgeColor.Name, "Edge Color");
		strcpy(EdgeColor.SafeName, "Edge_Color");
	}

	while (File.ReadLine(Line, sizeof(Line)))
	{
		char* Ptr = Line;

		GetToken(Ptr, Token);
		if (strcmp(Token, "0"))
			continue;

		GetToken(Ptr, Token);
		strupr(Token);
		if (strcmp(Token, "!COLOUR"))
			continue;

		bool GroupTranslucent = false;
		bool GroupSpecial = false;

		Color.Code = ~0U;
		Color.Translucent = false;
		Color.Value[0] = FLT_MAX;
		Color.Value[1] = FLT_MAX;
		Color.Value[2] = FLT_MAX;
		Color.Value[3] = 1.0f;
		Color.Edge[0] = FLT_MAX;
		Color.Edge[1] = FLT_MAX;
		Color.Edge[2] = FLT_MAX;
		Color.Edge[3] = 1.0f;

		GetToken(Ptr, Token);
		strncpy(Color.Name, Token, sizeof(Color.Name));
		Color.Name[LC_MAX_COLOR_NAME - 1] = 0;
		strncpy(Color.SafeName, Color.Name, sizeof(Color.SafeName));

		for (char* Underscore = strchr((char*)Color.Name, '_'); Underscore; Underscore = strchr(Underscore, '_'))
			*Underscore = ' ';

		for (GetToken(Ptr, Token); Token[0]; GetToken(Ptr, Token))
		{
			strupr(Token);

			if (!strcmp(Token, "CODE"))
			{
				GetToken(Ptr, Token);
				Color.Code = atoi(Token);
			}
			else if (!strcmp(Token, "VALUE"))
			{
				GetToken(Ptr, Token);
				if (Token[0] == '#')
					Token[0] = ' ';

				int Value;
				if (sscanf(Token, "%x", &Value) != 1)
					Value = 0;

				Color.Value[2] = (float)(Value & 0xff) / 255.0f;
				Value >>= 8;
				Color.Value[1] = (float)(Value & 0xff) / 255.0f;
				Value >>= 8;
				Color.Value[0] = (float)(Value & 0xff) / 255.0f;
			}
			else if (!strcmp(Token, "EDGE"))
			{
				GetToken(Ptr, Token);
				if (Token[0] == '#')
					Token[0] = ' ';

				int Value;
				if (sscanf(Token, "%x", &Value) != 1)
					Value = 0;

				Color.Edge[2] = (float)(Value & 0xff) / 255.0f;
				Value >>= 8;
				Color.Edge[1] = (float)(Value & 0xff) / 255.0f;
				Value >>= 8;
				Color.Edge[0] = (float)(Value & 0xff) / 255.0f;
			}
			else if (!strcmp(Token, "ALPHA"))
			{
				GetToken(Ptr, Token);
				int Value = atoi(Token);
				Color.Value[3] = (float)(Value & 0xff) / 255.0f;
				if (Value != 255)
					Color.Translucent = true;

				if (Value == 128)
					GroupTranslucent = true;
				else if (Value != 0)
					GroupSpecial = true;
			}
			else if (!strcmp(Token, "CHROME") || !strcmp(Token, "PEARLESCENT") || !strcmp(Token, "RUBBER") ||
			         !strcmp(Token, "MATTE_METALIC") || !strcmp(Token, "METAL") || !strcmp(Token, "LUMINANCE"))
			{
				GroupSpecial = true;
			}
			else if (!strcmp(Token, "MATERIAL"))
			{
				GroupSpecial = true;
				break; // Material is always last so ignore it and the rest of the line.
			}
		}

		if (Color.Code == ~0U || Color.Value[0] == FLT_MAX)
			continue;

		if (Color.Edge[0] == FLT_MAX)
		{
			Color.Edge[0] = 33.0f / 255.0f;
			Color.Edge[1] = 33.0f / 255.0f;
			Color.Edge[2] = 33.0f / 255.0f;
		}

		bool ExistingColorUpdate = false;

		for (lcColor& ExistingColor : Colors)
		{
			if (ExistingColor.Code == Color.Code)
			{
				ExistingColor = Color;
				ExistingColorUpdate = true;
				break;
			}
		}

		if (ExistingColorUpdate)
			continue;

		if (Color.Code == 16)
		{
			MainColor = Color;
			continue;
		}

		if (Color.Code == 24)
		{
			EdgeColor = Color;
			continue;
		}

		Colors.push_back(Color);

		if (GroupSpecial)
			gColorGroups[LC_COLORGROUP_SPECIAL].Colors.push_back((int)Colors.size() - 1);
		else if (GroupTranslucent)
			gColorGroups[LC_COLORGROUP_TRANSLUCENT].Colors.push_back((int)Colors.size() - 1);
		else
			gColorGroups[LC_COLORGROUP_SOLID].Colors.push_back((int)Colors.size() - 1);
	}

	gDefaultColor = (int)Colors.size();

	Colors.push_back(MainColor);
	gColorGroups[LC_COLORGROUP_SOLID].Colors.push_back(gDefaultColor);

	gNumUserColors = (int)Colors.size();

	gEdgeColor = (int)Colors.size();
	Colors.push_back(EdgeColor);

	return Colors.size() > 2;
}

void lcLoadDefaultColors()
{
	lcDiskFile ConfigFile(":/resources/ldconfig.ldr");

	if (ConfigFile.Open(QIODevice::ReadOnly))
		lcLoadColorFile(ConfigFile);
}

int lcGetColorIndex(quint32 ColorCode)
{
	for (size_t ColorIdx = 0; ColorIdx < gColorList.size(); ColorIdx++)
		if (gColorList[ColorIdx].Code == ColorCode)
			return (int)ColorIdx;

	lcColor Color;

	Color.Code = ColorCode;
	Color.Translucent = false;
	Color.Edge[0] = 0.2f;
	Color.Edge[1] = 0.2f;
	Color.Edge[2] = 0.2f;
	Color.Edge[3] = 1.0f;

	if (ColorCode & LC_COLOR_DIRECT)
	{
		Color.Value[0] = (float)((ColorCode & 0xff0000) >> 16) / 255.0f;
		Color.Value[1] = (float)((ColorCode & 0x00ff00) >>  8) / 255.0f;
		Color.Value[2] = (float)((ColorCode & 0x0000ff) >>  0) / 255.0f;
		Color.Value[3] = 1.0f;
		sprintf(Color.Name, "Color %06X", ColorCode & 0xffffff);
		sprintf(Color.SafeName, "Color_%06X", ColorCode & 0xffffff);
	}
	else
	{
		Color.Value[0] = 0.5f;
		Color.Value[1] = 0.5f;
		Color.Value[2] = 0.5f;
		Color.Value[3] = 1.0f;
		sprintf(Color.Name, "Color %03d", ColorCode);
		sprintf(Color.SafeName, "Color_%03d", ColorCode);
	}

	gColorList.push_back(Color);
	return (int)gColorList.size() - 1;
}

void lcLoadLegoStyleDisplayColors()
{
	QByteArray ColorData;
	const char *ColorEntry = "0 !COLOUR   Lego_Style_Stud_Black    CODE 4242    VALUE #1B2A34    EDGE #000000\r\n";
	ColorData.append(ColorEntry);
	lcMemFile ColorMemFile;
	ColorMemFile.WriteBuffer(ColorData.constData(), ColorData.size());
	ColorMemFile.Seek(0, SEEK_SET);

	lcLoadColorFile(ColorMemFile, true/*Update*/);

	lcColor* DarkGrey = &gColorList[lcGetColorIndexByName("Lego Dark Gray Edge")];
	lcColor* Black = &gColorList[lcGetColorIndexByName("Lego Black Edge")];

	for (size_t ColorIdx = 0; ColorIdx < gColorList.size(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];
		if (Color->Code == 4242)
			continue;

		float r = 0.0f, g = 0.0f, b = 0.0f;
		float rr = Color->Value[0] * 255.0f,
				gg = Color->Value[1] * 255.0f,
				bb = Color->Value[2] * 255.0f;
		if (30 * rr + 59 * gg + 11 * bb <= 3600)
		{
			if (DarkGrey)
			{
				r = DarkGrey->Edge[0];
				g = DarkGrey->Edge[1];
				b = DarkGrey->Edge[2];
			}
			else
			{
				r = 27.0f;
				g = 42.0f;
				b = 52.0f;
			}
		}
		else
		{
			if (Color->Code == 0)
			{
				r = 255.0f;
				g = 255.0f;
				b = 255.0f;
			}
			else if (Black)
			{
				r = Black->Edge[0];
				g = Black->Edge[1];
				b = Black->Edge[2];
			}
		}

		Color->Edge[0] = r / 255.0f;
		Color->Edge[1] = g / 255.0f;
		Color->Edge[2] = b / 255.0f;
	}
}

int lcGetColorIndexByName(const char * ColorName)
{
	for (size_t ColorIdx = 0; ColorIdx < gColorList.size(); ColorIdx++)
		if (gColorList[ColorIdx].Name == ColorName)
			return (int)ColorIdx;
	return 0x0;
}

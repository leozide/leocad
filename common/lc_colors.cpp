#include "lc_global.h"
#include "lc_colors.h"
#include "lc_file.h"
#include <float.h>

lcArray<lcColor> gColorList;
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
	lcVector4(0.098f, 0.898f, 0.500f, 1.000f)  // LC_COLOR_HIGHLIGHT
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
		int Code;
		const char* Name;
	};

	lcBrickLinkEntry BrickLinkColors[] =
	{
		{  41, "Aqua" },
		{  11, "Black" },
		{   7, "Blue" },
		{  97, "Blue Violet" },                     // Blue-Violet
		{  36, "Bright Green" },
		{ 105, "Bright Light Blue" },
		{ 110, "Bright Light Orange" },
		{ 103, "Bright Light Yellow" },
		{ 104, "Bright Pink" },
		{   8, "Brown" },
		{ 153, "Dark Azure" },
		{  63, "Dark Blue" },
		{ 109, "Dark Blue Violet" },                // Dark Blue-Violet
		{  85, "Dark Bluish Gray" },
		{ 120, "Dark Brown" },
		{  91, "Dark Flesh" },
		{  10, "Dark Gray" },
		{  80, "Dark Green" },
		{  68, "Dark Orange" },
		{  47, "Dark Pink" },
		{  89, "Dark Purple" },
		{  59, "Dark Red" },
		{  69, "Dark Tan" },
		{  39, "Dark Turquoise" },
		{  29, "Earth Orange" },
		{ 106, "Fabuland Brown" },
		{ 160, "Fabuland Orange" },                 // No match
		{  28, "Flesh" },
		{   6, "Green" },
		{ 154, "Lavender" },
		{ 152, "Light Aqua" },
		{  62, "Light Blue" },
		{  86, "Light Bluish Gray" },
		{  90, "Light Flesh" },
		{   9, "Light Gray" },
		{  38, "Light Green" },
		{  35, "Light Lime" },
		{  32, "Light Orange" },
		{  56, "Light Pink" },
		{  93, "Light Purple" },
		{  26, "Light Salmon" },
		{  40, "Light Turquoise" },
		{  44, "Light Violet" },
		{  33, "Light Yellow" },
		{  34, "Lime" },
		{  72, "Maersk Blue" },
		{  71, "Magenta" },
		{ 156, "Medium Azure" },
		{  42, "Medium Blue" },
		{ 150, "Medium Dark Flesh" },
		{  94, "Medium Dark Pink" },
		{  37, "Medium Green" },
		{ 157, "Medium Lavender" },
		{  76, "Medium Lime" },
		{  31, "Medium Orange" },
		{  73, "Medium Violet" },
		{ 155, "Olive Green" },
		{   4, "Orange" },
		{  23, "Pink" },
		{  24, "Purple" },
		{   5, "Red" },
		{  88, "Reddish Brown" },
		{  27, "Rust" },
		{  25, "Salmon" },
		{  55, "Sand Blue" },
		{  48, "Sand Green" },
		{  54, "Sand Purple" },
		{  58, "Sand Red" },
		{  87, "Sky Blue" },
		{   2, "Tan" },
		{  99, "Very Light Bluish Gray" },
		{  49, "Very Light Gray" },
		{  96, "Very Light Orange" },
		{  43, "Violet" },
		{   1, "White" },
		{   3, "Yellow" },
		{ 158, "Yellowish Green" },
		{  13, "Trans Black" },                     // Trans-Black
		{ 108, "Trans Bright Green" },              // Trans-Bright Green
		{  12, "Trans Clear" },                     // Trans-Clear
		{  14, "Trans Dark Blue" },                 // Trans-Dark Blue
		{  50, "Trans Dark Pink" },                 // Trans-Dark Pink
		{  20, "Trans Green" },                     // Trans-Green
		{  15, "Trans Light Blue" },                // Trans-Light Blue
		{ 114, "Trans Light Purple" },              // Trans-Light Purple
		{  74, "Trans Medium Blue" },               // Trans-Medium Blue
		{  16, "Trans Neon Green" },                // Trans-Neon Green
		{  18, "Trans Neon Orange" },               // Trans-Neon Orange
		{ 121, "Trans Neon Yellow" },               // Trans-Neon Yellow
		{  98, "Trans Orange" },                    // Trans-Orange
		{ 107, "Trans Pink" },                      // Trans-Pink
		{  51, "Trans Purple" },                    // Trans-Purple
		{  17, "Trans Red" },                       // Trans-Red
		{ 113, "Trans Very Light Blue" },           // Trans-Very Lt Blue
		{  19, "Trans Yellow" },                    // Trans-Yellow
		{  57, "Chrome Antique Brass" },
		{ 122, "Chrome Black" },
		{  52, "Chrome Blue" },
		{  21, "Chrome Gold" },
		{  64, "Chrome Green" },
		{  82, "Chrome Pink" },
		{  22, "Chrome Silver" },
		{  84, "Copper" },
		{  81, "Flat Dark Gold" },
		{  95, "Flat Silver" },
		{  78, "Metal Blue" },
		{  77, "Pearl Dark Gray" },
		{ 115, "Pearl Gold" },
		{  61, "Pearl Light Gold" },
		{  66, "Pearl Light Gray" },
		{ 119, "Pearl Very Light Gray" },
		{  83, "Pearl White" },
		{  65, "Metallic Gold" },
		{  70, "Metallic Green" },
		{  67, "Metallic Silver" },
		{  46, "Glow In Dark Opaque" },
		{ 118, "Glow In Dark Trans" },
		{ 159, "Glow in Dark White" },              // No match
		{  60, "Milky White" },
		{ 101, "Glitter Trans Clear" },             // Glitter Trans-Clear
		{ 100, "Glitter Trans Dark Pink" },         // Glitter Trans-Dark Pink
		{ 102, "Glitter Trans Purple" },            // Glitter Trans-Purple
		{ 116, "Speckle Black Copper" },            // Speckle Black-Copper
		{ 151, "Speckle Black Gold" },              // Speckle Black-Gold
		{ 111, "Speckle Black Silver" },            // Speckle Black-Silver
		{ 117, "Speckle Dark Bluish Gray Silver" }, // Speckle DBGray-Silver
	};

	const char* Name = gColorList[ColorIndex].Name;

	for (unsigned int Color = 0; Color < sizeof(BrickLinkColors) / sizeof(BrickLinkColors[0]); Color++)
		if (!strcmp(Name, BrickLinkColors[Color].Name))
			return BrickLinkColors[Color].Code;

	return 0;
}

bool lcLoadColorFile(lcFile& File)
{
	char Line[1024], Token[1024];
	lcArray<lcColor>& Colors = gColorList;
	lcColor Color, MainColor, EdgeColor;

	Colors.RemoveAll();

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
		gColorGroups[GroupIdx].Colors.RemoveAll();

	gColorGroups[0].Name = QApplication::tr("Solid Colors", "Colors");
	gColorGroups[1].Name = QApplication::tr("Translucent Colors", "Colors");
	gColorGroups[2].Name = QApplication::tr("Special Colors", "Colors");

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

		bool Duplicate = false;

		for (int i = 0; i < Colors.GetSize(); i++)
		{
			if (Colors[i].Code == Color.Code)
			{
				Colors[i] = Color;
				Duplicate = true;
				break;
			}
		}

		if (Duplicate)
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

		Colors.Add(Color);

		if (GroupSpecial)
			gColorGroups[LC_COLORGROUP_SPECIAL].Colors.Add(Colors.GetSize() - 1);
		else if (GroupTranslucent)
			gColorGroups[LC_COLORGROUP_TRANSLUCENT].Colors.Add(Colors.GetSize() - 1);
		else
			gColorGroups[LC_COLORGROUP_SOLID].Colors.Add(Colors.GetSize() - 1);
	}

	gDefaultColor = Colors.GetSize();
	Colors.Add(MainColor);
	gColorGroups[LC_COLORGROUP_SOLID].Colors.Add(gDefaultColor);

	gNumUserColors = Colors.GetSize();

	gEdgeColor = Colors.GetSize();
	Colors.Add(EdgeColor);

	return Colors.GetSize() > 2;
}

void lcLoadDefaultColors()
{
	QResource Resource(":/resources/ldconfig.ldr");

	if (!Resource.isValid())
		return;

	QByteArray Data;

	if (Resource.isCompressed())
		Data = qUncompress(Resource.data(), Resource.size());
	else
		Data = QByteArray::fromRawData((const char*)Resource.data(), Resource.size());

	lcMemFile MemSettings;

	MemSettings.WriteBuffer(Data.constData(), Data.size());
	MemSettings.Seek(0, SEEK_SET);
	lcLoadColorFile(MemSettings);
}

int lcGetColorIndex(quint32 ColorCode)
{
	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
		if (gColorList[ColorIdx].Code == ColorCode)
			return ColorIdx;

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

	gColorList.Add(Color);
	return gColorList.GetSize() - 1;
}

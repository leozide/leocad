#include "lc_global.h"
#include "lc_colors.h"
#include "lc_file.h"
#include <float.h>

ObjArray<lcColor> gColorList;
lcColorGroup gColorGroups[LC_NUM_COLORGROUPS];
int gNumUserColors;
int gEdgeColor;
int gDefaultColor;

static const char sDefaultColorConfig[] =
{
	"0 LDraw.org Configuration File\n"
	"0 Name: LDConfig.ldr\n"
	"0 Author: LDraw.org\n"
	"0 !LDRAW_ORG Configuration UPDATE 2012-09-13\n"
	"\n"
	"0 // LDraw Solid Colours\n"
	"0 !COLOUR Black                                                 CODE   0   VALUE #05131D   EDGE #595959\n"
	"0 !COLOUR Blue                                                  CODE   1   VALUE #0055BF   EDGE #333333\n"
	"0 !COLOUR Green                                                 CODE   2   VALUE #257A3E   EDGE #333333\n"
	"0 !COLOUR Dark_Turquoise                                        CODE   3   VALUE #00838F   EDGE #333333\n"
	"0 !COLOUR Red                                                   CODE   4   VALUE #C91A09   EDGE #333333\n"
	"0 !COLOUR Dark_Pink                                             CODE   5   VALUE #C870A0   EDGE #333333\n"
	"0 !COLOUR Brown                                                 CODE   6   VALUE #583927   EDGE #1E1E1E\n"
	"0 !COLOUR Light_Gray                                            CODE   7   VALUE #9BA19D   EDGE #333333\n"
	"0 !COLOUR Dark_Gray                                             CODE   8   VALUE #6D6E5C   EDGE #333333\n"
	"0 !COLOUR Light_Blue                                            CODE   9   VALUE #B4D2E3   EDGE #333333\n"
	"0 !COLOUR Bright_Green                                          CODE  10   VALUE #4B9F4A   EDGE #333333\n"
	"0 !COLOUR Light_Turquoise                                       CODE  11   VALUE #55A5AF   EDGE #333333\n"
	"0 !COLOUR Salmon                                                CODE  12   VALUE #F2705E   EDGE #333333\n"
	"0 !COLOUR Pink                                                  CODE  13   VALUE #FC97AC   EDGE #333333\n"
	"0 !COLOUR Yellow                                                CODE  14   VALUE #F2CD37   EDGE #333333\n"
	"0 !COLOUR White                                                 CODE  15   VALUE #FFFFFF   EDGE #333333\n"
	"0 !COLOUR Light_Green                                           CODE  17   VALUE #C2DAB8   EDGE #333333\n"
	"0 !COLOUR Light_Yellow                                          CODE  18   VALUE #FBE696   EDGE #333333\n"
	"0 !COLOUR Tan                                                   CODE  19   VALUE #E4CD9E   EDGE #333333\n"
	"0 !COLOUR Light_Violet                                          CODE  20   VALUE #C9CAE2   EDGE #333333\n"
	"0 !COLOUR Purple                                                CODE  22   VALUE #81007B   EDGE #333333\n"
	"0 !COLOUR Dark_Blue_Violet                                      CODE  23   VALUE #2032B0   EDGE #1E1E1E\n"
	"0 !COLOUR Orange                                                CODE  25   VALUE #FE8A18   EDGE #333333\n"
	"0 !COLOUR Magenta                                               CODE  26   VALUE #923978   EDGE #333333\n"
	"0 !COLOUR Lime                                                  CODE  27   VALUE #BBE90B   EDGE #333333\n"
	"0 !COLOUR Dark_Tan                                              CODE  28   VALUE #958A73   EDGE #333333\n"
	"0 !COLOUR Bright_Pink                                           CODE  29   VALUE #E4ADC8   EDGE #333333\n"
	"0 !COLOUR Medium_Lavender                                       CODE  30   VALUE #AC78BA   EDGE #333333\n"
	"0 !COLOUR Lavender                                              CODE  31   VALUE #E1D5ED   EDGE #333333\n"
	"0 !COLOUR Very_Light_Orange                                     CODE  68   VALUE #F3CF9B   EDGE #333333\n"
	"0 !COLOUR Light_Purple                                          CODE  69   VALUE #CD6298   EDGE #333333\n"
	"0 !COLOUR Reddish_Brown                                         CODE  70   VALUE #582A12   EDGE #595959\n"
	"0 !COLOUR Light_Bluish_Gray                                     CODE  71   VALUE #A0A5A9   EDGE #333333\n"
	"0 !COLOUR Dark_Bluish_Gray                                      CODE  72   VALUE #6C6E68   EDGE #333333\n"
	"0 !COLOUR Medium_Blue                                           CODE  73   VALUE #5C9DD1   EDGE #333333\n"
	"0 !COLOUR Medium_Green                                          CODE  74   VALUE #73DCA1   EDGE #333333\n"
	"0 !COLOUR Light_Pink                                            CODE  77   VALUE #FECCCF   EDGE #333333\n"
	"0 !COLOUR Light_Flesh                                           CODE  78   VALUE #F6D7B3   EDGE #333333\n"
	"0 !COLOUR Medium_Dark_Flesh                                     CODE  84   VALUE #CC702A   EDGE #333333\n"
	"0 !COLOUR Dark_Purple                                           CODE  85   VALUE #3F3691   EDGE #1E1E1E\n"
	"0 !COLOUR Dark_Flesh                                            CODE  86   VALUE #7C503A   EDGE #333333\n"
	"0 !COLOUR Blue_Violet                                           CODE  89   VALUE #4C61DB   EDGE #333333\n"
	"0 !COLOUR Flesh                                                 CODE  92   VALUE #D09168   EDGE #333333\n"
	"0 !COLOUR Light_Salmon                                          CODE 100   VALUE #FEBABD   EDGE #333333\n"
	"0 !COLOUR Violet                                                CODE 110   VALUE #4354A3   EDGE #333333\n"
	"0 !COLOUR Medium_Violet                                         CODE 112   VALUE #6874CA   EDGE #333333\n"
	"0 !COLOUR Medium_Lime                                           CODE 115   VALUE #C7D23C   EDGE #333333\n"
	"0 !COLOUR Aqua                                                  CODE 118   VALUE #B3D7D1   EDGE #333333\n"
	"0 !COLOUR Light_Lime                                            CODE 120   VALUE #D9E4A7   EDGE #333333\n"
	"0 !COLOUR Light_Orange                                          CODE 125   VALUE #F9BA61   EDGE #333333\n"
	"0 !COLOUR Very_Light_Bluish_Gray                                CODE 151   VALUE #E6E3E0   EDGE #333333\n"
	"0 !COLOUR Bright_Light_Orange                                   CODE 191   VALUE #F8BB3D   EDGE #333333\n"
	"0 !COLOUR Bright_Light_Blue                                     CODE 212   VALUE #86C1E1   EDGE #333333\n"
	"0 !COLOUR Rust                                                  CODE 216   VALUE #B31004   EDGE #333333\n"
	"0 !COLOUR Bright_Light_Yellow                                   CODE 226   VALUE #FFF03A   EDGE #333333\n"
	"0 !COLOUR Sky_Blue                                              CODE 232   VALUE #56BED6   EDGE #333333\n"
	"0 !COLOUR Dark_Blue                                             CODE 272   VALUE #0D325B   EDGE #1E1E1E\n"
	"0 !COLOUR Dark_Green                                            CODE 288   VALUE #184632   EDGE #595959\n"
	"0 !COLOUR Dark_Brown                                            CODE 308   VALUE #352100   EDGE #595959\n"
	"0 !COLOUR Maersk_Blue                                           CODE 313   VALUE #54A9C8   EDGE #333333\n"
	"0 !COLOUR Dark_Red                                              CODE 320   VALUE #720E0F   EDGE #333333\n"
	"0 !COLOUR Dark_Azure                                            CODE 321   VALUE #1498D7   EDGE #333333\n"
	"0 !COLOUR Medium_Azure                                          CODE 322   VALUE #3EC2DD   EDGE #333333\n"
	"0 !COLOUR Light_Aqua                                            CODE 323   VALUE #BDDCD8   EDGE #333333\n"
	"0 !COLOUR Yellowish_Green                                       CODE 326   VALUE #DFEEA5   EDGE #333333\n"
	"0 !COLOUR Olive_Green                                           CODE 330   VALUE #9B9A5A   EDGE #333333\n"
	"0 !COLOUR Sand_Red                                              CODE 335   VALUE #D67572   EDGE #333333\n"
	"0 !COLOUR Medium_Dark_Pink                                      CODE 351   VALUE #F785B1   EDGE #333333\n"
	"0 !COLOUR Earth_Orange                                          CODE 366   VALUE #FA9C1C   EDGE #333333\n"
	"0 !COLOUR Sand_Purple                                           CODE 373   VALUE #845E84   EDGE #333333\n"
	"0 !COLOUR Sand_Green                                            CODE 378   VALUE #A0BCAC   EDGE #333333\n"
	"0 !COLOUR Sand_Blue                                             CODE 379   VALUE #597184   EDGE #333333\n"
	"0 !COLOUR Fabuland_Brown                                        CODE 450   VALUE #B67B50   EDGE #333333\n"
	"0 !COLOUR Medium_Orange                                         CODE 462   VALUE #FFA70B   EDGE #333333\n"
	"0 !COLOUR Dark_Orange                                           CODE 484   VALUE #A95500   EDGE #333333\n"
	"0 !COLOUR Very_Light_Gray                                       CODE 503   VALUE #E6E3DA   EDGE #333333\n"
	"\n"
	"0 // LDraw Transparent Colours\n"
	"0 !COLOUR Trans_Clear                                           CODE  47   VALUE #FCFCFC   EDGE #C3C3C3   ALPHA 128\n"
	"0 !COLOUR Trans_Black                                           CODE  40   VALUE #635F52   EDGE #171316   ALPHA 128\n"
	"0 !COLOUR Trans_Red                                             CODE  36   VALUE #C91A09   EDGE #880000   ALPHA 128\n"
	"0 !COLOUR Trans_Neon_Orange                                     CODE  38   VALUE #FF800D   EDGE #BD2400   ALPHA 128\n"
	"0 !COLOUR Trans_Orange                                          CODE  57   VALUE #F08F1C   EDGE #A45C28   ALPHA 128\n"
	"0 !COLOUR Trans_Neon_Yellow                                     CODE  54   VALUE #DAB000   EDGE #C3BA3F   ALPHA 128\n"
	"0 !COLOUR Trans_Yellow                                          CODE  46   VALUE #F5CD2F   EDGE #8E7400   ALPHA 128\n"
	"0 !COLOUR Trans_Neon_Green                                      CODE  42   VALUE #C0FF00   EDGE #84C300   ALPHA 128\n"
	"0 !COLOUR Trans_Bright_Green                                    CODE  35   VALUE #56E646   EDGE #9DA86B   ALPHA 128\n"
	"0 !COLOUR Trans_Green                                           CODE  34   VALUE #237841   EDGE #1E6239   ALPHA 128\n"
	"0 !COLOUR Trans_Dark_Blue                                       CODE  33   VALUE #0020A0   EDGE #000064   ALPHA 128\n"
	"0 !COLOUR Trans_Medium_Blue                                     CODE  41   VALUE #559AB7   EDGE #196973   ALPHA 128\n"
	"0 !COLOUR Trans_Light_Blue                                      CODE  43   VALUE #AEE9EF   EDGE #72B3B0   ALPHA 128\n"
	"0 !COLOUR Trans_Very_Light_Blue                                 CODE  39   VALUE #C1DFF0   EDGE #85A3B4   ALPHA 128\n"
	"0 !COLOUR Trans_Light_Purple                                    CODE  44   VALUE #96709F   EDGE #5A3463   ALPHA 128\n"
	"0 !COLOUR Trans_Purple                                          CODE  52   VALUE #A5A5CB   EDGE #280025   ALPHA 128\n"
	"0 !COLOUR Trans_Dark_Pink                                       CODE  37   VALUE #DF6695   EDGE #A32A59   ALPHA 128\n"
	"0 !COLOUR Trans_Pink                                            CODE  45   VALUE #FC97AC   EDGE #A8718C   ALPHA 128\n"
	"\n"
	"0 // LDraw Chrome Colours\n"
	"0 !COLOUR Chrome_Gold                                           CODE 334   VALUE #BBA53D   EDGE #BBB23D                               CHROME\n"
	"0 !COLOUR Chrome_Silver                                         CODE 383   VALUE #E0E0E0   EDGE #A4A4A4                               CHROME\n"
	"0 !COLOUR Chrome_Antique_Brass                                  CODE  60   VALUE #645A4C   EDGE #281E10                               CHROME\n"
	"0 !COLOUR Chrome_Black                                          CODE  64   VALUE #1B2A34   EDGE #595959                               CHROME\n"
	"0 !COLOUR Chrome_Blue                                           CODE  61   VALUE #6C96BF   EDGE #202A68                               CHROME\n"
	"0 !COLOUR Chrome_Green                                          CODE  62   VALUE #3CB371   EDGE #007735                               CHROME\n"
	"0 !COLOUR Chrome_Pink                                           CODE  63   VALUE #AA4D8E   EDGE #6E1152                               CHROME\n"
	"\n"
	"0 // LDraw Pearl Colours\n"
	"0 !COLOUR Pearl_White                                           CODE 183   VALUE #F2F3F2   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_Very_Light_Grey                                 CODE 150   VALUE #BBBDBC   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_Light_Gray                                      CODE 135   VALUE #9CA3A8   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Flat_Silver                                           CODE 179   VALUE #898788   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_Dark_Gray                                       CODE 148   VALUE #575857   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Metal_Blue                                            CODE 137   VALUE #5677BA   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_Light_Gold                                      CODE 142   VALUE #DCBE61   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_Gold                                            CODE 297   VALUE #CC9C2B   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Flat_Dark_Gold                                        CODE 178   VALUE #B4883E   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Copper                                                CODE 134   VALUE #964A27   EDGE #333333                               PEARLESCENT\n"
	"\n"
	"0 // LDraw Metallic Colours\n"
	"0 !COLOUR Metallic_Silver                                       CODE  80   VALUE #A5A9B4   EDGE #333333                               METAL\n"
	"0 !COLOUR Metallic_Green                                        CODE  81   VALUE #899B5F   EDGE #333333                               METAL\n"
	"0 !COLOUR Metallic_Gold                                         CODE  82   VALUE #8C5C20   EDGE #333333                               METAL\n"
	"0 !COLOUR Metallic_Black                                        CODE  83   VALUE #1A2831   EDGE #333333                               METAL\n"
	"0 !COLOUR Metallic_Dark_Gray                                    CODE  87   VALUE #6D6E5C   EDGE #333333                               METAL\n"
	"\n"
	"0 // LDraw Milky Colours\n"
	"0 !COLOUR Milky_White                                           CODE  79   VALUE #FFFFFF   EDGE #C3C3C3   ALPHA 224\n"
	"0 !COLOUR Glow_In_Dark_Opaque                                   CODE  21   VALUE #E0FFB0   EDGE #A4C374   ALPHA 250   LUMINANCE 15\n"
	"0 !COLOUR Glow_In_Dark_Trans                                    CODE 294   VALUE #BDC6AD   EDGE #818A71   ALPHA 250   LUMINANCE 15\n"
	"\n"
	"0 // LDraw Glitter Colours\n"
	"0 !COLOUR Glitter_Trans_Dark_Pink                               CODE 114   VALUE #DF6695   EDGE #9A2A66   ALPHA 128                   MATERIAL GLITTER VALUE #923978 FRACTION 0.17 VFRACTION 0.2 SIZE 1\n"
	"0 !COLOUR Glitter_Trans_Clear                                   CODE 117   VALUE #FFFFFF   EDGE #C3C3C3   ALPHA 128                   MATERIAL GLITTER VALUE #FFFFFF FRACTION 0.08 VFRACTION 0.1 SIZE 1\n"
	"0 !COLOUR Glitter_Trans_Purple                                  CODE 129   VALUE #640061   EDGE #280025   ALPHA 128                   MATERIAL GLITTER VALUE #8C00FF FRACTION 0.3 VFRACTION 0.4 SIZE 1\n"
	"\n"
	"0 // LDraw Speckle Colours\n"
	"0 !COLOUR Speckle_Black_Silver                                  CODE 132   VALUE #000000   EDGE #898788                               MATERIAL SPECKLE VALUE #898788 FRACTION 0.4 MINSIZE 1 MAXSIZE 3\n"
	"0 !COLOUR Speckle_Black_Gold                                    CODE 133   VALUE #000000   EDGE #DBAC34                               MATERIAL SPECKLE VALUE #DBAC34 FRACTION 0.4 MINSIZE 1 MAXSIZE 3\n"
	"0 !COLOUR Speckle_Black_Copper                                  CODE  75   VALUE #000000   EDGE #AB6038                               MATERIAL SPECKLE VALUE #AB6038 FRACTION 0.4 MINSIZE 1 MAXSIZE 3\n"
	"0 !COLOUR Speckle_Dark_Bluish_Gray_Silver                       CODE  76   VALUE #635F61   EDGE #898788                               MATERIAL SPECKLE VALUE #898788 FRACTION 0.4 MINSIZE 1 MAXSIZE 3\n"
	"\n"
	"0 // LDraw Rubber Colours\n"
	"0 !COLOUR Rubber_Yellow                                         CODE  65   VALUE #F5CD2F   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Trans_Yellow                                   CODE  66   VALUE #CAB000   EDGE #8E7400   ALPHA 128                   RUBBER\n"
	"0 !COLOUR Rubber_Trans_Clear                                    CODE  67   VALUE #FFFFFF   EDGE #C3C3C3   ALPHA 128                   RUBBER\n"
	"0 !COLOUR Rubber_Black                                          CODE 256   VALUE #212121   EDGE #595959                               RUBBER\n"
	"0 !COLOUR Rubber_Blue                                           CODE 273   VALUE #0033B2   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Red                                            CODE 324   VALUE #C40026   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Orange                                         CODE 350   VALUE #D06610   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Light_Gray                                     CODE 375   VALUE #C1C2C1   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Dark_Blue                                      CODE 406   VALUE #001D68   EDGE #595959                               RUBBER\n"
	"0 !COLOUR Rubber_Purple                                         CODE 449   VALUE #81007B   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Lime                                           CODE 490   VALUE #D7F000   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Light_Bluish_Gray                              CODE 496   VALUE #A3A2A4   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Flat_Silver                                    CODE 504   VALUE #898788   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_White                                          CODE 511   VALUE #FAFAFA   EDGE #333333                               RUBBER\n"
	"\n"
	"0 // LDraw Internal Common Material Colours\n"
	"0 !COLOUR Main_Colour                                           CODE  16   VALUE #7F7F7F   EDGE #333333\n"
	"0 !COLOUR Edge_Colour                                           CODE  24   VALUE #7F7F7F   EDGE #333333\n"
	"0 !COLOUR Trans_Black_IR_Lens                                   CODE  32   VALUE #000000   EDGE #333333   ALPHA 200\n"
	"0 !COLOUR Magnet                                                CODE 493   VALUE #656761   EDGE #595959                               METAL\n"
	"0 !COLOUR Electric_Contact_Alloy                                CODE 494   VALUE #D0D0D0   EDGE #333333                               METAL\n"
	"0 !COLOUR Electric_Contact_Copper                               CODE 495   VALUE #AE7A59   EDGE #333333                               METAL\n"
	"0\n"
};

static void GetToken(char*& Ptr, char* Token)
{
	while (*Ptr && *Ptr <= 32)
		Ptr++;

	while (*Ptr > 32)
		*Token++ = *Ptr++;

	*Token = 0;
}

bool lcLoadColorFile(lcFile& File)
{
	char Line[1024], Token[1024];
	ObjArray<lcColor>& Colors = gColorList;
	lcColor Color, MainColor, EdgeColor;

	Colors.RemoveAll();

	strcpy(gColorGroups[0].Name, "Solid Colors");
	strcpy(gColorGroups[1].Name, "Translucent Colors");
	strcpy(gColorGroups[2].Name, "Special Colors");

	MainColor.Code = 16;
	MainColor.Translucent = false;
	MainColor.Value[0] = 0.5f;
	MainColor.Value[1] = 0.5f;
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

		Color.Code = -1;
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

		for (char* Ptr = strchr((char*)Color.Name, '_'); Ptr; Ptr = strchr(Ptr, '_'))
			*Ptr = ' ';

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
				sscanf(Token, "%x", &Value);

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
				sscanf(Token, "%x", &Value);

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

		// Check if the new color is valid.
		if (Color.Code == (lcuint32)-1 || Color.Value[0] == FLT_MAX)
			continue;

		if (Color.Edge[0] == FLT_MAX)
		{
			Color.Edge[0] = 33.0f / 255.0f;
			Color.Edge[1] = 33.0f / 255.0f;
			Color.Edge[2] = 33.0f / 255.0f;
		}

		// Check for duplicates.
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
	lcMemFile File;
	File.WriteBuffer(sDefaultColorConfig, sizeof(sDefaultColorConfig));
	File.Seek(0, SEEK_SET);
	lcLoadColorFile(File);
}

int lcGetColorIndex(lcuint32 ColorCode)
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

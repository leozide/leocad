#include "lc_global.h"
#include "lc_colors.h"

lcColor g_ColorList[] =
{
	{   0, { 0.1294f, 0.1294f, 0.1294f, 1.0000f }, false, "Black" },
	{   1, { 0.0000f, 0.2000f, 0.6980f, 1.0000f }, false, "Blue" },
	{   2, { 0.0000f, 0.5490f, 0.0784f, 1.0000f }, false, "Green" },
	{   3, { 0.0000f, 0.6000f, 0.6235f, 1.0000f }, false, "Teal" },
	{   4, { 0.7686f, 0.0000f, 0.1490f, 1.0000f }, false, "Red" },
	{   5, { 0.8745f, 0.4000f, 0.5843f, 1.0000f }, false, "Dark Pink" },
	{   6, { 0.3608f, 0.1255f, 0.0000f, 1.0000f }, false, "Brown" },
	{   7, { 0.7569f, 0.7608f, 0.7569f, 1.0000f }, false, "Gray" },
	{   8, { 0.3882f, 0.3725f, 0.3216f, 1.0000f }, false, "Dark Gray" },
	{   9, { 0.4196f, 0.6706f, 0.8627f, 1.0000f }, false, "Light Blue" },
	{  10, { 0.4196f, 0.9333f, 0.5647f, 1.0000f }, false, "Bright Green" },
	{  11, { 0.2000f, 0.6510f, 0.6549f, 1.0000f }, false, "Turquiose" },
	{  12, { 1.0000f, 0.5216f, 0.4784f, 1.0000f }, false, "Light Red" },
	{  13, { 0.9765f, 0.6431f, 0.7765f, 1.0000f }, false, "Pink" },
	{  14, { 1.0000f, 0.8627f, 0.0000f, 1.0000f }, false, "Yellow" },
	{  15, { 1.0000f, 1.0000f, 1.0000f, 1.0000f }, false, "White" },
	{  17, { 0.7294f, 1.0000f, 0.8078f, 1.0000f }, false, "Light Green" },
	{  18, { 0.9922f, 0.9098f, 0.5882f, 1.0000f }, false, "Light Yellow" },
	{  19, { 0.9098f, 0.8118f, 0.6314f, 1.0000f }, false, "Tan" },
	{  20, { 0.8431f, 0.7686f, 0.9020f, 1.0000f }, false, "Light Violet" },
	{  21, { 0.8784f, 1.0000f, 0.6902f, 0.9804f }, false, "Phosphor White" },
	{  22, { 0.5059f, 0.0000f, 0.4824f, 1.0000f }, false, "Violet" },
	{  23, { 0.2784f, 0.1961f, 0.6902f, 1.0000f }, false, "Violet Blue" },
	{  25, { 0.9765f, 0.3765f, 0.0000f, 1.0000f }, false, "Orange" },
	{  26, { 0.8471f, 0.1059f, 0.4275f, 1.0000f }, false, "Magenta" },
	{  27, { 0.8431f, 0.9412f, 0.0000f, 1.0000f }, false, "Lime" },
	{  28, { 0.7725f, 0.5922f, 0.3137f, 1.0000f }, false, "Dark Tan" },
	{  33, { 0.0000f, 0.1255f, 0.6275f, 0.5020f },  true, "Trans Blue" },
	{  34, { 0.0235f, 0.3921f, 0.1961f, 0.5020f },  true, "Trans Green" },
	{  36, { 0.7686f, 0.0000f, 0.1490f, 0.5020f },  true, "Trans Red" },
	{  37, { 0.3922f, 0.0000f, 0.3804f, 0.5020f },  true, "Trans Violet" },
	{  40, { 0.3882f, 0.3725f, 0.3216f, 0.5020f },  true, "Trans Gray" },
	{  41, { 0.6824f, 0.9373f, 0.9255f, 0.5020f },  true, "Trans Light Cyan" },
	{  42, { 0.7529f, 1.0000f, 0.0000f, 0.5020f },  true, "Trans Flu Lime" },
	{  45, { 0.8745f, 0.4000f, 0.5843f, 0.5020f },  true, "Trans Pink" },
	{  46, { 0.7922f, 0.6902f, 0.0000f, 0.5020f },  true, "Trans Yellow" },
	{  47, { 1.0000f, 1.0000f, 1.0000f, 0.5020f },  true, "Clear" },
	{  57, { 0.9765f, 0.3765f, 0.0000f, 0.5020f },  true, "Trans Flu Orange" },
	{  70, { 0.4118f, 0.2510f, 0.1529f, 1.0000f }, false, "Reddish Brown" },
	{  71, { 0.6392f, 0.6353f, 0.6431f, 1.0000f }, false, "Stone Gray" },
	{  72, { 0.3882f, 0.3725f, 0.3804f, 1.0000f }, false, "Dark Stone Gray" },
	{ 134, { 0.5765f, 0.5294f, 0.4039f, 1.0000f }, false, "Pearl Copper" },
	{ 135, { 0.6706f, 0.6784f, 0.6745f, 1.0000f }, false, "Pearl Gray" },
	{ 137, { 0.4157f, 0.4784f, 0.5882f, 1.0000f }, false, "Pearl Sand Blue" },
	{ 142, { 0.8431f, 0.6627f, 0.2941f, 1.0000f }, false, "Pearl Gold" },
	{ 256, { 0.1294f, 0.1294f, 0.1294f, 1.0000f }, false, "Rubber Black" },
	{ 272, { 0.0000f, 0.1137f, 0.4078f, 1.0000f }, false, "Dark Blue" },
	{ 273, { 0.0000f, 0.2000f, 0.6980f, 1.0000f }, false, "Rubber Blue" },
	{ 288, { 0.1529f, 0.2745f, 0.1725f, 1.0000f }, false, "Dark Green" },
	{ 320, { 0.4706f, 0.0000f, 0.1098f, 1.0000f }, false, "Dark Red" },
	{ 324, { 0.7686f, 0.0000f, 0.1490f, 1.0000f }, false, "Rubber Red" },
	{ 334, { 0.8824f, 0.4314f, 0.0745f, 1.0000f }, false, "Chrome Gold" },
	{ 335, { 0.7490f, 0.5294f, 0.5098f, 1.0000f }, false, "Sand Red" },
	{ 366, { 0.8196f, 0.5137f, 0.0157f, 1.0000f }, false, "Earth Orange" },
	{ 373, { 0.5176f, 0.3686f, 0.5176f, 1.0000f }, false, "Sand Violet" },
	{ 375, { 0.7569f, 0.7608f, 0.7569f, 1.0000f }, false, "Rubber Gray" },
	{ 378, { 0.6275f, 0.7373f, 0.6745f, 1.0000f }, false, "Sand Green" },
	{ 379, { 0.4157f, 0.4784f, 0.5882f, 1.0000f }, false, "Sand Blue" },
	{ 383, { 0.8784f, 0.8784f, 0.8784f, 1.0000f }, false, "Chrome Silver" },
	{ 462, { 0.9961f, 0.6235f, 0.0235f, 1.0000f }, false, "Light Orange" },
	{ 484, { 0.7020f, 0.2431f, 0.0000f, 1.0000f }, false, "Dark Orange" },
	{ 494, { 0.8157f, 0.8157f, 0.8157f, 1.0000f }, false, "Electric Contact" },
	{ 503, { 0.9020f, 0.8902f, 0.8549f, 1.0000f }, false, "Light Gray" },
	{ 511, { 1.0000f, 1.0000f, 1.0000f, 1.0000f }, false, "Rubber White" },

	{  29, { 0.8941f, 0.6784f, 0.7843f, 1.0000f }, false, "Light Purple" },
	{  69, { 0.8039f, 0.3843f, 0.5961f, 1.0000f }, false, "Bright Purple" },
	{  73, { 0.4314f, 0.6000f, 0.7882f, 1.0000f }, false, "Medium Blue" },
	{  74, { 0.6314f, 0.7686f, 0.5451f, 1.0000f }, false, "Medium Green" },
	{  77, { 0.9961f, 0.8000f, 0.8000f, 1.0000f }, false, "Paradisa Pink" },
	{  78, { 0.9804f, 0.8431f, 0.7647f, 1.0000f }, false, "Light Flesh" },
	{  79, { 1.0000f, 1.0000f, 1.0000f, 0.8784f },  true, "Translucent White" },
	{  85, { 0.2039f, 0.1686f, 0.4588f, 1.0000f }, false, "Medium Lilac" },
	{  86, { 0.4863f, 0.3608f, 0.2706f, 1.0000f }, false, "Dark Flesh" },
	{  89, { 0.6078f, 0.6980f, 0.9373f, 1.0000f }, false, "Royal Blue" },
	{  92, { 0.8000f, 0.5569f, 0.4078f, 1.0000f }, false, "Flesh" },
	{ 151, { 0.8980f, 0.8941f, 0.8706f, 1.0000f }, false, "Light Stone" },
	{ 313, { 0.2078f, 0.6353f, 0.7412f, 1.0000f }, false, "Maersk Blue" },

	{  16, { 0.5000f, 0.5000f, 0.5000f, 1.0000f }, false, "Default" },
	{  24, { 0.2000f, 0.2000f, 0.2000f, 1.0000f }, false, "Edge" },
	{  -1, { 0.8980f, 0.2980f, 0.4000f, 1.0000f }, false, "Selected" },
	{  -2, { 0.4000f, 0.2980f, 0.8980f, 1.0000f }, false, "Focused" },
};

u32 lcNumUserColors = sizeof(g_ColorList)/sizeof(g_ColorList[0]) - 3;
u32 lcNumColors = sizeof(g_ColorList)/sizeof(g_ColorList[0]);


/*
0 LDraw.org Configuration File
0 Name: ldconfig.ldr
0 Author: LDraw.org
0 LDRAW_ORG Configuration UPDATE 2004-03

0 !COLOUR Black             CODE   0  VALUE #212121  EDGE 8
0 !COLOUR Blue              CODE   1  VALUE #0033B2  EDGE 0 
0 !COLOUR Green             CODE   2  VALUE #008C14  EDGE 0 
0 !COLOUR Teal              CODE   3  VALUE #00999F  EDGE 0
0 !COLOUR Red               CODE   4  VALUE #C40026  EDGE 0
0 !COLOUR Dark_Pink         CODE   5  VALUE #DF6695  EDGE 0
0 !COLOUR Brown             CODE   6  VALUE #5C2000  EDGE 8
0 !COLOUR Gray              CODE   7  VALUE #C1C2C1  EDGE 0
0 !COLOUR Dark_Gray         CODE   8  VALUE #635F52  EDGE 0
0 !COLOUR Light_Blue        CODE   9  VALUE #6BABDC  EDGE 0
0 !COLOUR Bright_Green      CODE  10  VALUE #6BEE90  EDGE 0
0 !COLOUR Turquiose         CODE  11  VALUE #33A6A7  EDGE 0
0 !COLOUR Light_Red         CODE  12  VALUE #FF857A  EDGE 4
0 !COLOUR Pink              CODE  13  VALUE #F9A4C6  EDGE 0
0 !COLOUR Yellow            CODE  14  VALUE #FFDC00  EDGE 0
0 !COLOUR White             CODE  15  VALUE #FFFFFF  EDGE 0
0 !COLOUR Light_Green       CODE  17  VALUE #BAFFCE  EDGE 0
0 !COLOUR Light_Yellow      CODE  18  VALUE #FDE896  EDGE 0
0 !COLOUR Tan               CODE  19  VALUE #E8CFA1  EDGE 6
0 !COLOUR Light_Violet      CODE  20  VALUE #D7C4E6  EDGE 0
0 !COLOUR Phosphor_White    CODE  21  VALUE #E0FFB0  EDGE #77CC00  ALPHA 250  LUMINANCE 15
0 !COLOUR Violet            CODE  22  VALUE #81007B  EDGE 0
0 !COLOUR Violet_Blue       CODE  23  VALUE #4732B0  EDGE 0
0 !COLOUR Orange            CODE  25  VALUE #F96000  EDGE #000000
0 !COLOUR Magenta           CODE  26  VALUE #D81B6D  EDGE #000000
0 !COLOUR Lime              CODE  27  VALUE #D7F000  EDGE 0
0 !COLOUR Dark_Tan          CODE  28  VALUE #C59750  EDGE 6
0 !COLOUR Trans_Blue        CODE  33  VALUE #0020A0  EDGE #002266  ALPHA 128
0 !COLOUR Trans_Green       CODE  34  VALUE #066432  EDGE #004422  ALPHA 128
0 !COLOUR Trans_Red         CODE  36  VALUE #C40026  EDGE #660011  ALPHA 128
0 !COLOUR Trans_Violet      CODE  37  VALUE #640061  EDGE 0        ALPHA 128
0 !COLOUR Trans_Gray        CODE  40  VALUE #635F52  EDGE 0        ALPHA 128
0 !COLOUR Trans_Light_Cyan  CODE  41  VALUE #AEEFEC  EDGE 0        ALPHA 128
0 !COLOUR Trans_Flu_Lime    CODE  42  VALUE #C0FF00  EDGE 0        ALPHA 128
0 !COLOUR Trans_Pink        CODE  45  VALUE #DF6695  EDGE 0        ALPHA 128
0 !COLOUR Trans_Yellow      CODE  46  VALUE #CAB000  EDGE 0        ALPHA 128
0 !COLOUR Clear             CODE  47  VALUE #FFFFFF  EDGE 0        ALPHA 128
0 !COLOUR Trans_Flu_Orange  CODE  57  VALUE #F96000  EDGE 0        ALPHA 128
0 !COLOUR Reddish_Brown     CODE  70  VALUE #694027  EDGE 8
0 !COLOUR Stone_Gray        CODE  71  VALUE #A3A2A4  EDGE 0
0 !COLOUR Dark_Stone_Gray   CODE  72  VALUE #635F61  EDGE 0
0 !COLOUR Pearl_Copper      CODE 134  VALUE #938767  EDGE 6        PEARLESCENT
0 !COLOUR Pearl_Gray        CODE 135  VALUE #ABADAC  EDGE 8        PEARLESCENT
0 !COLOUR Pearl_Gold        CODE 142  VALUE #D7A94B  EDGE 12       PEARLESCENT
0 !COLOUR Pearl_Sand_Blue   CODE 137  VALUE #6A7A96  EDGE 1        PEARLESCENT
0 !COLOUR Rubber_Black      CODE 256  VALUE #212121  EDGE #000000  RUBBER
0 !COLOUR Dark_Blue         CODE 272  VALUE #001D68  EDGE #000000
0 !COLOUR Rubber_Blue       CODE 273  VALUE #0033B2  EDGE 0        RUBBER
0 !COLOUR Dark_Green        CODE 288  VALUE #27462C  EDGE #000000
0 !COLOUR Dark_Red          CODE 320  VALUE #78001C  EDGE #000000
0 !COLOUR Rubber_Red        CODE 324  VALUE #C40026  EDGE 0        RUBBER
0 !COLOUR Chrome_Gold       CODE 334  VALUE #E16E13  EDGE 14       CHROME
0 !COLOUR Sand_Red          CODE 335  VALUE #BF8782  EDGE 0
0 !COLOUR Earth_Orange      CODE 366  VALUE #D18304  EDGE 0
0 !COLOUR Sand_Violet       CODE 373  VALUE #845E84  EDGE 0
0 !COLOUR Rubber_Gray       CODE 375  VALUE #C1C2C1  EDGE 8        RUBBER
0 !COLOUR Sand_Green        CODE 378  VALUE #A0BCAC  EDGE 0
0 !COLOUR Sand_Blue         CODE 379  VALUE #6A7A96  EDGE 0
0 !COLOUR Chrome_Silver     CODE 383  VALUE #E0E0E0  EDGE 8        CHROME
0 !COLOUR Light_Orange      CODE 462  VALUE #FE9F06  EDGE 0
0 !COLOUR Dark_Orange       CODE 484  VALUE #B33E00  EDGE 0
0 !COLOUR Electric_Contact  CODE 494  VALUE #D0D0D0  EDGE 8
0 !COLOUR Light_Gray        CODE 503  VALUE #E6E3DA  EDGE 0
0 !COLOUR Rubber_White      CODE 511  VALUE #FFFFFF  EDGE 0        RUBBER
0

0 ---------------------------------------------------------
0 !COLOUR Light_Purple      CODE  29  VALUE #E4ADC8  EDGE 0
0 !COLOUR Bright_Purple     CODE  69  VALUE #CD6298  EDGE 0
0 !COLOUR Medium_Blue       CODE  73  VALUE #6E99C9  EDGE 0
0 !COLOUR Medium_Green      CODE  74  VALUE #A1C48B  EDGE 0
0 !COLOUR Paradisa_Pink     CODE  77  VALUE #FECCCC  EDGE 0
0 !COLOUR Light_Flesh       CODE  78  VALUE #FAD7C3  EDGE 8
0 !COLOUR Translucent_White CODE  79  VALUE #FFFFFF  EDGE 8 ALPHA 224
0 !COLOUR Medium_Lilac      CODE  85  VALUE #342B75  EDGE 0
0 !COLOUR Dark_Flesh        CODE  86  VALUE #7C5C45  EDGE 0
0 !COLOUR Royal_Blue        CODE  89  VALUE #9BB2EF  EDGE 0
0 !COLOUR Flesh             CODE  92  VALUE #CC8E68  EDGE 0
0 !COLOUR Light_Stone       CODE 151  VALUE #E5E4DE  EDGE 8
0 !COLOUR Maersk_Blue       CODE 313  VALUE #35A2BD  EDGE 0
0
*/

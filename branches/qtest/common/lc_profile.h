#ifndef LC_PROFILE_H
#define LC_PROFILE_H

enum LC_PROFILE_KEY
{
	// Settings.
	LC_PROFILE_DETAIL, // "Default", "Detail", LC_DET_BRICKEDGES
	LC_PROFILE_SNAP, // "Default", "Snap", LC_DRAW_SNAP_A | LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y | LC_DRAW_SNAP_Z);
	LC_PROFILE_ANGLE_SNAP,//	m_nAngleSnap = (unsigned short)lcGetProfileValue("Default", "Angle", 30);
	LC_PROFILE_LINE_WIDTH,//	m_fLineWidth = (float)lcGetProfileValue("Default", "Line", 100)/100;
	LC_PROFILE_GRID_SIZE,//	m_nGridSize = (unsigned short)lcGetProfileValue("Default", "Grid", 20);
	LC_PROFILE_ANTIALIASING_SAMPLES,//int CurrentAASamples = lcGetProfileValue("Default", "AASamples", 1);

	LC_PROFILE_CHECK_UPDATES,
	LC_PROFILE_PROJECTS_PATH, //strcpy(SaveFileName, lcGetProfileValue("Default", "Projects", ""));
	LC_PROFILE_PARTS_LIBRARY, //const char* CustomPath = lcGetProfileValue("Settings", "CustomPiecesLibrary", "");
	LC_PROFILE_SHORTCUTS_FILE, //const char* FileName = lcGetProfileValue("Settings", "Keyboard", "");
	LC_PROFILE_CATEGORIES_FILE, //strcpy(FileName, lcGetProfileValue("Settings", "Categories", ""));
	LC_PROFILE_RECENT_FILE1, //strcpy(mRecentFiles[FileIdx], lcGetProfileValue("RecentFiles", KeyName, ""));
	LC_PROFILE_RECENT_FILE2, //strcpy(mRecentFiles[FileIdx], lcGetProfileValue("RecentFiles", KeyName, ""));
	LC_PROFILE_RECENT_FILE3, //strcpy(mRecentFiles[FileIdx], lcGetProfileValue("RecentFiles", KeyName, ""));
	LC_PROFILE_RECENT_FILE4, //strcpy(mRecentFiles[FileIdx], lcGetProfileValue("RecentFiles", KeyName, ""));
	LC_PROFILE_AUTOSAVE_INTERVAL, // "Settings", "Autosave", 10
	LC_PROFILE_MOUSE_SENSITIVITY, // "Default", "Mouse", 11
	LC_PROFILE_IMAGE_WIDTH, //int ImageWidth = lcGetProfileValue("Default", "Image Width", 640); 1280
	LC_PROFILE_IMAGE_HEIGHT,//int ImageHeight = lcGetProfileValue("Default", "Image Height", 480); 720
	LC_PROFILE_IMAGE_OPTIONS,//unsigned long image = lcGetProfileValue("Default", "Image Options", 1|LC_IMAGE_TRANSPARENT);

	// Defaults for new projects.
	LC_PROFILE_DEFAULT_AUTHOR_NAME,
	LC_PROFILE_DEFAULT_SCENE,//m_nScene = lcGetProfileValue("Default", "Scene", 0);
	LC_PROFILE_DEFAULT_FLOOR_COLOR, // "Default", "Floor", RGB (0,191,0)
	LC_PROFILE_DEFAULT_FLOOR_TEXTURE, // "Default", "FloorBMP", ""
	LC_PROFILE_DEFAULT_FOG_DENSITY, //("Default", "Density", 10)/100;
	LC_PROFILE_DEFAULT_FOG_COLOR, //rgb = lcGetProfileValue("Default", "Fog", 0xFFFFFF);
	LC_PROFILE_DEFAULT_AMBIENT_COLOR, //rgb = lcGetProfileValue("Default", "Ambient", 0x4B4B4B);
	LC_PROFILE_DEFAULT_BACKGROUND_COLOR,//rgb = lcGetProfileValue("Default", "Background", 0xFFFFFF);
	LC_PROFILE_DEFAULT_GRADIENT_COLOR1,//rgb = lcGetProfileValue("Default", "Gradient1", 0xBF0000);
	LC_PROFILE_DEFAULT_GRADIENT_COLOR2,//rgb = lcGetProfileValue("Default", "Gradient2", 0xFFFFFF);
	LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE,//strcpy(m_strBackground, lcGetProfileValue("Default", "BMP", ""));

	LC_PROFILE_HTML_OPTIONS,//int HTMLOptions = lcGetProfileValue("Default", "HTML Options", LC_HTML_SINGLEPAGE);
	LC_PROFILE_HTML_IMAGE_OPTIONS,//int ImageOptions = lcGetProfileValue("Default", "HTML Image Options", LC_IMAGE_PNG | LC_IMAGE_TRANSPARENT);
	LC_PROFILE_HTML_IMAGE_WIDTH, // Options.StepImagesWidth = lcGetProfileValue("Default", "HTML Image Width", 640);
	LC_PROFILE_HTML_IMAGE_HEIGHT, //Options.StepImagesHeight = lcGetProfileValue("Default", "HTML Image Height", 480);
	LC_PROFILE_HTML_PARTS_COLOR, //Options.PartImagesColor = lcGetColorIndex(lcGetProfileValue("Default", "HTML Piece Color", 16));
	LC_PROFILE_HTML_PARTS_WIDTH, //Options.PartImagesWidth = lcGetProfileValue("Default", "HTML Parts Width", 128);
	LC_PROFILE_HTML_PARTS_HEIGHT, //Options.PartImagesHeight = lcGetProfileValue("Default", "HTML Parts Height", 128);
	LC_PROFILE_POVRAY_PATH, //strcpy(Options.POVRayPath, lcGetProfileValue("Settings", "POV-Ray", ""));
	LC_PROFILE_POVRAY_LGEO_PATH, //strcpy(Options.LGEOPath, lcGetProfileValue("Settings", "LGEO", ""));
	LC_PROFILE_POVRAY_RENDER, //Options.Render = lcGetProfileValue("Settings", "POV Render", 1);

	LC_NUM_PROFILE_KEYS
};

enum LC_PROFILE_ENTRY_TYPE
{
	LC_PROFILE_ENTRY_INT,
	LC_PROFILE_ENTRY_FLOAT,
	LC_PROFILE_ENTRY_STRING
};

class lcProfileEntry
{
public:
	lcProfileEntry(const char* Section, const char* Key, int DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, unsigned int DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, float DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, const char* DefaultValue);

	LC_PROFILE_ENTRY_TYPE mType;

	const char* mSection;
	const char* mKey;

	union
	{
		int IntValue;
		float FloatValue;
		const char* StringValue;
	} mDefault;
};

extern lcProfileEntry gProfileEntries[LC_NUM_PROFILE_KEYS];

int lcGetProfileInt(LC_PROFILE_KEY Key);
float lcGetProfileFloat(LC_PROFILE_KEY Key);
const char* lcGetProfileString(LC_PROFILE_KEY Key);

void lcSetProfileInt(LC_PROFILE_KEY Key, int Value);
void lcSetProfileFloat(LC_PROFILE_KEY Key, float Value);
void lcSetProfileString(LC_PROFILE_KEY Key, const char* Value);

#endif // LC_PROFILE_H

#include "lc_global.h"
#include "lc_profile.h"
#include "lc_context.h"
#include "image.h"
#include "lc_model.h"
#include "project.h"
#include "lc_viewsphere.h"

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key, int DefaultValue)
{
	mType = LC_PROFILE_ENTRY_INT;
	mSection = Section;
	mKey = Key;
	mDefault.IntValue = DefaultValue;
}

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key, unsigned int DefaultValue)
{
	mType = LC_PROFILE_ENTRY_INT;
	mSection = Section;
	mKey = Key;
	mDefault.IntValue = DefaultValue;
}

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key, float DefaultValue)
{
	mType = LC_PROFILE_ENTRY_FLOAT;
	mSection = Section;
	mKey = Key;
	mDefault.FloatValue = DefaultValue;
}

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key, const char* DefaultValue)
{
	mType = LC_PROFILE_ENTRY_STRING;
	mSection = Section;
	mKey = Key;
	mDefault.StringValue = DefaultValue;
}

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key, const QStringList& /*StringList*/)
{
	mType = LC_PROFILE_ENTRY_STRINGLIST;
	mSection = Section;
	mKey = Key;
	mDefault.IntValue = 0;
}

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key)
{
	mType = LC_PROFILE_ENTRY_BUFFER;
	mSection = Section;
	mKey = Key;
	mDefault.IntValue = 0;
}

static lcProfileEntry gProfileEntries[LC_NUM_PROFILE_KEYS] =
{
	lcProfileEntry("Settings", "FixedAxes", false),                                            // LC_PROFILE_FIXED_AXES
	lcProfileEntry("Settings", "LineWidth", 1.0f),                                             // LC_PROFILE_LINE_WIDTH
	lcProfileEntry("Settings", "AllowLOD", true),                                              // LC_PROFILE_ALLOW_LOD
	lcProfileEntry("Settings", "LODDistance", 750.0f),                                         // LC_PROFILE_LOD_DISTANCE
	lcProfileEntry("Settings", "FadeSteps", false),                                            // LC_PROFILE_FADE_STEPS
	lcProfileEntry("Settings", "FadeStepsColor", LC_RGBA(128, 128, 128, 128)),                 // LC_PROFILE_FADE_STEPS_COLOR
	lcProfileEntry("Settings", "HighlightNewParts", 0),                                        // LC_PROFILE_HIGHLIGHT_NEW_PARTS
	lcProfileEntry("Settings", "HighlightNewPartsColor", LC_RGBA(255, 242, 0, 192)),           // LC_PROFILE_HIGHLIGHT_NEW_PARTS_COLOR
	lcProfileEntry("Settings", "ShadingMode", static_cast<int>(lcShadingMode::DefaultLights)), // LC_PROFILE_SHADING_MODE
	lcProfileEntry("Settings", "BackgroundGradient", false),                                   // LC_PROFILE_BACKGROUND_GRADIENT
	lcProfileEntry("Settings", "BackgroundColor", LC_RGB(49, 52, 55)),                         // LC_PROFILE_BACKGROUND_COLOR
	lcProfileEntry("Settings", "GradientColorTop", LC_RGB(54, 72, 95)),                        // LC_PROFILE_GRADIENT_COLOR_TOP
	lcProfileEntry("Settings", "GradientColorBottom", LC_RGB(49, 52, 55)),                     // LC_PROFILE_GRADIENT_COLOR_BOTTOM
	lcProfileEntry("Settings", "DrawAxes", 0),                                                 // LC_PROFILE_DRAW_AXES
	lcProfileEntry("Settings", "AxesColor", LC_RGBA(0, 0, 0, 255)),                            // LC_PROFILE_AXES_COLOR
	lcProfileEntry("Settings", "TextColor", LC_RGBA(0, 0, 0, 255)),                            // LC_PROFILE_TEXT_COLOR
	lcProfileEntry("Settings", "MarqueeBorderColor", LC_RGBA(64, 64, 255, 255)),               // LC_PROFILE_MARQUEE_BORDER_COLOR
	lcProfileEntry("Settings", "MarqueeFillColor", LC_RGBA(64, 64, 255, 64)),                  // LC_PROFILE_MARQUEE_FILL_COLOR
	lcProfileEntry("Settings", "OverlayColor", LC_RGBA(0, 0, 0, 255)),                         // LC_PROFILE_OVERLAY_COLOR
	lcProfileEntry("Settings", "ActiveViewColor", LC_RGBA(41, 128, 185, 255)),                 // LC_PROFILE_ACTIVE_VIEW_COLOR
	lcProfileEntry("Settings", "InactiveViewColor", LC_RGBA(69, 69, 69, 255)),                 // LC_PROFILE_INACTIVE_VIEW_COLOR
	lcProfileEntry("Settings", "DrawEdgeLines", 1),                                            // LC_PROFILE_DRAW_EDGE_LINES
	lcProfileEntry("Settings", "GridStuds", 1),                                                // LC_PROFILE_GRID_STUDS
	lcProfileEntry("Settings", "GridStudColor", LC_RGBA(24, 24, 24, 192)),                     // LC_PROFILE_GRID_STUD_COLOR
	lcProfileEntry("Settings", "GridLines", 1),                                                // LC_PROFILE_GRID_LINES
	lcProfileEntry("Settings", "GridLineSpacing", 5),                                          // LC_PROFILE_GRID_LINE_SPACING
	lcProfileEntry("Settings", "GridLineColor", LC_RGBA(24, 24, 24, 255)),                     // LC_PROFILE_GRID_LINE_COLOR
	lcProfileEntry("Settings", "GridOrigin", 0),                                               // LC_PROFILE_GRID_ORIGIN
	lcProfileEntry("Settings", "AASamples", 1),                                                // LC_PROFILE_ANTIALIASING_SAMPLES
	lcProfileEntry("Settings", "ViewSphereEnabled", 1),                                        // LC_PROFILE_VIEW_SPHERE_ENABLED
	lcProfileEntry("Settings", "ViewSphereLocation", (int)lcViewSphereLocation::TopRight),     // LC_PROFILE_VIEW_SPHERE_LOCATION
	lcProfileEntry("Settings", "ViewSphereSize", 100),                                         // LC_PROFILE_VIEW_SPHERE_SIZE
	lcProfileEntry("Settings", "ViewSphereColor", LC_RGBA(35, 38, 41, 255)),                   // LC_PROFILE_VIEW_SPHERE_COLOR
	lcProfileEntry("Settings", "ViewSphereTextColor", LC_RGBA(224, 224, 224, 255)),            // LC_PROFILE_VIEW_SPHERE_TEXT_COLOR
	lcProfileEntry("Settings", "ViewSphereHighlightColor", LC_RGBA(41, 128, 185, 255)),        // LC_PROFILE_VIEW_SPHERE_HIGHLIGHT_COLOR

	lcProfileEntry("Settings", "Language", ""),                                                // LC_PROFILE_LANGUAGE
	lcProfileEntry("Settings", "ColorTheme", static_cast<int>(lcColorTheme::Dark)),            // LC_PROFILE_COLOR_THEME
	lcProfileEntry("Settings", "CheckUpdates", 1),                                             // LC_PROFILE_CHECK_UPDATES
	lcProfileEntry("Settings", "ProjectsPath", ""),                                            // LC_PROFILE_PROJECTS_PATH
	lcProfileEntry("Settings", "PartsLibrary", ""),                                            // LC_PROFILE_PARTS_LIBRARY
	lcProfileEntry("Settings", "PartPalettes"),                                                // LC_PROFILE_PART_PALETTES
	lcProfileEntry("Settings", "MinifigSettings", ""),                                         // LC_PROFILE_MINIFIG_SETTINGS
	lcProfileEntry("Settings", "ColorConfig", ""),                                             // LC_PROFILE_COLOR_CONFIG
	lcProfileEntry("Settings", "Shortcuts"),                                                   // LC_PROFILE_KEYBOARD_SHORTCUTS
	lcProfileEntry("Settings", "MouseShortcuts", QStringList()),                               // LC_PROFILE_MOUSE_SHORTCUTS
	lcProfileEntry("Settings", "Categories"),                                                  // LC_PROFILE_CATEGORIES
	lcProfileEntry("Settings", "RecentFile1", ""),                                             // LC_PROFILE_RECENT_FILE1
	lcProfileEntry("Settings", "RecentFile2", ""),                                             // LC_PROFILE_RECENT_FILE2
	lcProfileEntry("Settings", "RecentFile3", ""),                                             // LC_PROFILE_RECENT_FILE3
	lcProfileEntry("Settings", "RecentFile4", ""),                                             // LC_PROFILE_RECENT_FILE4
	lcProfileEntry("Settings", "AutoLoadMostRecent", false),                                   // LC_PROFILE_AUTOLOAD_MOSTRECENT
	lcProfileEntry("Settings", "RestoreTabLayout", true),                                      // LC_PROFILE_RESTORE_TAB_LAYOUT
	lcProfileEntry("Settings", "AutosaveInterval", 10),                                        // LC_PROFILE_AUTOSAVE_INTERVAL
	lcProfileEntry("Settings", "MouseSensitivity", 11),                                        // LC_PROFILE_MOUSE_SENSITIVITY
	lcProfileEntry("Settings", "ImageWidth", 1280),                                            // LC_PROFILE_IMAGE_WIDTH
	lcProfileEntry("Settings", "ImageHeight", 720),                                            // LC_PROFILE_IMAGE_HEIGHT
	lcProfileEntry("Settings", "ImageExtension", ".png"),                                      // LC_PROFILE_IMAGE_EXTENSION
	lcProfileEntry("Settings", "PartsListIcons", 64),                                          // LC_PROFILE_PARTS_LIST_ICONS
	lcProfileEntry("Settings", "PartsListNames", 0),                                           // LC_PROFILE_PARTS_LIST_NAMES
	lcProfileEntry("Settings", "PartsListFixedColor", -1),                                     // LC_PROFILE_PARTS_LIST_FIXED_COLOR
	lcProfileEntry("Settings", "PartsListDecorated", 1),                                       // LC_PROFILE_PARTS_LIST_DECORATED
	lcProfileEntry("Settings", "PartsListAliases", 1),                                         // LC_PROFILE_PARTS_LIST_ALIASES
	lcProfileEntry("Settings", "PartsListListMode", 0),                                        // LC_PROFILE_PARTS_LIST_LISTMODE
	lcProfileEntry("Settings", "StudStyle", 0),                                                // LC_PROFILE_STUD_STYLE

	lcProfileEntry("Defaults", "Author", ""),                                                  // LC_PROFILE_DEFAULT_AUTHOR_NAME
	lcProfileEntry("Defaults", "AmbientColor", LC_RGB(75, 75, 75)),                            // LC_PROFILE_DEFAULT_AMBIENT_COLOR

	lcProfileEntry("HTML", "Options", LC_HTML_SINGLEPAGE),                                     // LC_PROFILE_HTML_OPTIONS
	lcProfileEntry("HTML", "ImageOptions", LC_IMAGE_TRANSPARENT),                              // LC_PROFILE_HTML_IMAGE_OPTIONS
	lcProfileEntry("HTML", "ImageWidth", 640),                                                 // LC_PROFILE_HTML_IMAGE_WIDTH
	lcProfileEntry("HTML", "ImageHeight", 480),                                                // LC_PROFILE_HTML_IMAGE_HEIGHT

	lcProfileEntry("POVRay", "Path", "/usr/bin/povray"),                                       // LC_PROFILE_POVRAY_PATH
	lcProfileEntry("POVRay", "LGEOPath", ""),                                                  // LC_PROFILE_POVRAY_LGEO_PATH
	lcProfileEntry("POVRay", "Width", 1280),                                                   // LC_PROFILE_POVRAY_WIDTH
	lcProfileEntry("POVRay", "Height", 720),                                                   // LC_PROFILE_POVRAY_HEIGHT

	lcProfileEntry("Settgins", "PreviewViewSphereEnabled", 0),                                    // LC_PROFILE_PREVIEW_VIEW_SPHERE_ENABLED
	lcProfileEntry("Settings", "PreviewViewSphereSize", 75),                                      // LC_PROFILE_PREVIEW_VIEW_SPHERE_SIZE
	lcProfileEntry("Settings", "PreviewViewSphereLocation", (int)lcViewSphereLocation::TopRight), // LC_PROFILE_PREVIEW_VIEW_SPHERE_LOCATION
	lcProfileEntry("Settings", "DrawPreviewAxis", 0),                                             // LC_PROFILE_PREVIEW_DRAW_AXES

	lcProfileEntry("Settings", "StudColor", LC_RGBA(27, 42, 52, 255)),                         // LC_PROFILE_STUD_COLOR
	lcProfileEntry("Settings", "StudEdgeColor", LC_RGBA(0, 0, 0, 255)),                        // LC_PROFILE_STUD_EDGE_COLOR
	lcProfileEntry("Settings", "BlackEdgeColor", LC_RGBA(255, 255, 255, 255)),                 // LC_PROFILE_BLACK_EDGE_COLOR
	lcProfileEntry("Settings", "DarkEdgeColor", LC_RGBA(27, 42, 52, 255)),                     // LC_PROFILE_DARK_EDGE_COLOR
	lcProfileEntry("Settings", "PartEdgeContrast", 0.5f),                                      // LC_PROFILE_PART_EDGE_CONTRAST
	lcProfileEntry("Settings", "mPartColorValueLDIndex", 0.5f),                                // LC_PROFILE_PART_COLOR_VALUE_LD_INDEX
	lcProfileEntry("Settings", "AutomateEdgeColor", 0)                                         // LC_PROFILE_AUTOMATE_EDGE_COLOR
};

void lcRemoveProfileKey(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	Settings.remove(QString("%1/%2").arg(Entry.mSection, Entry.mKey));
}

int lcGetDefaultProfileInt(LC_PROFILE_KEY Key)
{
	return gProfileEntries[Key].mDefault.IntValue;
}

float lcGetDefaultProfileFloat(LC_PROFILE_KEY Key)
{
	return gProfileEntries[Key].mDefault.FloatValue;
}

QString lcGetDefaultProfileString(LC_PROFILE_KEY Key)
{
	return QString::fromLatin1(gProfileEntries[Key].mDefault.StringValue);
}

int lcGetProfileInt(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	return Settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Entry.mDefault.IntValue).toInt();
}

float lcGetProfileFloat(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	return Settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Entry.mDefault.FloatValue).toFloat();
}

QString lcGetProfileString(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	return Settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Entry.mDefault.StringValue).toString();
}

QStringList lcGetProfileStringList(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	return Settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), QStringList()).toStringList();
}

QByteArray lcGetProfileBuffer(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	return Settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey)).toByteArray();
}

void lcSetProfileInt(LC_PROFILE_KEY Key, int Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

void lcSetProfileFloat(LC_PROFILE_KEY Key, float Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

void lcSetProfileString(LC_PROFILE_KEY Key, const QString& Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

void lcSetProfileStringList(LC_PROFILE_KEY Key, const QStringList& Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

void lcSetProfileBuffer(LC_PROFILE_KEY Key, const QByteArray& Buffer)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Buffer);
}

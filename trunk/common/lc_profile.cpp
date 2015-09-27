#include "lc_global.h"
#include "lc_profile.h"
#include "lc_application.h"
#include "image.h"
#include "lc_model.h"
#include "project.h"
#include "system.h"

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

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key)
{
	mType = LC_PROFILE_ENTRY_BUFFER;
	mSection = Section;
	mKey = Key;
	mDefault.IntValue = 0;
}

lcProfileEntry gProfileEntries[LC_NUM_PROFILE_KEYS] =
{
	lcProfileEntry("Settings", "FixedAxes", false),                                  // LC_PROFILE_FIXED_AXES
	lcProfileEntry("Settings", "LineWidth", 1.0f),                                   // LC_PROFILE_LINE_WIDTH
	lcProfileEntry("Settings", "LightingMode", LC_LIGHTING_FLAT),                    // LC_PROFILE_LIGHTING_MODE
	lcProfileEntry("Settings", "DrawAxes", 0),                                       // LC_PROFILE_DRAW_AXES
	lcProfileEntry("Settings", "DrawEdgeLines", 1),                                  // LC_PROFILE_DRAW_EDGE_LINES
	lcProfileEntry("Settings", "GridStuds", 1),                                      // LC_PROFILE_GRID_STUDS
	lcProfileEntry("Settings", "GridStudColor", LC_RGBA(64, 64, 64, 192)),           // LC_PROFILE_GRID_STUD_COLOR
	lcProfileEntry("Settings", "GridLines", 1),                                      // LC_PROFILE_GRID_LINES
	lcProfileEntry("Settings", "GridLineSpacing", 5),                                // LC_PROFILE_GRID_LINE_SPACING
	lcProfileEntry("Settings", "GridLineColor", LC_RGBA(0, 0, 0, 255)),              // LC_PROFILE_GRID_LINE_COLOR
	lcProfileEntry("Settings", "AASamples", 1),                                      // LC_PROFILE_ANTIALIASING_SAMPLES

	lcProfileEntry("Settings", "CheckUpdates", 1),                                   // LC_PROFILE_CHECK_UPDATES
	lcProfileEntry("Settings", "ProjectsPath", ""),                                  // LC_PROFILE_PROJECTS_PATH
	lcProfileEntry("Settings", "PartsLibrary", ""),                                  // LC_PROFILE_PARTS_LIBRARY
	lcProfileEntry("Settings", "Shortcuts"),                                         // LC_PROFILE_SHORTCUTS
	lcProfileEntry("Settings", "Categories"),                                        // LC_PROFILE_CATEGORIES
	lcProfileEntry("Settings", "RecentFile1", ""),                                   // LC_PROFILE_RECENT_FILE1
	lcProfileEntry("Settings", "RecentFile2", ""),                                   // LC_PROFILE_RECENT_FILE2
	lcProfileEntry("Settings", "RecentFile3", ""),                                   // LC_PROFILE_RECENT_FILE3
	lcProfileEntry("Settings", "RecentFile4", ""),                                   // LC_PROFILE_RECENT_FILE4
	lcProfileEntry("Settings", "AutosaveInterval", 10),                              // LC_PROFILE_AUTOSAVE_INTERVAL
	lcProfileEntry("Settings", "MouseSensitivity", 11),                              // LC_PROFILE_MOUSE_SENSITIVITY
	lcProfileEntry("Settings", "ImageWidth", 1280),                                  // LC_PROFILE_IMAGE_WIDTH
	lcProfileEntry("Settings", "ImageHeight", 720),                                  // LC_PROFILE_IMAGE_HEIGHT
	lcProfileEntry("Settings", "ImageExtension", ".png"),                            // LC_PROFILE_IMAGE_EXTENSION
	lcProfileEntry("Settings", "PrintRows", 1),                                      // LC_PROFILE_PRINT_ROWS
	lcProfileEntry("Settings", "PrintColumns", 1),                                   // LC_PROFILE_PRINT_COLUMNS

	lcProfileEntry("Defaults", "Author", ""),                                        // LC_PROFILE_DEFAULT_AUTHOR_NAME
	lcProfileEntry("Defaults", "FloorColor", LC_RGB(0, 191, 0)),                     // LC_PROFILE_DEFAULT_FLOOR_COLOR
	lcProfileEntry("Defaults", "FloorTexture", ""),                                  // LC_PROFILE_DEFAULT_FLOOR_TEXTURE
	lcProfileEntry("Defaults", "FogEnabled", 0),                                     // LC_PROFILE_DEFAULT_FOG_ENABLED
	lcProfileEntry("Defaults", "FogDensity", 0.1f),                                  // LC_PROFILE_DEFAULT_FOG_DENSITY
	lcProfileEntry("Defaults", "FogColor", LC_RGB(255, 255, 255)),                   // LC_PROFILE_DEFAULT_FOG_COLOR
	lcProfileEntry("Defaults", "AmbientColor", LC_RGB(75, 75, 75)),                  // LC_PROFILE_DEFAULT_AMBIENT_COLOR
	lcProfileEntry("Defaults", "BackgroundType", LC_BACKGROUND_SOLID),               // LC_PROFILE_DEFAULT_BACKGROUND_TYPE
	lcProfileEntry("Defaults", "BackgroundColor", LC_RGB(255, 255, 255)),            // LC_PROFILE_DEFAULT_BACKGROUND_COLOR
	lcProfileEntry("Defaults", "GradientColor1", LC_RGB(0, 0, 191)),                 // LC_PROFILE_DEFAULT_GRADIENT_COLOR1
	lcProfileEntry("Defaults", "GradientColor2", LC_RGB(255, 255, 255)),             // LC_PROFILE_DEFAULT_GRADIENT_COLOR2
	lcProfileEntry("Defaults", "BackgroundTexture", ""),                             // LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE
	lcProfileEntry("Defaults", "BackgroundTile", 0),                                 // LC_PROFILE_DEFAULT_BACKGROUND_TILE

	lcProfileEntry("HTML", "Options", LC_HTML_SINGLEPAGE),                           // LC_PROFILE_HTML_OPTIONS
	lcProfileEntry("HTML", "ImageOptions", LC_IMAGE_PNG | LC_IMAGE_TRANSPARENT),     // LC_PROFILE_HTML_IMAGE_OPTIONS
	lcProfileEntry("HTML", "ImageWidth", 640),                                       // LC_PROFILE_HTML_IMAGE_WIDTH
	lcProfileEntry("HTML", "ImageHeight", 480),                                      // LC_PROFILE_HTML_IMAGE_HEIGHT
	lcProfileEntry("HTML", "PartsColor", 16),                                        // LC_PROFILE_HTML_PARTS_COLOR
	lcProfileEntry("HTML", "PartsWidth", 128),                                       // LC_PROFILE_HTML_PARTS_WIDTH
	lcProfileEntry("HTML", "PartsHeight", 128),                                      // LC_PROFILE_HTML_PARTS_HEIGHT

	lcProfileEntry("POVRay", "Path", ""),                                            // LC_PROFILE_POVRAY_PATH
	lcProfileEntry("POVRay", "LGEOPath", ""),                                        // LC_PROFILE_POVRAY_LGEO_PATH
	lcProfileEntry("POVRay", "Render", 1),                                           // LC_PROFILE_POVRAY_RENDER
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

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_INT);

	return Settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Entry.mDefault.IntValue).toInt();
}

float lcGetProfileFloat(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_FLOAT);

	return Settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Entry.mDefault.FloatValue).toFloat();
}

QString lcGetProfileString(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_STRING);

	return Settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Entry.mDefault.StringValue).toString();
}

QByteArray lcGetProfileBuffer(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_BUFFER);

	return Settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey)).toByteArray();
}

void lcGetProfileBuffer(LC_PROFILE_KEY Key, lcMemFile& Buffer) // todo: deprecated
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;
	QByteArray Value;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_BUFFER);

	Value = Settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey)).toByteArray();

	Buffer.Seek(0, SEEK_SET);
	Buffer.SetLength(Value.size());
	Buffer.WriteBuffer(Value.constData(), Value.size());
	Buffer.Seek(0, SEEK_SET);
}

void lcSetProfileInt(LC_PROFILE_KEY Key, int Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_INT);

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

void lcSetProfileFloat(LC_PROFILE_KEY Key, float Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_FLOAT);

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

void lcSetProfileString(LC_PROFILE_KEY Key, const QString& Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_STRING);

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

void lcSetProfileBuffer(LC_PROFILE_KEY Key, const QByteArray& Buffer)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_BUFFER);

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Buffer);
}

void lcSetProfileBuffer(LC_PROFILE_KEY Key, const lcMemFile& Buffer) // todo: deprecated
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;
	QByteArray Value = QByteArray::fromRawData((const char*)Buffer.mBuffer, Buffer.GetLength());

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_BUFFER);

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

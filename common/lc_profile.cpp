#include "lc_global.h"
#include "lc_profile.h"
#include "lc_context.h"
#include "lc_model.h"
#include "project.h"
#include "lc_viewsphere.h"
#include "lc_renderdialog.h"
#include "lc_library.h"
#include "lc_partselectionwidget.h"
#include "lc_colors.h"

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key, int DefaultValue)
{
	mSection = Section;
	mKey = Key;
	mDefault = DefaultValue;
}

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key, uint DefaultValue)
{
	mSection = Section;
	mKey = Key;
	mDefault = DefaultValue;
}

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key, float DefaultValue)
{
	mSection = Section;
	mKey = Key;
	mDefault = DefaultValue;
}

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key, const char* DefaultValue)
{
	mSection = Section;
	mKey = Key;
	mDefault = QString(DefaultValue);
}

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key, const QStringList& /*StringList*/)
{
	mSection = Section;
	mKey = Key;
	mDefault = QStringList();
}

lcProfileEntry::lcProfileEntry(const char* Section, const char* Key)
{
	mSection = Section;
	mKey = Key;
	mDefault = QByteArray();
}

static lcProfileEntry gProfileEntries[LC_NUM_PROFILE_KEYS] =
{
	lcProfileEntry("Settings", "FixedAxes", false),                                                        // LC_PROFILE_FIXED_AXES
	lcProfileEntry("Settings", "LineWidth", 1.0f),                                                         // LC_PROFILE_LINE_WIDTH
	lcProfileEntry("Settings", "AllowLOD", true),                                                          // LC_PROFILE_ALLOW_LOD
	lcProfileEntry("Settings", "LODDistance", 750.0f),                                                     // LC_PROFILE_LOD_DISTANCE
	lcProfileEntry("Settings", "FadeSteps", false),                                                        // LC_PROFILE_FADE_STEPS
	lcProfileEntry("Settings", "FadeStepsColor", LC_RGBA(128, 128, 128, 128)),                             // LC_PROFILE_FADE_STEPS_COLOR
	lcProfileEntry("Settings", "HighlightNewParts", 0),                                                    // LC_PROFILE_HIGHLIGHT_NEW_PARTS
	lcProfileEntry("Settings", "HighlightNewPartsColor", LC_RGBA(255, 242, 0, 192)),                       // LC_PROFILE_HIGHLIGHT_NEW_PARTS_COLOR
	lcProfileEntry("Settings", "ShadingMode", static_cast<int>(lcShadingMode::DefaultLights)),             // LC_PROFILE_SHADING_MODE
	lcProfileEntry("Settings", "HighlightRenderMode", static_cast<int>(lcHighlightRenderMode::Outline)),      // LC_PROFILE_HIGHLIGHT_RENDER_MODE
	lcProfileEntry("Settings", "BackgroundGradient", false),                                               // LC_PROFILE_BACKGROUND_GRADIENT
	lcProfileEntry("Settings", "BackgroundColor", LC_RGB(49, 52, 55)),                                     // LC_PROFILE_BACKGROUND_COLOR
	lcProfileEntry("Settings", "GradientColorTop", LC_RGB(54, 72, 95)),                                    // LC_PROFILE_GRADIENT_COLOR_TOP
	lcProfileEntry("Settings", "GradientColorBottom", LC_RGB(49, 52, 55)),                                 // LC_PROFILE_GRADIENT_COLOR_BOTTOM
	lcProfileEntry("Settings", "DrawAxes", 0),                                                             // LC_PROFILE_DRAW_AXES
	lcProfileEntry("Settings", "DrawAxesLocation", static_cast<int>(lcAxisIconLocation::BottomLeft)),      // LC_PROFILE_DRAW_AXES_LOCATION
	lcProfileEntry("Settings", "TextColor", LC_RGBA(214, 220, 227, 255)),                                  // LC_PROFILE_TEXT_COLOR
	lcProfileEntry("Settings", "TextHaloColor", LC_RGBA(17, 19, 24, 160)),                                // LC_PROFILE_TEXT_HALO_COLOR
	lcProfileEntry("Settings", "MarqueeBorderColor", LC_RGBA(64, 64, 255, 255)),                           // LC_PROFILE_MARQUEE_BORDER_COLOR
	lcProfileEntry("Settings", "MarqueeFillColor", LC_RGBA(64, 64, 255, 64)),                              // LC_PROFILE_MARQUEE_FILL_COLOR
	lcProfileEntry("Settings", "OverlayColor", LC_RGBA(0, 0, 0, 255)),                                     // LC_PROFILE_OVERLAY_COLOR
	lcProfileEntry("Settings", "ActiveViewColor", LC_RGBA(41, 128, 185, 255)),                             // LC_PROFILE_ACTIVE_VIEW_COLOR
	lcProfileEntry("Settings", "InactiveViewColor", LC_RGBA(69, 69, 69, 255)),                             // LC_PROFILE_INACTIVE_VIEW_COLOR
	lcProfileEntry("Settings", "DrawEdgeLines", 1),                                                        // LC_PROFILE_DRAW_EDGE_LINES
	lcProfileEntry("Settings", "DrawConditionalLines", 1),                                                 // LC_PROFILE_DRAW_CONDITIONAL_LINES
	lcProfileEntry("Settings", "GridStuds", 1),                                                            // LC_PROFILE_GRID_STUDS
	lcProfileEntry("Settings", "GridStudColor", LC_RGBA(24, 24, 24, 192)),                                 // LC_PROFILE_GRID_STUD_COLOR
	lcProfileEntry("Settings", "GridLines", 1),                                                            // LC_PROFILE_GRID_LINES
	lcProfileEntry("Settings", "GridLineSpacing", 5),                                                      // LC_PROFILE_GRID_LINE_SPACING
	lcProfileEntry("Settings", "GridLineColor", LC_RGBA(24, 24, 24, 255)),                                 // LC_PROFILE_GRID_LINE_COLOR
	lcProfileEntry("Settings", "GridOrigin", 0),                                                           // LC_PROFILE_GRID_ORIGIN
	lcProfileEntry("Settings", "AASamples", 1),                                                            // LC_PROFILE_ANTIALIASING_SAMPLES
	lcProfileEntry("Settings", "ViewSphereEnabled", 1),                                                    // LC_PROFILE_VIEW_SPHERE_ENABLED
	lcProfileEntry("Settings", "ViewSphereLocation", (int)lcViewSphereLocation::TopRight),                 // LC_PROFILE_VIEW_SPHERE_LOCATION
	lcProfileEntry("Settings", "ViewSphereSize", 100),                                                     // LC_PROFILE_VIEW_SPHERE_SIZE
	lcProfileEntry("Settings", "ViewSphereColor", LC_RGBA(27, 31, 36, 255)),                               // LC_PROFILE_VIEW_SPHERE_COLOR
	lcProfileEntry("Settings", "ViewSphereTextColor", LC_RGBA(232, 237, 242, 255)),                        // LC_PROFILE_VIEW_SPHERE_TEXT_COLOR
	lcProfileEntry("Settings", "ViewSphereHighlightColor", LC_RGBA(41, 128, 185, 255)),                    // LC_PROFILE_VIEW_SPHERE_HIGHLIGHT_COLOR
	lcProfileEntry("Settings", "ObjectSelectedColor", static_cast<uint>(LC_RGBA(229, 76, 102, 255))),      // LC_PROFILE_OBJECT_SELECTED_COLOR
	lcProfileEntry("Settings", "ObjectFocusedColor", static_cast<uint>(LC_RGBA(102, 76, 229, 255))),       // LC_PROFILE_OBJECT_FOCUSED_COLOR
	lcProfileEntry("Settings", "CameraColor", static_cast<uint>(LC_RGBA(128, 204, 128, 255))),             // LC_PROFILE_CAMERA_COLOR
	lcProfileEntry("Settings", "LightColor", static_cast<uint>(LC_RGBA(128, 204, 128, 255))),              // LC_PROFILE_LIGHT_COLOR
	lcProfileEntry("Settings", "ControlPointColor", static_cast<uint>(LC_RGBA(128, 204, 128, 128))),       // LC_PROFILE_CONTROL_POINT_COLOR
	lcProfileEntry("Settings", "ControlPointFocusedColor", static_cast<uint>(LC_RGBA(102, 76, 229, 128))), // LC_PROFILE_CONTROL_POINT_FOCUSED_COLOR

	lcProfileEntry("Settings", "Language", ""),                                                // LC_PROFILE_LANGUAGE
	lcProfileEntry("Settings", "ColorTheme", static_cast<int>(lcColorTheme::Dark)),            // LC_PROFILE_COLOR_THEME
	lcProfileEntry("Settings", "CheckUpdates", 1),                                             // LC_PROFILE_CHECK_UPDATES
	lcProfileEntry("Settings", "ProjectsPath", ""),                                            // LC_PROFILE_PROJECTS_PATH
	lcProfileEntry("Settings", "PartsLibrary", ""),                                            // LC_PROFILE_PARTS_LIBRARY
	lcProfileEntry("Settings", "ShadowLibrary", ""),                                           // LC_PROFILE_SHADOW_LIBRARY
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
	lcProfileEntry("Settings", "PartsListNames", 1),                                           // LC_PROFILE_PARTS_LIST_NAMES
	lcProfileEntry("Settings", "PartsListFixedColor", static_cast<uint>(LC_COLOR_NOCOLOR)),                                     // LC_PROFILE_PARTS_LIST_COLOR
	lcProfileEntry("Settings", "PartsListColorLocked", false),                                // LC_PROFILE_PARTS_LIST_COLOR_LOCKED
	lcProfileEntry("Settings", "PartsListDecorated", 1),                                       // LC_PROFILE_PARTS_LIST_DECORATED
	lcProfileEntry("Settings", "PartsListAliases", 1),                                         // LC_PROFILE_PARTS_LIST_ALIASES
	lcProfileEntry("Settings", "PartsListListMode", 0),                                        // LC_PROFILE_PARTS_LIST_LISTMODE
	lcProfileEntry("Settings", "PartsPopupIcons", 64),                                             // LC_PROFILE_PARTS_POPUP_ICONS
	lcProfileEntry("Settings", "PartsPopupNames", 1),                                              // LC_PROFILE_PARTS_POPUP_NAMES
	lcProfileEntry("Settings", "PartsPopupDecorated", 1),                                          // LC_PROFILE_PARTS_POPUP_DECORATED
	lcProfileEntry("Settings", "PartsPopupAliases", 1),                                            // LC_PROFILE_PARTS_POPUP_ALIASES
	lcProfileEntry("Settings", "PartsPopupListMode", 0),                                           // LC_PROFILE_PARTS_POPUP_LISTMODE
	lcProfileEntry("Settings", "PartsListPartFilter", static_cast<int>(lcPartFilterType::Default)),                                      // LC_PROFILE_PARTS_LIST_PART_FILTER
	lcProfileEntry("Settings", "PartsListCaseSensitiveFilter", 0),                             // LC_PROFILE_PARTS_LIST_CASE_SENSITIVE_FILTER
	lcProfileEntry("Settings", "PartsListFileNameFilter", 1),                                  // LC_PROFILE_PARTS_LIST_FILE_NAME_FILTER
	lcProfileEntry("Settings", "PartsListPartDescriptionFilter", 1),                           // LC_PROFILE_PARTS_LIST_PART_DESCRIPTION_FILTER
	lcProfileEntry("Settings", "StudStyle", static_cast<int>(lcStudStyle::Plain)),                                                // LC_PROFILE_STUD_STYLE

	lcProfileEntry("Defaults", "Author", ""),                                                  // LC_PROFILE_DEFAULT_AUTHOR_NAME
	lcProfileEntry("Defaults", "AmbientColor", LC_RGB(75, 75, 75)),                            // LC_PROFILE_DEFAULT_AMBIENT_COLOR

	lcProfileEntry("HTML", "Options", LC_HTML_SINGLEPAGE),                                     // LC_PROFILE_HTML_OPTIONS
	lcProfileEntry("HTML", "ImageOptions", LC_IMAGE_TRANSPARENT),                              // LC_PROFILE_HTML_IMAGE_OPTIONS
	lcProfileEntry("HTML", "ImageWidth", 640),                                                 // LC_PROFILE_HTML_IMAGE_WIDTH
	lcProfileEntry("HTML", "ImageHeight", 480),                                                // LC_PROFILE_HTML_IMAGE_HEIGHT

	lcProfileEntry("POVRay", "Path", "/usr/bin/povray"),                                       // LC_PROFILE_POVRAY_PATH
	lcProfileEntry("POVRay", "LGEOPath", ""),                                                  // LC_PROFILE_POVRAY_LGEO_PATH

	lcProfileEntry("Render", "Width", 1280),                                                   // LC_PROFILE_RENDER_WIDTH
	lcProfileEntry("Render", "Height", 720),                                                   // LC_PROFILE_RENDER_HEIGHT
	lcProfileEntry("Render", "Quality", (int)lcPOVRayRenderQuality::High),                     // LC_PROFILE_RENDER_QUALITY

	lcProfileEntry("Blender", "Path", ""),                                                     // LC_PROFILE_BLENDER_PATH
	lcProfileEntry("Blender", "LDrawConfigPath", ""),                                          // LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH
	lcProfileEntry("Blender", "Version", ""),                                                  // LC_PROFILE_BLENDER_VERSION
	lcProfileEntry("Blender", "AddonVersion", ""),                                             // LC_PROFILE_BLENDER_ADDON_VERSION
	lcProfileEntry("Blender", "AddonVersionCheck", 1),                                         // LC_PROFILE_BLENDER_ADDON_VERSION_CHECK
	lcProfileEntry("Blender", "ImportModule", ""),                                             // LC_PROFILE_BLENDER_IMPORT_MODULE

	lcProfileEntry("Settings", "PreviewViewSphereEnabled", 0),                                    // LC_PROFILE_PREVIEW_VIEW_SPHERE_ENABLED
	lcProfileEntry("Settings", "PreviewViewSphereSize", 75),                                      // LC_PROFILE_PREVIEW_VIEW_SPHERE_SIZE
	lcProfileEntry("Settings", "PreviewViewSphereLocation", (int)lcViewSphereLocation::TopRight), // LC_PROFILE_PREVIEW_VIEW_SPHERE_LOCATION
	lcProfileEntry("Settings", "DrawPreviewAxis", 0),                                             // LC_PROFILE_PREVIEW_DRAW_AXES

	lcProfileEntry("Settings", "StudCylinderColorEnabled", true),                              // LC_PROFILE_STUD_CYLINDER_COLOR_ENABLED
	lcProfileEntry("Settings", "StudCylinderColor", LC_RGBA(27, 42, 52, 255)),                 // LC_PROFILE_STUD_CYLINDER_COLOR
	lcProfileEntry("Settings", "PartEdgeColorEnabled", true),                                  // LC_PROFILE_PART_EDGE_COLOR_ENABLED
	lcProfileEntry("Settings", "PartEdgeColor", LC_RGBA(0, 0, 0, 255)),                        // LC_PROFILE_PART_EDGE_COLOR
	lcProfileEntry("Settings", "BlackEdgeColorEnabled", true),                                 // LC_PROFILE_BLACK_EDGE_COLOR_ENABLED
	lcProfileEntry("Settings", "BlackEdgeColor", LC_RGBA(255, 255, 255, 255)),                 // LC_PROFILE_BLACK_EDGE_COLOR
	lcProfileEntry("Settings", "DarkEdgeColorEnabled", true),                                  // LC_PROFILE_DARK_EDGE_COLOR_ENABLED
	lcProfileEntry("Settings", "DarkEdgeColor", LC_RGBA(27, 42, 52, 255)),                     // LC_PROFILE_DARK_EDGE_COLOR
	lcProfileEntry("Settings", "PartEdgeContrast", 0.5f),                                      // LC_PROFILE_PART_EDGE_CONTRAST
	lcProfileEntry("Settings", "PartColorValueLDIndex", 0.5f),                                 // LC_PROFILE_PART_COLOR_VALUE_LD_INDEX
	lcProfileEntry("Settings", "AutomateEdgeColor", 0)                                         // LC_PROFILE_AUTOMATE_EDGE_COLOR
};

void lcProfileInit()
{
	gProfileEntries[LC_PROFILE_PROJECTS_PATH].mDefault = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

void lcRemoveProfileKey(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	Settings.remove(QString("%1/%2").arg(Entry.mSection, Entry.mKey));
}

int lcGetProfileInt(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	int* DefaultInt = std::get_if<int>(&Entry.mDefault);
	int DefaultValue = DefaultInt ? *DefaultInt : 0;

	const QVariant Stored = QSettings().value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), DefaultValue);

	if (Stored.userType() == QMetaType::Float || Stored.userType() == QMetaType::Double)
		return DefaultValue;
	if (Stored.userType() == QMetaType::ULongLong && Stored.toULongLong() > static_cast<qulonglong>(std::numeric_limits<int>::max()))
		return DefaultValue;

	bool Valid = false;
	const qlonglong Value = Stored.toLongLong(&Valid);

	if (Valid && Value >= std::numeric_limits<int>::min() && Value <= std::numeric_limits<int>::max())
		return static_cast<int>(Value);

	return DefaultValue;
}

uint lcGetProfileUInt(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	uint* DefaultUInt = std::get_if<uint>(&Entry.mDefault);
	uint DefaultValue = DefaultUInt ? *DefaultUInt : 0;

	const QVariant Stored = QSettings().value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), DefaultValue);

	if (Stored.userType() == QMetaType::Float || Stored.userType() == QMetaType::Double)
		return DefaultValue;

	if (Stored.userType() == QMetaType::Int || Stored.userType() == QMetaType::LongLong)
	{
		const qlonglong Value = Stored.toLongLong();

		if (Value >= std::numeric_limits<qint32>::min() && Value <= static_cast<qlonglong>(std::numeric_limits<uint>::max()))
			return static_cast<uint>(Value);
		return DefaultValue;
	}

	bool Valid = false;
	const qulonglong Value = Stored.toULongLong(&Valid);

	if (Valid && Value <= std::numeric_limits<uint>::max())
		return static_cast<uint>(Value);

	return DefaultValue;
}

float lcGetProfileFloat(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	float* DefaultFloat = std::get_if<float>(&Entry.mDefault);
	float DefaultValue = DefaultFloat ? *DefaultFloat : 0.0f;

	bool Valid = false;
	const float Value = QSettings().value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), DefaultValue).toFloat(&Valid);

	return Valid && std::isfinite(Value) ? Value : DefaultValue;
}

QString lcGetProfileString(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QString* DefaultString = std::get_if<QString>(&Entry.mDefault);
	QString DefaultValue = DefaultString ? *DefaultString : QString();

	return QSettings().value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), DefaultValue).toString();
}

QStringList lcGetProfileStringList(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QStringList* DefaultStringList = std::get_if<QStringList>(&Entry.mDefault);
	QStringList DefaultValue = DefaultStringList ? *DefaultStringList : QStringList();

	return QSettings().value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), DefaultValue).toStringList();
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

void lcSetProfileUInt(LC_PROFILE_KEY Key, uint Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings Settings;

	Settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

void lcSetProfileFloat(LC_PROFILE_KEY Key, float Value)
{
	if (!std::isfinite(Value))
		return;

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

template<typename Enum>
struct lcProfileEnumValue
{
	Enum Value;
	const char* Token;
	int LegacyValue;
};

// Tokens and legacy numbers remain stable when enum members are reordered.
constexpr lcProfileEnumValue<lcShadingMode> ShadingModeValues[] =
{
	{ lcShadingMode::Wireframe, "Wireframe", 0 },
	{ lcShadingMode::Flat, "Flat", 1 },
	{ lcShadingMode::DefaultLights, "DefaultLights", 2 },
	{ lcShadingMode::Full, "Full", 3 }
};

constexpr lcProfileEnumValue<lcHighlightRenderMode> HighlightRenderModeValues[] =
{
	{ lcHighlightRenderMode::Outline, "Outline", 0 },
	{ lcHighlightRenderMode::Tint, "Tint", 1 }
};

constexpr lcProfileEnumValue<lcAxisIconLocation> AxisIconLocationValues[] =
{
	{ lcAxisIconLocation::TopLeft, "TopLeft", 0 },
	{ lcAxisIconLocation::TopRight, "TopRight", 1 },
	{ lcAxisIconLocation::BottomLeft, "BottomLeft", 2 },
	{ lcAxisIconLocation::BottomRight, "BottomRight", 3 }
};

constexpr lcProfileEnumValue<lcViewSphereLocation> ViewSphereLocationValues[] =
{
	{ lcViewSphereLocation::TopLeft, "TopLeft", 0 },
	{ lcViewSphereLocation::TopRight, "TopRight", 1 },
	{ lcViewSphereLocation::BottomLeft, "BottomLeft", 2 },
	{ lcViewSphereLocation::BottomRight, "BottomRight", 3 }
};

constexpr lcProfileEnumValue<lcColorTheme> ColorThemeValues[] =
{
	{ lcColorTheme::Dark, "Dark", 0 },
	{ lcColorTheme::System, "System", 1 }
};

constexpr lcProfileEnumValue<lcStudStyle> StudStyleValues[] =
{
	{ lcStudStyle::Plain, "Plain", 0 },
	{ lcStudStyle::ThinLinesLogo, "ThinLinesLogo", 1 },
	{ lcStudStyle::OutlineLogo, "OutlineLogo", 2 },
	{ lcStudStyle::SharpTopLogo, "SharpTopLogo", 3 },
	{ lcStudStyle::RoundedTopLogo, "RoundedTopLogo", 4 },
	{ lcStudStyle::FlattenedLogo, "FlattenedLogo", 5 },
	{ lcStudStyle::HighContrast, "HighContrast", 6 },
	{ lcStudStyle::HighContrastLogo, "HighContrastLogo", 7 }
};

constexpr lcProfileEnumValue<lcPartFilterType> PartFilterTypeValues[] =
{
	{ lcPartFilterType::Default, "Default", 0 },
	{ lcPartFilterType::RegularExpression, "RegularExpression", 1 }
};

constexpr lcProfileEnumValue<lcPOVRayRenderQuality> RenderQualityValues[] =
{
	{ lcPOVRayRenderQuality::Low, "Low", 0 },
	{ lcPOVRayRenderQuality::Medium, "Medium", 1 },
	{ lcPOVRayRenderQuality::High, "High", 2 }
};

template<typename Enum, size_t N>
static Enum lcReadProfileEnum(LC_PROFILE_KEY Key, const lcProfileEnumValue<Enum> (&Values)[N])
{
	const lcProfileEntry& Entry = gProfileEntries[Key];
	const Enum Default = static_cast<Enum>(std::get<int>(Entry.mDefault));
	const QString Token = QSettings().value(QString("%1/%2").arg(Entry.mSection, Entry.mKey)).toString();

	for (const lcProfileEnumValue<Enum>& Value : Values)
	{
		if (Token == QLatin1String(Value.Token))
			return Value.Value;
	}

	bool Valid = false;
	const int LegacyValue = Token.toInt(&Valid);

	if (Valid)
	{
		for (const lcProfileEnumValue<Enum>& Value : Values)
		{
			if (LegacyValue == Value.LegacyValue)
				return Value.Value;
		}
	}

	return Default;
}

template<typename Enum, size_t N>
static void lcWriteProfileEnum(LC_PROFILE_KEY Key, Enum Value, const lcProfileEnumValue<Enum> (&Values)[N])
{
	for (const lcProfileEnumValue<Enum>& Entry : Values)
	{
		if (Value == Entry.Value)
		{
			lcSetProfileString(Key, QString::fromLatin1(Entry.Token));
			return;
		}
	}
}

lcShadingMode lcGetProfileShadingMode()
{
	return lcReadProfileEnum(LC_PROFILE_SHADING_MODE, ShadingModeValues);
}

void lcSetProfileShadingMode(lcShadingMode Value)
{
	lcWriteProfileEnum(LC_PROFILE_SHADING_MODE, Value, ShadingModeValues);
}

lcHighlightRenderMode lcGetProfileHighlightRenderMode()
{
	return lcReadProfileEnum(LC_PROFILE_HIGHLIGHT_RENDER_MODE, HighlightRenderModeValues);
}

void lcSetProfileHighlightRenderMode(lcHighlightRenderMode Value)
{
	lcWriteProfileEnum(LC_PROFILE_HIGHLIGHT_RENDER_MODE, Value, HighlightRenderModeValues);
}

lcAxisIconLocation lcGetProfileAxisIconLocation()
{
	return lcReadProfileEnum(LC_PROFILE_DRAW_AXES_LOCATION, AxisIconLocationValues);
}

void lcSetProfileAxisIconLocation(lcAxisIconLocation Value)
{
	lcWriteProfileEnum(LC_PROFILE_DRAW_AXES_LOCATION, Value, AxisIconLocationValues);
}

lcViewSphereLocation lcGetProfileViewSphereLocation()
{
	return lcReadProfileEnum(LC_PROFILE_VIEW_SPHERE_LOCATION, ViewSphereLocationValues);
}

void lcSetProfileViewSphereLocation(lcViewSphereLocation Value)
{
	lcWriteProfileEnum(LC_PROFILE_VIEW_SPHERE_LOCATION, Value, ViewSphereLocationValues);
}

lcColorTheme lcGetProfileColorTheme()
{
	return lcReadProfileEnum(LC_PROFILE_COLOR_THEME, ColorThemeValues);
}

void lcSetProfileColorTheme(lcColorTheme Value)
{
	lcWriteProfileEnum(LC_PROFILE_COLOR_THEME, Value, ColorThemeValues);
}

lcStudStyle lcGetProfileStudStyle()
{
	return lcReadProfileEnum(LC_PROFILE_STUD_STYLE, StudStyleValues);
}

void lcSetProfileStudStyle(lcStudStyle Value)
{
	lcWriteProfileEnum(LC_PROFILE_STUD_STYLE, Value, StudStyleValues);
}

lcPartFilterType lcGetProfilePartFilterType()
{
	return lcReadProfileEnum(LC_PROFILE_PARTS_LIST_PART_FILTER, PartFilterTypeValues);
}

void lcSetProfilePartFilterType(lcPartFilterType Value)
{
	lcWriteProfileEnum(LC_PROFILE_PARTS_LIST_PART_FILTER, Value, PartFilterTypeValues);
}

lcPOVRayRenderQuality lcGetProfileRenderQuality()
{
	return lcReadProfileEnum(LC_PROFILE_RENDER_QUALITY, RenderQualityValues);
}

void lcSetProfileRenderQuality(lcPOVRayRenderQuality Value)
{
	lcWriteProfileEnum(LC_PROFILE_RENDER_QUALITY, Value, RenderQualityValues);
}

lcViewSphereLocation lcGetProfilePreviewViewSphereLocation()
{
	return lcReadProfileEnum(LC_PROFILE_PREVIEW_VIEW_SPHERE_LOCATION, ViewSphereLocationValues);
}

void lcSetProfilePreviewViewSphereLocation(lcViewSphereLocation Value)
{
	lcWriteProfileEnum(LC_PROFILE_PREVIEW_VIEW_SPHERE_LOCATION, Value, ViewSphereLocationValues);
}

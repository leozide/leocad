#include "lc_global.h"
#include "lc_blenderpreferences.h"
#include "lc_application.h"
#include "lc_mainwindow.h"
#include "lc_model.h"
#include "lc_colors.h"
#include "lc_colorpicker.h"
#include "lc_profile.h"
#include "lc_http.h"
#include "lc_zipfile.h"
#include "lc_file.h"
#include "lc_qutils.h"
#include "project.h"

#ifndef LC_DISABLE_BLENDER

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
using Qt::SkipEmptyParts;
#else
const auto SkipEmptyParts = QString::SplitBehavior::SkipEmptyParts;
#endif

const QLatin1String LineEnding("\r\n");

#define LC_PRODUCTNAME_STR                 "LeoCAD"
#define LC_BLENDER_ADDON                   "ImportLDraw"
#define LC_BLENDER_ADDON_MM                "ImportLDrawMM"
#define LC_BLENDER_ADDON_FILE              "LDrawBlenderRenderAddons.zip"
#define LC_BLENDER_ADDON_INSTALL_FILE      "install_blender_ldraw_addons.py"
#define LC_BLENDER_ADDON_CONFIG_FILE       "LDrawRendererPreferences.ini"
#define LC_BLENDER_ADDON_PARAMS_FILE       "LDrawParameters.lst"

#define LC_BLENDER_ADDON_REPO_STR          "https://github.com/trevorsandy"
#define LC_BLENDER_ADDON_REPO_API_STR      "https://api.github.com/repos/trevorsandy"
#define LC_BLENDER_ADDON_STR               LC_BLENDER_ADDON_REPO_STR "/blenderldrawrender/"
#define LC_BLENDER_ADDON_API_STR           LC_BLENDER_ADDON_REPO_API_STR "/blenderldrawrender/"
#define LC_BLENDER_ADDON_LATEST_URL        LC_BLENDER_ADDON_API_STR "releases/latest"
#define LC_BLENDER_ADDON_URL               LC_BLENDER_ADDON_STR "releases/latest/download/" LC_BLENDER_ADDON_FILE
#define LC_BLENDER_ADDON_SHA_HASH_URL      LC_BLENDER_ADDON_URL ".sha256"

#define LC_THEME_DARK_PALETTE_MIDLIGHT     QColor( 62,  62,  62, 255)
#define LC_THEME_DEFAULT_PALETTE_LIGHT     QColor(174, 173, 172, 255)
#define LC_THEME_DARK_DECORATE_QUOTED_TEXT "#81D4FA" // 129, 212, 250, 255
#define LC_DISABLED_TEXT                   QColor(128, 128, 128, 255)

#define LC_RENDER_IMAGE_MAX_SIZE           32768 // pixels

static constexpr char WhatsThisDescription[] = QT_TRANSLATE_NOOP("QObject",
	"<p><b>Blender LDraw Add-on Settings</b></p><p>Configure Blender and its LDraw import and render add-ons. "
	"Choose the Blender executable first. Editing its path or changing an import module can configure the add-on immediately.</p>"
	"<p>Choose an import module, then adjust its library paths and rendering options. TN and MM have separate settings. "
	"Use the scroll bar to reach all options; Show Paths and Hide Paths control the paths section.</p>"
	"<p>Image dimensions and render percentage start with the values from the Render dialog. Crop Image calculates dimensions for the current model. "
	"Choose Apply to save the paths and settings and close this dialog. Reset restores the last applied values or system defaults.</p>"
	"<p>Cancel closes without applying pending path and setting edits. Add-on installation, module configuration, and the update-check preference can take effect before Apply.</p>");

lcBlenderPreferences::BlenderPaths  lcBlenderPreferences::mBlenderPaths [NUM_PATHS];
lcBlenderPreferences::BlenderPaths  lcBlenderPreferences::mDefaultPaths [NUM_PATHS] =
{
	/*                             Key:                  MM Key:               Value:             Label:                                   Tooltip (Description):*/
	/* 0   PATH_BLENDER        */ {"blenderpath",        "blenderpath",        "",    QT_TRANSLATE_NOOP("QObject", "Blender Path"),             QT_TRANSLATE_NOOP("QObject", "Full file path to Blender application executable")},
	/* 1   PATH_BLENDFILE      */ {"blendfile",          "blendfile",          "",    QT_TRANSLATE_NOOP("QObject", "Blendfile Path"),           QT_TRANSLATE_NOOP("QObject", "Full file path to a supplement .blend file - specify to append additional settings")},
	/* 2   PATH_ENVIRONMENT    */ {"environmentfile",    "environmentfile",    "",    QT_TRANSLATE_NOOP("QObject", "Environment Texture Path"), QT_TRANSLATE_NOOP("QObject", "Full file path to .exr environment texture file - specify if not using default bundled in add-on")},
	/* 3   PATH_LDCONFIG       */ {"customldconfigfile", "customldconfigfile", "",    QT_TRANSLATE_NOOP("QObject", "Custom LDConfig Path"),     QT_TRANSLATE_NOOP("QObject", "Full file path to custom LDConfig file - specify if not %1 alternate LDConfig file")},
	/* 4   PATH_LDRAW          */ {"ldrawdirectory",     "ldrawpath",          "",    QT_TRANSLATE_NOOP("QObject", "LDraw Directory"),          QT_TRANSLATE_NOOP("QObject", "Full directory path to the LDraw pieces library (download from https://library.ldraw.org)")},
	/* 5   PATH_LSYNTH         */ {"lsynthdirectory",    "",                   "",    QT_TRANSLATE_NOOP("QObject", "LSynth Directory"),         QT_TRANSLATE_NOOP("QObject", "Full directory path to LSynth primitives - specify if not using default bundled in add-on")},
	/* 6   PATH_STUD_LOGO      */ {"studlogodirectory",  "",                   "",    QT_TRANSLATE_NOOP("QObject", "Stud Logo Directory"),      QT_TRANSLATE_NOOP("QObject", "Full directory path to stud logo primitives - if stud logo enabled, specify if unofficial pieces not used or not using default bundled in add-on")},
	/* 7   PATH_STUDIO_LDRAW   */ {"",                   "studioldrawpath",    "",    QT_TRANSLATE_NOOP("QObject", "Stud.io LDraw Path"),       QT_TRANSLATE_NOOP("QObject", "Full file path to the Stud.io LDraw Pieces Library (download from https://www.bricklink.com/v3/studio/download.page)")},
	/* 8   PATH_STUDIO_CUSTOM_PARTS */ {"",              "studiocustompartspath", "", QT_TRANSLATE_NOOP("QObject", "Stud.io Custom Pieces Path"),QT_TRANSLATE_NOOP("QObject", "Full file path to the Stud.io LDraw Custom Pieces")}
};

lcBlenderPreferences::BlenderSettings  lcBlenderPreferences::mBlenderSettings [NUM_SETTINGS];
lcBlenderPreferences::BlenderSettings  lcBlenderPreferences::mDefaultSettings [NUM_SETTINGS] =
{
	/*                                   Key:                              Value:                  Label                                  Tooltip (Description)*/
	/* 0   LBL_ADD_ENVIRONMENT       */ {"addenvironment",                 "1",        QT_TRANSLATE_NOOP("QObject", "Add Environment"),        QT_TRANSLATE_NOOP("QObject", "Adds a ground plane and environment texture (affects 'Photo-realistic' look only)")},
	/* 1   LBL_ADD_GAPS              */ {"gaps",                           "0",        QT_TRANSLATE_NOOP("QObject", "Add Piece Gap"),           QT_TRANSLATE_NOOP("QObject", "Add a small space between each piece")},
	/* 2   LBL_BEVEL_EDGES           */ {"beveledges",                     "1",        QT_TRANSLATE_NOOP("QObject", "Bevel Edges"),            QT_TRANSLATE_NOOP("QObject", "Adds a Bevel modifier for rounding off sharp edges")},
	/* 3   LBL_BLENDFILE_TRUSTED     */ {"blendfiletrusted",               "0",        QT_TRANSLATE_NOOP("QObject", "Trusted Blend File"),     QT_TRANSLATE_NOOP("QObject", "Specify whether to treat the .blend file as being loaded from a trusted source")},
	/* 4   LBL_CROP_IMAGE            */ {"cropimage",                      "0",        QT_TRANSLATE_NOOP("QObject", "Crop Image"),             QT_TRANSLATE_NOOP("QObject", "Crop the image border at opaque content. Requires transparent background set to True.")},
	/* 5   LBL_CURVED_WALLS          */ {"curvedwalls",                    "1",        QT_TRANSLATE_NOOP("QObject", "Curved Walls"),           QT_TRANSLATE_NOOP("QObject", "Makes surfaces look slightly concave, for interesting reflections")},
	/* 6   LBL_FLATTEN_HIERARCHY     */ {"flattenhierarchy",               "0",        QT_TRANSLATE_NOOP("QObject", "Flatten Hierarchy"),      QT_TRANSLATE_NOOP("QObject", "In Scene Outline, all pieces are placed directly below the root - there's no tree of submodels")},
	/* 7   LBL_IMPORT_CAMERAS        */ {"importcameras",                  "1",        QT_TRANSLATE_NOOP("QObject", "Import Cameras"),         QT_TRANSLATE_NOOP("QObject", "%1 can specify camera definitions within the LDraw data. Choose to load them or ignore them.")},
	/* 8   LBL_IMPORT_LIGHTS         */ {"importlights",                   "1",        QT_TRANSLATE_NOOP("QObject", "Import Lights"),          QT_TRANSLATE_NOOP("QObject", "%1 can specify point and sunlight definitions within the LDraw data. Choose to load them or ignore them.")},
	/* 9   LBL_INSTANCE_STUDS        */ {"instancestuds",                  "0",        QT_TRANSLATE_NOOP("QObject", "Instance Studs"),         QT_TRANSLATE_NOOP("QObject", "Creates a Blender Object for each and every stud (WARNING: can be slow to import and edit in Blender if there are lots of studs)")},
	/*10   LBL_KEEP_ASPECT_RATIO     */ {"keepaspectratio",                "1",        QT_TRANSLATE_NOOP("QObject", "Keep Aspect Ratio"),      QT_TRANSLATE_NOOP("QObject", "Maintain the aspect ratio when resizing the output image - this attribute is not passed to Blender")},
	/*11   LBL_LINK_PARTS            */ {"linkparts",                      "1",        QT_TRANSLATE_NOOP("QObject", "Link Like Pieces"),        QT_TRANSLATE_NOOP("QObject", "Identical pieces (of the same type and color) share the same mesh")},
	/*12   LBL_MINIFIG_HIERARCHY     */ {"minifighierarchy",               "1",        QT_TRANSLATE_NOOP("QObject", "Parent Minifigs"),        QT_TRANSLATE_NOOP("QObject", "Pieces of minifigs are automatically parented to each other in a hierarchy")},
	/*13   LBL_NUMBER_NODES          */ {"numbernodes",                    "1",        QT_TRANSLATE_NOOP("QObject", "Number Objects"),         QT_TRANSLATE_NOOP("QObject", "Each object has a five-digit prefix, e.g., 00001_car. This keeps the list in its proper order.")},
	/*14   LBL_OVERWRITE_IMAGE       */ {"overwriteimage",                 "1",        QT_TRANSLATE_NOOP("QObject", "Overwrite Image"),        QT_TRANSLATE_NOOP("QObject", "Specify whether to overwrite an existing rendered image file")},
	/*15   LBL_OVERWRITE_MATERIALS   */ {"overwriteexistingmaterials",     "0",        QT_TRANSLATE_NOOP("QObject", "Use Existing Material"),  QT_TRANSLATE_NOOP("QObject", "Overwrite existing material with the same name")},
	/*16   LBL_OVERWRITE_MESHES      */ {"overwriteexistingmeshes",        "0",        QT_TRANSLATE_NOOP("QObject", "Use Existing Mesh"),      QT_TRANSLATE_NOOP("QObject", "Overwrite existing mesh with the same name")},
	/*17   LBL_POSITION_CAMERA       */ {"positioncamera",                 "1",        QT_TRANSLATE_NOOP("QObject", "Position Camera"),        QT_TRANSLATE_NOOP("QObject", "Position the camera to show the whole model")},
	/*18   LBL_REMOVE_DOUBLES        */ {"removedoubles",                  "1",        QT_TRANSLATE_NOOP("QObject", "No Duplicate Vertices"),  QT_TRANSLATE_NOOP("QObject", "Remove duplicate vertices (recommended)")},
	/*19   LBL_RENDER_WINDOW         */ {"renderwindow",                   "1",        QT_TRANSLATE_NOOP("QObject", "Display Render Window"),  QT_TRANSLATE_NOOP("QObject", "Specify whether to display the render window during Blender user interface image file render")},
	/*10   LBL_USE_ARCHIVE_LIBRARY   */ {"usearchivelibrary",              "0",        QT_TRANSLATE_NOOP("QObject", "Use Archive Libraries"),  QT_TRANSLATE_NOOP("QObject", "Add any archive (zip) libraries in the LDraw file path to the library search list - impacts performance")},
	/*21   LBL_SEARCH_ADDL_PATHS     */ {"searchadditionalpaths",          "0",        QT_TRANSLATE_NOOP("QObject", "Search Additional Paths"),QT_TRANSLATE_NOOP("QObject", "Specify whether to search additional LDraw paths")},
	/*22   LBL_SMOOTH_SHADING        */ {"smoothshading",                  "1",        QT_TRANSLATE_NOOP("QObject", "Smooth Shading"),         QT_TRANSLATE_NOOP("QObject", "Smooth faces and add an edge-split modifier (recommended)")},
	/*23   LBL_TRANSPARENT_BACKGROUND*/ {"transparentbackground",          "0",        QT_TRANSLATE_NOOP("QObject", "Transparent Background"), QT_TRANSLATE_NOOP("QObject", "Specify whether to render a background (affects the photorealistic look only)")},
	/*24   LBL_UNOFFICIAL_PARTS      */ {"useunofficialparts",             "1",        QT_TRANSLATE_NOOP("QObject", "Use Unofficial Pieces"),   QT_TRANSLATE_NOOP("QObject", "Specify whether to use pieces from the LDraw unofficial pieces library path")},
	/*25   LBL_USE_LOGO_STUDS        */ {"uselogostuds",                   "1",        QT_TRANSLATE_NOOP("QObject", "Use Logo Studs"),         QT_TRANSLATE_NOOP("QObject", "Shows the LEGO logo on each stud (at the expense of some extra geometry and import time)")},
	/*26   LBL_VERBOSE               */ {"verbose",                        "1",        QT_TRANSLATE_NOOP("QObject", "Verbose Output"),         QT_TRANSLATE_NOOP("QObject", "Output all messages while working, else only show warnings and errors")},

	/*27/0 LBL_BEVEL_WIDTH           */ {"bevelwidth",                     "0.5",      QT_TRANSLATE_NOOP("QObject", "Bevel Width"),            QT_TRANSLATE_NOOP("QObject", "Width of the beveled edges")},
	/*28/1 LBL_CAMERA_BORDER_PERCENT */ {"cameraborderpercentage",         "5.0",      QT_TRANSLATE_NOOP("QObject", "Camera Border Percent"),  QT_TRANSLATE_NOOP("QObject", "When positioning the camera, include a (percentage) border leeway around the model in the rendered image")},
	/*29/2 LBL_DEFAULT_COLOUR        */ {"defaultcolour",                  "16",       QT_TRANSLATE_NOOP("QObject", "Default Color"),         QT_TRANSLATE_NOOP("QObject", "Sets the default piece color using LDraw color code")},
	/*20/3 LBL_GAPS_SIZE             */ {"realgapwidth",                   "0.0002",   QT_TRANSLATE_NOOP("QObject", "Gap Width"),              QT_TRANSLATE_NOOP("QObject", "Amount of space between each piece (default 0.2mm)")},
	/*31/4 LBL_IMAGE_WIDTH           */ {"resolutionwidth",                "800",      QT_TRANSLATE_NOOP("QObject", "Image Width"),            QT_TRANSLATE_NOOP("QObject", "Sets the rendered image width in pixels - from current step image, label shows config setting.")},
	/*32/5 LBL_IMAGE_HEIGHT          */ {"resolutionheight",               "600",      QT_TRANSLATE_NOOP("QObject", "Image Height"),           QT_TRANSLATE_NOOP("QObject", "Sets the rendered image height in pixels - from current step image, label shows config setting.")},
	/*33/6 LBL_IMAGE_SCALE           */ {"realscale",                      "1.0",      QT_TRANSLATE_NOOP("QObject", "Image Scale"),            QT_TRANSLATE_NOOP("QObject", "Sets a scale for the model (1.0 = real life scale)")},
	/*34/6 LBL_RENDER_PERCENTAGE     */ {"renderpercentage",               "100",      QT_TRANSLATE_NOOP("QObject", "Render Percentage"),      QT_TRANSLATE_NOOP("QObject", "Sets the rendered image percentage scale for its pixel resolution - updated from current step, label shows config setting.")},

	/*35/0 LBL_COLOUR_SCHEME         */ {"usecolourscheme",                "lgeo",     QT_TRANSLATE_NOOP("QObject", "Color Scheme"),          QT_TRANSLATE_NOOP("QObject", "Color scheme options - Realistic (lgeo), Original (LDConfig), Alternate (LDCfgalt), Custom (User Defined)")},
	/*36/1 LBL_FLEX_PARTS_SOURCE     */ {"uselsynthparts",                 "1",        QT_TRANSLATE_NOOP("QObject", "Flex Pieces Source"),      QT_TRANSLATE_NOOP("QObject", "Source used to create flexible pieces - string, hoses, etc. (LDCad, LSynth, or both)")},
	/*27/2 LBL_LOGO_STUD_VERSION     */ {"logostudversion",                "4",        QT_TRANSLATE_NOOP("QObject", "Logo Version"),           QT_TRANSLATE_NOOP("QObject", "Which version of the logo to use ('3' (flat), '4' (rounded) or '5' (subtle rounded))")},
	/*38/3 LBL_LOOK                  */ {"uselook",                        "normal",   QT_TRANSLATE_NOOP("QObject", "Look"),                   QT_TRANSLATE_NOOP("QObject", "Photorealistic or schematic instruction look")},
	/*39/4 LBL_POSITION_OBJECT       */ {"positionobjectongroundatorigin", "1",        QT_TRANSLATE_NOOP("QObject", "Position Object"),        QT_TRANSLATE_NOOP("QObject", "The object is centered at the origin, and on the ground plane")},
	/*40/5 LBL_RESOLUTION            */ {"resolution",                     "Standard", QT_TRANSLATE_NOOP("QObject", "Resolution"),             QT_TRANSLATE_NOOP("QObject", "Resolution of piece primitives, i.e., how much geometry they have")},
	/*41/6 LBL_RESOLVE_NORMALS       */ {"resolvenormals",                 "guess",    QT_TRANSLATE_NOOP("QObject", "Resolve Normals"),        QT_TRANSLATE_NOOP("QObject", "Some older LDraw pieces have faces with ambiguous normals. This specifies what to do with them.")}
};

lcBlenderPreferences::ComboItems  lcBlenderPreferences::mComboItems [NUM_COMBO_ITEMS] =
{
	/*    FIRST item set as default        Data                                  Item:   */
	/* 00 LBL_COLOUR_SCHEME            */ {"lgeo|ldraw|alt|custom",  QT_TRANSLATE_NOOP("QObject", "Realistic Colors|Original LDraw Colors|Alternate LDraw Colors|Custom Colors")},
	/* 01 LBL_FLEX_PARTS_SOURCE (t/f)  */ {"1|0|1",                  QT_TRANSLATE_NOOP("QObject", "LSynth|LDCad|LDCad and LSynth")},
	/* 02 LBL_LOGO_STUD_VERSION        */ {"4|3|5",                  QT_TRANSLATE_NOOP("QObject", "Rounded (4)|Flattened (3)|Subtle Rounded (5)")},
	/* 03 LBL_LOOK                     */ {"normal|instructions",    QT_TRANSLATE_NOOP("QObject", "Photorealistic|LEGO Instructions")},
	/* 04 LBL_POSITION_OBJECT (t/f)    */ {"1|0",                    QT_TRANSLATE_NOOP("QObject", "Centered at Origin on Ground|Centered at Origin")},
	/* 05 LBL_RESOLUTION               */ {"Standard|High|Low",      QT_TRANSLATE_NOOP("QObject", "Standard Primitives|High Resolution Primitives|Low Resolution Primitives")},
	/* 06 LBL_RESOLVE_NORMALS          */ {"guess|double",           QT_TRANSLATE_NOOP("QObject", "Recalculate Normals|Two Faces Back to Back")}
};

lcBlenderPreferences::BlenderSettings  lcBlenderPreferences::mBlenderSettingsMM [NUM_SETTINGS_MM];
lcBlenderPreferences::BlenderSettings  lcBlenderPreferences::mDefaultSettingsMM [NUM_SETTINGS_MM] =
{
	/*                                                Key:                             Value:                    Label                                    Tooltip (Description)*/
	/* 00 LBL_ADD_ENVIRONMENT_MM                  */ {"addenvironment",                "1",          QT_TRANSLATE_NOOP("QObject", "Add Environment"),          QT_TRANSLATE_NOOP("QObject", "Adds a ground plane and environment texture")},
	/* 01 LBL_BEVEL_EDGES_MM                      */ {"beveledges",                    "0",          QT_TRANSLATE_NOOP("QObject", "Bevel Edges"),             QT_TRANSLATE_NOOP("QObject", "Bevel edges. Can cause some pieces to render incorrectly.")},
	/* 02 LBL_BLEND_FILE_TRUSTED_MM               */ {"blendfiletrusted",              "0",          QT_TRANSLATE_NOOP("QObject", "Trusted Blend File"),       QT_TRANSLATE_NOOP("QObject", "Specify whether to treat the .blend file as being loaded from a trusted source")},
#ifdef Q_OS_LINUX
	/* 03 LBL_CASE_SENSITIVE_FILESYSTEM           */ {"casesensitivefilesystem",       "1",          QT_TRANSLATE_NOOP("QObject", "Case-sensitive Filesystem"),QT_TRANSLATE_NOOP("QObject", "Filesystem is case sensitive. Defaults to true on Linux.")},
#else
	/* 03 LBL_CASE_SENSITIVE_FILESYSTEM           */ {"casesensitivefilesystem",       "0",          QT_TRANSLATE_NOOP("QObject", "Case-sensitive Filesystem"),QT_TRANSLATE_NOOP("QObject", "Filesystem case sensitivity defaults to false on Windows and macOS. Set to true if the LDraw path is case-sensitive on a case-insensitive filesystem.")},
#endif
	/* 04 LBL_CROP_IMAGE_MM                       */ {"cropimage",                     "0",          QT_TRANSLATE_NOOP("QObject", "Crop Image"),               QT_TRANSLATE_NOOP("QObject", "Crop the image border at opaque content. Requires transparent background set to True.")},
	/* 05 LBL_DISPLAY_LOGO                        */ {"displaylogo",                   "1",          QT_TRANSLATE_NOOP("QObject", "Display Logo"),             QT_TRANSLATE_NOOP("QObject", "Display the logo on the stud")},
	/* 06 LBL_IMPORT_CAMERAS_MM                   */ {"importcameras",                 "1",          QT_TRANSLATE_NOOP("QObject", "Import Cameras"),           QT_TRANSLATE_NOOP("QObject", "%1 can specify camera definitions within the LDraw data. Choose to load them or ignore them.")},
	/* 07 LBL_IMPORT_EDGES                        */ {"importedges",                   "0",          QT_TRANSLATE_NOOP("QObject", "Import Edges"),             QT_TRANSLATE_NOOP("QObject", "Import LDraw edges as edges")},
	/* 08 LBL_IMPORT_LIGHTS_MM                    */ {"importlights",                  "1",          QT_TRANSLATE_NOOP("QObject", "Import Lights"),            QT_TRANSLATE_NOOP("QObject", "%1 can specify point and sunlight definitions within the LDraw data. Choose to load them or ignore them.")},
	/* 09 LBL_KEEP_ASPECT_RATIO_MM                */ {"keepaspectratio",               "1",          QT_TRANSLATE_NOOP("QObject", "Keep Aspect Ratio"),        QT_TRANSLATE_NOOP("QObject", "Maintain the aspect ratio when resizing the output image - this attribute is not passed to Blender")},
	/* 10 LBL_MAKE_GAPS                           */ {"makegaps",                      "1",          QT_TRANSLATE_NOOP("QObject", "Make Gaps"),                QT_TRANSLATE_NOOP("QObject", "Make small gaps between bricks. A small gap is more realistic.")},
	/* 11 LBL_META_BFC                            */ {"metabfc",                       "1",          QT_TRANSLATE_NOOP("QObject", "BFC"),                      QT_TRANSLATE_NOOP("QObject", "Process LDraw Back Face Culling meta commands")},
	/* 12 LBL_META_CLEAR                          */ {"metaclear",                     "0",          QT_TRANSLATE_NOOP("QObject", "CLEAR Command"),            QT_TRANSLATE_NOOP("QObject", "Hides all pieces in the timeline up to where this command is encountered")},
	/* 13 LBL_META_GROUP                          */ {"metagroup",                     "1",          QT_TRANSLATE_NOOP("QObject", "GROUP Command"),            QT_TRANSLATE_NOOP("QObject", "Process GROUP meta commands")},
	/* 14 LBL_META_PAUSE                          */ {"metapause",                     "0",          QT_TRANSLATE_NOOP("QObject", "PAUSE Command"),            QT_TRANSLATE_NOOP("QObject", "Not implemented")},
	/* 15 LBL_META_PRINT_WRITE                    */ {"metaprintwrite",                "0",          QT_TRANSLATE_NOOP("QObject", "PRINT/WRITE Command"),      QT_TRANSLATE_NOOP("QObject", "Prints PRINT/WRITE META commands to the system console.")},
	/* 16 LBL_META_SAVE                           */ {"metasave",                      "0",          QT_TRANSLATE_NOOP("QObject", "SAVE Command"),             QT_TRANSLATE_NOOP("QObject", "Not implemented")},
	/* 17 LBL_META_STEP                           */ {"metastep",                      "0",          QT_TRANSLATE_NOOP("QObject", "STEP Command"),             QT_TRANSLATE_NOOP("QObject", "Adds a keyframe that shows the piece at the moment in the timeline")},
	/* 18 LBL_META_STEP_GROUPS                    */ {"metastepgroups",                "0",          QT_TRANSLATE_NOOP("QObject", "STEP Groups"),              QT_TRANSLATE_NOOP("QObject", "Create collection for individual steps")},
	/* 19 LBL_META_TEXMAP                         */ {"metatexmap",                    "1",          QT_TRANSLATE_NOOP("QObject", "TEXMAP and DATA Command"),  QT_TRANSLATE_NOOP("QObject", "Process TEXMAP and DATA meta commands")},
	/* 20 LBL_NO_STUDS                            */ {"nostuds",                       "0",          QT_TRANSLATE_NOOP("QObject", "No Studs"),                 QT_TRANSLATE_NOOP("QObject", "Don't import studs")},
	/* 21 LBL_OVERWRITE_IMAGE_MM                  */ {"overwriteimage",                "1",          QT_TRANSLATE_NOOP("QObject", "Overwrite Image"),          QT_TRANSLATE_NOOP("QObject", "Specify whether to overwrite an existing rendered image file")},
	/* 22 LBL_POSITION_CAMERA_MM                  */ {"positioncamera",                "1",          QT_TRANSLATE_NOOP("QObject", "Position Camera"),          QT_TRANSLATE_NOOP("QObject", "Position the camera to show the whole model")},
	/* 23 LBL_PARENT_TO_EMPTY                     */ {"parenttoempty",                 "1",          QT_TRANSLATE_NOOP("QObject", "Parent to Empty"),          QT_TRANSLATE_NOOP("QObject", "Parent the model to an empty")},
	/* 24 LBL_PREFER_STUDIO                       */ {"preferstudio",                  "0",          QT_TRANSLATE_NOOP("QObject", "Prefer Stud.io Library"),   QT_TRANSLATE_NOOP("QObject", "Search for pieces in Stud.io library first")},
	/* 25 LBL_PREFER_UNOFFICIAL                   */ {"preferunofficial",              "0",          QT_TRANSLATE_NOOP("QObject", "Prefer Unofficial Pieces"),  QT_TRANSLATE_NOOP("QObject", "Search for unofficial pieces first")},
	/* 26 LBL_PROFILE                             */ {"profile",                       "0",          QT_TRANSLATE_NOOP("QObject", "Profile"),                  QT_TRANSLATE_NOOP("QObject", "Profile import performance")},
	/* 27 LBL_RECALCULATE_NORMALS                 */ {"recalculatenormals",            "0",          QT_TRANSLATE_NOOP("QObject", "Recalculate Normals"),      QT_TRANSLATE_NOOP("QObject", "Recalculate normals. Not recommended if BFC processing is active.")},
	/* 28 LBL_REMOVE_DOUBLES_MM                   */ {"removedoubles",                 "1",          QT_TRANSLATE_NOOP("QObject", "No Duplicate Vertices"),    QT_TRANSLATE_NOOP("QObject", "Merge vertices that are within a certain distance.")},
	/* 29 LBL_RENDER_WINDOW_MM                    */ {"renderwindow",                  "1",          QT_TRANSLATE_NOOP("QObject", "Display Render Window"),    QT_TRANSLATE_NOOP("QObject", "Specify whether to display the render window during Blender user interface image file render")},
	/* 30 LBL_SEARCH_ADDL_PATHS_MM                */ {"searchadditionalpaths",         "0",          QT_TRANSLATE_NOOP("QObject", "Search Additional Paths"),  QT_TRANSLATE_NOOP("QObject", "Specify whether to search additional LDraw paths")},
	/* 31 LBL_SETEND_FRAME                        */ {"setendframe",                   "1",          QT_TRANSLATE_NOOP("QObject", "Set Step End Frame"),       QT_TRANSLATE_NOOP("QObject", "Set the end frame to the last step")},
	/* 32 LBL_SET_TIMELINE_MARKERS                */ {"settimelinemarkers",            "0",          QT_TRANSLATE_NOOP("QObject", "Set Timeline Markers"),     QT_TRANSLATE_NOOP("QObject", "Set timeline markers for meta commands")},
	/* 33 LBL_SHADE_SMOOTH                        */ {"shadesmooth",                   "1",          QT_TRANSLATE_NOOP("QObject", "Shade Smooth"),             QT_TRANSLATE_NOOP("QObject", "Use flat or smooth shading for piece faces")},
	/* 34 LBL_TRANSPARENT_BACKGROUND_MM           */ {"transparentbackground",         "0",          QT_TRANSLATE_NOOP("QObject", "Transparent Background"),   QT_TRANSLATE_NOOP("QObject", "Specify whether to render a background")},
	/* 35 LBL_TREAT_SHORTCUT_AS_MODEL             */ {"treatshortcutasmodel",          "0",          QT_TRANSLATE_NOOP("QObject", "Treat Shortcuts as Models"),QT_TRANSLATE_NOOP("QObject", "Split shortcut pieces into their constituent pieces as if they were models")},
	/* 36 LBL_TRIANGULATE                         */ {"triangulate",                   "0",          QT_TRANSLATE_NOOP("QObject", "Triangulate Faces"),        QT_TRANSLATE_NOOP("QObject", "Triangulate all faces")},
	/* 37 LBL_USE_ARCHIVE_LIBRARY_MM              */ {"usearchivelibrary",             "0",          QT_TRANSLATE_NOOP("QObject", "Use Archive Libraries"),    QT_TRANSLATE_NOOP("QObject", "Add any archive (zip) libraries in the LDraw file path to the library search list - impacts performance")},
	/* 38 LBL_USE_FREESTYLE_EDGES                 */ {"usefreestyleedges",             "0",          QT_TRANSLATE_NOOP("QObject", "Use Freestyle Edges"),      QT_TRANSLATE_NOOP("QObject", "Render LDraw edges using freestyle")},
	/* 39 LBL_VERBOSE_MM                          */ {"verbose",                       "1",          QT_TRANSLATE_NOOP("QObject", "Verbose Output"),           QT_TRANSLATE_NOOP("QObject", "Output all messages while working, else only show warnings and errors")},

	/* 40/00 LBL_BEVEL_SEGMENTS                   */ {"bevelsegments",                 "4",          QT_TRANSLATE_NOOP("QObject", "Bevel Segments"),           QT_TRANSLATE_NOOP("QObject", "Bevel segments")},
	/* 41/01 LBL_BEVEL_WEIGHT                     */ {"bevelweight",                   "0.03",       QT_TRANSLATE_NOOP("QObject", "Bevel Weight"),             QT_TRANSLATE_NOOP("QObject", "Bevel weight")},
	/* 42/02 LBL_BEVEL_WIDTH_MM                   */ {"bevelwidth",                    "0.3",        QT_TRANSLATE_NOOP("QObject", "Bevel Width"),              QT_TRANSLATE_NOOP("QObject", "Width of the beveled edges")},
	/* 43/03 LBL_CAMERA_BORDER_PERCENT_MM         */ {"cameraborderpercent",           "5",          QT_TRANSLATE_NOOP("QObject", "Camera Border Percent"),    QT_TRANSLATE_NOOP("QObject", "When positioning the camera, include a (percentage) border around the model in the render")},
	/* 44/04 LBL_FRAMES_PER_STEP                  */ {"framesperstep",                 "3",          QT_TRANSLATE_NOOP("QObject", "Frames per Step"),          QT_TRANSLATE_NOOP("QObject", "Frames per step")},
	/* 45/05 LBL_GAP_SCALE                        */ {"gapscale",                      "0.997",      QT_TRANSLATE_NOOP("QObject", "Gap Scale"),                QT_TRANSLATE_NOOP("QObject", "Scale individual pieces by this much to create the gap")},
	/* 46/06 LBL_IMPORT_SCALE                     */ {"importscale",                   "0.02",       QT_TRANSLATE_NOOP("QObject", "Import Scale"),             QT_TRANSLATE_NOOP("QObject", "What scale to import at. Full scale is 1.0 and is so huge that it is unwieldy in the viewport.")},
	/* 47/07 LBL_MERGE_DISTANCE                   */ {"mergedistance",                 "0.05",       QT_TRANSLATE_NOOP("QObject", "Merge Distance"),           QT_TRANSLATE_NOOP("QObject", "Maximum distance between elements to merge")},
	/* 48/08 LBL_RENDER_PERCENTAGE_MM             */ {"renderpercentage",              "100",        QT_TRANSLATE_NOOP("QObject", "Render Percentage"),        QT_TRANSLATE_NOOP("QObject", "Sets the rendered image percentage scale for its pixel resolution - updated from current step, label shows config setting.")},
	/* 49/09 LBL_RESOLUTION_WIDTH                 */ {"resolutionwidth",               "800",        QT_TRANSLATE_NOOP("QObject", "Image Width"),              QT_TRANSLATE_NOOP("QObject", "Sets the rendered image width in pixels - from current step image, label shows config setting.")},
	/* 50/10 LBL_RESOLUTION_HEIGHT                */ {"resolutionheight",              "600",        QT_TRANSLATE_NOOP("QObject", "Image Height"),             QT_TRANSLATE_NOOP("QObject", "Sets the rendered image height in pixels - from current step image, label shows config setting.")},
	/* 51/11 LBL_STARTING_STEP_FRAME              */ {"startingstepframe",             "1",          QT_TRANSLATE_NOOP("QObject", "Starting Step Frame"),      QT_TRANSLATE_NOOP("QObject", "Frame to add the first STEP meta command")},

	/* 52/00 LBL_CHOSEN_LOGO                      */ {"chosenlogo",                    "logo3",      QT_TRANSLATE_NOOP("QObject", "Chosen Logo"),              QT_TRANSLATE_NOOP("QObject", "Which logo to display. logo and logo2 aren't used and are only included for completeness.")},
	/* 53/01 LBL_COLOUR_SCHEME_MM                 */ {"usecolourscheme",               "alt",        QT_TRANSLATE_NOOP("QObject", "Color Scheme"),            QT_TRANSLATE_NOOP("QObject", "Color scheme options - Realistic (lgeo), Original (LDConfig), Alternate (LDCfgalt), Custom (User Defined)")},
	/* 54/02 LBL_RESOLUTION_MM                    */ {"resolution",                    "Standard",   QT_TRANSLATE_NOOP("QObject", "Resolution"),               QT_TRANSLATE_NOOP("QObject", "Resolution of piece primitives, i.e., how much geometry they have")},
	/* 55/03 LBL_SCALE_STRATEGY                   */ {"scalestrategy",                 "mesh",       QT_TRANSLATE_NOOP("QObject", "How to Scale Pieces"),       QT_TRANSLATE_NOOP("QObject", "Apply import scaling to mesh - Recommended for rendering, Apply import scaling to object - Recommended for piece editing")},
	/* 56/04 LBL_SMOOTH_TYPE                      */ {"smoothtype",                    "edge_split", QT_TRANSLATE_NOOP("QObject", "Smooth Type"),              QT_TRANSLATE_NOOP("QObject", "Use either autosmooth or an edge split modifier to smooth piece faces")}
};

lcBlenderPreferences::ComboItems  lcBlenderPreferences::mComboItemsMM [NUM_COMBO_ITEMS_MM] =
{
	/*    FIRST item set as default        Data                                  Item: */
	/* 00 LBL_CHOSEN_LOGO              */ {"logo3|logo4|logo5",                  QT_TRANSLATE_NOOP("QObject", "Raised flattened logo geometry (3)|Raised rounded logo geometry (4)|Subtle rounded logo geometry (5)")},
	/* 01 LBL_COLOUR_SCHEME_MM         */ {"lgeo|ldraw|alt|custom",              QT_TRANSLATE_NOOP("QObject", "Realistic Colors|Original LDraw Colors|Alternate LDraw Colors|Custom Colors")},
	/* 02 LBL_RESOLUTION_MM            */ {"Low|Standard|High",                  QT_TRANSLATE_NOOP("QObject", "Low Resolution Primitives|Standard Primitives|High Resolution Primitives")},
	/* 03 LBL_SCALE_STRATEGY           */ {"mesh|object",                        QT_TRANSLATE_NOOP("QObject", "Scale Mesh|Scale Object")},
	/* 04 LBL_SMOOTH_TYPE              */ {"edge_split|auto_smooth|bmesh_split", QT_TRANSLATE_NOOP("QObject", "Smooth piece faces with edge split modifier|Auto-smooth piece faces|Split during initial mesh processing")}
};

lcBlenderPreferences* gAddonPreferences;

enum AddonEnc
{
	ADDON_EXTRACT,
	ADDON_DOWNLOAD,
	ADDON_NO_ACTION,
	ADDON_CANCELED,
	ADDON_FAIL
};

lcBlenderPreferencesDialog::lcBlenderPreferencesDialog(int Width, int Height, double Scale, QWidget* Parent)
	: QDialog(Parent)
{
	setWindowTitle(tr("Blender LDraw Add-on Settings"));

	QVBoxLayout* Layout = new QVBoxLayout(this);
	setLayout(Layout);

	QGroupBox* Box = new QGroupBox(this);
	Layout->addWidget(Box);

	mPreferences = new lcBlenderPreferences(Width,Height,Scale,Box);
	setWhatsThis(Box->whatsThis());

	QDialogButtonBox* ButtonBox;
	ButtonBox = new QDialogButtonBox(this);
	mApplyButton = new QPushButton(tr("Apply"), ButtonBox);
	mApplyButton->setToolTip(tr("Save the add-on paths and settings and close this dialog."));
	mApplyButton->setEnabled(false);
	ButtonBox->addButton(mApplyButton, QDialogButtonBox::ActionRole);
	connect(mApplyButton, &QPushButton::clicked, this, &lcBlenderPreferencesDialog::accept);

	mPathsButton = new QPushButton(tr("Hide Paths"), ButtonBox);
	mPathsButton->setToolTip(tr("Hide the add-on paths section."));
	ButtonBox->addButton(mPathsButton,QDialogButtonBox::ActionRole);
	connect(mPathsButton, &QPushButton::clicked, this, &lcBlenderPreferencesDialog::ShowPathsGroup);

	mResetButton = new QPushButton(tr("Reset"), ButtonBox);
	mResetButton->setEnabled(false);
	mResetButton->setToolTip(tr("Restore the last applied paths and settings or the system defaults."));
	ButtonBox->addButton(mResetButton,QDialogButtonBox::ActionRole);
	connect(mResetButton, &QPushButton::clicked, this, &lcBlenderPreferencesDialog::ResetSettings);

	ButtonBox->addButton(QDialogButtonBox::Help);
	lcSetupDialogHelp(this, ButtonBox);
	ButtonBox->addButton(QDialogButtonBox::Cancel);
	connect(ButtonBox, &QDialogButtonBox::rejected, this, &lcBlenderPreferencesDialog::reject);

	if (!QFileInfo(lcGetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH)).isReadable() && !lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty())
		mApplyButton->setEnabled(true);

	connect(mPreferences, &lcBlenderPreferences::SettingChangedSig, this, &lcBlenderPreferencesDialog::EnableButton);

	Layout->addWidget(ButtonBox);

	setMinimumSize(Box->sizeHint().width() + 50, 500);

	setSizeGripEnabled(true);

	setModal(true);
}

lcBlenderPreferencesDialog::~lcBlenderPreferencesDialog()
{
}

bool lcBlenderPreferencesDialog::GetBlenderPreferences(int& Width, int& Height, double & Scale, QWidget* Parent)
{
	lcBlenderPreferencesDialog* Dialog = new lcBlenderPreferencesDialog(Width,Height,Scale,Parent);

	bool Ok = Dialog->exec() == QDialog::Accepted;

	if (Ok)
	{
		Dialog->mPreferences->Apply(Ok);
		Width = Dialog->mPreferences->mImageWidth;
		Height = Dialog->mPreferences->mImageHeight;
		Scale = Dialog->mPreferences->mScale;
	}

	return Ok;
}

void lcBlenderPreferencesDialog::ShowPathsGroup()
{
	const bool HidePaths = mPathsButton->text() == tr("Hide Paths");
	mPathsButton->setText(HidePaths ? tr("Show Paths") : tr("Hide Paths"));
	mPathsButton->setToolTip(HidePaths ? tr("Show the add-on paths section.") : tr("Hide the add-on paths section."));
	mPreferences->ShowPathsGroup();
}

void lcBlenderPreferencesDialog::EnableButton(bool Change)
{
	if (Change)
	{
		mApplyButton->setEnabled(true);
		mResetButton->setEnabled(true);
	}
	mPathsButton->setText(tr("Hide Paths"));
	mPathsButton->setToolTip(tr("Hide the add-on paths section."));
}

void lcBlenderPreferencesDialog::ResetSettings()
{
	mPreferences->ResetSettings();
}

void lcBlenderPreferencesDialog::accept()
{
	for (QLineEdit* Edit : findChildren<QLineEdit*>())
	{
		if (Edit->isEnabled() && !Edit->isReadOnly() && Edit->validator() && !Edit->hasAcceptableInput())
		{
			QMessageBox::information(this, windowTitle(), tr("Please enter a valid value for '%1'.").arg(Edit->accessibleName()));
			Edit->setFocus();
			return;
		}
	}

	mApplyButton->setEnabled(false);
	mPreferences->Apply(QDialog::Accepted);
	QDialog::accept();
}

void lcBlenderPreferencesDialog::reject()
{
	if (mPreferences->PromptCancel())
		QDialog::reject();
}

lcBlenderPreferences::lcBlenderPreferences(int Width, int Height, double Scale, QWidget* Parent)
	: QWidget(Parent)
{
	gAddonPreferences = this;

#ifndef QT_NO_PROCESS
	mProcess = nullptr;
#endif

	mImageWidth = Width;
	mImageHeight = Height;
	mScale = Scale;

	mDialogCancelled = false;

	QVBoxLayout* Layout = new QVBoxLayout(Parent);

	if (Parent)
	{
		Parent->setLayout(Layout);
		Parent->setWhatsThis(QObject::tr(WhatsThisDescription));
	}
	else
	{
		setWindowTitle(tr("Blender LDraw Add-on Settings"));
		setLayout(Layout);
		setWhatsThis(QObject::tr(WhatsThisDescription));
	}

	mContent = new QWidget();

	mForm = new QFormLayout(mContent);

	mContent->setLayout(mForm);

	QPalette ReadOnlyPalette = QApplication::palette();
	if (gApplication->IsDarkTheme())
		ReadOnlyPalette.setColor(QPalette::Base, LC_THEME_DARK_PALETTE_MIDLIGHT);
	else
		ReadOnlyPalette.setColor(QPalette::Base, LC_THEME_DEFAULT_PALETTE_LIGHT);
	ReadOnlyPalette.setColor(QPalette::Text, LC_DISABLED_TEXT);

	QGroupBox* BlenderExeBox = new QGroupBox(tr("Blender Executable"),mContent);
	mForm->addRow(BlenderExeBox);
	BlenderExeBox->setWhatsThis(tr("<p><b>Blender Executable</b></p><p>Choose the Blender executable. Finishing an edit to this path starts Blender configuration and may install or update the LDraw add-on. Configuration must complete before add-on paths and settings become available.</p>"));

	mExeGridLayout = new QGridLayout(BlenderExeBox);
	BlenderExeBox->setLayout(mExeGridLayout);

	mBlenderVersionLabel = new QLabel(BlenderExeBox);
	mExeGridLayout->addWidget(mBlenderVersionLabel,0,0);

	mBlenderVersionEdit = new QLineEdit(BlenderExeBox);
	mBlenderVersionEdit->setPalette(ReadOnlyPalette);
	mBlenderVersionEdit->setReadOnly(true);
	mBlenderVersionEdit->setToolTip(tr("Installed Blender version reported during configuration."));
	mBlenderVersionEdit->setAccessibleName(tr("Blender Version"));
	mBlenderVersionLabel->setBuddy(mBlenderVersionEdit);
	mBlenderVersionLabel->setToolTip(mBlenderVersionEdit->toolTip());
	mExeGridLayout->addWidget(mBlenderVersionEdit,0,1,1,2);

	QLabel* PathLabel = new QLabel(BlenderExeBox);
	mExeGridLayout->addWidget(PathLabel,1,0);

	QLineEdit* PathLineEdit = new QLineEdit(BlenderExeBox);
	mExeGridLayout->addWidget(PathLineEdit,1,1);
	mPathLineEditList << PathLineEdit;
	connect(PathLineEdit, &QLineEdit::editingFinished, this, [this]() { ConfigureBlenderAddon(); });

	QPushButton* PathBrowseButton = new QPushButton(tr("Browse..."), BlenderExeBox);
	mExeGridLayout->addWidget(PathBrowseButton,1,2);
	mPathBrowseButtonList << PathBrowseButton;
	PathBrowseButton->setToolTip(tr("Locate the Blender executable."));
	connect(PathBrowseButton, &QPushButton::clicked, this, &lcBlenderPreferences::BrowseBlender);

	QGroupBox* BlenderAddonVersionBox = new QGroupBox(tr("%1 Blender LDraw Add-on").arg(LC_PRODUCTNAME_STR),mContent);
	mForm->addRow(BlenderAddonVersionBox);
	BlenderAddonVersionBox->setWhatsThis(tr("<p><b>Blender LDraw Add-on</b></p><p>Update downloads or reconfigures the LDraw add-on. Output opens the configuration log when available. The update-check preference takes effect as soon as you change it.</p>"));

	mAddonGridLayout = new QGridLayout(BlenderAddonVersionBox);
	BlenderAddonVersionBox->setLayout(mAddonGridLayout);

	QCheckBox* AddonVersionCheck = new QCheckBox(tr("Prompt to download new add-on version when available"), BlenderAddonVersionBox);
	AddonVersionCheck->setToolTip(tr("Check for add-on updates; this preference is saved immediately."));
	AddonVersionCheck->setChecked(lcGetProfileInt(LC_PROFILE_BLENDER_ADDON_VERSION_CHECK));
	QObject::connect(AddonVersionCheck, &QCheckBox::stateChanged, [](int State)
	{
		 const bool VersionCheck = static_cast<Qt::CheckState>(State) == Qt::CheckState::Checked;
		 lcSetProfileInt(LC_PROFILE_BLENDER_ADDON_VERSION_CHECK, (int)VersionCheck);
	});
	mAddonGridLayout->addWidget(AddonVersionCheck,0,0,1,4);

	mAddonVersionLabel = new QLabel(BlenderAddonVersionBox);
	mAddonGridLayout->addWidget(mAddonVersionLabel,1,0);

	mAddonVersionEdit = new QLineEdit(BlenderAddonVersionBox);
	mAddonVersionEdit->setToolTip(tr("Installed %1 Blender LDraw import and render add-on version.").arg(LC_PRODUCTNAME_STR));
	mAddonVersionEdit->setPalette(ReadOnlyPalette);
	mAddonVersionEdit->setReadOnly(true);
	mAddonVersionEdit->setAccessibleName(tr("Blender LDraw Add-on Version"));
	mAddonVersionLabel->setBuddy(mAddonVersionEdit);
	mAddonVersionLabel->setToolTip(mAddonVersionEdit->toolTip());
	mAddonGridLayout->addWidget(mAddonVersionEdit,1,1);
	mAddonGridLayout->setColumnStretch(1,1/*1 is greater than 0 (default)*/);

	mAddonUpdateButton = new QPushButton(tr("Update"), BlenderAddonVersionBox);
	mAddonUpdateButton->setToolTip(tr("Download or reconfigure the %1 Blender LDraw add-on.").arg(LC_PRODUCTNAME_STR));
	mAddonGridLayout->addWidget(mAddonUpdateButton,1,2);
	connect(mAddonUpdateButton, &QPushButton::clicked, this, &lcBlenderPreferences::UpdateBlenderAddon);

	mAddonStdOutButton = new QPushButton(tr("Output..."), BlenderAddonVersionBox);
	mAddonStdOutButton->setToolTip(tr("Open the standard output log."));
	mAddonStdOutButton->setEnabled(false);
	mAddonGridLayout->addWidget(mAddonStdOutButton,1,3);
	connect(mAddonStdOutButton, &QPushButton::clicked, this, &lcBlenderPreferences::GetStandardOutput);

	mModulesBox = new QGroupBox(tr("Enabled Add-on Modules"),mContent);
	QHBoxLayout* ModulesLayout = new QHBoxLayout(mModulesBox);
	mModulesBox->setLayout(ModulesLayout);
	mModulesBox->setWhatsThis(tr("<p><b>Import and Render Modules</b></p><p>Choose TN or MM to select the LDraw importer and its options. Changing the importer configures Blender immediately. The image render module is managed automatically; an import module is required to import or render a model.</p>"));
	mAddonGridLayout->addWidget(mModulesBox,2,0,1,4);

	mImportActBox = new QCheckBox(tr("LDraw Import TN"),mModulesBox);
	mImportActBox->setToolTip(tr("Enable add-on import module (adapted from LDraw Import by Toby Nelson) in Blender."));
	ModulesLayout->addWidget(mImportActBox);
	connect(mImportActBox, &QCheckBox::clicked, this, &lcBlenderPreferences::EnableImportModule);

	mImportMMActBox = new QCheckBox(tr("LDraw Import MM"),mModulesBox);
	mImportMMActBox->setToolTip(tr("Enable add-on import module (adapted from LDraw Import by Matthew Morrison) in Blender."));
	ModulesLayout->addWidget(mImportMMActBox);
	connect(mImportMMActBox, &QCheckBox::clicked, this, &lcBlenderPreferences::EnableImportModule);

	mRenderActBox = new QCheckBox(tr("%1 Image Render").arg(LC_PRODUCTNAME_STR), mModulesBox);
	mRenderActBox->setToolTip(tr("Add-on image render module in Blender."));
	mRenderActBox->setEnabled(false);
	ModulesLayout->addWidget(mRenderActBox);

	LoadSettings();

	mConfigured = !lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty();

	const int CtlIdx = PATH_BLENDER;
	PathLabel->setText(mBlenderPaths[CtlIdx].label);
	PathLabel->setToolTip(mBlenderPaths[CtlIdx].tooltip);

	PathLineEdit->setText(mBlenderPaths[CtlIdx].value);
	PathLineEdit->setToolTip(mBlenderPaths[CtlIdx].tooltip);
	SetupControlHelp(PathLabel, PathLineEdit);

	if (mAddonVersion.isEmpty())
	{
		mModulesBox->setEnabled(false);
		mAddonUpdateButton->setEnabled(true);
		mImportActBox->setChecked(true); // default addon module
	}
	else
	{
		mAddonVersionEdit->setText(mAddonVersion);
		mRenderActBox->setChecked(true);
		mImportActBox->setChecked(lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE) == QLatin1String("TN"));
		mImportMMActBox->setChecked(lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE) == QLatin1String("MM"));
	}

	QString VersionTextColour = QApplication::palette().text().color().name(),AddonTextColour;
	QString VersionText = tr("Blender"), AddonText = tr("Blender Add-on");
	if (mConfigured)
	{
		AddonTextColour = VersionTextColour;
		mBlenderVersionEdit->setText(mBlenderVersion);
		mBlenderVersionEdit->setVisible(true);
	}
	else
	{
		const QString TextColour = gApplication->IsDarkTheme() ? QLatin1String(LC_THEME_DARK_DECORATE_QUOTED_TEXT) : QLatin1String("blue");

		bool BlenderConfigured = !lcGetProfileString(LC_PROFILE_BLENDER_PATH).isEmpty();
		if (!BlenderConfigured)
		{
			VersionText = tr("Blender not configured.");
			VersionTextColour = TextColour;
		}
		else
			mBlenderVersionEdit->setText(mBlenderVersion);

		if (QFileInfo(QString("%1/Blender/%2").arg(mDataDir, LC_BLENDER_ADDON_FILE)).isReadable())
		{
			mModulesBox->setEnabled(false);
			mImportActBox->setChecked(true);
			mAddonUpdateButton->setEnabled(true);
			if (BlenderConfigured)
				AddonText = tr("Add-on not configured - Update.");
			else
				AddonText = tr("Add-on not configured.");
			AddonTextColour = TextColour;
		}
	}

	mAddonVersionEdit->setVisible(!mAddonVersion.isEmpty());
	mBlenderVersionLabel->setStyleSheet(QString("QLabel { color : %1; }").arg(VersionTextColour));
	mAddonVersionLabel->setStyleSheet(QString("QLabel { color : %1; }").arg(AddonTextColour));
	mBlenderVersionLabel->setText(VersionText);
	mAddonVersionLabel->setText(AddonText);

	if (mImportMMActBox->isChecked())
		InitPathsAndSettingsMM();
	else
		InitPathsAndSettings();

	QScrollArea* ScrollArea = new QScrollArea(this);
	ScrollArea->setWidgetResizable(true);
	ScrollArea->setWidget(mContent);
	Layout->addWidget(ScrollArea);
}

lcBlenderPreferences::~lcBlenderPreferences()
{
	gAddonPreferences = nullptr;
}

void lcBlenderPreferences::ClearGroupBox(QGroupBox* GroupBox)
{
	int Row = -1;
	QFormLayout::ItemRole ItemRole = QFormLayout::SpanningRole;

	mForm->getWidgetPosition(GroupBox,& Row,& ItemRole);

	if(Row == -1 || ItemRole != QFormLayout::SpanningRole) return;

	QLayoutItem* GroupBoxIitem = mForm->itemAt(Row, ItemRole);

	mForm->removeItem(GroupBoxIitem);

	QWidget* WidgetItem = GroupBoxIitem->widget();
	if(WidgetItem)
	{
		if (GroupBox == mPathsBox)
		{
			delete mPathsBox;
			mPathsBox = nullptr;
		}
		else if (GroupBox == mSettingsBox)
		{
			delete mSettingsBox;
			mSettingsBox = nullptr;
		}
	}

	delete GroupBoxIitem;
}

void lcBlenderPreferences::SetupControlHelp(QLabel* Label, QWidget* Control)
{
	Label->setBuddy(Control);
	Control->setAccessibleName(Label->text());
	Control->setWhatsThis(QString("<p><b>%1</b></p><p>%2</p>").arg(Label->text().toHtmlEscaped(), Control->toolTip().toHtmlEscaped()));
	Label->setWhatsThis(Control->whatsThis());
}

void lcBlenderPreferences::InitPathsAndSettings()
{
	if (!NumSettings())
		LoadSettings();

	// Paths
	ClearGroupBox(mPathsBox);
	mPathsBox = new QGroupBox(mContent);
	mPathsBox->setTitle(tr("LDraw Import TN Add-on Paths"));
	mPathsGridLayout = new QGridLayout(mPathsBox);
	mPathsBox->setLayout(mPathsGridLayout);
	mPathsBox->setWhatsThis(tr("<p><b>Add-on Paths</b></p><p>Set the files and folders used by the selected importer. Use Browse to locate an entry. TN and MM use different optional paths. Configure Blender first to enable these controls.</p>"));
	mForm->addRow(mPathsBox);

	for (int CtlIdx = 1/*skip blender executable*/; CtlIdx < NumPaths(); ++CtlIdx)
	{
		int ColumnIdx = CtlIdx - 1; // adjust for skipping first item - blender executable
		bool IsVisible = CtlIdx != PATH_STUDIO_LDRAW;
		QLabel* PathLabel = new QLabel(mBlenderPaths[CtlIdx].label, mPathsBox);
		PathLabel->setToolTip(mBlenderPaths[CtlIdx].tooltip);
		mPathsGridLayout->addWidget(PathLabel,ColumnIdx,0);
		PathLabel->setVisible(IsVisible);

		QLineEdit* PathLineEdit = new QLineEdit(mPathsBox);
		PathLineEdit->setProperty("ControlID",QVariant(CtlIdx));
		PathLineEdit->setText(mBlenderPaths[CtlIdx].value);
		PathLineEdit->setToolTip(mBlenderPaths[CtlIdx].tooltip);
		SetupControlHelp(PathLabel, PathLineEdit);
		if (mPathLineEditList.size() > CtlIdx)
			mPathLineEditList.replace(CtlIdx, PathLineEdit);
		else
			mPathLineEditList << PathLineEdit;
		mPathsGridLayout->addWidget(PathLineEdit,ColumnIdx,1);
		PathLineEdit->setVisible(IsVisible);

		QPushButton* PathBrowseButton = new QPushButton(tr("Browse..."), mPathsBox);
		PathBrowseButton->setToolTip(tr("Choose a file or folder for %1.").arg(PathLabel->text()));
		PathBrowseButton->setWhatsThis(PathLineEdit->whatsThis());
		if (mPathBrowseButtonList.size() > CtlIdx)
			mPathBrowseButtonList.replace(CtlIdx, PathBrowseButton);
		else
			mPathBrowseButtonList << PathBrowseButton;
		mPathsGridLayout->addWidget(PathBrowseButton,ColumnIdx,2);
		PathBrowseButton->setVisible(IsVisible);

		if (IsVisible)
		{
			connect(PathBrowseButton, &QPushButton::clicked, this, &lcBlenderPreferences::BrowseBlender);
			connect(PathLineEdit, &QLineEdit::editingFinished, this, &lcBlenderPreferences::PathChanged);
		}
	}

	mPathsBox->setEnabled(mConfigured);

	// Settings
	ClearGroupBox(mSettingsBox);
	mSettingsBox = new QGroupBox(mContent);
	mSettingsSubform = new QFormLayout(mSettingsBox);
	mSettingsBox->setLayout(mSettingsSubform);
	mSettingsBox->setWhatsThis(tr("<p><b>Import and Render Settings</b></p><p>Adjust the selected importer's geometry, materials, camera, and image options. Hover over a control or press F1 for its description. Image width and height are in pixels, from 1 to 32768; render percentage scales the output dimensions.</p>"));
	mForm->addRow(mSettingsBox);

	mSettingsBox->setTitle(tr("LDraw Import TN Add-on Settings"));
	mSettingLabelList.clear();
	mCheckBoxList.clear();
	mLineEditList.clear();
	mComboBoxList.clear();

	int ComboBoxItemsIndex = 0;

	for (int LblIdx = 0; LblIdx < NumSettings(); LblIdx++)
	{
		QLabel* Label = new QLabel(mSettingsBox);
		Label->setText(mBlenderSettings[LblIdx].label);
		Label->setToolTip(mBlenderSettings[LblIdx].tooltip);
		mSettingLabelList << Label;

		if (LblIdx < LBL_BEVEL_WIDTH)
		{   // QCheckBoxes
			QCheckBox* CheckBox = new QCheckBox(mSettingsBox);
			CheckBox->setProperty("ControlID",QVariant(LblIdx));
			CheckBox->setChecked(mBlenderSettings[LblIdx].value.toInt());
			CheckBox->setToolTip(mBlenderSettings[LblIdx].tooltip);
			SetupControlHelp(Label, CheckBox);
			if (LblIdx == LBL_CROP_IMAGE)
				connect(CheckBox, &QCheckBox::toggled, this, &lcBlenderPreferences::SetModelSize);
			else
				connect(CheckBox, &QCheckBox::clicked, this, [this]() { SettingIndexChanged(); });
			mCheckBoxList << CheckBox;
			mSettingsSubform->addRow(Label,CheckBox);
		}
		else if (LblIdx < LBL_COLOUR_SCHEME)
		{   // QLineEdits
			QLineEdit* LineEdit = new QLineEdit(mSettingsBox);
			LineEdit->setProperty("ControlID",QVariant(LblIdx));
			if (LblIdx == LBL_IMAGE_WIDTH || LblIdx == LBL_IMAGE_HEIGHT)
			{
				connect(LineEdit, &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
				LineEdit->setValidator(new QIntValidator(1, LC_MAX_IMAGE_DIMENSION));
			}
			else if(LblIdx == LBL_DEFAULT_COLOUR)
			{
				LineEdit->setReadOnly(true);
				LineEdit->setStyleSheet("Text-align:left");
				mDefaultColourEditAction = LineEdit->addAction(QIcon(), QLineEdit::TrailingPosition);
				connect(mDefaultColourEditAction, &QAction::triggered, this, &lcBlenderPreferences::ColorButtonClicked);
			}
			else
			{
				if (LblIdx == LBL_IMAGE_SCALE)
					LineEdit->setValidator(new QDoubleValidator(0.01,10.0,2));
				else if (LblIdx == LBL_RENDER_PERCENTAGE || LblIdx == LBL_CAMERA_BORDER_PERCENT)
					LineEdit->setValidator(new QIntValidator(1,1000));
				else
					LineEdit->setValidator(new QDoubleValidator(0.01,100.0,2));
				SetNumericText(LineEdit, mBlenderSettings[LblIdx].value);
				connect(LineEdit, &QLineEdit::textEdited, this, &lcBlenderPreferences::SettingTextChanged);
			}
			LineEdit->setToolTip(mBlenderSettings[LblIdx].tooltip);
			SetupControlHelp(Label, LineEdit);
			mLineEditList << LineEdit;
			if (LblIdx == LBL_DEFAULT_COLOUR)
				SetDefaultColor(lcGetColorIndex(mBlenderSettings[LBL_DEFAULT_COLOUR].value.toInt()));
			mSettingsSubform->addRow(Label,LineEdit);
		}
		else
		{   // QComboBoxes
			QComboBox* ComboBox = new QComboBox(mSettingsBox);
			ComboBox->setProperty("ControlID",QVariant(LblIdx));
			const QString Value = mBlenderSettings[LblIdx].value;
			QStringList const DataList = mComboItems[ComboBoxItemsIndex].dataList.split("|");
			QStringList const ItemList = TranslateDefaultText(mComboItems[ComboBoxItemsIndex].itemList).split("|");
			ComboBox->addItems(ItemList);
			for (int CtlIdx = 0; CtlIdx < ComboBox->count(); CtlIdx++)
				ComboBox->setItemData(CtlIdx, DataList.at(CtlIdx));
			ComboBox->setToolTip(mBlenderSettings[LblIdx].tooltip);
			SetupControlHelp(Label, ComboBox);
			int CurrentIndex = int(ComboBox->findData(QVariant::fromValue(Value)));
			ComboBox->setCurrentIndex(CurrentIndex);
			if (LblIdx == LBL_COLOUR_SCHEME)
				connect(ComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &lcBlenderPreferences::ValidateColourScheme);
			else
				connect(ComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &lcBlenderPreferences::SettingIndexChanged);
			mComboBoxList << ComboBox;
			ComboBoxItemsIndex++;
			mSettingsSubform->addRow(Label,ComboBox);
		}
	}

	SetModelSize();

	if (!mSettingsSubform->rowCount())
		mSettingsSubform = nullptr;

	mSettingsBox->setEnabled(mConfigured);
}

void lcBlenderPreferences::InitPathsAndSettingsMM()
{
	if (!NumSettingsMM())
		LoadSettings();

	// Paths
	ClearGroupBox(mPathsBox);
	mPathsBox = new QGroupBox(mContent);
	mPathsBox->setTitle(tr("LDraw Import MM Add-on Paths"));
	mPathsGridLayout = new QGridLayout(mPathsBox);
	mPathsBox->setLayout(mPathsGridLayout);
	mPathsBox->setWhatsThis(tr("<p><b>Add-on Paths</b></p><p>Set the files and folders used by the selected importer. Use Browse to locate an entry. TN and MM use different optional paths. Configure Blender first to enable these controls.</p>"));
	mForm->addRow(mPathsBox);

	for (int CtlIdx = 1/*skip blender executable*/; CtlIdx < NumPaths(); ++CtlIdx)
	{
		int ColumnIdx = CtlIdx - 1; // adjust for skipping first item - blender executable
		bool IsVisible = CtlIdx != PATH_LSYNTH && CtlIdx != PATH_STUD_LOGO;
		QLabel* PathLabel = new QLabel(mBlenderPaths[CtlIdx].label, mPathsBox);
		PathLabel->setToolTip(mBlenderPaths[CtlIdx].tooltip);
		mPathsGridLayout->addWidget(PathLabel,ColumnIdx,0);
		PathLabel->setVisible(IsVisible);

		QLineEdit* PathLineEdit = new QLineEdit(mPathsBox);
		PathLineEdit->setProperty("ControlID",QVariant(CtlIdx));
		PathLineEdit->setText(mBlenderPaths[CtlIdx].value);
		PathLineEdit->setToolTip(mBlenderPaths[CtlIdx].tooltip);
		SetupControlHelp(PathLabel, PathLineEdit);
		if (mPathLineEditList.size() > CtlIdx)
			mPathLineEditList.replace(CtlIdx, PathLineEdit);
		else
			mPathLineEditList << PathLineEdit;
		mPathsGridLayout->addWidget(PathLineEdit,ColumnIdx,1);
		PathLineEdit->setVisible(IsVisible);

		QPushButton* PathBrowseButton = new QPushButton(tr("Browse..."), mPathsBox);
		PathBrowseButton->setToolTip(tr("Choose a file or folder for %1.").arg(PathLabel->text()));
		PathBrowseButton->setWhatsThis(PathLineEdit->whatsThis());
		if (mPathBrowseButtonList.size() > CtlIdx)
			mPathBrowseButtonList.replace(CtlIdx, PathBrowseButton);
		else
			mPathBrowseButtonList << PathBrowseButton;
		mPathsGridLayout->addWidget(PathBrowseButton,ColumnIdx,2);
		PathBrowseButton->setVisible(IsVisible);

		if (IsVisible)
		{
			connect(PathBrowseButton, &QPushButton::clicked, this, &lcBlenderPreferences::BrowseBlender);
			connect(PathLineEdit, &QLineEdit::editingFinished, this, &lcBlenderPreferences::PathChanged);
		}
	}

	mPathsBox->setEnabled(mConfigured);

	// Settings
	ClearGroupBox(mSettingsBox);
	mSettingsBox = new QGroupBox(mContent);
	mSettingsSubform = new QFormLayout(mSettingsBox);
	mSettingsBox->setLayout(mSettingsSubform);
	mSettingsBox->setWhatsThis(tr("<p><b>Import and Render Settings</b></p><p>Adjust the selected importer's geometry, materials, camera, and image options. Hover over a control or press F1 for its description. Image width and height are in pixels, from 1 to 32768; render percentage scales the output dimensions.</p>"));
	mForm->addRow(mSettingsBox);

	mSettingsBox->setTitle(tr("LDraw Import MM Add-on Settings"));
	mSettingLabelList.clear();
	mCheckBoxList.clear();
	mLineEditList.clear();
	mComboBoxList.clear();

	int ComboBoxItemsIndex = 0;

	for (int LblIdx = 0; LblIdx < NumSettingsMM(); LblIdx++)
	{
		QLabel* Label = new QLabel(mSettingsBox);
		Label->setText(mBlenderSettingsMM[LblIdx].label);
		Label->setToolTip(mBlenderSettingsMM[LblIdx].tooltip);
		mSettingLabelList << Label;

		if (LblIdx < LBL_BEVEL_SEGMENTS)
		{   // QCheckBoxes
			QCheckBox* CheckBox = new QCheckBox(mSettingsBox);
			CheckBox->setProperty("ControlID",QVariant(LblIdx));
			CheckBox->setChecked(mBlenderSettingsMM[LblIdx].value.toInt());
			CheckBox->setToolTip(mBlenderSettingsMM[LblIdx].tooltip);
			SetupControlHelp(Label, CheckBox);
			if (LblIdx == LBL_CROP_IMAGE_MM)
				connect(CheckBox, &QCheckBox::toggled, this, &lcBlenderPreferences::SetModelSize);
			else
				connect(CheckBox, &QCheckBox::clicked, this, [this]() { SettingIndexChanged(); });
			mCheckBoxList << CheckBox;
			mSettingsSubform->addRow(Label,CheckBox);
		}
		else if (LblIdx < LBL_CHOSEN_LOGO)
		{   // QLineEdits
			QLineEdit* LineEdit = new QLineEdit(mSettingsBox);
			LineEdit->setProperty("ControlID",QVariant(LblIdx));
			if (LblIdx == LBL_RESOLUTION_WIDTH || LblIdx == LBL_RESOLUTION_HEIGHT)
			{
				connect(LineEdit, &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
				LineEdit->setValidator(new QIntValidator(1, LC_MAX_IMAGE_DIMENSION));
			}
			else
			{
				if (LblIdx == LBL_IMPORT_SCALE)
					LineEdit->setValidator(new QDoubleValidator(0.01,10.0,2));
				else if (LblIdx == LBL_RENDER_PERCENTAGE_MM || LblIdx == LBL_CAMERA_BORDER_PERCENT_MM)
					LineEdit->setValidator(new QIntValidator(1,1000));
				else if (LblIdx == LBL_GAP_SCALE || LblIdx == LBL_MERGE_DISTANCE)
					LineEdit->setValidator(new QDoubleValidator(0.001,100.0,3));
				else if (LblIdx == LBL_BEVEL_WEIGHT || LblIdx == LBL_BEVEL_WIDTH_MM)
					LineEdit->setValidator(new QDoubleValidator(0.0,10.0,1));
				else
					LineEdit->setValidator(new QIntValidator(1, LC_RENDER_IMAGE_MAX_SIZE));
				SetNumericText(LineEdit, mBlenderSettingsMM[LblIdx].value);
				connect(LineEdit, &QLineEdit::textEdited, this, &lcBlenderPreferences::SettingTextChanged);
			}
			LineEdit->setToolTip(mBlenderSettingsMM[LblIdx].tooltip);
			SetupControlHelp(Label, LineEdit);
			mLineEditList << LineEdit;
			mSettingsSubform->addRow(Label,LineEdit);
		}
		else
		{   // QComboBoxes
			QComboBox* ComboBox = new QComboBox(mSettingsBox);
			ComboBox->setProperty("ControlID",QVariant(LblIdx));
			const QString Value = mBlenderSettingsMM[LblIdx].value;
			QStringList const DataList = mComboItemsMM[ComboBoxItemsIndex].dataList.split("|");
			QStringList const ItemList = TranslateDefaultText(mComboItemsMM[ComboBoxItemsIndex].itemList).split("|");
			ComboBox->addItems(ItemList);
			for (int CtlIdx = 0; CtlIdx < ComboBox->count(); CtlIdx++)
				ComboBox->setItemData(CtlIdx, DataList.at(CtlIdx));
			ComboBox->setToolTip(mBlenderSettingsMM[LblIdx].tooltip);
			SetupControlHelp(Label, ComboBox);
			int CurrentIndex = int(ComboBox->findData(QVariant::fromValue(Value)));
			ComboBox->setCurrentIndex(CurrentIndex);
			if (LblIdx == LBL_COLOUR_SCHEME_MM)
				connect(ComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &lcBlenderPreferences::ValidateColourScheme);
			else
				connect(ComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &lcBlenderPreferences::SettingIndexChanged);
			mComboBoxList << ComboBox;
			ComboBoxItemsIndex++;
			mSettingsSubform->addRow(Label,ComboBox);
		}
	}

	SetModelSize();

	if (!mSettingsSubform->rowCount())
		mSettingsSubform = nullptr;

	mSettingsBox->setEnabled(mConfigured);
}

void lcBlenderPreferences::UpdateBlenderAddon()
{
	mAddonUpdateButton->setEnabled(false);

	disconnect(mPathLineEditList[PATH_BLENDER], &QLineEdit::editingFinished, this, nullptr);

	ConfigureBlenderAddon(sender() == mPathBrowseButtonList[PATH_BLENDER], sender() == mAddonUpdateButton);

	connect(mPathLineEditList[PATH_BLENDER], &QLineEdit::editingFinished, this, [this]() { ConfigureBlenderAddon(); });
}

void lcBlenderPreferences::ConfigureBlenderAddon(bool TestBlender, bool AddonUpdate, bool ModuleChange)
{
	mProgressBar = nullptr;

	const QString BlenderExe = QDir::toNativeSeparators(mPathLineEditList[PATH_BLENDER]->text());

	if (BlenderExe.isEmpty())
	{
		mBlenderVersion.clear();
		mConfigured = false;
		StatusUpdate(false, true);
		mAddonUpdateButton->setEnabled(mConfigured);
		mPathsBox->setEnabled(mConfigured);
		mSettingsBox->setEnabled(mConfigured);
		return;
	}

	if (QFileInfo(BlenderExe).isReadable())
	{
		enum ProcEnc
		{
			PR_OK, PR_FAIL, PR_WAIT, PR_INSTALL, PR_TEST
		};
		const QString BlenderDir = QDir::toNativeSeparators(QString("%1/Blender").arg(mDataDir));
		const QString BlenderConfigDir = QString("%1/setup/addon_setup/config").arg(BlenderDir);
		const QString BlenderAddonDir = QDir::toNativeSeparators(QString("%1/addons").arg(BlenderDir));
		const QString BlenderExeCompare = QDir::toNativeSeparators(lcGetProfileString(LC_PROFILE_BLENDER_PATH)).toLower();
		const QString BlenderInstallFile = QDir::toNativeSeparators(QString("%1/%2").arg(BlenderDir, LC_BLENDER_ADDON_INSTALL_FILE));
		const QString BlenderTestString = QLatin1String("###TEST_BLENDER###");
		QByteArray AddonPathsAndModuleNames;
		QString Message, ShellProgram;
		QStringList Arguments;
		ProcEnc Result = PR_OK;
		QFile ScriptFile;

		bool NewBlenderExe = BlenderExeCompare != BlenderExe.toLower();

		if (mConfigured && !AddonUpdate && !ModuleChange && !NewBlenderExe)
			return;

#ifdef Q_OS_WIN
		ShellProgram = QLatin1String(LC_WINDOWS_SHELL);
#else
		ShellProgram = QLatin1String(LC_UNIX_SHELL);
#endif
		auto ProcessCommand = [&](ProcEnc Action)
		{
#ifdef Q_OS_WIN
			if (Action == PR_INSTALL)
			{
				lcRunElevatedProcess(ShellProgram.toStdWString().c_str(), QString("/C %1").arg(ScriptFile.fileName()).toStdWString().c_str(), BlenderDir.toStdWString().c_str());

				mAddonVersionLabel->clear();

				if (mProgressBar)
					mProgressBar->close();

				return PR_OK;
			}
#endif

			mProcess = new QProcess();

			QString ProcessAction = tr("add-on install");
			if (Action == PR_INSTALL)
			{
				connect(mProcess, &QProcess::readyReadStandardOutput, this, [this]() { ReadStdOut(); });
				const QString& LdrawLibPath = QFileInfo(lcGetProfileString(LC_PROFILE_PARTS_LIBRARY)).absolutePath();
				QStringList SystemEnvironment = QProcess::systemEnvironment();
				SystemEnvironment.prepend("LDRAW_DIRECTORY=" + LdrawLibPath);
				SystemEnvironment.prepend("ADDONS_TO_LOAD=" + AddonPathsAndModuleNames);
				mProcess->setEnvironment(SystemEnvironment);
			}
			else
			{
				ProcessAction = tr("test");
				disconnect(&mUpdateTimer, &QTimer::timeout, this, &lcBlenderPreferences::Update);
			}

			mProcess->setWorkingDirectory(BlenderDir);

			mProcess->setStandardErrorFile(QString("%1/stderr-blender-addon-install").arg(BlenderDir));

			if (Action == PR_INSTALL)
			{
				mProcess->start(BlenderExe, Arguments);
			}
			else
			{
#ifdef Q_OS_WIN
				mProcess->start(ShellProgram, QStringList() << "/C" << ScriptFile.fileName());
#else
				mProcess->start(ShellProgram, QStringList() << ScriptFile.fileName());
#endif
			}

			if (!mProcess->waitForStarted())
			{
				Message = tr("Cannot start Blender %1 process.\n%2").arg(ProcessAction, QString(mProcess->readAllStandardError()));
				delete mProcess;
				mProcess = nullptr;
				return PR_WAIT;
			}
			else
			{
				if (mProcess->exitStatus() != QProcess::NormalExit || mProcess->exitCode() != 0)
				{
					Message = tr("Failed to execute Blender %1.\n%2").arg(ProcessAction, QString(mProcess->readAllStandardError()));
					return PR_FAIL;
				}
			}

			if (Action == PR_TEST)
			{
				while (mProcess && mProcess->state() != QProcess::NotRunning)
				{
					QTime Waiting = QTime::currentTime().addMSecs(500);
					while (QTime::currentTime() < Waiting)
						QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
				}
				connect(&mUpdateTimer, &QTimer::timeout, this, &lcBlenderPreferences::Update);
				const QString StdOut = QString(mProcess->readAllStandardOutput());
				if (!StdOut.contains(BlenderTestString))
				{
					Message =  tr("A simple check to test if the selected file is Blender failed. "
								 "Please create a %1 GitHub ticket if you are sure the file is Blender 2.8 or newer. "
								 "The ticket should contain the full path to the Blender executable.").arg(LC_PRODUCTNAME_STR);
					return PR_FAIL;
				}
				else
				{
					QStringList Items = StdOut.split('\n',SkipEmptyParts).last().split(" ");
					if (Items.count() > 6 && Items.at(0) == QLatin1String("Blender"))
					{
						Items.takeLast();
						mBlenderVersion.clear();
						for (int LblIdx = 1; LblIdx < Items.size(); LblIdx++)
							mBlenderVersion.append(Items.at(LblIdx)+" ");
						mBlenderVersionFound = !mBlenderVersion.isEmpty();
						if (mBlenderVersionFound)
						{
							mBlenderVersion = mBlenderVersion.trimmed().prepend("v").append(")");
							mBlenderVersionEdit->setText(mBlenderVersion);
							// set default LDraw import module if not configured
							if (!mImportActBox->isChecked() && !mImportMMActBox->isChecked())
								mImportActBox->setChecked(true);
							Message = tr("Blender %1 process completed. Version: %2 validated.").arg(ProcessAction, mBlenderVersion);
						}
					}
				}
			}

			return PR_OK;
		};

		connect(&mUpdateTimer, &QTimer::timeout, this, &lcBlenderPreferences::Update);
		mUpdateTimer.start(500);

		QDir ConfigDir(BlenderConfigDir);
		if (!ModuleChange || !ConfigDir.exists())
		{
			mProgressBar = new QProgressBar(mContent);
			mProgressBar->setMaximum(0);
			mProgressBar->setMinimum(0);
			mProgressBar->setValue(1);

			TestBlender |= sender() == mPathLineEditList[PATH_BLENDER];
			mAddonVersionLabel->setText(tr("Installing..."));
			if (TestBlender)
			{
				mExeGridLayout->replaceWidget(mBlenderVersionEdit, mProgressBar);
				mProgressBar->show();

				Arguments << QString("--factory-startup");
				Arguments << QString("-b");
				Arguments << QString("--python-expr");
				Arguments << QString("\"import sys;print('%1');sys.stdout.flush();sys.exit()\"").arg(BlenderTestString);

				bool Error = false;

				QString ScriptName, ScriptCommand;

#ifdef Q_OS_WIN
				ScriptName =  QLatin1String("blender_test.bat");
#else
				ScriptName =  QLatin1String("blender_test.sh");
#endif
				ScriptCommand = QString("\"%1\" %2").arg(BlenderExe, Arguments.join(" "));

				ScriptFile.setFileName(QString("%1/%2").arg(QDir::tempPath(), ScriptName));
				if (ScriptFile.open(QIODevice::WriteOnly | QIODevice::Text))
				{
					QTextStream Stream(&ScriptFile);
#ifdef Q_OS_WIN
					Stream << QLatin1String("@ECHO OFF& SETLOCAL") << LineEnding;
#else
					Stream << QLatin1String("#!/bin/bash") << LineEnding;
#endif
					Stream << ScriptCommand << LineEnding;
					ScriptFile.close();
				}
				else
				{
					Message = tr("Error writing to file '%1':\n%2.").arg(ScriptFile.fileName(), ScriptFile.errorString());
					Error = true;
				}

				if (Error)
				{
					StatusUpdate(false);
					return;
				}

				QThread::sleep(1);

				Result = ProcessCommand(PR_TEST);
				bool TestOk = Result != PR_FAIL;
				const QString statusLabel = TestOk ? "" : tr("Blender test failed.");
				StatusUpdate(false, TestOk, statusLabel);
				if (TestOk)
				{
					lcSetProfileString(LC_PROFILE_BLENDER_VERSION, mBlenderVersion);
					lcSetProfileString(LC_PROFILE_BLENDER_PATH, BlenderExe);
				}
				else
				{
					const QString& Title = tr("Test Blender");
					const QString& Header = tr ("Blender test failed.");
					ShowMessage(this, Header, Title, Message);
					return;
				}
			} // Test Blender

			if (!mBlenderVersion.isEmpty() && !mImportMMActBox->isChecked() && !mImportActBox->isChecked())
			{
				const QString& Title = tr("Configure Blender Add-on");
				const QString& Header = tr ("No import module enabled. If you continue, the default import module (Import TN) will be used.<br>If you select No, all add-on modules will be disabled.");
				const QString& Body = tr ("Continue with the default import module?");
				int Exec = ShowMessage(this, Header, Title, Body, QString(), MBB_YES_NO, QMessageBox::NoIcon);
				if (Exec != QMessageBox::Yes)
				{
					mRenderActBox->setChecked(false);
					if (Exec == QMessageBox::Cancel)
						return;
				}

				mImportActBox->setChecked(true);
			}

			if (TestBlender)
			{
				const QString preferredImportModule = mImportActBox->isChecked() ? QString("TN") : mImportMMActBox->isChecked() ? QString("MM") : QString();  // disable all import modules
				lcSetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE, preferredImportModule);
			}
			else
				mBlenderVersionEdit->setVisible(AddonUpdate);

			if (mProgressBar)
			{
				mProgressBar->setMaximum(0);
				mProgressBar->setMinimum(0);
				mProgressBar->setValue(1);
				mAddonGridLayout->replaceWidget(mAddonVersionEdit, mProgressBar);
				mAddonVersionLabel->setText(tr("Downloading..."));
				mProgressBar->show();
			}

			if (!ExtractBlenderAddon(BlenderDir))
			{
				if (AddonUpdate)
				{
					mConfigured = true;
					mBlenderVersionLabel->setText(tr("Blender"));
					mBlenderVersionLabel->setStyleSheet(QString("QLabel { color : %1; }").arg(QApplication::palette().text().color().name()));
					mBlenderVersionEdit->setText(mBlenderVersion);
					mBlenderVersionEdit->setVisible(mConfigured);
					if (!mAddonVersion.isEmpty())
					{
						mModulesBox->setEnabled(true);
						mAddonVersionEdit->setText(mAddonVersion);
					}
					mAddonUpdateButton->setEnabled(mConfigured);
					if (mProgressBar)
					{
						mAddonGridLayout->replaceWidget(mProgressBar, mAddonVersionEdit);
						mAddonVersionLabel->setText(tr("Blender Add-on"));
						mProgressBar->close();
					}
				}
				return;
			}

			if (!QFileInfo::exists(BlenderInstallFile))
			{
				QMessageBox::warning(this, tr("Install Blender Add-on"), tr("Could not find add-on install file: '%1'.").arg(BlenderInstallFile));
				StatusUpdate(true, true, tr("Not found."));
				return;
			}

			StatusUpdate(true, false, tr("Installing..."));

			emit SettingChangedSig(true);
		}

		SaveSettings();

		Arguments.clear();
		Arguments << QString("--background");
		Arguments << QString("--python");
		Arguments << BlenderInstallFile;
		Arguments << "--";
		if (!mRenderActBox->isChecked() && !mImportActBox->isChecked() && !mImportMMActBox->isChecked())
			Arguments << QString("--disable_ldraw_addons");
		else if (!mImportActBox->isChecked())
			Arguments << QString("--disable_ldraw_import");
		else if (!mImportMMActBox->isChecked())
			Arguments << QString("--disable_ldraw_import_mm");
		else if (!mRenderActBox->isChecked())
			Arguments << QString("--disable_ldraw_render");
		Arguments << QString("--leocad");

		Message = tr("Blender Add-on Install Arguments: '%1' %2").arg(BlenderExe, Arguments.join(" "));

		if (!TestBlender)
			mBlenderVersionFound = false;

		if (QDir(BlenderAddonDir).entryInfoList(QDir::Dirs|QDir::NoSymLinks).count() > 0)
		{
			QJsonArray JsonArray;
			const QStringList AddonDirs = QDir(BlenderAddonDir).entryList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::SortByMask);
			for (const QString& Addon : AddonDirs)
			{
				QDir Dir(QString("%1/%2").arg(BlenderAddonDir, Addon));
				Dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
				QFileInfoList List = Dir.entryInfoList();
				for (int LblIdx = 0; LblIdx < List.size(); LblIdx++)
				{
					if (List.at(LblIdx).fileName() == QLatin1String("__init__.py"))
					{
						QFile File(QFileInfo(List.at(LblIdx)).absoluteFilePath());
						if (!File.open(QFile::ReadOnly | QFile::Text))
						{
							QMessageBox::warning(this, tr("Install Blender Add-on"), tr("Cannot read add-on file '%1'<br>%2").arg(List.at(LblIdx).fileName(), File.errorString()));
							break;
						}
						else
						{
							bool FoundModule = false;
							QTextStream In(&File);
							while (!In.atEnd())
							{
								if (QString(In.readLine(0)).startsWith("bl_info"))
								{
									FoundModule = true;
									break;
								}
							}
							File.close();
							if (FoundModule)
							{
								QJsonObject JsonItemObj;
								JsonItemObj["load_dir"] = QDir::toNativeSeparators(Dir.absolutePath());
								JsonItemObj["module_name"] = Dir.dirName();
								JsonArray.append(JsonItemObj);
							}
						}
					}
				}
			}
			QJsonDocument JsonDoc;
			JsonDoc.setArray(JsonArray);
			AddonPathsAndModuleNames = JsonDoc.toJson(QJsonDocument::Compact);
		}

#ifdef Q_OS_WIN
		bool Error = false;

		QString ScriptName =  QLatin1String("blender_install.bat");

		ScriptFile.setFileName(QString("%1/%2").arg(QDir::tempPath()).arg(ScriptName));
		if (ScriptFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			const QString& LdrawLibPath = QFileInfo(lcGetProfileString(LC_PROFILE_PARTS_LIBRARY)).absolutePath();
			QTextStream Stream(&ScriptFile);

			Stream << QLatin1String("@ECHO OFF& SETLOCAL") << Qt::endl;
			Stream << QLatin1String("SET LDRAW_DIRECTORY=") << LdrawLibPath << Qt::endl;
			Stream << QLatin1String("SET ADDONS_TO_LOAD=") << AddonPathsAndModuleNames << Qt::endl;
			Stream << QString("\"%1\"").arg(BlenderExe);
			for (const QString& Argument : Arguments)
				Stream << " " << QString("\"%1\"").arg(Argument);
			Stream << Qt::endl;

			ScriptFile.close();
		}
		else
		{
			Message = tr("Error writing to file '%1':\n%2.").arg(ScriptFile.fileName(), ScriptFile.errorString());
			Error = true;
		}

		if (Error)
		{
			StatusUpdate(false);
			return;
		}
#endif

		Result = ProcessCommand(PR_INSTALL);

		if (Result != PR_OK)
			StatusUpdate(true, true, tr("Install failed."));
	}
	else
		QMessageBox::warning(this, tr("Install Blender Add-on"), tr("Blender executable not found at '%1'.").arg(BlenderExe));
}

bool lcBlenderPreferences::ExtractBlenderAddon(const QString& BlenderDir)
{
	bool Extracted = false;

	QDir Dir(BlenderDir);
	if (!Dir.exists())
		Dir.mkdir(BlenderDir);

	AddonEnc AddonAction = AddonEnc(GetBlenderAddon(BlenderDir));
	if (AddonAction == ADDON_EXTRACT)
	{
		gAddonPreferences->StatusUpdate(true, false, tr("Extracting..."));
		const QString BlenderAddonFile = QDir::toNativeSeparators(QString("%1/%2").arg(BlenderDir, LC_BLENDER_ADDON_FILE));

		QString Result;
		Extracted = gAddonPreferences->ExtractAddon(BlenderAddonFile, Result);

		if (!Extracted)
		{
			QString Message = tr("Failed to extract '%1' to '%2'.").arg(LC_BLENDER_ADDON_FILE, BlenderDir);
			if (Result.size())
				Message.append(" "+Result);

			QMessageBox::warning(this, tr("Extract Blender Add-on"), Message);
		}
	}
	else if (AddonAction == ADDON_NO_ACTION)
		return true;

	return Extracted;
}

int lcBlenderPreferences::GetBlenderAddon(const QString& BlenderDir)
{
	const QString BlenderAddonDir = QDir::toNativeSeparators(QString("%1/addons").arg(BlenderDir));
	const QString BlenderAddonFile = QDir::toNativeSeparators(QString("%1/%2").arg(BlenderDir, LC_BLENDER_ADDON_FILE));
	const QString AddonVersionFile = QDir::toNativeSeparators(QString("%1/%2/__version__.py").arg(BlenderAddonDir, LC_BLENDER_ADDON_RENDER_FOLDER));
	bool ExtractedAddon = QFileInfo(AddonVersionFile).isReadable();
	bool BlenderAddonValidated = ExtractedAddon || QFileInfo(BlenderAddonFile).isReadable();
	AddonEnc AddonAction = ADDON_DOWNLOAD;
	QString LocalVersion, OnlineVersion;

	using namespace std;
	auto VersionStringCompare = [](string V1, string V2)
	{   // Returns 1 if V2 is smaller, -1 if V1 is smaller, 0 if equal
		int Vnum1 = 0, Vnum2 = 0;
		for (quint32 i = 0, j = 0; (i < V1.length() || j < V2.length());)
		{
			while (i < V1.length() && V1[i] != '.')
			{
				Vnum1 = Vnum1 * 10 + (V1[i] - '0');
				i++;
			}
			while (j < V2.length() && V2[j] != '.')
			{
				Vnum2 = Vnum2 * 10 + (V2[j] - '0');
				j++;
			}
			if (Vnum1 > Vnum2)
				return 1;
			if (Vnum2 > Vnum1)
				return -1;
			Vnum1 = Vnum2 = 0;
			i++;
			j++;
		}

		return 0;
	};

	auto GetBlenderAddonVersionMatch = [&]()
	{
		lcHttpManager* HttpManager = new lcHttpManager(gAddonPreferences);
		connect(HttpManager, &lcHttpManager::DownloadFinished, gAddonPreferences, &lcBlenderPreferences::DownloadFinished);
		gAddonPreferences->mHttpReply = HttpManager->DownloadFile(QLatin1String(LC_BLENDER_ADDON_LATEST_URL));
		while (gAddonPreferences->mHttpReply)
			QApplication::processEvents();
		if (!gAddonPreferences->mData.isEmpty())
		{
			QJsonDocument Json = QJsonDocument::fromJson(gAddonPreferences->mData);
			OnlineVersion = Json.object().value("tag_name").toString();
			gAddonPreferences->mData.clear();
		}
		else
		{
			QMessageBox::warning(this, tr("Check for Blender Add-on Updates"), tr("Check latest add-on version failed."));
			return true; // Reload existing archive
		}

		QByteArray Ba;
		if (!ExtractedAddon)
		{
			const char* VersionFile = "addons/" LC_BLENDER_ADDON_RENDER_FOLDER "/__version__.py";

			lcZipFile ZipFile;

			if (!ZipFile.OpenRead(BlenderAddonFile))
			{
				QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Cannot open add-on archive file: '%1'.").arg(BlenderAddonFile));
				return false;
			}

			lcMemFile File;

			if (!ZipFile.ExtractFile(VersionFile, File))
			{
				QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Cannot extract add-on archive version file: '%1'.").arg(VersionFile));
				return false;
			}

			Ba = QByteArray::fromRawData((const char*)File.mBuffer, (int)File.GetLength());
			if (Ba.isEmpty())
			{
				QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Cannot read add-on archive version file: '%1'.").arg(VersionFile));
				return false; // Download new archive
			}
		}
		else
		{
			QFile File(AddonVersionFile);
			if (!File.open(QIODevice::ReadOnly))
			{
				QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Cannot read add-on version file: '%1'<br>%2.").arg(AddonVersionFile, File.errorString()));
				return false; // Download new archive
			}
			Ba = File.readAll();
			File.close();
		}

		QTextStream Content(Ba.data());
		while (!Content.atEnd())
		{
			QString Token;
			Content >> Token;
			if (Token == QLatin1String("version"))
			{
				Content >> Token;
				LocalVersion = Content.readAll().trimmed().replace("(","v").replace(",",".").replace(" ","").replace(")","");
			}
		}

		if (!LocalVersion.isEmpty() && !OnlineVersion.isEmpty())
		{
			// localVersion is smaller than onlineVersion so prompt to download new archive
			if (VersionStringCompare(LocalVersion.toStdString(), OnlineVersion.toStdString()) < 0)
				return false; // Download new archive
		}

		return true; // Reload existing archive
	};

	if (BlenderAddonValidated)
	{
		if (GetBlenderAddonVersionMatch())
		{
			if (ExtractedAddon)
				AddonAction = ADDON_NO_ACTION;
			else
				AddonAction = ADDON_EXTRACT;
		}
		else if (gMainWindow)
		{
			if (lcGetProfileInt(LC_PROFILE_BLENDER_ADDON_VERSION_CHECK))
			{
				if (LocalVersion.isEmpty())
					LocalVersion = gAddonPreferences->mAddonVersion;
				const QString& Title = tr("Update Blender Add-on");
				const QString& Header = tr ("Detected %1 Blender LDraw add-on %2. A newer version %3 exists.").arg(LC_PRODUCTNAME_STR, LocalVersion, OnlineVersion);
				const QString& Body = tr ("Do you want to download version %1?").arg(OnlineVersion);
				int Exec = ShowMessage(this, Header, Title, Body, QString(), MBB_YES, QMessageBox::NoIcon);
				if (Exec == QMessageBox::Cancel)
				{
					AddonAction = ADDON_CANCELED;
					gAddonPreferences->mDialogCancelled = true;
				}
				else if (Exec == QMessageBox::No)
					AddonAction = ADDON_NO_ACTION;
			}
			else
				AddonAction = ADDON_NO_ACTION;
		}

		if (AddonAction != ADDON_DOWNLOAD)
			return AddonAction;
	}

	auto RemoveOldBlenderAddon = [&] (const QString& OldBlenderAddonFile)
	{
		if (QFileInfo::exists(BlenderAddonDir))
		{
			bool Result = true;
			QDir Dir(BlenderAddonDir);
			for (QFileInfo const& FileInfo : Dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::DirsFirst))
			{
				if (FileInfo.isDir())
					Result &= QDir(FileInfo.absoluteFilePath()).removeRecursively();
				else
					Result &= QFile::remove(FileInfo.absoluteFilePath());
			}

			if (QFileInfo::exists(OldBlenderAddonFile))
				Result &= QFile::remove(OldBlenderAddonFile);

			Result &= Dir.rmdir(BlenderAddonDir);
			if (!Result)
				QMessageBox::warning(this, tr("Remove Existing Add-on"), tr("Failed to properly remove Blender add-on: '%1'.").arg(BlenderAddonDir));
		}
	};

	BlenderAddonValidated = false;
	lcHttpManager* HttpManager = new lcHttpManager(gAddonPreferences);
	connect(HttpManager, &lcHttpManager::DownloadFinished, gAddonPreferences, &lcBlenderPreferences::DownloadFinished);
	gAddonPreferences->mHttpReply = HttpManager->DownloadFile(QLatin1String(LC_BLENDER_ADDON_URL));
	while (gAddonPreferences->mHttpReply)
		QApplication::processEvents();
	if (!gAddonPreferences->mData.isEmpty())
	{
		const QString OldBlenderAddonFile = QString("%1.hold").arg(BlenderAddonFile);
		if (QFileInfo::exists(BlenderAddonFile))
		{
			if (!QFile::rename(BlenderAddonFile, OldBlenderAddonFile))
				QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Failed to rename existing Blender add-on archive '%1'.").arg(BlenderAddonFile));
		}

		QDir Dir(BlenderDir);
		if(!Dir.exists())
			Dir.mkpath(".");

		QString ArchiveFileName, OldArchiveFileName = QFileInfo(OldBlenderAddonFile).fileName();
		QFile File(BlenderAddonFile);
		if (File.open(QIODevice::WriteOnly))
		{
			File.write(gAddonPreferences->mData);
			File.close();
			if (File.open(QIODevice::ReadOnly))
			{
				QCryptographicHash Sha256Hash(QCryptographicHash::Sha256);
				qint64 DataSize = File.size();
				const qint64 BufferSize = Q_INT64_C(1000);
				char Buf[BufferSize];
				qint64 BytesRead;
				qint64 ReadSize = qMin(DataSize, BufferSize);
				while (ReadSize > 0 && (BytesRead = File.read(Buf, ReadSize)) > 0)
				{
					DataSize -= BytesRead;
					Sha256Hash.addData(Buf, BytesRead);
					ReadSize = qMin(DataSize, BufferSize);
				}
				File.close();
				const QString ShaCalculated = Sha256Hash.result().toHex();

				gAddonPreferences->mData.clear();
				gAddonPreferences->mHttpReply = HttpManager->DownloadFile(QLatin1String(LC_BLENDER_ADDON_SHA_HASH_URL));
				while (gAddonPreferences->mHttpReply)
					QApplication::processEvents();
				if (!gAddonPreferences->mData.isEmpty())
				{
					const QStringList ShaReceived = QString(gAddonPreferences->mData).trimmed().split(" ", SkipEmptyParts);
					if (ShaReceived.first() == ShaCalculated)
					{
						ArchiveFileName = QFileInfo(BlenderAddonFile).fileName();
						if (ArchiveFileName == ShaReceived.last())
						{
							RemoveOldBlenderAddon(OldBlenderAddonFile);
							BlenderAddonValidated = true;
						}
						else
							QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Failed to validate Blender add-on file name.<br>Downloaded: '%1'<br>Received: '%2'").arg(ArchiveFileName, ShaReceived.last()));
					}
					else
						QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Failed to validate Blender add-on SHA hash.<br>Calculated: %1<br>Received: %2").arg(ShaCalculated, ShaReceived.first()));
					gAddonPreferences->mData.clear();
				}
				else
					QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Failed to receive SHA hash for Blender add-on '%1.sha256'.").arg(LC_BLENDER_ADDON_FILE));
			}
			else
				QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Failed to read Blender add-on archive:<br>'%1':<br>%2").arg(BlenderAddonFile, File.errorString()));
		}
		else
			QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Failed to write Blender add-on archive:<br>'%1':<br>%2").arg(BlenderAddonFile, File.errorString()));

		if (!BlenderAddonValidated)
		{
			if (QFileInfo::exists(BlenderAddonFile))
				if (!QFile::remove(BlenderAddonFile))
					QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Failed to remove invalid Blender add-on archive:<br>'%1'").arg(BlenderAddonFile));
			if (QFileInfo::exists(OldBlenderAddonFile))
				if (!QFile::rename(OldBlenderAddonFile, BlenderAddonFile))
					QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Failed to restore Blender add-on archive:<br>'%1' from '%2'").arg(ArchiveFileName, OldArchiveFileName));
			AddonAction = ADDON_FAIL;
		}
	}
	else
	{
		QMessageBox::warning(this, tr("Update Blender Add-on"), tr("Failed to download Blender add-on archive:<br>'%1'").arg(BlenderAddonFile));
		AddonAction = ADDON_FAIL;
	}

	if (!BlenderAddonValidated)
		gAddonPreferences->StatusUpdate(true, true, tr("Download failed."));

	if (!QDir(BlenderAddonDir).exists() && QFileInfo::exists(BlenderAddonFile))
		AddonAction = ADDON_EXTRACT;

	return AddonAction;
}

void lcBlenderPreferences::StatusUpdate(bool Addon, bool Error, const QString& Message)
{
	QString Label, Colour;
	const QString Which = Addon ? tr("Blender add-on") : tr("Blender");
	if (mProgressBar)
	{
		if (Addon)
		{
			mAddonGridLayout->replaceWidget(mProgressBar, mAddonVersionEdit);
		}
		else
		{
			mExeGridLayout->replaceWidget(mProgressBar, mBlenderVersionEdit);
			mProgressBar->hide();
		}

		mAddonVersionLabel->setText(Which);
	}
	if (Error)
	{
		if (!Addon)
			mPathLineEditList[PATH_BLENDER]->text() = QString();

		lcSetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE, QString());

		Label  = ! Message.isEmpty() ? Message : tr("%1 not configured").arg(Which);
		Colour = Message.startsWith("Error:", Qt::CaseInsensitive)
						? QLatin1String("red")
						: gApplication->IsDarkTheme()
							? QLatin1String(LC_THEME_DARK_DECORATE_QUOTED_TEXT)
							: QLatin1String("blue");

		mDialogCancelled = true;
	}
	else
	{
		Label  = !Message.isEmpty() ? Message : tr("%1 setup...").arg(Which);
		Colour = QApplication::palette().text().color().name();
	}

	if (Addon)
	{
		mAddonVersionLabel->setStyleSheet(QString("QLabel { color : %1; }").arg(Colour));
		mAddonVersionLabel->setText(Label);
		mAddonVersionEdit->setVisible(!mAddonVersion.isEmpty());
	}
	else
	{
		mBlenderVersionLabel->setStyleSheet(QString("QLabel { color : %1; }").arg(Colour));
		mBlenderVersionLabel->setText(Label);
		mBlenderVersionEdit->setVisible(!mBlenderVersion.isEmpty());
	}
}

void lcBlenderPreferences::ShowResult()
{
	bool Error;
	const QString StdErrLog = ReadStdErr(Error);

	if (mProgressBar)
		mProgressBar->close();

	WriteStdOut();

	if (mProcess->exitStatus() != QProcess::NormalExit || mProcess->exitCode() != 0 || Error)
	{
		StatusUpdate(true, true,tr("Error: Install failed."));
		mConfigured = false;

		const QString& Title = tr("Install Blender Add-on");
		const QString& Header = "<b>" + tr ("Add-on install failed.") + "</b>";
		const QString& Body = tr ("LDraw add-on install encountered one or more errors. See Show Details...");
		ShowMessage(this, Header, Title, Body, StdErrLog, MBB_OK, QMessageBox::Warning);
	}
	else
	{
		const QString TextColour = QString("QLabel { color : %1; }").arg(QApplication::palette().text().color().name());
		mAddonGridLayout->replaceWidget(mProgressBar, mAddonVersionEdit);
		mConfigured = true;
		mBlenderVersionLabel->setText(tr("Blender"));
		mBlenderVersionLabel->setStyleSheet(TextColour);
		mBlenderVersionEdit->setText(mBlenderVersion);
		mBlenderVersionEdit->setVisible(mConfigured);
		mPathsBox->setEnabled(mConfigured);
		mSettingsBox->setEnabled(mConfigured);

		if (!mAddonVersion.isEmpty())
		{
			mAddonVersionLabel->setText(tr("Blender Add-on"));
			mAddonVersionLabel->setStyleSheet(TextColour);
			mAddonVersionEdit->setText(mAddonVersion);
			mAddonVersionEdit->setVisible(true);
			mModulesBox->setEnabled(true);
			mAddonUpdateButton->setEnabled(true);
			lcSetProfileString(LC_PROFILE_BLENDER_VERSION, mBlenderVersion);
			lcSetProfileString(LC_PROFILE_BLENDER_ADDON_VERSION, mAddonVersion);
			SetModelSize(true);
			SaveSettings();
			mDialogCancelled = false;
		}
	}

	delete mProcess;
	mProcess = nullptr;
}

void lcBlenderPreferences::SetNumericText(QLineEdit* Edit, const QString& Value)
{
	const QValidator* Validator = Edit->validator();
	QLocale Locale = Validator->locale();
	Locale.setNumberOptions(Locale.numberOptions() | QLocale::OmitGroupSeparator);
	bool Ok;
	QString Text;

	if (qobject_cast<const QIntValidator*>(Validator))
	{
		const int Number = Value.toInt(&Ok);
		Text = Locale.toString(Number);
	}
	else
	{
		const double Number = Value.toDouble(&Ok);
		Text = Locale.toString(Number, 'g', QLocale::FloatingPointShortest);
	}

	Edit->setText(Ok ? Text : Value);
}

double lcBlenderPreferences::GetNumericValue(const QLineEdit* Edit, bool& Ok)
{
	Ok = false;

	if (!Edit->hasAcceptableInput())
		return 0.0;

	const QValidator* Validator = Edit->validator();

	if (qobject_cast<const QIntValidator*>(Validator))
		return Validator->locale().toInt(Edit->text(), &Ok);

	return Validator->locale().toDouble(Edit->text(), &Ok);
}

void lcBlenderPreferences::SettingTextChanged(const QString& Value)
{
	bool Change = false;

	QLineEdit* LineEdit = qobject_cast<QLineEdit*>(sender());
	if (LineEdit)
	{
		int LblIdx = LineEdit->property("ControlID").toInt();
		if (mImportMMActBox->isChecked())
		{
			Change = mBlenderSettingsMM[LblIdx].value != Value;
		}
		else
		{
			Change = mBlenderSettings[LblIdx].value != Value;
		}

		Change |= SettingsModified(false);
		emit SettingChangedSig(Change);
	}
}

void lcBlenderPreferences::SettingIndexChanged(int Index)
{
	int LblIdx = -1;
	bool Change = false;
	QString Item;

	if (Index > -1)
	{
		QComboBox* ComboBox = qobject_cast<QComboBox*>(sender());
		if (ComboBox)
		{
			LblIdx = ComboBox->property("ControlID").toInt();
			Item = ComboBox->itemData(Index).toString();
		}
	}
	else
	{
		QCheckBox* CheckBox = qobject_cast<QCheckBox*>(sender());
		if (CheckBox)
		{
			LblIdx = CheckBox->property("ControlID").toInt();
			Item = QString::number(CheckBox->isChecked());
		}
	}

	if (LblIdx > -1)
	{
		if (mImportMMActBox->isChecked())
		{
			Change = mBlenderSettingsMM[LblIdx].value != Item;
		}
		else
		{
			Change = mBlenderSettings[LblIdx].value != Item;
		}
	}

	Change |= SettingsModified(false);
	emit SettingChangedSig(Change);
}

void lcBlenderPreferences::PathChanged()
{
	QLineEdit* LineEdit = qobject_cast<QLineEdit*>(sender());
	if (LineEdit)
	{
		bool Change = false;
		const int LblIdx = LineEdit->property("ControlID").toInt();
		const QString& Path = QDir::toNativeSeparators(LineEdit->text()).toLower();

		if (LblIdx != PATH_BLENDER)
		{
			Change = QDir::toNativeSeparators(mBlenderPaths[LblIdx].value).toLower() != Path;
		}

		Change |= SettingsModified(false);
		emit SettingChangedSig(Change);
	}
}

void lcBlenderPreferences::GetStandardOutput()
{
	const QString LogFile = QString("%1/Blender/stdout-blender-addon-install").arg(mDataDir);
	QFileInfo FileInfo(LogFile);
	if (!FileInfo.exists())
	{
		QMessageBox::warning(this, tr("View Blender Add-on Output"), tr("Blender add-on standard output file not found: '%1'.").arg(FileInfo.absoluteFilePath()));
		return;
	}

	if (LogFile.isEmpty())
		return;

	QDesktopServices::openUrl(QUrl("file:///"+LogFile, QUrl::TolerantMode));
}

void lcBlenderPreferences::ReadStdOut(const QString& StdOutput, QString& Errors)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QRegularExpression RxInfo("^INFO: ");
	QRegularExpression RxData("^DATA: ");
	QRegularExpression RxError("^(?:\\w)*ERROR: ", QRegularExpression::CaseInsensitiveOption);
	QRegularExpression RxWarning("^(?:\\w)*WARNING: ", QRegularExpression::CaseInsensitiveOption);
	QRegularExpression RxAddonVersion("^ADDON VERSION: ", QRegularExpression::CaseInsensitiveOption);
	const QStringList StdOutLines = StdOutput.split(QRegularExpression("\n|\r\n|\r"));
#else
	QRegExp RxInfo("^INFO: ");
	QRegExp RxData("^DATA: ");
	QRegExp RxError("^(?:\\w)*ERROR: ", Qt::CaseInsensitive);
	QRegExp RxWarning("^(?:\\w)*WARNING: ", Qt::CaseInsensitive);
	QRegExp RxAddonVersion("^ADDON VERSION: ", Qt::CaseInsensitive);
	QStringList StdOutLines = StdOutput.split(QRegExp("\n|\r\n|\r"));
#endif

	bool ErrorEncountered = false;
	QStringList Items, ErrorList;

	const QString SaveAddonVersion = mAddonVersion;
	const QString SaveVersion = mBlenderVersion;

	qsizetype EditListItems = mPathLineEditList.size();
	for (const QString& StdOutLine : StdOutLines)
	{
		if (StdOutLine.isEmpty())
			continue;

		if (!mBlenderVersionFound)
		{
			Items = StdOutLine.split(" ");
			if (Items.count() > 6 && Items.at(0) == QLatin1String("Blender"))
			{
				Items.takeLast();
				mBlenderVersion.clear();
				for (int LblIdx = 1; LblIdx < Items.size(); LblIdx++)
					mBlenderVersion.append(Items.at(LblIdx)+" ");
				mBlenderVersionFound = !mBlenderVersion.isEmpty();
				if (mBlenderVersionFound)
				{
					mBlenderVersion = mBlenderVersion.trimmed().prepend("v").append(")");
					mBlenderVersionEdit->setText(mBlenderVersion);
					if (!mImportActBox->isChecked() && !mImportMMActBox->isChecked())
						mImportActBox->setChecked(true);
				}
			}
		}

		if (StdOutLine.contains(RxInfo))
		{
			Items = StdOutLine.split(": ");
		}
		else if (StdOutLine.contains(RxData))
		{
			Items = StdOutLine.split(": ");
			if (Items.at(1) == "ENVIRONMENT_FILE")
			{
				mBlenderPaths[PATH_ENVIRONMENT].value = Items.at(2);
				if (EditListItems > PATH_ENVIRONMENT)
					mPathLineEditList[PATH_ENVIRONMENT]->setText(Items.at(2));
			}
			else if (Items.at(1) == "LSYNTH_DIRECTORY")
			{
				mBlenderPaths[PATH_LSYNTH].value = Items.at(2);
				if (EditListItems > PATH_LSYNTH)
					mPathLineEditList[PATH_LSYNTH]->setText(Items.at(2));
			}
			else if (Items.at(1) == "STUDLOGO_DIRECTORY")
			{
				mBlenderPaths[PATH_STUD_LOGO].value = Items.at(2);
				if (EditListItems > PATH_STUD_LOGO)
					mPathLineEditList[PATH_STUD_LOGO]->setText(Items.at(2));
			}
		}
		else if (StdOutLine.contains(RxError) || StdOutLine.contains(RxWarning))
		{
			ErrorList << StdOutLine.trimmed() + "<br>";
			if (!ErrorEncountered)
				ErrorEncountered = StdOutLine.contains(RxError);
		}
		else if (StdOutLine.contains(RxAddonVersion))
		{
			Items = StdOutLine.split(":");
			mAddonVersion = tr("v%1").arg(Items.at(1).trimmed());
			mAddonVersionEdit->setText(mAddonVersion);
		}
	}
	if (ErrorList.size())
	{
		if (!mBlenderVersionFound)
		{
			if (mBlenderVersion != SaveVersion)
			{
				mConfigured = false;
				mBlenderVersion = SaveVersion;
				mBlenderVersionEdit->setText(mBlenderVersion);
			}
		}
		if (mAddonVersion != SaveAddonVersion)
		{
			mConfigured = false;
			mAddonVersion = SaveAddonVersion;
			mAddonVersionEdit->setText(mAddonVersion);
		}
		Errors = ErrorList.join(" ");
	}
}

void lcBlenderPreferences::ReadStdOut()
{
	const QString& StdOut = QString(mProcess->readAllStandardOutput());

	mStdOutList.append(StdOut);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QRegularExpression RxInfo("^INFO: ");
	QRegularExpression RxError("(?:\\w)*ERROR: ", QRegularExpression::CaseInsensitiveOption);
	QRegularExpression RxWarning("(?:\\w)*WARNING: ", QRegularExpression::CaseInsensitiveOption);
#else
	QRegExp RxInfo("^INFO: ");
	QRegExp RxError("(?:\\w)*ERROR: ", Qt::CaseInsensitive);
	QRegExp RxWarning("(?:\\w)*WARNING: ", Qt::CaseInsensitive);
#endif

	bool const Error = StdOut.contains(RxError);
	bool const Warning = StdOut.contains(RxWarning);

	if (StdOut.contains(RxInfo) && !Error)
		StatusUpdate(true, false);

	QString ErrorsAndWarnings;

	ReadStdOut(StdOut, ErrorsAndWarnings);

	if (!ErrorsAndWarnings.isEmpty())
	{
		const QString StdOutLog = QDir::toNativeSeparators(QString("<br>- See '%1/Blender/stdout-blender-addon-install'")
																.arg(mDataDir));

		QMessageBox::Icon Icon = QMessageBox::Warning;
		const QString& Items = Error ? tr("errors%1").arg(Warning ? tr(" and warnings") : "") : Warning ? tr("warnings") : "";

		const QString& Title = tr("Install Blender Add-on");
		const QString& Header = "<b>" + tr ("Add-on install standard output.") + "</b>";
		const QString& Body = tr ("LDraw add-on install encountered %1. See Show Details...").arg(Items);
		ShowMessage(this, Header, Title, Body, ErrorsAndWarnings.append(StdOutLog), MBB_OK, Icon);
	}
}

QString lcBlenderPreferences::ReadStdErr(bool& Error) const
{
	auto CleanLine = [](const QString& Line)
	{
		return Line.trimmed() + "<br>";
	};

	Error = false;
	QStringList ReturnLines;

	const QString BlenderDir = QString("%1/Blender").arg(mDataDir);
	QFile File(QString("%1/stderr-blender-addon-install").arg(BlenderDir));
	if (!File.open(QFile::ReadOnly | QFile::Text))
	{
		const QString Message = tr("Failed to open log file: '%1':\n%2").arg(File.fileName(), File.errorString());
		return Message;
	}
	QTextStream In(&File);
	while (!In.atEnd())
	{
		const QString& Line = In.readLine(0);
		ReturnLines << CleanLine(Line);
		if (!Error)
			Error = !Line.isEmpty();
	}
	return ReturnLines.join(" ");
}

void lcBlenderPreferences::WriteStdOut()
{
	const QString BlenderDir = QString("%1/Blender").arg(mDataDir);
	QFile File(QString("%1/stdout-blender-addon-install").arg(BlenderDir));
	if (File.open(QFile::WriteOnly | QIODevice::Truncate | QFile::Text))
	{
		QTextStream Out(&File);
		for (const QString& Line : std::as_const(mStdOutList))
			Out << Line << LineEnding;
		File.close();
		mAddonStdOutButton->setEnabled(true);
	}
	else
		QMessageBox::warning(this, tr("Save Blender Add-on Output"), tr("Error writing to %1 file '%2':\n%3").arg("stdout", File.fileName(), File.errorString()));
}

bool lcBlenderPreferences::PromptCancel()
{
#ifndef QT_NO_PROCESS
	if (mProcess)
	{
		if (!gMainWindow)
			return false;

		const QString& Title = tr("Cancel Blender Add-on Installation");
		const QString& Header = "<b>" + tr("Are you sure you want to cancel the add-on installation?") + "</b>";
		int Exec = QMessageBox::question(this, Title, Header, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
		if (Exec == QMessageBox::Yes)
		{
			mProcess->kill();
			delete mProcess;
			mProcess = nullptr;
		}
		else
			return false;
	}
#endif

	mDialogCancelled = true;
	return true;
}

void lcBlenderPreferences::Update()
{
#ifndef QT_NO_PROCESS
	if (!mProcess)
		return;

	if (mProcess->state() == QProcess::NotRunning)
		ShowResult();
#endif
	QApplication::processEvents();
}

void lcBlenderPreferences::Apply(const int Response)
{
	if (Response == QDialog::Accepted)
	{
		if (SettingsModified())
			SaveSettings();
	}
	else if (mDialogCancelled)
	{
		if (SettingsModified())
			if (PromptAccept())
				SaveSettings();
	}
}

bool lcBlenderPreferences::SettingsModified(bool Update, const QString& Module)
{
	if  (gAddonPreferences->mDialogCancelled)
		return false;

	int& Width = gAddonPreferences->mImageWidth;
	int& Height = gAddonPreferences->mImageHeight;
	double& Scale = gAddonPreferences->mScale;

	bool ModuleMM = !Module.isEmpty()
						? Module == QLatin1String("MM")
						: gAddonPreferences->mImportMMActBox->isChecked();

	bool Ok, Modified = !QFileInfo(lcGetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH)).isReadable();
	qreal _Width = 0.0, _Height = 0.0, _Scale = 0.0, _Value = 0.0, _OldValue = 0.0;
	QString OldValue;

	auto ItemChanged = [](qreal OldValue, qreal NewValue)
	{
		return NewValue > OldValue || NewValue < OldValue;
	};

	if (ModuleMM)
	{
		for (int LblIdx = 0; LblIdx < NumSettingsMM(); LblIdx++)
		{
			// checkboxes
			if (LblIdx < LBL_BEVEL_SEGMENTS)
			{
				for (int CtlIdx = 0; CtlIdx < gAddonPreferences->mCheckBoxList.size(); CtlIdx++)
				{
					OldValue = mBlenderSettingsMM[LblIdx].value;
					if (Update)
						mBlenderSettingsMM[LblIdx].value = QString::number(gAddonPreferences->mCheckBoxList[CtlIdx]->isChecked());
					Modified |= mBlenderSettingsMM[LblIdx].value != OldValue;
					if (LblIdx < LBL_VERBOSE_MM)
						LblIdx++;
				}
			}
			// lineedits
			else if (LblIdx < LBL_CHOSEN_LOGO)
			{
				for (int CtlIdx = 0; CtlIdx < gAddonPreferences->mLineEditList.size(); CtlIdx++)
				{
					if (CtlIdx == CTL_RESOLUTION_WIDTH_EDIT)
					{
						_OldValue = Width;
						_Width  = GetNumericValue(gAddonPreferences->mLineEditList[CtlIdx], Ok);
						if (Ok)
						{
							if (Update)
							{
								Width = int(_Width);
								mBlenderSettingsMM[LblIdx].value = QString::number(Width);
							}
							Modified |= ItemChanged(_OldValue, _Width);
						}
					}
					else if (CtlIdx == CTL_RESOLUTION_HEIGHT_EDIT)
					{
						_OldValue = Height;
						_Height = GetNumericValue(gAddonPreferences->mLineEditList[CtlIdx], Ok);
						if (Ok)
						{
							if (Update)
							{
								Height = int(_Height);
								mBlenderSettingsMM[LblIdx].value = QString::number(Height);
							}
							Modified |= ItemChanged(_OldValue, _Height);
						}
					}
					else if (CtlIdx == CTL_RENDER_PERCENTAGE_EDIT_MM)
					{
						_OldValue = Scale;
						_Scale = GetNumericValue(gAddonPreferences->mLineEditList[CtlIdx], Ok);
						if (Ok)
						{
							if (Update)
							{
								Scale = double(_Scale / 100);
								mBlenderSettingsMM[LblIdx].value = QString::number(_Scale);
							}
							Modified |= ItemChanged(_OldValue, Scale);
						}
					}
					else
					{
						_OldValue = mBlenderSettingsMM[LblIdx].value.toDouble();
						_Value = GetNumericValue(gAddonPreferences->mLineEditList[CtlIdx], Ok);
						if (Ok)
						{
							if (Update)
								mBlenderSettingsMM[LblIdx].value = QString::number(_Value);
							Modified |= ItemChanged(_OldValue, _Value);
						}
					}
					if (LblIdx < LBL_STARTING_STEP_FRAME)
						LblIdx++;
				}
			}
			// comboboxes
			else
			{
				for (int CtlIdx = 0; CtlIdx < gAddonPreferences->mComboBoxList.size(); CtlIdx++)
				{
					OldValue = mBlenderSettingsMM[LblIdx].value;
					const QString Value = gAddonPreferences->mComboBoxList[CtlIdx]->itemData(gAddonPreferences->mComboBoxList[CtlIdx]->currentIndex()).toString();
					if (Update)
						mBlenderSettingsMM[LblIdx].value = Value;
					Modified |= Value != OldValue;
					LblIdx++;
				}
			}
		}
	}
	else
	{
		// settings
		for (int LblIdx = 0; LblIdx < NumSettings(); LblIdx++)
		{
			// checkboxes
			if (LblIdx < LBL_BEVEL_WIDTH)
			{
				for (int CtlIdx = 0; CtlIdx < gAddonPreferences->mCheckBoxList.size(); CtlIdx++)
				{
					OldValue = mBlenderSettings[LblIdx].value;
					if (Update)
						mBlenderSettings[LblIdx].value = QString::number(gAddonPreferences->mCheckBoxList[CtlIdx]->isChecked());
					Modified |= mBlenderSettings[LblIdx].value != OldValue;
					if (LblIdx < LBL_VERBOSE)
						LblIdx++;
				}
			}
			// lineedits
			else if (LblIdx < LBL_COLOUR_SCHEME)
			{
				for (int CtlIdx = 0; CtlIdx < gAddonPreferences->mLineEditList.size(); CtlIdx++)
				{
					if (CtlIdx == CTL_IMAGE_WIDTH_EDIT)
					{
						_OldValue = Width;
						_Width  = GetNumericValue(gAddonPreferences->mLineEditList[CtlIdx], Ok);
						if (Ok)
						{
							if (Update)
							{
								Width = int(_Width);
								mBlenderSettings[LblIdx].value = QString::number(Width);
							}
							Modified |= ItemChanged(_OldValue, _Width);
						}
					}
					else if (CtlIdx == CTL_IMAGE_HEIGHT_EDIT)
					{
						_OldValue = Height;
						_Height = GetNumericValue(gAddonPreferences->mLineEditList[CtlIdx], Ok);
						if (Ok)
						{
							if (Update)
							{
								Height = int(_Height);
								mBlenderSettings[LblIdx].value = QString::number(Height);
							}
							Modified |= ItemChanged(_OldValue, _Height);
						}
					}
					else if (CtlIdx == CTL_RENDER_PERCENTAGE_EDIT)
					{
						_OldValue = Scale;
						_Scale = GetNumericValue(gAddonPreferences->mLineEditList[CtlIdx], Ok);
						if (Ok)
						{
							if (Update)
							{
								Scale = double(_Scale / 100);
								mBlenderSettings[LblIdx].value = QString::number(_Scale);
							}
							Modified |= ItemChanged(_OldValue, Scale);
						}
					}
					else
					{
						if (CtlIdx == CTL_DEFAULT_COLOUR_EDIT)
						{
							_OldValue = lcGetColorIndex(mBlenderSettings[LblIdx].value.toInt());  // colour code
							_Value = gAddonPreferences->mLineEditList[CtlIdx]->property("ColorIndex").toInt(&Ok);
						}
						else
						{
							_OldValue = mBlenderSettings[LblIdx].value.toDouble();
							_Value = GetNumericValue(gAddonPreferences->mLineEditList[CtlIdx], Ok);
						}
						if (Ok)
						{
							if (Update)
							{
								if (CtlIdx == CTL_DEFAULT_COLOUR_EDIT)
								{
									mBlenderSettings[LblIdx].value = QString::number(lcGetColorCode(qint32(_Value)));
								}
								else
								{
									mBlenderSettings[LblIdx].value = QString::number(_Value);
								}
							}
							Modified |= ItemChanged(_OldValue, _Value);
						}
					}
					if (LblIdx < LBL_RENDER_PERCENTAGE)
						LblIdx++;
				}
			}
			// comboboxes
			else
			{
				for (int CtlIdx = 0; CtlIdx < gAddonPreferences->mComboBoxList.size(); CtlIdx++)
				{
					OldValue = mBlenderSettings[LblIdx].value;
					const QString Value = gAddonPreferences->mComboBoxList[CtlIdx]->itemData(gAddonPreferences->mComboBoxList[CtlIdx]->currentIndex()).toString();
					if (Update)
						mBlenderSettings[LblIdx].value = Value;
					Modified |= Value != OldValue;
					LblIdx++;
				}
			}
		}
	}

	// paths
	for (int LblIdx = 0; LblIdx < NumPaths(); LblIdx++)
	{
		OldValue = mBlenderPaths[LblIdx].value;
		const QString Value = gAddonPreferences->mPathLineEditList[LblIdx]->text();
		if (Update)
			mBlenderPaths[LblIdx].value = Value;
		Modified |= Value != OldValue;
	}

	return Modified;
}

void lcBlenderPreferences::ResetSettings()
{
	BlenderPaths const* Paths = mBlenderPaths;
	BlenderSettings const* Settings = mBlenderSettings;
	BlenderSettings const* SettingsMM = mBlenderSettingsMM;

	QDialog* Dialog = new QDialog(this);
	Dialog->setWindowTitle(tr("Add-on Reset"));
	Dialog->setWhatsThis(tr("<p><b>Reset Add-on Settings</b></p><p>Choose Last Apply to restore the saved paths and settings, or System Default to load the built-in defaults. Choose OK to update the settings form, then Apply in the parent dialog to save them. Cancel keeps the current edits.</p>"));
	QVBoxLayout* Layout = new QVBoxLayout(Dialog);
	QGroupBox* DlgGroup = new QGroupBox(Dialog);
	QHBoxLayout* DlgLayout = new QHBoxLayout(DlgGroup);
	QRadioButton* LastButton = new QRadioButton(tr("Last Apply"));
	LastButton->setToolTip(tr("Restore paths and settings to the last applied values."));
	DlgLayout->addWidget(LastButton);
	QRadioButton* DefaultButton = new QRadioButton(tr("System Default"));
	DefaultButton->setToolTip(tr("Replace paths and settings with the built-in defaults."));
	DlgLayout->addWidget(DefaultButton);
	DlgGroup->setLayout(DlgLayout);
	Layout->addWidget(DlgGroup);
	Dialog->setLayout(Layout);

	LastButton->setChecked(true);

	QDialogButtonBox ButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help,
							   Qt::Horizontal, Dialog);
	Layout->addWidget(&ButtonBox);
	lcSetupDialogHelp(Dialog, &ButtonBox);
	connect(&ButtonBox, &QDialogButtonBox::accepted, Dialog, &QDialog::accept);
	connect(&ButtonBox, &QDialogButtonBox::rejected, Dialog, &QDialog::reject);

	if (Dialog->exec() != QDialog::Accepted)
		return;

	if (DefaultButton->isChecked())
	{
		Paths = mDefaultPaths;
		Settings = mDefaultSettings;
		SettingsMM = mDefaultSettingsMM;
	}

	mConfigured = !lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty();

	mBlenderPaths[PATH_BLENDER].value = lcGetProfileString(LC_PROFILE_BLENDER_PATH);
	mBlenderVersion                   = lcGetProfileString(LC_PROFILE_BLENDER_VERSION);
	mAddonVersion                     = lcGetProfileString(LC_PROFILE_BLENDER_ADDON_VERSION);

	mBlenderVersionEdit->setText(mBlenderVersion);
	mAddonVersionEdit->setText(mAddonVersion);

	if (mImportActBox->isChecked())
	{
		disconnect(mLineEditList[CTL_IMAGE_HEIGHT_EDIT], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
		disconnect(mLineEditList[CTL_IMAGE_WIDTH_EDIT], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);

		for (int LblIdx = 0; LblIdx < NumSettings(); LblIdx++)
		{
			if (LblIdx < LBL_BEVEL_WIDTH)
			{
				for (int CtlIdx = 0; CtlIdx < mCheckBoxList.size(); CtlIdx++)
				{
					mCheckBoxList[CtlIdx]->setChecked(Settings[LblIdx].value.toInt());
					if (LblIdx < LBL_VERBOSE)
						LblIdx++;
				}
			}
			else if (LblIdx < LBL_COLOUR_SCHEME)
			{
				for (int CtlIdx = 0; CtlIdx < mLineEditList.size(); CtlIdx++)
				{
					if (CtlIdx == CTL_IMAGE_WIDTH_EDIT)
						SetNumericText(mLineEditList[CtlIdx], QString::number(mImageWidth));
					else if (CtlIdx == CTL_IMAGE_HEIGHT_EDIT)
						SetNumericText(mLineEditList[CtlIdx], QString::number(mImageHeight));
					else if (CtlIdx == CTL_RENDER_PERCENTAGE_EDIT)
						SetNumericText(mLineEditList[CtlIdx], QString::number(mScale * 100));
					else if (CtlIdx == CTL_DEFAULT_COLOUR_EDIT)
						SetDefaultColor(lcGetColorIndex(Settings[LBL_DEFAULT_COLOUR].value.toInt()));
					else
						SetNumericText(mLineEditList[CtlIdx], Settings[LblIdx].value);
					if (LblIdx < LBL_RENDER_PERCENTAGE)
						LblIdx++;
				}
			}
			else
			{
				for (int CtlIdx = 0; CtlIdx < mComboBoxList.size(); CtlIdx++)
				{
					mComboBoxList[CtlIdx]->setCurrentIndex(int(mComboBoxList[CtlIdx]->findData(QVariant::fromValue(Settings[LblIdx].value))));
					LblIdx++;
				}
			}
		}

		for (int LblIdx = 0; LblIdx < NumPaths(); LblIdx++)
		{
			mPathLineEditList[LblIdx]->setText(Paths[LblIdx].value);
		}

		connect(mLineEditList[CTL_IMAGE_HEIGHT_EDIT], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
		connect(mLineEditList[CTL_IMAGE_WIDTH_EDIT], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
	}
	else if (mImportMMActBox->isChecked())
	{
		disconnect(mLineEditList[CTL_RESOLUTION_HEIGHT_EDIT], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
		disconnect(mLineEditList[CTL_RESOLUTION_WIDTH_EDIT], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);

		for (int LblIdx = 0; LblIdx < NumSettingsMM(); LblIdx++)
		{
			if (LblIdx < LBL_BEVEL_SEGMENTS)
			{
				for (int CtlIdx = 0; CtlIdx < mCheckBoxList.size(); CtlIdx++)
				{
					mCheckBoxList[CtlIdx]->setChecked(SettingsMM[LblIdx].value.toInt());
					if (LblIdx < LBL_VERBOSE_MM)
						LblIdx++;
				}
			}
			else if (LblIdx < LBL_CHOSEN_LOGO)
			{
				for (int CtlIdx = 0; CtlIdx < mLineEditList.size(); CtlIdx++)
				{
					if (CtlIdx == CTL_RESOLUTION_WIDTH_EDIT)
						SetNumericText(mLineEditList[CtlIdx], QString::number(mImageWidth));
					else if (CtlIdx == CTL_RESOLUTION_HEIGHT_EDIT)
						SetNumericText(mLineEditList[CtlIdx], QString::number(mImageHeight));
					else if (CtlIdx == CTL_RENDER_PERCENTAGE_EDIT_MM)
						SetNumericText(mLineEditList[CtlIdx], QString::number(mScale * 100));
					else
						SetNumericText(mLineEditList[CtlIdx], SettingsMM[LblIdx].value);
					if (LblIdx < LBL_STARTING_STEP_FRAME)
						LblIdx++;
				}
			}
			else
			{
				for (int CtlIdx = 0; CtlIdx < mComboBoxList.size(); CtlIdx++)
				{
					mComboBoxList[CtlIdx]->setCurrentIndex(int(mComboBoxList[CtlIdx]->findData(QVariant::fromValue(SettingsMM[LblIdx].value))));
					LblIdx++;
				}
			}
		}

		for (int LblIdx = 0; LblIdx < NumPaths(); LblIdx++)
		{
			mPathLineEditList[LblIdx]->setText(Paths[LblIdx].value);
		}

		connect(mLineEditList[CTL_RESOLUTION_HEIGHT_EDIT], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
		connect(mLineEditList[CTL_RESOLUTION_WIDTH_EDIT], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
	}

	emit SettingChangedSig(true);
}

QString lcBlenderPreferences::TranslateDefaultText(const QString& Source)
{
	QString Text = QCoreApplication::translate("QObject", Source.toUtf8().constData());

	// Default text uses %1 only for the product name; substitute after translation.
	if (Source.contains(QLatin1String("%1")))
		Text = Text.arg(QLatin1String(LC_PRODUCTNAME_STR));

	return Text;
}

void lcBlenderPreferences::LoadSettings()
{
	QStringList const& DataPathList = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
	gAddonPreferences->mDataDir = DataPathList.first();

	if (QFileInfo(lcGetProfileString(LC_PROFILE_BLENDER_PATH)).isReadable())
	{
		if (lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty())
			lcSetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE, QLatin1String(LC_BLENDER_ADDON_IMPORT_MODULE));
	}
	else
	{
		lcSetProfileString(LC_PROFILE_BLENDER_PATH, QString());
		lcSetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE, QString());
	}

	if (!QDir(QString("%1/Blender/addons/%2").arg(gAddonPreferences->mDataDir, LC_BLENDER_ADDON_RENDER_FOLDER)).isReadable())
		lcSetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE, QString());

	bool UseArchiveLibrary = false;
	auto LDrawPartsLibrary = [&] (QString const& LDrawPath)
	{
		QString LDrawDir = LDrawPath;

		const QFileInfo FileInfo(LDrawDir);
		if (FileInfo.isFile())
			LDrawDir = FileInfo.absolutePath();

		const QString Suffix = FileInfo.suffix().toLower();
		const QFileInfo PartInfo(QString("%1/p/1-4cyli.dat").arg(LDrawDir));
		const QFileInfo LDConfigInfo(QString("%1/LDConfig.ldr").arg(LDrawDir));

		UseArchiveLibrary = (Suffix == "zip" || Suffix == "bin") && !(PartInfo.exists() && LDConfigInfo.exists());

		return QDir::toNativeSeparators(LDrawDir);
	};

	if (!NumPaths())
	{
		const QString DefaultBlendFile = QString("%1/Blender/config/%2").arg(gAddonPreferences->mDataDir, LC_BLENDER_ADDON_BLEND_FILE);

		QStringList const AddonPaths = QStringList()
		/* 0  PATH_BLENDER      */        << lcGetProfileString(LC_PROFILE_BLENDER_PATH)
		/* 1  PATH_BLENDFILE    */        << (QFileInfo::exists(DefaultBlendFile) ? DefaultBlendFile : QString())
		/* 2  PATH_ENVIRONMENT  */        << QString()
		/* 3  PATH_LDCONFIG     */        << lcGetProfileString(LC_PROFILE_COLOR_CONFIG)
		/* 4  PATH_LDRAW        */        << LDrawPartsLibrary(lcGetProfileString(LC_PROFILE_PARTS_LIBRARY))
		/* 5  PATH_LSYNTH       */        << QString()
		/* 6  PATH_STUD_LOGO    */        << QString()
		/* 7  PATH_STUDIO_LDRAW */        << QString()
		/* 8  PATH_STUDIO_CUSTOM_PARTS */ << QString();
		for (int LblIdx = 0; LblIdx < NumPaths(DEFAULT_SETTINGS); LblIdx++)
		{
			mBlenderPaths[LblIdx] =
				{
					mDefaultPaths[LblIdx].key,
					mDefaultPaths[LblIdx].key_mm,
					QDir::toNativeSeparators(AddonPaths.at(LblIdx)),
					TranslateDefaultText(mDefaultPaths[LblIdx].label),
					TranslateDefaultText(mDefaultPaths[LblIdx].tooltip)
				};
		}
	}

	if (!NumSettings())
	{
		for (int LblIdx = 0; LblIdx < NumSettings(DEFAULT_SETTINGS); LblIdx++)
		{
			mBlenderSettings[LblIdx] =
				{
					mDefaultSettings[LblIdx].key,
					mDefaultSettings[LblIdx].value,
					TranslateDefaultText(mDefaultSettings[LblIdx].label),
					TranslateDefaultText(mDefaultSettings[LblIdx].tooltip)
				};
			if (LblIdx == LBL_USE_ARCHIVE_LIBRARY)
				mBlenderSettings[LblIdx].value = QString::number(UseArchiveLibrary);
		}
	}

	if (!NumSettingsMM())
	{
		for (int LblIdx = 0; LblIdx < NumSettingsMM(DEFAULT_SETTINGS); LblIdx++)
		{
			mBlenderSettingsMM[LblIdx] =
				{
					mDefaultSettingsMM[LblIdx].key,
					mDefaultSettingsMM[LblIdx].value,
					TranslateDefaultText(mDefaultSettingsMM[LblIdx].label),
					TranslateDefaultText(mDefaultSettingsMM[LblIdx].tooltip)
				};
			if (LblIdx == LBL_USE_ARCHIVE_LIBRARY_MM)
				mBlenderSettingsMM[LblIdx].value = QString::number(UseArchiveLibrary);
		}
	}

	QFileInfo BlenderConfigFileInfo(lcGetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH));

	if (BlenderConfigFileInfo.exists())
	{
		QSettings Settings(BlenderConfigFileInfo.absoluteFilePath(), QSettings::IniFormat);

		for (int LblIdx = 1/*skip blender executable*/; LblIdx < NumPaths(); LblIdx++)
		{
			if (LblIdx >= PATH_STUDIO_LDRAW)
				continue;
			const QString& Key = QString("%1/%2").arg(LC_BLENDER_ADDON, mBlenderPaths[LblIdx].key);
			const QString& Value = Settings.value(Key, QString()).toString();
			if (QFileInfo::exists(Value))
				mBlenderPaths[LblIdx].value = QDir::toNativeSeparators(Value);
		}

		for (int LblIdx = 1/*skip blender executable*/; LblIdx < NumPaths(); LblIdx++)
		{
			if (LblIdx == PATH_LSYNTH || LblIdx == PATH_STUD_LOGO)
				continue;
			const QString& Key = QString("%1/%2").arg(LC_BLENDER_ADDON_MM, mBlenderPaths[LblIdx].key_mm);
			const QString& Value = Settings.value(Key, QString()).toString();
			if (QFileInfo::exists(Value))
				mBlenderPaths[LblIdx].value = QDir::toNativeSeparators(Value);
		}

		for (int LblIdx = 0; LblIdx < NumSettings(); LblIdx++)
		{
			const QString& Key = QString("%1/%2").arg(LC_BLENDER_ADDON, mBlenderSettings[LblIdx].key);
			const QString& Value = Settings.value(Key, QString()).toString();
			if (!Value.isEmpty())
			{
				if (LblIdx == LBL_USE_ARCHIVE_LIBRARY && Value == "False")
					mBlenderSettings[LblIdx].value = QString::number(UseArchiveLibrary);
				else
					mBlenderSettings[LblIdx].value = Value == "True" ? "1" : Value == "False" ? "0" : Value;
			}
			if (LblIdx == LBL_IMAGE_WIDTH || LblIdx == LBL_IMAGE_HEIGHT || LblIdx == LBL_RENDER_PERCENTAGE)
			{
				const QString Label = TranslateDefaultText(mDefaultSettings[LblIdx].label);
				mBlenderSettings[LblIdx].label = tr("%1 - Setting (%2)").arg(Label, Value);
			}
		}

		for (int LblIdx = 0; LblIdx < NumSettingsMM(); LblIdx++)
		{
			const QString& Key = QString("%1/%2").arg(LC_BLENDER_ADDON_MM, mBlenderSettingsMM[LblIdx].key);
			const QString& Value = Settings.value(Key, QString()).toString();
			if (!Value.isEmpty())
			{
				if (LblIdx == LBL_USE_ARCHIVE_LIBRARY_MM && Value == "False")
					mDefaultSettingsMM[LblIdx].value = QString::number(UseArchiveLibrary);
				else
					mDefaultSettingsMM[LblIdx].value = Value == "True" ? "1" : Value == "False" ? "0" : Value;
			}
			if (LblIdx == LBL_RENDER_PERCENTAGE_MM || LblIdx == LBL_RESOLUTION_WIDTH || LblIdx == LBL_RESOLUTION_HEIGHT)
			{
				const QString Label = TranslateDefaultText(mDefaultSettingsMM[LblIdx].label);
				mBlenderSettingsMM[LblIdx].label = tr("%1 - Setting (%2)").arg(Label, Value);
			}
		}
	}
	else
	{
		const QString LogFile = QString("%1/Blender/stdout-blender-addon-install").arg(gAddonPreferences->mDataDir);
		if (QFileInfo(LogFile).isReadable())
		{
			QFile File(LogFile);
			if (File.open(QFile::ReadOnly | QFile::Text))
			{
				QByteArray Ba = File.readAll();
				File.close();
				QString Errors;
				gAddonPreferences->mProgressBar = nullptr;
				gAddonPreferences->ReadStdOut(QString(Ba), Errors);

				if (!Errors.isEmpty() && gMainWindow)
					QMessageBox::warning(nullptr, tr("Load Blender Add-on Settings"), Errors);
			}
			else
			{
				if (gMainWindow)
					QMessageBox::warning(nullptr, tr("Load Blender Add-on Settings"), tr("Blender config file was not found. "
							   "Install log check failed:<br>%1:<br>%2")
								.arg(File.fileName(), File.errorString()));
			}
		}
	}

	mBlenderSettings[LBL_IMAGE_WIDTH].value = QString::number(gAddonPreferences->mImageWidth);
	mBlenderSettings[LBL_IMAGE_HEIGHT].value = QString::number(gAddonPreferences->mImageHeight);
	mBlenderSettings[LBL_RENDER_PERCENTAGE].value = QString::number(gAddonPreferences->mScale * 100);

	mBlenderSettingsMM[LBL_RESOLUTION_WIDTH].value = QString::number(gAddonPreferences->mImageWidth);
	mBlenderSettingsMM[LBL_RESOLUTION_HEIGHT].value = QString::number(gAddonPreferences->mImageHeight);
	mBlenderSettingsMM[LBL_RENDER_PERCENTAGE_MM].value = QString::number(gAddonPreferences->mScale * 100);

	mBlenderPaths[PATH_BLENDER].value = lcGetProfileString(LC_PROFILE_BLENDER_PATH);
	gAddonPreferences->mBlenderVersion = lcGetProfileString(LC_PROFILE_BLENDER_VERSION);
	gAddonPreferences->mAddonVersion = lcGetProfileString(LC_PROFILE_BLENDER_ADDON_VERSION);
}

void lcBlenderPreferences::SaveSettings()
{
	if (!NumSettings() || !NumSettingsMM())
		LoadSettings();

	QString Key, Value = mBlenderPaths[PATH_BLENDER].value;
	if (Value.isEmpty())
		Value = gAddonPreferences->mPathLineEditList[PATH_BLENDER]->text();

	lcSetProfileString(LC_PROFILE_BLENDER_PATH, QDir::toNativeSeparators(Value));

	Value.clear();
	if (!gAddonPreferences->mBlenderVersion.isEmpty())
		Value = gAddonPreferences->mBlenderVersion;

	if (!gAddonPreferences->mAddonVersion.isEmpty())
	{
		gAddonPreferences->mModulesBox->setEnabled(true);
		gAddonPreferences->mAddonVersionEdit->setText(gAddonPreferences->mAddonVersion);
	}

	lcSetProfileString(LC_PROFILE_BLENDER_VERSION, Value);

	const QString BlenderConfigDir = QString("%1/Blender/addons/%2/config").arg(gAddonPreferences->mDataDir, LC_BLENDER_ADDON_RENDER_FOLDER);

	Value = lcGetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH);
	if (!Value.contains(LC_BLENDER_ADDON_RENDER_FOLDER))
	{
		Value = QString("%1/%2").arg(BlenderConfigDir, LC_BLENDER_ADDON_CONFIG_FILE);
		lcSetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH, QDir::toNativeSeparators(Value));
	}

	QString searchDirectoriesKey = QLatin1String("additionalSearchPaths");
	QString parameterFileKey = QLatin1String("ParameterFile");
	QString ParameterFile = QString("%1/%2").arg(BlenderConfigDir, LC_BLENDER_ADDON_PARAMS_FILE);

	QSettings Settings(Value, QSettings::IniFormat);

	auto concludeSettingsGroup = [&]()
	{
		if (!QFileInfo::exists(ParameterFile))
			ExportParameterFile();
		Value = QDir::toNativeSeparators(QFileInfo(lcGetProfileString(LC_PROFILE_PROJECTS_PATH)).absolutePath());
		Settings.setValue(searchDirectoriesKey, QVariant(Value));
		Settings.endGroup();
	};

	Settings.beginGroup(LC_BLENDER_ADDON);

	for (int LblIdx = 1/*skip blender executable*/; LblIdx < NumPaths(); LblIdx++)
	{
		if (LblIdx >= PATH_STUDIO_LDRAW)
			continue;
		Key = mBlenderPaths[LblIdx].key;
		Value = QDir::toNativeSeparators(mBlenderPaths[LblIdx].value);

		if (!Key.isEmpty())
			Settings.setValue(Key, QVariant(Value));
	}

	for (int LblIdx = 0; LblIdx < NumSettings(); LblIdx++)
	{
		if (LblIdx == LBL_KEEP_ASPECT_RATIO)
		{
			continue;
		}
		else if (LblIdx < LBL_BEVEL_WIDTH)
		{
			Value = mBlenderSettings[LblIdx].value == "1" ? "True" : "False";
		}
		else if (LblIdx > LBL_VERBOSE)
		{
			if (LblIdx == LBL_FLEX_PARTS_SOURCE || LblIdx == LBL_POSITION_OBJECT)
				Value = mBlenderSettings[LblIdx].value == "1" ? "True" : "False";
			else
				Value = mBlenderSettings[LblIdx].value;
		}

		Key = mBlenderSettings[LblIdx].key;

		if (!Key.isEmpty())
			Settings.setValue(Key, QVariant(Value));
	}

	Settings.setValue(parameterFileKey, QVariant(QDir::toNativeSeparators(ParameterFile)));

	concludeSettingsGroup();

	Settings.beginGroup(LC_BLENDER_ADDON_MM);

	for (int LblIdx = 1/*skip blender executable*/; LblIdx < NumPaths(); LblIdx++)
	{
		if (LblIdx == PATH_LSYNTH || LblIdx == PATH_STUD_LOGO)
			continue;
		Key = mBlenderPaths[LblIdx].key_mm;
		Value = QDir::toNativeSeparators(mBlenderPaths[LblIdx].value);

		if (!Key.isEmpty())
			Settings.setValue(Key, QVariant(Value));
	}

	for (int LblIdx = 0; LblIdx < NumSettingsMM(); LblIdx++)
	{
		if (LblIdx == LBL_KEEP_ASPECT_RATIO_MM)
		{
			continue;
		}
		else if (LblIdx < LBL_BEVEL_SEGMENTS)
		{
			Value = mBlenderSettingsMM[LblIdx].value == "1" ? "True" : "False";
		}
		else if (LblIdx > LBL_VERBOSE_MM)
		{
			Value = mBlenderSettingsMM[LblIdx].value;
		}

		Key = mBlenderSettingsMM[LblIdx].key;

		if (!Key.isEmpty())
			Settings.setValue(Key, QVariant(Value));
	}

	concludeSettingsGroup();

	const QString preferredImportModule = gAddonPreferences->mImportActBox->isChecked() ? QString("TN") : gAddonPreferences->mImportMMActBox->isChecked() ? QString("MM") : QString();

	if (preferredImportModule != lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE))
		lcSetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE, preferredImportModule);
}

void lcBlenderPreferences::EnableImportModule()
{
	QString saveImportModule, preferredImportModule;
	if (sender() == mImportActBox && mImportActBox->isChecked())
	{
		preferredImportModule = QLatin1String("TN");
		if (mImportMMActBox->isChecked())
			saveImportModule = QLatin1String("MM");
		mImportMMActBox->setChecked(false);
	}
	else if (sender() == mImportMMActBox && mImportMMActBox->isChecked())
	{
		preferredImportModule = QLatin1String("MM");
		if (mImportActBox->isChecked())
			saveImportModule = QLatin1String("TN");
		mImportActBox->setChecked(false);
	}

	if (preferredImportModule.isEmpty())
		return;

	if (SettingsModified(true, saveImportModule))
		SaveSettings();

	lcSetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE, preferredImportModule);

	if (mImportMMActBox->isChecked())
		InitPathsAndSettingsMM();
	else
		InitPathsAndSettings();

	ConfigureBlenderAddon(false, false, true);
}

int lcBlenderPreferences::NumSettings(bool DefaultSettings)
{
	int Size = 0;
	if (!mBlenderSettings[0].key.isEmpty() || DefaultSettings)
		Size = sizeof(mBlenderSettings)/sizeof(mBlenderSettings[0]);
	return Size;
}

int lcBlenderPreferences::NumSettingsMM(bool DefaultSettings)
{
	int Size = 0;
	if (!mBlenderSettingsMM[0].key.isEmpty() || DefaultSettings)
		Size = sizeof(mBlenderSettingsMM)/sizeof(mBlenderSettingsMM[0]);
	return Size;
}

int lcBlenderPreferences::NumPaths(bool DefaultSettings)
{
	int Size = 0;
	if (!mBlenderPaths[0].key.isEmpty() || DefaultSettings)
		Size = sizeof(mBlenderPaths)/sizeof(mBlenderPaths[0]);
	return Size;
}

void lcBlenderPreferences::ShowPathsGroup()
{
	if (mPathsBox->isHidden())
		mPathsBox->show();
	else
		mPathsBox->hide();

	mContent->adjustSize();
}

void lcBlenderPreferences::ColorButtonClicked(bool)
{
	int ColorIndex = mLineEditList[CTL_DEFAULT_COLOUR_EDIT]->property("ColorIndex").toInt();

	QWidget* Parent = mLineEditList[CTL_DEFAULT_COLOUR_EDIT];
	lcColorPickerPopup* Popup = new lcColorPickerPopup(Parent, ColorIndex);
	connect(Popup, &lcColorPickerPopup::Selected, this, &lcBlenderPreferences::SetDefaultColor);
	Popup->setMinimumSize(300, 200);
	Popup->SetPosition(Parent);

	Popup->setFocus();
	Popup->show();
}

void lcBlenderPreferences::SetDefaultColor(int ColorIndex)
{
	QImage Image(12, 12, QImage::Format_ARGB32);
	Image.fill(0);

	lcColor* Colour =& gColorList[ColorIndex];
	QPainter Painter(&Image);
	Painter.setCompositionMode(QPainter::CompositionMode_Source);
	Painter.setPen(Qt::darkGray);
	Painter.setBrush(QColor::fromRgbF(qreal(Colour->Value[0]), qreal(Colour->Value[1]), qreal(Colour->Value[2])));
	Painter.drawRect(0, 0, Image.width() - 1, Image.height() - 1);
	Painter.end();

	int const ColourCode = lcGetColorCode(ColorIndex);
	mLineEditList[CTL_DEFAULT_COLOUR_EDIT]->setText(QString("%1 (%2)").arg(Colour->Name).arg(ColourCode));
	mLineEditList[CTL_DEFAULT_COLOUR_EDIT]->setProperty("ColorIndex", QVariant::fromValue(ColorIndex));
	mDefaultColourEditAction->setIcon(QPixmap::fromImage(Image));
	mDefaultColourEditAction->setToolTip(tr("Select Color"));

	bool Change = mBlenderSettings[LBL_DEFAULT_COLOUR].value != QString::number(ColourCode);
	Change |= SettingsModified(false);
	emit SettingChangedSig(Change);
}

void lcBlenderPreferences::BrowseBlender(bool)
{
	for (int LblIdx = 0; LblIdx < NumPaths(); ++LblIdx)
	{
		if (sender() == mPathBrowseButtonList.at(LblIdx))
		{
			const QString BlenderPath = QDir::toNativeSeparators(mBlenderPaths[LblIdx].value).toLower();
			QFileDialog FileDialog(nullptr);
			FileDialog.setWindowTitle(tr("Locate %1").arg(mBlenderPaths[LblIdx].label));
			if (LblIdx < PATH_LDRAW)
				FileDialog.setFileMode(QFileDialog::ExistingFile);
			else
				FileDialog.setFileMode(QFileDialog::Directory);
			if (!BlenderPath.isEmpty())
				FileDialog.setDirectory(QFileInfo(BlenderPath).absolutePath());
			if (FileDialog.exec())
			{
				QStringList SelectedPathList = FileDialog.selectedFiles();
				if (SelectedPathList.size() == 1)
				{
					QFileInfo  PathInfo(SelectedPathList.at(0));
					if (PathInfo.exists())
					{
						const QString SelectedPath = QDir::toNativeSeparators(PathInfo.absoluteFilePath()).toLower();
						mPathLineEditList[LblIdx]->setText(SelectedPathList.at(0));
						if (LblIdx != PATH_BLENDER)
						{
							bool Change = false;
							if (mImportMMActBox->isChecked())
								Change = QDir::toNativeSeparators(mBlenderSettingsMM[LblIdx].value).toLower() != SelectedPath;
							else
								Change = QDir::toNativeSeparators(mBlenderSettings[LblIdx].value).toLower() != SelectedPath;
							Change |= SettingsModified(false);
							emit SettingChangedSig(Change);
						}

						if (LblIdx == PATH_BLENDER && BlenderPath != SelectedPath)
						{
							mBlenderPaths[LblIdx].value = SelectedPath;
							UpdateBlenderAddon();
						}
					}
				}
			}
		}
	}
}

void lcBlenderPreferences::SizeChanged(const QString&)
{
	const bool ImportMM = mImportMMActBox->isChecked();
	const int Keep_Aspect_Ratio = ImportMM ? int(CTL_KEEP_ASPECT_RATIO_BOX_MM) : int(CTL_KEEP_ASPECT_RATIO_BOX);
	const int Width_Edit = ImportMM ? int(CTL_RESOLUTION_WIDTH_EDIT) : int(CTL_IMAGE_WIDTH_EDIT);
	const int Height_Edit = ImportMM ? int(CTL_RESOLUTION_HEIGHT_EDIT) : int(CTL_IMAGE_HEIGHT_EDIT);
	BlenderSettings const* Settings = ImportMM ? mBlenderSettingsMM : mBlenderSettings;

	bool Change = false;
	const QLineEdit* Edit = qobject_cast<const QLineEdit*>(sender());
	bool Ok = false;
	const int NewValue = Edit ? static_cast<int>(GetNumericValue(Edit, Ok)) : 0;

	if (!Edit || !Ok)
		return;
	if (mCheckBoxList[Keep_Aspect_Ratio]->isChecked())
	{
		if (sender() == mLineEditList[Width_Edit])
		{
			disconnect(mLineEditList[Height_Edit], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);

			const QString Height = QString::number(qRound(double(mImageHeight) * NewValue / mImageWidth));
			SetNumericText(mLineEditList[Height_Edit], Height);

			Change = Settings[Height_Edit].value != Height;

			connect(mLineEditList[Height_Edit], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
		}
		else if (sender() == mLineEditList[Height_Edit])
		{
			disconnect(mLineEditList[Width_Edit], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);

			const QString Width = QString::number(qRound(double(NewValue) * mImageWidth / mImageHeight));
			SetNumericText(mLineEditList[Width_Edit], Width);

			Change = Settings[Height_Edit].value != Width;

			connect(mLineEditList[Width_Edit], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
		}

		// Change is provided here for consistency only as ImageWidth,
		// ImageHeight, and RenderPercentage are passed at the render command
		Change |= SettingsModified(false);
		emit SettingChangedSig(Change);
	}
}

void lcBlenderPreferences::SetModelSize(bool Update)
{
	const bool ImportMM = mImportMMActBox->isChecked();
	const int Crop_Image = ImportMM ? int(CTL_CROP_IMAGE_BOX_MM) : int(CTL_CROP_IMAGE_BOX);
	const int Add_Environment = ImportMM ? int(CTL_ADD_ENVIRONMENT_BOX_MM) : int(CTL_ADD_ENVIRONMENT_BOX);
	const int Trans_Background = ImportMM ? int(CTL_TRANSPARENT_BACKGROUND_BOX_MM) : int(CTL_TRANSPARENT_BACKGROUND_BOX);
	const int Keep_Aspect_Ratio = ImportMM ? int(CTL_KEEP_ASPECT_RATIO_BOX_MM) : int(CTL_KEEP_ASPECT_RATIO_BOX);
	const int Width_Edit = ImportMM ? int(CTL_RESOLUTION_WIDTH_EDIT) : int(CTL_IMAGE_WIDTH_EDIT);
	const int Height_Edit = ImportMM ? int(CTL_RESOLUTION_HEIGHT_EDIT) : int(CTL_IMAGE_HEIGHT_EDIT);
	const int Crop_Image_Label = ImportMM ? int(LBL_CROP_IMAGE_MM) : int(LBL_CROP_IMAGE);

	const QString CropImageLabel = mSettingLabelList[Crop_Image_Label]->text();

	int ImageWidth = mImageWidth;
	int ImageHeight = mImageHeight;

	const bool CropImage = mCheckBoxList[Crop_Image]->isChecked();

	lcModel* Model = lcGetActiveProject()->GetActiveModel();
	if (Model)
	{
		struct NativeImage
		{
			QImage RenderedImage;
			QRect Bounds;
		};

		std::vector<NativeImage> Images;
		Images.push_back(NativeImage());
		NativeImage& Image = Images.back();
		Image.RenderedImage = Model->GetStepImage(false, ImageWidth, ImageHeight, Model->GetCurrentStep());

		if (Image.RenderedImage.isNull())
			return;

		auto CalculateImageBounds = [](NativeImage& Image)
		{
			QImage& RenderedImage = Image.RenderedImage;
			int Width  = RenderedImage.width();
			int Height = RenderedImage.height();

			int MinX = Width;
			int MinY = Height;
			int MaxX = 0;
			int MaxY = 0;

			for (int x = 0; x < Width; x++)
			{
				for (int y = 0; y < Height; y++)
				{
					if (qAlpha(RenderedImage.pixel(x, y)))
					{
						MinX = qMin(x, MinX);
						MinY = qMin(y, MinY);
						MaxX = qMax(x, MaxX);
						MaxY = qMax(y, MaxY);
					}
				}
			}

			Image.Bounds = QRect(QPoint(MinX, MinY), QPoint(MaxX, MaxY));
		};

		QtConcurrent::blockingMap(Images, CalculateImageBounds);

		ImageWidth = Image.Bounds.width();
		ImageHeight = Image.Bounds.height();
	}

	mSettingLabelList[Crop_Image_Label]->setText(QString("%1 (%2 x %3)").arg(CropImageLabel).arg(ImageWidth).arg(ImageHeight));

	if (CropImage)
	{
		bool Conflict[3];

		if ((Conflict[1] = mCheckBoxList[Add_Environment]->isChecked()))
			mCheckBoxList[Add_Environment]->setChecked(!CropImage);
		if ((Conflict[2] = !mCheckBoxList[Trans_Background]->isChecked()))
			mCheckBoxList[Trans_Background]->setChecked(CropImage);
		if ((Conflict[0] = mCheckBoxList[Keep_Aspect_Ratio]->isChecked()))
			mCheckBoxList[Keep_Aspect_Ratio]->setChecked(!CropImage);

		if (Conflict[0] || Conflict[1] || Conflict[2])
		{
			const QString& Title = tr("Apply Render Settings");
			const QString& Header = "<b>" + tr ("Crop image configuration settings conflict were resolved.") + "</b>";
			const QString& Body = QString("%1%2%3").arg(Conflict[0] ? tr("Keep aspect ratio set to false.<br>") : "", Conflict[1] ? tr("Add environment (backdrop and base plane) set to false.<br>") : "", Conflict[2] ? tr("Transparent background set to true.<br>") : "");
			ShowMessage(this, Header, Title, Body, QString(), MBB_OK, QMessageBox::Information);
		}
	}

	disconnect(mLineEditList[Width_Edit], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
	disconnect(mLineEditList[Height_Edit], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);

	const QString Width = QString::number(CropImage ? ImageWidth : mImageWidth);
	const QString Height = QString::number(CropImage ? ImageHeight : mImageHeight);

	SetNumericText(mLineEditList[Width_Edit], Width);
	SetNumericText(mLineEditList[Height_Edit], Height);

	if (Update)
		SettingsModified(true);

	connect(mLineEditList[Height_Edit], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
	connect(mLineEditList[Width_Edit], &QLineEdit::textChanged, this, &lcBlenderPreferences::SizeChanged);
}

void lcBlenderPreferences::ValidateColourScheme(int Index)
{
	QComboBox* ComboBox = qobject_cast<QComboBox*>(sender());
	if (!ComboBox)
		return;

	const bool ImportMM = mImportMMActBox->isChecked();
	const int Color_Scheme = ImportMM ? int(LBL_COLOUR_SCHEME_MM) : int(LBL_COLOUR_SCHEME);
	BlenderSettings* Settings = ImportMM ? mBlenderSettingsMM : mBlenderSettings;

	if (ComboBox->itemText(Index) == "custom" && mBlenderPaths[PATH_LDCONFIG].value.isEmpty() && lcGetProfileString(LC_PROFILE_COLOR_CONFIG).isEmpty())
	{
		BlenderSettings const* defaultSettings = ImportMM ? mDefaultSettingsMM : mDefaultSettings;
		Settings[Color_Scheme].value = defaultSettings[Color_Scheme].value;

		const QString& Title = tr("Set LDraw Color Scheme");
		const QString& Header = "<b>" + tr ("Color scheme 'custom' cannot be enabled. Custom LDConfig file not found.") + "</b>";
		const QString& Body = tr ("Color scheme 'custom' selected but no LDConfig file was specified.<br>The default color scheme '%1' will be used.<br>").arg(Settings[Color_Scheme].value);
		ShowMessage(this, Header, Title, Body, QString(), MBB_OK, QMessageBox::Warning);
	}
	else
	{
		bool Change = Settings[Color_Scheme].value != ComboBox->itemText(Index);
		Change |= SettingsModified(false);
		emit SettingChangedSig(Change);
	}
}

bool lcBlenderPreferences::PromptAccept()
{
	if (!gMainWindow)
		return false;

	 const QString& Title = tr("Apply Render Settings");
	 const QString& Header = "<b>" + tr("Do you want to accept the modified settings before quitting?") + "</b>";
	int Exec = QMessageBox::question(this, Title, Header, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
	if (Exec == QMessageBox::Yes)
		return true;

	return false;
}

void lcBlenderPreferences::LoadDefaultParameters(QByteArray& Buffer, int Which)
{
	/*
	# File: BlenderLDrawParameters.lst
	#
	# This config file captures parameters for the Blender LDraw Render addon
	# Parameters must be prefixed using one of the following predefined
	# 'Item' labels:
	# - lgeo_colour
	# - sloped_brick
	# - light_brick

	# LGEO CUSTOM COLOURS
	# LGEO is a parts library for rendering LEGO using the POV-Ray
	# rendering software. This is the list of LEGO colours suitable
	# for realistic rendering extracted from the LGEO file 'lg_color.inc'.
	# When the 'Colour Scheme' option is set to 'Realistic', the standard
	# LDraw colours RGB value is overwritten with the values defined here.
	# Note: You can customize these RGB values as you want.

	#   Item-------  ID--    R--   G--   B--
	*/
	const char DefaultCustomColours[] =
		{
			"lgeo_colour,   0,    33,   33,   33\n"
			"lgeo_colour,   1,    13,  105,  171\n"
			"lgeo_colour,   2,    40,  127,   70\n"
			"lgeo_colour,   3,     0,  143,  155\n"
			"lgeo_colour,   4,   196,   40,   27\n"
			"lgeo_colour,   5,   205,   98,  152\n"
			"lgeo_colour,   6,    98,   71,   50\n"
			"lgeo_colour,   7,   161,  165,  162\n"
			"lgeo_colour,   8,   109,  110,  108\n"
			"lgeo_colour,   9,   180,  210,  227\n"
			"lgeo_colour,   10,   75,  151,   74\n"
			"lgeo_colour,   11,   85,  165,  175\n"
			"lgeo_colour,   12,  242,  112,   94\n"
			"lgeo_colour,   13,  252,  151,  172\n"
			"lgeo_colour,   14,  245,  205,   47\n"
			"lgeo_colour,   15,  242,  243,  242\n"
			"lgeo_colour,   17,  194,  218,  184\n"
			"lgeo_colour,   18,  249,  233,  153\n"
			"lgeo_colour,   19,  215,  197,  153\n"
			"lgeo_colour,   20,  193,  202,  222\n"
			"lgeo_colour,   21,  224,  255,  176\n"
			"lgeo_colour,   22,  107,   50,  123\n"
			"lgeo_colour,   23,   35,   71,  139\n"
			"lgeo_colour,   25,  218,  133,   64\n"
			"lgeo_colour,   26,  146,   57,  120\n"
			"lgeo_colour,   27,  164,  189,   70\n"
			"lgeo_colour,   28,  149,  138,  115\n"
			"lgeo_colour,   29,  228,  173,  200\n"
			"lgeo_colour,   30,  172,  120,  186\n"
			"lgeo_colour,   31,  225,  213,  237\n"
			"lgeo_colour,   32,    0,   20,   20\n"
			"lgeo_colour,   33,  123,  182,  232\n"
			"lgeo_colour,   34,  132,  182,  141\n"
			"lgeo_colour,   35,  217,  228,  167\n"
			"lgeo_colour,   36,  205,   84,   75\n"
			"lgeo_colour,   37,  228,  173,  200\n"
			"lgeo_colour,   38,  255,   43,    0\n"
			"lgeo_colour,   40,  166,  145,  130\n"
			"lgeo_colour,   41,  170,  229,  255\n"
			"lgeo_colour,   42,  198,  255,    0\n"
			"lgeo_colour,   43,  193,  223,  240\n"
			"lgeo_colour,   44,  150,  112,  159\n"
			"lgeo_colour,   46,  247,  241,  141\n"
			"lgeo_colour,   47,  252,  252,  252\n"
			"lgeo_colour,   52,  156,  149,  199\n"
			"lgeo_colour,   54,  255,  246,  123\n"
			"lgeo_colour,   57,  226,  176,   96\n"
			"lgeo_colour,   65,  236,  201,   53\n"
			"lgeo_colour,   66,  202,  176,    0\n"
			"lgeo_colour,   67,  255,  255,  255\n"
			"lgeo_colour,   68,  243,  207,  155\n"
			"lgeo_colour,   69,  142,   66,  133\n"
			"lgeo_colour,   70,  105,   64,   39\n"
			"lgeo_colour,   71,  163,  162,  164\n"
			"lgeo_colour,   72,   99,   95,   97\n"
			"lgeo_colour,   73,  110,  153,  201\n"
			"lgeo_colour,   74,  161,  196,  139\n"
			"lgeo_colour,   77,  220,  144,  149\n"
			"lgeo_colour,   78,  246,  215,  179\n"
			"lgeo_colour,   79,  255,  255,  255\n"
			"lgeo_colour,   80,  140,  140,  140\n"
			"lgeo_colour,   82,  219,  172,   52\n"
			"lgeo_colour,   84,  170,  125,   85\n"
			"lgeo_colour,   85,   52,   43,  117\n"
			"lgeo_colour,   86,  124,   92,   69\n"
			"lgeo_colour,   89,  155,  178,  239\n"
			"lgeo_colour,   92,  204,  142,  104\n"
			"lgeo_colour,  100,  238,  196,  182\n"
			"lgeo_colour,  115,  199,  210,   60\n"
			"lgeo_colour,  134,  174,  122,   89\n"
			"lgeo_colour,  135,  171,  173,  172\n"
			"lgeo_colour,  137,  106,  122,  150\n"
			"lgeo_colour,  142,  220,  188,  129\n"
			"lgeo_colour,  148,   62,   60,   57\n"
			"lgeo_colour,  151,   14,   94,   77\n"
			"lgeo_colour,  179,  160,  160,  160\n"
			"lgeo_colour,  183,  242,  243,  242\n"
			"lgeo_colour,  191,  248,  187,   61\n"
			"lgeo_colour,  212,  159,  195,  233\n"
			"lgeo_colour,  216,  143,   76,   42\n"
			"lgeo_colour,  226,  253,  234,  140\n"
			"lgeo_colour,  232,  125,  187,  221\n"
			"lgeo_colour,  256,   33,   33,   33\n"
			"lgeo_colour,  272,   32,   58,   86\n"
			"lgeo_colour,  273,   13,  105,  171\n"
			"lgeo_colour,  288,   39,   70,   44\n"
			"lgeo_colour,  294,  189,  198,  173\n"
			"lgeo_colour,  297,  170,  127,   46\n"
			"lgeo_colour,  308,   53,   33,    0\n"
			"lgeo_colour,  313,  171,  217,  255\n"
			"lgeo_colour,  320,  123,   46,   47\n"
			"lgeo_colour,  321,   70,  155,  195\n"
			"lgeo_colour,  322,  104,  195,  226\n"
			"lgeo_colour,  323,  211,  242,  234\n"
			"lgeo_colour,  324,  196,    0,   38\n"
			"lgeo_colour,  326,  226,  249,  154\n"
			"lgeo_colour,  330,  119,  119,   78\n"
			"lgeo_colour,  334,  187,  165,   61\n"
			"lgeo_colour,  335,  149,  121,  118\n"
			"lgeo_colour,  366,  209,  131,    4\n"
			"lgeo_colour,  373,  135,  124,  144\n"
			"lgeo_colour,  375,  193,  194,  193\n"
			"lgeo_colour,  378,  120,  144,  129\n"
			"lgeo_colour,  379,   94,  116,  140\n"
			"lgeo_colour,  383,  224,  224,  224\n"
			"lgeo_colour,  406,    0,   29,  104\n"
			"lgeo_colour,  449,  129,    0,  123\n"
			"lgeo_colour,  450,  203,  132,   66\n"
			"lgeo_colour,  462,  226,  155,   63\n"
			"lgeo_colour,  484,  160,   95,   52\n"
			"lgeo_colour,  490,  215,  240,    0\n"
			"lgeo_colour,  493,  101,  103,   97\n"
			"lgeo_colour,  494,  208,  208,  208\n"
			"lgeo_colour,  496,  163,  162,  164\n"
			"lgeo_colour,  503,  199,  193,  183\n"
			"lgeo_colour,  504,  137,  135,  136\n"
			"lgeo_colour,  511,  250,  250,  250\n"
		};

	/*
	# SLOPED BRICKS
	# Dictionary with part number (without any extension for decorations), as key,
	# of pieces that have grainy slopes, and, as values, a set containing the angles (in
	# degrees) of the face's normal to the horizontal plane. Use a | delimited tuple to
	# represent a range within which the angle must lie.

	#   Item--------  PartID-  Angle/Angle Range (in degrees)
	*/
	const char DefaultSlopedBricks[] =
		{
			"sloped_brick,     962,  45\n"
			"sloped_brick,    2341, -45\n"
			"sloped_brick,    2449, -16\n"
			"sloped_brick,    2875,  45\n"
			"sloped_brick,    2876,  40|63\n"
			"sloped_brick,    3037,  45\n"
			"sloped_brick,    3038,  45\n"
			"sloped_brick,    3039,  45\n"
			"sloped_brick,    3040,  45\n"
			"sloped_brick,    3041,  45\n"
			"sloped_brick,    3042,  45\n"
			"sloped_brick,    3043,  45\n"
			"sloped_brick,    3044,  45\n"
			"sloped_brick,    3045,  45\n"
			"sloped_brick,    3046,  45\n"
			"sloped_brick,    3048,  45\n"
			"sloped_brick,    3049,  45\n"
			"sloped_brick,    3135,  45\n"
			"sloped_brick,    3297,  63\n"
			"sloped_brick,    3298,  63\n"
			"sloped_brick,    3299,  63\n"
			"sloped_brick,    3300,  63\n"
			"sloped_brick,    3660, -45\n"
			"sloped_brick,    3665, -45\n"
			"sloped_brick,    3675,  63\n"
			"sloped_brick,    3676, -45\n"
			"sloped_brick,   3678b,  24\n"
			"sloped_brick,    3684,  15\n"
			"sloped_brick,    3685,  16\n"
			"sloped_brick,    3688,  15\n"
			"sloped_brick,    3747, -63\n"
			"sloped_brick,    4089, -63\n"
			"sloped_brick,    4161,  63\n"
			"sloped_brick,    4286,  63\n"
			"sloped_brick,    4287, -63\n"
			"sloped_brick,    4445,  45\n"
			"sloped_brick,    4460,  16\n"
			"sloped_brick,    4509,  63\n"
			"sloped_brick,    4854, -45\n"
			"sloped_brick,    4856, -60|-70, -45\n"
			"sloped_brick,    4857,  45\n"
			"sloped_brick,    4858,  72\n"
			"sloped_brick,    4861,  45,      63\n"
			"sloped_brick,    4871, -45\n"
			"sloped_brick,    4885,  72\n"
			"sloped_brick,    6069,  72,      45\n"
			"sloped_brick,    6153,  60|70,   26|4\n"
			"sloped_brick,    6227,  45\n"
			"sloped_brick,    6270,  45\n"
			"sloped_brick,   13269,  40|63\n"
			"sloped_brick,   13548,  45\n"
			"sloped_brick,   15571,  45\n"
			"sloped_brick,   18759, -45\n"
			"sloped_brick,   22390,  40|55\n"
			"sloped_brick,   22391,  40|55\n"
			"sloped_brick,   22889, -45\n"
			"sloped_brick,   28192,  45\n"
			"sloped_brick,   30180,  47\n"
			"sloped_brick,   30182,  45\n"
			"sloped_brick,   30183, -45\n"
			"sloped_brick,   30249,  35\n"
			"sloped_brick,   30283, -45\n"
			"sloped_brick,   30363,  72\n"
			"sloped_brick,   30373, -24\n"
			"sloped_brick,   30382,  11,      45\n"
			"sloped_brick,   30390, -45\n"
			"sloped_brick,   30499,  16\n"
			"sloped_brick,   32083,  45\n"
			"sloped_brick,   43708,  72\n"
			"sloped_brick,   43710,  72,      45\n"
			"sloped_brick,   43711,  72,      45\n"
			"sloped_brick,   47759,  40|63\n"
			"sloped_brick,   52501, -45\n"
			"sloped_brick,   60219, -45\n"
			"sloped_brick,   60477,  72\n"
			"sloped_brick,   60481,  24\n"
			"sloped_brick,   63341,  45\n"
			"sloped_brick,   72454, -45\n"
			"sloped_brick,   92946,  45\n"
			"sloped_brick,   93348,  72\n"
			"sloped_brick,   95188,  65\n"
			"sloped_brick,   99301,  63\n"
			"sloped_brick,  303923,  45\n"
			"sloped_brick,  303926,  45\n"
			"sloped_brick,  304826,  45\n"
			"sloped_brick,  329826,  64\n"
			"sloped_brick,  374726, -64\n"
			"sloped_brick,  428621,  64\n"
			"sloped_brick, 4162628,  17\n"
			"sloped_brick, 4195004,  45\n"
		};

	/*
	# LIGHTED BRICKS
	# Dictionary with part number (with extension), as key,
	# of lighted bricks, light emission colour and intensity, as values.

	#    Item---------  PartID---  Light--------------  Intensity
	*/
	const char DefaultLightedBricks[] =
		{
			"lighted_brick, 62930.dat, 1.000, 0.373, 0.059, 1.0\n"
			"lighted_brick, 54869.dat, 1.000, 0.052, 0.017, 1.0\n"
		};

	Buffer.clear();
	if (Which == PARAMS_CUSTOM_COLOURS)
		Buffer.append(DefaultCustomColours, sizeof(DefaultCustomColours));
	else if (Which == PARAMS_SLOPED_BRICKS)
		Buffer.append(DefaultSlopedBricks, sizeof(DefaultSlopedBricks));
	else if (Which == PARAMS_LIGHTED_BRICKS)
		Buffer.append(DefaultLightedBricks, sizeof(DefaultLightedBricks));
}

bool lcBlenderPreferences::ExportParameterFile()
{
	const QString BlenderConfigDir = QString("%1/Blender/addons/%2/config").arg(gAddonPreferences->mDataDir, LC_BLENDER_ADDON_RENDER_FOLDER);
	const QString ParameterFile = QString("%1/%2").arg(BlenderConfigDir, LC_BLENDER_ADDON_PARAMS_FILE);
	QDir ConfigDir(BlenderConfigDir);
	if(!ConfigDir.exists())
		ConfigDir.mkpath(".");
	QFile File(ParameterFile);

	if (!OverwriteFile(File.fileName()))
		return true;

	QString Message;
	if(File.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		int Counter = 1;
		QByteArray Buffer;
		QTextStream Stream(&File);
		Stream << "# File: " << QFileInfo(ParameterFile).fileName() << LineEnding;
		Stream << "" << LineEnding;
		Stream << "# This config file captures parameters for the Blender LDraw Render addon" << LineEnding;
		Stream << "# Parameters must be prefixed using one of the following predefined" << LineEnding;
		Stream << "# 'Item' labels:" << LineEnding;
		Stream << "# - lgeo_colour" << LineEnding;
		Stream << "# - sloped_brick" << LineEnding;
		Stream << "# - light_brick" << LineEnding;
		Stream << "" << LineEnding;
		Stream << "# All items must use a comma',' delimiter as the primary delimiter." << LineEnding;
		Stream << "# For sloped_brick items, a pipe'|' delimiter must be used to specify" << LineEnding;
		Stream << "# a range (min|max) within which the angle must lie when appropriate." << LineEnding;
		Stream << "# Spaces between item attributes are not required and are used to" << LineEnding;
		Stream << "# facilitate human readability." << LineEnding;
		Stream << "" << LineEnding;
		Stream << "" << LineEnding;
		Stream << "# LGEO CUSTOM COLOURS" << LineEnding;
		Stream << "# LGEO is a parts library for rendering LEGO using the POV-Ray" << LineEnding;
		Stream << "# rendering software. This is the list of LEGO colours suitable" << LineEnding;
		Stream << "# for realistic rendering extracted from the LGEO file 'lg_color.inc'." << LineEnding;
		Stream << "# When the 'Colour Scheme' option is set to 'Realistic', the standard" << LineEnding;
		Stream << "# LDraw colours RGB value is overwritten with the values defined here." << LineEnding;
		Stream << "# Note: You can customize these RGB values as you want." << LineEnding;
		Stream << "" << LineEnding;
		Stream << "# Item-----  ID-    R--   G--   B--" << LineEnding;

		LoadDefaultParameters(Buffer, PARAMS_CUSTOM_COLOURS);
		QTextStream colourstream(Buffer);
		for (QString Line = colourstream.readLine(); !Line.isNull(); Line = colourstream.readLine())
		{
			Stream << Line << LineEnding;
			Counter++;
		}

		Stream << "" << LineEnding;
		Stream << "# SLOPED BRICKS" << LineEnding;
		Stream << "# Dictionary with part number (without any extension for decorations), as key," << LineEnding;
		Stream << "# of pieces that have grainy slopes, and, as values, a set containing the angles (in" << LineEnding;
		Stream << "# degrees) of the face's normal to the horizontal plane. Use a | delimited tuple to" << LineEnding;
		Stream << "# represent a range within which the angle must lie." << LineEnding;
		Stream << "" << LineEnding;
		Stream << "# Item------  PartID-  Angle/Angle Range (in degrees)" << LineEnding;

		LoadDefaultParameters(Buffer, PARAMS_SLOPED_BRICKS);
		QTextStream SlopedStream(Buffer);
		for (QString Line = SlopedStream.readLine(); !Line.isNull(); Line = SlopedStream.readLine())
		{
			Stream << Line << LineEnding;
			Counter++;
		}

		Stream << "" << LineEnding;
		Stream << "# LIGHTED BRICKS" << LineEnding;
		Stream << "# Dictionary with part number (with extension), as key," << LineEnding;
		Stream << "# of lighted bricks, light emission colour and intensity, as values." << LineEnding;
		Stream << "" << LineEnding;
		Stream << "# Item-------  PartID---  Light--------------  Intensity" << LineEnding;

		LoadDefaultParameters(Buffer, PARAMS_LIGHTED_BRICKS);
		QTextStream lightedstream(Buffer);
		for (QString Line = lightedstream.readLine(); !Line.isNull(); Line = lightedstream.readLine())
		{
			Stream << Line << LineEnding;
			Counter++;
		}

		Stream << "" << LineEnding;
		Stream << "# end of parameters" << LineEnding;

		File.close();
		Message = tr("Finished writing Blender parameter entries. Processed %n lines in file '%1'.", nullptr, Counter)
					  .arg(File.fileName());
	}
	else
	{
		Message = tr("Failed to open Blender parameter file: '%1':<br>%2")
					  .arg(File.fileName(), File.errorString());
		if (gMainWindow)
			QMessageBox::warning(nullptr, tr("Export Blender Parameters"), Message);
		return false;
	}
	return true;
}

bool lcBlenderPreferences::OverwriteFile(const QString& File)
{
	QFileInfo fileInfo(File);

	if (!fileInfo.exists())
		return true;

	const QString& Title = tr ("Replace Existing File");
	const QString Header = "<b>" + QMessageBox::tr ("Existing file '%1' detected.").arg(fileInfo.fileName()) + "</b>";
	const QString Body = QMessageBox::tr ("'%1'<br>This file already exists.<br>Replace existing file?").arg(fileInfo.fileName());
	int Exec = ShowMessage(nullptr, Header, Title, Body, QString(), MBB_YES, QMessageBox::NoIcon);

	return (Exec == QMessageBox::Yes);
}

int lcBlenderPreferences::ShowMessage(QWidget* Parent, const QString& Header,  const QString& Title,  const QString& Body, const QString& Detail, int const Buttons, int const Icon)
{
	if (!gMainWindow)
		return QMessageBox::Ok;

	QMessageBox Box(Parent);
	Box.setWindowIcon(QIcon());
	if (!Icon)
	{
		QPixmap Pixmap = QPixmap(":/resources/leocad.png");
		Box.setIconPixmap (Pixmap);
	}
	else
		Box.setIcon (static_cast<QMessageBox::Icon>(Icon));

	Box.setTextFormat (Qt::RichText);
	Box.setWindowFlags (Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
	Box.setWindowTitle(Title);

	Box.setText (Header);
	if (!Body.isEmpty())
		Box.setInformativeText (Body);

	if (!Detail.isEmpty())
		Box.setDetailedText(QString(Detail).replace("<br>", "\n"));

	switch (Buttons)
	{
	case MBB_YES:
		Box.setStandardButtons (QMessageBox::Yes | QMessageBox::Cancel);
		Box.setDefaultButton   (QMessageBox::Yes);
		break;
	case MBB_YES_NO:
		Box.setStandardButtons (QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		Box.setDefaultButton   (QMessageBox::Yes);
		break;
	default:
		Box.setStandardButtons (QMessageBox::Ok);
		break;
	}

	int MinimumWidth = 400;
	int FontWidth = QFontMetrics(Box.font()).averageCharWidth();
	int FixedTextLength = (MinimumWidth / FontWidth);
	if (Header.length() < Body.length() && Header.length() < FixedTextLength)
	{
		QGridLayout* BoxLayout = (QGridLayout*)Box.layout();
		QLayoutItem* BoxLayoutItem = BoxLayout->itemAtPosition(0, 2);
		QWidget* TextWidget = BoxLayoutItem->widget();
		if (TextWidget)
		{
			qsizetype FixedWidth = Body.length() * FontWidth;
			if (FixedWidth == MinimumWidth)
			{
				int Index = (MinimumWidth / FontWidth) - 1;
				if (!Body.mid(Index, 1).isEmpty())
					FixedWidth = Body.indexOf(" ", Index);
			}
			else if (FixedWidth < MinimumWidth)
				FixedWidth = MinimumWidth;
			TextWidget->setFixedWidth(static_cast<int>(FixedWidth));
		}
	}

	const bool DownloadRequest = Body.startsWith(tr("Do you want to download version "));

	if (DownloadRequest)
	{
		QCheckBox* AddonVersionCheck = new QCheckBox(tr("Do not show download new add-on version message again."));
		Box.setCheckBox(AddonVersionCheck);
		QObject::connect(AddonVersionCheck, &QCheckBox::stateChanged, [](int State)
		{
			bool VersionCheck = true;
			if (static_cast<Qt::CheckState>(State) == Qt::CheckState::Checked)
				VersionCheck = false;
			lcSetProfileInt(LC_PROFILE_BLENDER_ADDON_VERSION_CHECK, (int)VersionCheck);
		});
	}

	return Box.exec();
}

void lcBlenderPreferences::DownloadFinished(lcHttpReply* Reply)
{
	if (!Reply->error())
		mData = Reply->readAll();
	else
		QMessageBox::warning(this, tr("Download Blender Add-on"), tr("Add-on download failed."));

	mHttpReply = nullptr;

	Reply->deleteLater();
}

namespace WindowsFileAttributes
{
enum
{
	Dir        = 0x10, // FILE_ATTRIBUTE_DIRECTORY
	File       = 0x80, // FILE_ATTRIBUTE_NORMAL
	TypeMask   = 0x90,
	ReadOnly   = 0x01, // FILE_ATTRIBUTE_READONLY
	PermMask   = 0x01
};
}

namespace UnixFileAttributes
{
enum
{
	Dir        = 0040000, // __S_IFDIR
	File       = 0100000, // __S_IFREG
	SymLink    = 0120000, // __S_IFLNK
	TypeMask   = 0170000, // __S_IFMT
	ReadUser   = 0400,    // __S_IRUSR
	WriteUser  = 0200,    // __S_IWUSR
	ExeUser    = 0100,    // __S_IXUSR
	ReadGroup  = 0040,    // __S_IRGRP
	WriteGroup = 0020,    // __S_IWGRP
	ExeGroup   = 0010,    // __S_IXGRP
	ReadOther  = 0004,    // __S_IROTH
	WriteOther = 0002,    // __S_IWOTH
	ExeOther   = 0001,    // __S_IXOTH
	PermMask   = 0777
};
}

bool lcBlenderPreferences::ExtractAddon(const QString FileName, QString& Result)
{
	enum ZipHostOS
	{
		HostFAT      = 0,
		HostUnix     = 3,
		HostHPFS     = 6,  // filesystem used by OS/2 (and NT 3.x)
		HostNTFS     = 11, // filesystem used by Windows NT
		HostVFAT     = 14, // filesystem used by Windows 95, NT
		HostOSX      = 19
	};

	struct ZipFileInfo
	{
		ZipFileInfo(lcZipFileInfo& FileInfo) noexcept
			: ZipInfo(FileInfo), isDir(false), isFile(false), isSymLink(false)
		{
		}
		bool IsValid() const noexcept { return isDir || isFile || isSymLink; }
		lcZipFileInfo& ZipInfo;
		QString filePath;
		uint isDir : 1;
		uint isFile : 1;
		uint isSymLink : 1;
		QFile::Permissions permissions;
	};

	auto ModeToPermissions = [](quint32 Mode)
	{
		QFile::Permissions Permissions;
		if (Mode&  UnixFileAttributes::ReadUser)
			Permissions |= QFile::ReadOwner | QFile::ReadUser;
		if (Mode&  UnixFileAttributes::WriteUser)
			Permissions |= QFile::WriteOwner | QFile::WriteUser;
		if (Mode&  UnixFileAttributes::ExeUser)
			Permissions |= QFile::ExeOwner | QFile::ExeUser;
		if (Mode&  UnixFileAttributes::ReadGroup)
			Permissions |= QFile::ReadGroup;
		if (Mode&  UnixFileAttributes::WriteGroup)
			Permissions |= QFile::WriteGroup;
		if (Mode&  UnixFileAttributes::ExeGroup)
			Permissions |= QFile::ExeGroup;
		if (Mode&  UnixFileAttributes::ReadOther)
			Permissions |= QFile::ReadOther;
		if (Mode&  UnixFileAttributes::WriteOther)
			Permissions |= QFile::WriteOther;
		if (Mode&  UnixFileAttributes::ExeOther)
			Permissions |= QFile::ExeOther;
		return Permissions;
	};

	lcZipFile ZipFile;

	if (!ZipFile.OpenRead(FileName))
		return false;

	const QString DestinationDir = QFileInfo(FileName).absolutePath();

	bool Ok = true;

	int Extracted = 0;

	for (quint32 FileIdx = 0; FileIdx < ZipFile.mFiles.size(); FileIdx++)
	{
		ZipFileInfo FileInfo(ZipFile.mFiles[FileIdx]);
		quint32 Mode = FileInfo.ZipInfo.external_fa;
		const ZipHostOS HostOS = ZipHostOS(FileInfo.ZipInfo.version >> 8);
		switch (HostOS)
		{
		case HostOSX:
		case HostUnix:
			Mode = (Mode >> 16)&  0xffff;
			switch (Mode&  UnixFileAttributes::TypeMask)
			{
			case UnixFileAttributes::SymLink:
				FileInfo.isSymLink = true;
				break;
			case UnixFileAttributes::Dir:
				FileInfo.isDir = true;
				break;
			case UnixFileAttributes::File:
			default:
				FileInfo.isFile = true;
				break;
			}
			FileInfo.permissions = ModeToPermissions(Mode);
			break;
		case HostFAT:
		case HostNTFS:
		case HostHPFS:
		case HostVFAT:
			switch (Mode&  WindowsFileAttributes::TypeMask)
			{
			case WindowsFileAttributes::Dir:
				FileInfo.isDir = true;
				break;
			case WindowsFileAttributes::File:
			default:
				FileInfo.isFile = true;
				break;
			}
			FileInfo.permissions |= QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther;
			if ((Mode&  WindowsFileAttributes::ReadOnly) == 0)
				FileInfo.permissions |= QFile::WriteOwner | QFile::WriteUser | QFile::WriteGroup | QFile::WriteOther;
			if (FileInfo.isDir)
				FileInfo.permissions |= QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther;
			break;
		default:
			QMessageBox::warning(this, tr("Extract Blender Add-on"), tr("ZipFile entry format (HostOS %1) at index %2 is not supported. Extract terminated.").arg(HostOS).arg(FileIdx));
			Ok = false;
		}

		if (!Ok || !(Ok = FileInfo.IsValid()))
			break;

		// Check the file path - if broken, convert separators, eat leading and trailing ones
		FileInfo.filePath = QDir::fromNativeSeparators(FileInfo.ZipInfo.file_name);
		QString FilePathRef(FileInfo.filePath);
		while (FilePathRef.startsWith(QLatin1Char('.')) || FilePathRef.startsWith(QLatin1Char('/')))
			FilePathRef = FilePathRef.mid(1);
		while (FilePathRef.endsWith(QLatin1Char('/')))
			FilePathRef.chop(1);
		FileInfo.filePath = FilePathRef;

		const QString AbsPath = QDir::fromNativeSeparators(DestinationDir + QDir::separator() + FileInfo.filePath);

		// directories
		if (FileInfo.isDir)
		{
			QDir BaseDir(DestinationDir);
			if (!(Ok = BaseDir.mkpath(FileInfo.filePath)))
				break;
			if (!(Ok = QFile::setPermissions(AbsPath, FileInfo.permissions)))
				break;
			Extracted++;
			continue;
		}

		lcMemFile MemFile;

		ZipFile.ExtractFile(FileIdx, MemFile);

		QByteArray const& ByteArray = QByteArray::fromRawData((const char*)MemFile.mBuffer, (int)MemFile.GetLength());

		// symlinks
		if (FileInfo.isSymLink)
		{
			QString Destination = QFile::decodeName(ByteArray);
			if (!(Ok = !Destination.isEmpty()))
				break;
			QFileInfo LinkFileInfo(AbsPath);
			if (!QFile::exists(LinkFileInfo.absolutePath()))
				QDir::root().mkpath(LinkFileInfo.absolutePath());
			if (!(Ok = QFile::link(Destination, AbsPath)))
				break;
			Extracted++;
			continue;
		}

		// files
		if (FileInfo.isFile)
		{
			QFile File(AbsPath);
			if(!(Ok = File.open(QIODevice::WriteOnly)))
				break;
			File.write(ByteArray);
			File.setPermissions(FileInfo.permissions);
			File.close();
			Extracted++;
		}
	}

	if (!Ok)
		Result = tr("%1 of %n files extracted.", nullptr, static_cast<int>(ZipFile.mFiles.size())).arg(Extracted);

	return Ok;
}

#endif // !LC_DISABLE_BLENDER

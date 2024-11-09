#include <QFormLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QProgressBar>
#include <QProcess>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <zlib.h>

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
#include "project.h"

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
#define LC_BLENDER_ADDON_PARAMS_FILE       "BlenderLDrawParameters.lst"

#define LC_BLENDER_ADDON_REPO_STR          "https://github.com/trevorsandy"
#define LC_BLENDER_ADDON_REPO_API_STR      "https://api.github.com/repos/trevorsandy"
#define LC_BLENDER_ADDON_STR               LC_BLENDER_ADDON_REPO_STR "/blenderldrawrender/"
#define LC_BLENDER_ADDON_API_STR           LC_BLENDER_ADDON_REPO_API_STR "/blenderldrawrender/"
#define LC_BLENDER_ADDON_LATEST_URL        LC_BLENDER_ADDON_API_STR "releases/latest"
#define LC_BLENDER_ADDON_URL               LC_BLENDER_ADDON_STR "releases/latest/download/" LC_BLENDER_ADDON_FILE
#define LC_BLENDER_ADDON_SHA_HASH_URL      LC_BLENDER_ADDON_URL ".sha256"

#define LC_THEME_DARK_PALETTE_MIDLIGHT     "#3E3E3E" //  62,  62,  62, 255
#define LC_THEME_DEFAULT_PALETTE_LIGHT     "#AEADAC" // 174, 173, 172, 255
#define LC_THEME_DARK_DECORATE_QUOTED_TEXT "#81D4FA" // 129, 212, 250, 255
#define LC_DISABLED_TEXT                   "#808080" // 128, 128, 128, 255

#define LC_RENDER_IMAGE_MAX_SIZE           32768 // pixels

static QString WhatsThisDescription = QObject::tr(
	"  Blender LDraw Addon Settings\n\n"
	"  You can configure the Blender LDraw addon settings.\n"
	"  - Blender Executable: set Blender path. On entry,\n"
	"    %1 will automatically apply the setting and\n"
	"    attempt the configure the LDraw addon.\n"
	"  - %1 Blender LDraw Addon: you can update the LDraw\n"
	"    addon which will downlod the latest addon or apply\n"
	"    the current addon if the version is the same or newer\n"
	"    than the online version.\n"
	"    You can view the standard output log for the update.\n\n"
	"  - Enabled Addon Modules: check the desired import module.\n"
	"    The LDraw Import TN import module is the long-standing\n"
	"    Blender LDraw import addon, while the LDraw Import MM\n"
	"    addon was recently introduced. The latter addon also\n"
	"    offer LDraw export functionality.\n"
	"    The %1 3D Image Render addon is mandatory but if\n"
	"    no import module is enabled, none of the modules\n"
	"    will be enabled in Blender so it will not be possible\n"
	"    to perform an LDraw model import or render.\n\n"
	"  - LDraw Import Addon Paths: addon paths are specific\n"
	"    to the enabled addon import module.\n"
	"  - LDraw Import Addon Settings: addon settings are\n"
	"    specific to the enabled addon import module.\n"
	"  - Apply: apply the addon path and setting preferences.\n"
	"  - Show/Hide Paths: show or hide the addon paths\n"
	"    display box.\n"
	"  - Reset: reset the addon path and setting preferences.\n"
	"    You can select how to reset addon settings.\n"
	"    The choice is since last apply or system default.\n\n"
	"  You can see the specific description of each setting\n"
	"  if you hover over the setting to display its tooltip.\n\n"
	"  Image Width, Image Height and Render Percentage are\n"
	"  always updated from the current step model when the\n"
	"  this dialog is opened. These settngs can be manually\n"
	"  overridden, Also, when Crop Image is checked the\n"
	"  current step cropped image width and height is\n"
	"  calculated and and used\n\n"
	"  Use the dialogue window scroll bar to access the\n"
	"  complete selection of addon settings.\n")
.arg(QLatin1String(LC_PRODUCTNAME_STR));

lcBlenderPreferences::BlenderPaths  lcBlenderPreferences::mBlenderPaths [NUM_PATHS];
lcBlenderPreferences::BlenderPaths  lcBlenderPreferences::mDefaultPaths [NUM_PATHS] =
{
	/*                             Key:                  MM Key:               Value:             Label:                                   Tooltip (Description):*/
	/* 0   PATH_BLENDER        */ {"blenderpath",        "blenderpath",        "",    QObject::tr("Blender Path"),             QObject::tr("Full file path to Blender application executable")},
	/* 1   PATH_BLENDFILE      */ {"blendfile",          "blendfile",          "",    QObject::tr("Blendfile Path"),           QObject::tr("Full file path to a supplement .blend file - specify to append additional settings")},
	/* 2   PATH_ENVIRONMENT    */ {"environmentfile",    "environmentfile",    "",    QObject::tr("Environment Texture Path"), QObject::tr("Full file path to .exr environment texture file - specify if not using default bundled in addon")},
	/* 3   PATH_LDCONFIG       */ {"customldconfigfile", "customldconfigfile", "",    QObject::tr("Custom LDConfig Path"),     QObject::tr("Full file path to custom LDConfig file - specify if not %1 alternate LDConfig file").arg(LC_PRODUCTNAME_STR)},
	/* 4   PATH_LDRAW          */ {"ldrawdirectory",     "ldrawpath",          "",    QObject::tr("LDraw Directory"),          QObject::tr("Full directory path to the LDraw parts library (download from https://library.ldraw.org)")},
	/* 5   PATH_LSYNTH         */ {"lsynthdirectory",    "",                   "",    QObject::tr("LSynth Directory"),         QObject::tr("Full directory path to LSynth primitives - specify if not using default bundled in addon")},
	/* 6   PATH_STUD_LOGO      */ {"studlogodirectory",  "",                   "",    QObject::tr("Stud Logo Directory"),      QObject::tr("Full directory path to stud logo primitives - if stud logo enabled, specify if unofficial parts not used or not using default bundled in addon")},
	/* 7   PATH_STUDIO_LDRAW   */ {"",                   "studioldrawpath",    "",    QObject::tr("Stud.io LDraw Path"),       QObject::tr("Full filepath to the Stud.io LDraw Parts Library (download from https://www.bricklink.com/v3/studio/download.page)")},
	/* 8   PATH_STUDIO_CUSTOM_PARTS */ {"",              "studiocustompartspath", "", QObject::tr("Stud.io Custom Parts Path"),QObject::tr("Full filepath to the Stud.io LDraw Custom Parts")}
};

lcBlenderPreferences::BlenderSettings  lcBlenderPreferences::mBlenderSettings [NUM_SETTINGS];
lcBlenderPreferences::BlenderSettings  lcBlenderPreferences::mDefaultSettings [NUM_SETTINGS] =
{
	/*                                   Key:                              Value:                  Label                                  Tooltip (Description)*/
	/* 0   LBL_ADD_ENVIRONMENT       */ {"addenvironment",                 "1",        QObject::tr("Add Environment"),        QObject::tr("Adds a ground plane and environment texture (affects 'Photo-realistic' look only)")},
	/* 1   LBL_ADD_GAPS              */ {"gaps",                           "0",        QObject::tr("Add Part Gap"),           QObject::tr("Add a small space between each part")},
	/* 2   LBL_BEVEL_EDGES           */ {"beveledges",                     "1",        QObject::tr("Bevel Edges"),            QObject::tr("Adds a Bevel modifier for rounding off sharp edges")},
	/* 3   LBL_BLENDFILE_TRUSTED     */ {"blendfiletrusted",               "0",        QObject::tr("Trusted Blend File"),     QObject::tr("Specify whether to treat the .blend file as being loaded from a trusted source")},
	/* 4   LBL_CROP_IMAGE            */ {"cropimage",                      "0",        QObject::tr("Crop Image"),             QObject::tr("Crop the image border at opaque content. Requires transparent background set to True")},
	/* 5   LBL_CURVED_WALLS          */ {"curvedwalls",                    "1",        QObject::tr("Curved Walls"),           QObject::tr("Makes surfaces look slightly concave, for interesting reflections")},
	/* 6   LBL_FLATTEN_HIERARCHY     */ {"flattenhierarchy",               "0",        QObject::tr("Flatten Hierarchy"),      QObject::tr("In Scene Outline, all parts are placed directly below the root - there's no tree of submodels")},
	/* 7   LBL_IMPORT_CAMERAS        */ {"importcameras",                  "1",        QObject::tr("Import Cameras"),         QObject::tr("%1 can specify camera definitions within the ldraw data. Choose to load them or ignore them.").arg(LC_PRODUCTNAME_STR)},
	/* 8   LBL_IMPORT_LIGHTS         */ {"importlights",                   "1",        QObject::tr("Import Lights"),          QObject::tr("%1 can specify point and sunlight definitions within the ldraw data. Choose to load them or ignore them.").arg(LC_PRODUCTNAME_STR)},
	/* 9   LBL_INSTANCE_STUDS        */ {"instancestuds",                  "0",        QObject::tr("Instance Studs"),         QObject::tr("Creates a Blender Object for each and every stud (WARNING: can be slow to import and edit in Blender if there are lots of studs)")},
	/*10   LBL_KEEP_ASPECT_RATIO     */ {"keepaspectratio",                "1",        QObject::tr("Keep Aspect Ratio"),      QObject::tr("Maintain the aspect ratio when resizing the output image - this attribute is not passed to Blender")},
	/*11   LBL_LINK_PARTS            */ {"linkparts",                      "1",        QObject::tr("Link Like Parts"),        QObject::tr("Identical parts (of the same type and colour) share the same mesh")},
	/*12   LBL_MINIFIG_HIERARCHY     */ {"minifighierarchy",               "1",        QObject::tr("Parent Minifigs"),        QObject::tr("Parts of minifigs are automatically parented to each other in a hierarchy")},
	/*13   LBL_NUMBER_NODES          */ {"numbernodes",                    "1",        QObject::tr("Number Objects"),         QObject::tr("Each object has a five digit prefix eg. 00001_car. This keeps the list in it's proper order")},
	/*14   LBL_OVERWRITE_IMAGE       */ {"overwriteimage",                 "1",        QObject::tr("Overwrite Image"),        QObject::tr("Specify whether to overwrite an existing rendered image file")},
	/*15   LBL_OVERWRITE_MATERIALS   */ {"overwriteexistingmaterials",     "0",        QObject::tr("Use Existing Material"),  QObject::tr("Overwrite existing material with the same name")},
	/*16   LBL_OVERWRITE_MESHES      */ {"overwriteexistingmeshes",        "0",        QObject::tr("Use Existing Mesh"),      QObject::tr("Overwrite existing mesh with the same name")},
	/*17   LBL_POSITION_CAMERA       */ {"positioncamera",                 "1",        QObject::tr("Position Camera"),        QObject::tr("Position the camera to show the whole model")},
	/*18   LBL_REMOVE_DOUBLES        */ {"removedoubles",                  "1",        QObject::tr("No Duplicate Vertices"),  QObject::tr("Remove duplicate vertices (recommended)")},
	/*19   LBL_RENDER_WINDOW         */ {"renderwindow",                   "1",        QObject::tr("Display Render Window"),  QObject::tr("Specify whether to display the render window during Blender user interface image file render")},
	/*10   LBL_USE_ARCHIVE_LIBS      */ {"usearchivelibrary",              "0",        QObject::tr("Use Archive Libraries"),  QObject::tr("Add any archive (zip) libraries in the LDraw file path to the library search list")},
	/*21   LBL_SEARCH_ADDL_PATHS     */ {"searchadditionalpaths",          "0",        QObject::tr("Search Additional Paths"),QObject::tr("Specify whether to search additional LDraw paths")},
	/*22   LBL_SMOOTH_SHADING        */ {"smoothshading",                  "1",        QObject::tr("Smooth Shading"),         QObject::tr("Smooth faces and add an edge-split modifier (recommended)")},
	/*23   LBL_TRANSPARENT_BACKGROUND*/ {"transparentbackground",          "0",        QObject::tr("Transparent Background"), QObject::tr("Specify whether to render a background (affects 'Photo-realistic look only)")},
	/*24   LBL_UNOFFICIAL_PARTS      */ {"useunofficialparts",             "1",        QObject::tr("Use Unofficial Parts"),   QObject::tr("Specify whether to use parts from the LDraw unofficial parts library path")},
	/*25   LBL_USE_LOGO_STUDS        */ {"uselogostuds",                   "1",        QObject::tr("Use Logo Studs"),         QObject::tr("Shows the LEGO logo on each stud (at the expense of some extra geometry and import time)")},
	/*26   LBL_VERBOSE               */ {"verbose",                        "1",        QObject::tr("Verbose output"),         QObject::tr("Output all messages while working, else only show warnings and errors")},

	/*27/0 LBL_BEVEL_WIDTH           */ {"bevelwidth",                     "0.5",      QObject::tr("Bevel Width"),            QObject::tr("Width of the bevelled edges")},
	/*28/1 LBL_CAMERA_BORDER_PERCENT */ {"cameraborderpercentage",         "5.0",      QObject::tr("Camera Border Percent"),  QObject::tr("When positioning the camera, include a (percentage) border leeway around the model in the rendered image")},
	/*29/2 LBL_DEFAULT_COLOUR        */ {"defaultcolour",                  "16",       QObject::tr("Default Colour"),         QObject::tr("Sets the default part colour using LDraw colour code")},
	/*20/3 LBL_GAPS_SIZE             */ {"realgapwidth",                   "0.0002",   QObject::tr("Gap Width"),              QObject::tr("Amount of space between each part (default 0.2mm)")},
	/*31/4 LBL_IMAGE_WIDTH           */ {"resolutionwidth",                "800",      QObject::tr("Image Width"),            QObject::tr("Sets the rendered image width in pixels - from current step image, label shows config setting.")},
	/*32/5 LBL_IMAGE_HEIGHT          */ {"resolutionheight",               "600",      QObject::tr("Image Height"),           QObject::tr("Sets the rendered image height in pixels - from current step image, label shows config setting.")},
	/*33/6 LBL_IMAGE_SCALE           */ {"realscale",                      "1.0",      QObject::tr("Image Scale"),            QObject::tr("Sets a scale for the model (1.0 = real life scale)")},
	/*34/6 LBL_RENDER_PERCENTAGE     */ {"renderpercentage",               "100",      QObject::tr("Render Percentage"),      QObject::tr("Sets the rendered image percentage scale for its pixel resolution - updated from current step, label shows config setting.")},

	/*35/0 LBL_COLOUR_SCHEME         */ {"usecolourscheme",                "lgeo",     QObject::tr("Colour Scheme"),          QObject::tr("Colour scheme options - Realistic (lgeo), Original (LDConfig), Alternate (LDCfgalt), Custom (User Defined)")},
	/*36/1 LBL_FLEX_PARTS_SOURCE     */ {"uselsynthparts",                 "1",        QObject::tr("Flex Parts Source"),      QObject::tr("Source used to create flexible parts - string, hoses etc. (LDCad, LSynth or both")},
	/*27/2 LBL_LOGO_STUD_VERSION     */ {"logostudversion",                "4",        QObject::tr("Logo Version"),           QObject::tr("Which version of the logo to use ('3' (flat), '4' (rounded) or '5' (subtle rounded))")},
	/*38/3 LBL_LOOK                  */ {"uselook",                        "normal",   QObject::tr("Look"),                   QObject::tr("Photo-realistic or Schematic 'Instruction' look")},
	/*39/4 LBL_POSITION_OBJECT       */ {"positionobjectongroundatorigin", "1",        QObject::tr("Position Object"),        QObject::tr("The object is centred at the origin, and on the ground plane")},
	/*40/5 LBL_RESOLUTION            */ {"resolution",                     "Standard", QObject::tr("Resolution"),             QObject::tr("Resolution of part primitives, ie. how much geometry they have")},
	/*41/6 LBL_RESOLVE_NORMALS       */ {"resolvenormals",                 "guess",    QObject::tr("Resolve Normals"),        QObject::tr("Some older LDraw parts have faces with ambiguous normals, this specifies what do do with them")}
};

lcBlenderPreferences::ComboItems  lcBlenderPreferences::mComboItems [NUM_COMBO_ITEMS] =
{
	/*    FIRST item set as default        Data                                  Item:   */
	/* 00 LBL_COLOUR_SCHEME            */ {"lgeo|ldraw|alt|custom",  QObject::tr("Realistic Colours|Original LDraw Colours|Alternate LDraw Colours|Custom Colours")},
	/* 01 LBL_FLEX_PARTS_SOURCE (t/f)  */ {"1|0|1",                  QObject::tr("LSynth|LDCad|LDCad and LSynth")},
	/* 02 LBL_LOGO_STUD_VERSION        */ {"4|3|5",                  QObject::tr("Rounded(4)|Flattened(3)|Subtle Rounded(5)")},
	/* 03 LBL_LOOK                     */ {"normal|instructions",    QObject::tr("Photo Realistic|Lego Instructions")},
	/* 04 LBL_POSITION_OBJECT (t/f)    */ {"1|0",                    QObject::tr("Centered At Origin On Ground|Centered At Origin")},
	/* 05 LBL_RESOLUTION               */ {"Standard|High|Low",      QObject::tr("Standard Primitives|High Resolution Primitives|Low Resolution Primitives")},
	/* 06 LBL_RESOLVE_NORMALS          */ {"guess|double",           QObject::tr("Recalculate Normals|Two Faces Back To Back")}
};

lcBlenderPreferences::BlenderSettings  lcBlenderPreferences::mBlenderSettingsMM [NUM_SETTINGS_MM];
lcBlenderPreferences::BlenderSettings  lcBlenderPreferences::mDefaultSettingsMM [NUM_SETTINGS_MM] =
{
	/*                                                Key:                             Value:                    Label                                    Tooltip (Description)*/
	/* 00 LBL_ADD_ENVIRONMENT_MM                  */ {"addenvironment",                "1",          QObject::tr("Add Environment"),          QObject::tr("Adds a ground plane and environment texture")},
	/* 01 LBL_BEVEL_EDGES_MM                      */ {"beveledges",                    "0",          QObject::tr("Bevel Edgest"),             QObject::tr("Bevel edges. Can cause some parts to render incorrectly")},
	/* 02 LBL_BLEND_FILE_TRUSTED_MM               */ {"blendfiletrusted",              "0",          QObject::tr("Trusted Blend File"),       QObject::tr("Specify whether to treat the .blend file as being loaded from a trusted source")},
#ifdef Q_OS_LINUX
	/* 03 LBL_CASE_SENSITIVE_FILESYSTEM           */ {"casesensitivefilesystem",       "1",          QObject::tr("Case-sensitive Filesystem"),QObject::tr("Filesystem is case sensitive. Defaults to true on Linux.")},
#else
	/* 03 LBL_CASE_SENSITIVE_FILESYSTEM           */ {"casesensitivefilesystem",       "0",          QObject::tr("Case-sensitive Filesystem"),QObject::tr("Filesystem case sensitive defaults to false Windows and MacOS. Set true if LDraw path set to case-sensitive on case-insensitive filesystem.")},
#endif
	/* 04 LBL_CROP_IMAGE_MM                       */ {"cropimage",                     "0",          QObject::tr("Crop Image"),               QObject::tr("Crop the image border at opaque content. Requires transparent background set to True")},
	/* 05 LBL_DISPLAY_LOGO                        */ {"displaylogo",                   "1",          QObject::tr("Display Logo"),             QObject::tr("Display the logo on the stud")},
	/* 06 LBL_IMPORT_CAMERAS_MM                   */ {"importcameras",                 "1",          QObject::tr("Import Cameras"),           QObject::tr("%1 can specify camera definitions within the ldraw data. Choose to load them or ignore them.").arg(LC_PRODUCTNAME_STR)},
	/* 07 LBL_IMPORT_EDGES                        */ {"importedges",                   "0",          QObject::tr("Import Edges"),             QObject::tr("Import LDraw edges as edges")},
	/* 08 LBL_IMPORT_LIGHTS_MM                    */ {"importlights",                  "1",          QObject::tr("Import Lights"),            QObject::tr("%1 can specify point and sunlight definitions within the ldraw data. Choose to load them or ignore them.").arg(LC_PRODUCTNAME_STR)},
	/* 09 LBL_KEEP_ASPECT_RATIO_MM                */ {"keepaspectratio",               "1",          QObject::tr("Keep Aspect Ratio"),        QObject::tr("Maintain the aspect ratio when resizing the output image - this attribute is not passed to Blender")},
	/* 10 LBL_MAKE_GAPS                           */ {"makegaps",                      "1",          QObject::tr("Make Gaps"),                QObject::tr("Make small gaps between bricks. A small gap is more realistic")},
	/* 11 LBL_META_BFC                            */ {"metabfc",                       "1",          QObject::tr("BFC"),                      QObject::tr("Process LDraw Back Face Culling meta commands")},
	/* 12 LBL_META_CLEAR                          */ {"metaclear",                     "0",          QObject::tr("CLEAR Command"),            QObject::tr("Hides all parts in the timeline up to where this command is encountered")},
	/* 13 LBL_META_GROUP                          */ {"metagroup",                     "1",          QObject::tr("GROUP Command"),            QObject::tr("Process GROUP meta commands")},
	/* 14 LBL_META_PAUSE                          */ {"metapause",                     "0",          QObject::tr("PAUSE Command"),            QObject::tr("Not implemented")},
	/* 15 LBL_META_PRINT_WRITE                    */ {"metaprintwrite",                "0",          QObject::tr("PRINT/WRITE Command"),      QObject::tr("Prints PRINT/WRITE META commands to the system console.")},
	/* 16 LBL_META_SAVE                           */ {"metasave",                      "0",          QObject::tr("SAVE Command"),             QObject::tr("Not implemented")},
	/* 17 LBL_META_STEP                           */ {"metastep",                      "0",          QObject::tr("STEP Command"),             QObject::tr("Adds a keyframe that shows the part at the moment in the timeline")},
	/* 18 LBL_META_STEP_GROUPS                    */ {"metastepgroups",                "0",          QObject::tr("STEP Groups"),              QObject::tr("Create collection for individual steps")},
	/* 19 LBL_META_TEXMAP                         */ {"metatexmap",                    "1",          QObject::tr("TEXMAP and DATA Command"),  QObject::tr("Process TEXMAP and DATA meta commands")},
	/* 20 LBL_NO_STUDS                            */ {"nostuds",                       "0",          QObject::tr("No Studs"),                 QObject::tr("Don't import studs")},
	/* 21 LBL_OVERWRITE_IMAGE_MM                  */ {"overwriteimage",                "1",          QObject::tr("Overwrite Image"),          QObject::tr("Specify whether to overwrite an existing rendered image file")},
	/* 22 LBL_POSITION_CAMERA_MM                  */ {"positioncamera",                "1",          QObject::tr("Position Camera"),          QObject::tr("Position the camera to show the whole model")},
	/* 23 LBL_PARENT_TO_EMPTY                     */ {"parenttoempty",                 "1",          QObject::tr("Parent To Empty"),          QObject::tr("Parent the model to an empty")},
	/* 24 LBL_PREFER_STUDIO                       */ {"preferstudio",                  "0",          QObject::tr("Prefer Stud.io Library"),   QObject::tr("Search for parts in Stud.io library first")},
	/* 25 LBL_PREFER_UNOFFICIAL                   */ {"preferunofficial",              "0",          QObject::tr("Prefer Unofficial Parts"),  QObject::tr("Search for unofficial parts first")},
	/* 26 LBL_PROFILE                             */ {"profile",                       "0",          QObject::tr("Profile"),                  QObject::tr("Profile import performance")},
	/* 27 LBL_RECALCULATE_NORMALS                 */ {"recalculatenormals",            "0",          QObject::tr("Recalculate Normals"),      QObject::tr("Recalculate normals. Not recommended if BFC processing is active")},
	/* 28 LBL_REMOVE_DOUBLES_MM                   */ {"removedoubles",                 "0",          QObject::tr("No Duplicate Vertices"),    QObject::tr("Merge vertices that are within a certain distance.")},
	/* 29 LBL_RENDER_WINDOW_MM                    */ {"renderwindow",                  "1",          QObject::tr("Display Render Window"),    QObject::tr("Specify whether to display the render window during Blender user interface image file render")},
	/* 30 LBL_SEARCH_ADDL_PATHS_MM                */ {"searchadditionalpaths",         "0",          QObject::tr("Search Additional Paths"),  QObject::tr("Specify whether to search additional LDraw paths")},
	/* 31 LBL_SETEND_FRAME                        */ {"setendframe",                   "1",          QObject::tr("Set Step End Frame"),       QObject::tr("Set the end frame to the last step")},
	/* 32 LBL_SET_TIMELINE_MARKERS                */ {"settimelinemarkers",            "0",          QObject::tr("Set Timeline Markers"),     QObject::tr("Set timeline markers for meta commands")},
	/* 33 LBL_SHADE_SMOOTH                        */ {"shadesmooth",                   "1",          QObject::tr("Shade Smooth"),             QObject::tr("Use flat or smooth shading for part faces")},
	/* 34 LBL_TRANSPARENT_BACKGROUND_MM           */ {"transparentbackground",         "0",          QObject::tr("Transparent Background"),   QObject::tr("Specify whether to render a background")},
	/* 35 LBL_TREAT_SHORTCUT_AS_MODEL             */ {"treatshortcutasmodel",          "0",          QObject::tr("Treat Shortcuts As Models"),QObject::tr("Split shortcut parts into their constituent pieces as if they were models")},
	/* 36 LBL_TRIANGULATE                         */ {"triangulate",                   "0",          QObject::tr("Triangulate Faces"),        QObject::tr("Triangulate all faces")},
	/* 37 LBL_USE_ARCHIVE_LIBRARY_MM              */ {"usearchivelibrary",             "0",          QObject::tr("Use Archive Libraries"),    QObject::tr("Add any archive (zip) libraries in the LDraw file path to the library search list")},
	/* 38 LBL_USE_FREESTYLE_EDGES                 */ {"usefreestyleedges",             "0",          QObject::tr("Use Freestyle Edges"),      QObject::tr("Render LDraw edges using freestyle")},
	/* 39 LBL_VERBOSE_MM                          */ {"verbose",                       "1",          QObject::tr("Verbose output"),           QObject::tr("Output all messages while working, else only show warnings and errors")},

	/* 40/00 LBL_BEVEL_SEGMENTS                   */ {"bevelsegments",                 "4",          QObject::tr("Bevel Segments"),           QObject::tr("Bevel segments")},
	/* 41/01 LBL_BEVEL_WEIGHT                     */ {"bevelweight",                   "0.3",        QObject::tr("Bevel Weight"),             QObject::tr("Bevel weight")},
	/* 42/02 LBL_BEVEL_WIDTH_MM                   */ {"bevelwidth",                    "0.3",        QObject::tr("Bevel Width"),              QObject::tr("Width of the bevelled edges")},
	/* 43/03 LBL_CAMERA_BORDER_PERCENT_MM         */ {"cameraborderpercent",           "5",          QObject::tr("Camera Border Percent"),    QObject::tr("When positioning the camera, include a (percentage) border around the model in the render")},
	/* 44/04 LBL_FRAMES_PER_STEP                  */ {"framesperstep",                 "3",          QObject::tr("Frames Per Step"),          QObject::tr("Frames per step")},
	/* 45/05 LBL_GAP_SCALE                        */ {"gapscale",                      "0.997",      QObject::tr("Gap Scale"),                QObject::tr("Scale individual parts by this much to create the gap")},
	/* 46/06 LBL_IMPORT_SCALE                     */ {"importscale",                   "0.02",       QObject::tr("Import Scale"),             QObject::tr("What scale to import at. Full scale is 1.0 and is so huge that it is unwieldy in the viewport")},
	/* 47/07 LBL_MERGE_DISTANCE                   */ {"mergedistance",                 "0.05",       QObject::tr("Merge Distance"),           QObject::tr("Maximum distance between elements to merge")},
	/* 48/08 LBL_RENDER_PERCENTAGE_MM             */ {"renderpercentage",              "100",        QObject::tr("Render Percentage"),        QObject::tr("Sets the rendered image percentage scale for its pixel resolution - updated from current step, label shows config setting.")},
	/* 49/09 LBL_RESOLUTION_WIDTH                 */ {"resolutionwidth",               "800",        QObject::tr("Image Width"),              QObject::tr("Sets the rendered image width in pixels - from current step image, label shows config setting.")},
	/* 50/10 LBL_RESOLUTION_HEIGHT                */ {"resolutionheight",              "600",        QObject::tr("Image Height"),             QObject::tr("Sets the rendered image height in pixels - from current step image, label shows config setting.")},
	/* 51/11 LBL_STARTING_STEP_FRAME              */ {"startingstepframe",             "1",          QObject::tr("Starting Step Frame"),      QObject::tr("Frame to add the first STEP meta command")},

	/* 52/00 LBL_CHOSEN_LOGO                      */ {"chosenlogo",                    "logo3",      QObject::tr("Chosen Logo"),              QObject::tr("Which logo to display. logo and logo2 aren't used and are only included for completeness")},
	/* 53/01 LBL_COLOUR_SCHEME_MM                 */ {"usecolourscheme",               "lgeo",       QObject::tr("Colour Scheme"),            QObject::tr("Colour scheme options - Realistic (lgeo), Original (LDConfig), Alternate (LDCfgalt), Custom (User Defined)")},
	/* 54/02 LBL_RESOLUTION_MM                    */ {"resolution",                    "Standard",   QObject::tr("Resolution"),               QObject::tr("Resolution of part primitives, ie. how much geometry they have")},
	/* 55/03 LBL_SCALE_STRATEGY                   */ {"scalestrategy",                 "mesh",       QObject::tr("How To Scale Parts"),       QObject::tr("Apply import scaling to mesh - Recommended for rendering, Apply import scaling to object - Recommended for part editing")},
	/* 56/04 LBL_SMOOTH_TYPE                      */ {"smoothtype",                    "edge_split", QObject::tr("Smooth Type"),              QObject::tr("Use either autosmooth or an edge split modifier to smooth part faces")}
};

lcBlenderPreferences::ComboItems  lcBlenderPreferences::mComboItemsMM [NUM_COMBO_ITEMS_MM] =
{
	/*    FIRST item set as default        Data                                  Item: */
	/* 00 LBL_CHOSEN_LOGO              */ {"logo3|logo4|logo5",                  QObject::tr("Raised flattened logo geometry(3)|Raised rounded logo geometry(4)|Subtle rounded logo geometry(5)")},
	/* 01 LBL_COLOUR_SCHEME_MM         */ {"lgeo|ldraw|alt|custom",              QObject::tr("Realistic Colours|Original LDraw Colours|Alternate LDraw Colours|Custom Colours")},
	/* 02 LBL_RESOLUTION_MM            */ {"Low|Standard|High",                  QObject::tr("Low Resolution Primitives|Standard Primitives|High Resolution Primitives")},
	/* 03 LBL_SCALE_STRATEGY           */ {"mesh|object",                        QObject::tr("Scale Mesh|Scale Object")},
	/* 04 LBL_SMOOTH_TYPE              */ {"edge_split|auto_smooth|bmesh_split", QObject::tr("Smooth part faces with edge split modifier|Auto-smooth part faces|Split during initial mesh processing")}
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
	setWindowTitle(tr("Blender LDraw Addon Settings"));

	QVBoxLayout* Layout = new QVBoxLayout(this);
	setLayout(Layout);

	QGroupBox* Box = new QGroupBox(this);
	Layout->addWidget(Box);

	mPreferences = new lcBlenderPreferences(Width,Height,Scale,Box);

	QDialogButtonBox* ButtonBox;
	ButtonBox = new QDialogButtonBox(this);
	mApplyButton = new QPushButton(tr("Apply"), ButtonBox);
	mApplyButton->setToolTip(tr("Apply addon paths and settings preferences"));
	mApplyButton->setEnabled(false);
	ButtonBox->addButton(mApplyButton, QDialogButtonBox::ActionRole);
	connect(mApplyButton,SIGNAL(clicked()), this, SLOT(accept()));

	mPathsButton = new QPushButton(tr("Hide Paths"), ButtonBox);
	mPathsButton->setToolTip(tr("Hide addon path preferences dialog"));
	ButtonBox->addButton(mPathsButton,QDialogButtonBox::ActionRole);
	connect(mPathsButton,SIGNAL(clicked()), this, SLOT(ShowPathsGroup()));

	mResetButton = new QPushButton(tr("Reset"), ButtonBox);
	mResetButton->setEnabled(false);
	mResetButton->setToolTip(tr("Reset addon paths and settings preferences"));
	ButtonBox->addButton(mResetButton,QDialogButtonBox::ActionRole);
	connect(mResetButton,SIGNAL(clicked()), this, SLOT(ResetSettings()));

	ButtonBox->addButton(QDialogButtonBox::Cancel);
	connect(ButtonBox,SIGNAL(rejected()), this, SLOT(reject()));

	if (!QFileInfo(lcGetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH)).isReadable() && !lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty())
		mApplyButton->setEnabled(true);

	connect(mPreferences,SIGNAL(SettingChangedSig(bool)), this, SLOT(EnableButton(bool)));

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
	const QString Display = mPathsButton->text().startsWith("Hide") ? tr("Show") : tr("Hide");
	mPathsButton->setText(tr("%1 Paths").arg(Display));
	mPathsButton->setToolTip(tr("%1 addon path preferences dialog").arg(Display));
	mPreferences->ShowPathsGroup();
}

void lcBlenderPreferencesDialog::EnableButton(bool Change)
{
	mApplyButton->setEnabled(Change);
	mResetButton->setEnabled(Change);
	mPathsButton->setText(tr("Hide Paths"));
	mPathsButton->setToolTip(tr("Hide addon path preferences dialog"));
}

void lcBlenderPreferencesDialog::ResetSettings()
{
	mPreferences->ResetSettings();
	mApplyButton->setEnabled(false);
}

void lcBlenderPreferencesDialog::accept()
{
	if (mPreferences->SettingsModified())
	{
		mApplyButton->setEnabled(false);
		mPreferences->SaveSettings();
		QDialog::accept();
	} else
		QDialog::reject();
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
		Parent->setWhatsThis(WhatsThisDescription);
	}
	else
	{
		setWindowTitle(tr("Blender LDraw Addon Settings"));
		setLayout(Layout);
		setWhatsThis(WhatsThisDescription);
	}

	mContent = new QWidget();

	mForm = new QFormLayout(mContent);

	mContent->setLayout(mForm);

	QPalette ReadOnlyPalette = QApplication::palette();
	const lcPreferences& Preferences = lcGetPreferences();
	if (Preferences.mColorTheme == lcColorTheme::Dark)
		ReadOnlyPalette.setColor(QPalette::Base,QColor(LC_THEME_DARK_PALETTE_MIDLIGHT));
	else
		ReadOnlyPalette.setColor(QPalette::Base,QColor(LC_THEME_DEFAULT_PALETTE_LIGHT));
	ReadOnlyPalette.setColor(QPalette::Text,QColor(LC_DISABLED_TEXT));

	QGroupBox* BlenderExeBox = new QGroupBox(tr("Blender Executable"),mContent);
	mForm->addRow(BlenderExeBox);

	mExeGridLayout = new QGridLayout(BlenderExeBox);
	BlenderExeBox->setLayout(mExeGridLayout);

	mBlenderVersionLabel = new QLabel(BlenderExeBox);
	mExeGridLayout->addWidget(mBlenderVersionLabel,0,0);

	mBlenderVersionEdit = new QLineEdit(BlenderExeBox);
	mBlenderVersionEdit->setPalette(ReadOnlyPalette);
	mBlenderVersionEdit->setReadOnly(true);
	mExeGridLayout->addWidget(mBlenderVersionEdit,0,1,1,2);

	QLabel* PathLabel = new QLabel(BlenderExeBox);
	mExeGridLayout->addWidget(PathLabel,1,0);

	QLineEdit* PathLineEdit = new QLineEdit(BlenderExeBox);
	mExeGridLayout->addWidget(PathLineEdit,1,1);
	mPathLineEditList << PathLineEdit;
	connect(PathLineEdit, SIGNAL(editingFinished()), this, SLOT(ConfigureBlenderAddon()));

	QPushButton* PathBrowseButton = new QPushButton(tr("Browse..."), BlenderExeBox);
	mExeGridLayout->addWidget(PathBrowseButton,1,2);
	mPathBrowseButtonList << PathBrowseButton;
	connect(PathBrowseButton, SIGNAL(clicked(bool)), this, SLOT(BrowseBlender(bool)));

	QGroupBox* BlenderAddonVersionBox = new QGroupBox(tr("%1 Blender LDraw Addon").arg(LC_PRODUCTNAME_STR),mContent);
	mForm->addRow(BlenderAddonVersionBox);

	mAddonGridLayout = new QGridLayout(BlenderAddonVersionBox);
	BlenderAddonVersionBox->setLayout(mAddonGridLayout);

	QCheckBox* AddonVersionCheck = new QCheckBox(tr("Prompt to download new addon version when available"), BlenderAddonVersionBox);
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
	mAddonVersionEdit->setToolTip(tr("%1 Blender LDraw import and image renderer addon").arg(LC_PRODUCTNAME_STR));
	mAddonVersionEdit->setPalette(ReadOnlyPalette);
	mAddonVersionEdit->setReadOnly(true);
	mAddonGridLayout->addWidget(mAddonVersionEdit,1,1);
	mAddonGridLayout->setColumnStretch(1,1/*1 is greater than 0 (default)*/);

	mAddonUpdateButton = new QPushButton(tr("Update"), BlenderAddonVersionBox);
	mAddonUpdateButton->setToolTip(tr("Update %1 Blender LDraw addon").arg(LC_PRODUCTNAME_STR));
	mAddonGridLayout->addWidget(mAddonUpdateButton,1,2);
	connect(mAddonUpdateButton, SIGNAL(clicked(bool)), this, SLOT(UpdateBlenderAddon()));

	mAddonStdOutButton = new QPushButton(tr("Output..."), BlenderAddonVersionBox);
	mAddonStdOutButton->setToolTip(tr("Open the standrd output log"));
	mAddonStdOutButton->setEnabled(false);
	mAddonGridLayout->addWidget(mAddonStdOutButton,1,3);
	connect(mAddonStdOutButton, SIGNAL(clicked(bool)), this, SLOT(GetStandardOutput()));

	mModulesBox = new QGroupBox(tr("Enabled Addon Modules"),mContent);
	QHBoxLayout* ModulesLayout = new QHBoxLayout(mModulesBox);
	mModulesBox->setLayout(ModulesLayout);
	mAddonGridLayout->addWidget(mModulesBox,2,0,1,4);

	mImportActBox = new QCheckBox(tr("LDraw Import TN"),mModulesBox);
	mImportActBox->setToolTip(tr("Enable addon import module (adapted from LDraw Import by Toby Nelson) in Blender"));
	ModulesLayout->addWidget(mImportActBox);
	connect(mImportActBox, SIGNAL(clicked(bool)), this, SLOT(EnableImportModule()));

	mImportMMActBox = new QCheckBox(tr("LDraw Import MM"),mModulesBox);
	mImportMMActBox->setToolTip(tr("Enable addon import module (adapted from LDraw Import by Matthew Morrison) in Blender"));
	ModulesLayout->addWidget(mImportMMActBox);
	connect(mImportMMActBox, SIGNAL(clicked(bool)), this, SLOT(EnableImportModule()));

	mRenderActBox = new QCheckBox(tr("%1 Image Render").arg(LC_PRODUCTNAME_STR), mModulesBox);
	mRenderActBox->setToolTip(tr("Addon image render module in Blender"));
	mRenderActBox->setEnabled(false);
	ModulesLayout->addWidget(mRenderActBox);

	LoadSettings();

	mConfigured = !lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty();

	const int CtlIdx = PATH_BLENDER;
	PathLabel->setText(mBlenderPaths[CtlIdx].label);
	PathLabel->setToolTip(mBlenderPaths[CtlIdx].tooltip);

	PathLineEdit->setText(mBlenderPaths[CtlIdx].value);
	PathLineEdit->setToolTip(mBlenderPaths[CtlIdx].tooltip);

	if (mAddonVersion.isEmpty())
	{
		mModulesBox->setEnabled(false);
		mAddonUpdateButton->setEnabled(false);
		mImportActBox->setChecked(true); // default addon module
	}
	else
	{
		mAddonVersionEdit->setText(mAddonVersion);
		mRenderActBox->setChecked(true);
		mImportActBox->setChecked(  lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE) == QLatin1String("TN"));
		mImportMMActBox->setChecked(lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE) == QLatin1String("MM"));
	}

	QString VersionTextColour = QApplication::palette().text().color().name(),AddonTextColour;
	QString VersionText = tr("Blender"), AddonText = tr("Blender Addon");
	if (mConfigured)
	{
		AddonTextColour = VersionTextColour;
		mBlenderVersionEdit->setText(mBlenderVersion);
		mBlenderVersionEdit->setVisible(true);
	}
	else
	{
		const QString TextColour = Preferences.mColorTheme == lcColorTheme::Dark ? QLatin1String(LC_THEME_DARK_DECORATE_QUOTED_TEXT) : QLatin1String("blue");

		bool BlenderConfigured = !lcGetProfileString(LC_PROFILE_BLENDER_PATH).isEmpty();
		if (!BlenderConfigured)
		{
			VersionText = tr("Blender not configured.");
			VersionTextColour = TextColour;
		}
		else
			mBlenderVersionEdit->setText(mBlenderVersion);

		if (QFileInfo(QString("%1/Blender/%2").arg(mDataDir).arg(LC_BLENDER_ADDON_FILE)).isReadable())
		{
			mModulesBox->setEnabled(false);
			mImportActBox->setChecked(true);
			mAddonUpdateButton->setEnabled(true);
			if (BlenderConfigured)
				AddonText = tr("Addon not configured - Update.");
			else
				AddonText = tr("Addon not configured.");
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

	QLayoutItem* GroupBoxIitem = mForm->itemAt ( Row, ItemRole );

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

void lcBlenderPreferences::InitPathsAndSettings()
{
	if (!NumSettings())
		LoadSettings();

	// Paths
	ClearGroupBox(mPathsBox);
	mPathsBox = new QGroupBox(mContent);
	mPathsBox->setTitle(tr("LDraw Import TN Addon Paths"));
	mPathsGridLayout = new QGridLayout(mPathsBox);
	mPathsBox->setLayout(mPathsGridLayout);
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
		if (mPathLineEditList.size() > CtlIdx)
			mPathLineEditList.replace(CtlIdx, PathLineEdit);
		else
			mPathLineEditList << PathLineEdit;
		mPathsGridLayout->addWidget(PathLineEdit,ColumnIdx,1);
		PathLineEdit->setVisible(IsVisible);

		QPushButton* PathBrowseButton = new QPushButton(tr("Browse..."), mPathsBox);
		if (mPathBrowseButtonList.size() > CtlIdx)
			mPathBrowseButtonList.replace(CtlIdx, PathBrowseButton);
		else
			mPathBrowseButtonList << PathBrowseButton;
		mPathsGridLayout->addWidget(PathBrowseButton,ColumnIdx,2);
		PathBrowseButton->setVisible(IsVisible);

		if (IsVisible)
		{
			connect(PathBrowseButton, SIGNAL(clicked(bool)), this, SLOT (BrowseBlender(bool)));
			connect(PathLineEdit, SIGNAL(editingFinished()), this, SLOT (PathChanged()));
		}
	}

	mPathsBox->setEnabled(mConfigured);

	// Settings
	ClearGroupBox(mSettingsBox);
	mSettingsBox = new QGroupBox(mContent);
	mSettingsSubform = new QFormLayout(mSettingsBox);
	mSettingsBox->setLayout(mSettingsSubform);
	mForm->addRow(mSettingsBox);

	mSettingsBox->setTitle(tr("LDraw Import TN Addon Settings"));
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
			if (LblIdx == LBL_CROP_IMAGE)
				connect(CheckBox, SIGNAL(toggled(bool)), this, SLOT (SetModelSize(bool)));
			else
				connect(CheckBox, SIGNAL(clicked()), this, SLOT (SettingChanged()));
			mCheckBoxList << CheckBox;
			mSettingsSubform->addRow(Label,CheckBox);
		}
		else if (LblIdx < LBL_COLOUR_SCHEME)
		{   // QLineEdits
			QLineEdit* LineEdit = new QLineEdit(mSettingsBox);
			LineEdit->setProperty("ControlID",QVariant(LblIdx));
			if (LblIdx == LBL_IMAGE_WIDTH || LblIdx == LBL_IMAGE_HEIGHT)
			{
				connect(LineEdit, SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
				LineEdit->setValidator(new QIntValidator(16, LC_RENDER_IMAGE_MAX_SIZE));
			}
			else if(LblIdx == LBL_DEFAULT_COLOUR)
			{
				LineEdit->setReadOnly(true);
				LineEdit->setStyleSheet("Text-align:left");
				mDefaultColourEditAction = LineEdit->addAction(QIcon(), QLineEdit::TrailingPosition);
				connect(mDefaultColourEditAction, SIGNAL(triggered(bool)), this, SLOT (ColorButtonClicked(bool)));
			}
			else
			{
				LineEdit->setText(mBlenderSettings[LblIdx].value);
				if (LblIdx == LBL_IMAGE_SCALE)
					LineEdit->setValidator(new QDoubleValidator(0.01,10.0,2));
				else if (LblIdx == LBL_RENDER_PERCENTAGE || LblIdx == LBL_CAMERA_BORDER_PERCENT)
					LineEdit->setValidator(new QIntValidator(1,1000));
				else
					LineEdit->setValidator(new QDoubleValidator(0.01,100.0,2));
				connect(LineEdit, SIGNAL(textEdited(const QString&)), this, SLOT (SettingChanged(const QString&)));
			}
			LineEdit->setToolTip(mBlenderSettings[LblIdx].tooltip);
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
			QStringList const ItemList = mComboItems[ComboBoxItemsIndex].itemList.split("|");
			ComboBox->addItems(ItemList);
			for (int CtlIdx = 0; CtlIdx < ComboBox->count(); CtlIdx++)
				ComboBox->setItemData(CtlIdx, DataList.at(CtlIdx));
			ComboBox->setToolTip(mBlenderSettings[LblIdx].tooltip);
			int CurrentIndex = int(ComboBox->findData(QVariant::fromValue(Value)));
			ComboBox->setCurrentIndex(CurrentIndex);
			if (LblIdx == LBL_COLOUR_SCHEME)
				connect(ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT (ValidateColourScheme(int)));
			else
				connect(ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT (SettingChanged(int)));
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
	mPathsBox->setTitle(tr("LDraw Import MM Addon Paths"));
	mPathsGridLayout = new QGridLayout(mPathsBox);
	mPathsBox->setLayout(mPathsGridLayout);
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
		if (mPathLineEditList.size() > CtlIdx)
			mPathLineEditList.replace(CtlIdx, PathLineEdit);
		else
			mPathLineEditList << PathLineEdit;
		mPathsGridLayout->addWidget(PathLineEdit,ColumnIdx,1);
		PathLineEdit->setVisible(IsVisible);

		QPushButton* PathBrowseButton = new QPushButton(tr("Browse..."), mPathsBox);
		if (mPathBrowseButtonList.size() > CtlIdx)
			mPathBrowseButtonList.replace(CtlIdx, PathBrowseButton);
		else
			mPathBrowseButtonList << PathBrowseButton;
		mPathsGridLayout->addWidget(PathBrowseButton,ColumnIdx,2);
		PathBrowseButton->setVisible(IsVisible);

		if (IsVisible)
		{
			connect(PathBrowseButton, SIGNAL(clicked(bool)), this, SLOT (BrowseBlender(bool)));
			connect(PathLineEdit, SIGNAL(editingFinished()), this, SLOT (PathChanged()));
		}
	}

	mPathsBox->setEnabled(mConfigured);

	// Settings
	ClearGroupBox(mSettingsBox);
	mSettingsBox = new QGroupBox(mContent);
	mSettingsSubform = new QFormLayout(mSettingsBox);
	mSettingsBox->setLayout(mSettingsSubform);
	mForm->addRow(mSettingsBox);

	mSettingsBox->setTitle(tr("LDraw Import MM Addon Settings"));
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
			if (LblIdx == LBL_CROP_IMAGE_MM)
				connect(CheckBox, SIGNAL(toggled(bool)), this, SLOT (SetModelSize(bool)));
			else
				connect(CheckBox, SIGNAL(clicked()), this, SLOT (SettingChanged()));
			mCheckBoxList << CheckBox;
			mSettingsSubform->addRow(Label,CheckBox);
		}
		else if (LblIdx < LBL_CHOSEN_LOGO)
		{   // QLineEdits
			QLineEdit* LineEdit = new QLineEdit(mSettingsBox);
			LineEdit->setProperty("ControlID",QVariant(LblIdx));
			if (LblIdx == LBL_RESOLUTION_WIDTH || LblIdx == LBL_RESOLUTION_HEIGHT)
			{
				connect(LineEdit, SIGNAL(textChanged(const QString&)), this, SLOT  (SizeChanged(const QString&)));
				LineEdit->setValidator(new QIntValidator(16, LC_RENDER_IMAGE_MAX_SIZE));
			}
			else
			{
				LineEdit->setText(mBlenderSettingsMM[LblIdx].value);
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
				connect(LineEdit, SIGNAL(textEdited(const QString&)), this, SLOT (SettingChanged(const QString&)));
			}
			LineEdit->setToolTip(mBlenderSettingsMM[LblIdx].tooltip);
			mLineEditList << LineEdit;
			mSettingsSubform->addRow(Label,LineEdit);
		}
		else
		{   // QComboBoxes
			QComboBox* ComboBox = new QComboBox(mSettingsBox);
			ComboBox->setProperty("ControlID",QVariant(LblIdx));
			const QString Value = mBlenderSettingsMM[LblIdx].value;
			QStringList const DataList = mComboItemsMM[ComboBoxItemsIndex].dataList.split("|");
			QStringList const ItemList = mComboItemsMM[ComboBoxItemsIndex].itemList.split("|");
			ComboBox->addItems(ItemList);
			for (int CtlIdx = 0; CtlIdx < ComboBox->count(); CtlIdx++)
				ComboBox->setItemData(CtlIdx, DataList.at(CtlIdx));
			ComboBox->setToolTip(mBlenderSettingsMM[LblIdx].tooltip);
			int CurrentIndex = int(ComboBox->findData(QVariant::fromValue(Value)));
			ComboBox->setCurrentIndex(CurrentIndex);
			if (LblIdx == LBL_COLOUR_SCHEME_MM)
				connect(ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT (ValidateColourScheme(int)));
			else
				connect(ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT  (SettingChanged(int)));
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

	disconnect(mPathLineEditList[PATH_BLENDER], SIGNAL(editingFinished()), this, SLOT (ConfigureBlenderAddon()));

	ConfigureBlenderAddon(sender() == mPathBrowseButtonList[PATH_BLENDER],
						  sender() == mAddonUpdateButton);

	connect(mPathLineEditList[PATH_BLENDER], SIGNAL(editingFinished()), this, SLOT (ConfigureBlenderAddon()));
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
		const QString BlenderInstallFile = QDir::toNativeSeparators(QString("%1/%2").arg(BlenderDir).arg(LC_BLENDER_ADDON_INSTALL_FILE));
		const QString BlenderTestString = QLatin1String("###TEST_BLENDER###");
		QByteArray AddonPathsAndModuleNames;
		QString Message, ShellProgram;
		QStringList Arguments;
		ProcEnc Result = PR_OK;
		QFile Script;

		bool NewBlenderExe = BlenderExeCompare != BlenderExe.toLower();

		if (mConfigured && !AddonUpdate && !ModuleChange && !NewBlenderExe)
			return;

		auto ProcessCommand = [&](ProcEnc Action)
		{
			mProcess = new QProcess();

			QString ProcessAction = tr("addon install");
			if (Action == PR_INSTALL)
			{
				connect(mProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(ReadStdOut()));
				const QString& LdrawLibPath = QFileInfo(lcGetProfileString(LC_PROFILE_PARTS_LIBRARY)).absolutePath();
				QStringList SystemEnvironment = QProcess::systemEnvironment();
				SystemEnvironment.prepend("LDRAW_DIRECTORY=" + LdrawLibPath);
				SystemEnvironment.prepend("ADDONS_TO_LOAD=" + AddonPathsAndModuleNames);
				mProcess->setEnvironment(SystemEnvironment);
			}
			else
			{
				ProcessAction = tr("test");
				disconnect(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(Update()));
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
				mProcess->start(ShellProgram, QStringList() << "/C" << Script.fileName());
#else
				mProcess->start(ShellProgram, QStringList() << Script.fileName());
#endif
			}

			if (!mProcess->waitForStarted())
			{
				Message = tr("Cannot start Blender %1 mProcess.\n%2").arg(ProcessAction).arg(QString(mProcess->readAllStandardError()));
				delete mProcess;
				mProcess = nullptr;
				return PR_WAIT;
			}
			else
			{
				if (mProcess->exitStatus() != QProcess::NormalExit || mProcess->exitCode() != 0)
				{
					Message = tr("Failed to execute Blender %1.\n%2").arg(ProcessAction).arg(QString(mProcess->readAllStandardError()));
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
				connect(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(Update()));
				const QString StdOut = QString(mProcess->readAllStandardOutput());
				if (!StdOut.contains(BlenderTestString))
				{
					Message =  tr("A simple check to test if the selected file is Blender failed."
								 "Please create an %1 GitHub ticket if you are sure the file is Blender 2.8 or newer."
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
							Message = tr("Blender %1 mProcess completed. Version: %2 validated.").arg(ProcessAction).arg(mBlenderVersion);
						}
					}
				}
			}

			return PR_OK;
		};

		connect(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(Update()));
		mUpdateTimer.start(500);

		if (!ModuleChange)
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
				ScriptCommand = QString("%1 %2").arg(BlenderExe).arg(Arguments.join(" "));

				Script.setFileName(QString("%1/%2").arg(QDir::tempPath()).arg(ScriptName));
				if(Script.open(QIODevice::WriteOnly | QIODevice::Text))
				{
					QTextStream Stream(&Script);
#ifdef Q_OS_WIN
					Stream << QLatin1String("@ECHO OFF& SETLOCAL") << LineEnding;
#else
					Stream << QLatin1String("#!/bin/bash") << LineEnding;
#endif
					Stream << ScriptCommand << LineEnding;
					Script.close();
				}
				else
				{
					Message = tr("Cannot write Blender render script file [%1] %2.").arg(Script.fileName()).arg(Script.errorString());
					Error = true;
				}

				if (Error)
				{
					StatusUpdate(false);
					return;
				}

				QThread::sleep(1);

#ifdef Q_OS_WIN
				ShellProgram = QLatin1String(LC_WINDOWS_SHELL);
#else
				ShellProgram = QLatin1String(LC_UNIX_SHELL);
#endif
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
					const QString& Title = tr ("%1 Blender LDraw Addon").arg(LC_PRODUCTNAME_STR);
					const QString& Header = tr ("Blender test failed.");
					ShowMessage(Header, Title, Message);
					return;
				}
			} // Test Blender

			if (!mBlenderVersion.isEmpty() && !mImportMMActBox->isChecked() && !mImportActBox->isChecked())
			{
				const QString& Title = tr ("%1 Blender LDraw Addon Modules").arg(LC_PRODUCTNAME_STR);
				const QString& Header = tr ("No import module enabled. If you continue, the default import module (Import TN) will be used.<br>If you select No, all addon modules will be disabled.");
				const QString& Body = tr ("Continue with the default import module ?");
				int Exec = ShowMessage(Header, Title, Body, QString(), MBB_YES_NO, QMessageBox::NoIcon);
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
					mBlenderVersionEdit->setToolTip(tr("Display the Blender and %1 Render addon version").arg(LC_PRODUCTNAME_STR));
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
						mAddonVersionLabel->setText(tr("Blender Addon"));
						mProgressBar->close();
					}
				}
				return;
			}

			if (!QFileInfo(BlenderInstallFile).exists())
			{
				ShowMessage(tr ("Could not find addon install file: %1").arg(BlenderInstallFile));
				StatusUpdate(true, true, tr("Not found."));
				return;
			}

			StatusUpdate(true, false, tr("Installing..."));

			QDir ConfigDir(BlenderConfigDir);
			if(!QDir(ConfigDir).exists())
				ConfigDir.mkpath(".");
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

		Message = tr("Blender Addon Install Arguments: %1 %2").arg(BlenderExe).arg(Arguments.join(" "));

		if (!TestBlender)
			mBlenderVersionFound = false;

		if (QDir(BlenderAddonDir).entryInfoList(QDir::Dirs|QDir::NoSymLinks).count() > 0)
		{
			QJsonArray JsonArray;
			QStringList AddonDirs = QDir(BlenderAddonDir).entryList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::SortByMask);
			for (const QString& Addon : AddonDirs)
			{
				QDir Dir(QString("%1/%2").arg(BlenderAddonDir).arg(Addon));
				Dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
				QFileInfoList List = Dir.entryInfoList();
				for (int LblIdx = 0; LblIdx < List.size(); LblIdx++)
				{
					if (List.at(LblIdx).fileName() == QLatin1String("__init__.py"))
					{
						QFile File(QFileInfo(List.at(LblIdx)).absoluteFilePath());
						if (!File.open(QFile::ReadOnly | QFile::Text))
						{
							ShowMessage(tr("Cannot read addon file %1<br>%2").arg(List.at(LblIdx).fileName()).arg(File.errorString()));
							break;
						}
						else
						{
							bool FoundModule = false;
							QTextStream In(&File);
							while ( ! In.atEnd())
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

		Result = ProcessCommand(PR_INSTALL);

		if (Result != PR_OK)
			StatusUpdate(true, true, tr("Install failed."));
	}
	else
		ShowMessage(tr("Blender executable not found at [%1]").arg(BlenderExe), tr("Addon install failed."));
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
		const QString BlenderAddonFile = QDir::toNativeSeparators(QString("%1/%2").arg(BlenderDir).arg(LC_BLENDER_ADDON_FILE));

		QString Result;
		Extracted = gAddonPreferences->ExtractAddon(BlenderAddonFile, Result);

		if (!Extracted)
		{
			QString Message = tr("Failed to extract %1 to %2").arg(LC_BLENDER_ADDON_FILE).arg(BlenderDir);
			if (Result.size())
				Message.append(" "+Result);

			ShowMessage(Message, tr("Extract addon"));
		}
	}

	return Extracted;
}

int lcBlenderPreferences::GetBlenderAddon(const QString& BlenderDir)
{
	const QString BlenderAddonDir = QDir::toNativeSeparators(QString("%1/addons").arg(BlenderDir));
	const QString BlenderAddonFile = QDir::toNativeSeparators(QString("%1/%2").arg(BlenderDir).arg(LC_BLENDER_ADDON_FILE));
	const QString AddonVersionFile = QDir::toNativeSeparators(QString("%1/%2/__version__.py").arg(BlenderAddonDir).arg(LC_BLENDER_ADDON_FOLDER_STR));
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
		connect(HttpManager, SIGNAL(DownloadFinished(lcHttpReply*)), gAddonPreferences, SLOT(DownloadFinished(lcHttpReply*)));
		gAddonPreferences->mHttpReply = HttpManager->DownloadFile(QLatin1String(LC_BLENDER_ADDON_LATEST_URL));
		while (gAddonPreferences->mHttpReply)
			QApplication::processEvents();
		if (!gAddonPreferences->mData.isEmpty())
		{
			QJsonDocument Json = QJsonDocument::fromJson(gAddonPreferences->mData);
			OnlineVersion = Json.object()["tag_name"].toString();
			gAddonPreferences->mData.clear();
		}
		else
		{
			ShowMessage(tr("Check latest addon version failed."), tr("Latest Addon"), QString(), QString(), MBB_OK, QMessageBox::Warning);
			return true; // Reload existing archive
		}

		QByteArray Ba;
		if (!ExtractedAddon)
		{
			const char* VersionFile = "addons/" LC_BLENDER_ADDON_FOLDER_STR "/__version__.py";

			lcZipFile ZipFile;

			if (!ZipFile.OpenRead(BlenderAddonFile))
			{
				ShowMessage(tr("Cannot open addon archive file: %1.").arg(BlenderAddonFile));
				return false;
			}

			lcMemFile File;

			if (!ZipFile.ExtractFile(VersionFile, File))
			{
				ShowMessage(tr("Cannot extract addon archive version file: %1.").arg(VersionFile));
				return false;
			}

			Ba = QByteArray::fromRawData((const char*)File.mBuffer, (int)File.GetLength());
			if (Ba.isEmpty())
			{
				ShowMessage(tr("Cannot read addon archive version file: %1.").arg(VersionFile));
				return false; // Download new archive
			}
		}
		else
		{
			QFile File(AddonVersionFile);
			if (!File.open(QIODevice::ReadOnly))
			{
				ShowMessage(tr("Cannot read addon version file: [%1]<br>%2.").arg(AddonVersionFile).arg(File.errorString()));
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
			AddonAction = ADDON_NO_ACTION;
		}
		else if (gMainWindow)
		{
			if (lcGetProfileInt(LC_PROFILE_BLENDER_ADDON_VERSION_CHECK))
			{
				if (LocalVersion.isEmpty())
					LocalVersion = gAddonPreferences->mAddonVersion;
				const QString& Title = tr ("%1 Blender LDraw Addon").arg(LC_PRODUCTNAME_STR);
				const QString& Header = tr ("Detected %1 Blender LDraw addon %2. A newer version %3 exists.").arg(LC_PRODUCTNAME_STR).arg(LocalVersion).arg(OnlineVersion);
				const QString& Body = tr ("Do you want to download version %1 ?").arg(OnlineVersion);
				int Exec = ShowMessage(Header, Title, Body, QString(), MBB_YES, QMessageBox::NoIcon);
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
		if (QFileInfo(BlenderAddonDir).exists())
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

			if (QFileInfo(OldBlenderAddonFile).exists())
				Result &= QFile::remove(OldBlenderAddonFile);

			Result &= Dir.rmdir(BlenderAddonDir);
			if (!Result)
				ShowMessage(tr("Failed to properly remove Blender addon: %1").arg(BlenderAddonDir), tr("Remove Existing Addon"), QString(), QString(), MBB_OK, QMessageBox::Warning);
		}
	};

	BlenderAddonValidated = false;
	lcHttpManager* HttpManager = new lcHttpManager(gAddonPreferences);
	connect(HttpManager, SIGNAL(DownloadFinished(lcHttpReply*)), gAddonPreferences, SLOT(DownloadFinished(lcHttpReply*)));
	gAddonPreferences->mHttpReply = HttpManager->DownloadFile(QLatin1String(LC_BLENDER_ADDON_URL));
	while (gAddonPreferences->mHttpReply)
		QApplication::processEvents();
	if (!gAddonPreferences->mData.isEmpty())
	{
		const QString OldBlenderAddonFile = QString("%1.hold").arg(BlenderAddonFile);
		if (QFileInfo(BlenderAddonFile).exists())
		{
			if (!QFile::rename(BlenderAddonFile, OldBlenderAddonFile))
				ShowMessage(tr("Failed to rename existing Blender addon archive %1.").arg(BlenderAddonFile));
		}

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
				int BytesRead;
				int ReadSize = qMin(DataSize, BufferSize);
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
							ShowMessage(tr("Failed to validate Blender addon file name<br>Downloaded:%1<br>Received:%2").arg(ArchiveFileName, ShaReceived.last()));
					}
					else
						ShowMessage(tr("Failed to validate Blender addon SHA hash <br>Calculated:%1<br>Received:%2").arg(ShaCalculated, ShaReceived.first()));
					gAddonPreferences->mData.clear();
				}
				else
					ShowMessage(tr("Failed to receive SHA hash for Blender addon %1.sha256").arg(LC_BLENDER_ADDON_FILE));
			}
			else
				ShowMessage(tr("Failed to read Blender addon archive:<br>%1:<br>%2").arg(BlenderAddonFile).arg(File.errorString()));
		}
		else
			ShowMessage(tr("Failed to write Blender addon archive:<br>%1:<br>%2").arg(BlenderAddonFile).arg(File.errorString()));

		if (!BlenderAddonValidated)
		{
			if (QFileInfo(BlenderAddonFile).exists())
				if (!QFile::remove(BlenderAddonFile))
					ShowMessage(tr("Failed to remove invalid Blender addon archive:<br>%1").arg(BlenderAddonFile));
			if (QFileInfo(OldBlenderAddonFile).exists())
				if (!QFile::rename(OldBlenderAddonFile, BlenderAddonFile))
					ShowMessage(tr("Failed to restore Blender addon archive:<br>%1 from %2").arg(ArchiveFileName, OldArchiveFileName));
			AddonAction = ADDON_FAIL;
		}
	}
	else
	{
		ShowMessage(tr("Failed to download Blender addon archive:<br>%1").arg(BlenderAddonFile));
		AddonAction = ADDON_FAIL;
	}

	if (!BlenderAddonValidated)
		gAddonPreferences->StatusUpdate(true, true, tr("Download failed."));

	if (!QDir(BlenderAddonDir).exists() && QFileInfo(BlenderAddonFile).exists())
		AddonAction = ADDON_EXTRACT;

	return AddonAction;
}

void lcBlenderPreferences::StatusUpdate(bool Addon, bool Error, const QString& Message)
{
	QString Label, Colour;
	const QString Which = Addon ? tr("Blender addon") : tr("Blender");
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

		const lcPreferences& Preferences = lcGetPreferences();
		Label  = ! Message.isEmpty() ? Message : tr("%1 not configured").arg(Which);
		Colour = Message.startsWith("Error:", Qt::CaseInsensitive)
						? QLatin1String("red")
						: Preferences.mColorTheme == lcColorTheme::Dark
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
	QString Message;
	bool Error;
	const QString StdErrLog = ReadStdErr(Error);

	if (mProgressBar)
		mProgressBar->close();

	WriteStdOut();

	if (mProcess->exitStatus() != QProcess::NormalExit || mProcess->exitCode() != 0 || Error)
	{
		const QString BlenderDir = QString("%1/Blender").arg(mDataDir);
		Message = tr("Addon install failed. See %1/stderr-blender-addon-install for details.").arg(BlenderDir);
		StatusUpdate(true, true,tr("Error: Install failed."));
		mConfigured = false;

		const QString& Title = tr ("%1 Blender Addon Install").arg(LC_PRODUCTNAME_STR);
		const QString& Header = "<b>" + tr ("Addon install failed.") + "</b>";
		const QString& Body = tr ("LDraw addon install encountered one or more errors. See Show Details...");
		ShowMessage(Header, Title, Body, StdErrLog, MBB_OK, QMessageBox::Warning);
	}
	else
	{
		const QString TextColour = QString("QLabel { color : %1; }").arg(QApplication::palette().text().color().name());
		mAddonGridLayout->replaceWidget(mProgressBar, mAddonVersionEdit);
		mConfigured = true;
		mBlenderVersionLabel->setText(tr("Blender"));
		mBlenderVersionLabel->setStyleSheet(TextColour);
		mBlenderVersionEdit->setText(mBlenderVersion);
		mBlenderVersionEdit->setToolTip(tr("Display the Blender and %1 Render addon version").arg(LC_PRODUCTNAME_STR));
		mBlenderVersionEdit->setVisible(mConfigured);
		mPathsBox->setEnabled(mConfigured);
		mSettingsBox->setEnabled(mConfigured);

		if (!mAddonVersion.isEmpty())
		{
			mAddonVersionLabel->setText(tr("Blender Addon"));
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
		Message = tr("Blender version %1").arg(mBlenderVersion);
	}

	delete mProcess;
	mProcess = nullptr;

	if (Error)
		ShowMessage(Message);
}

void lcBlenderPreferences::SettingChanged(const QString& Value)
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

void lcBlenderPreferences::SettingChanged(int Index)
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
		ShowMessage(tr("Blender addon standard output file not found: %1.").arg(FileInfo.absoluteFilePath()));
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
	QStringList StdOutLines = StdOutput.split(QRegularExpression("\n|\r\n|\r"));
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

	int EditListItems = mPathLineEditList.size();
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
		const QString StdOutLog = QDir::toNativeSeparators(QString("<br>- See %1/Blender/stdout-blender-addon-install")
																.arg(mDataDir));

		QMessageBox::Icon Icon = QMessageBox::Warning;
		const QString& Items = Error ? tr("errors%1").arg(Warning ? tr(" and warnings") : "") : Warning ? tr("warnings") : "";

		const QString& Title = tr ("%1 Blender Addon Install").arg(LC_PRODUCTNAME_STR);
		const QString& Header = "<b>" + tr ("Addon install standard output.") + "</b>";
		const QString& Body = tr ("LDraw addon install encountered %1. See Show Details...").arg(Items);
		ShowMessage(Header, Title, Body, ErrorsAndWarnings.append(StdOutLog), MBB_OK, Icon);
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
	if ( ! File.open(QFile::ReadOnly | QFile::Text))
	{
		const QString Message = tr("Failed to open log file: %1:\n%2").arg(File.fileName()).arg(File.errorString());
		return Message;
	}
	QTextStream In(&File);
	while ( ! In.atEnd())
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
		for (const QString& Line : mStdOutList)
			Out << Line << LineEnding;
		File.close();
		mAddonStdOutButton->setEnabled(true);
	}
	else
		ShowMessage(tr("Error writing to %1 file '%2':\n%3").arg("stdout").arg(File.fileName(), File.errorString()));
}

bool lcBlenderPreferences::PromptCancel()
{
#ifndef QT_NO_PROCESS
	if (mProcess)
	{
		const QString& Title = tr ("Cancel %1 Addon Install").arg(LC_PRODUCTNAME_STR);
		const QString& Header = "<b>" + tr("Are you sure you want to cancel the add on install ?") + "</b>";
		int Exec = ShowMessage(Header, Title, QString(), QString(), MBB_YES_NO, QMessageBox::Question);
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
						_Width  = gAddonPreferences->mLineEditList[CtlIdx]->text().toDouble(&Ok);
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
						_Height = gAddonPreferences->mLineEditList[CtlIdx]->text().toDouble(&Ok);
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
						_Scale = gAddonPreferences->mLineEditList[CtlIdx]->text().toInt(&Ok);
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
						_Value = gAddonPreferences->mLineEditList[CtlIdx]->text().toDouble(&Ok);
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
						_Width  = gAddonPreferences->mLineEditList[CtlIdx]->text().toDouble(&Ok);
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
						_Height = gAddonPreferences->mLineEditList[CtlIdx]->text().toDouble(&Ok);
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
						_Scale = gAddonPreferences->mLineEditList[CtlIdx]->text().toInt(&Ok);
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
							_Value = gAddonPreferences->mLineEditList[CtlIdx]->text().toDouble(&Ok);
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
	Dialog->setWindowTitle(tr("Addon Reset"));
	Dialog->setWhatsThis(tr("Select how to reset settings. Choice is since last apply or system default."));
	QVBoxLayout* Layout = new QVBoxLayout(Dialog);
	QGroupBox* DlgGroup = new QGroupBox(Dialog);
	QHBoxLayout* DlgLayout = new QHBoxLayout(DlgGroup);
	QRadioButton* LastButton = new QRadioButton(tr("Last Apply"));
	DlgLayout->addWidget(LastButton);
	QRadioButton* DefaultButton = new QRadioButton(tr("System Default"));
	DlgLayout->addWidget(DefaultButton);
	DlgGroup->setLayout(DlgLayout);
	Layout->addWidget(DlgGroup);
	Dialog->setLayout(Layout);

	LastButton->setChecked(true);

	QDialogButtonBox ButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
							   Qt::Horizontal, Dialog);
	Layout->addWidget(&ButtonBox);
	connect(&ButtonBox, SIGNAL(accepted()), Dialog, SLOT(accept()));
	connect(&ButtonBox, SIGNAL(rejected()), Dialog, SLOT(reject()));

	if (Dialog->exec() == QDialog::Accepted)
	{
		if (DefaultButton->isChecked())
		{
			Paths = mDefaultPaths;
			Settings = mDefaultSettings;
			SettingsMM = mDefaultSettingsMM;
		}
	}

	mConfigured = !lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty();

	mBlenderPaths[PATH_BLENDER].value = lcGetProfileString(LC_PROFILE_BLENDER_PATH);
	mBlenderVersion                   = lcGetProfileString(LC_PROFILE_BLENDER_VERSION);
	mAddonVersion                     = lcGetProfileString(LC_PROFILE_BLENDER_ADDON_VERSION);

	mBlenderVersionEdit->setText(mBlenderVersion);
	mAddonVersionEdit->setText(mAddonVersion);

	if (mImportActBox->isChecked())
	{
		disconnect(mLineEditList[CTL_IMAGE_HEIGHT_EDIT], SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
		disconnect(mLineEditList[CTL_IMAGE_WIDTH_EDIT], SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));

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
						mLineEditList[CtlIdx]->setText(QString::number(mImageWidth));
					else if (CtlIdx == CTL_IMAGE_HEIGHT_EDIT)
						mLineEditList[CtlIdx]->setText(QString::number(mImageHeight));
					else if (CtlIdx == CTL_RENDER_PERCENTAGE_EDIT)
						mLineEditList[CtlIdx]->setText(QString::number(mScale * 100));
					else if (CtlIdx == CTL_DEFAULT_COLOUR_EDIT)
						SetDefaultColor(lcGetColorIndex(Settings[LBL_DEFAULT_COLOUR].value.toInt()));
					else
						mLineEditList[CtlIdx]->setText(Settings[LblIdx].value);
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

		connect(mLineEditList[CTL_IMAGE_HEIGHT_EDIT],SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
		connect(mLineEditList[CTL_IMAGE_WIDTH_EDIT], SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
	}
	else if (mImportMMActBox->isChecked())
	{
		disconnect(mLineEditList[CTL_RESOLUTION_HEIGHT_EDIT], SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
		disconnect(mLineEditList[CTL_RESOLUTION_WIDTH_EDIT], SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));

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
						mLineEditList[CtlIdx]->setText(QString::number(mImageWidth));
					else if (CtlIdx == CTL_RESOLUTION_HEIGHT_EDIT)
						mLineEditList[CtlIdx]->setText(QString::number(mImageHeight));
					else if (CtlIdx == CTL_RENDER_PERCENTAGE_EDIT_MM)
						mLineEditList[CtlIdx]->setText(QString::number(mScale * 100));
					else
						mLineEditList[CtlIdx]->setText(SettingsMM[LblIdx].value);
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

		connect(mLineEditList[CTL_RESOLUTION_HEIGHT_EDIT], SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
		connect(mLineEditList[CTL_RESOLUTION_WIDTH_EDIT], SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
	}

	emit SettingChangedSig(true);
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

	if (!QDir(QString("%1/Blender/addons/%2").arg(gAddonPreferences->mDataDir).arg(LC_BLENDER_ADDON_FOLDER_STR)).isReadable())
		lcSetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE, QString());

	if (!NumPaths())
	{
		const QString DefaultBlendFile = QString("%1/Blender/config/%2").arg(gAddonPreferences->mDataDir).arg(LC_BLENDER_ADDON_BLEND_FILE);

		QStringList const AddonPaths = QStringList()
		/* 0  PATH_BLENDER      */        << lcGetProfileString(LC_PROFILE_BLENDER_PATH)
		/* 1  PATH_BLENDFILE    */        << (QFileInfo(DefaultBlendFile).exists() ? DefaultBlendFile : QString())
		/* 2  PATH_ENVIRONMENT  */        << QString()
		/* 3  PATH_LDCONFIG     */        << lcGetProfileString(LC_PROFILE_COLOR_CONFIG)
		/* 4  PATH_LDRAW        */        << QFileInfo(lcGetProfileString(LC_PROFILE_PARTS_LIBRARY)).absolutePath()
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
					mDefaultPaths[LblIdx].label,
					mDefaultPaths[LblIdx].tooltip
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
					mDefaultSettings[LblIdx].label,
					mDefaultSettings[LblIdx].tooltip
				};
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
					mDefaultSettingsMM[LblIdx].label,
					mDefaultSettingsMM[LblIdx].tooltip
				};
		}
	}

	QFileInfo BlenderConfigFileInfo(lcGetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH));

	bool ConfigFileExists = BlenderConfigFileInfo.exists();

	if (ConfigFileExists)
	{
		QSettings Settings(BlenderConfigFileInfo.absoluteFilePath(), QSettings::IniFormat);

		for (int LblIdx = 1/*skip blender executable*/; LblIdx < NumPaths(); LblIdx++)
		{
			if (LblIdx >= PATH_STUDIO_LDRAW)
				continue;
			const QString& Key = QString("%1/%2").arg(LC_BLENDER_ADDON, mBlenderPaths[LblIdx].key);
			const QString& Value = Settings.value(Key, QString()).toString();
			if (QFileInfo(Value).exists())
			{
				mBlenderPaths[LblIdx].value = QDir::toNativeSeparators(Value);
			}
		}

		for (int LblIdx = 1/*skip blender executable*/; LblIdx < NumPaths(); LblIdx++)
		{
			if (LblIdx == PATH_LSYNTH || LblIdx == PATH_STUD_LOGO)
				continue;
			const QString& Key = QString("%1/%2").arg(LC_BLENDER_ADDON_MM, mBlenderPaths[LblIdx].key_mm);
			const QString& Value = Settings.value(Key, QString()).toString();
			if (QFileInfo(Value).exists())
			{
				mBlenderPaths[LblIdx].value = QDir::toNativeSeparators(Value);
			}
		}

		for (int LblIdx = 0; LblIdx < NumSettings(); LblIdx++)
		{
			const QString& Key = QString("%1/%2").arg(LC_BLENDER_ADDON, mBlenderSettings[LblIdx].key);
			const QString& Value = Settings.value(Key, QString()).toString();
			if (!Value.isEmpty())
			{
				mBlenderSettings[LblIdx].value = Value == "True" ? "1" : Value == "False" ? "0" : Value;
			}
			if (LblIdx == LBL_IMAGE_WIDTH || LblIdx == LBL_IMAGE_HEIGHT || LblIdx == LBL_RENDER_PERCENTAGE)
			{
				const QString& Label = mDefaultSettings[LblIdx].label;
				mBlenderSettings[LblIdx].label = QString("%1 - Setting (%2)").arg(Label).arg(Value);
			}
		}

		for (int LblIdx = 0; LblIdx < NumSettingsMM(); LblIdx++)
		{
			const QString& Key = QString("%1/%2").arg(LC_BLENDER_ADDON_MM, mBlenderSettingsMM[LblIdx].key);
			const QString& Value = Settings.value(Key, QString()).toString();
			if (!Value.isEmpty())
			{
				mBlenderSettingsMM[LblIdx].value = Value == "True" ? "1" : Value == "False" ? "0" : Value;
			}
			if (LblIdx == LBL_RENDER_PERCENTAGE_MM || LblIdx == LBL_RESOLUTION_WIDTH || LblIdx == LBL_RESOLUTION_HEIGHT)
			{
				const QString& Label = mDefaultSettingsMM[LblIdx].label;
				mBlenderSettingsMM[LblIdx].label = QString("%1 - Setting (%2)").arg(Label).arg(Value);
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
			}
			else
			{
				ShowMessage(tr("Failed to open log file: %1:\n%2")
								.arg(File.fileName())
								.arg(File.errorString()));
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

	const QString BlenderConfigDir = QString("%1/Blender/setup/addon_setup/config").arg(gAddonPreferences->mDataDir);

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

	Value = lcGetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH).isEmpty() ? QString("%1/%2").arg(BlenderConfigDir).arg(LC_BLENDER_ADDON_CONFIG_FILE) : lcGetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH);
	lcSetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH, QDir::toNativeSeparators(Value));

	QString searchDirectoriesKey;
	QString parameterFileKey = QLatin1String("ParameterFile");
	QString ParameterFile = QString("%1/%2").arg(BlenderConfigDir).arg(LC_BLENDER_ADDON_PARAMS_FILE);

	QSettings Settings(Value, QSettings::IniFormat);

	auto concludeSettingsGroup = [&]()
	{
		if (!QFileInfo(ParameterFile).exists())
			ExportParameterFile();
		Value = QDir::toNativeSeparators(QFileInfo(lcGetProfileString(LC_PROFILE_PARTS_LIBRARY)).absolutePath());
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

	searchDirectoriesKey = QLatin1String("additionalSearchDirectories");

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

	searchDirectoriesKey = QLatin1String("additionalSearchPaths");

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
	connect(Popup, SIGNAL(selected(int)), SLOT(SetDefaultColor(int)));
	Popup->setMinimumSize(300, 200);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QScreen* Screen = screen();
	const QRect DesktopGeom = Screen ? Screen->geometry() : QRect();
#else
	const QRect DesktopGeom = QApplication::desktop()->geometry();
#endif

	QPoint Pos = Parent->mapToGlobal(Parent->rect().bottomLeft());
	if (Pos.x() < DesktopGeom.left())
		Pos.setX(DesktopGeom.left());

	if (Pos.y() < DesktopGeom.top())
		Pos.setY(DesktopGeom.top());

	if ((Pos.x() + Popup->width()) > DesktopGeom.width())
		Pos.setX(DesktopGeom.width() - Popup->width());

	if ((Pos.y() + Popup->height()) > DesktopGeom.bottom())
		Pos.setY(DesktopGeom.bottom() - Popup->height());

	Popup->move(Pos);

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
	mDefaultColourEditAction->setToolTip(tr("Select Colour"));

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

void lcBlenderPreferences::SizeChanged(const QString& Value)
{
	const bool ImportMM = mImportMMActBox->isChecked();
	const int Keep_Aspect_Ratio = ImportMM ? int(CTL_KEEP_ASPECT_RATIO_BOX_MM) : int(CTL_KEEP_ASPECT_RATIO_BOX);
	const int Width_Edit = ImportMM ? int(CTL_RESOLUTION_WIDTH_EDIT) : int(CTL_IMAGE_WIDTH_EDIT);
	const int Height_Edit = ImportMM ? int(CTL_RESOLUTION_HEIGHT_EDIT) : int(CTL_IMAGE_HEIGHT_EDIT);
	BlenderSettings const* Settings = ImportMM ? mBlenderSettingsMM : mBlenderSettings;

	bool Change = false;
	int NewValue = Value.toInt();
	if (mCheckBoxList[Keep_Aspect_Ratio]->isChecked())
	{
		if (sender() == mLineEditList[Width_Edit])
		{
			disconnect(mLineEditList[Height_Edit],SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));

			const QString Height = QString::number(qRound(double(mImageHeight * NewValue / mImageWidth)));
			mLineEditList[Height_Edit]->setText(Height);

			Change = Settings[Height_Edit].value != Height;

			connect(mLineEditList[Height_Edit],SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
		}
		else if (sender() == mLineEditList[Height_Edit])
		{
			disconnect(mLineEditList[Width_Edit],SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));

			const QString Width = QString::number(qRound(double(NewValue * mImageWidth / mImageHeight)));
			mLineEditList[Width_Edit]->setText(Width);

			Change = Settings[Height_Edit].value != Width;

			connect(mLineEditList[Width_Edit],SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
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
			const QString& Title = tr ("LDraw Render Settings Conflict");
			const QString& Header = "<b>" + tr ("Crop image configuration settings conflict were resolved.") + "</b>";
			const QString& Body = QString("%1%2%3").arg(Conflict[0] ? tr("Keep aspect ratio set to false.<br>") : "").arg(Conflict[1] ? tr("Add environment (backdrop and base plane) set to false.<br>") : "").arg(Conflict[2] ? tr("Transparent background set to true.<br>") : "");
			ShowMessage(Header, Title, Body, QString(), MBB_OK, QMessageBox::Information);
		}
	}

	disconnect(mLineEditList[Width_Edit],SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
	disconnect(mLineEditList[Height_Edit],SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));

	const QString Width = QString::number(CropImage ? ImageWidth : mImageWidth);
	const QString Height = QString::number(CropImage ? ImageHeight : mImageHeight);

	mLineEditList[Width_Edit]->setText(Width);
	mLineEditList[Height_Edit]->setText(Height);

	if (Update)
		SettingsModified(true);

	connect(mLineEditList[Height_Edit],SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
	connect(mLineEditList[Width_Edit],SIGNAL(textChanged(const QString&)), this, SLOT (SizeChanged(const QString&)));
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

		const QString& Title = tr ("Custom LDraw Colours");
		const QString& Header = "<b>" + tr ("Colour scheme 'custom' cannot be enabled. Custom LDConfig file not found.") + "</b>";
		const QString& Body = tr ("Colour scheme 'custom' selected but no LDConfig file was specified.<br>The default colour scheme '%1' will be used.<br>").arg(Settings[Color_Scheme].value);
		ShowMessage(Header, Title, Body, QString(), MBB_OK, QMessageBox::Warning);
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
	 const QString& Title = tr ("Render Settings Modified");
	 const QString& Header = "<b>" + tr("Do you want to accept the modified settings before quitting ?") + "</b>";
	int Exec = ShowMessage(Header, Title, QString(), QString(), MBB_YES_NO, QMessageBox::Question);
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
	const QString BlenderConfigDir = QString("%1/Blender/setup/addon_setup/config").arg(gAddonPreferences->mDataDir);
	const QString ParameterFile = QString("%1/%2").arg(BlenderConfigDir).arg(LC_BLENDER_ADDON_PARAMS_FILE);
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
		Message = tr("Finished writing Blender parameter entries. Processed %1 lines in file [%2].")
					  .arg(Counter)
					  .arg(File.fileName());
	}
	else
	{
		Message = tr("Failed to open Blender parameter file: %1:<br>%2")
					  .arg(File.fileName())
					  .arg(File.errorString());
		ShowMessage(Message);
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
	const QString Header = "<b>" + QMessageBox::tr ("Existing file %1 detected.").arg(fileInfo.fileName()) + "</b>";
	const QString Body = QMessageBox::tr ("\"%1\"<br>This file already exists.<br>Replace existing file?").arg(fileInfo.fileName());
	int Exec = ShowMessage(Header, Title, Body, QString(), MBB_YES, QMessageBox::NoIcon);

	return (Exec == QMessageBox::Yes);
}

int lcBlenderPreferences::ShowMessage(const QString& Header,  const QString& Title,  const QString& Body, const QString& Detail, int const Buttons, int const Icon)
{
	if (!gMainWindow)
		return QMessageBox::Ok;

	QMessageBox Box;
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
	if (!Title.isEmpty())
		Box.setWindowTitle(Title);
	else
		Box.setWindowTitle(tr("%1 Blender LDraw Addon").arg(LC_PRODUCTNAME_STR));

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
			int FixedWidth = Body.length() * FontWidth;
			if (FixedWidth == MinimumWidth)
			{
				int Index = (MinimumWidth / FontWidth) - 1;
				if (!Body.mid(Index,1).isEmpty())
					FixedWidth = Body.indexOf(" ", Index);
			}
			else if (FixedWidth < MinimumWidth)
				FixedWidth = MinimumWidth;
			TextWidget->setFixedWidth(FixedWidth);
		}
	}

	const bool DownloadRequest = Body.startsWith(tr("Do you want to download version "));

	if (DownloadRequest){
		QCheckBox* AddonVersionCheck = new QCheckBox(tr("Do not show download new addon version message again."));
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
		ShowMessage(tr("Addon download failed."));

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
			ShowMessage(tr("ZipFile entry format (HostOS %1) at index %2 is not supported. Extract terminated.").arg(HostOS).arg(FileIdx), tr("Extract Addon"), QString(), QString(), MBB_OK, QMessageBox::Warning);
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
		Result = tr("%1 of %2 files extracted.").arg(Extracted).arg(ZipFile.mFiles.size());

	return Ok;
}

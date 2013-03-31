#include "lc_global.h"
#include "lc_action.h"
#include "system.h"

lcAction gActions[LC_NUM_COMMANDS] =
{
	// LC_FILE_NEW
	{
		"&New",
		"Create a new project",
		"Ctrl+N"
	},
	// LC_FILE_OPEN
	{
		"&Open...",
		"Open an existing project",
		"Ctrl+O"
	},
	// LC_FILE_MERGE
	{
		"&Merge...",
		"Merge the contents of another project with the current one",
		""
	},
	// LC_FILE_SAVE
	{
		"&Save",
		"Save the active project",
		"Ctrl+S"
	},
	// LC_FILE_SAVEAS
	{
		"Save &As...",
		"Save the active project with a new name",
		""
	},
	// LC_FILE_SAVE_IMAGE
	{
		"Save &Image...",
		"Save a picture of the current view",
		""
	},
	// LC_FILE_EXPORT_3DS
	{
		"3D &Studio...",
		"Export the project in 3D Studio 3DS format",
		""
	},
	// LC_FILE_EXPORT_HTML
	{
		"&HTML...",
		"Create an HTML page for this project",
		""
	},
	// LC_FILE_EXPORT_BRICKLINK
	{
		"&BrickLink...",
		"Export a list of pieces used in BrickLink XML format",
		""
	},
	// LC_FILE_EXPORT_CSV
	{
		"&CSV...",
		"Export a list of pieces used in comma delimited file format",
		""
	},
	// LC_FILE_EXPORT_POVRAY
	{
		"&POV-Ray...",
		"Export the project in POV-Ray format",
		""
	},
	// LC_FILE_EXPORT_WAVEFRONT
	{
		"&Wavefront...",
		"Export the project in Wavefront OBJ format",
		""
	},
	// LC_FILE_PROPERTIES
	{
		"Prope&rties...",
		"Display project properties",
		""
	},
	// LC_FILE_TERRAIN_EDITOR
	{
		"&Terrain Editor...",
		"Edit terrain",
		""
	},
	// LC_FILE_PRINT
	{
		"&Print...",
		"Print the active project",
		""
	},
	// LC_FILE_PRINT_PREVIEW
	{
		"Print Pre&view",
		"Display how the project would look if printed",
		""
	},
	// LC_FILE_PRINT_BOM
	{
		"Print &Bill of Materials...",
		"Print a list of pieces used",
		""
	},
	// LC_FILE_RECENT1
	{
		"&Recent1",
		"Open this document",
		""
	},
	// LC_FILE_RECENT2
	{
		"&Recent2",
		"Open this document",
		""
	},
	// LC_FILE_RECENT3
	{
		"&Recent3",
		"Open this document",
		""
	},
	// LC_FILE_RECENT4
	{
		"&Recent4",
		"Open this document",
		""
	},
	// LC_FILE_EXIT
	{
		"E&xit",
		"Quit the application; prompts to save project",
		""
	},
	// LC_EDIT_UNDO
	{
		"Undo",
		"Undo the last action",
		"Ctrl+Z"
	},
	// LC_EDIT_REDO
	{
		"Redo",
		"Redo the previously undone action",
		"Ctrl+Y"
	},
	// LC_EDIT_CUT
	{
		"Cut",
		"Cut the selection and put it on the Clipboard",
		"Ctrl+X"
	},
	// LC_EDIT_COPY
	{
		"Copy",
		"Copy the selection and put it on the Clipboard",
		"Ctrl+C"
	},
	// LC_EDIT_PASTE
	{
		"Paste",
		"Insert Clipboard contents",
		"Ctrl+V"
	},
	// LC_EDIT_SELECT_ALL
	{
		"Select All",
		"Select all pieces in the project",
		"Ctrl+A"
	},
	// LC_EDIT_SELECT_NONE
	{
		"Select None",
		"De-select everything",
		""
	},
	// LC_EDIT_SELECT_INVERT
	{
		"Select Invert",
		"Invert the current selection set",
		"Ctrl+I"
	},
	// LC_EDIT_SELECT_BY_NAME
	{
		"Select by Name...",
		"Select objects by name",
		""
	},
	// LC_EDIT_LOCK_X
	{
		"Lock X",
		"Prevents movement and rotation along the X axis",
		""
	},
	// LC_EDIT_LOCK_Y
	{
		"Lock Y",
		"Prevents movement and rotation along the Y axis",
		""
	},
	// LC_EDIT_LOCK_Z
	{
		"Lock Z",
		"Prevents movement and rotation along the Z axis",
		""
	},
	// LC_EDIT_LOCK_TOGGLE
	{
		"Lock Toggle",
		"Toggle locked axes",
		""
	},
	// LC_EDIT_LOCK_NONE
	{
		"Unlock All",
		"Allows movement and rotation in all directions",
		""
	},
	// LC_EDIT_SNAP_X
	{
		"Snap X",
		"Snap movement along the X axis to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_Y
	{
		"Snap Y",
		"Snap movement along the Y axis to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_Z
	{
		"Snap Z",
		"Snap movement along the Z axis to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_TOGGLE
	{
		"Snap Toggle",
		"Toggle snap axes",
		""
	},
	// LC_EDIT_SNAP_NONE
	{
		"Snap None",
		"Disable snapping along all axes",
		""
	},
	// LC_EDIT_SNAP_ALL
	{
		"Snap All",
		"Snap movement along all axes to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_ANGLE
	{
		"Snap Angle Toggle",
		"Snap rotations to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_MOVE_XY0
	{
		"None",
		"Do not snap movement along the XY plane",
		"0"
	},
	// LC_EDIT_SNAP_MOVE_XY1
	{
		"1/20 Stud",
		"Snap movement along the XY plane to 1/20 stud",
		"1"
	},
	// LC_EDIT_SNAP_MOVE_XY2
	{
		"1/4 Stud",
		"Snap movement along the XY plane to 1/4 stud",
		"2"
	},
	// LC_EDIT_SNAP_MOVE_XY3
	{
		"1 Flat",
		"Snap movement along the XY plane to 1 flat",
		"3"
	},
	// LC_EDIT_SNAP_MOVE_XY4
	{
		"1/2 Stud",
		"Snap movement along the XY plane to 1/2 stud",
		"4"
	},
	// LC_EDIT_SNAP_MOVE_XY5
	{
		"1 Stud",
		"Snap movement along the XY plane to 1 stud",
		"5"
	},
	// LC_EDIT_SNAP_MOVE_XY6
	{
		"2 Studs",
		"Snap movement along the XY plane to 2 studs",
		"6"
	},
	// LC_EDIT_SNAP_MOVE_XY7
	{
		"3 Studs",
		"Snap movement along the XY plane to 3 studs",
		"7"
	},
	// LC_EDIT_SNAP_MOVE_XY8
	{
		"4 Studs",
		"Snap movement along the XY plane to 4 studs",
		"8"
	},
	// LC_EDIT_SNAP_MOVE_XY9
	{
		"8 Studs",
		"Snap movement along the XY plane to 8 studs",
		"9"
	},
	// LC_EDIT_SNAP_MOVE_Z0
	{
		"None",
		"Do not snap movement along the Z axis",
		"Ctrl+Shift+0"
	},
	// LC_EDIT_SNAP_MOVE_Z1
	{
		"1/20 Stud",
		"Snap movement along the Z axis to 1/20 stud",
		"Ctrl+Shift+1"
	},
	// LC_EDIT_SNAP_MOVE_Z2
	{
		"1/4 Stud",
		"Snap movement along the Z axis to 1/4 stud",
		"Ctrl+Shift+2"
	},
	// LC_EDIT_SNAP_MOVE_Z3
	{
		"1 Flat",
		"Snap movement along the Z axis to 1 flat",
		"Ctrl+Shift+3"
	},
	// LC_EDIT_SNAP_MOVE_Z4
	{
		"1/2 Stud",
		"Snap movement along the Z axis to 1/2 stud",
		"Ctrl+Shift+4"
	},
	// LC_EDIT_SNAP_MOVE_Z5
	{
		"1 Stud",
		"Snap movement along the Z axis to 1 stud",
		"Ctrl+Shift+5"
	},
	// LC_EDIT_SNAP_MOVE_Z6
	{
		"1 Brick",
		"Snap movement along the Z axis to 1 brick",
		"Ctrl+Shift+6"
	},
	// LC_EDIT_SNAP_MOVE_Z7
	{
		"2 Bricks",
		"Snap movement along the Z axis to 2 bricks",
		"Ctrl+Shift+7"
	},
	// LC_EDIT_SNAP_MOVE_Z8
	{
		"4 Bricks",
		"Snap movement along the Z axis to 4 bricks",
		"Ctrl+Shift+8"
	},
	// LC_EDIT_SNAP_MOVE_Z9
	{
		"8 Bricks",
		"Snap movement along the Z axis to 8 bricks",
		"Ctrl+Shift+9"
	},
	// LC_EDIT_SNAP_ANGLE0
	{
		"None",
		"Do not snap rotations",
		"Shift+0"
	},
	// LC_EDIT_SNAP_ANGLE1
	{
		"1 Degree",
		"Snap rotations to 1 degree",
		"Shift+1"
	},
	// LC_EDIT_SNAP_ANGLE2
	{
		"5 Degrees",
		"Snap rotations to 5 degrees",
		"Shift+2"
	},
	// LC_EDIT_SNAP_ANGLE3
	{
		"10 Degrees",
		"Snap rotations to 10 degrees",
		"Shift+3"
	},
	// LC_EDIT_SNAP_ANGLE4
	{
		"15 Degrees",
		"Snap rotations to 15 degrees",
		"Shift+4"
	},
	// LC_EDIT_SNAP_ANGLE5
	{
		"30 Degrees",
		"Snap rotations to 30 degrees",
		"Shift+5"
	},
	// LC_EDIT_SNAP_ANGLE6
	{
		"45 Degrees",
		"Snap rotations to 45 degrees",
		"Shift+6"
	},
	// LC_EDIT_SNAP_ANGLE7
	{
		"60 Degrees",
		"Snap rotations to 60 degrees",
		"Shift+7"
	},
	// LC_EDIT_SNAP_ANGLE8
	{
		"90 Degrees",
		"Snap rotations to 90 degrees",
		"Shift+8"
	},
	// LC_EDIT_SNAP_ANGLE9
	{
		"180 Degrees",
		"Snap rotations to 180 degrees",
		"Shift+9"
	},
	// LC_EDIT_TRANSFORM
	{
		"Transform",
		"Apply transform to selected objects",
		""
	},
	// LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION
	{
		"Absolute Translation",
		"Switch to absolute translation mode when applying transforms",
		""
	},
	// LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION
	{
		"Relative Translation",
		"Switch to relative translation mode when applying transforms",
		""
	},
	// LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION
	{
		"Absolute Rotation",
		"Switch to absolute rotation mode when applying transforms",
		""
	},
	// LC_EDIT_TRANSFORM_RELATIVE_ROTATION
	{
		"Relative Rotation",
		"Switch to relative rotation mode when applying transforms",
		""
	},
	// LC_EDIT_ACTION_INSERT
	{
		"Insert",
		"Add new pieces to the model",
		""
	},
	// LC_EDIT_ACTION_LIGHT
	{
		"Light",
		"Add new omni light sources to the model",
		""
	},
	// LC_EDIT_ACTION_SPOTLIGHT
	{
		"Spotlight",
		"Add new spotlights to the model",
		""
	},
	// LC_EDIT_ACTION_CAMERA
	{
		"Camera",
		"Create a new camera",
		""
	},
	// LC_EDIT_ACTION_SELECT
	{
		"Select",
		"Select objects (hold the CTRL key down or drag the mouse to select multiple objects)",
		"Shift+S"
	},
	// LC_EDIT_ACTION_MOVE
	{
		"Move",
		"Move selected objects",
		"Shift+M"
	},
	// LC_EDIT_ACTION_ROTATE
	{
		"Rotate",
		"Rotate selected pieces",
		"Shift+R"
	},
	// LC_EDIT_ACTION_DELETE
	{
		"Delete",
		"Delete objects",
		"Shift+D"
	},
	// LC_EDIT_ACTION_PAINT
	{
		"Paint",
		"Change piece color",
		"Shift+N"
	},
	// LC_EDIT_ACTION_ZOOM
	{
		"Zoom",
		"Zoom in or out",
		"Shift+Z"
	},
	// LC_EDIT_ACTION_PAN
	{
		"Pan",
		"Pan the current view",
		"Shift+P"
	},
	// LC_EDIT_ACTION_ROTATE_VIEW
	{
		"Rotate View",
		"Rotate the current view",
		"Shift+T"
	},
	// LC_EDIT_ACTION_ROLL
	{
		"Roll",
		"Roll the current view",
		"Shift+L"
	},
	// LC_EDIT_ACTION_ZOOM_REGION
	{
		"Zoom Region",
		"Zoom into a region of the screen",
		""
	},
	// LC_VIEW_PREFERENCES
	{
		"Preferences...",
		"Change program settings",
		""
	},
	// LC_VIEW_ZOOM_IN
	{
		"Zoom In",
		"Zoom in",
		""
	},
	// LC_VIEW_ZOOM_OUT
	{
		"Zoom Out",
		"Zoom out",
		""
	},
	// LC_VIEW_ZOOM_EXTENTS
	{
		"Zoom Extents",
		"Fit all pieces in current the view (hold the CTRL key down to zoom all views)",
		""
	},
	// LC_VIEW_VIEWPOINT_FRONT
	{
		"Front",
		"View model from the front",
		"F"
	},
	// LC_VIEW_VIEWPOINT_BACK
	{
		"Back",
		"View model from the back",
		"B"
	},
	// LC_VIEW_VIEWPOINT_TOP
	{
		"Top",
		"View model from the top",
		"T"
	},
	// LC_VIEW_VIEWPOINT_BOTTOM
	{
		"Bottom",
		"View model from the bottom",
		"B"
	},
	// LC_VIEW_VIEWPOINT_LEFT
	{
		"Left",
		"View model from the left",
		"L"
	},
	// LC_VIEW_VIEWPOINT_RIGHT
	{
		"Right",
		"View model from the right",
		"R"
	},
	// LC_VIEW_VIEWPOINT_HOME
	{
		"Home",
		"View model from the default position",
		"H"
	},
	// LC_VIEW_CAMERA_NONE
	{
		"None",
		"Do not use a camera",
		""
	},
	// LC_VIEW_CAMERA1
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA2
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA3
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA4
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA5
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA6
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA7
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA8
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA9
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA10
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA11
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA12
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA13
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA14
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA15
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA16
	{
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA_RESET
	{
		"Reset",
		"Reset views to their default positions",
		""
	},
	// LC_VIEW_TIME_FIRST
	{
		"First",
		"Go to the first step of the model",
		""
	},
	// LC_VIEW_TIME_PREVIOUS
	{
		"Previous",
		"Go to the previous step",
		""
	},
	// LC_VIEW_TIME_NEXT
	{
		"Next",
		"Go to the next step",
		""
	},
	// LC_VIEW_TIME_LAST
	{
		"Last",
		"Go to the last step of the model",
		""
	},
	// LC_VIEW_TIME_STOP
	{
		"Stop",
		"Stop playing animation",
		""
	},
	// LC_VIEW_TIME_PLAY
	{
		"Play",
		"Play animation",
		""
	},
	// LC_VIEW_TIME_INSERT
	{
		"Insert",
		"Insert new step",
		""
	},
	// LC_VIEW_TIME_DELETE
	{
		"Delete",
		"Delete current step",
		""
	},
	// LC_VIEW_TIME_ANIMATION
	{
		"Animation",
		"Toggle between animation and instruction mode",
		""
	},
	// LC_VIEW_TIME_ADD_KEYS
	{
		"Add Keys",
		"Toggle adding new animation keys",
		""
	},
	// LC_PIECE_INSERT
	{
		"Insert",
		"Add a new piece to the model",
		"Insert"
	},
	// LC_PIECE_DELETE
	{
		"Delete",
		"Delete selected objects",
		"Delete"
	},
	// LC_PIECE_MINIFIG_WIZARD
	{
		"Minifig Wizard...",
		"Add a new minifig to the model",
		""
	},
	// LC_PIECE_ARRAY
	{
		"Array...",
		"Make copies of the selected pieces",
		""
	},
	// LC_PIECE_COPY_KEYS
	{
		"Copy Keys",
		"Copy keys between animation and instruction modes",
		""
	},
	// LC_PIECE_GROUP
	{
		"Group...",
		"Group selected pieces together",
		"Ctrl+G"
	},
	// LC_PIECE_UNGROUP
	{
		"Ungroup",
		"Ungroup selected group",
		"Ctrl+U"
	},
	// LC_PIECE_GROUP_ADD
	{
		"Add to Group",
		"Add focused piece to selected group",
		""
	},
	// LC_PIECE_GROUP_REMOVE
	{
		"Remove from Group",
		"Remove focused piece from group",
		""
	},
	// LC_PIECE_GROUP_EDIT
	{
		"Edit Groups...",
		"Edit groups",
		""
	},
	// LC_PIECE_HIDE_SELECTED
	{
		"Hide Selected",
		"Hide selected objects",
		"Ctrl+H"
	},
	// LC_PIECE_HIDE_UNSELECTED
	{
		"Hide Unselected",
		"Hide objects that are not selected",
		""
	},
	// LC_PIECE_UNHIDE_ALL
	{
		"Unhide All",
		"Show all hidden objects",
		""
	},
	// LC_PIECE_SHOW_EARLIER
	{
		"Show Earlier",
		"Show selected pieces one step earlier",
		""
	},
	// LC_PIECE_SHOW_LATER
	{
		"Show Later",
		"Show selected pieces one step later",
		""
	},
	// LC_HELP_HOMEPAGE
	{
		"LeoCAD &Home Page",
		"Open LeoCAD's home page on the internet using your default web browser",
		""
	},
	// LC_HELP_EMAIL
	{
		"Send Support &E-Mail",
		"Send an e-mail message for help or support using your default e-mail client",
		""
	},
	// LC_HELP_UPDATES
	{
		"Check for &Updates...",
		"Check if a newer LeoCAD version or parts library has been released",
		""
	},
	// LC_HELP_ABOUT
	{
		"&About...",
		"Display program version number and system information",
		""
	}
};

LC_CASSERT(sizeof(gActions)/sizeof(gActions[0]) == LC_NUM_COMMANDS);

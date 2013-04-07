#include "lc_global.h"
#include "lc_action.h"
#include "system.h"

lcAction gActions[LC_NUM_COMMANDS] =
{
	// LC_FILE_NEW
	{
		"File.New",
		"&New",
		"Create a new project",
		"Ctrl+N"
	},
	// LC_FILE_OPEN
	{
		"File.Open",
		"&Open...",
		"Open an existing project",
		"Ctrl+O"
	},
	// LC_FILE_MERGE
	{
		"File.Merge",
		"&Merge...",
		"Merge the contents of another project with the current one",
		""
	},
	// LC_FILE_SAVE
	{
		"File.Save",
		"&Save",
		"Save the active project",
		"Ctrl+S"
	},
	// LC_FILE_SAVEAS
	{
		"File.SaveAs",
		"Save &As...",
		"Save the active project with a new name",
		""
	},
	// LC_FILE_SAVE_IMAGE
	{
		"File.SaveImage",
		"Save &Image...",
		"Save a picture of the current view",
		""
	},
	// LC_FILE_EXPORT_3DS
	{
		"File.Export.3DS",
		"3D &Studio...",
		"Export the project in 3D Studio 3DS format",
		""
	},
	// LC_FILE_EXPORT_HTML
	{
		"File.Export.HTML",
		"&HTML...",
		"Create an HTML page for this project",
		""
	},
	// LC_FILE_EXPORT_BRICKLINK
	{
		"File.Export.BrickLink",
		"&BrickLink...",
		"Export a list of pieces used in BrickLink XML format",
		""
	},
	// LC_FILE_EXPORT_CSV
	{
		"File.Export.CSV",
		"&CSV...",
		"Export a list of pieces used in comma delimited file format",
		""
	},
	// LC_FILE_EXPORT_POVRAY
	{
		"File.Export.POVRay",
		"&POV-Ray...",
		"Export the project in POV-Ray format",
		""
	},
	// LC_FILE_EXPORT_WAVEFRONT
	{
		"File.Export.Wavefront",
		"&Wavefront...",
		"Export the project in Wavefront OBJ format",
		""
	},
	// LC_FILE_PROPERTIES
	{
		"File.Properties",
		"Prope&rties...",
		"Display project properties",
		""
	},
	// LC_FILE_TERRAIN_EDITOR
	{
		"File.TerrainEditor",
		"&Terrain Editor...",
		"Edit terrain",
		""
	},
	// LC_FILE_PRINT
	{
		"File.Print",
		"&Print...",
		"Print the active project",
		""
	},
	// LC_FILE_PRINT_PREVIEW
	{
		"File.PrintPreview",
		"Print Pre&view",
		"Display how the project would look if printed",
		""
	},
	// LC_FILE_PRINT_BOM
	{
		"File.PrintBOM",
		"Print &Bill of Materials...",
		"Print a list of pieces used",
		""
	},
	// LC_FILE_RECENT1
	{
		"File.Recent1",
		"&Recent1",
		"Open this document",
		""
	},
	// LC_FILE_RECENT2
	{
		"File.Recent2",
		"&Recent2",
		"Open this document",
		""
	},
	// LC_FILE_RECENT3
	{
		"File.Recent3",
		"&Recent3",
		"Open this document",
		""
	},
	// LC_FILE_RECENT4
	{
		"File.Recent4",
		"&Recent4",
		"Open this document",
		""
	},
	// LC_FILE_EXIT
	{
		"File.Exit",
		"E&xit",
		"Quit the application; prompts to save project",
		""
	},
	// LC_EDIT_UNDO
	{
		"Edit.Undo",
		"Undo",
		"Undo the last action",
		"Ctrl+Z"
	},
	// LC_EDIT_REDO
	{
		"Edit.Redo",
		"Redo",
		"Redo the previously undone action",
		"Ctrl+Y"
	},
	// LC_EDIT_CUT
	{
		"Edit.Cut",
		"Cut",
		"Cut the selection and put it on the Clipboard",
		"Ctrl+X"
	},
	// LC_EDIT_COPY
	{
		"Edit.Copy",
		"Copy",
		"Copy the selection and put it on the Clipboard",
		"Ctrl+C"
	},
	// LC_EDIT_PASTE
	{
		"Edit.Paste",
		"Paste",
		"Insert Clipboard contents",
		"Ctrl+V"
	},
	// LC_EDIT_SELECT_ALL
	{
		"Edit.SelectAll",
		"Select All",
		"Select all pieces in the project",
		"Ctrl+A"
	},
	// LC_EDIT_SELECT_NONE
	{
		"Edit.SelectNone",
		"Select None",
		"De-select everything",
		""
	},
	// LC_EDIT_SELECT_INVERT
	{
		"Edit.SelectInvert",
		"Select Invert",
		"Invert the current selection set",
		"Ctrl+I"
	},
	// LC_EDIT_SELECT_BY_NAME
	{
		"Edit.SelectByName",
		"Select by Name...",
		"Select objects by name",
		""
	},
	// LC_EDIT_LOCK_X
	{
		"Edit.LockX",
		"Lock X",
		"Prevents movement and rotation along the X axis",
		""
	},
	// LC_EDIT_LOCK_Y
	{
		"Edit.LockY",
		"Lock Y",
		"Prevents movement and rotation along the Y axis",
		""
	},
	// LC_EDIT_LOCK_Z
	{
		"Edit.LockZ",
		"Lock Z",
		"Prevents movement and rotation along the Z axis",
		""
	},
	// LC_EDIT_LOCK_TOGGLE
	{
		"Edit.LockToggle",
		"Lock Toggle",
		"Toggle locked axes",
		""
	},
	// LC_EDIT_LOCK_NONE
	{
		"Edit.LockNone",
		"Unlock All",
		"Allows movement and rotation in all directions",
		""
	},
	// LC_EDIT_SNAP_X
	{
		"Edit.SnapX",
		"Snap X",
		"Snap movement along the X axis to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_Y
	{
		"Edit.SnapY",
		"Snap Y",
		"Snap movement along the Y axis to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_Z
	{
		"Edit.SnapZ",
		"Snap Z",
		"Snap movement along the Z axis to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_TOGGLE
	{
		"Edit.SnapToggle",
		"Snap Toggle",
		"Toggle snap axes",
		""
	},
	// LC_EDIT_SNAP_NONE
	{
		"Edit.SnapNone",
		"Snap None",
		"Disable snapping along all axes",
		""
	},
	// LC_EDIT_SNAP_ALL
	{
		"Edit.SnapAll",
		"Snap All",
		"Snap movement along all axes to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_ANGLE
	{
		"Edit.SnapAngle",
		"Snap Angle Toggle",
		"Snap rotations to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_MOVE_XY0
	{
		"Edit.SnapMoveXY0",
		"None",
		"Do not snap movement along the XY plane",
		"0"
	},
	// LC_EDIT_SNAP_MOVE_XY1
	{
		"Edit.SnapMoveXY1",
		"1/20 Stud",
		"Snap movement along the XY plane to 1/20 stud",
		"1"
	},
	// LC_EDIT_SNAP_MOVE_XY2
	{
		"Edit.SnapMoveXY2",
		"1/4 Stud",
		"Snap movement along the XY plane to 1/4 stud",
		"2"
	},
	// LC_EDIT_SNAP_MOVE_XY3
	{
		"Edit.SnapMoveXY3",
		"1 Flat",
		"Snap movement along the XY plane to 1 flat",
		"3"
	},
	// LC_EDIT_SNAP_MOVE_XY4
	{
		"Edit.SnapMoveXY4",
		"1/2 Stud",
		"Snap movement along the XY plane to 1/2 stud",
		"4"
	},
	// LC_EDIT_SNAP_MOVE_XY5
	{
		"Edit.SnapMoveXY5",
		"1 Stud",
		"Snap movement along the XY plane to 1 stud",
		"5"
	},
	// LC_EDIT_SNAP_MOVE_XY6
	{
		"Edit.SnapMoveXY6",
		"2 Studs",
		"Snap movement along the XY plane to 2 studs",
		"6"
	},
	// LC_EDIT_SNAP_MOVE_XY7
	{
		"Edit.SnapMoveXY7",
		"3 Studs",
		"Snap movement along the XY plane to 3 studs",
		"7"
	},
	// LC_EDIT_SNAP_MOVE_XY8
	{
		"Edit.SnapMoveXY8",
		"4 Studs",
		"Snap movement along the XY plane to 4 studs",
		"8"
	},
	// LC_EDIT_SNAP_MOVE_XY9
	{
		"Edit.SnapMoveXY9",
		"8 Studs",
		"Snap movement along the XY plane to 8 studs",
		"9"
	},
	// LC_EDIT_SNAP_MOVE_Z0
	{
		"Edit.SnapMoveZ0",
		"None",
		"Do not snap movement along the Z axis",
		"Ctrl+Shift+0"
	},
	// LC_EDIT_SNAP_MOVE_Z1
	{
		"Edit.SnapMoveZ1",
		"1/20 Stud",
		"Snap movement along the Z axis to 1/20 stud",
		"Ctrl+Shift+1"
	},
	// LC_EDIT_SNAP_MOVE_Z2
	{
		"Edit.SnapMoveZ2",
		"1/4 Stud",
		"Snap movement along the Z axis to 1/4 stud",
		"Ctrl+Shift+2"
	},
	// LC_EDIT_SNAP_MOVE_Z3
	{
		"Edit.SnapMoveZ3",
		"1 Flat",
		"Snap movement along the Z axis to 1 flat",
		"Ctrl+Shift+3"
	},
	// LC_EDIT_SNAP_MOVE_Z4
	{
		"Edit.SnapMoveZ4",
		"1/2 Stud",
		"Snap movement along the Z axis to 1/2 stud",
		"Ctrl+Shift+4"
	},
	// LC_EDIT_SNAP_MOVE_Z5
	{
		"Edit.SnapMoveZ5",
		"1 Stud",
		"Snap movement along the Z axis to 1 stud",
		"Ctrl+Shift+5"
	},
	// LC_EDIT_SNAP_MOVE_Z6
	{
		"Edit.SnapMoveZ6",
		"1 Brick",
		"Snap movement along the Z axis to 1 brick",
		"Ctrl+Shift+6"
	},
	// LC_EDIT_SNAP_MOVE_Z7
	{
		"Edit.SnapMoveZ7",
		"2 Bricks",
		"Snap movement along the Z axis to 2 bricks",
		"Ctrl+Shift+7"
	},
	// LC_EDIT_SNAP_MOVE_Z8
	{
		"Edit.SnapMoveZ8",
		"4 Bricks",
		"Snap movement along the Z axis to 4 bricks",
		"Ctrl+Shift+8"
	},
	// LC_EDIT_SNAP_MOVE_Z9
	{
		"Edit.SnapMoveZ9",
		"8 Bricks",
		"Snap movement along the Z axis to 8 bricks",
		"Ctrl+Shift+9"
	},
	// LC_EDIT_SNAP_ANGLE0
	{
		"Edit.SnapAngle0",
		"None",
		"Do not snap rotations",
		"Shift+0"
	},
	// LC_EDIT_SNAP_ANGLE1
	{
		"Edit.SnapAngle1",
		"1 Degree",
		"Snap rotations to 1 degree",
		"Shift+1"
	},
	// LC_EDIT_SNAP_ANGLE2
	{
		"Edit.SnapAngle2",
		"5 Degrees",
		"Snap rotations to 5 degrees",
		"Shift+2"
	},
	// LC_EDIT_SNAP_ANGLE3
	{
		"Edit.SnapAngle3",
		"10 Degrees",
		"Snap rotations to 10 degrees",
		"Shift+3"
	},
	// LC_EDIT_SNAP_ANGLE4
	{
		"Edit.SnapAngle4",
		"15 Degrees",
		"Snap rotations to 15 degrees",
		"Shift+4"
	},
	// LC_EDIT_SNAP_ANGLE5
	{
		"Edit.SnapAngle5",
		"30 Degrees",
		"Snap rotations to 30 degrees",
		"Shift+5"
	},
	// LC_EDIT_SNAP_ANGLE6
	{
		"Edit.SnapAngle6",
		"45 Degrees",
		"Snap rotations to 45 degrees",
		"Shift+6"
	},
	// LC_EDIT_SNAP_ANGLE7
	{
		"Edit.SnapAngle7",
		"60 Degrees",
		"Snap rotations to 60 degrees",
		"Shift+7"
	},
	// LC_EDIT_SNAP_ANGLE8
	{
		"Edit.SnapAngle8",
		"90 Degrees",
		"Snap rotations to 90 degrees",
		"Shift+8"
	},
	// LC_EDIT_SNAP_ANGLE9
	{
		"Edit.SnapAngle9",
		"180 Degrees",
		"Snap rotations to 180 degrees",
		"Shift+9"
	},
	// LC_EDIT_TRANSFORM
	{
		"Edit.Transform",
		"Transform",
		"Apply transform to selected objects",
		""
	},
	// LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION
	{
		"Edit.TransformAbsoluteTranslation",
		"Absolute Translation",
		"Switch to absolute translation mode when applying transforms",
		""
	},
	// LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION
	{
		"Edit.TransformRelativeTranslation",
		"Relative Translation",
		"Switch to relative translation mode when applying transforms",
		""
	},
	// LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION
	{
		"Edit.TransformAbsoluteRotation",
		"Absolute Rotation",
		"Switch to absolute rotation mode when applying transforms",
		""
	},
	// LC_EDIT_TRANSFORM_RELATIVE_ROTATION
	{
		"Edit.TransformRelativeRotation",
		"Relative Rotation",
		"Switch to relative rotation mode when applying transforms",
		""
	},
	// LC_EDIT_ACTION_INSERT
	{
		"Edit.Tool.Insert",
		"Insert",
		"Add new pieces to the model",
		""
	},
	// LC_EDIT_ACTION_LIGHT
	{
		"Edit.Tool.Light",
		"Light",
		"Add new omni light sources to the model",
		""
	},
	// LC_EDIT_ACTION_SPOTLIGHT
	{
		"Edit.Tool.Spotlight",
		"Spotlight",
		"Add new spotlights to the model",
		""
	},
	// LC_EDIT_ACTION_CAMERA
	{
		"Edit.Tool.Camera",
		"Camera",
		"Create a new camera",
		""
	},
	// LC_EDIT_ACTION_SELECT
	{
		"Edit.Tool.Select",
		"Select",
		"Select objects (hold the CTRL key down or drag the mouse to select multiple objects)",
		"Shift+S"
	},
	// LC_EDIT_ACTION_MOVE
	{
		"Edit.Tool.Move",
		"Move",
		"Move selected objects",
		"Shift+M"
	},
	// LC_EDIT_ACTION_ROTATE
	{
		"Edit.Tool.Rotate",
		"Rotate",
		"Rotate selected pieces",
		"Shift+R"
	},
	// LC_EDIT_ACTION_DELETE
	{
		"Edit.Tool.Delete",
		"Delete",
		"Delete objects",
		"Shift+D"
	},
	// LC_EDIT_ACTION_PAINT
	{
		"Edit.Tool.Paint",
		"Paint",
		"Change piece color",
		"Shift+N"
	},
	// LC_EDIT_ACTION_ZOOM
	{
		"Edit.Tool.Zoom",
		"Zoom",
		"Zoom in or out",
		"Shift+Z"
	},
	// LC_EDIT_ACTION_PAN
	{
		"Edit.Tool.Pan",
		"Pan",
		"Pan the current view",
		"Shift+P"
	},
	// LC_EDIT_ACTION_ROTATE_VIEW
	{
		"Edit.Tool.RotateView",
		"Rotate View",
		"Rotate the current view",
		"Shift+T"
	},
	// LC_EDIT_ACTION_ROLL
	{
		"Edit.Tool.Roll",
		"Roll",
		"Roll the current view",
		"Shift+L"
	},
	// LC_EDIT_ACTION_ZOOM_REGION
	{
		"Edit.Tool.ZoomRegion",
		"Zoom Region",
		"Zoom into a region of the screen",
		""
	},
	// LC_VIEW_PREFERENCES
	{
		"View.Preferences",
		"Preferences...",
		"Change program settings",
		""
	},
	// LC_VIEW_ZOOM_IN
	{
		"View.ZoomIn",
		"Zoom In",
		"Zoom in",
		""
	},
	// LC_VIEW_ZOOM_OUT
	{
		"View.ZoomOut",
		"Zoom Out",
		"Zoom out",
		""
	},
	// LC_VIEW_ZOOM_EXTENTS
	{
		"View.ZoomExtents",
		"Zoom Extents",
		"Fit all pieces in current the view (hold the CTRL key down to zoom all views)",
		""
	},
	// LC_VIEW_VIEWPOINT_FRONT
	{
		"View.Viewpoint.Front",
		"Front",
		"View model from the front",
		"F"
	},
	// LC_VIEW_VIEWPOINT_BACK
	{
		"View.Viewpoint.Back",
		"Back",
		"View model from the back",
		"B"
	},
	// LC_VIEW_VIEWPOINT_TOP
	{
		"View.Viewpoint.Top",
		"Top",
		"View model from the top",
		"T"
	},
	// LC_VIEW_VIEWPOINT_BOTTOM
	{
		"View.Viewpoint.Bottom",
		"Bottom",
		"View model from the bottom",
		"B"
	},
	// LC_VIEW_VIEWPOINT_LEFT
	{
		"View.Viewpoint.Left",
		"Left",
		"View model from the left",
		"L"
	},
	// LC_VIEW_VIEWPOINT_RIGHT
	{
		"View.Viewpoint.Right",
		"Right",
		"View model from the right",
		"R"
	},
	// LC_VIEW_VIEWPOINT_HOME
	{
		"View.Viewpoint.Home",
		"Home",
		"View model from the default position",
		"H"
	},
	// LC_VIEW_CAMERA_NONE
	{
		"View.Camera.None",
		"None",
		"Do not use a camera",
		""
	},
	// LC_VIEW_CAMERA1
	{
		"View.Camera.01",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA2
	{
		"View.Camera.02",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA3
	{
		"View.Camera.03",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA4
	{
		"View.Camera.04",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA5
	{
		"View.Camera.05",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA6
	{
		"View.Camera.06",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA7
	{
		"View.Camera.07",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA8
	{
		"View.Camera.08",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA9
	{
		"View.Camera.09",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA10
	{
		"View.Camera.10",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA11
	{
		"View.Camera.11",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA12
	{
		"View.Camera.12",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA13
	{
		"View.Camera.13",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA14
	{
		"View.Camera.14",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA15
	{
		"View.Camera.15",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA16
	{
		"View.Camera.16",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA_RESET
	{
		"View.Camera.Reset",
		"Reset",
		"Reset views to their default positions",
		""
	},
	// LC_VIEW_TIME_FIRST
	{
		"View.Time.First",
		"First",
		"Go to the first step of the model",
		""
	},
	// LC_VIEW_TIME_PREVIOUS
	{
		"View.Time.Previous",
		"Previous",
		"Go to the previous step",
		""
	},
	// LC_VIEW_TIME_NEXT
	{
		"View.Time.Next",
		"Next",
		"Go to the next step",
		""
	},
	// LC_VIEW_TIME_LAST
	{
		"View.Time.Last",
		"Last",
		"Go to the last step of the model",
		""
	},
	// LC_VIEW_TIME_STOP
	{
		"View.Time.Stop",
		"Stop",
		"Stop playing animation",
		""
	},
	// LC_VIEW_TIME_PLAY
	{
		"View.Time.Play",
		"Play",
		"Play animation",
		""
	},
	// LC_VIEW_TIME_INSERT
	{
		"View.Time.Insert",
		"Insert",
		"Insert new step",
		""
	},
	// LC_VIEW_TIME_DELETE
	{
		"View.Time.Delete",
		"Delete",
		"Delete current step",
		""
	},
	// LC_VIEW_TIME_ANIMATION
	{
		"View.Time.Animation",
		"Animation",
		"Toggle between animation and instruction mode",
		""
	},
	// LC_VIEW_TIME_ADD_KEYS
	{
		"View.Time.AddKeys",
		"Add Keys",
		"Toggle adding new animation keys",
		""
	},
	// LC_PIECE_INSERT
	{
		"Piece.Insert",
		"Insert",
		"Add a new piece to the model",
		"Insert"
	},
	// LC_PIECE_DELETE
	{
		"Piece.Delete",
		"Delete",
		"Delete selected objects",
		"Delete"
	},
	// LC_PIECE_MINIFIG_WIZARD
	{
		"Piece.MinifigWizard",
		"Minifig Wizard...",
		"Add a new minifig to the model",
		""
	},
	// LC_PIECE_ARRAY
	{
		"Piece.Array",
		"Array...",
		"Make copies of the selected pieces",
		""
	},
	// LC_PIECE_COPY_KEYS
	{
		"Piece.CopyKeys",
		"Copy Keys",
		"Copy keys between animation and instruction modes",
		""
	},
	// LC_PIECE_GROUP
	{
		"Piece.Group",
		"Group...",
		"Group selected pieces together",
		"Ctrl+G"
	},
	// LC_PIECE_UNGROUP
	{
		"Piece.Ungroup",
		"Ungroup",
		"Ungroup selected group",
		"Ctrl+U"
	},
	// LC_PIECE_GROUP_ADD
	{
		"Piece.GroupAdd",
		"Add to Group",
		"Add focused piece to selected group",
		""
	},
	// LC_PIECE_GROUP_REMOVE
	{
		"Piece.GroupRemove",
		"Remove from Group",
		"Remove focused piece from group",
		""
	},
	// LC_PIECE_GROUP_EDIT
	{
		"Piece.GroupEdit",
		"Edit Groups...",
		"Edit groups",
		""
	},
	// LC_PIECE_HIDE_SELECTED
	{
		"Piece.HideSelected",
		"Hide Selected",
		"Hide selected objects",
		"Ctrl+H"
	},
	// LC_PIECE_HIDE_UNSELECTED
	{
		"Piece.HideUnselected",
		"Hide Unselected",
		"Hide objects that are not selected",
		""
	},
	// LC_PIECE_UNHIDE_ALL
	{
		"Piece.UnhideAll",
		"Unhide All",
		"Show all hidden objects",
		""
	},
	// LC_PIECE_SHOW_EARLIER
	{
		"Piece.ShowEarlier",
		"Show Earlier",
		"Show selected pieces one step earlier",
		""
	},
	// LC_PIECE_SHOW_LATER
	{
		"Piece.ShowLater",
		"Show Later",
		"Show selected pieces one step later",
		""
	},
	// LC_HELP_HOMEPAGE
	{
		"Help.HomePage",
		"LeoCAD &Home Page",
		"Open LeoCAD's home page on the internet using your default web browser",
		""
	},
	// LC_HELP_EMAIL
	{
		"Help.Email",
		"Send Support &E-Mail",
		"Send an e-mail message for help or support using your default e-mail client",
		""
	},
	// LC_HELP_UPDATES
	{
		"Help.Updates",
		"Check for &Updates...",
		"Check if a newer LeoCAD version or parts library has been released",
		""
	},
	// LC_HELP_ABOUT
	{
		"Help.About",
		"&About...",
		"Display program version number and system information",
		""
	}
};

LC_CASSERT(sizeof(gActions)/sizeof(gActions[0]) == LC_NUM_COMMANDS);

#include "lc_global.h"
#include "lc_commands.h"
#include "system.h"

lcCommand gCommands[LC_NUM_COMMANDS] =
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
		"Export a list of parts used in BrickLink XML format",
		""
	},
	// LC_FILE_EXPORT_CSV
	{
		"File.Export.CSV",
		"&CSV...",
		"Export a list of parts used in comma delimited file format",
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
		"Print Pre&view...",
		"Display how the project would look if printed",
		""
	},
	// LC_FILE_PRINT_BOM
	{
		"File.PrintBOM",
		"Print &Bill of Materials...",
		"Print a list of parts used",
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
		"&Undo",
		"Undo the last action",
		"Ctrl+Z"
	},
	// LC_EDIT_REDO
	{
		"Edit.Redo",
		"&Redo",
		"Redo the previously undone action",
		"Ctrl+Y"
	},
	// LC_EDIT_CUT
	{
		"Edit.Cut",
		"Cu&t",
		"Cut the selection and put it on the Clipboard",
		"Ctrl+X"
	},
	// LC_EDIT_COPY
	{
		"Edit.Copy",
		"&Copy",
		"Copy the selection and put it on the Clipboard",
		"Ctrl+C"
	},
	// LC_EDIT_PASTE
	{
		"Edit.Paste",
		"&Paste",
		"Insert Clipboard contents",
		"Ctrl+V"
	},
	// LC_EDIT_FIND
	{
		"Edit.Find",
		"&Find...",
		"Find object",
		"Ctrl+F",
	},
	// LC_EDIT_FIND_NEXT
	{
		"Edit.FindNext",
		"Find Ne&xt",
		"Find next object",
		"F3",
	},
	// LC_EDIT_FIND_PREVIOUS
	{
		"Edit.FindPrevious",
		"Find Pre&vious",
		"Find object",
		"Shift+F3",
	},
	// LC_EDIT_SELECT_ALL
	{
		"Edit.SelectAll",
		"Select &All",
		"Select all parts in the project",
		"Ctrl+A"
	},
	// LC_EDIT_SELECT_NONE
	{
		"Edit.SelectNone",
		"Select &None",
		"De-select everything",
		""
	},
	// LC_EDIT_SELECT_INVERT
	{
		"Edit.SelectInvert",
		"Select &Invert",
		"Invert the current selection set",
		"Ctrl+I"
	},
	// LC_EDIT_SELECT_BY_NAME
	{
		"Edit.SelectByName",
		"Select by Na&me...",
		"Select objects by name",
		""
	},
	// LC_EDIT_LOCK_X
	{
		"Edit.Lock.LockX",
		"Lock X",
		"Prevents movement and rotation along the X axis",
		""
	},
	// LC_EDIT_LOCK_Y
	{
		"Edit.Lock.LockY",
		"Lock Y",
		"Prevents movement and rotation along the Y axis",
		""
	},
	// LC_EDIT_LOCK_Z
	{
		"Edit.Lock.LockZ",
		"Lock Z",
		"Prevents movement and rotation along the Z axis",
		""
	},
	// LC_EDIT_LOCK_TOGGLE
	{
		"Edit.Lock.Toggle",
		"Lock Toggle",
		"Toggle locked axes",
		""
	},
	// LC_EDIT_LOCK_NONE
	{
		"Edit.Lock.None",
		"Unlock All",
		"Allows movement and rotation in all directions",
		""
	},
	// LC_EDIT_SNAP_X
	{
		"Edit.Snap.SnapX",
		"Snap X",
		"Snap movement along the X axis to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_Y
	{
		"Edit.Snap.SnapY",
		"Snap Y",
		"Snap movement along the Y axis to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_Z
	{
		"Edit.Snap.SnapZ",
		"Snap Z",
		"Snap movement along the Z axis to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_TOGGLE
	{
		"Edit.Snap.Toggle",
		"Snap Toggle",
		"Toggle snap axes",
		""
	},
	// LC_EDIT_SNAP_NONE
	{
		"Edit.Snap.None",
		"Snap None",
		"Disable snapping along all axes",
		""
	},
	// LC_EDIT_SNAP_ALL
	{
		"Edit.Snap.All",
		"Snap All",
		"Snap movement along all axes to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_ANGLE
	{
		"Edit.SnapAngle.Toggle",
		"Snap Angle Toggle",
		"Snap rotations to fixed intervals",
		""
	},
	// LC_EDIT_SNAP_MOVE_XY0
	{
		"Edit.SnapMove.XY0",
		"None",
		"Do not snap movement along the XY plane",
		"0"
	},
	// LC_EDIT_SNAP_MOVE_XY1
	{
		"Edit.SnapMove.XY1",
		"1/20 Stud",
		"Snap movement along the XY plane to 1/20 stud",
		"1"
	},
	// LC_EDIT_SNAP_MOVE_XY2
	{
		"Edit.SnapMove.XY2",
		"1/4 Stud",
		"Snap movement along the XY plane to 1/4 stud",
		"2"
	},
	// LC_EDIT_SNAP_MOVE_XY3
	{
		"Edit.SnapMove.XY3",
		"1 Flat",
		"Snap movement along the XY plane to 1 flat",
		"3"
	},
	// LC_EDIT_SNAP_MOVE_XY4
	{
		"Edit.SnapMove.XY4",
		"1/2 Stud",
		"Snap movement along the XY plane to 1/2 stud",
		"4"
	},
	// LC_EDIT_SNAP_MOVE_XY5
	{
		"Edit.SnapMove.XY5",
		"1 Stud",
		"Snap movement along the XY plane to 1 stud",
		"5"
	},
	// LC_EDIT_SNAP_MOVE_XY6
	{
		"Edit.SnapMove.XY6",
		"2 Studs",
		"Snap movement along the XY plane to 2 studs",
		"6"
	},
	// LC_EDIT_SNAP_MOVE_XY7
	{
		"Edit.SnapMove.XY7",
		"3 Studs",
		"Snap movement along the XY plane to 3 studs",
		"7"
	},
	// LC_EDIT_SNAP_MOVE_XY8
	{
		"Edit.SnapMove.XY8",
		"4 Studs",
		"Snap movement along the XY plane to 4 studs",
		"8"
	},
	// LC_EDIT_SNAP_MOVE_XY9
	{
		"Edit.SnapMove.XY9",
		"8 Studs",
		"Snap movement along the XY plane to 8 studs",
		"9"
	},
	// LC_EDIT_SNAP_MOVE_Z0
	{
		"Edit.SnapMove.Z0",
		"None",
		"Do not snap movement along the Z axis",
		"Ctrl+Shift+0"
	},
	// LC_EDIT_SNAP_MOVE_Z1
	{
		"Edit.SnapMove.Z1",
		"1/20 Stud",
		"Snap movement along the Z axis to 1/20 stud",
		"Ctrl+Shift+1"
	},
	// LC_EDIT_SNAP_MOVE_Z2
	{
		"Edit.SnapMove.Z2",
		"1/4 Stud",
		"Snap movement along the Z axis to 1/4 stud",
		"Ctrl+Shift+2"
	},
	// LC_EDIT_SNAP_MOVE_Z3
	{
		"Edit.SnapMove.Z3",
		"1 Flat",
		"Snap movement along the Z axis to 1 flat",
		"Ctrl+Shift+3"
	},
	// LC_EDIT_SNAP_MOVE_Z4
	{
		"Edit.SnapMove.Z4",
		"1/2 Stud",
		"Snap movement along the Z axis to 1/2 stud",
		"Ctrl+Shift+4"
	},
	// LC_EDIT_SNAP_MOVE_Z5
	{
		"Edit.SnapMove.Z5",
		"1 Stud",
		"Snap movement along the Z axis to 1 stud",
		"Ctrl+Shift+5"
	},
	// LC_EDIT_SNAP_MOVE_Z6
	{
		"Edit.SnapMove.Z6",
		"1 Brick",
		"Snap movement along the Z axis to 1 brick",
		"Ctrl+Shift+6"
	},
	// LC_EDIT_SNAP_MOVE_Z7
	{
		"Edit.SnapMove.Z7",
		"2 Bricks",
		"Snap movement along the Z axis to 2 bricks",
		"Ctrl+Shift+7"
	},
	// LC_EDIT_SNAP_MOVE_Z8
	{
		"Edit.SnapMove.Z8",
		"4 Bricks",
		"Snap movement along the Z axis to 4 bricks",
		"Ctrl+Shift+8"
	},
	// LC_EDIT_SNAP_MOVE_Z9
	{
		"Edit.SnapMove.Z9",
		"8 Bricks",
		"Snap movement along the Z axis to 8 bricks",
		"Ctrl+Shift+9"
	},
	// LC_EDIT_SNAP_ANGLE0
	{
		"Edit.SnapAngle.Angle0",
		"None",
		"Do not snap rotations",
		"Shift+0"
	},
	// LC_EDIT_SNAP_ANGLE1
	{
		"Edit.SnapAngle.Angle1",
		"1 Degree",
		"Snap rotations to 1 degree",
		"Shift+1"
	},
	// LC_EDIT_SNAP_ANGLE2
	{
		"Edit.SnapAngle.Angle2",
		"5 Degrees",
		"Snap rotations to 5 degrees",
		"Shift+2"
	},
	// LC_EDIT_SNAP_ANGLE3
	{
		"Edit.SnapAngle.Angle3",
		"10 Degrees",
		"Snap rotations to 10 degrees",
		"Shift+3"
	},
	// LC_EDIT_SNAP_ANGLE4
	{
		"Edit.SnapAngle.Angle4",
		"15 Degrees",
		"Snap rotations to 15 degrees",
		"Shift+4"
	},
	// LC_EDIT_SNAP_ANGLE5
	{
		"Edit.SnapAngle.Angle5",
		"30 Degrees",
		"Snap rotations to 30 degrees",
		"Shift+5"
	},
	// LC_EDIT_SNAP_ANGLE6
	{
		"Edit.SnapAngle.Angle6",
		"45 Degrees",
		"Snap rotations to 45 degrees",
		"Shift+6"
	},
	// LC_EDIT_SNAP_ANGLE7
	{
		"Edit.SnapAngle.Angle7",
		"60 Degrees",
		"Snap rotations to 60 degrees",
		"Shift+7"
	},
	// LC_EDIT_SNAP_ANGLE8
	{
		"Edit.SnapAngle.Angle8",
		"90 Degrees",
		"Snap rotations to 90 degrees",
		"Shift+8"
	},
	// LC_EDIT_SNAP_ANGLE9
	{
		"Edit.SnapAngle.Angle9",
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
		"Add new parts to the model",
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
		"Rotate selected parts",
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
		"Change part color",
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
	// LC_EDIT_CANCEL
	{
		"Edit.Cancel",
		"Cancel Action",
		"Cancel current mouse action",
		"Esc"
	},
	// LC_VIEW_PREFERENCES
	{
		"View.Preferences",
		"P&references...",
		"Change program settings",
		""
	},
	// LC_VIEW_ZOOM_IN
	{
		"View.ZoomIn",
		"Zoom In",
		"Zoom in",
		"+"
	},
	// LC_VIEW_ZOOM_OUT
	{
		"View.ZoomOut",
		"Zoom Out",
		"Zoom out",
		"-"
	},
	// LC_VIEW_ZOOM_EXTENTS
	{
		"View.ZoomExtents",
		"Zoom E&xtents",
		"Fit all parts in current the view (hold the CTRL key down to zoom all views)",
		""
	},
	// LC_VIEW_VIEWPOINT_FRONT
	{
		"View.Viewpoint.Front",
		"&Front",
		"View model from the front",
		"F"
	},
	// LC_VIEW_VIEWPOINT_BACK
	{
		"View.Viewpoint.Back",
		"&Back",
		"View model from the back",
		"B"
	},
	// LC_VIEW_VIEWPOINT_TOP
	{
		"View.Viewpoint.Top",
		"&Top",
		"View model from the top",
		"T"
	},
	// LC_VIEW_VIEWPOINT_BOTTOM
	{
		"View.Viewpoint.Bottom",
		"B&ottom",
		"View model from the bottom",
		"O"
	},
	// LC_VIEW_VIEWPOINT_LEFT
	{
		"View.Viewpoint.Left",
		"&Left",
		"View model from the left",
		"L"
	},
	// LC_VIEW_VIEWPOINT_RIGHT
	{
		"View.Viewpoint.Right",
		"&Right",
		"View model from the right",
		"R"
	},
	// LC_VIEW_VIEWPOINT_HOME
	{
		"View.Viewpoint.Home",
		"&Home",
		"View model from the default position",
		"H"
	},
	// LC_VIEW_CAMERA_NONE
	{
		"View.Cameras.None",
		"None",
		"Do not use a camera",
		""
	},
	// LC_VIEW_CAMERA1
	{
		"View.Cameras.Camera01",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA2
	{
		"View.Cameras.Camera02",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA3
	{
		"View.Cameras.Camera03",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA4
	{
		"View.Cameras.Camera04",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA5
	{
		"View.Cameras.Camera05",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA6
	{
		"View.Cameras.Camera06",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA7
	{
		"View.Cameras.Camera07",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA8
	{
		"View.Cameras.Camera08",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA9
	{
		"View.Cameras.Camera09",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA10
	{
		"View.Cameras.Camera10",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA11
	{
		"View.Cameras.Camera11",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA12
	{
		"View.Cameras.Camera12",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA13
	{
		"View.Cameras.Camera13",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA14
	{
		"View.Cameras.Camera14",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA15
	{
		"View.Cameras.Camera15",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA16
	{
		"View.Cameras.Camera16",
		"Camera",
		"Use this camera",
		""
	},
	// LC_VIEW_CAMERA_RESET
	{
		"View.Cameras.Reset",
		"Reset",
		"Reset views to their default positions",
		""
	},
	// LC_VIEW_TIME_FIRST
	{
		"View.Time.First",
		"First",
		"Go to the first step of the model",
		"Alt+Up"
	},
	// LC_VIEW_TIME_PREVIOUS
	{
		"View.Time.Previous",
		"Previous",
		"Go to the previous step",
		"Alt+Left"
	},
	// LC_VIEW_TIME_NEXT
	{
		"View.Time.Next",
		"Next",
		"Go to the next step",
		"Alt+Right"
	},
	// LC_VIEW_TIME_LAST
	{
		"View.Time.Last",
		"Last",
		"Go to the last step of the model",
		"Alt+Down"
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
	// LC_VIEW_SPLIT_HORIZONTAL
	{
		"View.SplitHorizontal",
		"Split &Horizontal",
		"Split the current view horizontally",
		""
	},
	// LC_VIEW_SPLIT_VERTICAL
	{
		"View.SplitVertical",
		"Split &Vertical",
		"Split the current view vertically",
		""
	},
	// LC_VIEW_REMOVE_VIEW
	{
		"View.RemoveView",
		"Re&move View",
		"Remove the current view",
		""
	},
	// LC_VIEW_RESET_VIEWS
	{
		"View.ResetViews",
		"Rese&t Views",
		"Reset all views",
		""
	},
	// LC_VIEW_FULLSCREEN
	{
		"View.FullScreen",
		"&Full Screen",
		"Toggle fullscreen mode",
		""
	},
	// LC_PIECE_INSERT
	{
		"Part.Insert",
		"&Insert",
		"Add a new part to the model",
		"Insert"
	},
	// LC_PIECE_DELETE
	{
		"Part.Delete",
		"&Delete",
		"Delete selected objects",
		"Delete"
	},
	// LC_PIECE_MOVE_PLUSX
	{
		"Part.Move.PlusX",
		"Move +X",
		"Move selected objects along the X axis",
		"Down"
	},
	// LC_PIECE_MOVE_MINUSX
	{
		"Part.Move.MinusX",
		"Move -X",
		"Move selected objects along the X axis",
		"Up"
	},
	// LC_PIECE_MOVE_PLUSY
	{
		"Part.Move.PlusY",
		"Move +Y",
		"Move selected objects along the Y axis",
		"Right"
	},
	// LC_PIECE_MOVE_MINUSY
	{
		"Part.Move.MinusY",
		"Move -Y",
		"Move selected objects along the Y axis",
		"Left"
	},
	// LC_PIECE_MOVE_PLUSZ
	{
		"Part.Move.PlusZ",
		"Move +Z",
		"Move selected objects along the Z axis",
		"PgUp"
	},
	// LC_PIECE_MOVE_MINUSZ
	{
		"Part.Move.MinusZ",
		"Move -Z",
		"Move selected objects along the Z axis",
		"PgDown"
	},
	// LC_PIECE_ROTATE_PLUSX
	{
		"Part.Rotate.PlusX",
		"Rotate +X",
		"Rotate selected objects along the X axis",
		"Shift+Down"
	},
	// LC_PIECE_ROTATE_MINUSX
	{
		"Part.Rotate.MinusX",
		"Rotate -X",
		"Rotate selected objects along the X axis",
		"Shift+Up"
	},
	// LC_PIECE_ROTATE_PLUSY
	{
		"Part.Rotate.PlusY",
		"Rotate +Y",
		"Rotate selected objects along the Y axis",
		"Shift+Right"
	},
	// LC_PIECE_ROTATE_MINUSY
	{
		"Part.Rotate.MinusY",
		"Rotate -Y",
		"Rotate selected objects along the Y axis",
		"Shift+Left"
	},
	// LC_PIECE_ROTATE_PLUSZ
	{
		"Part.Rotate.PlusZ",
		"Rotate +Z",
		"Rotate selected objects along the Z axis",
		"Shift+PgUp"
	},
	// LC_PIECE_ROTATE_MINUSZ
	{
		"Part.Rotate.MinusZ",
		"Rotate -Z",
		"Rotate selected objects along the Z axis",
		"Shift+PgDown"
	},
	// LC_PIECE_MINIFIG_WIZARD
	{
		"Part.MinifigWizard",
		"Minifig &Wizard...",
		"Add a new minifig to the model",
		""
	},
	// LC_PIECE_ARRAY
	{
		"Part.Array",
		"A&rray...",
		"Make copies of the selected parts",
		""
	},
	// LC_PIECE_COPY_KEYS
	{
		"Part.CopyKeys",
		"Copy Keys",
		"Copy keys between animation and instruction modes",
		""
	},
	// LC_PIECE_GROUP
	{
		"Part.Group",
		"&Group...",
		"Group selected parts together",
		"Ctrl+G"
	},
	// LC_PIECE_UNGROUP
	{
		"Part.Ungroup",
		"&Ungroup",
		"Ungroup selected group",
		"Ctrl+U"
	},
	// LC_PIECE_GROUP_ADD
	{
		"Part.GroupAdd",
		"&Add to Group",
		"Add focused part to selected group",
		""
	},
	// LC_PIECE_GROUP_REMOVE
	{
		"Part.GroupRemove",
		"Re&move from Group",
		"Remove focused part from group",
		""
	},
	// LC_PIECE_GROUP_EDIT
	{
		"Part.GroupEdit",
		"&Edit Groups...",
		"Edit groups",
		""
	},
	// LC_PIECE_HIDE_SELECTED
	{
		"Part.HideSelected",
		"&Hide Selected",
		"Hide selected objects",
		"Ctrl+H"
	},
	// LC_PIECE_HIDE_UNSELECTED
	{
		"Part.HideUnselected",
		"Hide &Unselected",
		"Hide objects that are not selected",
		""
	},
	// LC_PIECE_UNHIDE_ALL
	{
		"Part.UnhideAll",
		"U&nhide All",
		"Show all hidden objects",
		""
	},
	// LC_PIECE_SHOW_EARLIER
	{
		"Part.ShowEarlier",
		"Show Earlier",
		"Show selected parts one step earlier",
		""
	},
	// LC_PIECE_SHOW_LATER
	{
		"Part.ShowLater",
		"Show Later",
		"Show selected parts one step later",
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

LC_CASSERT(sizeof(gCommands)/sizeof(gCommands[0]) == LC_NUM_COMMANDS);

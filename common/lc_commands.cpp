#include "lc_global.h"
#include "lc_commands.h"
#include "system.h"

lcCommand gCommands[LC_NUM_COMMANDS] =
{
	// LC_FILE_NEW
	{
		"File.New",
		QT_TRANSLATE_NOOP("Menu", "&New"),
		QT_TRANSLATE_NOOP("Status", "Create a new project"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+N")
	},
	// LC_FILE_OPEN
	{
		"File.Open",
		QT_TRANSLATE_NOOP("Menu", "&Open..."),
		QT_TRANSLATE_NOOP("Status", "Open an existing project"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+O")
	},
	// LC_FILE_MERGE
	{
		"File.Merge",
		QT_TRANSLATE_NOOP("Menu", "&Merge..."),
		QT_TRANSLATE_NOOP("Status", "Merge the contents of another project with the current one"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_SAVE
	{
		"File.Save",
		QT_TRANSLATE_NOOP("Menu", "&Save"),
		QT_TRANSLATE_NOOP("Status", "Save the active project"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+S")
	},
	// LC_FILE_SAVEAS
	{
		"File.SaveAs",
		QT_TRANSLATE_NOOP("Menu", "Save &As..."),
		QT_TRANSLATE_NOOP("Status", "Save the active project with a new name"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_SAVE_IMAGE
	{
		"File.SaveImage",
		QT_TRANSLATE_NOOP("Menu", "Save &Image..."),
		QT_TRANSLATE_NOOP("Status", "Save a picture of the current view"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_EXPORT_3DS
	{
		"File.Export.3DS",
		QT_TRANSLATE_NOOP("Menu", "3D &Studio..."),
		QT_TRANSLATE_NOOP("Status", "Export the project in 3D Studio 3DS format"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_EXPORT_HTML
	{
		"File.Export.HTML",
		QT_TRANSLATE_NOOP("Menu", "&HTML..."),
		QT_TRANSLATE_NOOP("Status", "Create an HTML page for this project"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_EXPORT_BRICKLINK
	{
		"File.Export.BrickLink",
		QT_TRANSLATE_NOOP("Menu", "&BrickLink..."),
		QT_TRANSLATE_NOOP("Status", "Export a list of parts used in BrickLink XML format"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_EXPORT_CSV
	{
		"File.Export.CSV",
		QT_TRANSLATE_NOOP("Menu", "&CSV..."),
		QT_TRANSLATE_NOOP("Status", "Export a list of parts used in comma delimited file format"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_EXPORT_POVRAY
	{
		"File.Export.POVRay",
		QT_TRANSLATE_NOOP("Menu", "&POV-Ray..."),
		QT_TRANSLATE_NOOP("Status", "Export the project in POV-Ray format"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_EXPORT_WAVEFRONT
	{
		"File.Export.Wavefront",
		QT_TRANSLATE_NOOP("Menu", "&Wavefront..."),
		QT_TRANSLATE_NOOP("Status", "Export the project in Wavefront OBJ format"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_PROPERTIES
	{
		"File.Properties",
		QT_TRANSLATE_NOOP("Menu", "Prope&rties..."),
		QT_TRANSLATE_NOOP("Status", "Display project properties"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_PRINT
	{
		"File.Print",
		QT_TRANSLATE_NOOP("Menu", "&Print..."),
		QT_TRANSLATE_NOOP("Status", "Print the active project"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_PRINT_PREVIEW
	{
		"File.PrintPreview",
		QT_TRANSLATE_NOOP("Menu", "Print Pre&view..."),
		QT_TRANSLATE_NOOP("Status", "Display how the project would look if printed"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_PRINT_BOM
	{
		"File.PrintBOM",
		QT_TRANSLATE_NOOP("Menu", "Print &Bill of Materials..."),
		QT_TRANSLATE_NOOP("Status", "Print a list of parts used"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_RECENT1
	{
		"File.Recent1",
		QT_TRANSLATE_NOOP("Menu", "&Recent1"),
		QT_TRANSLATE_NOOP("Status", "Open this document"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_RECENT2
	{
		"File.Recent2",
		QT_TRANSLATE_NOOP("Menu", "&Recent2"),
		QT_TRANSLATE_NOOP("Status", "Open this document"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_RECENT3
	{
		"File.Recent3",
		QT_TRANSLATE_NOOP("Menu", "&Recent3"),
		QT_TRANSLATE_NOOP("Status", "Open this document"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_RECENT4
	{
		"File.Recent4",
		QT_TRANSLATE_NOOP("Menu", "&Recent4"),
		QT_TRANSLATE_NOOP("Status", "Open this document"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_FILE_EXIT
	{
		"File.Exit",
		QT_TRANSLATE_NOOP("Menu", "E&xit"),
		QT_TRANSLATE_NOOP("Status", "Quit the application; prompts to save project"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_UNDO
	{
		"Edit.Undo",
		QT_TRANSLATE_NOOP("Menu", "&Undo"),
		QT_TRANSLATE_NOOP("Status", "Undo the last action"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Z")
	},
	// LC_EDIT_REDO
	{
		"Edit.Redo",
		QT_TRANSLATE_NOOP("Menu", "&Redo"),
		QT_TRANSLATE_NOOP("Status", "Redo the previously undone action"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Y")
	},
	// LC_EDIT_CUT
	{
		"Edit.Cut",
		QT_TRANSLATE_NOOP("Menu", "Cu&t"),
		QT_TRANSLATE_NOOP("Status", "Cut the selection and put it on the Clipboard"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+X")
	},
	// LC_EDIT_COPY
	{
		"Edit.Copy",
		QT_TRANSLATE_NOOP("Menu", "&Copy"),
		QT_TRANSLATE_NOOP("Status", "Copy the selection and put it on the Clipboard"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+C")
	},
	// LC_EDIT_PASTE
	{
		"Edit.Paste",
		QT_TRANSLATE_NOOP("Menu", "&Paste"),
		QT_TRANSLATE_NOOP("Status", "Insert Clipboard contents"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+V")
	},
	// LC_EDIT_FIND
	{
		"Edit.Find",
		QT_TRANSLATE_NOOP("Menu", "&Find..."),
		QT_TRANSLATE_NOOP("Status", "Find object"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+F")
	},
	// LC_EDIT_FIND_NEXT
	{
		"Edit.FindNext",
		QT_TRANSLATE_NOOP("Menu", "Find Ne&xt"),
		QT_TRANSLATE_NOOP("Status", "Find next object"),
		QT_TRANSLATE_NOOP("Shortcut", "F3")
	},
	// LC_EDIT_FIND_PREVIOUS
	{
		"Edit.FindPrevious",
		QT_TRANSLATE_NOOP("Menu", "Find Pre&vious"),
		QT_TRANSLATE_NOOP("Status", "Find object"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+F3")
	},
	// LC_EDIT_SELECT_ALL
	{
		"Edit.SelectAll",
		QT_TRANSLATE_NOOP("Menu", "Select &All"),
		QT_TRANSLATE_NOOP("Status", "Select all pieces in the project"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+A")
	},
	// LC_EDIT_SELECT_NONE
	{
		"Edit.SelectNone",
		QT_TRANSLATE_NOOP("Menu", "Select &None"),
		QT_TRANSLATE_NOOP("Status", "De-select everything"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_SELECT_INVERT
	{
		"Edit.SelectInvert",
		QT_TRANSLATE_NOOP("Menu", "Select &Invert"),
		QT_TRANSLATE_NOOP("Status", "Invert the current selection set"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+I")
	},
	// LC_EDIT_SELECT_BY_NAME
	{
		"Edit.SelectByName",
		QT_TRANSLATE_NOOP("Menu", "Select by Na&me..."),
		QT_TRANSLATE_NOOP("Status", "Select objects by name"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_LOCK_X
	{
		"Edit.Lock.LockX",
		QT_TRANSLATE_NOOP("Menu", "Lock X"),
		QT_TRANSLATE_NOOP("Status", "Prevents movement and rotation along the X axis"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_LOCK_Y
	{
		"Edit.Lock.LockY",
		QT_TRANSLATE_NOOP("Menu", "Lock Y"),
		QT_TRANSLATE_NOOP("Status", "Prevents movement and rotation along the Y axis"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_LOCK_Z
	{
		"Edit.Lock.LockZ",
		QT_TRANSLATE_NOOP("Menu", "Lock Z"),
		QT_TRANSLATE_NOOP("Status", "Prevents movement and rotation along the Z axis"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_LOCK_NONE
	{
		"Edit.Lock.None",
		QT_TRANSLATE_NOOP("Menu", "Unlock All"),
		QT_TRANSLATE_NOOP("Status", "Allows movement and rotation in all directions"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_SNAP_RELATIVE
	{
		"Edit.SnapRelative",
		QT_TRANSLATE_NOOP("Menu", "Relative Snap"),
		QT_TRANSLATE_NOOP("Status", "Enable relative movement and rotation"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_SNAP_MOVE_XY0
	{
		"Edit.SnapMove.XY0",
		QT_TRANSLATE_NOOP("Menu", "None"),
		QT_TRANSLATE_NOOP("Status", "Do not snap movement along the XY plane"),
		QT_TRANSLATE_NOOP("Shortcut", "0")
	},
	// LC_EDIT_SNAP_MOVE_XY1
	{
		"Edit.SnapMove.XY1",
		QT_TRANSLATE_NOOP("Menu", "1/20 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 1/20 stud"),
		QT_TRANSLATE_NOOP("Shortcut", "1")
	},
	// LC_EDIT_SNAP_MOVE_XY2
	{
		"Edit.SnapMove.XY2",
		QT_TRANSLATE_NOOP("Menu", "1/4 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 1/4 stud"),
		QT_TRANSLATE_NOOP("Shortcut", "2")
	},
	// LC_EDIT_SNAP_MOVE_XY3
	{
		"Edit.SnapMove.XY3",
		QT_TRANSLATE_NOOP("Menu", "1 Flat"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 1 flat"),
		QT_TRANSLATE_NOOP("Shortcut", "3")
	},
	// LC_EDIT_SNAP_MOVE_XY4
	{
		"Edit.SnapMove.XY4",
		QT_TRANSLATE_NOOP("Menu", "1/2 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 1/2 stud"),
		QT_TRANSLATE_NOOP("Shortcut", "4")
	},
	// LC_EDIT_SNAP_MOVE_XY5
	{
		"Edit.SnapMove.XY5",
		QT_TRANSLATE_NOOP("Menu", "1 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 1 stud"),
		QT_TRANSLATE_NOOP("Shortcut", "5")
	},
	// LC_EDIT_SNAP_MOVE_XY6
	{
		"Edit.SnapMove.XY6",
		QT_TRANSLATE_NOOP("Menu", "2 Studs"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 2 studs"),
		QT_TRANSLATE_NOOP("Shortcut", "6")
	},
	// LC_EDIT_SNAP_MOVE_XY7
	{
		"Edit.SnapMove.XY7",
		QT_TRANSLATE_NOOP("Menu", "3 Studs"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 3 studs"),
		QT_TRANSLATE_NOOP("Shortcut", "7")
	},
	// LC_EDIT_SNAP_MOVE_XY8
	{
		"Edit.SnapMove.XY8",
		QT_TRANSLATE_NOOP("Menu", "4 Studs"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 4 studs"),
		QT_TRANSLATE_NOOP("Shortcut", "8")
	},
	// LC_EDIT_SNAP_MOVE_XY9
	{
		"Edit.SnapMove.XY9",
		QT_TRANSLATE_NOOP("Menu", "8 Studs"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 8 studs"),
		QT_TRANSLATE_NOOP("Shortcut", "9")
	},
	// LC_EDIT_SNAP_MOVE_Z0
	{
		"Edit.SnapMove.Z0",
		QT_TRANSLATE_NOOP("Menu", "None"),
		QT_TRANSLATE_NOOP("Status", "Do not snap movement along the Z axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Shift+0")
	},
	// LC_EDIT_SNAP_MOVE_Z1
	{
		"Edit.SnapMove.Z1",
		QT_TRANSLATE_NOOP("Menu", "1/20 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1/20 stud"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Shift+1")
	},
	// LC_EDIT_SNAP_MOVE_Z2
	{
		"Edit.SnapMove.Z2",
		QT_TRANSLATE_NOOP("Menu", "1/4 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1/4 stud"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Shift+2")
	},
	// LC_EDIT_SNAP_MOVE_Z3
	{
		"Edit.SnapMove.Z3",
		QT_TRANSLATE_NOOP("Menu", "1 Flat"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1 flat"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Shift+3")
	},
	// LC_EDIT_SNAP_MOVE_Z4
	{
		"Edit.SnapMove.Z4",
		QT_TRANSLATE_NOOP("Menu", "1/2 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1/2 stud"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Shift+4")
	},
	// LC_EDIT_SNAP_MOVE_Z5
	{
		"Edit.SnapMove.Z5",
		QT_TRANSLATE_NOOP("Menu", "1 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1 stud"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Shift+5")
	},
	// LC_EDIT_SNAP_MOVE_Z6
	{
		"Edit.SnapMove.Z6",
		QT_TRANSLATE_NOOP("Menu", "1 Brick"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1 brick"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Shift+6")
	},
	// LC_EDIT_SNAP_MOVE_Z7
	{
		"Edit.SnapMove.Z7",
		QT_TRANSLATE_NOOP("Menu", "2 Bricks"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 2 bricks"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Shift+7")
	},
	// LC_EDIT_SNAP_MOVE_Z8
	{
		"Edit.SnapMove.Z8",
		QT_TRANSLATE_NOOP("Menu", "4 Bricks"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 4 bricks"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Shift+8")
	},
	// LC_EDIT_SNAP_MOVE_Z9
	{
		"Edit.SnapMove.Z9",
		QT_TRANSLATE_NOOP("Menu", "8 Bricks"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 8 bricks"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+Shift+9")
	},
	// LC_EDIT_SNAP_ANGLE0
	{
		"Edit.SnapAngle.Angle0",
		QT_TRANSLATE_NOOP("Menu", "None"),
		QT_TRANSLATE_NOOP("Status", "Do not snap rotations"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+0")
	},
	// LC_EDIT_SNAP_ANGLE1
	{
		"Edit.SnapAngle.Angle1",
		QT_TRANSLATE_NOOP("Menu", "1 Degree"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 1 degree"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+1")
	},
	// LC_EDIT_SNAP_ANGLE2
	{
		"Edit.SnapAngle.Angle2",
		QT_TRANSLATE_NOOP("Menu", "5 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 5 degrees"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+2")
	},
	// LC_EDIT_SNAP_ANGLE3
	{
		"Edit.SnapAngle.Angle3",
		QT_TRANSLATE_NOOP("Menu", "10 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 10 degrees"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+3")
	},
	// LC_EDIT_SNAP_ANGLE4
	{
		"Edit.SnapAngle.Angle4",
		QT_TRANSLATE_NOOP("Menu", "15 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 15 degrees"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+4")
	},
	// LC_EDIT_SNAP_ANGLE5
	{
		"Edit.SnapAngle.Angle5",
		QT_TRANSLATE_NOOP("Menu", "30 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 30 degrees"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+5")
	},
	// LC_EDIT_SNAP_ANGLE6
	{
		"Edit.SnapAngle.Angle6",
		QT_TRANSLATE_NOOP("Menu", "45 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 45 degrees"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+6")
	},
	// LC_EDIT_SNAP_ANGLE7
	{
		"Edit.SnapAngle.Angle7",
		QT_TRANSLATE_NOOP("Menu", "60 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 60 degrees"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+7")
	},
	// LC_EDIT_SNAP_ANGLE8
	{
		"Edit.SnapAngle.Angle8",
		QT_TRANSLATE_NOOP("Menu", "90 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 90 degrees"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+8")
	},
	// LC_EDIT_SNAP_ANGLE9
	{
		"Edit.SnapAngle.Angle9",
		QT_TRANSLATE_NOOP("Menu", "180 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 180 degrees"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+9")
	},
	// LC_EDIT_TRANSFORM
	{
		"Edit.Transform",
		QT_TRANSLATE_NOOP("Menu", "Transform"),
		QT_TRANSLATE_NOOP("Status", "Apply transform to selected objects"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION
	{
		"Edit.TransformAbsoluteTranslation",
		QT_TRANSLATE_NOOP("Menu", "Absolute Translation"),
		QT_TRANSLATE_NOOP("Status", "Switch to absolute translation mode when applying transforms"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION
	{
		"Edit.TransformRelativeTranslation",
		QT_TRANSLATE_NOOP("Menu", "Relative Translation"),
		QT_TRANSLATE_NOOP("Status", "Switch to relative translation mode when applying transforms"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION
	{
		"Edit.TransformAbsoluteRotation",
		QT_TRANSLATE_NOOP("Menu", "Absolute Rotation"),
		QT_TRANSLATE_NOOP("Status", "Switch to absolute rotation mode when applying transforms"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_TRANSFORM_RELATIVE_ROTATION
	{
		"Edit.TransformRelativeRotation",
		QT_TRANSLATE_NOOP("Menu", "Relative Rotation"),
		QT_TRANSLATE_NOOP("Status", "Switch to relative rotation mode when applying transforms"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_ACTION_INSERT
	{
		"Edit.Tool.Insert",
		QT_TRANSLATE_NOOP("Menu", "Insert"),
		QT_TRANSLATE_NOOP("Status", "Add new pieces to the model"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_ACTION_LIGHT
	{
		"Edit.Tool.Light",
		QT_TRANSLATE_NOOP("Menu", "Light"),
		QT_TRANSLATE_NOOP("Status", "Add new omni light sources to the model"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_ACTION_SPOTLIGHT
	{
		"Edit.Tool.Spotlight",
		QT_TRANSLATE_NOOP("Menu", "Spotlight"),
		QT_TRANSLATE_NOOP("Status", "Add new spotlights to the model"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_ACTION_CAMERA
	{
		"Edit.Tool.Camera",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Create a new camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_ACTION_SELECT
	{
		"Edit.Tool.Select",
		QT_TRANSLATE_NOOP("Menu", "Select"),
		QT_TRANSLATE_NOOP("Status", "Select objects (hold the CTRL key down or drag the mouse to select multiple objects)"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+S")
	},
	// LC_EDIT_ACTION_MOVE
	{
		"Edit.Tool.Move",
		QT_TRANSLATE_NOOP("Menu", "Move"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+M")
	},
	// LC_EDIT_ACTION_ROTATE
	{
		"Edit.Tool.Rotate",
		QT_TRANSLATE_NOOP("Menu", "Rotate"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected pieces"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+R")
	},
	// LC_EDIT_ACTION_DELETE
	{
		"Edit.Tool.Delete",
		QT_TRANSLATE_NOOP("Menu", "Delete"),
		QT_TRANSLATE_NOOP("Status", "Delete objects"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+D")
	},
	// LC_EDIT_ACTION_PAINT
	{
		"Edit.Tool.Paint",
		QT_TRANSLATE_NOOP("Menu", "Paint"),
		QT_TRANSLATE_NOOP("Status", "Change piece color"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+N")
	},
	// LC_EDIT_ACTION_ZOOM
	{
		"Edit.Tool.Zoom",
		QT_TRANSLATE_NOOP("Menu", "Zoom"),
		QT_TRANSLATE_NOOP("Status", "Zoom in or out"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+Z")
	},
	// LC_EDIT_ACTION_PAN
	{
		"Edit.Tool.Pan",
		QT_TRANSLATE_NOOP("Menu", "Pan"),
		QT_TRANSLATE_NOOP("Status", "Pan the current view"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+P")
	},
	// LC_EDIT_ACTION_ROTATE_VIEW
	{
		"Edit.Tool.RotateView",
		QT_TRANSLATE_NOOP("Menu", "Rotate View"),
		QT_TRANSLATE_NOOP("Status", "Rotate the current view"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+T")
	},
	// LC_EDIT_ACTION_ROLL
	{
		"Edit.Tool.Roll",
		QT_TRANSLATE_NOOP("Menu", "Roll"),
		QT_TRANSLATE_NOOP("Status", "Roll the current view"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+L")
	},
	// LC_EDIT_ACTION_ZOOM_REGION
	{
		"Edit.Tool.ZoomRegion",
		QT_TRANSLATE_NOOP("Menu", "Zoom Region"),
		QT_TRANSLATE_NOOP("Status", "Zoom into a region of the screen"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_EDIT_CANCEL
	{
		"Edit.Cancel",
		QT_TRANSLATE_NOOP("Menu", "Cancel Action"),
		QT_TRANSLATE_NOOP("Status", "Cancel current mouse action"),
		QT_TRANSLATE_NOOP("Shortcut", "Esc")
	},
	// LC_VIEW_PREFERENCES
	{
		"View.Preferences",
		QT_TRANSLATE_NOOP("Menu", "P&references..."),
		QT_TRANSLATE_NOOP("Status", "Change program settings"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_ZOOM_IN
	{
		"View.ZoomIn",
		QT_TRANSLATE_NOOP("Menu", "Zoom In"),
		QT_TRANSLATE_NOOP("Status", "Zoom in"),
		QT_TRANSLATE_NOOP("Shortcut", "+")
	},
	// LC_VIEW_ZOOM_OUT
	{
		"View.ZoomOut",
		QT_TRANSLATE_NOOP("Menu", "Zoom Out"),
		QT_TRANSLATE_NOOP("Status", "Zoom out"),
		QT_TRANSLATE_NOOP("Shortcut", "-")
	},
	// LC_VIEW_ZOOM_EXTENTS
	{
		"View.ZoomExtents",
		QT_TRANSLATE_NOOP("Menu", "Zoom E&xtents"),
		QT_TRANSLATE_NOOP("Status", "Fit all pieces in current the view (hold the CTRL key down to zoom all views)"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_LOOK_AT
	{
		"View.LookAt",
		QT_TRANSLATE_NOOP("Menu", "Look At"),
		QT_TRANSLATE_NOOP("Status", "Rotate view so selected pieces are at center"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_VIEWPOINT_FRONT
	{
		"View.Viewpoint.Front",
		QT_TRANSLATE_NOOP("Menu", "&Front"),
		QT_TRANSLATE_NOOP("Status", "View model from the front"),
		QT_TRANSLATE_NOOP("Shortcut", "F")
	},
	// LC_VIEW_VIEWPOINT_BACK
	{
		"View.Viewpoint.Back",
		QT_TRANSLATE_NOOP("Menu", "&Back"),
		QT_TRANSLATE_NOOP("Status", "View model from the back"),
		QT_TRANSLATE_NOOP("Shortcut", "B")
	},
	// LC_VIEW_VIEWPOINT_TOP
	{
		"View.Viewpoint.Top",
		QT_TRANSLATE_NOOP("Menu", "&Top"),
		QT_TRANSLATE_NOOP("Status", "View model from the top"),
		QT_TRANSLATE_NOOP("Shortcut", "T")
	},
	// LC_VIEW_VIEWPOINT_BOTTOM
	{
		"View.Viewpoint.Bottom",
		QT_TRANSLATE_NOOP("Menu", "B&ottom"),
		QT_TRANSLATE_NOOP("Status", "View model from the bottom"),
		QT_TRANSLATE_NOOP("Shortcut", "O")
	},
	// LC_VIEW_VIEWPOINT_LEFT
	{
		"View.Viewpoint.Left",
		QT_TRANSLATE_NOOP("Menu", "&Left"),
		QT_TRANSLATE_NOOP("Status", "View model from the left"),
		QT_TRANSLATE_NOOP("Shortcut", "L")
	},
	// LC_VIEW_VIEWPOINT_RIGHT
	{
		"View.Viewpoint.Right",
		QT_TRANSLATE_NOOP("Menu", "&Right"),
		QT_TRANSLATE_NOOP("Status", "View model from the right"),
		QT_TRANSLATE_NOOP("Shortcut", "R")
	},
	// LC_VIEW_VIEWPOINT_HOME
	{
		"View.Viewpoint.Home",
		QT_TRANSLATE_NOOP("Menu", "&Home"),
		QT_TRANSLATE_NOOP("Status", "View model from the default position"),
		QT_TRANSLATE_NOOP("Shortcut", "H")
	},
	// LC_VIEW_CAMERA_NONE
	{
		"View.Cameras.None",
		QT_TRANSLATE_NOOP("Menu", "None"),
		QT_TRANSLATE_NOOP("Status", "Do not use a camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA1
	{
		"View.Cameras.Camera01",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA2
	{
		"View.Cameras.Camera02",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA3
	{
		"View.Cameras.Camera03",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA4
	{
		"View.Cameras.Camera04",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA5
	{
		"View.Cameras.Camera05",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA6
	{
		"View.Cameras.Camera06",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA7
	{
		"View.Cameras.Camera07",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA8
	{
		"View.Cameras.Camera08",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA9
	{
		"View.Cameras.Camera09",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA10
	{
		"View.Cameras.Camera10",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA11
	{
		"View.Cameras.Camera11",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA12
	{
		"View.Cameras.Camera12",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA13
	{
		"View.Cameras.Camera13",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA14
	{
		"View.Cameras.Camera14",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA15
	{
		"View.Cameras.Camera15",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA16
	{
		"View.Cameras.Camera16",
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_CAMERA_RESET
	{
		"View.Cameras.Reset",
		QT_TRANSLATE_NOOP("Menu", "Reset"),
		QT_TRANSLATE_NOOP("Status", "Reset views to their default positions"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_TIME_FIRST
	{
		"View.Time.First",
		QT_TRANSLATE_NOOP("Menu", "First"),
		QT_TRANSLATE_NOOP("Status", "Go to the first step of the model"),
		QT_TRANSLATE_NOOP("Shortcut", "Alt+Up")
	},
	// LC_VIEW_TIME_PREVIOUS
	{
		"View.Time.Previous",
		QT_TRANSLATE_NOOP("Menu", "Previous"),
		QT_TRANSLATE_NOOP("Status", "Go to the previous step"),
		QT_TRANSLATE_NOOP("Shortcut", "Alt+Left")
	},
	// LC_VIEW_TIME_NEXT
	{
		"View.Time.Next",
		QT_TRANSLATE_NOOP("Menu", "Next"),
		QT_TRANSLATE_NOOP("Status", "Go to the next step"),
		QT_TRANSLATE_NOOP("Shortcut", "Alt+Right")
	},
	// LC_VIEW_TIME_LAST
	{
		"View.Time.Last",
		QT_TRANSLATE_NOOP("Menu", "Last"),
		QT_TRANSLATE_NOOP("Status", "Go to the last step of the model"),
		QT_TRANSLATE_NOOP("Shortcut", "Alt+Down")
	},
	// LC_VIEW_TIME_INSERT
	{
		"View.Time.Insert",
		QT_TRANSLATE_NOOP("Menu", "Insert"),
		QT_TRANSLATE_NOOP("Status", "Insert new step"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_TIME_DELETE
	{
		"View.Time.Delete",
		QT_TRANSLATE_NOOP("Menu", "Delete"),
		QT_TRANSLATE_NOOP("Status", "Delete current step"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_TIME_ADD_KEYS
	{
		"View.Time.AddKeys",
		QT_TRANSLATE_NOOP("Menu", "Add Keys"),
		QT_TRANSLATE_NOOP("Status", "Toggle adding new animation keys"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_SPLIT_HORIZONTAL
	{
		"View.SplitHorizontal",
		QT_TRANSLATE_NOOP("Menu", "Split &Horizontal"),
		QT_TRANSLATE_NOOP("Status", "Split the current view horizontally"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_SPLIT_VERTICAL
	{
		"View.SplitVertical",
		QT_TRANSLATE_NOOP("Menu", "Split &Vertical"),
		QT_TRANSLATE_NOOP("Status", "Split the current view vertically"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_REMOVE_VIEW
	{
		"View.RemoveView",
		QT_TRANSLATE_NOOP("Menu", "Re&move View"),
		QT_TRANSLATE_NOOP("Status", "Remove the current view"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_RESET_VIEWS
	{
		"View.ResetViews",
		QT_TRANSLATE_NOOP("Menu", "Rese&t Views"),
		QT_TRANSLATE_NOOP("Status", "Reset all views"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_FULLSCREEN
	{
		"View.FullScreen",
		QT_TRANSLATE_NOOP("Menu", "&Full Screen"),
		QT_TRANSLATE_NOOP("Status", "Toggle fullscreen mode"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_PROJECTION_PERSPECTIVE
	{
		"View.Projection.Perspective",
		QT_TRANSLATE_NOOP("Menu", "&Perspective"),
		QT_TRANSLATE_NOOP("Status", "Set the current camera to use a perspective projection"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_PROJECTION_ORTHO
	{
		"View.Projection.Orthographic",
		QT_TRANSLATE_NOOP("Menu", "&Orthographic"),
		QT_TRANSLATE_NOOP("Status", "Set the current camera to use an orthographic projection"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_PROJECTION_CYCLE
	{
		"View.Projection.Cycle",
		QT_TRANSLATE_NOOP("Menu", "&Cycle"),
		QT_TRANSLATE_NOOP("Status", "Cycle to next projection type"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_VIEW_PROJECTION_FOCUS
	{
		"View.Projection.Focus",
		QT_TRANSLATE_NOOP("Menu", "&Focus"),
		QT_TRANSLATE_NOOP("Status", "Focus projection on selected piece"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_PIECE_INSERT
	{
		"Piece.Insert",
		QT_TRANSLATE_NOOP("Menu", "&Insert"),
		QT_TRANSLATE_NOOP("Status", "Add a new piece to the model"),
		QT_TRANSLATE_NOOP("Shortcut", "Insert")
	},
	// LC_PIECE_DELETE
	{
		"Piece.Delete",
		QT_TRANSLATE_NOOP("Menu", "&Delete"),
		QT_TRANSLATE_NOOP("Status", "Delete selected objects"),
		QT_TRANSLATE_NOOP("Shortcut", "Delete")
	},
	// LC_PIECE_MOVE_PLUSX
	{
		"Piece.Move.PlusX",
		QT_TRANSLATE_NOOP("Menu", "Move +X"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the X axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Down")
	},
	// LC_PIECE_MOVE_MINUSX
	{
		"Piece.Move.MinusX",
		QT_TRANSLATE_NOOP("Menu", "Move -X"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the X axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Up")
	},
	// LC_PIECE_MOVE_PLUSY
	{
		"Piece.Move.PlusY",
		QT_TRANSLATE_NOOP("Menu", "Move +Y"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the Y axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Right")
	},
	// LC_PIECE_MOVE_MINUSY
	{
		"Piece.Move.MinusY",
		QT_TRANSLATE_NOOP("Menu", "Move -Y"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the Y axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Left")
	},
	// LC_PIECE_MOVE_PLUSZ
	{
		"Piece.Move.PlusZ",
		QT_TRANSLATE_NOOP("Menu", "Move +Z"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the Z axis"),
		QT_TRANSLATE_NOOP("Shortcut", "PgUp")
	},
	// LC_PIECE_MOVE_MINUSZ
	{
		"Piece.Move.MinusZ",
		QT_TRANSLATE_NOOP("Menu", "Move -Z"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the Z axis"),
		QT_TRANSLATE_NOOP("Shortcut", "PgDown")
	},
	// LC_PIECE_ROTATE_PLUSX
	{
		"Piece.Rotate.PlusX",
		QT_TRANSLATE_NOOP("Menu", "Rotate +X"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the X axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+Down")
	},
	// LC_PIECE_ROTATE_MINUSX
	{
		"Piece.Rotate.MinusX",
		QT_TRANSLATE_NOOP("Menu", "Rotate -X"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the X axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+Up")
	},
	// LC_PIECE_ROTATE_PLUSY
	{
		"Piece.Rotate.PlusY",
		QT_TRANSLATE_NOOP("Menu", "Rotate +Y"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the Y axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+Right")
	},
	// LC_PIECE_ROTATE_MINUSY
	{
		"Piece.Rotate.MinusY",
		QT_TRANSLATE_NOOP("Menu", "Rotate -Y"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the Y axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+Left")
	},
	// LC_PIECE_ROTATE_PLUSZ
	{
		"Piece.Rotate.PlusZ",
		QT_TRANSLATE_NOOP("Menu", "Rotate +Z"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the Z axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+PgUp")
	},
	// LC_PIECE_ROTATE_MINUSZ
	{
		"Piece.Rotate.MinusZ",
		QT_TRANSLATE_NOOP("Menu", "Rotate -Z"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the Z axis"),
		QT_TRANSLATE_NOOP("Shortcut", "Shift+PgDown")
	},
	// LC_PIECE_MINIFIG_WIZARD
	{
		"Piece.MinifigWizard",
		QT_TRANSLATE_NOOP("Menu", "Minifig &Wizard..."),
		QT_TRANSLATE_NOOP("Status", "Add a new minifig to the model"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_PIECE_ARRAY
	{
		"Piece.Array",
		QT_TRANSLATE_NOOP("Menu", "A&rray..."),
		QT_TRANSLATE_NOOP("Status", "Make copies of the selected pieces"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_PIECE_GROUP
	{
		"Piece.Group",
		QT_TRANSLATE_NOOP("Menu", "&Group..."),
		QT_TRANSLATE_NOOP("Status", "Group selected pieces together"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+G")
	},
	// LC_PIECE_UNGROUP
	{
		"Piece.Ungroup",
		QT_TRANSLATE_NOOP("Menu", "&Ungroup"),
		QT_TRANSLATE_NOOP("Status", "Ungroup selected group"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+U")
	},
	// LC_PIECE_GROUP_ADD
	{
		"Piece.GroupAdd",
		QT_TRANSLATE_NOOP("Menu", "&Add to Group"),
		QT_TRANSLATE_NOOP("Status", "Add focused piece to selected group"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_PIECE_GROUP_REMOVE
	{
		"Piece.GroupRemove",
		QT_TRANSLATE_NOOP("Menu", "Re&move from Group"),
		QT_TRANSLATE_NOOP("Status", "Remove focused piece from group"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_PIECE_GROUP_EDIT
	{
		"Piece.GroupEdit",
		QT_TRANSLATE_NOOP("Menu", "&Edit Groups..."),
		QT_TRANSLATE_NOOP("Status", "Edit groups"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_PIECE_HIDE_SELECTED
	{
		"Piece.HideSelected",
		QT_TRANSLATE_NOOP("Menu", "&Hide Selected"),
		QT_TRANSLATE_NOOP("Status", "Hide selected objects"),
		QT_TRANSLATE_NOOP("Shortcut", "Ctrl+H")
	},
	// LC_PIECE_HIDE_UNSELECTED
	{
		"Piece.HideUnselected",
		QT_TRANSLATE_NOOP("Menu", "Hide &Unselected"),
		QT_TRANSLATE_NOOP("Status", "Hide objects that are not selected"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_PIECE_UNHIDE_ALL
	{
		"Piece.UnhideAll",
		QT_TRANSLATE_NOOP("Menu", "U&nhide All"),
		QT_TRANSLATE_NOOP("Status", "Show all hidden objects"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_PIECE_SHOW_EARLIER
	{
		"Piece.ShowEarlier",
		QT_TRANSLATE_NOOP("Menu", "Show Earlier"),
		QT_TRANSLATE_NOOP("Status", "Show selected pieces one step earlier"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_PIECE_SHOW_LATER
	{
		"Piece.ShowLater",
		QT_TRANSLATE_NOOP("Menu", "Show Later"),
		QT_TRANSLATE_NOOP("Status", "Show selected pieces one step later"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_HELP_HOMEPAGE
	{
		"Help.HomePage",
		QT_TRANSLATE_NOOP("Menu", "LeoCAD &Home Page"),
		QT_TRANSLATE_NOOP("Status", "Open LeoCAD's home page on the internet using your default web browser"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_HELP_EMAIL
	{
		"Help.Email",
		QT_TRANSLATE_NOOP("Menu", "Send Support &E-Mail"),
		QT_TRANSLATE_NOOP("Status", "Send an e-mail message for help or support using your default e-mail client"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_HELP_UPDATES
	{
		"Help.Updates",
		QT_TRANSLATE_NOOP("Menu", "Check for &Updates..."),
		QT_TRANSLATE_NOOP("Status", "Check if a newer LeoCAD version or parts library has been released"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	},
	// LC_HELP_ABOUT
	{
		"Help.About",
		QT_TRANSLATE_NOOP("Menu", "&About..."),
		QT_TRANSLATE_NOOP("Status", "Display program version number and system information"),
		QT_TRANSLATE_NOOP("Shortcut", "")
	}
};

LC_CASSERT(sizeof(gCommands)/sizeof(gCommands[0]) == LC_NUM_COMMANDS);

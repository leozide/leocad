#include "lc_global.h"
#include "lc_commands.h"

lcCommand gCommands[LC_NUM_COMMANDS] =
{
	// LC_FILE_NEW
	{
		QT_TRANSLATE_NOOP("Action", "File.New"),
		QT_TRANSLATE_NOOP("Menu", "&New"),
		QT_TRANSLATE_NOOP("Status", "Create a new model"),
		"Ctrl+N"
	},
	// LC_FILE_OPEN
	{
		QT_TRANSLATE_NOOP("Action", "File.Open"),
		QT_TRANSLATE_NOOP("Menu", "&Open..."),
		QT_TRANSLATE_NOOP("Status", "Open an existing model"),
		"Ctrl+O"
	},
	// LC_FILE_MERGE
	{
		QT_TRANSLATE_NOOP("Action", "File.Merge"),
		QT_TRANSLATE_NOOP("Menu", "&Merge..."),
		QT_TRANSLATE_NOOP("Status", "Merge the contents of another file with the current one"),
		""
	},
	// LC_FILE_SAVE
	{
		QT_TRANSLATE_NOOP("Action", "File.Save"),
		QT_TRANSLATE_NOOP("Menu", "&Save"),
		QT_TRANSLATE_NOOP("Status", "Save the current model"),
		"Ctrl+S"
	},
	// LC_FILE_SAVEAS
	{
		QT_TRANSLATE_NOOP("Action", "File.SaveAs"),
		QT_TRANSLATE_NOOP("Menu", "Save &As..."),
		QT_TRANSLATE_NOOP("Status", "Save the current model with a new name"),
		""
	},
	// LC_FILE_SAVE_IMAGE
	{
		QT_TRANSLATE_NOOP("Action", "File.SaveImage"),
		QT_TRANSLATE_NOOP("Menu", "Save &Image..."),
		QT_TRANSLATE_NOOP("Status", "Save a picture of the current view"),
		""
	},
	// LC_FILE_IMPORT_LDD
	{
		QT_TRANSLATE_NOOP("Action", "File.Import.LDD"),
		QT_TRANSLATE_NOOP("Menu", "&LEGO Digital Designer..."),
		QT_TRANSLATE_NOOP("Status", "Import a file in LEGO Digital Designer LXF format"),
		""
	},
	// LC_FILE_IMPORT_INVENTORY
	{
		QT_TRANSLATE_NOOP("Action", "File.Import.Inventory"),
		QT_TRANSLATE_NOOP("Menu", "Set &Inventory..."),
		QT_TRANSLATE_NOOP("Status", "Import all parts from an official set"),
		""
	},
	// LC_FILE_EXPORT_3DS
	{
		QT_TRANSLATE_NOOP("Action", "File.Export.3DS"),
		QT_TRANSLATE_NOOP("Menu", "3D &Studio..."),
		QT_TRANSLATE_NOOP("Status", "Export the current model in 3D Studio 3DS format"),
		""
	},
	// LC_FILE_EXPORT_COLLADA
	{
		QT_TRANSLATE_NOOP("Action", "File.Export.COLLADA"),
		QT_TRANSLATE_NOOP("Menu", "&COLLADA..."),
		QT_TRANSLATE_NOOP("Status", "Export the current model in COLLADA DAE format"),
		""
	},
	// LC_FILE_EXPORT_HTML
	{
		QT_TRANSLATE_NOOP("Action", "File.Export.HTML"),
		QT_TRANSLATE_NOOP("Menu", "&HTML..."),
		QT_TRANSLATE_NOOP("Status", "Create an HTML page for the current model"),
		""
	},
	// LC_FILE_EXPORT_BRICKLINK
	{
		QT_TRANSLATE_NOOP("Action", "File.Export.BrickLink"),
		QT_TRANSLATE_NOOP("Menu", "&BrickLink..."),
		QT_TRANSLATE_NOOP("Status", "Export a list of parts used in BrickLink XML format"),
		""
	},
	// LC_FILE_EXPORT_CSV
	{
		QT_TRANSLATE_NOOP("Action", "File.Export.CSV"),
		QT_TRANSLATE_NOOP("Menu", "&CSV..."),
		QT_TRANSLATE_NOOP("Status", "Export a list of parts used in comma delimited file format"),
		""
	},
	// LC_FILE_EXPORT_POVRAY
	{
		QT_TRANSLATE_NOOP("Action", "File.Export.POVRay"),
		QT_TRANSLATE_NOOP("Menu", "&POV-Ray..."),
		QT_TRANSLATE_NOOP("Status", "Export the current model in POV-Ray format"),
		""
	},
	// LC_FILE_EXPORT_WAVEFRONT
	{
		QT_TRANSLATE_NOOP("Action", "File.Export.Wavefront"),
		QT_TRANSLATE_NOOP("Menu", "&Wavefront..."),
		QT_TRANSLATE_NOOP("Status", "Export the current model in Wavefront OBJ format"),
		""
	},
	// LC_FILE_RENDER
	{
		QT_TRANSLATE_NOOP("Action", "File.Render"),
		QT_TRANSLATE_NOOP("Menu", "&Render..."),
		QT_TRANSLATE_NOOP("Status", "Render the current model using POV-Ray"),
		""
	},
	// LC_FILE_PRINT
	{
		QT_TRANSLATE_NOOP("Action", "File.Print"),
		QT_TRANSLATE_NOOP("Menu", "&Print..."),
		QT_TRANSLATE_NOOP("Status", "Print the current model"),
		""
	},
	// LC_FILE_PRINT_PREVIEW
	{
		QT_TRANSLATE_NOOP("Action", "File.PrintPreview"),
		QT_TRANSLATE_NOOP("Menu", "Print Pre&view..."),
		QT_TRANSLATE_NOOP("Status", "Display how the model would look if printed"),
		""
	},
	// LC_FILE_PRINT_BOM
	{
		QT_TRANSLATE_NOOP("Action", "File.PrintBOM"),
		QT_TRANSLATE_NOOP("Menu", "Print &Bill of Materials..."),
		QT_TRANSLATE_NOOP("Status", "Print a list of parts used"),
		""
	},
	// LC_FILE_RECENT1
	{
		QT_TRANSLATE_NOOP("Action", "File.Recent1"),
		QT_TRANSLATE_NOOP("Menu", "&Recent 1"),
		QT_TRANSLATE_NOOP("Status", "Open this model"),
		""
	},
	// LC_FILE_RECENT2
	{
		QT_TRANSLATE_NOOP("Action", "File.Recent2"),
		QT_TRANSLATE_NOOP("Menu", "&Recent 2"),
		QT_TRANSLATE_NOOP("Status", "Open this model"),
		""
	},
	// LC_FILE_RECENT3
	{
		QT_TRANSLATE_NOOP("Action", "File.Recent3"),
		QT_TRANSLATE_NOOP("Menu", "&Recent 3"),
		QT_TRANSLATE_NOOP("Status", "Open this model"),
		""
	},
	// LC_FILE_RECENT4
	{
		QT_TRANSLATE_NOOP("Action", "File.Recent4"),
		QT_TRANSLATE_NOOP("Menu", "&Recent 4"),
		QT_TRANSLATE_NOOP("Status", "Open this model"),
		""
	},
	// LC_FILE_EXIT
	{
		QT_TRANSLATE_NOOP("Action", "File.Exit"),
		QT_TRANSLATE_NOOP("Menu", "E&xit"),
		QT_TRANSLATE_NOOP("Status", "Quit the application; prompts to save model"),
		""
	},
	// LC_EDIT_UNDO
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Undo"),
		QT_TRANSLATE_NOOP("Menu", "&Undo"),
		QT_TRANSLATE_NOOP("Status", "Undo the last action"),
		"Ctrl+Z"
	},
	// LC_EDIT_REDO
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Redo"),
		QT_TRANSLATE_NOOP("Menu", "&Redo"),
		QT_TRANSLATE_NOOP("Status", "Redo the previously undone action"),
		"Ctrl+Y"
	},
	// LC_EDIT_CUT
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Cut"),
		QT_TRANSLATE_NOOP("Menu", "Cu&t"),
		QT_TRANSLATE_NOOP("Status", "Cut the selection and put it on the Clipboard"),
		"Ctrl+X"
	},
	// LC_EDIT_COPY
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Copy"),
		QT_TRANSLATE_NOOP("Menu", "&Copy"),
		QT_TRANSLATE_NOOP("Status", "Copy the selection and put it on the Clipboard"),
		"Ctrl+C"
	},
	// LC_EDIT_PASTE
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Paste"),
		QT_TRANSLATE_NOOP("Menu", "&Paste"),
		QT_TRANSLATE_NOOP("Status", "Insert Clipboard contents"),
		"Ctrl+V"
	},
	// LC_EDIT_FIND
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Find"),
		QT_TRANSLATE_NOOP("Menu", "&Find..."),
		QT_TRANSLATE_NOOP("Status", "Find object"),
		"Ctrl+F"
	},
	// LC_EDIT_FIND_NEXT
	{
		QT_TRANSLATE_NOOP("Action", "Edit.FindNext"),
		QT_TRANSLATE_NOOP("Menu", "Find Ne&xt"),
		QT_TRANSLATE_NOOP("Status", "Find next object"),
		"F3"
	},
	// LC_EDIT_FIND_PREVIOUS
	{
		QT_TRANSLATE_NOOP("Action", "Edit.FindPrevious"),
		QT_TRANSLATE_NOOP("Menu", "Find Pre&vious"),
		QT_TRANSLATE_NOOP("Status", "Find object"),
		"Shift+F3"
	},
	// LC_EDIT_SELECT_ALL
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SelectAll"),
		QT_TRANSLATE_NOOP("Menu", "Select &All"),
		QT_TRANSLATE_NOOP("Status", "Select all pieces in the model"),
		"Ctrl+A"
	},
	// LC_EDIT_SELECT_NONE
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SelectNone"),
		QT_TRANSLATE_NOOP("Menu", "Select &None"),
		QT_TRANSLATE_NOOP("Status", "De-select everything"),
		""
	},
	// LC_EDIT_SELECT_INVERT
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SelectInvert"),
		QT_TRANSLATE_NOOP("Menu", "Select &Invert"),
		QT_TRANSLATE_NOOP("Status", "Invert the current selection set"),
		"Ctrl+I"
	},
	// LC_EDIT_SELECT_BY_NAME
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SelectByName"),
		QT_TRANSLATE_NOOP("Menu", "Select by Na&me..."),
		QT_TRANSLATE_NOOP("Status", "Select objects by name"),
		""
	},
	// LC_EDIT_SELECT_BY_COLOR
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SelectByColor"),
		QT_TRANSLATE_NOOP("Menu", "Select by Col&or..."),
		QT_TRANSLATE_NOOP("Status", "Select pieces by color"),
		""
	},
	// LC_EDIT_SELECT_SINGLE
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SelectSingle"),
		QT_TRANSLATE_NOOP("Menu", "Single Selection"),
		QT_TRANSLATE_NOOP("Status", "Select one piece at a time"),
		""
	},
	// LC_EDIT_SELECT_PIECE
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SelectPiece"),
		QT_TRANSLATE_NOOP("Menu", "Piece Selection"),
		QT_TRANSLATE_NOOP("Status", "Select all pieces of the same type"),
		""
	},
	// LC_EDIT_SELECT_COLOR
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SelectColor"),
		QT_TRANSLATE_NOOP("Menu", "Color Selection"),
		QT_TRANSLATE_NOOP("Status", "Select all pieces of the same color"),
		""
	},
	// LC_EDIT_SELECT_PIECE_COLOR
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SelectPieceColor"),
		QT_TRANSLATE_NOOP("Menu", "Piece and Color Selection"),
		QT_TRANSLATE_NOOP("Status", "Select all pieces of the same type and color"),
		""
	},
	// LC_EDIT_TRANSFORM_RELATIVE
	{
		QT_TRANSLATE_NOOP("Action", "Edit.TransformRelative"),
		QT_TRANSLATE_NOOP("Menu", "Relative Transforms"),
		QT_TRANSLATE_NOOP("Status", "Move and rotate objects relative to the one that has focus"),
		""
	},
	// LC_EDIT_SNAP_MOVE_TOGGLE
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Snap.Toggle"),
		QT_TRANSLATE_NOOP("Menu", "Move Snap Enabled"),
		QT_TRANSLATE_NOOP("Status", "Toggle snap axes"),
		""
	},
	// LC_EDIT_SNAP_MOVE_XY0
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.XY0"),
		QT_TRANSLATE_NOOP("Menu", "None"),
		QT_TRANSLATE_NOOP("Status", "Do not snap movement along the XY plane"),
		"0"
	},
	// LC_EDIT_SNAP_MOVE_XY1
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.XY1"),
		QT_TRANSLATE_NOOP("Menu", "1/20 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 1/20 stud"),
		"1"
	},
	// LC_EDIT_SNAP_MOVE_XY2
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.XY2"),
		QT_TRANSLATE_NOOP("Menu", "1/4 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 1/4 stud"),
		"2"
	},
	// LC_EDIT_SNAP_MOVE_XY3
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.XY3"),
		QT_TRANSLATE_NOOP("Menu", "1 Flat"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 1 flat"),
		"3"
	},
	// LC_EDIT_SNAP_MOVE_XY4
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.XY4"),
		QT_TRANSLATE_NOOP("Menu", "1/2 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 1/2 stud"),
		"4"
	},
	// LC_EDIT_SNAP_MOVE_XY5
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.XY5"),
		QT_TRANSLATE_NOOP("Menu", "1 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 1 stud"),
		"5"
	},
	// LC_EDIT_SNAP_MOVE_XY6
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.XY6"),
		QT_TRANSLATE_NOOP("Menu", "2 Studs"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 2 studs"),
		"6"
	},
	// LC_EDIT_SNAP_MOVE_XY7
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.XY7"),
		QT_TRANSLATE_NOOP("Menu", "3 Studs"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 3 studs"),
		"7"
	},
	// LC_EDIT_SNAP_MOVE_XY8
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.XY8"),
		QT_TRANSLATE_NOOP("Menu", "4 Studs"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 4 studs"),
		"8"
	},
	// LC_EDIT_SNAP_MOVE_XY9
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.XY9"),
		QT_TRANSLATE_NOOP("Menu", "8 Studs"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the XY plane to 8 studs"),
		"9"
	},
	// LC_EDIT_SNAP_MOVE_Z0
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.Z0"),
		QT_TRANSLATE_NOOP("Menu", "None"),
		QT_TRANSLATE_NOOP("Status", "Do not snap movement along the Z axis"),
		"Ctrl+Shift+0"
	},
	// LC_EDIT_SNAP_MOVE_Z1
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.Z1"),
		QT_TRANSLATE_NOOP("Menu", "1/20 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1/20 stud"),
		"Ctrl+Shift+1"
	},
	// LC_EDIT_SNAP_MOVE_Z2
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.Z2"),
		QT_TRANSLATE_NOOP("Menu", "1/4 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1/4 stud"),
		"Ctrl+Shift+2"
	},
	// LC_EDIT_SNAP_MOVE_Z3
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.Z3"),
		QT_TRANSLATE_NOOP("Menu", "1 Flat"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1 flat"),
		"Ctrl+Shift+3"
	},
	// LC_EDIT_SNAP_MOVE_Z4
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.Z4"),
		QT_TRANSLATE_NOOP("Menu", "1/2 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1/2 stud"),
		"Ctrl+Shift+4"
	},
	// LC_EDIT_SNAP_MOVE_Z5
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.Z5"),
		QT_TRANSLATE_NOOP("Menu", "1 Stud"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1 stud"),
		"Ctrl+Shift+5"
	},
	// LC_EDIT_SNAP_MOVE_Z6
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.Z6"),
		QT_TRANSLATE_NOOP("Menu", "1 Brick"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 1 brick"),
		"Ctrl+Shift+6"
	},
	// LC_EDIT_SNAP_MOVE_Z7
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.Z7"),
		QT_TRANSLATE_NOOP("Menu", "2 Bricks"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 2 bricks"),
		"Ctrl+Shift+7"
	},
	// LC_EDIT_SNAP_MOVE_Z8
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.Z8"),
		QT_TRANSLATE_NOOP("Menu", "4 Bricks"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 4 bricks"),
		"Ctrl+Shift+8"
	},
	// LC_EDIT_SNAP_MOVE_Z9
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapMove.Z9"),
		QT_TRANSLATE_NOOP("Menu", "8 Bricks"),
		QT_TRANSLATE_NOOP("Status", "Snap movement along the Z axis to 8 bricks"),
		"Ctrl+Shift+9"
	},
	// LC_EDIT_SNAP_ANGLE_TOGGLE
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Toggle"),
		QT_TRANSLATE_NOOP("Menu", "Rotation Snap Enabled"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to fixed intervals"),
		""
	},
	// LC_EDIT_SNAP_ANGLE0
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Angle0"),
		QT_TRANSLATE_NOOP("Menu", "None"),
		QT_TRANSLATE_NOOP("Status", "Do not snap rotations"),
		"Shift+0"
	},
	// LC_EDIT_SNAP_ANGLE1
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Angle1"),
		QT_TRANSLATE_NOOP("Menu", "1 Degree"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 1 degree"),
		"Shift+1"
	},
	// LC_EDIT_SNAP_ANGLE2
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Angle2"),
		QT_TRANSLATE_NOOP("Menu", "5 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 5 degrees"),
		"Shift+2"
	},
	// LC_EDIT_SNAP_ANGLE3
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Angle3"),
		QT_TRANSLATE_NOOP("Menu", "15 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 15 degrees"),
		"Shift+3"
	},
	// LC_EDIT_SNAP_ANGLE4
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Angle4"),
		QT_TRANSLATE_NOOP("Menu", "22.5 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 22.5 degrees"),
		"Shift+4"
	},
	// LC_EDIT_SNAP_ANGLE5
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Angle5"),
		QT_TRANSLATE_NOOP("Menu", "30 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 30 degrees"),
		"Shift+5"
	},
	// LC_EDIT_SNAP_ANGLE6
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Angle6"),
		QT_TRANSLATE_NOOP("Menu", "45 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 45 degrees"),
		"Shift+6"
	},
	// LC_EDIT_SNAP_ANGLE7
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Angle7"),
		QT_TRANSLATE_NOOP("Menu", "60 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 60 degrees"),
		"Shift+7"
	},
	// LC_EDIT_SNAP_ANGLE8
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Angle8"),
		QT_TRANSLATE_NOOP("Menu", "90 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 90 degrees"),
		"Shift+8"
	},
	// LC_EDIT_SNAP_ANGLE9
	{
		QT_TRANSLATE_NOOP("Action", "Edit.SnapAngle.Angle9"),
		QT_TRANSLATE_NOOP("Menu", "180 Degrees"),
		QT_TRANSLATE_NOOP("Status", "Snap rotations to 180 degrees"),
		"Shift+9"
	},
	// LC_EDIT_TRANSFORM
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Transform"),
		QT_TRANSLATE_NOOP("Menu", "Transform"),
		QT_TRANSLATE_NOOP("Status", "Apply transform to selected objects"),
		""
	},
	// LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION
	{
		QT_TRANSLATE_NOOP("Action", "Edit.TransformAbsoluteTranslation"),
		QT_TRANSLATE_NOOP("Menu", "Absolute Translation"),
		QT_TRANSLATE_NOOP("Status", "Switch to absolute translation mode when applying transforms"),
		""
	},
	// LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION
	{
		QT_TRANSLATE_NOOP("Action", "Edit.TransformRelativeTranslation"),
		QT_TRANSLATE_NOOP("Menu", "Relative Translation"),
		QT_TRANSLATE_NOOP("Status", "Switch to relative translation mode when applying transforms"),
		""
	},
	// LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION
	{
		QT_TRANSLATE_NOOP("Action", "Edit.TransformAbsoluteRotation"),
		QT_TRANSLATE_NOOP("Menu", "Absolute Rotation"),
		QT_TRANSLATE_NOOP("Status", "Switch to absolute rotation mode when applying transforms"),
		""
	},
	// LC_EDIT_TRANSFORM_RELATIVE_ROTATION
	{
		QT_TRANSLATE_NOOP("Action", "Edit.TransformRelativeRotation"),
		QT_TRANSLATE_NOOP("Menu", "Relative Rotation"),
		QT_TRANSLATE_NOOP("Status", "Switch to relative rotation mode when applying transforms"),
		""
	},
	// LC_EDIT_ACTION_INSERT
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Insert"),
		QT_TRANSLATE_NOOP("Menu", "Insert"),
		QT_TRANSLATE_NOOP("Status", "Add new pieces to the model"),
		""
	},
	// LC_EDIT_ACTION_LIGHT
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Light"),
		QT_TRANSLATE_NOOP("Menu", "Light"),
		QT_TRANSLATE_NOOP("Status", "Add new omni light sources to the model"),
		""
	},
	// LC_EDIT_ACTION_SPOTLIGHT
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Spotlight"),
		QT_TRANSLATE_NOOP("Menu", "Spotlight"),
		QT_TRANSLATE_NOOP("Status", "Add new spotlights to the model"),
		""
	},
	// LC_EDIT_ACTION_CAMERA
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Camera"),
		QT_TRANSLATE_NOOP("Menu", "Camera"),
		QT_TRANSLATE_NOOP("Status", "Create a new camera"),
		""
	},
	// LC_EDIT_ACTION_SELECT
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Select"),
		QT_TRANSLATE_NOOP("Menu", "Select"),
		QT_TRANSLATE_NOOP("Status", "Select objects (hold the CTRL key down or drag the mouse to select multiple objects)"),
		""
	},
	// LC_EDIT_ACTION_MOVE
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Move"),
		QT_TRANSLATE_NOOP("Menu", "Move"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects"),
		""
	},
	// LC_EDIT_ACTION_ROTATE
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Rotate"),
		QT_TRANSLATE_NOOP("Menu", "Rotate"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected pieces"),
		""
	},
	// LC_EDIT_ACTION_DELETE
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Delete"),
		QT_TRANSLATE_NOOP("Menu", "Delete"),
		QT_TRANSLATE_NOOP("Status", "Delete objects"),
		""
	},
	// LC_EDIT_ACTION_PAINT
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Paint"),
		QT_TRANSLATE_NOOP("Menu", "Paint"),
		QT_TRANSLATE_NOOP("Status", "Change piece color"),
		""
	},
	// LC_EDIT_ACTION_COLOR_PICKER
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.ColorPicker"),
		QT_TRANSLATE_NOOP("Menu", "Color Picker"),
		QT_TRANSLATE_NOOP("Status", "Get piece color"),
		""
	},
	// LC_EDIT_ACTION_ZOOM
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Zoom"),
		QT_TRANSLATE_NOOP("Menu", "Zoom"),
		QT_TRANSLATE_NOOP("Status", "Zoom in or out"),
		""
	},
	// LC_EDIT_ACTION_PAN
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Pan"),
		QT_TRANSLATE_NOOP("Menu", "Pan"),
		QT_TRANSLATE_NOOP("Status", "Pan the current view"),
		""
	},
	// LC_EDIT_ACTION_ROTATE_VIEW
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.RotateView"),
		QT_TRANSLATE_NOOP("Menu", "Rotate View"),
		QT_TRANSLATE_NOOP("Status", "Rotate the current view"),
		""
	},
	// LC_EDIT_ACTION_ROLL
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.Roll"),
		QT_TRANSLATE_NOOP("Menu", "Roll"),
		QT_TRANSLATE_NOOP("Status", "Roll the current view"),
		""
	},
	// LC_EDIT_ACTION_ZOOM_REGION
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Tool.ZoomRegion"),
		QT_TRANSLATE_NOOP("Menu", "Zoom Region"),
		QT_TRANSLATE_NOOP("Status", "Zoom into a region of the screen"),
		""
	},
	// LC_EDIT_CANCEL
	{
		QT_TRANSLATE_NOOP("Action", "Edit.Cancel"),
		QT_TRANSLATE_NOOP("Menu", "Cancel Action"),
		QT_TRANSLATE_NOOP("Status", "Cancel current mouse action"),
		"Esc"
	},
	// LC_VIEW_PREFERENCES
	{
		QT_TRANSLATE_NOOP("Action", "View.Preferences"),
		QT_TRANSLATE_NOOP("Menu", "P&references..."),
		QT_TRANSLATE_NOOP("Status", "Change program settings"),
		""
	},
	// LC_VIEW_ZOOM_IN
	{
		QT_TRANSLATE_NOOP("Action", "View.ZoomIn"),
		QT_TRANSLATE_NOOP("Menu", "Zoom In"),
		QT_TRANSLATE_NOOP("Status", "Zoom in"),
		"+"
	},
	// LC_VIEW_ZOOM_OUT
	{
		QT_TRANSLATE_NOOP("Action", "View.ZoomOut"),
		QT_TRANSLATE_NOOP("Menu", "Zoom Out"),
		QT_TRANSLATE_NOOP("Status", "Zoom out"),
		"-"
	},
	// LC_VIEW_ZOOM_EXTENTS
	{
		QT_TRANSLATE_NOOP("Action", "View.ZoomExtents"),
		QT_TRANSLATE_NOOP("Menu", "Zoom E&xtents"),
		QT_TRANSLATE_NOOP("Status", "Fit all pieces in current the view (hold the CTRL key down to zoom all views)"),
		""
	},
	// LC_VIEW_LOOK_AT
	{
		QT_TRANSLATE_NOOP("Action", "View.LookAt"),
		QT_TRANSLATE_NOOP("Menu", "Look At"),
		QT_TRANSLATE_NOOP("Status", "Rotate view so selected pieces are at center"),
		""
	},
	// LC_VIEW_MOVE_FORWARD
	{
		QT_TRANSLATE_NOOP("Action", "View.MoveForward"),
		QT_TRANSLATE_NOOP("Menu", "Move Forward"),
		QT_TRANSLATE_NOOP("Status", "Move the current view forward"),
		""
	},
	// LC_VIEW_MOVE_BACKWARD
	{
		QT_TRANSLATE_NOOP("Action", "View.MoveBackward"),
		QT_TRANSLATE_NOOP("Menu", "Move Backward"),
		QT_TRANSLATE_NOOP("Status", "Move the current view backward"),
		""
	},
	// LC_VIEW_MOVE_LEFT
	{
		QT_TRANSLATE_NOOP("Action", "View.MoveLeft"),
		QT_TRANSLATE_NOOP("Menu", "Move Left"),
		QT_TRANSLATE_NOOP("Status", "Move the current view to the left"),
		""
	},
	// LC_VIEW_MOVE_RIGHT
	{
		QT_TRANSLATE_NOOP("Action", "View.MoveRight"),
		QT_TRANSLATE_NOOP("Menu", "Move Right"),
		QT_TRANSLATE_NOOP("Status", "Move the current view to the right"),
		""
	},
	// LC_VIEW_MOVE_UP
	{
		QT_TRANSLATE_NOOP("Action", "View.MoveUp"),
		QT_TRANSLATE_NOOP("Menu", "Move Up"),
		QT_TRANSLATE_NOOP("Status", "Move the current view up"),
		""
	},
	// LC_VIEW_MOVE_DOWN
	{
		QT_TRANSLATE_NOOP("Action", "View.MoveDown"),
		QT_TRANSLATE_NOOP("Menu", "Move Down"),
		QT_TRANSLATE_NOOP("Status", "Move the current view down"),
		""
	},
	// LC_VIEW_VIEWPOINT_FRONT
	{
		QT_TRANSLATE_NOOP("Action", "View.Viewpoint.Front"),
		QT_TRANSLATE_NOOP("Menu", "&Front"),
		QT_TRANSLATE_NOOP("Status", "View model from the front"),
		"F"
	},
	// LC_VIEW_VIEWPOINT_BACK
	{
		QT_TRANSLATE_NOOP("Action", "View.Viewpoint.Back"),
		QT_TRANSLATE_NOOP("Menu", "&Back"),
		QT_TRANSLATE_NOOP("Status", "View model from the back"),
		"B"
	},
	// LC_VIEW_VIEWPOINT_TOP
	{
		QT_TRANSLATE_NOOP("Action", "View.Viewpoint.Top"),
		QT_TRANSLATE_NOOP("Menu", "&Top"),
		QT_TRANSLATE_NOOP("Status", "View model from the top"),
		"T"
	},
	// LC_VIEW_VIEWPOINT_BOTTOM
	{
		QT_TRANSLATE_NOOP("Action", "View.Viewpoint.Bottom"),
		QT_TRANSLATE_NOOP("Menu", "B&ottom"),
		QT_TRANSLATE_NOOP("Status", "View model from the bottom"),
		"O"
	},
	// LC_VIEW_VIEWPOINT_LEFT
	{
		QT_TRANSLATE_NOOP("Action", "View.Viewpoint.Left"),
		QT_TRANSLATE_NOOP("Menu", "&Left"),
		QT_TRANSLATE_NOOP("Status", "View model from the left"),
		"L"
	},
	// LC_VIEW_VIEWPOINT_RIGHT
	{
		QT_TRANSLATE_NOOP("Action", "View.Viewpoint.Right"),
		QT_TRANSLATE_NOOP("Menu", "&Right"),
		QT_TRANSLATE_NOOP("Status", "View model from the right"),
		"R"
	},
	// LC_VIEW_VIEWPOINT_HOME
	{
		QT_TRANSLATE_NOOP("Action", "View.Viewpoint.Home"),
		QT_TRANSLATE_NOOP("Menu", "&Home"),
		QT_TRANSLATE_NOOP("Status", "View model from the default position"),
		"H"
	},
	// LC_VIEW_CAMERA_NONE
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.None"),
		QT_TRANSLATE_NOOP("Menu", "None"),
		QT_TRANSLATE_NOOP("Status", "Do not use a camera"),
		""
	},
	// LC_VIEW_CAMERA1
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera01"),
		QT_TRANSLATE_NOOP("Menu", "Camera 1"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA2
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera02"),
		QT_TRANSLATE_NOOP("Menu", "Camera 2"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA3
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera03"),
		QT_TRANSLATE_NOOP("Menu", "Camera 3"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA4
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera04"),
		QT_TRANSLATE_NOOP("Menu", "Camera 4"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA5
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera05"),
		QT_TRANSLATE_NOOP("Menu", "Camera 5"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA6
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera06"),
		QT_TRANSLATE_NOOP("Menu", "Camera 6"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA7
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera07"),
		QT_TRANSLATE_NOOP("Menu", "Camera 7"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA8
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera08"),
		QT_TRANSLATE_NOOP("Menu", "Camera 8"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA9
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera09"),
		QT_TRANSLATE_NOOP("Menu", "Camera 9"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA10
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera10"),
		QT_TRANSLATE_NOOP("Menu", "Camera 10"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA11
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera11"),
		QT_TRANSLATE_NOOP("Menu", "Camera 11"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA12
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera12"),
		QT_TRANSLATE_NOOP("Menu", "Camera 12"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA13
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera13"),
		QT_TRANSLATE_NOOP("Menu", "Camera 13"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA14
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera14"),
		QT_TRANSLATE_NOOP("Menu", "Camera 14"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA15
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera15"),
		QT_TRANSLATE_NOOP("Menu", "Camera 15"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA16
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Camera16"),
		QT_TRANSLATE_NOOP("Menu", "Camera 16"),
		QT_TRANSLATE_NOOP("Status", "Use this camera"),
		""
	},
	// LC_VIEW_CAMERA_RESET
	{
		QT_TRANSLATE_NOOP("Action", "View.Cameras.Reset"),
		QT_TRANSLATE_NOOP("Menu", "Reset"),
		QT_TRANSLATE_NOOP("Status", "Reset views to their default positions"),
		""
	},
	// LC_VIEW_TIME_FIRST
	{
		QT_TRANSLATE_NOOP("Action", "View.Time.First"),
		QT_TRANSLATE_NOOP("Menu", "First"),
		QT_TRANSLATE_NOOP("Status", "Go to the first step of the model"),
		"Alt+Up"
	},
	// LC_VIEW_TIME_PREVIOUS
	{
		QT_TRANSLATE_NOOP("Action", "View.Time.Previous"),
		QT_TRANSLATE_NOOP("Menu", "Previous"),
		QT_TRANSLATE_NOOP("Status", "Go to the previous step"),
		"Alt+Left"
	},
	// LC_VIEW_TIME_NEXT
	{
		QT_TRANSLATE_NOOP("Action", "View.Time.Next"),
		QT_TRANSLATE_NOOP("Menu", "Next"),
		QT_TRANSLATE_NOOP("Status", "Go to the next step"),
		"Alt+Right"
	},
	// LC_VIEW_TIME_LAST
	{
		QT_TRANSLATE_NOOP("Action", "View.Time.Last"),
		QT_TRANSLATE_NOOP("Menu", "Last"),
		QT_TRANSLATE_NOOP("Status", "Go to the last step of the model"),
		"Alt+Down"
	},
	// LC_VIEW_TIME_INSERT_BEFORE
	{
		QT_TRANSLATE_NOOP("Action", "View.Time.InsertBefore"),
		QT_TRANSLATE_NOOP("Menu", "Insert Before"),
		QT_TRANSLATE_NOOP("Status", "Insert a new step before the current step"),
		""
	},
	// LC_VIEW_TIME_INSERT_AFTER
	{
		QT_TRANSLATE_NOOP("Action", "View.Time.InsertAfter"),
		QT_TRANSLATE_NOOP("Menu", "Insert After"),
		QT_TRANSLATE_NOOP("Status", "Insert a new step after the current step"),
		""
	},
	// LC_VIEW_TIME_DELETE
	{
		QT_TRANSLATE_NOOP("Action", "View.Time.Delete"),
		QT_TRANSLATE_NOOP("Menu", "Remove Step"),
		QT_TRANSLATE_NOOP("Status", "Remove current step"),
		""
	},
	// LC_VIEW_TIME_ADD_KEYS
	{
		QT_TRANSLATE_NOOP("Action", "View.Time.AddKeys"),
		QT_TRANSLATE_NOOP("Menu", "Add Keys"),
		QT_TRANSLATE_NOOP("Status", "Toggle adding new animation keys"),
		""
	},
	// LC_VIEW_SPLIT_HORIZONTAL
	{
		QT_TRANSLATE_NOOP("Action", "View.SplitHorizontal"),
		QT_TRANSLATE_NOOP("Menu", "Split &Horizontal"),
		QT_TRANSLATE_NOOP("Status", "Split the current view horizontally"),
		""
	},
	// LC_VIEW_SPLIT_VERTICAL
	{
		QT_TRANSLATE_NOOP("Action", "View.SplitVertical"),
		QT_TRANSLATE_NOOP("Menu", "Split &Vertical"),
		QT_TRANSLATE_NOOP("Status", "Split the current view vertically"),
		""
	},
	// LC_VIEW_REMOVE_VIEW
	{
		QT_TRANSLATE_NOOP("Action", "View.RemoveView"),
		QT_TRANSLATE_NOOP("Menu", "Re&move View"),
		QT_TRANSLATE_NOOP("Status", "Remove the current view"),
		""
	},
	// LC_VIEW_RESET_VIEWS
	{
		QT_TRANSLATE_NOOP("Action", "View.ResetViews"),
		QT_TRANSLATE_NOOP("Menu", "Rese&t Views"),
		QT_TRANSLATE_NOOP("Status", "Reset all views"),
		""
	},
	// LC_VIEW_FULLSCREEN
	{
		QT_TRANSLATE_NOOP("Action", "View.FullScreen"),
		QT_TRANSLATE_NOOP("Menu", "&Full Screen"),
		QT_TRANSLATE_NOOP("Status", "Toggle fullscreen mode"),
		""
	},
	// LC_VIEW_CLOSE_CURRENT_TAB
	{
		QT_TRANSLATE_NOOP("Action", "View.CloseCurrentTab"),
		QT_TRANSLATE_NOOP("Menu", "Close &Tab"),
		QT_TRANSLATE_NOOP("Status", "Close current tab"),
		"Ctrl+W"
	},
	// LC_VIEW_SHADE_WIREFRAME
	{
		QT_TRANSLATE_NOOP("Action", "View.Shade.Wireframe"),
		QT_TRANSLATE_NOOP("Menu", "&Wireframe"),
		QT_TRANSLATE_NOOP("Status", "Display the scene as wireframe"),
		""
	},
	// LC_VIEW_SHADE_FLAT
	{
		QT_TRANSLATE_NOOP("Action", "View.Shade.Flat"),
		QT_TRANSLATE_NOOP("Menu", "&Flat Shading"),
		QT_TRANSLATE_NOOP("Status", "Display the scene without any shading or lights"),
		""
	},
	// LC_VIEW_SHADE_DEFAULT_LIGHTS
	{
		QT_TRANSLATE_NOOP("Action", "View.Shade.DefaultLights"),
		QT_TRANSLATE_NOOP("Menu", "&Default Lights"),
		QT_TRANSLATE_NOOP("Status", "Display the scene with the default lights"),
		""
	},
	// LC_VIEW_PROJECTION_PERSPECTIVE
	{
		QT_TRANSLATE_NOOP("Action", "View.Projection.Perspective"),
		QT_TRANSLATE_NOOP("Menu", "&Perspective"),
		QT_TRANSLATE_NOOP("Status", "Set the current camera to use a perspective projection"),
		""
	},
	// LC_VIEW_PROJECTION_ORTHO
	{
		QT_TRANSLATE_NOOP("Action", "View.Projection.Orthographic"),
		QT_TRANSLATE_NOOP("Menu", "&Orthographic"),
		QT_TRANSLATE_NOOP("Status", "Set the current camera to use an orthographic projection"),
		""
	},
	// LC_VIEW_TOGGLE_VIEW_SPHERE
	{
		QT_TRANSLATE_NOOP("Action", "View.ToggleViewSphere"),
		QT_TRANSLATE_NOOP("Menu", "View Sphere"),
		QT_TRANSLATE_NOOP("Status", "Toggle the view sphere"),
		""
	},
	// LC_VIEW_FADE_PREVIOUS_STEPS
	{
		QT_TRANSLATE_NOOP("Action", "View.FadePreviousSteps"),
		QT_TRANSLATE_NOOP("Menu", "Fade Previous Steps"),
		QT_TRANSLATE_NOOP("Status", "Toggle fading previous model steps"),
		""
	},
	// LC_PIECE_INSERT
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Insert"),
		QT_TRANSLATE_NOOP("Menu", "&Insert"),
		QT_TRANSLATE_NOOP("Status", "Add a new piece to the model"),
		"Insert"
	},
	// LC_PIECE_DELETE
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Delete"),
		QT_TRANSLATE_NOOP("Menu", "&Delete"),
		QT_TRANSLATE_NOOP("Status", "Delete selected objects"),
		"Delete"
	},
	// LC_PIECE_DUPLICATE
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Duplicate"),
		QT_TRANSLATE_NOOP("Menu", "&Duplicate"),
		QT_TRANSLATE_NOOP("Status", "Create a copy of the selected pieces"),
		"Ctrl+D"
	},
	// LC_PIECE_RESET_PIVOT_POINT
	{
		QT_TRANSLATE_NOOP("Action", "Piece.ResetPivotPoint"),
		QT_TRANSLATE_NOOP("Menu", "Reset &Pivot Point"),
		QT_TRANSLATE_NOOP("Status", "Reset the pivot point of the selected pieces to their origin"),
		""
	},
	// LC_PIECE_REMOVE_KEY_FRAMES
	{
		QT_TRANSLATE_NOOP("Action", "Piece.RemoveKeyFrames"),
		QT_TRANSLATE_NOOP("Menu", "Remove &Key Frames"),
		QT_TRANSLATE_NOOP("Status", "Remove all key frames from the selected pieces"),
		""
	},
	// LC_PIECE_CONTROL_POINT_INSERT
	{
		QT_TRANSLATE_NOOP("Action", "Piece.ControlPoint.Insert"),
		QT_TRANSLATE_NOOP("Menu", "Insert Control Point"),
		QT_TRANSLATE_NOOP("Status", "Insert a new control point"),
		""
	},
	// LC_PIECE_CONTROL_POINT_REMOVE
	{
		QT_TRANSLATE_NOOP("Action", "Piece.ControlPoint.Remove"),
		QT_TRANSLATE_NOOP("Menu", "Remove Control Point"),
		QT_TRANSLATE_NOOP("Status", "Remove the selected control point"),
		""
	},
	// LC_PIECE_MOVE_PLUSX
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Move.PlusX"),
		QT_TRANSLATE_NOOP("Menu", "Move +X"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the X axis"),
		"Down"
	},
	// LC_PIECE_MOVE_MINUSX
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Move.MinusX"),
		QT_TRANSLATE_NOOP("Menu", "Move -X"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the X axis"),
		"Up"
	},
	// LC_PIECE_MOVE_PLUSY
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Move.PlusY"),
		QT_TRANSLATE_NOOP("Menu", "Move +Y"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the Y axis"),
		"Right"
	},
	// LC_PIECE_MOVE_MINUSY
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Move.MinusY"),
		QT_TRANSLATE_NOOP("Menu", "Move -Y"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the Y axis"),
		"Left"
	},
	// LC_PIECE_MOVE_PLUSZ
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Move.PlusZ"),
		QT_TRANSLATE_NOOP("Menu", "Move +Z"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the Z axis"),
		"PgUp"
	},
	// LC_PIECE_MOVE_MINUSZ
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Move.MinusZ"),
		QT_TRANSLATE_NOOP("Menu", "Move -Z"),
		QT_TRANSLATE_NOOP("Status", "Move selected objects along the Z axis"),
		"PgDown"
	},
	// LC_PIECE_ROTATE_PLUSX
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Rotate.PlusX"),
		QT_TRANSLATE_NOOP("Menu", "Rotate +X"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the X axis"),
		"Shift+Down"
	},
	// LC_PIECE_ROTATE_MINUSX
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Rotate.MinusX"),
		QT_TRANSLATE_NOOP("Menu", "Rotate -X"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the X axis"),
		"Shift+Up"
	},
	// LC_PIECE_ROTATE_PLUSY
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Rotate.PlusY"),
		QT_TRANSLATE_NOOP("Menu", "Rotate +Y"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the Y axis"),
		"Shift+Right"
	},
	// LC_PIECE_ROTATE_MINUSY
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Rotate.MinusY"),
		QT_TRANSLATE_NOOP("Menu", "Rotate -Y"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the Y axis"),
		"Shift+Left"
	},
	// LC_PIECE_ROTATE_PLUSZ
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Rotate.PlusZ"),
		QT_TRANSLATE_NOOP("Menu", "Rotate +Z"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the Z axis"),
		"Shift+PgUp"
	},
	// LC_PIECE_ROTATE_MINUSZ
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Rotate.MinusZ"),
		QT_TRANSLATE_NOOP("Menu", "Rotate -Z"),
		QT_TRANSLATE_NOOP("Status", "Rotate selected objects along the Z axis"),
		"Shift+PgDown"
	},
	// LC_PIECE_MINIFIG_WIZARD
	{
		QT_TRANSLATE_NOOP("Action", "Piece.MinifigWizard"),
		QT_TRANSLATE_NOOP("Menu", "Minifig &Wizard..."),
		QT_TRANSLATE_NOOP("Status", "Add a new minifig to the model"),
		""
	},
	// LC_PIECE_ARRAY
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Array"),
		QT_TRANSLATE_NOOP("Menu", "A&rray..."),
		QT_TRANSLATE_NOOP("Status", "Make copies of the selected pieces"),
		""
	},
	// 	LC_PIECE_VIEW_SELECTED_MODEL
	{
		QT_TRANSLATE_NOOP("Action", "Piece.ViewSelectedModel"),
		QT_TRANSLATE_NOOP("Menu", "Open Selected Model"),
		QT_TRANSLATE_NOOP("Status", "Open the model referenced by the selected piece in a new tab"),
		""
	},
	// LC_PIECE_MOVE_SELECTION_TO_MODEL
	{
		QT_TRANSLATE_NOOP("Action", "Piece.MoveSelectionToModel"),
		QT_TRANSLATE_NOOP("Menu", "Move to New Model..."),
		QT_TRANSLATE_NOOP("Status", "Move the selected pieces to a new model and replace them with a reference to the model"),
		""
	},
	// LC_PIECE_INLINE_SELECTED_MODELS
	{
		QT_TRANSLATE_NOOP("Action", "Piece.InlineSelectedModels"),
		QT_TRANSLATE_NOOP("Menu", "Inline Selected Models"),
		QT_TRANSLATE_NOOP("Status", "Insert the contents of the selected model references into the current model"),
		""
	},
	// LC_PIECE_EDIT_SELECTED_SUBMODEL
	{
		QT_TRANSLATE_NOOP("Action", "Piece.EditSelectedSubmodel"),
		QT_TRANSLATE_NOOP("Menu", "Edit Selected Submodel"),
		QT_TRANSLATE_NOOP("Status", "Edit the currently selected submodel in-place"),
		""
	},
	// LC_PIECE_EDIT_END_SUBMODEL
	{
		QT_TRANSLATE_NOOP("Action", "Piece.EditEndSubmodel"),
		QT_TRANSLATE_NOOP("Menu", "End Submodel Editing"),
		QT_TRANSLATE_NOOP("Status", "End in-place submodel editing"),
		""
	},
	// LC_PIECE_GROUP
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Group"),
		QT_TRANSLATE_NOOP("Menu", "&Group..."),
		QT_TRANSLATE_NOOP("Status", "Group selected pieces together"),
		"Ctrl+G"
	},
	// LC_PIECE_UNGROUP
	{
		QT_TRANSLATE_NOOP("Action", "Piece.Ungroup"),
		QT_TRANSLATE_NOOP("Menu", "&Ungroup"),
		QT_TRANSLATE_NOOP("Status", "Ungroup selected group"),
		"Ctrl+U"
	},
	// LC_PIECE_GROUP_ADD
	{
		QT_TRANSLATE_NOOP("Action", "Piece.GroupAdd"),
		QT_TRANSLATE_NOOP("Menu", "&Add to Group"),
		QT_TRANSLATE_NOOP("Status", "Add focused piece to selected group"),
		""
	},
	// LC_PIECE_GROUP_REMOVE
	{
		QT_TRANSLATE_NOOP("Action", "Piece.GroupRemove"),
		QT_TRANSLATE_NOOP("Menu", "Re&move from Group"),
		QT_TRANSLATE_NOOP("Status", "Remove focused piece from group"),
		""
	},
	// LC_PIECE_GROUP_EDIT
	{
		QT_TRANSLATE_NOOP("Action", "Piece.GroupEdit"),
		QT_TRANSLATE_NOOP("Menu", "&Edit Groups..."),
		QT_TRANSLATE_NOOP("Status", "Edit groups"),
		""
	},
	// LC_PIECE_HIDE_SELECTED
	{
		QT_TRANSLATE_NOOP("Action", "Piece.HideSelected"),
		QT_TRANSLATE_NOOP("Menu", "&Hide Selected"),
		QT_TRANSLATE_NOOP("Status", "Hide selected objects"),
		"Ctrl+H"
	},
	// LC_PIECE_HIDE_UNSELECTED
	{
		QT_TRANSLATE_NOOP("Action", "Piece.HideUnselected"),
		QT_TRANSLATE_NOOP("Menu", "Hide &Unselected"),
		QT_TRANSLATE_NOOP("Status", "Hide objects that are not selected"),
		""
	},
	// LC_PIECE_UNHIDE_SELECTED
	{
		QT_TRANSLATE_NOOP("Action", "Piece.UnhideSelected"),
		QT_TRANSLATE_NOOP("Menu", "&Unhide Selected"),
		QT_TRANSLATE_NOOP("Status", "Show hidden objects that are selected"),
		""
	},
	// LC_PIECE_UNHIDE_ALL
	{
		QT_TRANSLATE_NOOP("Action", "Piece.UnhideAll"),
		QT_TRANSLATE_NOOP("Menu", "U&nhide All"),
		QT_TRANSLATE_NOOP("Status", "Show all hidden objects"),
		""
	},
	// LC_PIECE_SHOW_EARLIER
	{
		QT_TRANSLATE_NOOP("Action", "Piece.ShowEarlier"),
		QT_TRANSLATE_NOOP("Menu", "Show Earlier"),
		QT_TRANSLATE_NOOP("Status", "Show selected pieces one step earlier"),
		""
	},
	// LC_PIECE_SHOW_LATER
	{
		QT_TRANSLATE_NOOP("Action", "Piece.ShowLater"),
		QT_TRANSLATE_NOOP("Menu", "Show Later"),
		QT_TRANSLATE_NOOP("Status", "Show selected pieces one step later"),
		""
	},
	// LC_MODEL_NEW
	{
		QT_TRANSLATE_NOOP("Action", "Model.New"),
		QT_TRANSLATE_NOOP("Menu", "New Submodel..."),
		QT_TRANSLATE_NOOP("Status", "Create a new submodel"),
		""
	},
	// LC_MODEL_PROPERTIES
	{
		QT_TRANSLATE_NOOP("Action", "Model.Properties"),
		QT_TRANSLATE_NOOP("Menu", "Prope&rties..."),
		QT_TRANSLATE_NOOP("Status", "Display the properties of the current submodel"),
		""
	},
	// LC_MODEL_LIST
	{
		QT_TRANSLATE_NOOP("Action", "Model.List"),
		QT_TRANSLATE_NOOP("Menu", "Submodels..."),
		QT_TRANSLATE_NOOP("Status", "Show a list of all submodels"),
		""
	},
	// LC_MODEL_01
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model01"),
		QT_TRANSLATE_NOOP("Menu", "Model 1"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_02
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model02"),
		QT_TRANSLATE_NOOP("Menu", "Model 2"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_03
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model03"),
		QT_TRANSLATE_NOOP("Menu", "Model 3"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_04
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model04"),
		QT_TRANSLATE_NOOP("Menu", "Model 4"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_05
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model05"),
		QT_TRANSLATE_NOOP("Menu", "Model 5"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_06
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model06"),
		QT_TRANSLATE_NOOP("Menu", "Model 6"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_07
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model07"),
		QT_TRANSLATE_NOOP("Menu", "Model 7"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_08
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model08"),
		QT_TRANSLATE_NOOP("Menu", "Model 8"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_09
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model09"),
		QT_TRANSLATE_NOOP("Menu", "Model 9"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_10
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model10"),
		QT_TRANSLATE_NOOP("Menu", "Model 10"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_11
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model11"),
		QT_TRANSLATE_NOOP("Menu", "Model 11"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_12
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model12"),
		QT_TRANSLATE_NOOP("Menu", "Model 12"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_13
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model13"),
		QT_TRANSLATE_NOOP("Menu", "Model 13"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_14
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model14"),
		QT_TRANSLATE_NOOP("Menu", "Model 14"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_15
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model15"),
		QT_TRANSLATE_NOOP("Menu", "Model 15"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_16
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model16"),
		QT_TRANSLATE_NOOP("Menu", "Model 16"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_17
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model17"),
		QT_TRANSLATE_NOOP("Menu", "Model 17"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_18
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model18"),
		QT_TRANSLATE_NOOP("Menu", "Model 18"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_19
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model19"),
		QT_TRANSLATE_NOOP("Menu", "Model 19"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_20
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model20"),
		QT_TRANSLATE_NOOP("Menu", "Model 20"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_21
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model21"),
		QT_TRANSLATE_NOOP("Menu", "Model 21"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_22
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model22"),
		QT_TRANSLATE_NOOP("Menu", "Model 22"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_23
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model23"),
		QT_TRANSLATE_NOOP("Menu", "Model 23"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_MODEL_24
	{
		QT_TRANSLATE_NOOP("Action", "Model.Model24"),
		QT_TRANSLATE_NOOP("Menu", "Model 24"),
		QT_TRANSLATE_NOOP("Status", "Switch to this submodel"),
		""
	},
	// LC_HELP_HOMEPAGE
	{
		QT_TRANSLATE_NOOP("Action", "Help.HomePage"),
		QT_TRANSLATE_NOOP("Menu", "LeoCAD &Home Page"),
		QT_TRANSLATE_NOOP("Status", "Open LeoCAD's home page on the internet using your default web browser"),
		""
	},
	// LC_HELP_BUG_REPORT
	{
		QT_TRANSLATE_NOOP("Action", "Help.BugReport"),
		QT_TRANSLATE_NOOP("Menu", "Report a Bug"),
		QT_TRANSLATE_NOOP("Status", "Open LeoCAD's bug report form on your default web browser"),
		""
	},
	// LC_HELP_UPDATES
	{
		QT_TRANSLATE_NOOP("Action", "Help.Updates"),
		QT_TRANSLATE_NOOP("Menu", "Check for &Updates..."),
		QT_TRANSLATE_NOOP("Status", "Check if a newer LeoCAD version or parts library has been released"),
		""
	},
	// LC_HELP_ABOUT
	{
		QT_TRANSLATE_NOOP("Action", "Help.About"),
		QT_TRANSLATE_NOOP("Menu", "&About..."),
		QT_TRANSLATE_NOOP("Status", "Display program version number and system information"),
		""
	},
	// LC_TIMELINE_INSERT_BEFORE
	{
		"",
		QT_TRANSLATE_NOOP("Menu", "Insert Step Before"),
		QT_TRANSLATE_NOOP("Status", "Insert a new step before the current step"),
		""
	},
	// LC_TIMELINE_INSERT_AFTER
	{
		"",
		QT_TRANSLATE_NOOP("Menu", "Insert Step After"),
		QT_TRANSLATE_NOOP("Status", "Insert a new step after the current step"),
		""
	},
	// LC_TIMELINE_DELETE
	{
		"",
		QT_TRANSLATE_NOOP("Menu", "Remove Step"),
		QT_TRANSLATE_NOOP("Status", "Remove current step"),
		""
	},
	// LC_TIMELINE_MOVE_SELECTION
	{
		"",
		QT_TRANSLATE_NOOP("Menu", "Move Selection Here"),
		QT_TRANSLATE_NOOP("Status", "Move the selected parts into this step"),
		""
	},
	// LC_TIMELINE_SET_CURRENT
	{
		"",
		QT_TRANSLATE_NOOP("Menu", "Set Current Step"),
		QT_TRANSLATE_NOOP("Status", "View the model at this point in the timeline"),
		""
	}
};

static_assert(sizeof(gCommands)/sizeof(gCommands[0]) == LC_NUM_COMMANDS, "Array size mismatch.");

const char* gToolNames[LC_NUM_TOOLS] =
{
	QT_TRANSLATE_NOOP("Mouse", "NewPiece"),      // LC_TOOL_INSERT
	QT_TRANSLATE_NOOP("Mouse", "NewPointLight"), // LC_TOOL_LIGHT
	QT_TRANSLATE_NOOP("Mouse", "NewSpotLight"),  // LC_TOOL_SPOTLIGHT
	QT_TRANSLATE_NOOP("Mouse", "NewCamera"),     // LC_TOOL_CAMERA
	QT_TRANSLATE_NOOP("Mouse", "Select"),        // LC_TOOL_SELECT
	QT_TRANSLATE_NOOP("Mouse", "Move"),          // LC_TOOL_MOVE
	QT_TRANSLATE_NOOP("Mouse", "Rotate"),        // LC_TOOL_ROTATE
	QT_TRANSLATE_NOOP("Mouse", "Delete"),        // LC_TOOL_ERASER
	QT_TRANSLATE_NOOP("Mouse", "Paint"),         // LC_TOOL_PAINT
    QT_TRANSLATE_NOOP("Mouse", "ColorPicker"),   // LC_TOOL_COLOR_PICKER
	QT_TRANSLATE_NOOP("Mouse", "Zoom"),          // LC_TOOL_ZOOM
	QT_TRANSLATE_NOOP("Mouse", "Pan"),           // LC_TOOL_PAN
	QT_TRANSLATE_NOOP("Mouse", "Orbit"),         // LC_TOOL_ROTATE_VIEW
	QT_TRANSLATE_NOOP("Mouse", "Roll"),          // LC_TOOL_ROLL
	QT_TRANSLATE_NOOP("Mouse", "ZoomRegion")     // LC_TOOL_ZOOM_REGION
};

static_assert(LC_ARRAY_COUNT(gToolNames) == LC_NUM_TOOLS, "Array size mismatch.");

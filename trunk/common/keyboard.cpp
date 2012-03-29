//
// Code to handle user-defined keyboard shortcuts.
//

#include "lc_global.h"
#include <stdio.h>
#include "system.h"
#include "keyboard.h"
#include "lc_file.h"
#include "str.h"

// ============================================================================
// Globals.

LC_KEYBOARD_COMMAND DefaultKeyboardShortcuts[] = 
{
	{ LC_FILE_NEW, "New Project", LC_KEYMOD1_CONTROL, LC_KEY_N, 0 },
	{ LC_FILE_OPEN, "Open Project", LC_KEYMOD1_CONTROL, LC_KEY_O, 0 },
	{ LC_FILE_MERGE, "Merge Project", 0, 0, 0 },
	{ LC_FILE_SAVE, "Save Project", LC_KEYMOD1_CONTROL, LC_KEY_S, 0 },
	{ LC_FILE_SAVEAS, "Save Project As", 0, 0, 0 },
	{ LC_FILE_PICTURE, "Save Picture", 0, 0, 0 },
	{ LC_FILE_3DS, "Export 3D Studio", 0, 0, 0 },
	{ LC_FILE_HTML, "Export HTML", 0, 0, 0 },
	{ LC_FILE_POVRAY, "Export POV-Ray", 0, 0, 0 },
	{ LC_FILE_WAVEFRONT, "Export Wavefront", 0, 0, 0 },
	{ LC_FILE_PROPERTIES, "Project Properties", 0, 0, 0 },
//	{ LC_FILE_TERRAIN, "Terrain Editor", 0, 0, 0 },
	{ LC_FILE_LIBRARY, "Piece Library Manager", 0, 0, 0 },
//	{ LC_FILE_RECENT, "Open Recent File", 0, 0, 0 },
	{ LC_EDIT_UNDO, "Undo", LC_KEYMOD1_CONTROL, LC_KEY_Z, 0 },
	{ LC_EDIT_REDO, "Redo", LC_KEYMOD1_CONTROL, LC_KEY_Y, 0 },
	{ LC_EDIT_CUT, "Cut", LC_KEYMOD1_CONTROL, LC_KEY_X, 0 },
	{ LC_EDIT_COPY, "Copy", LC_KEYMOD1_CONTROL, LC_KEY_C, 0 },
	{ LC_EDIT_PASTE, "Paste", LC_KEYMOD1_CONTROL, LC_KEY_V, 0 },
	{ LC_EDIT_SELECT_ALL, "Select All", LC_KEYMOD1_CONTROL, LC_KEY_A, 0 },
	{ LC_EDIT_SELECT_NONE, "Select None", 0, 0, 0 },
	{ LC_EDIT_SELECT_INVERT, "Select Invert", LC_KEYMOD1_CONTROL, LC_KEY_I, 0 },
	{ LC_EDIT_SELECT_BYNAME, "Select By Name", 0, 0, 0 },
	{ LC_PIECE_INSERT, "Piece Insert", 0, LC_KEY_INSERT, 0 },
	{ LC_PIECE_DELETE, "Piece Delete", 0, LC_KEY_DELETE, 0 },
//	{ LC_PIECE_MINIFIG, "Minifig Wizard", 0, 0, 0 },
	{ LC_PIECE_ARRAY, "Piece Array", 0, 0, 0 },
//	{ LC_PIECE_COPYKEYS, "", 0, 0, 0 },
	{ LC_PIECE_GROUP, "Piece Group", LC_KEYMOD1_CONTROL, LC_KEY_G, 0 },
	{ LC_PIECE_UNGROUP, "Piece Ungroup", LC_KEYMOD1_CONTROL, LC_KEY_U, 0 },
	{ LC_PIECE_GROUP_ADD, "Group Add Piece", 0, 0, 0 },
	{ LC_PIECE_GROUP_REMOVE, "Group Remove Piece", 0, 0, 0 },
	{ LC_PIECE_GROUP_EDIT, "Group Edit", 0, 0, 0 },
	{ LC_PIECE_HIDE_SELECTED, "Hide Selection", LC_KEYMOD1_CONTROL, LC_KEY_H, 0 },
	{ LC_PIECE_HIDE_UNSELECTED, "Unhide Selection", 0, 0, 0 },
	{ LC_PIECE_UNHIDE_ALL, "Unhide All", 0, 0, 0 },
	{ LC_PIECE_PREVIOUS, "Piece Previous Step", 0, 0, 0 },
	{ LC_PIECE_NEXT, "Piece Next Step", 0, 0, 0 },
	{ LC_VIEW_PREFERENCES, "Preferences", 0, 0, 0 },
//	{ LC_VIEW_ZOOM, "", 0, 0, 0 },
	{ LC_VIEW_ZOOMIN, "Zoom In", 0, 0, 0 },
	{ LC_VIEW_ZOOMOUT, "Zoom Out", 0, 0, 0 },
	{ LC_VIEW_ZOOMEXTENTS, "Zoom Extents", 0, 0, 0 },
	{ LC_VIEW_STEP_NEXT, "Step Next", 0, 0, 0 },
	{ LC_VIEW_STEP_PREVIOUS, "Step Previous", 0, 0, 0 },
	{ LC_VIEW_STEP_FIRST, "Step First", 0, 0, 0 },
	{ LC_VIEW_STEP_LAST, "Step Last", 0, 0, 0 },
//	{ LC_VIEW_STEP_CHOOSE, "", 0, 0, 0 },
//	{ LC_VIEW_STEP_SET, "", 0, 0, 0 },
//	{ LC_VIEW_STOP, "", 0, 0, 0 },
//	{ LC_VIEW_PLAY, "", 0, 0, 0 },
	{ LC_VIEW_CAMERA_FRONT, "Camera Front", LC_KEYMOD_VIEWONLY, LC_KEY_F, 0 },
	{	LC_VIEW_CAMERA_BACK, "Camera Back", LC_KEYMOD_VIEWONLY, LC_KEY_B, 0 },
	{	LC_VIEW_CAMERA_TOP, "Camera Top", LC_KEYMOD_VIEWONLY, LC_KEY_T, 0 },
	{ LC_VIEW_CAMERA_BOTTOM, "Camera Bottom", LC_KEYMOD_VIEWONLY, LC_KEY_O, 0 },
	{	LC_VIEW_CAMERA_LEFT, "Camera Left", LC_KEYMOD_VIEWONLY, LC_KEY_L, 0 },
	{	LC_VIEW_CAMERA_RIGHT, "Camera Right", LC_KEYMOD_VIEWONLY, LC_KEY_R, 0 },
	{	LC_VIEW_CAMERA_MAIN, "Camera Main", LC_KEYMOD_VIEWONLY, LC_KEY_M, 0 },
//	{ LC_VIEW_CAMERA_MENU, "", 0, 0, 0 },
//	{ LC_VIEW_CAMERA_RESET, "", 0, 0, 0 },
//	{ LC_HELP_ABOUT, "", 0, 0, 0 },
//	{ LC_TOOLBAR_ANIMATION, "", 0, 0, 0 },
//	{ LC_TOOLBAR_ADDKEYS, "", 0, 0, 0 },
//	{ LC_TOOLBAR_SNAPMENU, "", 0, 0, 0 },
//	{ LC_TOOLBAR_LOCKMENU, "", 0, 0, 0 },
//	{ LC_TOOLBAR_FASTRENDER, "", 0, 0, 0 },
	{ LC_VIEW_STEP_INSERT, "Step Insert", 0, 0, 0 },
	{ LC_VIEW_STEP_DELETE, "Step Delete", 0, 0, 0 },
	{ LC_EDIT_MOVEXY_SNAP_0, "Move XY Snap 0", 0, LC_KEY_0, 0 },
	{ LC_EDIT_MOVEXY_SNAP_1, "Move XY Snap 1", 0, LC_KEY_1, 0 },
	{ LC_EDIT_MOVEXY_SNAP_2, "Move XY Snap 2", 0, LC_KEY_2, 0 },
	{ LC_EDIT_MOVEXY_SNAP_3, "Move XY Snap 3", 0, LC_KEY_3, 0 },
	{ LC_EDIT_MOVEXY_SNAP_4, "Move XY Snap 4", 0, LC_KEY_4, 0 },
	{ LC_EDIT_MOVEXY_SNAP_5, "Move XY Snap 5", 0, LC_KEY_5, 0 },
	{ LC_EDIT_MOVEXY_SNAP_6, "Move XY Snap 6", 0, LC_KEY_6, 0 },
	{ LC_EDIT_MOVEXY_SNAP_7, "Move XY Snap 7", 0, LC_KEY_7, 0 },
	{ LC_EDIT_MOVEXY_SNAP_8, "Move XY Snap 8", 0, LC_KEY_8, 0 },
	{ LC_EDIT_MOVEXY_SNAP_9, "Move XY Snap 9", 0, LC_KEY_9, 0 },
	{ LC_EDIT_MOVEZ_SNAP_0, "Move Z Snap 0", LC_KEYMOD1_SHIFT|LC_KEYMOD1_CONTROL, LC_KEY_0, 0 },
	{ LC_EDIT_MOVEZ_SNAP_1, "Move Z Snap 1", LC_KEYMOD1_SHIFT|LC_KEYMOD1_CONTROL, LC_KEY_1, 0 },
	{ LC_EDIT_MOVEZ_SNAP_2, "Move Z Snap 2", LC_KEYMOD1_SHIFT|LC_KEYMOD1_CONTROL, LC_KEY_2, 0 },
	{ LC_EDIT_MOVEZ_SNAP_3, "Move Z Snap 3", LC_KEYMOD1_SHIFT|LC_KEYMOD1_CONTROL, LC_KEY_3, 0 },
	{ LC_EDIT_MOVEZ_SNAP_4, "Move Z Snap 4", LC_KEYMOD1_SHIFT|LC_KEYMOD1_CONTROL, LC_KEY_4, 0 },
	{ LC_EDIT_MOVEZ_SNAP_5, "Move Z Snap 5", LC_KEYMOD1_SHIFT|LC_KEYMOD1_CONTROL, LC_KEY_5, 0 },
	{ LC_EDIT_MOVEZ_SNAP_6, "Move Z Snap 6", LC_KEYMOD1_SHIFT|LC_KEYMOD1_CONTROL, LC_KEY_6, 0 },
	{ LC_EDIT_MOVEZ_SNAP_7, "Move Z Snap 7", LC_KEYMOD1_SHIFT|LC_KEYMOD1_CONTROL, LC_KEY_7, 0 },
	{ LC_EDIT_MOVEZ_SNAP_8, "Move Z Snap 8", LC_KEYMOD1_SHIFT|LC_KEYMOD1_CONTROL, LC_KEY_8, 0 },
	{ LC_EDIT_MOVEZ_SNAP_9, "Move Z Snap 9", LC_KEYMOD1_SHIFT|LC_KEYMOD1_CONTROL, LC_KEY_9, 0 },
	{ LC_EDIT_ANGLE_SNAP_0, "Angle Snap 0", LC_KEYMOD1_SHIFT, LC_KEY_0, 0 },
	{ LC_EDIT_ANGLE_SNAP_1, "Angle Snap 1", LC_KEYMOD1_SHIFT, LC_KEY_1, 0 },
	{ LC_EDIT_ANGLE_SNAP_2, "Angle Snap 5", LC_KEYMOD1_SHIFT, LC_KEY_2, 0 },
	{ LC_EDIT_ANGLE_SNAP_3, "Angle Snap 10", LC_KEYMOD1_SHIFT, LC_KEY_3, 0 },
	{ LC_EDIT_ANGLE_SNAP_4, "Angle Snap 15", LC_KEYMOD1_SHIFT, LC_KEY_4, 0 },
	{ LC_EDIT_ANGLE_SNAP_5, "Angle Snap 30", LC_KEYMOD1_SHIFT, LC_KEY_5, 0 },
	{ LC_EDIT_ANGLE_SNAP_6, "Angle Snap 45", LC_KEYMOD1_SHIFT, LC_KEY_6, 0 },
	{ LC_EDIT_ANGLE_SNAP_7, "Angle Snap 60", LC_KEYMOD1_SHIFT, LC_KEY_7, 0 },
	{ LC_EDIT_ANGLE_SNAP_8, "Angle Snap 90", LC_KEYMOD1_SHIFT, LC_KEY_8, 0 },
	{ LC_EDIT_ANGLE_SNAP_9, "Angle Snap 180", LC_KEYMOD1_SHIFT, LC_KEY_9, 0 },
	{ LC_EDIT_ACTION_SELECT, "Select Mode", 0, 0, 0 },
	{ LC_EDIT_ACTION_INSERT, "Insert Mode", 0, 0, 0 },
	{ LC_EDIT_ACTION_LIGHT, "Light Mode", 0, 0, 0 },
	{ LC_EDIT_ACTION_SPOTLIGHT, "Spotlight Mode", 0, 0, 0 },
	{ LC_EDIT_ACTION_CAMERA, "Camera Mode", 0, 0, 0 },
	{ LC_EDIT_ACTION_MOVE, "Move Mode", LC_KEYMOD1_SHIFT, LC_KEY_M, 0 },
	{ LC_EDIT_ACTION_ROTATE, "Rotate Mode", LC_KEYMOD1_SHIFT, LC_KEY_R, 0 },
	{ LC_EDIT_ACTION_ERASER, "Eraser Mode", LC_KEYMOD1_SHIFT, LC_KEY_E, 0 },
	{ LC_EDIT_ACTION_PAINT, "Paint Mode", LC_KEYMOD1_SHIFT, LC_KEY_N, 0 },
	{ LC_EDIT_ACTION_ZOOM, "Zoom Mode", LC_KEYMOD1_SHIFT, LC_KEY_Z, 0 },
	{ LC_EDIT_ACTION_ZOOM_REGION, "Zoom Region Mode", 0, 0, 0 },
	{ LC_EDIT_ACTION_PAN, "Pan Mode", LC_KEYMOD1_SHIFT, LC_KEY_P, 0 },
	{ LC_EDIT_ACTION_ROTATE_VIEW, "Rotate View Mode", LC_KEYMOD1_SHIFT, LC_KEY_T, 0 },
	{ LC_EDIT_ACTION_ROLL, "Roll Camera Mode", LC_KEYMOD1_SHIFT, LC_KEY_L, 0 },
};

const int KeyboardShortcutsCount = sizeof(DefaultKeyboardShortcuts)/sizeof(KeyboardShortcuts[0]);

LC_KEYBOARD_COMMAND KeyboardShortcuts[KeyboardShortcutsCount];

// ============================================================================
// Functions

bool SaveKeyboardShortcuts(const char* FileName)
{
	lcDiskFile f;

	if (!f.Open(FileName, "wt"))
		return false;

	for (int i = 0; i < KeyboardShortcutsCount; i++)
	{
		LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[i];
		String str;

		str = Cmd.Description;
		str += "=";

		if (Cmd.Key1)
		{
			if (Cmd.Flags & LC_KEYMOD1_SHIFT)
				str += "Shift+";

			if (Cmd.Flags & LC_KEYMOD1_CONTROL)
				str += "Ctrl+";

			str += "\"";
			str += GetKeyName(Cmd.Key1);
			str += "\"";
		}

		if (Cmd.Key2)
		{
			str += ",";

			if (Cmd.Flags & LC_KEYMOD2_SHIFT)
				str += "Shift+";

			if (Cmd.Flags & LC_KEYMOD2_CONTROL)
				str += "Ctrl+";

			str += "\"";
			str += GetKeyName(Cmd.Key2);
			str += "\"";
		}

		str += "\n";

		f.WriteBuffer((const char*)str, str.GetLength());
	}

	return true;
}

bool LoadKeyboardShortcuts(const char* FileName)
{
	lcDiskFile f;
	int i;

	if (!f.Open(FileName, "rt"))
		return false;

	// Remove all existing shortcuts
	for (i = 0; i < KeyboardShortcutsCount; i++)
	{
		LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[i];

		Cmd.Key1 = 0;
		Cmd.Key2 = 0;
		Cmd.Flags = DefaultKeyboardShortcuts[i].Flags & ~LC_KEYMOD_MASK;
	}

	char Line[1024];
	while (f.ReadLine(Line, 1024))
	{
		char* ptr = strchr(Line, '=');

		if (ptr == NULL)
			continue;

		*ptr = 0;
		ptr++;


		for (i = 0; i < KeyboardShortcutsCount; i++)
		{
			LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[i];

			if (strcmp(Line, Cmd.Description))
				continue;

			if (!strncmp(ptr, "Shift+", 6))
			{
				Cmd.Flags |= LC_KEYMOD1_SHIFT;
				ptr += 6;
			}

			if (!strncmp(ptr, "Ctrl+", 5))
			{
				Cmd.Flags |= LC_KEYMOD1_CONTROL;
				ptr += 5;
			}

			ptr++;
			char* ptr2 = strchr(ptr, '\"');

			if (ptr2 == NULL)
			{
				Cmd.Flags &= ~(LC_KEYMOD1_SHIFT | LC_KEYMOD1_CONTROL);
				break;
			}

			*ptr2 = 0;
			Cmd.Key1 = GetKeyFromName(ptr);

			ptr = ptr2 + 1;

			if (*ptr != ',')
				break;
			ptr++;

			if (!strncmp(ptr, "Shift+", 6))
			{
				Cmd.Flags |= LC_KEYMOD2_SHIFT;
				ptr += 6;
			}

			if (!strncmp(ptr, "Ctrl+", 5))
			{
				Cmd.Flags |= LC_KEYMOD2_CONTROL;
				ptr += 5;
			}

			ptr++;
			ptr2 = strchr(ptr, '\"');

			if (ptr2 == NULL)
			{
				Cmd.Flags &= ~(LC_KEYMOD2_SHIFT | LC_KEYMOD2_CONTROL);
				break;
			}

			*ptr2 = 0;
			Cmd.Key2 = GetKeyFromName(ptr);

			break;
		}
	}

	return true;
}

void ResetKeyboardShortcuts()
{
	memcpy(KeyboardShortcuts, DefaultKeyboardShortcuts, sizeof(KeyboardShortcuts));
}

void InitKeyboardShortcuts()
{
	const char* FileName = Sys_ProfileLoadString("Settings", "Keyboard", "");

	ResetKeyboardShortcuts();
	LoadKeyboardShortcuts(FileName);
}

struct LC_KEYNAME_ENTRY
{
	int Key;
	const char* Name;
};

static LC_KEYNAME_ENTRY KeyNames[] =
{
	{ LC_KEY_BACK,       "Backspace" },
	{ LC_KEY_TAB,        "Tab" },
	{ LC_KEY_RETURN,     "Return" },
	{ LC_KEY_PAUSE,      "Pause" },
	{ LC_KEY_CAPITAL,    "Caps" },
	{ LC_KEY_ESCAPE,     "Escape" },
	{ LC_KEY_SPACE,      "Space" },
	{ LC_KEY_PRIOR,      "Page Up" },
	{ LC_KEY_NEXT,       "Page Down" },
	{ LC_KEY_END,        "End" },
	{ LC_KEY_HOME,       "Home" },
	{ LC_KEY_LEFT,       "Left" },
	{ LC_KEY_UP,         "Up" },
	{ LC_KEY_RIGHT,      "Right" },
	{ LC_KEY_DOWN,       "Down" },
	{ LC_KEY_SELECT,     "Select" },
	{ LC_KEY_PRINT,      "Print" },
	{ LC_KEY_INSERT,     "Insert" },
	{ LC_KEY_DELETE,     "Delete" },
	{ LC_KEY_0,          "0" },
	{ LC_KEY_1,          "1" },
	{ LC_KEY_2,          "2" },
	{ LC_KEY_3,          "3" },
	{ LC_KEY_4,          "4" },
	{ LC_KEY_5,          "5" },
	{ LC_KEY_6,          "6" },
	{ LC_KEY_7,          "7" },
	{ LC_KEY_8,          "8" },
	{ LC_KEY_9,          "9" },
	{ LC_KEY_A,          "A" },
	{ LC_KEY_B,          "B" },
	{ LC_KEY_C,          "C" },
	{ LC_KEY_D,          "D" },
	{ LC_KEY_E,          "E" },
	{ LC_KEY_F,          "F" },
	{ LC_KEY_G,          "G" },
	{ LC_KEY_H,          "H" },
	{ LC_KEY_I,          "I" },
	{ LC_KEY_J,          "J" },
	{ LC_KEY_K,          "K" },
	{ LC_KEY_L,          "L" },
	{ LC_KEY_M,          "M" },
	{ LC_KEY_N,          "N" },
	{ LC_KEY_O,          "O" },
	{ LC_KEY_P,          "P" },
	{ LC_KEY_Q,          "Q" },
	{ LC_KEY_R,          "R" },
	{ LC_KEY_S,          "S" },
	{ LC_KEY_T,          "T" },
	{ LC_KEY_U,          "U" },
	{ LC_KEY_V,          "V" },
	{ LC_KEY_W,          "W" },
	{ LC_KEY_X,          "X" },
	{ LC_KEY_Y,          "Y" },
	{ LC_KEY_Z,          "Z" },
	{ LC_KEY_NUMPAD0,    "Numpad 0" },
	{ LC_KEY_NUMPAD1,    "Numpad 1" },
	{ LC_KEY_NUMPAD2,    "Numpad 2" },
	{ LC_KEY_NUMPAD3,    "Numpad 3" },
	{ LC_KEY_NUMPAD4,    "Numpad 4" },
	{ LC_KEY_NUMPAD5,    "Numpad 5" },
	{ LC_KEY_NUMPAD6,    "Numpad 6" },
	{ LC_KEY_NUMPAD7,    "Numpad 7" },
	{ LC_KEY_NUMPAD8,    "Numpad 8" },
	{ LC_KEY_NUMPAD9,    "Numpad 9" },
	{ LC_KEY_MULTIPLY,   "Numpad *" },
	{ LC_KEY_ADD,        "Numpad +" },
	{ LC_KEY_SUBTRACT,   "Numpad -" },
	{ LC_KEY_DECIMAL,    "Numpad ." },
	{ LC_KEY_DIVIDE,     "Numpad /" },
	{ LC_KEY_F1,         "F1" },
	{ LC_KEY_F2,         "F2" },
	{ LC_KEY_F3,         "F3" },
	{ LC_KEY_F4,         "F4" },
	{ LC_KEY_F5,         "F5" },
	{ LC_KEY_F6,         "F6" },
	{ LC_KEY_F7,         "F7" },
	{ LC_KEY_F8,         "F8" },
	{ LC_KEY_F9,         "F9" },
	{ LC_KEY_F10,        "F10" },
	{ LC_KEY_F11,        "F11" },
	{ LC_KEY_F12,        "F12" },
	{ LC_KEY_F13,        "F13" },
	{ LC_KEY_F14,        "F14" },
	{ LC_KEY_F15,        "F15" },
	{ LC_KEY_F16,        "F16" },
	{ LC_KEY_F17,        "F17" },
	{ LC_KEY_F18,        "F18" },
	{ LC_KEY_F19,        "F19" },
	{ LC_KEY_F20,        "F20" },
	{ LC_KEY_F21,        "F21" },
	{ LC_KEY_F22,        "F22" },
	{ LC_KEY_F23,        "F23" },
	{ LC_KEY_F24,        "F24" },
	{ LC_KEY_NUMLOCK,    "Num Lock" },
	{ LC_KEY_SCROLL,     "Scroll" }
};

// Returns a string with the name of the key.
const char* GetKeyName(char Key)
{
	int Count = sizeof(KeyNames)/sizeof(KeyNames[0]);

	for (int i = 0; i < Count; i++)
	{
		if (Key == KeyNames[i].Key)
			return KeyNames[i].Name;
	}

	return NULL;
}

char GetKeyFromName(const char* Name)
{
	int Count = sizeof(KeyNames)/sizeof(KeyNames[0]);

	for (int i = 0; i < Count; i++)
	{
		if (!strcmp(Name, KeyNames[i].Name))
			return KeyNames[i].Key;
	}

	return 0;
}

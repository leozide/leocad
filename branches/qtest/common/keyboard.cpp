#include "lc_global.h"
#include "lc_profile.h"
#include <stdio.h>
#include "system.h"
#include "keyboard.h"
#include "lc_file.h"
#include "str.h"

char gKeyboardShortcuts[LC_NUM_COMMANDS][LC_SHORTCUT_LENGTH];

void ResetKeyboardShortcuts()
{
	for (int ActionIdx = 0; ActionIdx < LC_NUM_COMMANDS; ActionIdx++)
		strcpy(gKeyboardShortcuts[ActionIdx], gActions[ActionIdx].DefaultShortcut);
}

void InitKeyboardShortcuts()
{
	char FileName[LC_MAXPATH];

	strcpy(FileName, lcGetProfileString(LC_PROFILE_SHORTCUTS_FILE));

	if (!LoadKeyboardShortcuts(FileName))
		ResetKeyboardShortcuts();
}

bool SaveKeyboardShortcuts(const char* FileName)
{
	return false;
}

bool LoadKeyboardShortcuts(const char* FileName)
{
	return false; // TODO: shortcuts
}

/*
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
*/

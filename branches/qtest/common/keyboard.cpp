#include "lc_global.h"
#include "keyboard.h"
#include "lc_profile.h"
#include "lc_file.h"

lcKeyboardShortcuts gKeyboardShortcuts;

void lcLoadDefaultKeyboardShortcuts()
{
	lcMemFile File;

	lcGetProfileBuffer(LC_PROFILE_SHORTCUTS, File);

	if (!File.GetLength() || !lcLoadKeyboardShortcuts(File, gKeyboardShortcuts))
		lcResetKeyboardShortcuts(gKeyboardShortcuts);
}

void lcSaveDefaultKeyboardShortcuts()
{
	lcMemFile File;

	lcSaveKeyboardShortcuts(File, gKeyboardShortcuts);

	lcSetProfileBuffer(LC_PROFILE_SHORTCUTS, File);
}

void lcResetDefaultKeyboardShortcuts()
{
	lcResetKeyboardShortcuts(gKeyboardShortcuts);

	lcRemoveProfileKey(LC_PROFILE_SHORTCUTS);
}

void lcResetKeyboardShortcuts(lcKeyboardShortcuts& Shortcuts)
{
	for (int ActionIdx = 0; ActionIdx < LC_NUM_COMMANDS; ActionIdx++)
		strcpy(Shortcuts.Shortcuts[ActionIdx], gActions[ActionIdx].DefaultShortcut);
}

bool lcSaveKeyboardShortcuts(const char* FileName, const lcKeyboardShortcuts& Shortcuts)
{
	lcDiskFile File;

	if (!File.Open(FileName, "wt"))
		return false;

	return lcSaveKeyboardShortcuts(File, Shortcuts);
}

bool lcSaveKeyboardShortcuts(lcFile& File, const lcKeyboardShortcuts& Shortcuts)
{
	char Line[1024];

	for (int ActionIdx = 0; ActionIdx < LC_NUM_COMMANDS; ActionIdx++)
	{
		if (!Shortcuts.Shortcuts[ActionIdx][0])
			continue;

		sprintf(Line, "%s=%s\n", gActions[ActionIdx].ID, Shortcuts.Shortcuts[ActionIdx]);

		File.WriteLine(Line);
	}

	return true;
}

bool lcLoadKeyboardShortcuts(const char* FileName, lcKeyboardShortcuts& Shortcuts)
{
	lcDiskFile File;

	if (!File.Open(FileName, "rt"))
		return false;

	return lcLoadKeyboardShortcuts(File, Shortcuts);
}

bool lcLoadKeyboardShortcuts(lcFile& File, lcKeyboardShortcuts& Shortcuts)
{
	for (int ActionIdx = 0; ActionIdx < LC_NUM_COMMANDS; ActionIdx++)
		Shortcuts.Shortcuts[ActionIdx][0] = 0;

	char Line[1024];

	while (File.ReadLine(Line, sizeof(Line)))
	{
		char* Key = strchr(Line, '=');

		if (!Key)
			continue;

		*Key = 0;
		Key++;

		int ActionIdx;
		for (ActionIdx = 0; ActionIdx < LC_NUM_COMMANDS; ActionIdx++)
			if (!strcmp(gActions[ActionIdx].ID, Line))
				break;

		if (ActionIdx == LC_NUM_COMMANDS)
			continue;

		char* NewLine = strchr(Key, '\n');
		if (NewLine)
			*NewLine = 0;

		strncpy(Shortcuts.Shortcuts[ActionIdx], Key, LC_SHORTCUT_LENGTH);
	}

	return true;
}

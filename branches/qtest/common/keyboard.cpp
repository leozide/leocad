#include "lc_global.h"
#include "keyboard.h"
#include "lc_profile.h"
#include "lc_file.h"

lcKeyboardShortcuts gKeyboardShortcuts;

void LoadDefaultKeyboardShortcuts()
{
	lcMemFile File;

	lcGetProfileBuffer(LC_PROFILE_SHORTCUTS, File);

	if (!File.GetLength() || !LoadKeyboardShortcuts(File, gKeyboardShortcuts))
		ResetKeyboardShortcuts(gKeyboardShortcuts);
}

void SaveDefaultKeyboardShortcuts()
{
	lcMemFile File;

	SaveKeyboardShortcuts(File, gKeyboardShortcuts);

	lcSetProfileBuffer(LC_PROFILE_SHORTCUTS, File);
}

void ResetDefaultKeyboardShortcuts()
{
	ResetKeyboardShortcuts(gKeyboardShortcuts);

	lcRemoveProfileKey(LC_PROFILE_SHORTCUTS);
}

void ResetKeyboardShortcuts(lcKeyboardShortcuts& Shortcuts)
{
	for (int ActionIdx = 0; ActionIdx < LC_NUM_COMMANDS; ActionIdx++)
		strcpy(Shortcuts.Shortcuts[ActionIdx], gActions[ActionIdx].DefaultShortcut);
}

bool SaveKeyboardShortcuts(const char* FileName, const lcKeyboardShortcuts& Shortcuts)
{
	lcDiskFile File;

	if (!File.Open(FileName, "wt"))
		return false;

	return SaveKeyboardShortcuts(File, Shortcuts);
}

bool SaveKeyboardShortcuts(lcFile& File, const lcKeyboardShortcuts& Shortcuts)
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

bool LoadKeyboardShortcuts(const char* FileName, lcKeyboardShortcuts& Shortcuts)
{
	lcDiskFile File;

	if (!File.Open(FileName, "rt"))
		return false;

	return LoadKeyboardShortcuts(File, Shortcuts);
}

bool LoadKeyboardShortcuts(lcFile& File, lcKeyboardShortcuts& Shortcuts)
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

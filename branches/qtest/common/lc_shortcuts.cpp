#include "lc_global.h"
#include "lc_shortcuts.h"
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
	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
		strcpy(Shortcuts.Shortcuts[CommandIdx], gCommands[CommandIdx].DefaultShortcut);
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

	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
	{
		if (!Shortcuts.Shortcuts[CommandIdx][0])
			continue;

		sprintf(Line, "%s=%s\n", gCommands[CommandIdx].ID, Shortcuts.Shortcuts[CommandIdx]);

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
	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
		Shortcuts.Shortcuts[CommandIdx][0] = 0;

	char Line[1024];

	while (File.ReadLine(Line, sizeof(Line)))
	{
		char* Key = strchr(Line, '=');

		if (!Key)
			continue;

		*Key = 0;
		Key++;

		int CommandIdx;
		for (CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
			if (!strcmp(gCommands[CommandIdx].ID, Line))
				break;

		if (CommandIdx == LC_NUM_COMMANDS)
			continue;

		char* NewLine = strchr(Key, '\n');
		if (NewLine)
			*NewLine = 0;

		strncpy(Shortcuts.Shortcuts[CommandIdx], Key, LC_SHORTCUT_LENGTH);
	}

	return true;
}

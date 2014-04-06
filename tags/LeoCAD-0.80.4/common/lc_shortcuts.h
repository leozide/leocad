#ifndef _LC_SHORTCUTS_H_
#define _LC_SHORTCUTS_H_

#include "lc_commands.h"

#define LC_SHORTCUT_LENGTH 32

struct lcKeyboardShortcuts
{
	char Shortcuts[LC_NUM_COMMANDS][LC_SHORTCUT_LENGTH];
};

extern lcKeyboardShortcuts gKeyboardShortcuts;

void lcLoadDefaultKeyboardShortcuts();
void lcSaveDefaultKeyboardShortcuts();
void lcResetDefaultKeyboardShortcuts();

void lcResetKeyboardShortcuts(lcKeyboardShortcuts& Shortcuts);
bool lcSaveKeyboardShortcuts(const char* FileName, const lcKeyboardShortcuts& Shortcuts);
bool lcSaveKeyboardShortcuts(lcFile& File, const lcKeyboardShortcuts& Shortcuts);
bool lcLoadKeyboardShortcuts(const char* FileName, lcKeyboardShortcuts& Shortcuts);
bool lcLoadKeyboardShortcuts(lcFile& File, lcKeyboardShortcuts& Shortcuts);

#endif // _LC_SHORTCUTS_H_

#ifndef _LC_SHORTCUTS_H_
#define _LC_SHORTCUTS_H_

#include "lc_commands.h"

struct lcKeyboardShortcuts
{
	QString Shortcuts[LC_NUM_COMMANDS];
};

extern lcKeyboardShortcuts gKeyboardShortcuts;

void lcLoadDefaultKeyboardShortcuts();
void lcSaveDefaultKeyboardShortcuts();
void lcResetDefaultKeyboardShortcuts();

void lcResetKeyboardShortcuts(lcKeyboardShortcuts& Shortcuts);
bool lcSaveKeyboardShortcuts(const QString& FileName, const lcKeyboardShortcuts& Shortcuts);
bool lcSaveKeyboardShortcuts(lcFile& File, const lcKeyboardShortcuts& Shortcuts);
bool lcLoadKeyboardShortcuts(const QString& FileName, lcKeyboardShortcuts& Shortcuts);
bool lcLoadKeyboardShortcuts(lcFile& File, lcKeyboardShortcuts& Shortcuts);

#endif // _LC_SHORTCUTS_H_

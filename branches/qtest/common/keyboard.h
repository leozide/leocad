#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "lc_action.h"

#define LC_SHORTCUT_LENGTH 32

struct lcKeyboardShortcuts
{
	char Shortcuts[LC_NUM_COMMANDS][LC_SHORTCUT_LENGTH];
};

extern lcKeyboardShortcuts gKeyboardShortcuts;

void LoadDefaultKeyboardShortcuts();
void SaveDefaultKeyboardShortcuts();
void ResetDefaultKeyboardShortcuts();

void ResetKeyboardShortcuts(lcKeyboardShortcuts& Shortcuts);
bool SaveKeyboardShortcuts(const char* FileName, const lcKeyboardShortcuts& Shortcuts);
bool SaveKeyboardShortcuts(lcFile& File, const lcKeyboardShortcuts& Shortcuts);
bool LoadKeyboardShortcuts(const char* FileName, lcKeyboardShortcuts& Shortcuts);
bool LoadKeyboardShortcuts(lcFile& File, lcKeyboardShortcuts& Shortcuts);

#endif // _KEYBOARD_H_

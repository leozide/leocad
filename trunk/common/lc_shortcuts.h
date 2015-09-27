#ifndef _LC_SHORTCUTS_H_
#define _LC_SHORTCUTS_H_

#include "lc_commands.h"

class lcKeyboardShortcuts
{
public:
	void Reset();
	bool Save(const QString& FileName);
	bool Save(QTextStream& Stream);
	bool Load(const QString& FileName);
	bool Load(QTextStream& Stream);

	QString mShortcuts[LC_NUM_COMMANDS];
};

extern lcKeyboardShortcuts gKeyboardShortcuts;

void lcLoadDefaultKeyboardShortcuts();
void lcSaveDefaultKeyboardShortcuts();
void lcResetDefaultKeyboardShortcuts();

#endif // _LC_SHORTCUTS_H_

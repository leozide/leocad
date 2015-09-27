#include "lc_global.h"
#include "lc_shortcuts.h"
#include "lc_profile.h"

lcKeyboardShortcuts gKeyboardShortcuts;

void lcLoadDefaultKeyboardShortcuts()
{
	QByteArray Buffer = lcGetProfileBuffer(LC_PROFILE_SHORTCUTS);

	if (Buffer.isEmpty() || !gKeyboardShortcuts.Load(QTextStream(Buffer, QIODevice::ReadOnly)))
		gKeyboardShortcuts.Reset();
}

void lcSaveDefaultKeyboardShortcuts()
{
	QByteArray Buffer;

	gKeyboardShortcuts.Save(QTextStream(&Buffer, QIODevice::WriteOnly));

	lcSetProfileBuffer(LC_PROFILE_SHORTCUTS, Buffer);
}

void lcResetDefaultKeyboardShortcuts()
{
	gKeyboardShortcuts.Reset();

	lcRemoveProfileKey(LC_PROFILE_SHORTCUTS);
}

void lcKeyboardShortcuts::Reset()
{
	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
		mShortcuts[CommandIdx] = qApp->translate("Shortcut", gCommands[CommandIdx].DefaultShortcut);
}

bool lcKeyboardShortcuts::Save(const QString& FileName)
{
	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
		return false;

	return Save(QTextStream(&File));
}

bool lcKeyboardShortcuts::Save(QTextStream& Stream)
{
	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
	{
		if (mShortcuts[CommandIdx].isEmpty())
			continue;

		Stream << gCommands[CommandIdx].ID << QLatin1String("=") << mShortcuts[CommandIdx] << QLatin1String("\n");
	}

	return true;
}

bool lcKeyboardShortcuts::Load(const QString& FileName)
{
	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
		return false;

	return Load(QTextStream(&File));
}

bool lcKeyboardShortcuts::Load(QTextStream& Stream)
{
	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
		mShortcuts[CommandIdx].clear();

	for (QString Line = Stream.readLine(); !Line.isNull(); Line = Stream.readLine())
	{
		int Equals = Line.indexOf('=');

		if (Equals == -1)
			continue;

		QString Key = Line.left(Equals);

		int CommandIdx;
		for (CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
			if (gCommands[CommandIdx].ID == Key)
				break;

		if (CommandIdx == LC_NUM_COMMANDS)
			continue;

		mShortcuts[CommandIdx] = Line.mid(Equals + 1);
	}

	return true;
}

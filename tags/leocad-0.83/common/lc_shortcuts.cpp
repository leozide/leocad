#include "lc_global.h"
#include "lc_shortcuts.h"
#include "lc_profile.h"

lcKeyboardShortcuts gKeyboardShortcuts;
lcMouseShortcuts gMouseShortcuts;

void lcLoadDefaultKeyboardShortcuts()
{
	QByteArray Buffer = lcGetProfileBuffer(LC_PROFILE_KEYBOARD_SHORTCUTS);
	QTextStream Stream(Buffer, QIODevice::ReadOnly);

	if (Buffer.isEmpty() || !gKeyboardShortcuts.Load(Stream))
		gKeyboardShortcuts.Reset();
}

void lcSaveDefaultKeyboardShortcuts()
{
	QByteArray Buffer;
	QTextStream Stream(&Buffer, QIODevice::WriteOnly);

	gKeyboardShortcuts.Save(Stream);

	lcSetProfileBuffer(LC_PROFILE_KEYBOARD_SHORTCUTS, Buffer);
}

void lcResetDefaultKeyboardShortcuts()
{
	gKeyboardShortcuts.Reset();

	lcRemoveProfileKey(LC_PROFILE_KEYBOARD_SHORTCUTS);
}

void lcLoadDefaultMouseShortcuts()
{
	QStringList Shortcuts = lcGetProfileStringList(LC_PROFILE_MOUSE_SHORTCUTS);

	if (Shortcuts.isEmpty() || !gMouseShortcuts.Load(Shortcuts))
		gMouseShortcuts.Reset();
}

void lcSaveDefaultMouseShortcuts()
{
	QStringList Shortcuts;

	gMouseShortcuts.Save(Shortcuts);

	lcSetProfileStringList(LC_PROFILE_MOUSE_SHORTCUTS, Shortcuts);
}

void lcResetDefaultMouseShortcuts()
{
	gMouseShortcuts.Reset();

	lcRemoveProfileKey(LC_PROFILE_MOUSE_SHORTCUTS);
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

	QTextStream Stream(&File);

	return Save(Stream);
}

bool lcKeyboardShortcuts::Save(QTextStream& Stream)
{
	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
	{
		if (mShortcuts[CommandIdx].isEmpty())
			continue;

		Stream << gCommands[CommandIdx].ID << QLatin1String("=") << mShortcuts[CommandIdx] << QLatin1String("\n");
	}

	Stream.flush();

	return true;
}

bool lcKeyboardShortcuts::Load(const QString& FileName)
{
	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
		return false;

	QTextStream Stream(&File);

	return Load(Stream);
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

void lcMouseShortcuts::Reset()
{
	memset(mShortcuts, 0, sizeof(mShortcuts));
	mShortcuts[LC_TOOL_ROTATE_VIEW].Modifiers = Qt::AltModifier;
	mShortcuts[LC_TOOL_ROTATE_VIEW].Button = Qt::LeftButton;
	mShortcuts[LC_TOOL_PAN].Modifiers = Qt::AltModifier;
	mShortcuts[LC_TOOL_PAN].Button = Qt::MiddleButton;
	mShortcuts[LC_TOOL_ZOOM].Modifiers = Qt::AltModifier;
	mShortcuts[LC_TOOL_ZOOM].Button = Qt::RightButton;
}

bool lcMouseShortcuts::Save(QStringList& Shortcuts)
{
	Shortcuts.clear();

	for (int ToolIdx = 0; ToolIdx < LC_NUM_TOOLS; ToolIdx++)
	{
		int ButtonIndex = 0;
		for (int Button = mShortcuts[ToolIdx].Button; Button; Button >>= 1)
			ButtonIndex++;

		if (!ButtonIndex)
			continue;

		QString Shortcut = QKeySequence(mShortcuts[ToolIdx].Modifiers | (Qt::Key_0 + ButtonIndex)).toString(QKeySequence::PortableText);
		Shortcuts << gToolNames[ToolIdx] + QLatin1String("=") + Shortcut;
	}

	return true;
}

bool lcMouseShortcuts::Load(const QStringList& Shortcuts)
{
	memset(mShortcuts, 0, sizeof(mShortcuts));

	foreach (const QString& Shortcut, Shortcuts)
	{
		int Equals = Shortcut.indexOf('=');

		if (Equals == -1)
			continue;

		QString Key = Shortcut.left(Equals);

		int ToolIdx;
		for (ToolIdx = 0; ToolIdx < LC_NUM_TOOLS; ToolIdx++)
			if (gToolNames[ToolIdx] == Key)
				break;

		if (ToolIdx == LC_NUM_TOOLS)
			continue;

		QKeySequence KeySequence(Shortcut.mid(Equals + 1));

		if (KeySequence.isEmpty())
			continue;

		int ShortcutKey = KeySequence[0];
		mShortcuts[ToolIdx].Modifiers = (Qt::KeyboardModifier)(ShortcutKey & Qt::KeyboardModifierMask);
		mShortcuts[ToolIdx].Button = (Qt::MouseButton)(1 << ((ShortcutKey & ~Qt::KeyboardModifierMask) - Qt::Key_0 - 1));
	}

	return true;
}

lcTool lcMouseShortcuts::GetTool(Qt::MouseButton Button, Qt::KeyboardModifiers Modifiers) const
{
	for (int ToolIdx = 0; ToolIdx < LC_NUM_TOOLS; ToolIdx++)
		if (mShortcuts[ToolIdx].Button == Button && mShortcuts[ToolIdx].Modifiers == Modifiers)
			return (lcTool)ToolIdx;

	return LC_NUM_TOOLS;
}

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
		mShortcuts[CommandIdx] = gCommands[CommandIdx].DefaultShortcut;
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

	mShortcuts[LC_TOOL_ROTATE_VIEW].Modifiers1 = Qt::AltModifier;
	mShortcuts[LC_TOOL_ROTATE_VIEW].Button1 = Qt::LeftButton;
	mShortcuts[LC_TOOL_ROTATE_VIEW].Modifiers2 = Qt::NoModifier;
	mShortcuts[LC_TOOL_ROTATE_VIEW].Button2 = Qt::RightButton;

#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
	mShortcuts[LC_TOOL_PAN].Modifiers1 = Qt::AltModifier;
	mShortcuts[LC_TOOL_PAN].Button1 = Qt::MiddleButton;
	mShortcuts[LC_TOOL_PAN].Modifiers2 = Qt::ShiftModifier;
	mShortcuts[LC_TOOL_PAN].Button2 = Qt::RightButton;
#else
	mShortcuts[LC_TOOL_PAN].Modifiers1 = Qt::ShiftModifier;
	mShortcuts[LC_TOOL_PAN].Button1 = Qt::RightButton;
#endif

	mShortcuts[LC_TOOL_ZOOM].Modifiers1 = Qt::AltModifier;
	mShortcuts[LC_TOOL_ZOOM].Button1 = Qt::RightButton;
}

bool lcMouseShortcuts::Save(QStringList& Shortcuts)
{
	Shortcuts.clear();

	for (int ToolIdx = 0; ToolIdx < LC_NUM_TOOLS; ToolIdx++)
	{
		int ButtonIndex1 = 0;
		for (int Button1 = mShortcuts[ToolIdx].Button1; Button1; Button1 >>= 1)
			ButtonIndex1++;

		if (!ButtonIndex1)
			continue;

		QString Shortcut = QKeySequence(mShortcuts[ToolIdx].Modifiers1 | (Qt::Key_0 + ButtonIndex1)).toString(QKeySequence::PortableText);

		int ButtonIndex2 = 0;
		for (int Button2 = mShortcuts[ToolIdx].Button2; Button2; Button2 >>= 1)
			ButtonIndex2++;

		if (ButtonIndex2)
			Shortcut += ',' + QKeySequence(mShortcuts[ToolIdx].Modifiers2 | (Qt::Key_0 + ButtonIndex2)).toString(QKeySequence::PortableText);

		Shortcuts << QString::fromLatin1(gToolNames[ToolIdx]) + QLatin1String("=") + Shortcut;
	}

	return true;
}

bool lcMouseShortcuts::Load(const QStringList& Shortcuts)
{
	memset(mShortcuts, 0, sizeof(mShortcuts));

	for (const QString& Shortcut : Shortcuts)
	{
		int Equals = Shortcut.indexOf('=');

		if (Equals == -1)
			continue;

		QString Key = Shortcut.left(Equals);

		int ToolIdx;
		for (ToolIdx = 0; ToolIdx < LC_NUM_TOOLS; ToolIdx++)
			if (Key == gToolNames[ToolIdx])
				break;

		if (ToolIdx == LC_NUM_TOOLS)
			continue;

		QStringList Shortcuts = Shortcut.mid(Equals + 1).split(',');
		bool AddedShortcut = false;

		for (const QString& Shortcut : Shortcuts)
		{
			QKeySequence KeySequence(Shortcut);

			if (KeySequence.isEmpty())
				continue;

			int ShortcutKey = KeySequence[0];
			Qt::KeyboardModifiers Modifiers = (Qt::KeyboardModifier)(ShortcutKey & Qt::KeyboardModifierMask);
			Qt::MouseButton Button = (Qt::MouseButton)(1 << ((ShortcutKey & ~Qt::KeyboardModifierMask) - Qt::Key_0 - 1));

			if (!AddedShortcut)
			{
				mShortcuts[ToolIdx].Modifiers1 = Modifiers;
				mShortcuts[ToolIdx].Button1 = Button;
				AddedShortcut = true;
			}
			else
			{
				mShortcuts[ToolIdx].Modifiers2 = Modifiers;
				mShortcuts[ToolIdx].Button2 = Button;
			}
		}
	}

	return true;
}

lcTool lcMouseShortcuts::GetTool(Qt::MouseButton Button, Qt::KeyboardModifiers Modifiers) const
{
	for (int ToolIdx = 0; ToolIdx < LC_NUM_TOOLS; ToolIdx++)
		if ((mShortcuts[ToolIdx].Button1 == Button && mShortcuts[ToolIdx].Modifiers1 == Modifiers) || (mShortcuts[ToolIdx].Button2 == Button && mShortcuts[ToolIdx].Modifiers2 == Modifiers))
			return (lcTool)ToolIdx;

	return LC_NUM_TOOLS;
}

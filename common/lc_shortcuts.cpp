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

	lcToolShortcut& RotateViewShortcut = mShortcuts[static_cast<int>(lcTool::RotateView)];
	RotateViewShortcut.Modifiers1 = Qt::AltModifier;
	RotateViewShortcut.Button1 = Qt::LeftButton;
	RotateViewShortcut.Modifiers2 = Qt::NoModifier;
	RotateViewShortcut.Button2 = Qt::RightButton;

	lcToolShortcut& PanShortcut = mShortcuts[static_cast<int>(lcTool::Pan)];
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
	PanShortcut.Modifiers1 = Qt::AltModifier;
	PanShortcut.Button1 = Qt::MiddleButton;
	PanShortcut.Modifiers2 = Qt::ShiftModifier;
	PanShortcut.Button2 = Qt::RightButton;
#else
	PanShortcut.Modifiers1 = Qt::ShiftModifier;
	PanShortcut.Button1 = Qt::RightButton;
#endif

	lcToolShortcut& ZoomShortcut = mShortcuts[static_cast<int>(lcTool::Zoom)];
	ZoomShortcut.Modifiers1 = Qt::AltModifier;
	ZoomShortcut.Button1 = Qt::RightButton;
}

bool lcMouseShortcuts::Save(const QString& FileName)
{
	QStringList Shortcuts;

	if (!Save(Shortcuts))
		return false;

	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
		return false;

	QTextStream Stream(&File);

	for (const QString& Shortcut : Shortcuts)
		Stream << Shortcut << QLatin1String("\n");

	Stream.flush();

	return true;
}
bool lcMouseShortcuts::Save(QStringList& Shortcuts)
{
	Shortcuts.clear();

	for (int ToolIdx = 0; ToolIdx < static_cast<int>(lcTool::Count); ToolIdx++)
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

bool lcMouseShortcuts::Load(const QString& FileName)
{
	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
		return false;

	QTextStream Stream(&File);
	QStringList Lines;

	while (!Stream.atEnd())
		Lines += Stream.readLine();

	return Load(Lines);
}

bool lcMouseShortcuts::Load(const QStringList& FullShortcuts)
{
	memset(mShortcuts, 0, sizeof(mShortcuts));

	for (const QString& FullShortcut : FullShortcuts)
	{
		int Equals = FullShortcut.indexOf('=');

		if (Equals == -1)
			continue;

		QString Key = FullShortcut.left(Equals);

		int ToolIdx;
		for (ToolIdx = 0; ToolIdx < static_cast<int>(lcTool::Count); ToolIdx++)
			if (Key == gToolNames[ToolIdx])
				break;

		if (ToolIdx == static_cast<int>(lcTool::Count))
			continue;

		QStringList Shortcuts = FullShortcut.mid(Equals + 1).split(',');
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
	for (int ToolIdx = 0; ToolIdx < static_cast<int>(lcTool::Count); ToolIdx++)
		if ((mShortcuts[ToolIdx].Button1 == Button && mShortcuts[ToolIdx].Modifiers1 == Modifiers) || (mShortcuts[ToolIdx].Button2 == Button && mShortcuts[ToolIdx].Modifiers2 == Modifiers))
			return (lcTool)ToolIdx;

	return lcTool::Count;
}

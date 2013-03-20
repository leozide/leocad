#include "lc_global.h"
#include "lc_profile.h"
#include "system.h"

int lcGetProfileInt(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_INT);

	return settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Entry.mDefault.IntValue).toInt();
}

float lcGetProfileFloat(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_FLOAT);

	return settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Entry.mDefault.FloatValue).toFloat();
}

const char* lcGetProfileString(LC_PROFILE_KEY Key)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_STRING);

	static QByteArray Value = settings.value(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Entry.mDefault.StringValue).toString().toLocal8Bit();

	return Value.data();
}

void lcSetProfileInt(LC_PROFILE_KEY Key, int Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_INT);

	settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

void lcSetProfileFloat(LC_PROFILE_KEY Key, float Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_FLOAT);

	settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

void lcSetProfileString(LC_PROFILE_KEY Key, const char* Value)
{
	lcProfileEntry& Entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(Entry.mType == LC_PROFILE_ENTRY_STRING);

	settings.setValue(QString("%1/%2").arg(Entry.mSection, Entry.mKey), Value);
}

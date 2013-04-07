#include "lc_global.h"
#include "lc_profile.h"
#include "lc_file.h"
#include "system.h"

void lcRemoveProfileKey(LC_PROFILE_KEY Key)
{
	lcProfileEntry& entry = gProfileEntries[Key];
	QSettings settings;

	settings.remove(QString("%1/%2").arg(entry.mSection, entry.mKey));
}

int lcGetProfileInt(LC_PROFILE_KEY Key)
{
	lcProfileEntry& entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(entry.mType == LC_PROFILE_ENTRY_INT);

	return settings.value(QString("%1/%2").arg(entry.mSection, entry.mKey), entry.mDefault.IntValue).toInt();
}

float lcGetProfileFloat(LC_PROFILE_KEY Key)
{
	lcProfileEntry& entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(entry.mType == LC_PROFILE_ENTRY_FLOAT);

	return settings.value(QString("%1/%2").arg(entry.mSection, entry.mKey), entry.mDefault.FloatValue).toFloat();
}

const char* lcGetProfileString(LC_PROFILE_KEY Key)
{
	lcProfileEntry& entry = gProfileEntries[Key];
	QSettings settings;
	static QByteArray value;

	LC_ASSERT(entry.mType == LC_PROFILE_ENTRY_STRING);

	value = settings.value(QString("%1/%2").arg(entry.mSection, entry.mKey), entry.mDefault.StringValue).toString().toLocal8Bit();

	return value.data();
}

void lcGetProfileBuffer(LC_PROFILE_KEY Key, lcMemFile& Buffer)
{
	lcProfileEntry& entry = gProfileEntries[Key];
	QSettings settings;
	QByteArray value;

	LC_ASSERT(entry.mType == LC_PROFILE_ENTRY_BUFFER);

	value = settings.value(QString("%1/%2").arg(entry.mSection, entry.mKey)).toByteArray();

	Buffer.Seek(0, SEEK_SET);
	Buffer.SetLength(value.size());
	Buffer.WriteBuffer(value.constData(), value.size());
	Buffer.Seek(0, SEEK_SET);
}

void lcSetProfileInt(LC_PROFILE_KEY Key, int Value)
{
	lcProfileEntry& entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(entry.mType == LC_PROFILE_ENTRY_INT);

	settings.setValue(QString("%1/%2").arg(entry.mSection, entry.mKey), Value);
}

void lcSetProfileFloat(LC_PROFILE_KEY Key, float Value)
{
	lcProfileEntry& entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(entry.mType == LC_PROFILE_ENTRY_FLOAT);

	settings.setValue(QString("%1/%2").arg(entry.mSection, entry.mKey), Value);
}

void lcSetProfileString(LC_PROFILE_KEY Key, const char* Value)
{
	lcProfileEntry& entry = gProfileEntries[Key];
	QSettings settings;

	LC_ASSERT(entry.mType == LC_PROFILE_ENTRY_STRING);

	settings.setValue(QString("%1/%2").arg(entry.mSection, entry.mKey), Value);
}

void lcSetProfileBuffer(LC_PROFILE_KEY Key, const lcMemFile& Buffer)
{
	lcProfileEntry& entry = gProfileEntries[Key];
	QSettings settings;
	QByteArray value = QByteArray::fromRawData((const char*)Buffer.mBuffer, Buffer.GetLength());

	LC_ASSERT(entry.mType == LC_PROFILE_ENTRY_BUFFER);

	settings.setValue(QString("%1/%2").arg(entry.mSection, entry.mKey), value);
}

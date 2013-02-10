#include "lc_global.h"

bool Sys_ProfileSaveInt (const char *section, const char *key, int value)
{
	return true;
}

bool Sys_ProfileSaveString (const char *section, const char *key, const char *value)
{
	return true;
}

int Sys_ProfileLoadInt (const char *section, const char *key, int default_value)
{
	return default_value;
}

char* Sys_ProfileLoadString (const char *section, const char *key, const char *default_value)
{
	static char value[1024];

	strcpy (value, default_value);

	return value;
}

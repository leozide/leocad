#include "lc_global.h"
#include "object.h"
#include "lc_file.h"

lcObject::lcObject(lcObjectType ObjectType)
	: mObjectType(ObjectType)
{
}

lcObject::~lcObject()
{
}

void lcObject::SaveKeysLDraw(const lcArray<lcObjectKey<lcVector3>>& Keys, const char* KeyName, lcFile& File) const
{
	char Line[1024];

	for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
	{
		lcObjectKey<lcVector3>& Key = Keys[KeyIdx];
		sprintf(Line, "0 !LEOCAD %s %d %f %f %f\r\n", KeyName, Key.Step, Key.Value[0], Key.Value[1], Key.Value[2]);
		File.WriteBuffer(Line, strlen(Line));
	}
}

void lcObject::SaveKeysLDraw(const lcArray<lcObjectKey<lcVector4>>& Keys, const char* KeyName, lcFile& File) const
{
	char Line[1024];

	for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
	{
		lcObjectKey<lcVector4>& Key = Keys[KeyIdx];
		sprintf(Line, "0 !LEOCAD %s %d %f %f %f %f\r\n", KeyName, Key.Step, Key.Value[0], Key.Value[1], Key.Value[2], Key.Value[3]);
		File.WriteBuffer(Line, strlen(Line));
	}
}

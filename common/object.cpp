#include "lc_global.h"
#include "object.h"

lcObject::lcObject(lcObjectType ObjectType)
	: mObjectType(ObjectType)
{
}

lcObject::~lcObject()
{
}

QJsonArray lcObject::SaveKeys(const lcArray<lcObjectKey<lcVector3>>& Keys)
{
	QJsonArray KeyArray;

	for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
	{
		lcObjectKey<lcVector3>& Key = Keys[KeyIdx];
		QJsonObject KeyObject;

		KeyObject[QStringLiteral("Step")] = QString::number(Key.Step);
		KeyObject[QStringLiteral("Value")] = QStringLiteral("%1 %2 %3").arg(QString::number(Key.Value[0]), QString::number(Key.Value[1]), QString::number(Key.Value[2]));
		KeyArray.append(KeyObject);
	}

	return KeyArray;
}

QJsonArray lcObject::SaveKeys(const lcArray<lcObjectKey<lcVector4>>& Keys)
{
	QJsonArray KeyArray;

	for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
	{
		lcObjectKey<lcVector4>& Key = Keys[KeyIdx];
		QJsonObject KeyObject;

		KeyObject[QStringLiteral("Step")] = QString::number(Key.Step);
		KeyObject[QStringLiteral("Value")] = QStringLiteral("%1 %2 %3 %4").arg(QString::number(Key.Value[0]), QString::number(Key.Value[1]), QString::number(Key.Value[2]), QString::number(Key.Value[3]));
		KeyArray.append(KeyObject);
	}

	return KeyArray;
}

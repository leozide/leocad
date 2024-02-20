#include "lc_global.h"
#include "object.h"

lcObject::lcObject(lcObjectType ObjectType)
	: mObjectType(ObjectType)
{
}

lcObject::~lcObject()
{
}

QString lcObject::GetCheckpointString(lcObjectPropertyId PropertyId)
{
	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
	case lcObjectPropertyId::PieceColor:
	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
	case lcObjectPropertyId::CameraName:
		break;

	case lcObjectPropertyId::CameraType:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Camera Type");

	case lcObjectPropertyId::CameraFOV:
	case lcObjectPropertyId::CameraNear:
	case lcObjectPropertyId::CameraFar:
	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
	case lcObjectPropertyId::LightName:
		break;

	case lcObjectPropertyId::LightType:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Light Type");

	case lcObjectPropertyId::LightColor:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Light Color");

	case lcObjectPropertyId::LightPower:
		break;

	case lcObjectPropertyId::LightCastShadow:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Light Shadow");

	case lcObjectPropertyId::LightAttenuationDistance:
	case lcObjectPropertyId::LightAttenuationPower:
	case lcObjectPropertyId::LightPointSize:
	case lcObjectPropertyId::LightSpotSize:
	case lcObjectPropertyId::LightDirectionalSize:
	case lcObjectPropertyId::LightAreaSize:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotTightness:
		break;

	case lcObjectPropertyId::LightAreaShape:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Area Light Shape");

	case lcObjectPropertyId::LightAreaGridX:
	case lcObjectPropertyId::LightAreaGridY:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Area Light Grid");

	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
	case lcObjectPropertyId::Count:
		break;
	}

	return QString();
}

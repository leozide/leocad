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
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Piece Id");

	case lcObjectPropertyId::PieceColor:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Piece Color");

	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
		break;

	case lcObjectPropertyId::CameraName:
		return QT_TRANSLATE_NOOP("Checkpoint", "Renaming Camera");

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
		break;

	case lcObjectPropertyId::LightName:
		return QT_TRANSLATE_NOOP("Checkpoint", "Renaming Light");

	case lcObjectPropertyId::LightType:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Light Type");

	case lcObjectPropertyId::LightColor:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Light Color");

	case lcObjectPropertyId::LightBlenderPower:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Light Blender Power");

	case lcObjectPropertyId::LightPOVRayPower:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Light POV-Ray Power");

	case lcObjectPropertyId::LightCastShadow:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Light Shadow");

	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointSize:
	case lcObjectPropertyId::LightSpotSize:
	case lcObjectPropertyId::LightDirectionalSize:
	case lcObjectPropertyId::LightAreaSize:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
		break;

	case lcObjectPropertyId::LightAreaShape:
		return QT_TRANSLATE_NOOP("Checkpoint", "Changing Area Light Shape");

	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
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

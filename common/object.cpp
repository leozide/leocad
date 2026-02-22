#include "lc_global.h"
#include "object.h"

lcObjectId lcObject::mNextId;

lcObject::lcObject(lcObjectType ObjectType)
    : mObjectType(ObjectType), mId(mNextId)
{
	mNextId = static_cast<lcObjectId>(static_cast<uint32_t>(mNextId) + 1);
}

lcObject::~lcObject()
{
}

QString lcObject::GetCheckpointString(lcObjectPropertyId PropertyId)
{
	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Piece");

	case lcObjectPropertyId::PieceColor:
		return QT_TRANSLATE_NOOP("Checkpoint", "Chang Piece Color");

	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
		break;

	case lcObjectPropertyId::CameraName:
		return QT_TRANSLATE_NOOP("Checkpoint", "Rename Camera");

	case lcObjectPropertyId::CameraProjection:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Camera Projection");

	case lcObjectPropertyId::CameraFOV:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Camera FOV");

	case lcObjectPropertyId::CameraNear:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Camera Near Plane");

	case lcObjectPropertyId::CameraFar:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Camera Far Plane");

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
		return QT_TRANSLATE_NOOP("Checkpoint", "Rename Light");

	case lcObjectPropertyId::LightType:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Light Type");

	case lcObjectPropertyId::LightColor:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Light Color");

	case lcObjectPropertyId::LightBlenderPower:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Light Blender Power");

	case lcObjectPropertyId::LightPOVRayPower:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Light POV-Ray Power");

	case lcObjectPropertyId::LightCastShadow:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Light Shadow");

	case lcObjectPropertyId::LightPOVRayFadeDistance:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Light POV-Ray Fade Distance");

	case lcObjectPropertyId::LightPOVRayFadePower:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Light POV-Ray Fade Power");

	case lcObjectPropertyId::LightBlenderRadius:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Light Blender Radius");

	case lcObjectPropertyId::LightBlenderAngle:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Light Blender Angle");

	case lcObjectPropertyId::LightAreaSizeX:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Area Light X Size");

	case lcObjectPropertyId::LightAreaSizeY:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Area Light Y Size");

	case lcObjectPropertyId::LightSpotConeAngle:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Spot Light Cone Angle");

	case lcObjectPropertyId::LightSpotPenumbraAngle:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Spot Light Penumbra Angle");

	case lcObjectPropertyId::LightPOVRaySpotTightness:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Spot Light POV-Ray Tightness");

	case lcObjectPropertyId::LightAreaShape:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Area Light Shape");

	case lcObjectPropertyId::LightPOVRayAreaGridX:
	case lcObjectPropertyId::LightPOVRayAreaGridY:
		return QT_TRANSLATE_NOOP("Checkpoint", "Change Area Light POV-Ray Grid");

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

#pragma once

#include "lc_math.h"
#include "lc_objectproperty.h"

enum class lcObjectType
{
	Piece,
	Camera,
	Light
};

struct lcObjectSection
{
	lcObject* Object = nullptr;
	quint32 Section = 0;
};

struct lcPieceInfoRayTest
{
	const PieceInfo* Info = nullptr;
	lcMatrix44 Transform;
	lcVector3 Plane;
};

struct lcObjectRayTest
{
	lcCamera* ViewCamera;
	bool PiecesOnly;
	bool IgnoreSelected;
	lcVector3 Start;
	lcVector3 End;
	float Distance = FLT_MAX;
	lcObjectSection ObjectSection;
	lcPieceInfoRayTest PieceInfoRayTest;
};

struct lcObjectBoxTest
{
	lcCamera* ViewCamera;
	lcVector4 Planes[6];
	std::vector<lcObject*> Objects;
};

#define LC_OBJECT_TRANSFORM_MOVE_X    0x001
#define LC_OBJECT_TRANSFORM_MOVE_Y    0x002
#define LC_OBJECT_TRANSFORM_MOVE_Z    0x004
#define LC_OBJECT_TRANSFORM_MOVE_XYZ (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z)
#define LC_OBJECT_TRANSFORM_ROTATE_X  0x010
#define LC_OBJECT_TRANSFORM_ROTATE_Y  0x020
#define LC_OBJECT_TRANSFORM_ROTATE_Z  0x040
#define LC_OBJECT_TRANSFORM_ROTATE_XYZ (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z)
#define LC_OBJECT_TRANSFORM_SCALE_X   0x100
#define LC_OBJECT_TRANSFORM_SCALE_Y   0x200
#define LC_OBJECT_TRANSFORM_SCALE_Z   0x400
#define LC_OBJECT_TRANSFORM_SCALE_XYZ (LC_OBJECT_TRANSFORM_SCALE_X | LC_OBJECT_TRANSFORM_SCALE_Y | LC_OBJECT_TRANSFORM_SCALE_Z)

class lcObject
{
public:
	lcObject(lcObjectType ObjectType);
	virtual ~lcObject();

	lcObject(const lcObject&) = delete;
	lcObject(lcObject&&) = delete;
	lcObject& operator=(const lcObject&) = delete;
	lcObject& operator=(lcObject&&) = delete;

public:
	bool IsPiece() const
	{
		return mObjectType == lcObjectType::Piece;
	}

	bool IsCamera() const
	{
		return mObjectType == lcObjectType::Camera;
	}

	bool IsLight() const
	{
		return mObjectType == lcObjectType::Light;
	}

	lcObjectType GetType() const
	{
		return mObjectType;
	}

	virtual bool IsSelected() const = 0;
	virtual bool IsSelected(quint32 Section) const = 0;
	virtual void SetSelected(bool Selected) = 0;
	virtual void SetSelected(quint32 Section, bool Selected) = 0;
	virtual bool IsFocused() const = 0;
	virtual bool IsFocused(quint32 Section) const = 0;
	virtual void SetFocused(quint32 Section, bool Focused) = 0;
	virtual quint32 GetFocusSection() const = 0;

	virtual void UpdatePosition(lcStep Step) = 0;
	virtual quint32 GetAllowedTransforms() const = 0;
	virtual lcVector3 GetSectionPosition(quint32 Section) const = 0;
	virtual void RayTest(lcObjectRayTest& ObjectRayTest) const = 0;
	virtual void BoxTest(lcObjectBoxTest& ObjectBoxTest) const = 0;
	virtual void DrawInterface(lcContext* Context, const lcScene& Scene) const = 0;
	virtual QVariant GetPropertyValue(lcObjectPropertyId PropertyId) const = 0;
	virtual bool SetPropertyValue(lcObjectPropertyId PropertyId, lcStep Step, bool AddKey, QVariant Value) = 0;
	virtual bool HasKeyFrame(lcObjectPropertyId PropertyId, lcStep Time) const = 0;
	virtual bool SetKeyFrame(lcObjectPropertyId PropertyId, lcStep Time, bool KeyFrame) = 0;
	virtual void RemoveKeyFrames() = 0;
	virtual QString GetName() const = 0;
	static QString GetCheckpointString(lcObjectPropertyId PropertyId);

private:
	lcObjectType mObjectType;
};

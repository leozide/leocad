#ifndef _LC_CAMERA_TARGET_H_
#define _LC_CAMERA_TARGET_H_

#include "lc_object.h"

// Camera Target key types.
enum LC_CAMERA_TARGET_KEY_TYPES
{
	LC_CAMERA_TARGET_POSITION,
	LC_CAMERA_TARGET_NUMKEYS
};

class lcCameraTarget : public lcObject
{
public:
	lcCameraTarget(lcCamera* Parent);
	virtual ~lcCameraTarget();

	// Render the camera target.
	void Render();

	// Base class implementation.
	virtual void ClosestRayIntersect(LC_CLICK_RAY* Ray) const;
	virtual bool IntersectsVolume(const class Vector4* Planes, int NumPlanes) const;
	virtual void Update(u32 Time);
	virtual void AddToScene(lcScene* Scene, const Matrix44& ParentWorld, int Color);
};

#endif // _LC_CAMERA_TARGET_H_

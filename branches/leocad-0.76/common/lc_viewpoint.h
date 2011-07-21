#ifndef _LC_VIEWPOINT_H_
#define _LC_VIEWPOINT_H_

class View;

#include "algebra.h"
#include "lc_array.h"

enum LC_DEFAULT_VIEWPOINTS
{
	LC_VIEWPOINT_FRONT,
	LC_VIEWPOINT_BACK,
	LC_VIEWPOINT_LEFT,
	LC_VIEWPOINT_RIGHT,
	LC_VIEWPOINT_TOP,
	LC_VIEWPOINT_BOTTOM,
	LC_VIEWPOINT_HOME
};

class lcViewpoint
{
public:
	lcViewpoint();
	~lcViewpoint();

	// Set one of the default viewpoints.
	void SetDefault(int Viewpoint);

	// Update mViewWorld and mWorldView using the values of mPosition, mTarget and mRoll.
	void CalculateMatrices();

	// Move the camera along its Z axis.
	void Zoom(u32 Time, bool AddKey, int MouseX, int MouseY);

	// Move the camera along its XY plane.
	void Pan(u32 Time, bool AddKey, int MouseX, int MouseY);

	// Rotate the camera around its target.
	void Orbit(u32 Time, bool AddKey, int MouseX, int MouseY);

	// Rotate the target around the camera.
	void Rotate(u32 Time, bool AddKey, int MouseX, int MouseY);

	// Rotate the camera around its Z axis.
	void Roll(u32 Time, bool AddKey, int MouseX, int MouseY);

	// Make sure all points are visible.
	void ZoomExtents(u32 Time, bool AddKey, View* view, lcObjArray<Vector3>& Points);

	// Set new position.
	virtual void SetPosition(u32 Time, bool AddKey, const Vector3& Position)
	{
		mPosition = Position;
	}

	// Set new target.
	virtual void SetTarget(u32 Time, bool AddKey, const Vector3& Target)
	{
		mTarget = Target;
	}

	// Set new roll.
	virtual void SetRoll(u32 Time, bool AddKey, float Roll)
	{
		mRoll = Roll;
	}

	float mNearDist;
	float mFarDist;
	float mFOV;

	Matrix44 mWorldView;
	Matrix44 mViewWorld;

	Vector3 mPosition;
	Vector3 mTarget;
	float mRoll;
};

#endif // _LC_VIEWPOINT_H_

#ifndef _LC_CAMERA_H_
#define _LC_CAMERA_H_

#include "lc_object.h"

// Camera types.
enum LC_CAMERA_TYPES
{
	LC_CAMERA_FRONT,
	LC_CAMERA_BACK,
	LC_CAMERA_TOP,
	LC_CAMERA_UNDER,
	LC_CAMERA_LEFT,
	LC_CAMERA_RIGHT,
	LC_CAMERA_MAIN,
	LC_CAMERA_USER
};

// Camera key types.
enum LC_CAMERA_KEY_TYPES
{
	LC_CAMERA_POSITION,
	LC_CAMERA_ROLL,
	LC_CAMERA_NUMKEYS
};

// Camera flags.
#define LC_CAMERA_ORTHOGRAPHIC      0x0100
#define LC_CAMERA_SHOW_CONE         0x0200
#define LC_CAMERA_AUTO_CLIP         0x0400
#define LC_CAMERA_SYSTEM            0x0800

class lcCamera : public lcObject
{
public:
	lcCamera();
	virtual ~lcCamera();

	// Initialize camera.
	void CreateCamera(int CameraType, bool Target);

	// Render the camera.
	void Render();

	// Toggle between orthographic and perspective projection.
	void SetOrtho(bool Ortho)
	{
		SetFlag(LC_CAMERA_ORTHOGRAPHIC, Ortho);
	}

	// Check if this camera is set to use an orthographic projection matrix.
	bool IsOrtho() const
	{
		return IsFlagged(LC_CAMERA_ORTHOGRAPHIC);
	}

	// Show the camera projection even when the camera isn't selected.
	void ShowCone(bool Show)
	{
		SetFlag(LC_CAMERA_SHOW_CONE, Show);
	}

	// FIXME: remove IsSide
	bool IsSide() const
	{ return m_CameraType < LC_CAMERA_MAIN; }

	// Change the roll value.
	void SetRoll(u32 Time, bool AddKey, float NewRoll);

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

	// Base class implementation.
	virtual void ClosestRayIntersect(LC_CLICK_RAY* Ray) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;
	virtual void Update(u32 Time);
	virtual void AddToScene(lcScene* Scene, int Color);

	// Base class overrides.
	void Move(u32 Time, bool AddKey, const Vector3& Delta);

public:
	// Camera properties.
	int m_CameraType;
	float m_NearDist;
	float m_FarDist;
	float m_FOV;

	// Temporary values.
	float m_Roll;
	Matrix44 m_WorldView;
	Matrix44 m_ViewWorld;
};

#endif // _LC_CAMERA_H_

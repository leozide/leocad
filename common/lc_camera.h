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

	// Change the roll value.
	void SetRoll(u32 Time, bool AddKey, const float NewRoll);

	// Move the camera along its Z direction.
	void Zoom(float MouseY, u32 Time, bool AddKey);

	// Move the camera along its XY plane.
	void Pan(float MouseX, float MouseY, u32 Time, bool AddKey);

	// Rotate the camera around its target.
	void Rotate(float MouseX, float MouseY, u32 Time, bool AddKey);

	// Rotate the camera around its Z direction.
	void Roll(u32 Time, bool AddKey, float MouseX, float MouseY);

	// Base class implementation.
	virtual void ClosestRayIntersect(LC_CLICK_RAY* Ray) const;
	virtual bool IntersectsVolume(const class Vector4* Planes, int NumPlanes) const;
	virtual void Update(u32 Time);
	virtual void AddToScene(lcScene* Scene, const Matrix44& ParentWorld, int Color);

	// Base class overrides.
	void Move(u32 Time, bool AddKey, const Vector3& Delta);

	// FIXME: Temp functions to get LeoCAD to compile again.
	void LoadProjection(float Aspect);
	void GetFrustumPlanes(float Aspect, Vector4 Planes[6]) const;
	void DoZoom(int dy, int mouse, u32 Time, bool AddKey)
	{ Zoom(2.0f*(float)dy/(21-mouse), Time, AddKey); }
	void DoPan(int dx, int dy, int mouse, u32 Time, bool AddKey)
	{ Pan(2.0f*dx/(21-mouse), 2.0f*dy/(21-mouse), Time, AddKey); }
	void lcCamera::DoRotate(int dx, int dy, int mouse, u32 Time, bool AddKey)
	{ Rotate(2.0f*dx/(21-mouse), 2.0f*dy/(21-mouse), Time, AddKey); }
	void lcCamera::DoRoll(int dx, int mouse, u32 Time, bool AddKey)
	{	Roll(Time, AddKey, 2.0f*dx/(21-mouse), 0); }
	bool IsSide() const
	{ return m_CameraType < LC_CAMERA_MAIN; }

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

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "opengl.h"
#include "object.h"
#include "algebra.h"
#include "lc_viewpoint.h"

#define LC_CAMERA_HIDDEN            0x01
#define LC_CAMERA_SELECTED          0x02
#define LC_CAMERA_FOCUSED           0x04
#define LC_CAMERA_TARGET_SELECTED   0x08
#define LC_CAMERA_TARGET_FOCUSED    0x10
#define LC_CAMERA_ORTHOGRAPHIC      0x20
#define LC_CAMERA_SHOW_CONE         0x40

class lcCamera;
class CameraTarget;

enum LC_CK_TYPES
{
	LC_CK_EYE,
	LC_CK_TARGET,
	LC_CK_ROLL,
	LC_CK_COUNT
};

class CameraTarget : public lcObject
{
public:
	CameraTarget(lcCamera* Parent);
	virtual ~CameraTarget();

	// Base class implementation.
	virtual void ClosestLineIntersect(lcClickLine& ClickLine) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;


	void Select(bool bSelecting, bool bFocus, bool bMultiple);
	void Move(u32 Time, bool AddKey, const Vector3& Delta)
	{
		// FIXME: move the position handling to the camera target
	}

	lcCamera* m_Parent;
};

class lcCamera : public lcObject, public lcViewpoint
{
public:
	lcCamera();
	lcCamera(const Vector3& Position, const Vector3& Target);
	lcCamera(lcCamera* Camera);
	virtual ~lcCamera();

	// Base class implementation.
	virtual void ClosestLineIntersect(lcClickLine& ClickLine) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;

	// Set new position.
	virtual void SetPosition(u32 Time, bool AddKey, const Vector3& Position)
	{
		lcViewpoint::SetPosition(Time, AddKey, Position);

		ChangeKey(Time, AddKey, mPosition, LC_CK_TARGET);
	}

	// Set new target.
	virtual void SetTarget(u32 Time, bool AddKey, const Vector3& Target)
	{
		lcViewpoint::SetTarget(Time, AddKey, Target);

		ChangeKey(Time, AddKey, mTarget, LC_CK_TARGET);
	}

	// Set new roll.
	virtual void SetRoll(u32 Time, bool AddKey, float Roll)
	{
		lcViewpoint::SetRoll(Time, AddKey, Roll);

		ChangeKey(Time, AddKey, &mRoll, LC_CK_ROLL);
	}





public:
	void Hide()
		{ m_nState = LC_CAMERA_HIDDEN; }
	void UnHide()
		{ m_nState &= ~LC_CAMERA_HIDDEN; }
	bool IsVisible()
		{ return (m_nState & LC_CAMERA_HIDDEN) == 0; }
	bool IsSelected()
		{ return (m_nState & (LC_CAMERA_SELECTED|LC_CAMERA_TARGET_SELECTED)) != 0; } 
	bool IsEyeSelected()
		{ return (m_nState & LC_CAMERA_SELECTED) != 0; } 
	bool IsTargetSelected()
		{ return (m_nState & LC_CAMERA_TARGET_SELECTED) != 0; } 
	bool IsEyeFocused()
		{ return (m_nState & LC_CAMERA_FOCUSED) != 0; } 
	bool IsTargetFocused()
		{ return (m_nState & LC_CAMERA_TARGET_FOCUSED) != 0; } 
	bool IsOrtho() const
		{ return (m_nState & LC_CAMERA_ORTHOGRAPHIC) != 0; }

	/*
	void Select()
		{ m_nState |= (LC_CAMERA_SELECTED|LC_CAMERA_TARGET_SELECTED); } 
	void UnSelect()
		{ m_nState &= ~(LC_CAMERA_SELECTED|LC_CAMERA_FOCUSED|LC_CAMERA_TARGET_SELECTED|LC_CAMERA_TARGET_FOCUSED); } 
	void UnFocus()
		{ m_nState &= ~(LC_CAMERA_FOCUSED|LC_CAMERA_TARGET_FOCUSED); } 
	void FocusEye()
		{ m_nState |= (LC_CAMERA_FOCUSED|LC_CAMERA_SELECTED); } 
	void FocusTarget()
		{ m_nState |= (LC_CAMERA_TARGET_FOCUSED|LC_CAMERA_TARGET_SELECTED); } 
	*/

	void SelectTarget(bool bSelecting, bool bFocus, bool bMultiple);

public:
	bool FileLoad(lcFile& file);
	void FileSave(lcFile& file) const;
	void Select(bool bSelecting, bool bFocus, bool bMultiple);
	void Move(u32 Time, bool AddKey, const Vector3& Delta);

	void UpdatePosition(u32 Time);
	void Render(float fLineWidth);

public:
	// Camera properties.
	float m_NearDist;
	float m_FarDist;
	float m_FOV;

	// Temporary values.
	Matrix44 m_WorldView;
	Matrix44 m_ViewWorld;

	// Current position.
	Vector3 m_Position;
	Vector3 m_TargetPosition;
	float m_Roll;

	// Camera target
	CameraTarget* m_Target;

protected:
	void Initialize();

	// Attributes
	unsigned char m_nState;
};

#endif // _CAMERA_H_

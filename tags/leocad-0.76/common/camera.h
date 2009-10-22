#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "opengl.h"
#include "object.h"
#include "algebra.h"

#define LC_CAMERA_HIDDEN            0x01
#define LC_CAMERA_SELECTED          0x02
#define LC_CAMERA_FOCUSED           0x04
#define LC_CAMERA_TARGET_SELECTED   0x08
#define LC_CAMERA_TARGET_FOCUSED    0x10
#define LC_CAMERA_ORTHOGRAPHIC      0x20
#define LC_CAMERA_SHOW_CONE         0x40

class lcCamera;
class CameraTarget;

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

class lcCamera : public lcObject
{
public:
	lcCamera();
	lcCamera(unsigned char nType);
	lcCamera(const Vector3& Position, const Vector3& Target);
	lcCamera(lcCamera* Camera);
	virtual ~lcCamera();

	// Base class implementation.
	virtual void ClosestLineIntersect(lcClickLine& ClickLine) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;

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





public:
	void Hide()
		{ m_nState = LC_CAMERA_HIDDEN; }
	void UnHide()
		{ m_nState &= ~LC_CAMERA_HIDDEN; }
	bool IsSide()
		{ return m_nType < LC_CAMERA_MAIN; }
	bool IsUser()
		{ return m_nType == LC_CAMERA_USER; }
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
	unsigned char m_nType;
};

#endif // _CAMERA_H_

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "object.h"
#include "algebra.h"

#define LC_LIGHT_HIDDEN           0x01
#define LC_LIGHT_SELECTED         0x02
#define LC_LIGHT_FOCUSED          0x04
#define LC_LIGHT_TARGET_SELECTED  0x08
#define LC_LIGHT_TARGET_FOCUSED   0x10
#define LC_LIGHT_ENABLED          0x20

class lcLight;
class LightTarget;

typedef enum
{
	LC_LK_POSITION,
	LC_LK_TARGET,
	LC_LK_AMBIENT_COLOR,
	LC_LK_DIFFUSE_COLOR,
	LC_LK_SPECULAR_COLOR,
	LC_LK_CONSTANT_ATTENUATION,
	LC_LK_LINEAR_ATTENUATION,
	LC_LK_QUADRATIC_ATTENUATION,
	LC_LK_SPOT_CUTOFF,
	LC_LK_SPOT_EXPONENT,
	LC_LK_COUNT
} LC_LK_TYPES;

class LightTarget : public lcObject
{
public:
	LightTarget(lcLight* Parent);
	~LightTarget();

	// Base class implementation.
	virtual void ClosestLineIntersect(lcClickLine& ClickLine) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;



public:
	void Select(bool bSelecting, bool bFocus, bool bMultiple);
	void Move(u32 Time, bool AddKey, const Vector3& Delta)
	{
		// FIXME: move the position handling to the light target
	}

	lcLight* m_Parent;
};

class lcLight : public lcObject
{
public:
	lcLight(float px, float py, float pz);
	lcLight(float px, float py, float pz, float tx, float ty, float tz);
	virtual ~lcLight();

	// Base class implementation.
	virtual void ClosestLineIntersect(lcClickLine& ClickLine) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;



	void Select(bool bSelecting, bool bFocus, bool bMultiple);
	void SelectTarget(bool bSelecting, bool bFocus, bool bMultiple);

public:
	bool IsVisible()
	{ return (m_nState & LC_LIGHT_HIDDEN) == 0; }
	bool IsSelected()
	{ return (m_nState & (LC_LIGHT_SELECTED|LC_LIGHT_TARGET_SELECTED)) != 0; }
	bool IsEyeSelected()
	{ return (m_nState & LC_LIGHT_SELECTED) != 0; }
	bool IsTargetSelected()
	{ return (m_nState & LC_LIGHT_TARGET_SELECTED) != 0; }
	bool IsEyeFocused()
	{ return (m_nState & LC_LIGHT_FOCUSED) != 0; }
	bool IsTargetFocused()
	{ return (m_nState & LC_LIGHT_TARGET_FOCUSED) != 0; }

	void Select()
	{ m_nState |= (LC_LIGHT_SELECTED|LC_LIGHT_TARGET_SELECTED); }
	void UnSelect()
	{ m_nState &= ~(LC_LIGHT_SELECTED|LC_LIGHT_FOCUSED|LC_LIGHT_TARGET_SELECTED|LC_LIGHT_TARGET_FOCUSED); }
	void UnFocus()
	{ m_nState &= ~(LC_LIGHT_FOCUSED|LC_LIGHT_TARGET_FOCUSED); }
	void FocusEye()
	{ m_nState |= (LC_LIGHT_FOCUSED|LC_LIGHT_SELECTED); }
	void FocusTarget()
	{ m_nState |= (LC_LIGHT_TARGET_FOCUSED|LC_LIGHT_TARGET_SELECTED); }
	LightTarget* GetTarget() const
	{ return m_Target; }

	void Render(float fLineWidth);
	void UpdatePosition(u32 Time);
	void Move(u32 Time, bool AddKey, const Vector3& Delta);
	void Setup(int index);

protected:
	void Initialize ();

	// Attributes
	float m_fCone;
	unsigned char m_nState;
	bool m_bEnabled;

	void DrawCone();
	void DrawTarget();
	void DrawSphere();

public:
	// Camera target
	LightTarget* m_Target;

	// Temporary parameters
	Matrix44 m_WorldLight;
	Vector3 m_Position;
	Vector3 m_TargetPosition;
	Vector4 m_AmbientColor;
	Vector4 m_DiffuseColor;
	Vector4 m_SpecularColor;
	float m_ConstantAttenuation;
	float m_LinearAttenuation;
	float m_QuadraticAttenuation;
	float m_SpotCutoff;
	float m_SpotExponent;
};

#endif // _LIGHT_H_

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "opengl.h"
#include "object.h"

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
	LightTarget(lcLight *pParent);
	~LightTarget();

public:
	void MinIntersectDist (LC_CLICKLINE* pLine);
	bool IntersectsVolume(const Vector4* Planes, int NumPlanes)
	{ return false; }
	void Select (bool bSelecting, bool bFocus, bool bMultiple);
	void Move (unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z)
	{
		// FIXME: move the position handling to the light target
	}

	lcLight* GetParent () const
	{ return m_pParent; }

protected:
	lcLight* m_pParent;

	friend class lcLight; // FIXME: needed for BoundingBoxCalculate ()
	// remove and use UpdatePosition instead
};

class lcLight : public lcObject
{
public:
	lcLight(float px, float py, float pz);
	lcLight(float px, float py, float pz, float tx, float ty, float tz);
	virtual ~lcLight();

	void Select (bool bSelecting, bool bFocus, bool bMultiple);
	void SelectTarget (bool bSelecting, bool bFocus, bool bMultiple);

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
	{ return m_pTarget; }

	void Render(float fLineWidth);
	void MinIntersectDist(LC_CLICKLINE* Line);
	bool IntersectsVolume(const Vector4* Planes, int NumPlanes)
	{ return false; }
	void UpdatePosition(unsigned short nTime, bool bAnimation);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz);
	void Setup(int index);
	void CreateName(const lcLight* pLight);

protected:
	void Initialize ();

	// Camera target
	LightTarget* m_pTarget;

	// Attributes
	float m_fCone;
	unsigned char m_nState;
	bool m_bEnabled;

	void DrawCone();
	void DrawTarget();
	void DrawSphere();

	// Temporary parameters
	float m_fPos[4];
	float m_fTarget[3];
	float m_fAmbient[4];
	float m_fDiffuse[4];
	float m_fSpecular[4];
	float m_fConstant;
	float m_fLinear;
	float m_fQuadratic;
	float m_fCutoff;
	float m_fExponent;
};

#endif // _LIGHT_H_

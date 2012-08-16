#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "opengl.h"
#include "object.h"
#include "lc_math.h"

#define LC_LIGHT_HIDDEN			0x01
#define LC_LIGHT_SELECTED		0x02
#define LC_LIGHT_FOCUSED		0x04
#define LC_LIGHT_TARGET_SELECTED	0x08
#define LC_LIGHT_TARGET_FOCUSED		0x10
#define LC_LIGHT_ENABLED		0x20

class Light;
class LightTarget;

enum LC_LK_TYPES
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
};

class LightTarget : public Object
{
public:
	LightTarget(Light *pParent);
	~LightTarget();

public:
	virtual void MinIntersectDist(lcClickLine* ClickLine);
	bool IntersectsVolume(const lcVector4 Planes[6])
	{ return false; }
	void Select (bool bSelecting, bool bFocus, bool bMultiple);
	void Move (unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z)
	{
		// FIXME: move the position handling to the light target
	}

	const char* GetName() const;

	Light* GetParent () const
	{ return m_pParent; }

protected:
	Light* m_pParent;

	friend class Light; // FIXME: needed for BoundingBoxCalculate ()
	// remove and use UpdatePosition instead
};

class Light : public Object
{
public:
	Light (float px, float py, float pz);
	Light (float px, float py, float pz, float tx, float ty, float tz);
	virtual ~Light ();

	void Select (bool bSelecting, bool bFocus, bool bMultiple);
	void SelectTarget (bool bSelecting, bool bFocus, bool bMultiple);

public:
	Light* m_pNext;

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
	const char* GetName()
	{ return m_strName; }
	LightTarget* GetTarget () const
	{ return m_pTarget; }

	const char* GetName() const
	{ return m_strName; };

	void Render(float fLineWidth);
	virtual void MinIntersectDist(lcClickLine* ClickLine);
	bool IntersectsVolume(const lcVector4 Planes[6])
	{ return false; }
	void UpdatePosition(unsigned short nTime, bool bAnimation);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz);
	void Setup(int index);
	void CreateName(const Light* pLight);

	// Temporary parameters
	lcMatrix44 mWorldLight;
	lcVector3 mPosition;
	lcVector3 mTargetPosition;
	lcVector4 mAmbientColor;
	lcVector4 mDiffuseColor;
	lcVector4 mSpecularColor;
	float mConstantAttenuation;
	float mLinearAttenuation;
	float mQuadraticAttenuation;
	float mSpotCutoff;
	float mSpotExponent;

protected:
	void Initialize();

	// Camera target
	LightTarget* m_pTarget;

	// Attributes
	float m_fCone;
	unsigned char m_nState;
	char m_strName[81];
	bool m_bEnabled;

	GLuint m_nList;
	static GLuint m_nSphereList;
	static GLuint m_nTargetList;
};

#endif // _LIGHT_H_

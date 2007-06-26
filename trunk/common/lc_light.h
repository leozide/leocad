#ifndef _LC_LIGHT_H_
#define _LC_LIGHT_H_

#include "lc_object.h"

enum LC_LIGHT_TYPES
{
	LC_LIGHT_POINT,
	LC_LIGHT_DIRECTIONAL,
	LC_LIGHT_SPOT
};

enum LC_LIGHT_KEY_TYPES
{
	LC_LIGHT_POSITION,
	LC_LIGHT_NUMKEYS
};

// Light flags.
//#define LC_LIGHT_SHOW_CONE         0x0100

class lcLight : public lcObject
{
public:
	lcLight();
	virtual ~lcLight();

	// Initializes light.
	void CreateLight(int LightType);

	// Setup light for rendering.
	void Setup(int LightIndex) const;

	// Base class implementation.
	virtual void ClosestRayIntersect(LC_CLICK_RAY* Ray) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;
	virtual void Update(u32 Time);
	virtual void AddToScene(lcScene* Scene, int Color);

public:
	// Temporary values.
	Vector3 m_AmbientColor;
	Vector3 m_DiffuseColor;
	Vector3 m_SpecularColor;
	float m_ConstantAttenuation;
	float m_LinearAttenuation;
	float m_QuadraticAttenuation;
	float m_SpotCutoff;
	float m_SpotExponent;
};

#endif // _LC_LIGHT_H_

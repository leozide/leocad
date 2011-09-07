#include "lc_global.h"
#include "lc_light.h"

lcLight::lcLight()
	: lcObject(LC_OBJECT_LIGHT, LC_LIGHT_NUMKEYS)
{
}

lcLight::~lcLight()
{
	delete m_Children;
}

void lcLight::CreateLight(int LightType)
{
	// FIXME: create light
}

void lcLight::Update(u32 Time)
{
	// FIXME: light update
}

void lcLight::AddToScene(lcScene* Scene, int Color)
{
	// fixme: addtoscene
}

void lcLight::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	// FIXME: light intersect
}

bool lcLight::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	// FIXME: light intersect
	return false;
}

void lcLight::Setup(int LightIndex) const
{
	// FIXME: light setup
}

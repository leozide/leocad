#pragma once

#include "lc_math.h"

enum class lcConnectionGender
{
	Male,
	Female
};

struct lcConnectionSection
{
	QString Shape; // R (round), A (axle), S (square), _L or L_ (radius extension)
	float Radius;
	float Length;
};

// Which scaling of a sub-file a shape survives; it is dropped from scaled sub-files otherwise.
enum class lcConnectionScale
{
	None,
	Length,
	Radius,
	LengthOrRadius
};

struct lcConnectionCylinder
{
	lcMatrix44 Transform; // In LDraw part coordinates, the shape runs along its -Y axis.
	lcConnectionGender Gender;
	std::vector<lcConnectionSection> Sections;
	QString Id;
	QString Group;
	bool Slide;
	bool Center;
	bool ClosedStart; // caps: A, the end at pos (or the near end of a centered shape).
	bool ClosedEnd; // caps: B, the far end along the shape.
	lcConnectionScale Scale;
	bool InheritMirrored; // mirror=cor (the default): flip a radius axis when used mirrored; mirror=none: drop it.
	std::string Source;

	lcVector3 GetPosition() const
	{
		return Transform.GetTranslation();
	}

	lcVector3 GetAxis() const
	{
		return lcNormalize(lcMul30(lcVector3(0.0f, -1.0f, 0.0f), Transform));
	}
};

// How connection shapes fit together, on lists of shapes and transforms, wherever the shapes come from.
class lcConnectivity
{
public:


	static float GetDeterminant(const lcMatrix44& m);
private:
};

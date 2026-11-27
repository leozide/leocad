#pragma once

#include "lc_math.h"

enum class lcConnectionGender : quint8
{
	Male,
	Female
};

enum class lcStudType : quint8
{
	Male,
	Female,
	None
};

struct lcCylinderConnection
{
	enum class SectionProfile : quint8 { Round, Axle, Square, FlexPrevious, FlexNext };
	enum class CylinderCaps : quint8 { None, A, B, Both };

	struct Section
	{
		SectionProfile Profile;
		float Radius;
		float Length;
	};

	std::vector<Section> Sections;
	lcConnectionGender Gender = lcConnectionGender::Male;
	CylinderCaps Caps = CylinderCaps::A;
	bool Centered = false;
	bool Sliding = false;
	lcStudType StudType = lcStudType::None;

	bool IsCapped(CylinderCaps End) const;
	const Section& GetNominalSection(size_t Index) const;
};

struct lcClipConnection
{
	float Radius = 4.0f;
	float Length = 8.0f;
	bool Centered = false;
	bool Sliding = false;
};

struct lcFingersConnection
{
	std::vector<float> Widths;
	float Radius = 0.0f;
	lcConnectionGender FirstGender = lcConnectionGender::Male;
	bool Centered = true;
};

struct lcGenericConnection
{
	enum class Bounds : quint8 { Point, Box, Cube, Cylinder, Sphere };

	Bounds BoundsShape = Bounds::Point;
	// Box half-extents use connector-local LeoCAD axes. Other bounds keep the
	// parameter order of the Shadow declaration.
	lcVector3 Dimensions = lcVector3(0.0f, 0.0f, 0.0f);
	lcConnectionGender Gender = lcConnectionGender::Male;
	std::string MatchRule = "shape";
	std::string PlacementMode;

	lcVector3 GetScaledDimensions(const lcVector3& Scale) const;
};

// Offsets are measured along each frame's local +Z. Direction is the sign
// relating the two axes after mating. Travel is the allowed axial displacement.
struct lcConnectionMating
{
	float FirstOffset;
	float SecondOffset;
	float Direction = 1.0f;
	float Travel = 0.0f;
	bool TravelOnFirst = false;
	bool AxleInRoundHole = false;
	float ClipBarHalfSpan = 0.0f;
};

struct lcConnection
{
	// Returned frames map from connector-local to piece-local LeoCAD coordinates.
	// Apply Scale along the connector's local axes first. The connector axis is +Z.
	lcMatrix44 Frame = lcMatrix44Identity();
	lcVector3 Scale = lcVector3(1.0f, 1.0f, 1.0f);
	std::string Group;
	std::variant<lcCylinderConnection, lcClipConnection, lcFingersConnection, lcGenericConnection> Shape;

	float GetAxialReach() const;
	std::vector<lcConnectionMating> GetMatings(const lcConnection& Other) const;
	bool IsCompatibleWith(const lcConnection& Other) const;
};

using lcConnections = std::vector<lcConnection>;

bool lcSaveConnections(lcFile& File, const lcConnections& Connections);
bool lcLoadConnections(lcFile& File, lcConnections& Connections);

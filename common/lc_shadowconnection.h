#pragma once

#include "lc_math.h"

enum class lcShadowGender : quint8
{
	Male,
	Female
};

struct lcShadowCylinder
{
	enum class Profile : quint8 { Round, Axle, Square, FlexPrevious, FlexNext };
	enum class Caps : quint8 { None, A, B, Both };

	struct Section
	{
		Profile Shape;
		float Radius;
		float Length;
	};

	std::vector<Section> Sections;
	lcShadowGender Gender = lcShadowGender::Male;
	Caps Ends = Caps::A;
	bool Centered = false;
	bool Sliding = false;
};

struct lcShadowClip
{
	float Radius = 4.0f;
	float Length = 8.0f;
	bool Centered = false;
	bool Sliding = false;
};

struct lcShadowFingers
{
	std::vector<float> Widths;
	float Radius = 0.0f;
	lcShadowGender FirstGender = lcShadowGender::Male;
	bool Centered = true;
};

struct lcShadowGenericConnector
{
	enum class Bounds : quint8 { Point, Box, Cube, Cylinder, Sphere };

	Bounds Shape = Bounds::Point;
	// Box half-extents use connector-local LeoCAD axes. Other bounds keep the
	// parameter order of the Shadow declaration.
	lcVector3 Dimensions = lcVector3(0.0f, 0.0f, 0.0f);
	lcShadowGender Gender = lcShadowGender::Male;
	QByteArray Match = "shape";
	QByteArray Placement;
};

struct lcShadowConnection
{
	// Returned frames map from connector-local to piece-local LeoCAD coordinates.
	// Apply Scale along the connector's local axes first. The connector axis is +Z.
	lcMatrix44 Frame = lcMatrix44Identity();
	lcVector3 Scale = lcVector3(1.0f, 1.0f, 1.0f);
	QByteArray Group;
	std::variant<lcShadowCylinder, lcShadowClip, lcShadowFingers, lcShadowGenericConnector> Shape;
};

struct lcShadowConnectionData
{
	std::vector<lcShadowConnection> Connections;
	// Distinguishes an authored empty result (e.g. SNAP_CLEAR) from missing definitions.
	bool HasDefinitions = false;
};

lcShadowConnectionData lcLoadShadowConnections(lcPiecesLibrary& Library, lcFile& File, const QString& ShadowPath);

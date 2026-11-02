#pragma once

#include "lc_connectivity.h"

// Connection shapes ("snap info") in the LDCad shadow library format:
// https://www.melkert.net/LDCad/tech/meta
// Only SNAP_CYL, SNAP_INCL and SNAP_CLEAR are supported (studs, anti studs,
// holes, axles and bars); clips, fingers and generic shapes are ignored.

enum class lcShadowLineType
{
	Cylinder,
	Include,
	Clear
};

struct lcShadowLine
{
	lcShadowLineType Type;
	std::vector<lcConnectionCylinder> Cylinders; // SNAP_CYL: one per grid position.
	std::string Ref; // SNAP_INCL: the file whose own cylinders are placed at each of Placements.
	std::vector<lcMatrix44> Placements;
	QString Id; // SNAP_CLEAR: the id of the shapes to remove, or empty to remove all.
};

class lcShadowParser
{
public:
	// A line of a shadow file, or nothing if it isn't a supported, well-formed snap line.
	static std::optional<lcShadowLine> ParseLine(const QString& Line, const std::string& Source);

	// For placing the shapes of sub-files: the file name and transform of an LDraw type 1 line,
	// or nothing for other lines.
	static std::optional<std::pair<std::string, lcMatrix44>> ParseReference(const char* Line);
	static lcMatrix44 LDrawMatrix(const float* Ori, const lcVector3& Position);
	static bool NormalizeFrame(lcConnectionCylinder& Cylinder);

private:
	static std::vector<lcConnectionCylinder> ParseCylinder(const std::map<QString, QString>& Params, const std::string& Source);
	static std::map<QString, QString> ParseParams(const QString& Line);
	static QString GetParam(const std::map<QString, QString>& Params, const char* Key);
	static std::optional<lcMatrix44> ParseTransform(const std::map<QString, QString>& Params);
	static std::optional<std::vector<lcVector3>> ParseGridOffsets(const std::map<QString, QString>& Params);
	static lcMatrix44 GetOffsetTransform(const lcMatrix44& Transform, const lcVector3& Offset);
	static QStringList SplitTokens(const QString& Text);
	static bool IsParamTrue(const std::map<QString, QString>& Params, const char* Key);
	static std::optional<std::vector<float>> ParseNumbers(const QString& Text, qsizetype Count);
};

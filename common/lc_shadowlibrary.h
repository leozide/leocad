#pragma once

#include "lc_shadowparser.h"

// The connection shapes of library parts, read from the LDCad shadow library and inherited from
// the parts' sub-files.
class lcShadowLibrary
{
public:
	// An empty path, or one without a shadow library, leaves all parts without connection shapes.
	static bool SetPath(const QString& Path);
	static bool IsLoaded();

	// PartName is a library file name such as "3001.dat" or "s/3001s01.dat".
	static const std::vector<lcConnectionCylinder>& GetCylinders(const char* PartName);

private:
	static std::string NormalizeName(const std::string& Name);
	static const std::vector<lcShadowLine>& GetShadowLines(const std::string& Name);
	static std::vector<lcConnectionCylinder> GetOwnCylinders(const std::string& Name);
	static std::vector<std::pair<std::string, lcMatrix44>> ReadReferences(const std::string& Name);
	static void AddPlacedCylinders(const std::vector<lcConnectionCylinder>& Cylinders, const lcMatrix44& Transform, std::vector<lcConnectionCylinder>& Result);
	static void ApplyShadowLine(const lcShadowLine& Line, std::vector<lcConnectionCylinder>& Cylinders);

	static std::map<std::string, QString> mShadowFiles;
	static std::map<std::string, std::vector<lcShadowLine>> mShadowLines;
	static std::map<std::string, std::vector<lcConnectionCylinder>> mCylinders;
};

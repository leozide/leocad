#pragma once

#include "lc_math.h"
#include "lc_result.h"

class lcPiecesLibrary;
class lcZipFile;

enum class lcConnectionType
{
	StudMale,
	StudReceiver
};

struct lcConnectionPoint
{
	lcConnectionType Type;
	lcVector3 Position;
	lcVector3 Direction;
};

class lcConnectionInfo
{
public:
	explicit lcConnectionInfo(std::vector<lcConnectionPoint> Connections)
		: mConnections(std::move(Connections))
	{
	}

	static void Initialize(lcPiecesLibrary* Library);

	const std::vector<lcConnectionPoint>& GetConnections() const
	{
		return mConnections;
	}

protected:
	static lcResult<void> LoadArchive(lcZipFile& Archive, lcPiecesLibrary* Library);
	static bool ParsePart(const QByteArray& Data, const std::string& PartName, int ExpectedVersion, int ExpectedConnections, std::vector<lcConnectionPoint>& Connections);
	static bool ParseVector(const QJsonValue& Value, lcVector3& Vector);
	
	std::vector<lcConnectionPoint> mConnections;
};

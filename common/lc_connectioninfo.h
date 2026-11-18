#pragma once

#include "lc_math.h"
#include "lc_result.h"

class lcModel;
class lcPiece;
class lcPieceData;
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

struct lcConnectionSnapContext
{
	lcMatrix44 ModelToClip;
	lcVector2 LogicalViewportSize;
	lcVector3 ViewDirection;
	lcVector2 Cursor;
	const lcPiece* SurfacePiece;
};

struct lcConnectionDragState
{
	static constexpr float DefaultReleaseRadius = 16.0f;

	struct Endpoint
	{
		const lcPiece* Piece;
		lcConnectionPoint Connection;
		std::optional<bool> Occupied;
	};

	const lcModel* Model = nullptr;
	lcStep Step = 0;
	quint64 HistoryRevision = 0;
	const lcPieceData* InsertData = nullptr;
	std::vector<Endpoint> Stationary;
	std::optional<std::pair<size_t, size_t>> Pair;
	lcVector2 ScreenCorrection = lcVector2(0, 0);
	lcVector2 ReleaseCenter = lcVector2(0, 0);
	float ReleaseRadius = DefaultReleaseRadius;
	lcMatrix44 SnappedTransform = lcMatrix44Identity();
};

class lcConnectionInfo
{
public:
	explicit lcConnectionInfo(std::vector<lcConnectionPoint> Connections)
		: mConnections(std::move(Connections))
	{
	}

	static void Initialize(lcPiecesLibrary* Library);
	static bool AreStudDirectionsAligned(const lcVector3& FirstDirection, const lcVector3& SecondDirection);
	static std::optional<lcVector3> GetStudSnapTranslation(const lcConnectionPoint& MovingConnection, const lcMatrix44& MovingTransform, const lcConnectionPoint& TargetConnection, const lcMatrix44& TargetTransform);

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

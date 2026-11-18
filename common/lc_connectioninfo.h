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
	const lcPiece* SurfacePiece;
	bool HasSurfaceHit;
};

struct lcConnectionDragState
{
	static constexpr float DefaultReleaseRadius = 16.0f;

	bool IsWithinReleaseRadius(const lcVector2& Cursor) const;

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

struct lcStudMatingFace
{
	lcVector3 Point;
	float PlaneZ;
	bool IsTop;
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

	// Stationary endpoints are sorted by model-space X. Matching updates the retained pair and occupancy cache.
	static std::optional<lcVector3> FindStudSnap(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, lcConnectionDragState& DragState);

	std::optional<lcStudMatingFace> FindStudMatingFace(const lcVector3& HitPosition, const lcVector3& RayDirection, float HitDistance, const lcBoundingBox& SurfaceBounds) const;
	static lcMatrix44 GetStudMatingTransform(const lcStudMatingFace& Face, const lcMatrix44& SurfaceTransform, const lcMatrix33& SnapRotation, const lcVector3& GrabOffset, float PlacementOffset, const lcConnectionInfo* MovingConnections);
	static lcMatrix33 GetStudAxisRotation(const lcMatrix33& StartRotation, const lcVector3& TargetDirection);

	const std::vector<lcConnectionPoint>& GetConnections() const
	{
		return mConnections;
	}

protected:
	static lcResult<void> LoadArchive(lcZipFile& Archive, lcPiecesLibrary* Library);
	static bool ParsePart(const QByteArray& Data, const std::string& PartName, int ExpectedVersion, int ExpectedConnections, std::vector<lcConnectionPoint>& Connections);
	static bool ParseVector(const QJsonValue& Value, lcVector3& Vector);

	float GetStudMatingOffset(lcConnectionType Type, const lcMatrix33& Rotation, const lcVector3& SurfaceAxis, float PlacementOffset) const;
	static std::vector<lcConnectionDragState::Endpoint>::const_iterator FindEndpointByX(const std::vector<lcConnectionDragState::Endpoint>& Endpoints, float X);
	static bool IsOccupied(lcConnectionDragState::Endpoint& Point, const std::vector<lcConnectionDragState::Endpoint>& Endpoints, bool Sorted);
	static bool HasOccupiedContact(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const lcMatrix44& MovingToProposed, const lcVector3& Translation);
	static std::optional<float> GetStudPairScreenDistanceSquared(const lcConnectionPoint& Proposed, const lcConnectionDragState::Endpoint& Target, const lcVector2& MovingScreen, const lcConnectionSnapContext& Context, bool Retained, lcVector2& ScreenCorrection);
	static std::optional<lcVector2> ProjectEndpoint(const lcConnectionSnapContext& Context, const lcVector3& Position);

	std::vector<lcConnectionPoint> mConnections;
};

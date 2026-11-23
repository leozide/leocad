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

struct lcStudTurnContext
{
	lcVector3 GrabPoint;
	lcVector3 FacePoint;
	lcVector3 FaceDirection;
	lcConnectionType TargetType;
};

struct lcConnectionSnapContext
{
	lcMatrix44 ModelToClip;
	lcVector2 LogicalViewportSize;
	lcVector3 ViewDirection;
	const lcPiece* SurfacePiece;
	bool HasSurfaceHit;
	std::optional<lcStudTurnContext> Turn = std::nullopt;
	lcVector2 Cursor = lcVector2(0, 0);
};

struct lcConnectionDragState
{
	static constexpr float DefaultReleaseRadius = 16.0f;

	float GetReleaseDistanceSquared(const lcVector2& Cursor) const;
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
	lcMatrix33 ProposalRotation = lcMatrix33Identity();
	float TurnAngle = 0.0f;
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

	// Returns a correction from the proposed pose. Stationary endpoints are sorted by model-space X.
	static std::optional<lcMatrix44> FindStudSnap(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, lcConnectionDragState& DragState);

	std::optional<lcStudMatingFace> FindStudMatingFace(const lcVector3& HitPosition, const lcVector3& RayDirection, float HitDistance, const lcBoundingBox& SurfaceBounds) const;
	static lcMatrix44 GetStudMatingTransform(const lcStudMatingFace& Face, const lcMatrix44& SurfaceTransform, const lcMatrix33& SnapRotation, const lcVector3& GrabOffset, float PlacementOffset, const lcConnectionInfo* MovingConnections);
	static lcMatrix33 GetStudAxisRotation(const lcMatrix33& StartRotation, const lcVector3& TargetDirection);

	const std::vector<lcConnectionPoint>& GetConnections() const
	{
		return mConnections;
	}

protected:
	struct StudSnapAnchor
	{
		std::pair<size_t, size_t> Pair;
		lcVector3 ProposedPosition;
		float DistanceSquared;
	};

	struct StudSnapCandidate
	{
		size_t AnchorIndex;
		lcMatrix44 Correction;
		float Angle;
		lcVector2 SnappedGrabScreen;
		float GrabDistanceSquared;
	};

	static constexpr int ArchiveVersion = 3;

	static bool IsArchivePartName(std::string_view Name);
	static lcResult<void> LoadArchive(lcZipFile& Archive, lcPiecesLibrary* Library);
	static bool ParsePart(const QByteArray& Data, const std::string& PartName, int ExpectedConnections, std::vector<lcConnectionPoint>& Connections);
	static bool ParseVector(const QJsonValue& Value, lcVector3& Vector);

	float GetStudMatingOffset(lcConnectionType Type, const lcMatrix33& Rotation, const lcVector3& SurfaceAxis, float PlacementOffset) const;
	static std::vector<lcConnectionDragState::Endpoint>::const_iterator FindEndpointByX(const std::vector<lcConnectionDragState::Endpoint>& Endpoints, float X);
	static bool IsOccupied(lcConnectionDragState::Endpoint& Point, const std::vector<lcConnectionDragState::Endpoint>& Endpoints, bool Sorted);
	static std::optional<size_t> GetStudContactCount(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const lcMatrix44& MovingToSnapped, bool CountContacts);
	static lcMatrix44 GetStudTurnCorrection(const lcVector3& MovingAnchor, const lcConnectionPoint& Target, float Angle);
	static bool IsStudTurnTarget(const lcConnectionDragState::Endpoint& Target, const lcConnectionSnapContext& Context);
	static std::vector<float> GetStudTurnAngles(const std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const StudSnapAnchor& Anchor, const lcMatrix44& MovingToProposed);
	static std::optional<StudSnapCandidate> FindStudSnapWithTurn(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const std::vector<StudSnapAnchor>& Anchors, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, const lcVector2& GrabScreen, const std::optional<lcMatrix44>& UnturnedCorrection);
	static std::optional<float> GetStudPairScreenDistanceSquared(const lcConnectionPoint& Proposed, const lcConnectionDragState::Endpoint& Target, const lcVector2& MovingScreen, const lcConnectionSnapContext& Context, bool Retained, lcVector2& ScreenCorrection);
	static std::optional<float> GetStudGrabScreenDistanceSquared(const lcMatrix44& Correction, const lcConnectionSnapContext& Context, const lcVector2& GrabScreen, lcVector2& SnappedGrabScreen);
	static std::optional<lcVector2> ProjectEndpoint(const lcConnectionSnapContext& Context, const lcVector3& Position);

	std::vector<lcConnectionPoint> mConnections;
};

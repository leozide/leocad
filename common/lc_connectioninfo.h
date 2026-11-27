#pragma once

#include "lc_math.h"
#include "lc_connection.h"

class lcModel;
class lcPiece;
class lcPieceData;

struct lcConnectionPoint
{
	lcStudType Type;
	lcVector3 Position;
	lcVector3 Direction;
	lcVector3 Transverse;
	// Borrowed from the piece data until the next connection publication revision.
	const lcConnection* Connection;
	float Reach;

	lcConnectionPoint(const lcConnection& SourceConnection, const lcMatrix44& Transform);
	void Transform(const lcMatrix44& Transform);
};

struct lcStudTurnContext
{
	lcVector3 GrabPoint;
	lcVector3 FacePoint;
	lcVector3 FaceDirection;
	lcStudType TargetType;
};

struct lcConnectionSnapRay
{
	lcVector3 Start;
	lcVector3 End;
	lcVector3 ProposedGrab;
	float HitDistance;
	const lcPiece* HitPiece;
	bool RayOnly;
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
	std::optional<lcConnectionSnapRay> PlacementRay = std::nullopt;
	std::function<lcConnectionSnapRay(const lcVector3&)> RayAtPoint = {};
};

struct lcConnectionDragState
{
	static constexpr float DefaultReleaseRadius = 16.0f;

	float GetReleaseDistanceSquared(const lcVector2& Cursor) const;
	bool IsWithinReleaseRadius(const lcVector2& Cursor) const;

	struct Occupancy
	{
		bool Exclusive = false;
		std::vector<std::pair<float, float>> SlidingSpans;

		bool Overlaps(const std::pair<float, float>& Span) const;
	};

	struct EndpointId
	{
		const lcPiece* Piece;
		const lcPieceData* PieceData;
		size_t Index;

		bool operator==(const EndpointId& Other) const
		{
			return Piece == Other.Piece && PieceData == Other.PieceData && Index == Other.Index;
		}
	};

	struct Endpoint
	{
		const lcPiece* Piece;
		lcConnectionPoint Connection;
		std::optional<Occupancy> Occupied;
		const lcPieceData* PieceData = nullptr;
		size_t Index = 0;

		EndpointId GetId() const
		{
			return EndpointId{ Piece, PieceData, Index };
		}
	};

	const lcModel* Model = nullptr;
	lcStep Step = 0;
	quint64 HistoryRevision = 0;
	quint64 ConnectionsRevision = 0;
	float StationaryReach = 0.0f;
	const lcPieceData* InsertData = nullptr;
	std::unordered_map<const lcPieceData*, quint64> SourceRevisions;
	std::vector<Endpoint> Stationary;
	std::optional<std::pair<size_t, size_t>> Pair;
	std::optional<EndpointId> MovingConnection;
	lcVector2 ScreenCorrection = lcVector2(0, 0);
	lcVector2 ReleaseCenter = lcVector2(0, 0);
	float ReleaseRadius = DefaultReleaseRadius;
	lcMatrix44 SnappedTransform = lcMatrix44Identity();
	lcMatrix33 ProposalRotation = lcMatrix33Identity();
	float TurnAngle = 0.0f;
	bool IsClipSnap = false;
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
	static bool CanSnapToRay(const lcConnection& Connection);
	static bool AreStudDirectionsAligned(const lcVector3& FirstDirection, const lcVector3& SecondDirection);

	// Returns a correction from the proposed pose. Stationary endpoints are sorted by model-space X.
	static std::optional<lcMatrix44> FindSnap(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, lcConnectionDragState& DragState);
	static bool IsSnapSeated(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, lcConnectionDragState& DragState);

	static std::optional<lcStudMatingFace> FindStudMatingFace(const lcConnections& SurfaceConnections, const lcVector3& HitPosition, const lcVector3& RayDirection, float HitDistance, const lcBoundingBox& SurfaceBounds, bool& HasVerticalStuds);
	static lcMatrix44 GetStudMatingTransform(const lcStudMatingFace& Face, const lcMatrix44& SurfaceTransform, const lcMatrix33& SnapRotation, const lcVector3& GrabOffset, float PlacementOffset, const lcConnections& MovingConnections);
	static lcMatrix33 GetStudAxisRotation(const lcMatrix33& StartRotation, const lcVector3& TargetDirection);

private:
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

	struct SnapCorrection
	{
		lcMatrix44 Transform;
		lcVector3 ProposedAnchor;
		lcVector3 SeatedAnchor;
		bool SlidesOnMoving = false;
	};

	struct RaySnapCandidate
	{
		lcMatrix44 Transform;
		std::pair<size_t, size_t> Pair;
		float Depth;
		bool DirectHit;
		float CorrectionDistanceSquared;

		bool IsAfter(bool OtherDirectHit, float OtherDepth) const;
		bool NeedsMoreCorrection(float OtherDistanceSquared, const lcMatrix44& OtherTransform) const;
	};

	static float GetStudMatingOffset(const lcConnections& Connections, lcStudType Type, const lcMatrix33& Rotation, const lcVector3& SurfaceAxis, float PlacementOffset);
	static const lcCylinderConnection* GetThroughBore(const lcConnection& Connection);
	static const lcCylinderConnection* GetBoreInsert(const lcConnection& Connection);
	static const lcCylinderConnection* GetClipBar(const lcConnection& Connection);
	static bool IsClipMateVisible(const lcConnectionDragState::Endpoint& Target, const lcVector3& Seat, const lcConnectionSnapRay& Ray);
	static std::optional<RaySnapCandidate> FindBoreSnap(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, lcConnectionDragState& DragState);
	static std::optional<RaySnapCandidate> FindClipSnap(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, lcConnectionDragState& DragState);
	static std::optional<SnapCorrection> GetSnapCorrection(const lcConnectionPoint& Moving, const lcConnectionPoint& Target, bool AlreadySeated, std::optional<float> PickedBarPosition);
	static std::vector<lcConnectionDragState::Endpoint>::const_iterator FindEndpointByX(const std::vector<lcConnectionDragState::Endpoint>& Endpoints, float X);
	static std::pair<float, float> GetSeatedSpan(const lcConnectionPoint& Host, const lcConnectionPoint& Mate);
	static const lcConnectionDragState::Occupancy& GetOccupancy(lcConnectionDragState::Endpoint& Point, const std::vector<lcConnectionDragState::Endpoint>& Endpoints, bool Sorted, float Reach);
	static std::optional<size_t> GetSnapContactCount(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const lcMatrix44& MovingToSnapped, float StationaryReach, bool CountContacts);
	static lcMatrix44 GetStudTurnCorrection(const lcVector3& MovingAnchor, const lcConnectionPoint& Target, float Angle);
	static bool IsStudTurnTarget(const lcConnectionDragState::Endpoint& Target, const lcConnectionSnapContext& Context);
	static std::vector<float> GetStudTurnAngles(const std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const StudSnapAnchor& Anchor, const lcMatrix44& MovingToProposed);
	static std::optional<StudSnapCandidate> FindStudSnapWithTurn(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const std::vector<StudSnapAnchor>& Anchors, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, const lcVector2& GrabScreen, float StationaryReach, const std::optional<lcMatrix44>& UnturnedCorrection);
	static std::optional<float> GetPairScreenDistanceSquared(const lcConnectionPoint& Proposed, const lcConnectionDragState::Endpoint& Target, const SnapCorrection& Correction, const lcConnectionSnapContext& Context, bool Retained, lcVector2& ScreenCorrection);
	static std::optional<float> GetStudGrabScreenDistanceSquared(const lcMatrix44& Correction, const lcConnectionSnapContext& Context, const lcVector2& GrabScreen, lcVector2& SnappedGrabScreen);
	static std::optional<lcVector2> ProjectEndpoint(const lcConnectionSnapContext& Context, const lcVector3& Position);
};

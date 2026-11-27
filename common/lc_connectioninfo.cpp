#include "lc_global.h"
#include "lc_connectioninfo.h"

namespace
{
	constexpr float ScreenLimit = 12.0f;
	constexpr float SwitchMargin = 3.0f;
	// Distances in LDraw units.
	constexpr float WorldLimit = 20.0f;
	constexpr float DepthLimit = 8.0f;
	constexpr float OccupiedTolerance = 0.1f;
	constexpr float HalfStudPitch = 10.0f;
	constexpr float MaxStudTurn = LC_PI / 4.0f;
	constexpr float StudTurnTolerance = 0.001f;
	constexpr float StudTurnNeighborDistance = 4.0f * HalfStudPitch;
	constexpr float StudTurnGrabScreenLimit = 2.0f * ScreenLimit;
	constexpr size_t MaxStudTurnAnchors = 8;

	struct lcClipPickingRay
	{
		lcClipPickingRay(const lcConnectionPoint& Point, const lcConnectionSnapRay& Ray, float RayLength)
		{
			const lcVector3 Across = lcCross(Point.Direction, Point.Transverse);
			const lcVector3& Scale = Point.Connection->Scale;
			auto ToLocal = [&](const lcVector3& Vector)
			{
				return lcVector3(lcDot(Vector, Point.Transverse) / Scale.x, lcDot(Vector, Across) / Scale.y, lcDot(Vector, Point.Direction));
			};
			Start = ToLocal(Ray.Start - Point.Position);
			Direction = ToLocal((Ray.End - Ray.Start) / RayLength);
			LocalDistancePerUnit = lcLength(Direction);
			Length = RayLength;
			VisibleDepth = lcMin(Length, Ray.HitDistance + OccupiedTolerance);
		}

		std::optional<float> Intersect(float AxialStart, float AxialLength, float Radius) const
		{
			const lcVector3 VolumeStart = Start - lcVector3(0, 0, AxialStart);
			float LocalDistance;
			if (!lcCylinderRayMinIntersectDistance(Radius, AxialLength, VolumeStart, VolumeStart + Direction, &LocalDistance))
				return std::nullopt;

			const float Depth = LocalDistance / LocalDistancePerUnit;
			return Depth <= Length ? std::optional<float>(Depth) : std::nullopt;
		}

		lcVector3 Start, Direction;
		float LocalDistancePerUnit, Length, VisibleDepth;
	};
}

float lcConnectionDragState::GetReleaseDistanceSquared(const lcVector2& Cursor) const
{
	const float X = Cursor.x - ReleaseCenter.x;
	const float Y = Cursor.y - ReleaseCenter.y;

	return X * X + Y * Y;
}

bool lcConnectionDragState::IsWithinReleaseRadius(const lcVector2& Cursor) const
{
	return GetReleaseDistanceSquared(Cursor) < ReleaseRadius * ReleaseRadius;
}

bool lcConnectionInfo::AreStudDirectionsAligned(const lcVector3& FirstDirection, const lcVector3& SecondDirection)
{
	// The male axis points out from its base and the receiver axis points in from its opening.
	constexpr float MinAlignment = 0.9999f;
	const float Alignment = lcDot(FirstDirection, SecondDirection) / (FirstDirection.Length() * SecondDirection.Length());

	return std::isfinite(Alignment) && Alignment >= MinAlignment;
}

lcConnectionPoint::lcConnectionPoint(const lcConnection& SourceConnection, const lcMatrix44& PieceTransform)
	: Type(lcStudType::None), Connection(&SourceConnection), Reach(SourceConnection.GetAxialReach())
{
	const lcMatrix44 Frame = lcMul(SourceConnection.Frame, PieceTransform);
	Position = Frame.GetTranslation();
	Direction = lcNormalize(lcVector3(Frame[2]));
	Transverse = lcNormalize(lcVector3(Frame[0]) - lcDot(lcVector3(Frame[0]), Direction) * Direction);

	if (const lcCylinderConnection* Cylinder = std::get_if<lcCylinderConnection>(&SourceConnection.Shape))
		Type = Cylinder->StudType;
}

void lcConnectionPoint::Transform(const lcMatrix44& Transform)
{
	Position = lcMul31(Position, Transform);
	Direction = lcMul30(Direction, Transform);
	Transverse = lcMul30(Transverse, Transform);
}

std::optional<lcConnectionInfo::SnapCorrection> lcConnectionInfo::GetSnapCorrection(const lcConnectionPoint& Moving, const lcConnectionPoint& Target, bool AlreadySeated, std::optional<float> PickedBarPosition)
{
	const bool StudPair = Moving.Type != lcStudType::None && Target.Type != lcStudType::None;
	const float BaseDistance = AlreadySeated ? OccupiedTolerance : WorldLimit;
	const float SearchDistance = StudPair ? BaseDistance : BaseDistance + Moving.Reach + Target.Reach;
	if (lcLengthSquared(Target.Position - Moving.Position) > SearchDistance * SearchDistance)
		return std::nullopt;

	if (StudPair && (Moving.Type == Target.Type || !AreStudDirectionsAligned(Moving.Direction, Target.Direction)))
		return std::nullopt;

	const std::vector<lcConnectionMating> Matings = Moving.Connection->GetMatings(*Target.Connection);
	if (Matings.empty())
		return std::nullopt;

	if (StudPair)
		return SnapCorrection{ lcMatrix44Translation(Target.Position - Moving.Position), Moving.Position, Target.Position };

	const lcGenericConnection* Generic = std::get_if<lcGenericConnection>(&Moving.Connection->Shape);
	auto RetainsOrientation = [](const lcGenericConnection& Connection)
	{
		return Connection.PlacementMode == "free" || Connection.PlacementMode == "retain";
	};
	const bool KeepOrientation = Generic && (RetainsOrientation(*Generic) || RetainsOrientation(std::get<lcGenericConnection>(Target.Connection->Shape)));
	auto IsKeyed = [](const lcConnection& Connection)
	{
		const lcCylinderConnection* Cylinder = std::get_if<lcCylinderConnection>(&Connection.Shape);
		return Cylinder && std::any_of(Cylinder->Sections.begin(), Cylinder->Sections.end(), [](const lcCylinderConnection::Section& Section)
		{
			return Section.Profile == lcCylinderConnection::SectionProfile::Axle || Section.Profile == lcCylinderConnection::SectionProfile::Square;
		});
	};
	const bool BothCylinders = std::holds_alternative<lcCylinderConnection>(Moving.Connection->Shape) && std::holds_alternative<lcCylinderConnection>(Target.Connection->Shape);
	const bool Keyed = BothCylinders && IsKeyed(*Moving.Connection) && IsKeyed(*Target.Connection);
	const bool UnevenScale = fabsf(Moving.Connection->Scale.x - Moving.Connection->Scale.y) > 0.01f ||
		fabsf(Target.Connection->Scale.x - Target.Connection->Scale.y) > 0.01f;

	std::optional<SnapCorrection> Best;
	float BestScore = std::numeric_limits<float>::infinity();
	for (const lcConnectionMating& Mating : Matings)
	{
		if (PickedBarPosition && fabsf(*PickedBarPosition - Mating.SecondOffset) > Mating.ClipBarHalfSpan + OccupiedTolerance)
			continue;

		const lcVector3 Axis = Mating.Direction * Target.Direction;
		if (AlreadySeated && !KeepOrientation && !AreStudDirectionsAligned(Moving.Direction, Axis))
			continue;

		lcMatrix33 Rotation = lcMatrix33Identity();
		if (!KeepOrientation && !AreStudDirectionsAligned(Moving.Direction, Axis))
			Rotation = GetStudAxisRotation(lcMatrix33(Moving.Transverse, lcCross(Moving.Direction, Moving.Transverse), Moving.Direction), Axis);

		if ((Generic && !KeepOrientation) || Keyed || (BothCylinders && UnevenScale))
		{
			const lcVector3 Transverse = lcMul(Moving.Transverse, Rotation);
			float Angle = atan2f(lcDot(Axis, lcCross(Transverse, Target.Transverse)), lcDot(Transverse, Target.Transverse));
			if (BothCylinders)
				Angle = remainderf(Angle, UnevenScale ? LC_PI : LC_PI * 0.5f);
			if (AlreadySeated && fabsf(Angle) > StudTurnTolerance)
				continue;

			if (!AlreadySeated)
				Rotation = lcMul(Rotation, lcMatrix33FromAxisAngle(Axis, Angle));
		}

		lcVector3 MovingAnchor = Moving.Position + Mating.FirstOffset * Moving.Direction;
		lcVector3 Seated = Target.Position + Mating.SecondOffset * Target.Direction;
		const bool SlidesOnMoving = Mating.Travel > OccupiedTolerance && Mating.TravelOnFirst;
		const bool SlidesOnTarget = Mating.Travel > OccupiedTolerance && !Mating.TravelOnFirst;
		if (SlidesOnMoving)
			MovingAnchor += lcClamp(lcDot(Seated - MovingAnchor, Moving.Direction), -Mating.Travel, Mating.Travel) * Moving.Direction;
		if (SlidesOnTarget)
			Seated += lcClamp(lcDot(MovingAnchor - Seated, Target.Direction), -Mating.Travel, Mating.Travel) * Target.Direction;

		const float Distance = lcLengthSquared(Seated - MovingAnchor);
		if (AlreadySeated && Distance > OccupiedTolerance * OccupiedTolerance)
			continue;

		// Prefer the pin end of a pin-and-axle part when both ends are equally close.
		const float FitPenalty = Mating.AxleInRoundHole ? 0.01f : 0.0f;
		const float Score = Distance + FitPenalty + (!KeepOrientation ? 1.0f - lcDot(Moving.Direction, Axis) : 0.0f) +
			lcLengthSquared(lcMul(Moving.Transverse, Rotation) - Moving.Transverse);
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = SnapCorrection{ lcMatrix44(Rotation, Seated - lcMul(MovingAnchor, Rotation)), MovingAnchor, Seated, SlidesOnMoving };
		}
	}
	return Best;
}

std::optional<lcStudMatingFace> lcConnectionInfo::FindStudMatingFace(const lcConnections& SurfaceConnections, const lcVector3& HitPosition, const lcVector3& RayDirection, float HitDistance, const lcBoundingBox& SurfaceBounds, bool& HasVerticalStuds)
{
	// The ray can enter the bounding box through a side before hitting the studded face.
	constexpr float StudHeight = 4.0f;
	constexpr float SurfaceTolerance = 0.1f;

	for (const lcConnection& Connection : SurfaceConnections)
	{
		const lcConnectionPoint Point(Connection, lcMatrix44Identity());
		if (Point.Type == lcStudType::None || !AreStudDirectionsAligned(Point.Direction, lcVector3(0, 0, 1)))
			continue;

		HasVerticalStuds = true;
		const bool IsTop = Point.Type == lcStudType::Male;
		const float Height = HitPosition.z - Point.Position.z;
		lcVector3 Opening = HitPosition;
		bool HitReceiverFace = false;

		if (!IsTop && RayDirection.z > LC_EPSILON && Height >= -SurfaceTolerance)
		{
			// The mesh hit can be inside the cavity behind the receiver opening.
			const float DistanceToOpening = Height / RayDirection.z;
			Opening = HitPosition - DistanceToOpening * RayDirection;
			HitReceiverFace = DistanceToOpening <= HitDistance &&
				Opening.x >= SurfaceBounds.Min.x - SurfaceTolerance && Opening.x <= SurfaceBounds.Max.x + SurfaceTolerance &&
				Opening.y >= SurfaceBounds.Min.y - SurfaceTolerance && Opening.y <= SurfaceBounds.Max.y + SurfaceTolerance;
		}

		if ((IsTop && RayDirection.z < 0.0f && Height >= -SurfaceTolerance && Height <= StudHeight + SurfaceTolerance) || HitReceiverFace)
			return lcStudMatingFace{ Opening, Point.Position.z, IsTop };
	}

	return std::nullopt;
}

lcMatrix44 lcConnectionInfo::GetStudMatingTransform(const lcStudMatingFace& Face, const lcMatrix44& SurfaceTransform, const lcMatrix33& SnapRotation, const lcVector3& GrabOffset, float PlacementOffset, const lcConnections& MovingConnections)
{
	const lcStudType MovingType = Face.IsTop ? lcStudType::Female : lcStudType::Male;
	const float MatingOffset = GetStudMatingOffset(MovingConnections, MovingType, SnapRotation, lcVector3(SurfaceTransform[2]), PlacementOffset);
	const lcVector3 LocalGrabOffset = lcMul30(GrabOffset, lcMatrix44AffineInverse(SurfaceTransform));

	// Project anchors at mating depth even when the ray hits the top of a raised stud.
	const lcVector3 Position(Face.Point.x + LocalGrabOffset.x, Face.Point.y + LocalGrabOffset.y, Face.PlaneZ + MatingOffset);
	return lcMatrix44(SnapRotation, lcMul31(Position, SurfaceTransform));
}

float lcConnectionInfo::GetStudMatingOffset(const lcConnections& Connections, lcStudType Type, const lcMatrix33& Rotation, const lcVector3& SurfaceAxis, float PlacementOffset)
{
	float MatingOffset = PlacementOffset;
	float BestOffsetDifference = std::numeric_limits<float>::infinity();

	for (const lcConnection& Connection : Connections)
	{
		const lcConnectionPoint Point(Connection, lcMatrix44Identity());
		if (Point.Type != Type || !AreStudDirectionsAligned(lcMul(Point.Direction, Rotation), SurfaceAxis))
			continue;

		const float Offset = -lcDot(lcMul(Point.Position, Rotation), SurfaceAxis);
		const float Difference = fabsf(Offset - PlacementOffset);

		if (Difference < BestOffsetDifference)
		{
			BestOffsetDifference = Difference;
			MatingOffset = Offset;
		}
	}

	return MatingOffset;
}

lcMatrix33 lcConnectionInfo::GetStudAxisRotation(const lcMatrix33& StartRotation, const lcVector3& TargetDirection)
{
	const lcVector3 SurfaceAxis = lcNormalize(TargetDirection);
	const lcVector3 MovingAxis = lcNormalize(StartRotation[2]);
	const lcVector3 Axis = lcCross(MovingAxis, SurfaceAxis);
	const float Sine = Axis.Length();
	const float Cosine = lcDot(MovingAxis, SurfaceAxis);

	if (Sine > LC_EPSILON)
		return lcMatrix33FromAxisAngle(Axis, atan2f(Sine, Cosine));
	if (Cosine < 0.0f)
		return lcMatrix33FromAxisAngle(StartRotation[0], LC_PI);

	return lcMatrix33Identity();
}

std::vector<lcConnectionDragState::Endpoint>::const_iterator lcConnectionInfo::FindEndpointByX(const std::vector<lcConnectionDragState::Endpoint>& Endpoints, float X)
{
	using Endpoint = lcConnectionDragState::Endpoint;
	return std::lower_bound(Endpoints.cbegin(), Endpoints.cend(), X, [](const Endpoint& Point, float Value)
	{
		return Point.Connection.Position.x < Value;
	});
}

std::pair<float, float> lcConnectionInfo::GetSeatedSpan(const lcConnectionPoint& Host, const lcConnectionPoint& Mate)
{
	float Length = Mate.Reach;
	bool Centered;
	if (const lcClipConnection* Clip = std::get_if<lcClipConnection>(&Mate.Connection->Shape))
	{
		Length = Clip->Length * Mate.Connection->Scale.z;
		Centered = Clip->Centered;
	}
	else
	{
		Centered = std::get<lcCylinderConnection>(Mate.Connection->Shape).Centered;
		if (Centered)
			Length *= 2.0f;
	}

	const float AxisLength = Length * lcDot(Mate.Direction, Host.Direction);
	const float Origin = lcDot(Mate.Position - Host.Position, Host.Direction);
	const float Start = Centered ? Origin - AxisLength * 0.5f : Origin;
	const float End = Start + AxisLength;

	return std::pair<float, float>(lcMin(Start, End), lcMax(Start, End));
}

bool lcConnectionDragState::Occupancy::Overlaps(const std::pair<float, float>& Span) const
{
	return std::any_of(SlidingSpans.begin(), SlidingSpans.end(), [&Span](const std::pair<float, float>& Other)
	{
		return lcMin(Span.second, Other.second) - lcMax(Span.first, Other.first) > OccupiedTolerance;
	});
}

const lcConnectionDragState::Occupancy& lcConnectionInfo::GetOccupancy(lcConnectionDragState::Endpoint& Point, const std::vector<lcConnectionDragState::Endpoint>& Endpoints, bool Sorted, float Reach)
{
	if (!Point.Occupied)
	{
		const float X = Point.Connection.Position.x;
		const float Range = Point.Connection.Reach + Reach + OccupiedTolerance;
		lcConnectionDragState::Occupancy& Occupied = Point.Occupied.emplace();

		for (auto Other = Sorted ? FindEndpointByX(Endpoints, X - Range) : Endpoints.cbegin(); Other != Endpoints.cend(); ++Other)
		{
			if (Sorted && Other->Connection.Position.x > X + Range)
				break;

			const std::optional<SnapCorrection> Correction = Point.Piece != Other->Piece
				? GetSnapCorrection(Point.Connection, Other->Connection, true, std::nullopt) : std::nullopt;
			if (!Correction)
				continue;

			if (!Correction->SlidesOnMoving)
			{
				Occupied.Exclusive = true;
				break;
			}

			Occupied.SlidingSpans.push_back(GetSeatedSpan(Point.Connection, Other->Connection));
		}
	}

	return *Point.Occupied;
}

std::optional<size_t> lcConnectionInfo::GetSnapContactCount(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const lcMatrix44& MovingToSnapped, float StationaryReach, bool CountContacts)
{
	using Endpoint = lcConnectionDragState::Endpoint;
	std::unordered_set<size_t> MatchedTargets;

	for (Endpoint& Moving : MovingEndpoints)
	{
		lcConnectionPoint Proposed = Moving.Connection;
		Proposed.Transform(MovingToSnapped);

		const float X = Proposed.Position.x;
		const float Range = Proposed.Reach + StationaryReach + OccupiedTolerance;
		bool Matched = false;

		for (auto Target = FindEndpointByX(StationaryEndpoints, X - Range);
			Target != StationaryEndpoints.cend() && Target->Connection.Position.x <= X + Range; ++Target)
		{
			const std::optional<SnapCorrection> Correction = GetSnapCorrection(Proposed, Target->Connection, true, std::nullopt);
			if (!Correction)
				continue;

			const size_t TargetIndex = static_cast<size_t>(Target - StationaryEndpoints.cbegin());
			Endpoint& Stationary = StationaryEndpoints[TargetIndex];

			const lcConnectionDragState::Occupancy& MovingOccupancy = GetOccupancy(Moving, MovingEndpoints, false, 0.0f);
			const lcConnectionDragState::Occupancy& StationaryOccupancy = GetOccupancy(Stationary, StationaryEndpoints, true, StationaryReach);
			if (MovingOccupancy.Exclusive || StationaryOccupancy.Exclusive ||
				(!MovingOccupancy.SlidingSpans.empty() && MovingOccupancy.Overlaps(GetSeatedSpan(Proposed, Stationary.Connection))) ||
				(!StationaryOccupancy.SlidingSpans.empty() && StationaryOccupancy.Overlaps(GetSeatedSpan(Stationary.Connection, Proposed))))
				return std::nullopt;

			if (CountContacts && Proposed.Type != lcStudType::None && Target->Connection.Type != lcStudType::None && !Matched)
				Matched = MatchedTargets.insert(TargetIndex).second;
		}
	}

	return MatchedTargets.size();
}

bool lcConnectionInfo::IsSnapSeated(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, lcConnectionDragState& DragState)
{
	if (!DragState.Pair || DragState.Pair->first >= MovingEndpoints.size() || DragState.Pair->second >= DragState.Stationary.size())
		return false;

	const auto [Moving, Target] = *DragState.Pair;
	return GetSnapCorrection(MovingEndpoints[Moving].Connection, DragState.Stationary[Target].Connection, true, std::nullopt).has_value() &&
		GetSnapContactCount(MovingEndpoints, DragState.Stationary, lcMatrix44Identity(), DragState.StationaryReach, false).has_value();
}

lcMatrix44 lcConnectionInfo::GetStudTurnCorrection(const lcVector3& MovingAnchor, const lcConnectionPoint& Target, float Angle)
{
	const lcMatrix33 Rotation = lcMatrix33FromAxisAngle(Target.Direction, Angle);
	return lcMatrix44(Rotation, Target.Position - lcMul(MovingAnchor, Rotation));
}

bool lcConnectionInfo::IsStudTurnTarget(const lcConnectionDragState::Endpoint& Target, const lcConnectionSnapContext& Context)
{
	if (!Context.Turn || Target.Piece != Context.SurfacePiece)
		return false;

	const lcStudTurnContext& Turn = *Context.Turn;
	return Target.Connection.Type == Turn.TargetType && AreStudDirectionsAligned(Target.Connection.Direction, Turn.FaceDirection) &&
		fabsf(lcDot(Target.Connection.Position - Turn.FacePoint, Turn.FaceDirection)) <= OccupiedTolerance;
}

std::vector<float> lcConnectionInfo::GetStudTurnAngles(const std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const StudSnapAnchor& Anchor, const lcMatrix44& MovingToProposed)
{
	const lcConnectionDragState::Endpoint& MovingAnchor = MovingEndpoints[Anchor.Pair.first];
	const lcConnectionDragState::Endpoint& TargetAnchor = StationaryEndpoints[Anchor.Pair.second];
	const lcVector3 Axis = lcNormalize(TargetAnchor.Connection.Direction);
	std::vector<float> Angles;

	for (const lcConnectionDragState::Endpoint& Moving : MovingEndpoints)
	{
		if (Moving.Piece != MovingAnchor.Piece || Moving.Connection.Type != MovingAnchor.Connection.Type)
			continue;

		const lcVector3 MovingOffset = lcMul30(Moving.Connection.Position - MovingAnchor.Connection.Position, MovingToProposed);
		const float Distance = MovingOffset.Length();
		if (Distance <= OccupiedTolerance || Distance > StudTurnNeighborDistance + OccupiedTolerance || fabsf(lcDot(MovingOffset, Axis)) > OccupiedTolerance)
			continue;

		if (!AreStudDirectionsAligned(lcMul30(Moving.Connection.Direction, MovingToProposed), Axis))
			continue;

		const float X = TargetAnchor.Connection.Position.x;
		const float Range = Distance + OccupiedTolerance;
		for (auto Target = FindEndpointByX(StationaryEndpoints, X - Range); Target != StationaryEndpoints.cend() && Target->Connection.Position.x <= X + Range; ++Target)
		{
			if (Target->Piece != TargetAnchor.Piece || Target->Connection.Type != TargetAnchor.Connection.Type)
				continue;

			const lcVector3 TargetOffset = Target->Connection.Position - TargetAnchor.Connection.Position;
			if (fabsf(lcDot(TargetOffset, Axis)) > OccupiedTolerance || fabsf(TargetOffset.Length() - Distance) > OccupiedTolerance ||
				!AreStudDirectionsAligned(Target->Connection.Direction, Axis))
				continue;

			const float Angle = atan2f(lcDot(Axis, lcCross(MovingOffset, TargetOffset)), lcDot(MovingOffset, TargetOffset));
			if (fabsf(Angle) <= StudTurnTolerance || fabsf(Angle) > MaxStudTurn + StudTurnTolerance)
				continue;

			if (std::none_of(Angles.begin(), Angles.end(), [Angle](float Other) { return fabsf(Angle - Other) < StudTurnTolerance; }))
				Angles.push_back(Angle);
		}
	}

	return Angles;
}

std::optional<lcConnectionInfo::StudSnapCandidate> lcConnectionInfo::FindStudSnapWithTurn(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const std::vector<StudSnapAnchor>& Anchors, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, const lcVector2& GrabScreen, float StationaryReach, const std::optional<lcMatrix44>& UnturnedCorrection)
{
	std::optional<StudSnapCandidate> BestTurn;
	size_t BestCount = 1;
	constexpr float GrabLimitSquared = StudTurnGrabScreenLimit * StudTurnGrabScreenLimit;

	auto CountContacts = [&](const lcMatrix44& Correction)
	{
		return GetSnapContactCount(MovingEndpoints, StationaryEndpoints, lcMul(MovingToProposed, Correction), StationaryReach, true).value_or(0);
	};

	for (size_t Index = 0; Index < Anchors.size(); Index++)
	{
		const StudSnapAnchor& Anchor = Anchors[Index];
		const lcConnectionPoint& Target = StationaryEndpoints[Anchor.Pair.second].Connection;
		for (float Angle : GetStudTurnAngles(MovingEndpoints, StationaryEndpoints, Anchor, MovingToProposed))
		{
			const lcMatrix44 Correction = GetStudTurnCorrection(Anchor.ProposedPosition, Target, Angle);
			lcVector2 SnappedGrabScreen;
			const std::optional<float> GrabDistance = GetStudGrabScreenDistanceSquared(Correction, Context, GrabScreen, SnappedGrabScreen);
			if (!GrabDistance || *GrabDistance > GrabLimitSquared)
				continue;

			const size_t Count = CountContacts(Correction);

			if (Count < 2 || Count < BestCount)
				continue;

			if (BestTurn && Count == BestCount && (*GrabDistance > BestTurn->GrabDistanceSquared ||
				(*GrabDistance == BestTurn->GrabDistanceSquared && fabsf(Angle) >= fabsf(BestTurn->Angle))))
				continue;

			BestCount = Count;
			BestTurn = StudSnapCandidate{ Index, Correction, Angle, SnappedGrabScreen, *GrabDistance };
		}
	}

	if (!BestTurn || (UnturnedCorrection && CountContacts(*UnturnedCorrection) >= BestCount))
		return std::nullopt;

	// On equal coverage, keep the original rotation.
	for (size_t Index = 0; Index < Anchors.size(); Index++)
	{
		const StudSnapAnchor& Anchor = Anchors[Index];
		const lcVector3 Position = StationaryEndpoints[Anchor.Pair.second].Connection.Position;
		const lcMatrix44 Correction = lcMatrix44Translation(Position - Anchor.ProposedPosition);
		lcVector2 SnappedGrabScreen;
		const std::optional<float> GrabDistance = GetStudGrabScreenDistanceSquared(Correction, Context, GrabScreen, SnappedGrabScreen);
		if (!GrabDistance || *GrabDistance > GrabLimitSquared)
			continue;

		const size_t Count = CountContacts(Correction);
		if (Count > BestCount || (Count == BestCount && BestTurn->Angle != 0.0f))
		{
			BestCount = Count;
			BestTurn = StudSnapCandidate{ Index, Correction, 0.0f, SnappedGrabScreen, *GrabDistance };
		}
	}

	return BestTurn;
}

std::optional<lcVector2> lcConnectionInfo::ProjectEndpoint(const lcConnectionSnapContext& Context, const lcVector3& Position)
{
	const lcVector4 Clip = lcMul4(lcVector4(Position, 1.0f), Context.ModelToClip);

	if (!(Clip.w > LC_EPSILON && fabsf(Clip.z) <= Clip.w))
		return std::nullopt;

	return lcVector2((Clip.x / Clip.w + 1.0f) * Context.LogicalViewportSize.x * 0.5f, (Clip.y / Clip.w + 1.0f) * Context.LogicalViewportSize.y * 0.5f);
}

std::optional<float> lcConnectionInfo::GetStudGrabScreenDistanceSquared(const lcMatrix44& Correction, const lcConnectionSnapContext& Context, const lcVector2& GrabScreen, lcVector2& SnappedGrabScreen)
{
	const lcVector3& GrabPoint = Context.Turn->GrabPoint;
	const lcVector3 SnappedGrab = lcMul31(GrabPoint, Correction);
	if (lcLengthSquared(SnappedGrab - GrabPoint) > WorldLimit * WorldLimit)
		return std::nullopt;

	const std::optional<lcVector2> SnappedScreen = ProjectEndpoint(Context, SnappedGrab);
	if (!SnappedScreen)
		return std::nullopt;

	SnappedGrabScreen = *SnappedScreen;
	const float X = SnappedGrabScreen.x - GrabScreen.x;
	const float Y = SnappedGrabScreen.y - GrabScreen.y;
	return X * X + Y * Y;
}

std::optional<float> lcConnectionInfo::GetPairScreenDistanceSquared(const lcConnectionPoint& Proposed, const lcConnectionDragState::Endpoint& Target, const SnapCorrection& Correction, const lcConnectionSnapContext& Context, bool Retained, lcVector2& ScreenCorrection)
{
	const lcVector3 Difference = Correction.SeatedAnchor - Correction.ProposedAnchor;
	const float WorldDistanceSquared = lcLengthSquared(Difference);
	const bool OnMatingPlane = fabsf(lcDot(Difference, Target.Connection.Direction)) <= OccupiedTolerance;
	// A ray hit establishes which surface is under the cursor. On that same
	// mating plane, cover the gaps between studs even when zoomed in.
	const bool StudPair = Proposed.Type != lcStudType::None && Target.Connection.Type != lcStudType::None;
	const bool HoveredPiece = Context.HasSurfaceHit && Target.Piece == Context.SurfacePiece;
	const bool OnHoveredFace = StudPair && HoveredPiece && OnMatingPlane &&
		WorldDistanceSquared <= 2.0f * HalfStudPitch * HalfStudPitch + OccupiedTolerance;

	// The ray disambiguates depth for the hovered piece. A studded face has
	// its own depth check so the tilt and release behavior stays consistent.
	const bool DepthValid = Retained && !Context.HasSurfaceHit ? OnMatingPlane :
		OnHoveredFace || (HoveredPiece && !StudPair) || fabsf(lcDot(Difference, Context.ViewDirection)) <= DepthLimit;

	if (WorldDistanceSquared > WorldLimit * WorldLimit || !DepthValid)
		return std::nullopt;

	const std::optional<lcVector2> TargetScreen = ProjectEndpoint(Context, Correction.SeatedAnchor);
	const std::optional<lcVector2> MovingScreen = ProjectEndpoint(Context, Correction.ProposedAnchor);

	if (!TargetScreen || !MovingScreen)
		return std::nullopt;

	const float X = TargetScreen->x - MovingScreen->x;
	const float Y = TargetScreen->y - MovingScreen->y;
	const float DistanceSquared = X * X + Y * Y;

	if (!Retained && !OnHoveredFace && DistanceSquared >= ScreenLimit * ScreenLimit)
		return std::nullopt;

	ScreenCorrection = lcVector2(X, Y);
	return DistanceSquared;
}

const lcCylinderConnection* lcConnectionInfo::GetBoreInsert(const lcConnection& Connection)
{
	const lcCylinderConnection* Cylinder = std::get_if<lcCylinderConnection>(&Connection.Shape);
	if (Cylinder && Cylinder->Gender == lcConnectionGender::Male && Cylinder->Caps != lcCylinderConnection::CylinderCaps::Both &&
		!Cylinder->Sections.empty() && Cylinder->StudType != lcStudType::Male)
		return Cylinder;

	return nullptr;
}

const lcCylinderConnection* lcConnectionInfo::GetClipBar(const lcConnection& Connection)
{
	const lcCylinderConnection* Cylinder = std::get_if<lcCylinderConnection>(&Connection.Shape);
	if (Cylinder && Cylinder->Gender == lcConnectionGender::Male && std::any_of(Cylinder->Sections.begin(), Cylinder->Sections.end(), [](const lcCylinderConnection::Section& Section)
	{
		return Section.Profile == lcCylinderConnection::SectionProfile::Round;
	}))
		return Cylinder;

	return nullptr;
}

bool lcConnectionInfo::CanSnapToRay(const lcConnection& Connection)
{
	return std::holds_alternative<lcClipConnection>(Connection.Shape) || GetBoreInsert(Connection) || GetClipBar(Connection);
}

const lcCylinderConnection* lcConnectionInfo::GetThroughBore(const lcConnection& Connection)
{
	const lcCylinderConnection* Cylinder = std::get_if<lcCylinderConnection>(&Connection.Shape);
	if (Cylinder && Cylinder->Gender == lcConnectionGender::Female && Cylinder->Caps == lcCylinderConnection::CylinderCaps::None && !Cylinder->Sections.empty())
		return Cylinder;

	return nullptr;
}

bool lcConnectionInfo::IsClipMateVisible(const lcConnectionDragState::Endpoint& Target, const lcVector3& Seat, const lcConnectionSnapRay& Ray)
{
	const float RayLength = lcLength(Ray.End - Ray.Start);
	if (RayLength <= LC_EPSILON)
		return false;

	const lcConnectionPoint& Point = Target.Connection;
	const lcClipPickingRay Pick(Point, Ray, RayLength);
	const float Scale = Point.Connection->Scale.z;
	if (const lcClipConnection* Clip = std::get_if<lcClipConnection>(&Point.Connection->Shape))
	{
		const float Length = Clip->Length * Scale;
		const std::optional<float> Depth = Pick.Intersect(Clip->Centered ? -Length * 0.5f : 0.0f, Length, Clip->Radius);
		return Depth && *Depth <= Pick.VisibleDepth;
	}

	const lcCylinderConnection* Bar = GetClipBar(*Point.Connection);
	if (!Bar)
		return false;

	const float AxialSeat = lcDot(Seat - Point.Position, Point.Direction);
	float Start = Bar->Centered ? -Point.Reach : 0.0f;
	for (const lcCylinderConnection::Section& Section : Bar->Sections)
	{
		const float Length = Section.Length * Scale;
		if (Section.Profile == lcCylinderConnection::SectionProfile::Round && AxialSeat >= Start - OccupiedTolerance && AxialSeat <= Start + Length + OccupiedTolerance)
		{
			const std::optional<float> Depth = Pick.Intersect(Start, Length, Section.Radius);
			if (Depth && *Depth <= Pick.VisibleDepth)
				return true;
		}
		Start += Length;
	}

	return false;
}

bool lcConnectionInfo::RaySnapCandidate::IsAfter(bool OtherDirectHit, float OtherDepth) const
{
	if (DirectHit != OtherDirectHit)
		return OtherDirectHit;

	return Depth > OtherDepth;
}

bool lcConnectionInfo::RaySnapCandidate::NeedsMoreCorrection(float OtherDistanceSquared, const lcMatrix44& OtherTransform) const
{
	constexpr float TranslationPrecision = 0.001f;
	const float Translation = roundf(sqrtf(CorrectionDistanceSquared) / TranslationPrecision);
	const float OtherTranslation = roundf(sqrtf(OtherDistanceSquared) / TranslationPrecision);
	if (Translation != OtherTranslation)
		return Translation > OtherTranslation;

	// The trace of a rotation matrix decreases with its angle from identity.
	const float Trace = Transform[0].x + Transform[1].y + Transform[2].z;
	const float OtherTrace = OtherTransform[0].x + OtherTransform[1].y + OtherTransform[2].z;
	if (Trace != OtherTrace)
		return Trace < OtherTrace;

	return CorrectionDistanceSquared > OtherDistanceSquared;
}

std::optional<lcConnectionInfo::RaySnapCandidate> lcConnectionInfo::FindBoreSnap(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, lcConnectionDragState& DragState)
{
	if (!Context.PlacementRay)
		return std::nullopt;

	struct InsertCandidate
	{
		size_t Index;
		lcConnectionPoint Proposed;
		bool UniformShaft;
	};

	std::vector<InsertCandidate> Inserts;
	for (size_t Index = 0; Index < MovingEndpoints.size(); Index++)
	{
		lcConnectionPoint Proposed = MovingEndpoints[Index].Connection;
		const lcCylinderConnection* Insert = GetBoreInsert(*Proposed.Connection);
		if (!Insert)
			continue;

		Proposed.Transform(MovingToProposed);
		const bool UniformShaft = Insert->Sections.size() == 1 && Insert->Caps == lcCylinderConnection::CylinderCaps::None;
		Inserts.push_back({ Index, Proposed, UniformShaft });
	}

	if (Inserts.empty())
		return std::nullopt;

	const lcConnectionSnapRay& Ray = *Context.PlacementRay;
	const float RayLength = lcLength(Ray.End - Ray.Start);
	if (RayLength <= LC_EPSILON)
		return std::nullopt;

	const lcVector3 RayDirection = (Ray.End - Ray.Start) / RayLength;
	const float VisibleDepth = lcMin(RayLength, Ray.HitDistance + OccupiedTolerance);
	float BestDistance = std::numeric_limits<float>::infinity();
	std::optional<RaySnapCandidate> Best;

	// Test the visible opening once per stationary bore before checking fit.
	for (size_t TargetIndex = 0; TargetIndex < DragState.Stationary.size(); TargetIndex++)
	{
		lcConnectionDragState::Endpoint& Target = DragState.Stationary[TargetIndex];
		const lcConnectionPoint& Bore = Target.Connection;
		const lcCylinderConnection* Cylinder = GetThroughBore(*Bore.Connection);
		if (!Cylinder)
			continue;

		const float Approach = lcDot(Bore.Direction, RayDirection);
		if (fabsf(Approach) <= LC_EPSILON)
			continue;

		const float Length = Bore.Reach * (Cylinder->Centered ? 2.0f : 1.0f);
		const lcVector3 Start = Cylinder->Centered ? Bore.Position - Bore.Reach * Bore.Direction : Bore.Position;
		const bool FromStart = Approach > 0.0f;
		const lcVector3 Mouth = FromStart ? Start : Start + Length * Bore.Direction;
		const float Depth = lcDot(Mouth - Ray.Start, Bore.Direction) / Approach;
		if (Depth < 0.0f || Depth > VisibleDepth)
			continue;

		const std::optional<lcVector2> MouthScreen = ProjectEndpoint(Context, Mouth);
		if (!MouthScreen)
			continue;

		const float X = Context.Cursor.x - MouthScreen->x;
		const float Y = Context.Cursor.y - MouthScreen->y;
		const float Distance = X * X + Y * Y;
		// Allow screen-space tolerance for rays in empty space or on the rim plane.
		const float MouthRadius = FromStart ? Cylinder->Sections.front().Radius : Cylinder->Sections.back().Radius;
		const lcVector3 RayOffset = Mouth - (Ray.Start + Depth * RayDirection);
		const lcVector3 Across = lcCross(Bore.Direction, Bore.Transverse);
		const float RadialX = lcDot(RayOffset, Bore.Transverse) / Bore.Connection->Scale.x;
		const float RadialY = lcDot(RayOffset, Across) / Bore.Connection->Scale.y;
		const bool DirectHit = RadialX * RadialX + RadialY * RadialY <= MouthRadius * MouthRadius;
		const bool HitOffRim = Ray.HitPiece && (Target.Piece != Ray.HitPiece || fabsf(Depth - Ray.HitDistance) > OccupiedTolerance);
		if (!DirectHit && (Distance > ScreenLimit * ScreenLimit || HitOffRim))
			continue;

		if (Best && !Best->IsAfter(DirectHit, Depth) &&
			(Best->DirectHit != DirectHit || Best->Depth != Depth || Distance >= BestDistance))
			continue;

		for (const InsertCandidate& Insert : Inserts)
		{
			const lcConnectionPoint& Proposed = Insert.Proposed;
			const lcVector3 AimPoint = Insert.UniformShaft ? Start + 0.5f * Length * Bore.Direction : Mouth;
			const float AimDepth = lcDot(AimPoint - Ray.Start, RayDirection);
			const lcVector3 BaseShift = Ray.Start + AimDepth * RayDirection - Ray.ProposedGrab;
			lcVector3 Shift = BaseShift;
			if (!Insert.UniformShaft)
				Shift -= lcDot(Proposed.Position + Shift - AimPoint, Bore.Direction) * Bore.Direction;

			const lcMatrix44 ToBore = lcMatrix44Translation(Shift);
			lcConnectionPoint NearBore = Proposed;
			NearBore.Transform(ToBore);

			const std::optional<SnapCorrection> Correction = GetSnapCorrection(NearBore, Bore, false, std::nullopt);
			if (!Correction || lcLengthSquared(Correction->SeatedAnchor - Correction->ProposedAnchor) > WorldLimit * WorldLimit)
				continue;

			const float CorrectionDistanceSquared = lcLengthSquared(Shift - BaseShift + Correction->SeatedAnchor - Correction->ProposedAnchor);
			if ((Best && Best->Pair.second == TargetIndex && !Best->NeedsMoreCorrection(CorrectionDistanceSquared, Correction->Transform)) ||
				GetOccupancy(Target, DragState.Stationary, true, DragState.StationaryReach).Exclusive ||
				GetOccupancy(MovingEndpoints[Insert.Index], MovingEndpoints, false, 0.0f).Exclusive)
				continue;

			const lcMatrix44 Transform = lcMul(ToBore, Correction->Transform);
			if (!GetSnapContactCount(MovingEndpoints, DragState.Stationary, lcMul(MovingToProposed, Transform), DragState.StationaryReach, false))
				continue;

			BestDistance = Distance;
			Best = RaySnapCandidate{ Transform, std::make_pair(Insert.Index, TargetIndex), Depth, DirectHit, CorrectionDistanceSquared };
		}
	}

	return Best;
}

std::optional<lcConnectionInfo::RaySnapCandidate> lcConnectionInfo::FindClipSnap(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, lcConnectionDragState& DragState)
{
	if (!Context.PlacementRay)
		return std::nullopt;

	using Endpoint = lcConnectionDragState::Endpoint;
	std::vector<size_t> MovingClips, MovingBars;
	for (size_t Index = 0; Index < MovingEndpoints.size(); Index++)
	{
		const lcConnection& Connection = *MovingEndpoints[Index].Connection.Connection;
		if (std::holds_alternative<lcClipConnection>(Connection.Shape))
			MovingClips.push_back(Index);
		else if (GetClipBar(Connection))
			MovingBars.push_back(Index);
	}

	if (MovingClips.empty() && MovingBars.empty())
		return std::nullopt;

	const lcConnectionSnapRay& Ray = *Context.PlacementRay;
	const float RayLength = lcLength(Ray.End - Ray.Start);
	if (RayLength <= LC_EPSILON)
		return std::nullopt;

	const lcVector3 RayDirection = (Ray.End - Ray.Start) / RayLength;
	const std::optional<std::pair<size_t, size_t>> Retained = DragState.IsClipSnap ? DragState.Pair : std::nullopt;
	std::optional<RaySnapCandidate> Best;

	for (size_t TargetIndex = 0; TargetIndex < DragState.Stationary.size(); TargetIndex++)
	{
		Endpoint& Target = DragState.Stationary[TargetIndex];
		const lcConnectionPoint& Point = Target.Connection;
		const lcConnection& Connection = *Point.Connection;
		const lcClipConnection* Clip = MovingBars.empty() ? nullptr : std::get_if<lcClipConnection>(&Connection.Shape);
		const lcCylinderConnection* Bar = MovingClips.empty() ? nullptr : GetClipBar(Connection);
		if (!Clip && !Bar)
			continue;

		const lcClipPickingRay Pick(Point, Ray, RayLength);

		// Pick each stationary volume once, then try the moving connectors that can mate with it.
		auto ConsiderVolume = [&](float Start, float Length, float Radius, const std::vector<size_t>& MovingIndices)
		{
			const std::optional<float> Intersection = Pick.Intersect(Start, Length, Radius);
			const bool DirectHit = Intersection.has_value();
			float Depth = Intersection.value_or(std::numeric_limits<float>::infinity());

			// The visible lip can extend beyond the authored opening.
			if (!DirectHit && Clip && Ray.HitPiece == Target.Piece && Ray.HitDistance <= RayLength)
			{
				constexpr float RimReach = 2.5f;
				const lcVector3 Hit = Pick.Start + Ray.HitDistance * Pick.Direction - lcVector3(0, 0, Start);
				const float RadialDistanceSquared = Hit.x * Hit.x + Hit.y * Hit.y;
				if (Hit.z >= -OccupiedTolerance && Hit.z <= Length + OccupiedTolerance &&
					RadialDistanceSquared >= Radius * Radius && RadialDistanceSquared <= (Radius + RimReach) * (Radius + RimReach))
					Depth = Ray.HitDistance;
			}

			const bool CanRetain = Retained && Retained->second == TargetIndex;
			auto IsSameHit = [&]()
			{
				return Best && Best->Pair.second == TargetIndex && Best->Depth == Depth && Best->DirectHit == DirectHit;
			};
			if (Depth > Pick.VisibleDepth || (Best && !Best->IsAfter(DirectHit, Depth) && !IsSameHit() && (!CanRetain || Retained == Best->Pair)))
				return;

			const float AxialPosition = Bar ? lcClamp(Pick.Start.z + Depth * Pick.Direction.z, Start, Start + Length) : Start + Length * 0.5f;
			const lcVector3 AimPoint = Point.Position + AxialPosition * Point.Direction;
			const float AimDepth = lcDot(AimPoint - Ray.Start, RayDirection);
			const lcVector3 Shift = Ray.Start + AimDepth * RayDirection - Ray.ProposedGrab;
			const std::optional<float> PickedBarPosition = Bar ? std::optional<float>(AxialPosition) : std::nullopt;

			for (size_t MovingIndex : MovingIndices)
			{
				const std::pair<size_t, size_t> Pair(MovingIndex, TargetIndex);
				const bool BestRetained = Best && Retained == Best->Pair;
				const bool ThisRetained = Retained == Pair;
				if (Best)
				{
					if (BestRetained != ThisRetained ? BestRetained : (!Best->IsAfter(DirectHit, Depth) && !IsSameHit()))
						continue;
				}

				Endpoint& Moving = MovingEndpoints[MovingIndex];
				lcConnectionPoint Proposed = Moving.Connection;
				Proposed.Transform(MovingToProposed);

				lcVector3 Offset = Shift;
				if (Bar && !Moving.Piece)
				{
					const lcClipConnection& MovingClip = std::get<lcClipConnection>(Proposed.Connection->Shape);
					const float CenterOffset = MovingClip.Centered ? 0.0f : MovingClip.Length * Proposed.Connection->Scale.z * 0.5f;
					const lcVector3 Center = Proposed.Position + CenterOffset * Proposed.Direction;
					Offset += lcDot(AimPoint - Center - Offset, Point.Direction) * Point.Direction;
				}
				const lcMatrix44 ToTarget = lcMatrix44Translation(Offset);
				Proposed.Transform(ToTarget);

				const std::optional<SnapCorrection> Correction = GetSnapCorrection(Proposed, Point, false, PickedBarPosition);
				if (!Correction || lcLengthSquared(Correction->SeatedAnchor - Correction->ProposedAnchor) > WorldLimit * WorldLimit)
					continue;

				const float CorrectionDistanceSquared = lcLengthSquared(Offset - Shift + Correction->SeatedAnchor - Correction->ProposedAnchor);
				if (IsSameHit() && BestRetained == ThisRetained && !Best->NeedsMoreCorrection(CorrectionDistanceSquared, Correction->Transform))
					continue;

				const lcMatrix44 Transform = lcMul(ToTarget, Correction->Transform);
				if (!GetSnapContactCount(MovingEndpoints, DragState.Stationary, lcMul(MovingToProposed, Transform), DragState.StationaryReach, false))
					continue;

				Best = RaySnapCandidate{ Transform, Pair, Depth, DirectHit, CorrectionDistanceSquared };
			}
		};

		if (Clip)
		{
			const float Length = Clip->Length * Connection.Scale.z;
			ConsiderVolume(Clip->Centered ? -Length * 0.5f : 0.0f, Length, Clip->Radius, MovingBars);
		}
		else
		{
			float Start = Bar->Centered ? -Point.Reach : 0.0f;
			for (const lcCylinderConnection::Section& Section : Bar->Sections)
			{
				const float Length = Section.Length * Connection.Scale.z;
				if (Section.Profile == lcCylinderConnection::SectionProfile::Round)
					ConsiderVolume(Start, Length, Section.Radius, MovingClips);

				Start += Length;
			}
		}

		if (Best && Retained == Best->Pair)
			return Best;
	}

	return Best;
}

std::optional<lcMatrix44> lcConnectionInfo::FindSnap(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, lcConnectionDragState& DragState)
{
	const bool RayOnly = Context.PlacementRay && Context.PlacementRay->RayOnly;
	const bool RetainedClip = DragState.IsClipSnap && DragState.Pair.has_value();
	const bool RayFirst = Context.PlacementRay && (RayOnly || RetainedClip || (!Context.HasSurfaceHit && !DragState.Pair));
	auto FindRaySnap = [&]() -> std::optional<lcMatrix44>
	{
		const std::optional<RaySnapCandidate> Clip = FindClipSnap(MovingEndpoints, MovingToProposed, Context, DragState);
		std::optional<RaySnapCandidate> Bore;
		const bool KeepClip = Clip && RetainedClip && Clip->Pair == DragState.Pair;
		if (!KeepClip)
			Bore = FindBoreSnap(MovingEndpoints, MovingToProposed, Context, DragState);

		const bool PickClip = Clip && (!Bore || Bore->IsAfter(Clip->DirectHit, Clip->Depth));
		const std::optional<RaySnapCandidate>& Best = PickClip ? Clip : Bore;
		if (!Best)
			return std::nullopt;

		DragState.Pair = Best->Pair;
		DragState.ReleaseRadius = lcConnectionDragState::DefaultReleaseRadius;
		DragState.ScreenCorrection = lcVector2(0.0f, 0.0f);
		DragState.TurnAngle = 0.0f;
		DragState.IsClipSnap = PickClip;
		return Best->Transform;
	};
	if (RayFirst)
	{
		const std::optional<lcMatrix44> Correction = FindRaySnap();
		if (Correction || RayOnly)
			return Correction;
	}

	using Endpoint = lcConnectionDragState::Endpoint;
	const std::optional<lcVector2> GrabScreen = Context.Turn && Context.SurfacePiece ? ProjectEndpoint(Context, Context.Turn->GrabPoint) : std::nullopt;
	std::vector<StudSnapAnchor> TurnAnchors;
	if (GrabScreen)
		TurnAnchors.reserve(MaxStudTurnAnchors + 1);

	float BestDistanceSquared = std::numeric_limits<float>::infinity();
	std::optional<lcMatrix44> BestCorrection;
	std::optional<std::pair<size_t, size_t>> BestPair;
	float BestAngle = 0.0f;
	float SearchRadiusSquared = std::numeric_limits<float>::infinity();

	auto ConsiderPair = [&](size_t MovingIndex, size_t TargetIndex, const lcConnectionPoint& Proposed, float RadiusSquared, bool Retained)
	{
		Endpoint& Moving = MovingEndpoints[MovingIndex];
		Endpoint& Target = DragState.Stationary[TargetIndex];

		// A new pin or axle snap needs a visible opening when placement has a picking ray.
		if (!Retained && Context.PlacementRay && GetBoreInsert(*Proposed.Connection) && GetThroughBore(*Target.Connection.Connection))
			return;

		const lcConnection& First = *Proposed.Connection;
		const lcConnection& Second = *Target.Connection.Connection;
		const bool CheckClipVisibility = Context.PlacementRay &&
			((std::holds_alternative<lcClipConnection>(First.Shape) && GetClipBar(Second)) ||
			 (std::holds_alternative<lcClipConnection>(Second.Shape) && GetClipBar(First)));
		if (CheckClipVisibility && (!Moving.Piece || !Context.RayAtPoint))
			return;

		const std::optional<SnapCorrection> Correction = GetSnapCorrection(Proposed, Target.Connection, false, std::nullopt);
		if (!Correction)
			return;

		lcVector2 ScreenCorrection;
		const std::optional<float> DistanceSquared = GetPairScreenDistanceSquared(Proposed, Target, *Correction, Context, Retained, ScreenCorrection);
		if (!DistanceSquared)
			return;

		const bool TryTurn = Proposed.Type != lcStudType::None && !Retained && GrabScreen &&
			*DistanceSquared < SearchRadiusSquared && IsStudTurnTarget(Target, Context) &&
			(TurnAnchors.size() < MaxStudTurnAnchors || *DistanceSquared < TurnAnchors.back().DistanceSquared);
		if (!TryTurn && !(*DistanceSquared < RadiusSquared))
			return;

		if (CheckClipVisibility && !IsClipMateVisible(Target, Correction->SeatedAnchor, Context.RayAtPoint(Correction->SeatedAnchor)))
			return;

		if (GetOccupancy(Moving, MovingEndpoints, false, 0.0f).Exclusive || GetOccupancy(Target, DragState.Stationary, true, DragState.StationaryReach).Exclusive)
			return;

		if (TryTurn)
		{
			// Keep the search local even when the hovered piece has thousands of studs.
			const StudSnapAnchor Candidate{ { MovingIndex, TargetIndex }, Proposed.Position, *DistanceSquared };
			const auto Position = std::upper_bound(TurnAnchors.begin(), TurnAnchors.end(), *DistanceSquared,
				[](float Distance, const StudSnapAnchor& Other) { return Distance < Other.DistanceSquared; });
			TurnAnchors.insert(Position, Candidate);
			if (TurnAnchors.size() > MaxStudTurnAnchors)
				TurnAnchors.pop_back();
		}

		if (!(*DistanceSquared < RadiusSquared))
			return;

		if (!GetSnapContactCount(MovingEndpoints, DragState.Stationary, lcMul(MovingToProposed, Correction->Transform), DragState.StationaryReach, false))
			return;

		BestCorrection = Correction->Transform;
		BestDistanceSquared = *DistanceSquared;
		BestPair = std::make_pair(MovingIndex, TargetIndex);
		DragState.ScreenCorrection = ScreenCorrection;
	};

	if (DragState.Pair)
	{
		const auto [MovingIndex, TargetIndex] = *DragState.Pair;

		if (MovingIndex < MovingEndpoints.size() && TargetIndex < DragState.Stationary.size())
		{
			lcConnectionPoint Proposed = MovingEndpoints[MovingIndex].Connection;

			Proposed.Transform(MovingToProposed);

			const float RadiusSquared = DragState.ReleaseRadius * DragState.ReleaseRadius;
			if (DragState.TurnAngle == 0.0f)
			{
				ConsiderPair(MovingIndex, TargetIndex, Proposed, RadiusSquared, true);
			}
			else if (IsStudTurnTarget(DragState.Stationary[TargetIndex], Context))
			{
				const lcConnectionPoint& Target = DragState.Stationary[TargetIndex].Connection;
				const lcVector3 AnchorDelta = Target.Position - Proposed.Position;
				const bool DepthValid = fabsf(lcDot(AnchorDelta, Target.Direction)) <= OccupiedTolerance ||
					fabsf(lcDot(AnchorDelta, Context.ViewDirection)) <= DepthLimit;
				if (DepthValid && Proposed.Type != lcStudType::None && Proposed.Type != Target.Type &&
					AreStudDirectionsAligned(Proposed.Direction, Target.Direction) && Proposed.Connection->IsCompatibleWith(*Target.Connection))
				{
					const lcMatrix44 Correction = GetStudTurnCorrection(Proposed.Position, Target, DragState.TurnAngle);
					const std::optional<lcVector2> SnappedGrab = ProjectEndpoint(Context, lcMul31(Context.Turn->GrabPoint, Correction));
					if (SnappedGrab)
					{
						DragState.ReleaseCenter = *SnappedGrab;
						const float DistanceSquared = DragState.GetReleaseDistanceSquared(Context.Cursor);
						if (DistanceSquared < RadiusSquared &&
							GetSnapContactCount(MovingEndpoints, DragState.Stationary, lcMul(MovingToProposed, Correction), DragState.StationaryReach, true).value_or(0) >= 2)
						{
							// The larger radius only covers the initial move to the seated grip.
							constexpr float DefaultRadius = lcConnectionDragState::DefaultReleaseRadius;
							if (DistanceSquared < DefaultRadius * DefaultRadius)
								DragState.ReleaseRadius = DefaultRadius;

							return Correction;
						}
					}
				}
			}
		}
	}

	if (!BestPair)
		DragState.Pair.reset();

	if (BestPair)
	{
		const float SwitchRadius = lcMax(sqrtf(BestDistanceSquared) - SwitchMargin, 0.0f);
		SearchRadiusSquared = SwitchRadius * SwitchRadius;
	}

	for (size_t MovingIndex = 0; MovingIndex < MovingEndpoints.size() && SearchRadiusSquared > 0.0f; MovingIndex++)
	{
		lcConnectionPoint Proposed = MovingEndpoints[MovingIndex].Connection;

		Proposed.Transform(MovingToProposed);

		const float Range = WorldLimit + Proposed.Reach + DragState.StationaryReach;
		for (auto Target = FindEndpointByX(DragState.Stationary, Proposed.Position.x - Range);
			Target != DragState.Stationary.cend() && Target->Connection.Position.x <= Proposed.Position.x + Range; ++Target)
		{
			const size_t TargetIndex = static_cast<size_t>(Target - DragState.Stationary.cbegin());
			ConsiderPair(MovingIndex, TargetIndex, Proposed, lcMin(SearchRadiusSquared, BestDistanceSquared), false);
		}
	}

	if (GrabScreen && !TurnAnchors.empty())
	{
		if (const std::optional<StudSnapCandidate> Candidate = FindStudSnapWithTurn(MovingEndpoints, DragState.Stationary, TurnAnchors, MovingToProposed, Context, *GrabScreen, DragState.StationaryReach, BestCorrection))
		{
			const StudSnapAnchor& Anchor = TurnAnchors[Candidate->AnchorIndex];
			BestAngle = Candidate->Angle;
			BestCorrection = Candidate->Correction;
			BestPair = Anchor.Pair;
			BestDistanceSquared = Candidate->GrabDistanceSquared;

			const lcVector2 SnappedGrab = Candidate->SnappedGrabScreen;
			if (BestAngle != 0.0f)
			{
				DragState.ReleaseCenter = SnappedGrab;
				BestDistanceSquared = DragState.GetReleaseDistanceSquared(Context.Cursor);
			}
			else
				DragState.ScreenCorrection = lcVector2(SnappedGrab.x - GrabScreen->x, SnappedGrab.y - GrabScreen->y);
		}
	}

	if (BestPair)
	{
		if (BestPair != DragState.Pair || BestAngle != DragState.TurnAngle)
		{
			constexpr float DefaultRadius = lcConnectionDragState::DefaultReleaseRadius;
			DragState.ReleaseRadius = DefaultRadius;
			if (BestAngle == 0.0f || BestDistanceSquared >= DefaultRadius * DefaultRadius)
				DragState.ReleaseRadius = lcMax(DefaultRadius, sqrtf(BestDistanceSquared) + 4.0f);
		}

		DragState.Pair = BestPair;
		DragState.IsClipSnap = false;
	}

	DragState.TurnAngle = BestAngle;
	if (!BestCorrection && !RayFirst)
		return FindRaySnap();

	return BestCorrection;
}

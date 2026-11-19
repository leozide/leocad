#include "lc_global.h"
#include "lc_connectioninfo.h"
#include "lc_file.h"
#include "lc_library.h"
#include "lc_piecedata.h"
#include "lc_zipfile.h"

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

void lcConnectionInfo::Initialize(lcPiecesLibrary* Library)
{
	lcZipFile Archive;
	
	if (!Archive.OpenRead(QLatin1String(":/resources/connections.zip")))
		return;

	lcResult<void> LoadResult = LoadArchive(Archive, Library);
	
	if (!LoadResult)
		qWarning() << "Could not load bundled connection data:" << LoadResult.error();
}

bool lcConnectionInfo::AreStudDirectionsAligned(const lcVector3& FirstDirection, const lcVector3& SecondDirection)
{
	// The male axis points out from its base and the receiver axis points in from its opening.
	constexpr float MinAlignment = 0.9999f;
	const float Alignment = lcDot(FirstDirection, SecondDirection) / (FirstDirection.Length() * SecondDirection.Length());

	return std::isfinite(Alignment) && Alignment >= MinAlignment;
}

std::optional<lcVector3> lcConnectionInfo::GetStudSnapTranslation(const lcConnectionPoint& MovingConnection, const lcMatrix44& MovingTransform, const lcConnectionPoint& TargetConnection, const lcMatrix44& TargetTransform)
{
	const bool StudPair = (MovingConnection.Type == lcConnectionType::StudMale && TargetConnection.Type == lcConnectionType::StudReceiver) ||
		(MovingConnection.Type == lcConnectionType::StudReceiver && TargetConnection.Type == lcConnectionType::StudMale);

	if (!StudPair)
		return std::nullopt;

	const lcVector3 MovingDirection = lcMul30(MovingConnection.Direction, MovingTransform);
	const lcVector3 TargetDirection = lcMul30(TargetConnection.Direction, TargetTransform);

	if (!AreStudDirectionsAligned(MovingDirection, TargetDirection))
		return std::nullopt;

	const lcVector3 MovingAnchor = lcMul31(MovingConnection.Position, MovingTransform);
	const lcVector3 TargetAnchor = lcMul31(TargetConnection.Position, TargetTransform);

	return TargetAnchor - MovingAnchor;
}

std::optional<lcStudMatingFace> lcConnectionInfo::FindStudMatingFace(const lcVector3& HitPosition, const lcVector3& RayDirection, float HitDistance, const lcBoundingBox& SurfaceBounds) const
{
	// The ray can enter the bounding box through a side before hitting the studded face.
	constexpr float StudHeight = 4.0f;
	constexpr float SurfaceTolerance = 0.1f;

	for (const lcConnectionPoint& Connection : mConnections)
	{
		const bool IsTop = Connection.Type == lcConnectionType::StudMale;
		const float Height = HitPosition.z - Connection.Position.z;
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
			return lcStudMatingFace{ Opening, Connection.Position.z, IsTop };
	}

	return std::nullopt;
}

lcMatrix44 lcConnectionInfo::GetStudMatingTransform(const lcStudMatingFace& Face, const lcMatrix44& SurfaceTransform, const lcMatrix33& SnapRotation, const lcVector3& GrabOffset, float PlacementOffset, const lcConnectionInfo* MovingConnections)
{
	const lcConnectionType MovingType = Face.IsTop ? lcConnectionType::StudReceiver : lcConnectionType::StudMale;
	const float MatingOffset = MovingConnections ? MovingConnections->GetStudMatingOffset(MovingType, SnapRotation, lcVector3(SurfaceTransform[2]), PlacementOffset) : PlacementOffset;
	const lcVector3 LocalGrabOffset = lcMul30(GrabOffset, lcMatrix44AffineInverse(SurfaceTransform));

	// Project anchors at mating depth even when the ray hits the top of a raised stud.
	const lcVector3 Position(Face.Point.x + LocalGrabOffset.x, Face.Point.y + LocalGrabOffset.y, Face.PlaneZ + MatingOffset);
	return lcMatrix44(SnapRotation, lcMul31(Position, SurfaceTransform));
}

float lcConnectionInfo::GetStudMatingOffset(lcConnectionType Type, const lcMatrix33& Rotation, const lcVector3& SurfaceAxis, float PlacementOffset) const
{
	float MatingOffset = PlacementOffset;
	float BestOffsetDifference = std::numeric_limits<float>::infinity();

	for (const lcConnectionPoint& Connection : mConnections)
	{
		if (Connection.Type != Type)
			continue;

		const float Offset = -lcDot(lcMul(Connection.Position, Rotation), SurfaceAxis);
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

bool lcConnectionInfo::ParseVector(const QJsonValue& Value, lcVector3& Vector)
{
	if (!Value.isArray())
		return false;

	const QJsonArray Array = Value.toArray();
	
	if (Array.size() != 3)
		return false;

	float Components[3];
	
	for (int ComponentIndex = 0; ComponentIndex < 3; ComponentIndex++)
	{
		if (!Array[ComponentIndex].isDouble())
			return false;

		const double Component = Array[ComponentIndex].toDouble();
		if (!std::isfinite(Component) || Component < -FLT_MAX || Component > FLT_MAX)
			return false;

		Components[ComponentIndex] = static_cast<float>(Component);
	}

	Vector = lcVector3LDrawToLeoCAD(lcVector3(Components[0], Components[1], Components[2]));
	
	return !Vector.IsNan();
}

bool lcConnectionInfo::ParsePart(const QByteArray& Data, const std::string& PartName, int ExpectedVersion, int ExpectedConnections, std::vector<lcConnectionPoint>& Connections)
{
	QJsonParseError ParseError;
	const QJsonDocument Document = QJsonDocument::fromJson(Data, &ParseError);
	
	if (ParseError.error != QJsonParseError::NoError || !Document.isObject())
		return false;

	const QJsonObject Root = Document.object();
	
	if (!Root.value(QStringLiteral("part")).isString() || Root.value(QStringLiteral("part")).toString().toStdString() != PartName ||
		!Root.value(QStringLiteral("version")).isDouble() || Root.value(QStringLiteral("version")).toInt(-1) != ExpectedVersion ||
		!Root.value(QStringLiteral("connections")).isArray())
		return false;

	const QJsonArray Array = Root.value(QStringLiteral("connections")).toArray();
	
	if (Array.size() != ExpectedConnections)
		return false;

	std::vector<lcConnectionPoint> ParsedConnections;
	ParsedConnections.reserve(static_cast<size_t>(Array.size()));
	std::unordered_set<std::string> ConnectionIds;

	for (const QJsonValue& ConnectionValue : Array)
	{
		if (!ConnectionValue.isObject())
			return false;

		const QJsonObject ConnectionObject = ConnectionValue.toObject();
		
		if (!ConnectionObject.value(QStringLiteral("id")).isString() || !ConnectionObject.value(QStringLiteral("type")).isString())
			return false;

		const std::string ConnectionId = ConnectionObject.value(QStringLiteral("id")).toString().toStdString();
		
		if (ConnectionId.empty() || !ConnectionIds.insert(ConnectionId).second)
			return false;

		lcConnectionPoint Connection;
		const QString ConnectionType = ConnectionObject.value(QStringLiteral("type")).toString();
		
		if (ConnectionType == QStringLiteral("stud_male"))
			Connection.Type = lcConnectionType::StudMale;
		else if (ConnectionType == QStringLiteral("stud_receiver"))
			Connection.Type = lcConnectionType::StudReceiver;
		else
			return false;

		if (!ParseVector(ConnectionObject.value(QStringLiteral("position")), Connection.Position) ||
			!ParseVector(ConnectionObject.value(QStringLiteral("direction")), Connection.Direction))
			return false;

		const float DirectionLength = Connection.Direction.Length();
		
		if (!std::isfinite(DirectionLength) || fabsf(DirectionLength - 1.0f) > 0.001f)
			return false;

		ParsedConnections.push_back(std::move(Connection));
	}

	Connections = std::move(ParsedConnections);
	
	return true;
}

lcResult<void> lcConnectionInfo::LoadArchive(lcZipFile& Archive, lcPiecesLibrary* Library)
{
	const std::optional<quint32> ManifestIndex = Archive.FindFileIndex("manifest.json");
	
	if (!ManifestIndex)
		return lcUnexpected(QStringLiteral("Connection archive is missing manifest.json."));

	lcMemFile ManifestFile;
	
	if (!Archive.ExtractFile(*ManifestIndex, ManifestFile, 1024 * 1024))
		return lcUnexpected(QStringLiteral("Could not read connection archive manifest."));
	
	const QByteArray ManifestBytes(reinterpret_cast<const char*>(ManifestFile.mBuffer), static_cast<int>(ManifestFile.mFileSize));
	QJsonParseError ParseError;
	const QJsonDocument ManifestDocument = QJsonDocument::fromJson(ManifestBytes, &ParseError);
	
	if (ParseError.error != QJsonParseError::NoError || !ManifestDocument.isObject())
		return lcUnexpected(QStringLiteral("Connection archive manifest is invalid JSON."));

	const QJsonObject Manifest = ManifestDocument.object();
	
	if (Manifest.value(QStringLiteral("format")).toString() != QStringLiteral("leocad-connections") ||
		!Manifest.value(QStringLiteral("version")).isDouble() ||
		Manifest.value(QStringLiteral("version")).toInt(-1) != 1 ||
		Manifest.value(QStringLiteral("coordinates")).toString() != QStringLiteral("ldraw_part_local") ||
		Manifest.value(QStringLiteral("units")).toString() != QStringLiteral("ldu") ||
		!Manifest.value(QStringLiteral("parts")).isObject())
		return lcUnexpected(QStringLiteral("Connection archive format or version is unsupported."));

	const QJsonObject PartEntries = Manifest.value(QStringLiteral("parts")).toObject();
	std::unordered_set<std::string> ExpectedArchiveEntries { "manifest.json" };
	int ConnectionTotal = 0;
	int MaleTotal = 0;
	int ReceiverTotal = 0;
	std::vector<std::pair<lcPieceData*, std::vector<lcConnectionPoint>>> PendingConnections;

	for (auto Entry = PartEntries.constBegin(); Entry != PartEntries.constEnd(); ++Entry)
	{
		if (!Entry.value().isObject() || !Entry.key().endsWith(QStringLiteral(".dat")))
			return lcUnexpected(QStringLiteral("Connection archive contains an invalid part entry."));

		const QJsonObject PartEntry = Entry.value().toObject();
		const QString ArchivePath = PartEntry.value(QStringLiteral("file")).toString();
		const int Count = PartEntry.value(QStringLiteral("connections")).toInt(-1);
		const QString ExpectedHash = PartEntry.value(QStringLiteral("sha256")).toString().toLower();
		const QJsonValue Version = PartEntry.value(QStringLiteral("version"));
		const bool SupportedVersion = Version.isUndefined() || (Version.isDouble() && Version.toDouble() == 1.0);
		const bool HashIsHex = ExpectedHash.size() == 64 && std::all_of(ExpectedHash.cbegin(), ExpectedHash.cend(), [](QChar Character)
		{
			return Character.isDigit() || (Character >= QLatin1Char('a') && Character <= QLatin1Char('f'));
		});
		
		if (ArchivePath != QStringLiteral("parts/") + Entry.key().left(Entry.key().size() - 4) + QStringLiteral(".json") || Count < 0 || !SupportedVersion || !HashIsHex)
			return lcUnexpected(QStringLiteral("Connection archive manifest has invalid metadata for %1.").arg(Entry.key()));

		const std::optional<quint32> PartIndex = Archive.FindFileIndex(ArchivePath.toLatin1().constData());
		
		if (!PartIndex)
			return lcUnexpected(QStringLiteral("Connection archive is missing %1.").arg(ArchivePath));

		lcMemFile PartFile;
		
		if (!Archive.ExtractFile(*PartIndex, PartFile, 16 * 1024 * 1024))
			return lcUnexpected(QStringLiteral("Could not read %1.").arg(ArchivePath));

		const QByteArray PartBytes(reinterpret_cast<const char*>(PartFile.mBuffer), static_cast<int>(PartFile.mFileSize));
		const QByteArray ActualHash = QCryptographicHash::hash(PartBytes, QCryptographicHash::Sha256).toHex();
		
		if (ActualHash != ExpectedHash.toLatin1())
			return lcUnexpected(QStringLiteral("Connection data hash mismatch for %1.").arg(Entry.key()));

		const std::string PartName = Entry.key().toStdString();
		std::vector<lcConnectionPoint> Connections;
		
		if (!ParsePart(PartBytes, PartName, 1, Count, Connections))
			return lcUnexpected(QStringLiteral("Connection data is invalid for %1.").arg(Entry.key()));

		for (const lcConnectionPoint& Connection : Connections)
			if (Connection.Type == lcConnectionType::StudMale)
				MaleTotal++;
			else
				ReceiverTotal++;

		ConnectionTotal += Count;
		ExpectedArchiveEntries.insert(ArchivePath.toStdString());

		lcPieceData* PieceData = Library->FindPieceData(PartName.c_str(), nullptr);
		
		if (PieceData && PieceData->IsLibraryPiece() && !PieceData->GetConnectionInfo())
			PendingConnections.emplace_back(PieceData, std::move(Connections));
	}

	const QJsonObject Counts = Manifest.value(QStringLiteral("counts")).toObject();
	
	if (Counts.value(QStringLiteral("parts")).toInt(-1) != PartEntries.size() ||
		Counts.value(QStringLiteral("connections")).toInt(-1) != ConnectionTotal ||
		Counts.value(QStringLiteral("stud_male")).toInt(-1) != MaleTotal ||
		Counts.value(QStringLiteral("stud_receiver")).toInt(-1) != ReceiverTotal ||
		ExpectedArchiveEntries.size() != Archive.mFiles.size())
		return lcUnexpected(QStringLiteral("Connection archive entries or manifest counts do not match."));

	for (const lcZipFileInfo& FileInfo : Archive.mFiles)
		if (ExpectedArchiveEntries.find(FileInfo.file_name) == ExpectedArchiveEntries.end())
			return lcUnexpected(QStringLiteral("Connection archive contains an unlisted entry."));
	
	for (auto& [PieceData, Connections] : PendingConnections)
		if (!PieceData->GetConnectionInfo())
			PieceData->SetConnectionInfo(new lcConnectionInfo(std::move(Connections)));

	return lcResult<void>();
}

std::vector<lcConnectionDragState::Endpoint>::const_iterator lcConnectionInfo::FindEndpointByX(const std::vector<lcConnectionDragState::Endpoint>& Endpoints, float X)
{
	using Endpoint = lcConnectionDragState::Endpoint;
	return std::lower_bound(Endpoints.cbegin(), Endpoints.cend(), X, [](const Endpoint& Point, float Value)
	{
		return Point.Connection.Position.x < Value;
	});
}

bool lcConnectionInfo::IsOccupied(lcConnectionDragState::Endpoint& Point, const std::vector<lcConnectionDragState::Endpoint>& Endpoints, bool Sorted)
{
	if (!Point.Occupied)
	{
		const float X = Point.Connection.Position.x;
		Point.Occupied = false;

		for (auto Other = Sorted ? FindEndpointByX(Endpoints, X - OccupiedTolerance) : Endpoints.cbegin(); Other != Endpoints.cend(); ++Other)
		{
			if (Sorted && Other->Connection.Position.x > X + OccupiedTolerance)
				break;

			if (Point.Piece != Other->Piece && Point.Connection.Type != Other->Connection.Type &&
				lcLengthSquared(Other->Connection.Position - Point.Connection.Position) <= OccupiedTolerance * OccupiedTolerance &&
				AreStudDirectionsAligned(Point.Connection.Direction, Other->Connection.Direction))
			{
				Point.Occupied = true;
				break;
			}
		}
	}

	return *Point.Occupied;
}

std::optional<size_t> lcConnectionInfo::GetStudContactCount(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const lcMatrix44& MovingToSnapped, bool CountContacts)
{
	using Endpoint = lcConnectionDragState::Endpoint;
	std::unordered_set<size_t> MatchedTargets;

	for (Endpoint& Moving : MovingEndpoints)
	{
		const lcConnectionPoint Proposed
		{
			Moving.Connection.Type,
			lcMul31(Moving.Connection.Position, MovingToSnapped),
			lcMul30(Moving.Connection.Direction, MovingToSnapped)
		};

		const float X = Proposed.Position.x;
		bool Matched = false;

		for (auto Target = FindEndpointByX(StationaryEndpoints, X - OccupiedTolerance);
			Target != StationaryEndpoints.cend() && Target->Connection.Position.x <= X + OccupiedTolerance; ++Target)
		{
			if (Proposed.Type == Target->Connection.Type ||
				lcLengthSquared(Proposed.Position - Target->Connection.Position) > OccupiedTolerance * OccupiedTolerance ||
				!AreStudDirectionsAligned(Proposed.Direction, Target->Connection.Direction))
				continue;

			const size_t TargetIndex = static_cast<size_t>(Target - StationaryEndpoints.cbegin());
			Endpoint& Stationary = StationaryEndpoints[TargetIndex];

			// Reject the entire pose if it tries to reuse an occupied connector.
			if (IsOccupied(Moving, MovingEndpoints, false) || IsOccupied(Stationary, StationaryEndpoints, true))
				return std::nullopt;

			if (CountContacts && !Matched)
				Matched = MatchedTargets.insert(TargetIndex).second;
		}
	}

	return MatchedTargets.size();
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

std::optional<lcConnectionInfo::StudSnapCandidate> lcConnectionInfo::FindStudSnapWithTurn(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, std::vector<lcConnectionDragState::Endpoint>& StationaryEndpoints, const std::vector<StudSnapAnchor>& Anchors, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, const lcVector2& GrabScreen, const std::optional<lcMatrix44>& UnturnedCorrection)
{
	std::optional<StudSnapCandidate> BestTurn;
	size_t BestCount = 1;
	constexpr float GrabLimitSquared = StudTurnGrabScreenLimit * StudTurnGrabScreenLimit;

	auto CountContacts = [&](const lcMatrix44& Correction)
	{
		return GetStudContactCount(MovingEndpoints, StationaryEndpoints, lcMul(MovingToProposed, Correction), true).value_or(0);
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

std::optional<float> lcConnectionInfo::GetStudPairScreenDistanceSquared(const lcConnectionPoint& Proposed, const lcConnectionDragState::Endpoint& Target, const lcVector2& MovingScreen, const lcConnectionSnapContext& Context, bool Retained, lcVector2& ScreenCorrection)
{
	if (Proposed.Type == Target.Connection.Type)
		return std::nullopt;

	const lcVector3 Difference = Target.Connection.Position - Proposed.Position;
	const float WorldDistanceSquared = lcLengthSquared(Difference);
	const bool OnMatingPlane = fabsf(lcDot(Difference, Target.Connection.Direction)) <= OccupiedTolerance;
	// A ray hit establishes which surface is under the cursor. On that same
	// mating plane, cover the gaps between studs even when zoomed in.
	const bool OnHoveredFace = Target.Piece == Context.SurfacePiece &&
		WorldDistanceSquared <= 2.0f * HalfStudPitch * HalfStudPitch + OccupiedTolerance &&
		OnMatingPlane;

	// With no new surface hit, the retained pair already identifies the mating plane.
	const bool DepthValid = Retained && !Context.HasSurfaceHit ? OnMatingPlane :
		OnHoveredFace || fabsf(lcDot(Difference, Context.ViewDirection)) <= DepthLimit;

	if (WorldDistanceSquared > WorldLimit * WorldLimit || !DepthValid)
		return std::nullopt;

	const std::optional<lcVector2> TargetScreen = ProjectEndpoint(Context, Target.Connection.Position);

	if (!TargetScreen)
		return std::nullopt;

	const float X = TargetScreen->x - MovingScreen.x;
	const float Y = TargetScreen->y - MovingScreen.y;
	const float DistanceSquared = X * X + Y * Y;

	if (!Retained && !OnHoveredFace && DistanceSquared >= ScreenLimit * ScreenLimit)
		return std::nullopt;

	ScreenCorrection = lcVector2(X, Y);
	return DistanceSquared;
}

std::optional<lcMatrix44> lcConnectionInfo::FindStudSnap(std::vector<lcConnectionDragState::Endpoint>& MovingEndpoints, const lcMatrix44& MovingToProposed, const lcConnectionSnapContext& Context, lcConnectionDragState& DragState)
{
	using Endpoint = lcConnectionDragState::Endpoint;
	const lcMatrix44 Identity = lcMatrix44Identity();
	const std::optional<lcVector2> GrabScreen = Context.Turn && Context.SurfacePiece ? ProjectEndpoint(Context, Context.Turn->GrabPoint) : std::nullopt;
	std::vector<StudSnapAnchor> TurnAnchors;
	if (GrabScreen)
		TurnAnchors.reserve(MaxStudTurnAnchors + 1);

	float BestDistanceSquared = std::numeric_limits<float>::infinity();
	std::optional<lcMatrix44> BestCorrection;
	std::optional<std::pair<size_t, size_t>> BestPair;
	float BestAngle = 0.0f;
	float SearchRadiusSquared = std::numeric_limits<float>::infinity();

	auto ConsiderPair = [&](size_t MovingIndex, size_t TargetIndex, const lcConnectionPoint& Proposed, const lcVector2& MovingScreen, float RadiusSquared, bool Retained)
	{
		Endpoint& Moving = MovingEndpoints[MovingIndex];
		Endpoint& Target = DragState.Stationary[TargetIndex];
		lcVector2 ScreenCorrection;
		const std::optional<float> DistanceSquared = GetStudPairScreenDistanceSquared(Proposed, Target, MovingScreen, Context, Retained, ScreenCorrection);
		if (!DistanceSquared)
			return;

		const bool TryTurn = !Retained && GrabScreen && *DistanceSquared < SearchRadiusSquared && IsStudTurnTarget(Target, Context) &&
			(TurnAnchors.size() < MaxStudTurnAnchors || *DistanceSquared < TurnAnchors.back().DistanceSquared);
		if (!TryTurn && !(*DistanceSquared < RadiusSquared))
			return;

		const std::optional<lcVector3> Translation = lcConnectionInfo::GetStudSnapTranslation(Proposed, Identity, Target.Connection, Identity);

		if (!Translation || IsOccupied(Moving, MovingEndpoints, false) || IsOccupied(Target, DragState.Stationary, true))
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

		const lcMatrix44 Correction = lcMatrix44Translation(*Translation);
		if (!GetStudContactCount(MovingEndpoints, DragState.Stationary, lcMul(MovingToProposed, Correction), false))
			return;

		BestCorrection = Correction;
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

			Proposed.Position = lcMul31(Proposed.Position, MovingToProposed);
			Proposed.Direction = lcMul30(Proposed.Direction, MovingToProposed);

			const float RadiusSquared = DragState.ReleaseRadius * DragState.ReleaseRadius;
			if (DragState.TurnAngle == 0.0f)
			{
				if (const std::optional<lcVector2> Screen = ProjectEndpoint(Context, Proposed.Position))
					ConsiderPair(MovingIndex, TargetIndex, Proposed, *Screen, RadiusSquared, true);
			}
			else if (IsStudTurnTarget(DragState.Stationary[TargetIndex], Context))
			{
				const lcConnectionPoint& Target = DragState.Stationary[TargetIndex].Connection;
				const lcVector3 AnchorDelta = Target.Position - Proposed.Position;
				const bool DepthValid = fabsf(lcDot(AnchorDelta, Target.Direction)) <= OccupiedTolerance ||
					fabsf(lcDot(AnchorDelta, Context.ViewDirection)) <= DepthLimit;
				if (DepthValid && Proposed.Type != Target.Type && AreStudDirectionsAligned(Proposed.Direction, Target.Direction))
				{
					const lcMatrix44 Correction = GetStudTurnCorrection(Proposed.Position, Target, DragState.TurnAngle);
					const std::optional<lcVector2> SnappedGrab = ProjectEndpoint(Context, lcMul31(Context.Turn->GrabPoint, Correction));
					if (SnappedGrab)
					{
						DragState.ReleaseCenter = *SnappedGrab;
						const float DistanceSquared = DragState.GetReleaseDistanceSquared(Context.Cursor);
						if (DistanceSquared < RadiusSquared &&
							GetStudContactCount(MovingEndpoints, DragState.Stationary, lcMul(MovingToProposed, Correction), true).value_or(0) >= 2)
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

		Proposed.Position = lcMul31(Proposed.Position, MovingToProposed);
		Proposed.Direction = lcMul30(Proposed.Direction, MovingToProposed);

		const std::optional<lcVector2> Screen = ProjectEndpoint(Context, Proposed.Position);

		if (!Screen)
			continue;

		for (auto Target = FindEndpointByX(DragState.Stationary, Proposed.Position.x - WorldLimit);
			Target != DragState.Stationary.cend() && Target->Connection.Position.x <= Proposed.Position.x + WorldLimit; ++Target)
		{
			const size_t TargetIndex = static_cast<size_t>(Target - DragState.Stationary.cbegin());
			ConsiderPair(MovingIndex, TargetIndex, Proposed, *Screen, lcMin(SearchRadiusSquared, BestDistanceSquared), false);
		}
	}

	if (GrabScreen && !TurnAnchors.empty())
	{
		if (const std::optional<StudSnapCandidate> Candidate = FindStudSnapWithTurn(MovingEndpoints, DragState.Stationary, TurnAnchors, MovingToProposed, Context, *GrabScreen, BestCorrection))
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
	}

	DragState.TurnAngle = BestAngle;
	return BestCorrection;
}

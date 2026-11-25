#include "lc_global.h"
#include "lc_shadowconnection.h"
#include "lc_file.h"
#include "lc_library.h"

using lcShadowProperties = std::map<QByteArray, QByteArray>;

enum class lcShadowScalePolicy { None, Axial, Radial, AxialOrRadial };
enum class lcShadowMirrorPolicy { None, CorrectX, CorrectZ };
enum class lcShadowTransformKind { Declaration, Include, Reference };

struct lcShadowConnectionRecord
{
	lcShadowConnection Data;
	std::vector<QByteArray> Ids;
	lcShadowScalePolicy Scaling = lcShadowScalePolicy::None;
	lcShadowMirrorPolicy Mirroring = lcShadowMirrorPolicy::None;
};

struct lcShadowConnectionRecords
{
	std::vector<lcShadowConnectionRecord> Items;
	bool HasDefinitions = false;
};

using lcShadowWords = QList<QByteArray>;

struct lcShadowGrid
{
	qlonglong Count[3]{ 1, 1, 1 };
	lcVector3 Step = lcVector3(0.0f, 0.0f, 0.0f);
	lcVector3 Center = lcVector3(0.0f, 0.0f, 0.0f);
};

static lcShadowWords lcShadowSplitWords(const QByteArray& Text)
{
	return Text.isEmpty() ? lcShadowWords() : Text.simplified().split(' ');
}

static QByteArray lcShadowGetProperty(const lcShadowProperties& Values, const char* Key, const char* Default = "")
{
	const lcShadowProperties::const_iterator Entry = Values.find(Key);
	return Entry == Values.end() ? QByteArray(Default) : Entry->second;
}

static bool lcShadowReadProperties(const QByteArray& Text, int Offset, lcShadowProperties& Values)
{
	while (Offset < Text.size())
	{
		while (Offset < Text.size() && Text[Offset] == ' ')
			Offset++;

		if (Offset == Text.size())
			return true;

		const int End = Text.indexOf(']', Offset + 1);
		const int Equals = Text.indexOf('=', Offset + 1);

		if (Text[Offset] != '[' || End < 0 || Equals < 0 || Equals >= End)
			return false;

		const QByteArray Key = Text.mid(Offset + 1, Equals - Offset - 1).trimmed().toLower();
		const QByteArray Value = Text.mid(Equals + 1, End - Equals - 1).trimmed();
		Values[Key] = Value;
		Offset = End + 1;
	}

	return true;
}

static bool lcShadowReadNumber(const QByteArray& Text, float& Number)
{
	bool Ok;
	Number = Text.toFloat(&Ok);
	return Ok && std::isfinite(Number);
}

static bool lcShadowReadNumbers(const lcShadowWords& Tokens, int Offset, int Count, float* Numbers)
{
	if (Tokens.size() - Offset < Count)
		return false;

	for (int Index = 0; Index < Count; Index++)
		if (!lcShadowReadNumber(Tokens[Offset + Index], Numbers[Index]))
			return false;

	return true;
}

static lcMatrix44 lcShadowMakeFrame(const float (&Position)[3], const float (&Orientation)[9])
{
	return lcMatrix44(lcVector4(Orientation[0], Orientation[3], Orientation[6], 0.0f),
		lcVector4(Orientation[1], Orientation[4], Orientation[7], 0.0f),
		lcVector4(Orientation[2], Orientation[5], Orientation[8], 0.0f),
		lcVector4(Position[0], Position[1], Position[2], 1.0f));
}

static bool lcShadowReadFrame(const lcShadowProperties& Values, lcMatrix44& Frame)
{
	const lcShadowWords Position = lcShadowSplitWords(lcShadowGetProperty(Values, "pos", "0 0 0"));
	const lcShadowWords Orientation = lcShadowSplitWords(lcShadowGetProperty(Values, "ori", "1 0 0 0 1 0 0 0 1"));
	float Offset[3], Basis[9];

	if (Position.size() != 3 || Orientation.size() != 9 ||
		!lcShadowReadNumbers(Position, 0, 3, Offset) || !lcShadowReadNumbers(Orientation, 0, 9, Basis))
		return false;

	Frame = lcShadowMakeFrame(Offset, Basis);
	return true;
}

static bool lcShadowReadGrid(const QByteArray& Text, lcShadowGrid& Grid)
{
	const lcShadowWords Tokens = lcShadowSplitWords(Text);
	if (Tokens.empty())
		return true;

	const int ValueCount = Tokens.size() - std::count_if(Tokens.begin(), Tokens.end(), [](const QByteArray& Token) { return Token.toUpper() == "C"; });
	if (ValueCount != 4 && ValueCount != 6)
		return false;

	const bool HasYAxis = ValueCount == 6;
	int Offset = 0;

	for (int Axis = 0; Axis < 3; Axis++)
	{
		if (Axis == 1 && !HasYAxis)
			continue;

		const bool Centered = Offset < Tokens.size() && Tokens[Offset].toUpper() == "C";
		if (Centered)
			Offset++;

		if (Offset == Tokens.size())
			return false;

		bool Ok;
		Grid.Count[Axis] = Tokens[Offset++].toLongLong(&Ok);
		if (!Ok || Grid.Count[Axis] <= 0)
			return false;

		if (Centered)
			Grid.Center[Axis] = static_cast<float>(Grid.Count[Axis] - 1) * 0.5f;
	}

	for (int Axis = 0; Axis < 3; Axis++)
	{
		if (Axis == 1 && !HasYAxis)
			continue;

		if (Offset == Tokens.size() || !lcShadowReadNumber(Tokens[Offset++], Grid.Step[Axis]))
			return false;
	}

	return Offset == Tokens.size();
}

static lcShadowGender lcShadowReadGender(const lcShadowProperties& Values, const char* Key)
{
	return lcShadowGetProperty(Values, Key).toUpper() == "F" ? lcShadowGender::Female : lcShadowGender::Male;
}

static bool lcShadowReadCylinder(const lcShadowProperties& Values, lcShadowConnectionRecord& Item)
{
	const lcShadowWords Sections = lcShadowSplitWords(lcShadowGetProperty(Values, "secs"));

	if (Sections.empty() || Sections.size() % 3)
		return false;

	lcShadowCylinder Cylinder;
	using Profile = lcShadowCylinder::Profile;
	static const std::map<QByteArray, Profile> Profiles = {
		{ "R", Profile::Round }, { "A", Profile::Axle }, { "S", Profile::Square },
		{ "_L", Profile::FlexPrevious }, { "L_", Profile::FlexNext }
	};

	for (int Offset = 0; Offset < Sections.size(); Offset += 3)
	{
		const auto Shape = Profiles.find(Sections[Offset].toUpper());
		float Dimensions[2];
		if (Shape == Profiles.end() || !lcShadowReadNumbers(Sections, Offset + 1, 2, Dimensions) || Dimensions[0] < 0.0f || Dimensions[1] < 0.0f)
			return false;

		Cylinder.Sections.push_back({ Shape->second, Dimensions[0], Dimensions[1] });
	}

	Cylinder.Gender = lcShadowReadGender(Values, "gender");
	Cylinder.Centered = lcShadowGetProperty(Values, "center").toLower() == "true";
	Cylinder.Sliding = lcShadowGetProperty(Values, "slide").toLower() == "true";
	using Caps = lcShadowCylinder::Caps;
	const QByteArray Ends = lcShadowGetProperty(Values, "caps", "one").toLower();
	if (Ends == "none")
		Cylinder.Ends = Caps::None;
	else if (Ends == "two")
		Cylinder.Ends = Caps::Both;
	else if (Ends == "a")
		Cylinder.Ends = Caps::A;
	else if (Ends == "b" || (Ends == "one" && Cylinder.Gender == lcShadowGender::Female))
		Cylinder.Ends = Caps::B;
	else if (Ends != "one")
		return false;

	Item.Data.Shape = std::move(Cylinder);
	return true;
}

static bool lcShadowReadGeneric(const QByteArray& Kind, const lcShadowProperties& Values, lcShadowConnectionRecord& Item)
{
	const bool LegacySphere = Kind == "SNAP_SPH";
	const lcShadowWords Bounds = lcShadowSplitWords(LegacySphere
		? QByteArray("sph ") + lcShadowGetProperty(Values, "radius")
		: lcShadowGetProperty(Values, "bounding", "pnt"));
	if (Bounds.empty())
		return false;

	using Shape = lcShadowGenericConnector::Bounds;
	static const std::map<QByteArray, std::pair<Shape, int>> Shapes = {
		{ "pnt", { Shape::Point, 0 } }, { "box", { Shape::Box, 3 } }, { "cube", { Shape::Cube, 1 } },
		{ "cyl", { Shape::Cylinder, 2 } }, { "sph", { Shape::Sphere, 1 } }
	};
	const auto BoundingShape = Shapes.find(Bounds.front().toLower());
	float Dimensions[3]{ 0.0f, 0.0f, 0.0f };
	if (BoundingShape == Shapes.end() || Bounds.size() != BoundingShape->second.second + 1 ||
		!lcShadowReadNumbers(Bounds, 1, BoundingShape->second.second, Dimensions) ||
		std::any_of(std::begin(Dimensions), std::end(Dimensions), [](float Value) { return Value < 0.0f; }))
		return false;

	lcShadowGenericConnector Generic;
	Generic.Shape = BoundingShape->second.first;
	Generic.Dimensions = lcVector3(Dimensions[0], Dimensions[1], Dimensions[2]);
	Generic.Gender = lcShadowReadGender(Values, "gender");
	Generic.Match = lcShadowGetProperty(Values, "match", LegacySphere ? "size" : "shape").toLower();
	Generic.Placement = lcShadowGetProperty(Values, "placement", LegacySphere ? "free" : "").toLower();
	Item.Data.Shape = std::move(Generic);
	return true;
}

static bool lcShadowReadShape(const QByteArray& Kind, const lcShadowProperties& Values, lcShadowConnectionRecord& Item)
{
	if (Kind == "SNAP_CYL")
		return lcShadowReadCylinder(Values, Item);

	if (Kind == "SNAP_CLP")
	{
		lcShadowClip Clip;
		if (!lcShadowReadNumber(lcShadowGetProperty(Values, "radius", "4"), Clip.Radius) || Clip.Radius < 0.0f ||
			!lcShadowReadNumber(lcShadowGetProperty(Values, "length", "8"), Clip.Length) || Clip.Length < 0.0f)
			return false;

		Clip.Centered = lcShadowGetProperty(Values, "center").toLower() == "true";
		Clip.Sliding = lcShadowGetProperty(Values, "slide").toLower() == "true";
		Item.Data.Shape = Clip;
		return true;
	}

	if (Kind == "SNAP_FGR")
	{
		const lcShadowWords Sequence = lcShadowSplitWords(lcShadowGetProperty(Values, "seq"));
		lcShadowFingers Fingers;
		Fingers.Widths.resize(Sequence.size());
		if (Sequence.empty() || !lcShadowReadNumbers(Sequence, 0, Sequence.size(), Fingers.Widths.data()) ||
			!lcShadowReadNumber(lcShadowGetProperty(Values, "radius", "0"), Fingers.Radius) || Fingers.Radius < 0.0f ||
			std::any_of(Fingers.Widths.begin(), Fingers.Widths.end(), [](float Width) { return Width <= 0.0f; }))
			return false;

		Fingers.Centered = lcShadowGetProperty(Values, "center", "true").toLower() == "true";
		Fingers.FirstGender = lcShadowReadGender(Values, "genderofs");
		Item.Data.Shape = std::move(Fingers);
		return true;
	}

	return (Kind == "SNAP_GEN" || Kind == "SNAP_SPH") && lcShadowReadGeneric(Kind, Values, Item);
}

static bool lcShadowTransformConnection(lcShadowConnectionRecord& Item, const lcMatrix44& Parent, lcShadowTransformKind Kind)
{
	lcMatrix44 Frame = lcMul(Item.Data.Frame, Parent);
	lcVector3 Axes[3] = { lcVector3(Frame[0]), lcVector3(Frame[1]), lcVector3(Frame[2]) };
	lcVector3 Scale(lcLength(Axes[0]), lcLength(Axes[1]), lcLength(Axes[2]));
	constexpr float Tolerance = 0.001f;
	for (int Axis = 0; Axis < 3; Axis++)
	{
		if (!std::isfinite(Scale[Axis]) || Scale[Axis] <= LC_EPSILON || !std::isfinite(Frame[3][Axis]))
			return false;

		Axes[Axis] = Axes[Axis] / Scale[Axis];
	}

	if (Kind == lcShadowTransformKind::Declaration)
	{
		if (fabsf(lcDot(lcCross(Axes[0], Axes[1]), Axes[2])) <= LC_EPSILON)
			return false;
	}
	else
	{
		// An authored frame can be skewed. Reject additional skew introduced
		// by the parent by comparing the angles between normalized axes.
		const lcMatrix44& Original = Item.Data.Frame;
		for (int Axis = 1; Axis < 3; Axis++)
			for (int Previous = 0; Previous < Axis; Previous++)
				if (fabsf(lcDot(Axes[Previous], Axes[Axis]) - lcDot(lcVector3(Original[Previous]), lcVector3(Original[Axis]))) > Tolerance)
					return false;
	}

	const bool ScaledAxially = fabsf(Scale.y - 1.0f) > Tolerance;
	const bool ScaledRadially = fabsf(Scale.x - 1.0f) > Tolerance || fabsf(Scale.z - 1.0f) > Tolerance;
	if (Kind == lcShadowTransformKind::Reference)
	{
		if (ScaledAxially && (ScaledRadially || (Item.Scaling != lcShadowScalePolicy::Axial && Item.Scaling != lcShadowScalePolicy::AxialOrRadial)))
			return false;

		if (ScaledRadially && (fabsf(Scale.x - Scale.z) > Tolerance ||
			(Item.Scaling != lcShadowScalePolicy::Radial && Item.Scaling != lcShadowScalePolicy::AxialOrRadial)))
			return false;

		if (!ScaledAxially)
			Scale.y = 1.0f;

		if (!ScaledRadially)
			Scale.x = Scale.z = 1.0f;
	}

	// Mirroring policy applies to the parent transform. The authored frame
	// can itself be reflected, including when inherited through an identity.
	if (Kind != lcShadowTransformKind::Declaration && Parent.Determinant() < 0.0f)
	{
		if (Item.Mirroring == lcShadowMirrorPolicy::None)
			return false;

		const int Axis = Item.Mirroring == lcShadowMirrorPolicy::CorrectZ ? 2 : 0;
		Axes[Axis] = Axes[Axis] * -1.0f;
	}

	Item.Data.Frame = lcMatrix44(lcVector4(Axes[0], 0.0f), lcVector4(Axes[1], 0.0f), lcVector4(Axes[2], 0.0f), Frame[3]);
	Item.Data.Scale = lcVector3(Item.Data.Scale.x * Scale.x, Item.Data.Scale.y * Scale.y, Item.Data.Scale.z * Scale.z);
	return std::isfinite(Item.Data.Scale.x) && std::isfinite(Item.Data.Scale.y) && std::isfinite(Item.Data.Scale.z);
}

static QString lcShadowNormalizeReference(QString Path)
{
	Path.replace(QLatin1Char('\\'), QLatin1Char('/'));
	Path = QDir::cleanPath(Path).toUpper();

	if (Path == QLatin1String("..") || Path.startsWith(QLatin1String("../")) || QDir::isAbsolutePath(Path) || Path.contains(QLatin1Char(':')))
		return QString();

	return Path;
}

class lcShadowConnectionResolver
{
public:
	explicit lcShadowConnectionResolver(lcPiecesLibrary& Library)
		: mLibrary(Library)
	{
	}

	lcShadowConnectionData Resolve(lcFile& File, const QString& ShadowPath)
	{
		lcShadowConnectionRecords Resolved = ReadGeometry(File, lcShadowNormalizeReference(ShadowPath));
		lcShadowConnectionData Data;
		Data.HasDefinitions = Resolved.HasDefinitions;
		Data.Connections.reserve(Resolved.Items.size());

		for (lcShadowConnectionRecord& Item : Resolved.Items)
		{
			lcShadowConnection& Connection = Item.Data;
			Connection.Frame = lcMatrix44LDrawToLeoCAD(Connection.Frame);
			Connection.Scale = lcVector3Abs(lcVector3LDrawToLeoCAD(Connection.Scale));

			lcShadowGenericConnector* Generic = std::get_if<lcShadowGenericConnector>(&Connection.Shape);
			if (Generic && Generic->Shape == lcShadowGenericConnector::Bounds::Box)
				Generic->Dimensions = lcVector3Abs(lcVector3LDrawToLeoCAD(Generic->Dimensions));

			Data.Connections.emplace_back(std::move(Connection));
		}

		return Data;
	}

private:
	lcShadowConnectionRecords ReadGeometry(lcFile& File, const QString& Path);
	std::vector<QByteArray> ReadGeometryLines(lcFile& File) const;
	void ReadShadow(lcFile& File, lcShadowConnectionRecords& Result, bool AllowIncludes);
	void ReadDirective(const QByteArray& Line, lcShadowConnectionRecords& Result, bool AllowIncludes);
	void Include(const lcShadowProperties& Values, const lcMatrix44& Frame, const lcShadowGrid& Grid, lcShadowConnectionRecords& Result);
	void PlaceGrid(const lcShadowGrid& Grid, const lcMatrix44& Frame, const lcShadowConnectionRecord* Connections, size_t Count, lcShadowConnectionRecords& Result);
	void Append(lcShadowConnectionRecords& Result, const lcShadowConnectionRecords& Included, const lcMatrix44& Frame,
		const QByteArray& Id, const QByteArray& Slide, lcShadowTransformKind Kind);

	lcPiecesLibrary& mLibrary;
	std::map<QString, lcShadowConnectionRecords> mShadowIncludes;
};

void lcShadowConnectionResolver::Append(lcShadowConnectionRecords& Result, const lcShadowConnectionRecords& Included, const lcMatrix44& Frame,
	const QByteArray& Id, const QByteArray& Slide, lcShadowTransformKind Kind)
{
	Result.HasDefinitions |= Included.HasDefinitions;
	for (const lcShadowConnectionRecord& Source : Included.Items)
	{
		if (mLibrary.ShouldCancelLoading())
			return;

		lcShadowConnectionRecord Item = Source;
		if (!lcShadowTransformConnection(Item, Frame, Kind))
			continue;

		if (!Id.isEmpty())
			Item.Ids.push_back(Id);

		if (!Slide.isEmpty())
		{
			const bool Sliding = Slide.toLower() == "true";
			if (lcShadowCylinder* Cylinder = std::get_if<lcShadowCylinder>(&Item.Data.Shape))
				Cylinder->Sliding = Sliding;
			else if (lcShadowClip* Clip = std::get_if<lcShadowClip>(&Item.Data.Shape))
				Clip->Sliding = Sliding;
		}

		Result.Items.emplace_back(std::move(Item));
	}
}

void lcShadowConnectionResolver::Include(const lcShadowProperties& Values, const lcMatrix44& Frame,
	const lcShadowGrid& Grid, lcShadowConnectionRecords& Result)
{
	if (mLibrary.ShouldCancelLoading())
		return;

	const QString Ref = lcShadowNormalizeReference(QString::fromLatin1(lcShadowGetProperty(Values, "ref")));
	if (Ref.isEmpty() || Ref == QLatin1String("."))
		return;

	const lcShadowWords ScaleWords = lcShadowSplitWords(lcShadowGetProperty(Values, "scale", "1 1 1"));
	float Scale[3];
	if (ScaleWords.size() != 3 || !lcShadowReadNumbers(ScaleWords, 0, 3, Scale))
		return;

	auto [Entry, Inserted] = mShadowIncludes.try_emplace(Ref);
	if (Inserted)
	{
		lcMemFile Shadow;
		if (Ref.startsWith(QLatin1String("P/")) || Ref.startsWith(QLatin1String("PARTS/")))
		{
			if (!mLibrary.ReadShadowFile(Ref, Shadow))
				return;
		}
		else if (Ref.startsWith(QLatin1String("S/")))
		{
			if (!mLibrary.ReadShadowFile(QStringLiteral("parts/") + Ref, Shadow))
				return;
		}
		else if (!mLibrary.ReadShadowFile(QStringLiteral("p/") + Ref, Shadow) &&
			!mLibrary.ReadShadowFile(QStringLiteral("parts/") + Ref, Shadow))
			return;

		// Included directives are evaluated in isolation. CLEAR in an included
		// file only affects that file's records, and nested INCL is skipped.
		ReadShadow(Shadow, Entry->second, false);
	}

	lcMatrix44 Transform = Frame;
	for (int Axis = 0; Axis < 3; Axis++)
		Transform[Axis] = Transform[Axis] * Scale[Axis];

	// Grid positions share the same orientation, scale and mirror eligibility.
	Transform.SetTranslation(lcVector3(0.0f, 0.0f, 0.0f));

	const QByteArray Id = lcShadowGetProperty(Values, "id");
	const QByteArray Slide = lcShadowGetProperty(Values, "slide");
	lcShadowConnectionRecords Transformed;
	Append(Transformed, Entry->second, Transform, Id, Slide, lcShadowTransformKind::Include);
	Result.HasDefinitions |= Transformed.HasDefinitions;

	PlaceGrid(Grid, Frame, Transformed.Items.data(), Transformed.Items.size(), Result);
}

void lcShadowConnectionResolver::PlaceGrid(const lcShadowGrid& Grid, const lcMatrix44& Frame,
	const lcShadowConnectionRecord* Connections, size_t Count, lcShadowConnectionRecords& Result)
{
	if (!Count)
		return;

	lcVector3 Min = Connections[0].Data.Frame.GetTranslation();
	lcVector3 Max = Min;

	for (size_t Index = 1; Index < Count; Index++)
	{
		if (mLibrary.ShouldCancelLoading())
			return;

		const lcVector3 Position = Connections[Index].Data.Frame.GetTranslation();
		Min = lcMin(Min, Position);
		Max = lcMax(Max, Position);
	}

	for (qlonglong X = 0; X < Grid.Count[0]; X++)
		for (qlonglong Y = 0; Y < Grid.Count[1]; Y++)
			for (qlonglong Z = 0; Z < Grid.Count[2]; Z++)
			{
				if (mLibrary.ShouldCancelLoading())
					return;

				const lcVector3 Indices(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z));
				const lcVector3 Offset((Indices.x - Grid.Center.x) * Grid.Step.x,
					(Indices.y - Grid.Center.y) * Grid.Step.y, (Indices.z - Grid.Center.z) * Grid.Step.z);
				const lcVector3 Translation = lcMul31(Offset, Frame);

				// If even the closest bound overflows on an axis, no connection fits here.
				const lcVector3 Closest(Translation.x + (Translation.x < 0.0f ? Max.x : Min.x),
					Translation.y + (Translation.y < 0.0f ? Max.y : Min.y), Translation.z + (Translation.z < 0.0f ? Max.z : Min.z));

				if (!std::isfinite(Closest.x) || !std::isfinite(Closest.y) || !std::isfinite(Closest.z))
					continue;

				for (size_t Index = 0; Index < Count; Index++)
				{
					if (mLibrary.ShouldCancelLoading())
						return;

					const lcShadowConnectionRecord& Source = Connections[Index];
					const lcVector3 Position = Source.Data.Frame.GetTranslation() + Translation;

					if (!std::isfinite(Position.x) || !std::isfinite(Position.y) || !std::isfinite(Position.z))
						continue;

					Result.Items.push_back(Source);
					Result.Items.back().Data.Frame.SetTranslation(Position);
				}
			}
}

void lcShadowConnectionResolver::ReadDirective(const QByteArray& Line, lcShadowConnectionRecords& Result, bool AllowIncludes)
{
	if (!Line.startsWith("0 ") || Line.left(14).toUpper() != "0 !LDCAD SNAP_")
		return;

	int End = Line.indexOf(' ', 9);
	if (End < 0)
		End = Line.size();

	const QByteArray Kind = Line.mid(9, End - 9).toUpper();
	lcShadowProperties Values;
	if (!lcShadowReadProperties(Line, End, Values))
		return;

	const QByteArray Id = lcShadowGetProperty(Values, "id");
	if (Kind == "SNAP_CLEAR")
	{
		Result.HasDefinitions = true;
		if (Id.isEmpty())
			Result.Items.clear();
		else
			Result.Items.erase(std::remove_if(Result.Items.begin(), Result.Items.end(), [&Id](const lcShadowConnectionRecord& Item)
			{
				return std::find(Item.Ids.begin(), Item.Ids.end(), Id) != Item.Ids.end();
			}), Result.Items.end());
		return;
	}

	const bool IsInclude = Kind == "SNAP_INCL";
	lcShadowConnectionRecord Item;

	if ((IsInclude && !AllowIncludes) || (!IsInclude && !lcShadowReadShape(Kind, Values, Item)))
		return;

	lcMatrix44 Frame;
	lcShadowGrid Grid;
	if (!lcShadowReadFrame(Values, Frame) || !lcShadowReadGrid(lcShadowGetProperty(Values, "grid"), Grid))
		return;

	if (IsInclude)
	{
		Include(Values, Frame, Grid, Result);
		return;
	}

	Result.HasDefinitions = true;
	Item.Data.Group = lcShadowGetProperty(Values, "group");
	if (!Id.isEmpty())
		Item.Ids.push_back(Id);

	const QByteArray Scale = lcShadowGetProperty(Values, "scale").toLower();
	if (Scale == "yonly")
		Item.Scaling = lcShadowScalePolicy::Axial;
	else if (Scale == "ronly")
		Item.Scaling = lcShadowScalePolicy::Radial;
	else if (Scale == "yandr")
		Item.Scaling = lcShadowScalePolicy::AxialOrRadial;

	const QByteArray Mirror = lcShadowGetProperty(Values, "mirror", Kind == "SNAP_CYL" ? "cor" : "none").toLower();
	if (Mirror == "cor" || Mirror == "corx")
		Item.Mirroring = lcShadowMirrorPolicy::CorrectX;
	else if (Mirror == "corz")
		Item.Mirroring = lcShadowMirrorPolicy::CorrectZ;

	lcMatrix44 Orientation = Frame;
	Orientation.SetTranslation(lcVector3(0.0f, 0.0f, 0.0f));
	if (lcShadowTransformConnection(Item, Orientation, lcShadowTransformKind::Declaration))
		PlaceGrid(Grid, Frame, &Item, 1, Result);
}

void lcShadowConnectionResolver::ReadShadow(lcFile& File, lcShadowConnectionRecords& Result, bool AllowIncludes)
{
	char Buffer[4096];
	while (!mLibrary.ShouldCancelLoading() && File.ReadLine(Buffer, sizeof(Buffer)))
		ReadDirective(QByteArray(Buffer).simplified(), Result, AllowIncludes);
}

std::vector<QByteArray> lcShadowConnectionResolver::ReadGeometryLines(lcFile& File) const
{
	std::vector<QByteArray> Lines;
	std::vector<bool> TexmapFallback;
	size_t FallbackDepth = 0;
	char Buffer[4096];

	while (!mLibrary.ShouldCancelLoading() && File.ReadLine(Buffer, sizeof(Buffer)))
	{
		QByteArray Line = QByteArray(Buffer).simplified();
		// Alternate TEXMAP geometry can repeat references from the primary definition.
		if (Line.startsWith("0 !TEXMAP "))
		{
			if (Line.startsWith("0 !TEXMAP START "))
				TexmapFallback.push_back(false);
			else if (Line == "0 !TEXMAP FALLBACK" && !TexmapFallback.empty() && !TexmapFallback.back())
			{
				TexmapFallback.back() = true;
				FallbackDepth++;
			}
			else if (Line == "0 !TEXMAP END" && !TexmapFallback.empty())
			{
				FallbackDepth -= TexmapFallback.back();
				TexmapFallback.pop_back();
			}
			continue;
		}

		if (FallbackDepth)
			continue;

		if (Line.startsWith("0 !: "))
			Line.remove(0, 5);

		if (Line.startsWith("1 ") || Line.left(14).toUpper() == "0 !LDCAD SNAP_")
			Lines.emplace_back(std::move(Line));
	}

	return Lines;
}

struct lcShadowGeometrySource
{
	lcShadowGeometrySource(const QString& FilePath, std::vector<QByteArray> SourceLines, const lcMatrix44& ParentTransform)
		: Path(FilePath), Lines(std::move(SourceLines)), Transform(ParentTransform)
	{
	}

	QString Path;
	std::vector<QByteArray> Lines;
	lcMatrix44 Transform;
	lcShadowConnectionRecords Connections;
	size_t NextLine = 0;
};

lcShadowConnectionRecords lcShadowConnectionResolver::ReadGeometry(lcFile& File, const QString& Path)
{
	std::map<QString, lcShadowConnectionRecords> References;
	std::set<QString> Active{ Path };
	std::vector<lcShadowGeometrySource> Stack;
	Stack.emplace_back(Path, ReadGeometryLines(File), lcMatrix44Identity());

	while (!mLibrary.ShouldCancelLoading() && !Stack.empty())
	{
		lcShadowGeometrySource& Current = Stack.back();

		if (Current.NextLine == Current.Lines.size())
		{
			lcMemFile Shadow;
			if (mLibrary.ReadShadowFile(Current.Path, Shadow))
				ReadShadow(Shadow, Current.Connections, true);

			if (Stack.size() == 1)
				return std::move(Current.Connections);

			const lcMatrix44 Transform = Current.Transform;
			const auto Entry = References.try_emplace(Current.Path, std::move(Current.Connections)).first;
			Active.erase(Current.Path);
			Stack.pop_back();
			Append(Stack.back().Connections, Entry->second, Transform, QByteArray(), QByteArray(), lcShadowTransformKind::Reference);
			continue;
		}

		const QByteArray& Line = Current.Lines[Current.NextLine++];

		if (!Line.startsWith("1 "))
		{
			ReadDirective(Line, Current.Connections, true);
			continue;
		}

		const lcShadowWords Tokens = lcShadowSplitWords(Line);
		float Position[3], Orientation[9];
		if (Tokens.size() < 15 || !lcShadowReadNumbers(Tokens, 2, 3, Position) || !lcShadowReadNumbers(Tokens, 5, 9, Orientation))
			continue;

		const QString Reference = lcShadowNormalizeReference(QString::fromLatin1(Tokens[14]));
		if (Reference.isEmpty())
			continue;

		const QByteArray Name = Reference.toLatin1();
		const lcLibraryPrimitive* Primitive = mLibrary.FindPrimitive(Name.constData());
		const QString ChildPath = (Primitive && !Primitive->mSubFile ? QStringLiteral("P/") : QStringLiteral("PARTS/")) + Reference;
		if (Active.count(ChildPath))
			continue;

		const lcMatrix44 Transform = lcShadowMakeFrame(Position, Orientation);
		const auto Cached = References.find(ChildPath);
		if (Cached != References.end())
		{
			Append(Current.Connections, Cached->second, Transform, QByteArray(), QByteArray(), lcShadowTransformKind::Reference);
			continue;
		}

		lcResult<lcOpenedLibraryFile> Child = Primitive ? mLibrary.OpenPrimitiveFile(Primitive) : mLibrary.OpenPieceDataFile(Name.constData());
		if (!Child)
		{
			References.try_emplace(ChildPath);
			continue;
		}

		Active.insert(ChildPath);
		Stack.emplace_back(ChildPath, ReadGeometryLines(*Child.value().File), Transform);
	}

	return lcShadowConnectionRecords();
}

lcShadowConnectionData lcLoadShadowConnections(lcPiecesLibrary& Library, lcFile& File, const QString& ShadowPath)
{
	File.Seek(0, SEEK_SET);
	return lcShadowConnectionResolver(Library).Resolve(File, ShadowPath);
}

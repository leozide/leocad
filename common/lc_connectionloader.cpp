#include "lc_global.h"
#include "lc_connectionloader.h"
#include "lc_file.h"
#include "lc_library.h"
#include "lc_meshloader.h"

lcConnectionLoader::lcConnectionLoader(const lcPiecesLibrary& Library)
	: mLibrary(Library)
{
}

lcConnections lcConnectionLoader::Load(lcFile& File, const QString& FilePath, const std::set<std::string>& UnavailableTextures)
{
	File.Seek(0, SEEK_SET);
	
	lcConnectionRecords ConnectionRecords = ReadGeometry(File, NormalizeReference(FilePath), UnavailableTextures);
	lcConnections Connections;

	Connections.reserve(ConnectionRecords.size());

	for (lcConnectionRecord& Item : ConnectionRecords)
	{
		lcConnection& Connection = Item.Data;
		
		Connection.Frame = lcMatrix44LDrawToLeoCAD(Connection.Frame);
		Connection.Scale = lcVector3Abs(lcVector3LDrawToLeoCAD(Connection.Scale));

		lcCylinderConnection* Cylinder = std::get_if<lcCylinderConnection>(&Connection.Shape);
		
		if (Cylinder && Connection.Group.empty() && Cylinder->Sections.size() == 1 && !Cylinder->Centered)
		{
			const lcCylinderConnection::Section& Section = Cylinder->Sections.front();
			using Profile = lcCylinderConnection::SectionProfile;
			using Caps = lcCylinderConnection::CylinderCaps;
			const bool StandardStudSize = fabsf(Section.Radius * Connection.Scale.x - 6.0f) < 0.01f &&
				fabsf(Section.Radius * Connection.Scale.y - 6.0f) < 0.01f;
			const bool MaleStud = Cylinder->Gender == lcConnectionGender::Male && Cylinder->Caps == Caps::A &&
				Section.Profile == Profile::Round && fabsf(Section.Length * Connection.Scale.z - 4.0f) < 0.01f;
			const bool FemaleReceiver = Cylinder->Gender == lcConnectionGender::Female && Cylinder->Caps == Caps::B &&
				(Section.Profile == Profile::Round || Section.Profile == Profile::Square);
			
			if (StandardStudSize && MaleStud)
				Cylinder->StudType = lcStudType::Male;
			else if (StandardStudSize && FemaleReceiver)
				Cylinder->StudType = lcStudType::Female;
		}

		lcGenericConnection* Generic = std::get_if<lcGenericConnection>(&Connection.Shape);
		
		if (Generic && Generic->BoundsShape == lcGenericConnection::Bounds::Box)
			Generic->Dimensions = lcVector3Abs(lcVector3LDrawToLeoCAD(Generic->Dimensions));

		Connections.emplace_back(std::move(Connection));
	}

	return Connections;
}

lcConnectionLoader::lcConnectionTokens lcConnectionLoader::SplitWords(const QByteArray& Text)
{
	return Text.isEmpty() ? lcConnectionTokens() : Text.simplified().split(' ');
}

QByteArray lcConnectionLoader::GetProperty(const lcConnectionProperties& Values, const char* Key, const char* Default)
{
	const lcConnectionProperties::const_iterator Entry = Values.find(Key);
	
	return Entry == Values.end() ? QByteArray(Default) : Entry->second;
}

bool lcConnectionLoader::ReadProperties(const QByteArray& Text, int Offset, lcConnectionProperties& Values)
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

bool lcConnectionLoader::ReadNumber(const QByteArray& Text, float& Number)
{
	bool Ok;
	Number = Text.toFloat(&Ok);
	return Ok && std::isfinite(Number);
}

bool lcConnectionLoader::ReadNumbers(const lcConnectionTokens& Tokens, int Offset, int Count, float* Numbers)
{
	if (Tokens.size() - Offset < Count)
		return false;

	for (int Index = 0; Index < Count; Index++)
		if (!ReadNumber(Tokens[Offset + Index], Numbers[Index]))
			return false;

	return true;
}

lcMatrix44 lcConnectionLoader::MakeFrame(const float (&Position)[3], const float (&Orientation)[9])
{
	return lcMatrix44(lcVector4(Orientation[0], Orientation[3], Orientation[6], 0.0f),
		lcVector4(Orientation[1], Orientation[4], Orientation[7], 0.0f),
		lcVector4(Orientation[2], Orientation[5], Orientation[8], 0.0f),
		lcVector4(Position[0], Position[1], Position[2], 1.0f));
}

bool lcConnectionLoader::ReadFrame(const lcConnectionProperties& Values, lcMatrix44& Frame)
{
	const lcConnectionTokens Position = SplitWords(GetProperty(Values, "pos", "0 0 0"));
	const lcConnectionTokens Orientation = SplitWords(GetProperty(Values, "ori", "1 0 0 0 1 0 0 0 1"));
	float Offset[3], Basis[9];

	if (Position.size() != 3 || Orientation.size() != 9 ||
		!ReadNumbers(Position, 0, 3, Offset) || !ReadNumbers(Orientation, 0, 9, Basis))
		return false;

	Frame = MakeFrame(Offset, Basis);
	
	return true;
}

bool lcConnectionLoader::ReadGrid(const QByteArray& Text, lcConnectionGrid& Grid)
{
	const lcConnectionTokens Tokens = SplitWords(Text);
	
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

		if (Offset == Tokens.size() || !ReadNumber(Tokens[Offset++], Grid.Step[Axis]))
			return false;
	}

	return Offset == Tokens.size();
}

lcConnectionGender lcConnectionLoader::ReadGender(const lcConnectionProperties& Values, const char* Key)
{
	return GetProperty(Values, Key).toUpper() == "F" ? lcConnectionGender::Female : lcConnectionGender::Male;
}

bool lcConnectionLoader::ReadCylinder(const lcConnectionProperties& Values, lcConnectionRecord& Item)
{
	const lcConnectionTokens Sections = SplitWords(GetProperty(Values, "secs"));

	if (Sections.empty() || Sections.size() % 3)
		return false;

	lcCylinderConnection Cylinder;
	using Profile = lcCylinderConnection::SectionProfile;
	static const std::map<QByteArray, Profile> Profiles = {
		{ "R", Profile::Round }, { "A", Profile::Axle }, { "S", Profile::Square },
		{ "_L", Profile::FlexPrevious }, { "L_", Profile::FlexNext }
	};

	for (int Offset = 0; Offset < Sections.size(); Offset += 3)
	{
		const auto Shape = Profiles.find(Sections[Offset].toUpper());
		float Dimensions[2];
		
		if (Shape == Profiles.end() || !ReadNumbers(Sections, Offset + 1, 2, Dimensions) || Dimensions[0] < 0.0f || Dimensions[1] < 0.0f)
			return false;

		Cylinder.Sections.push_back({ Shape->second, Dimensions[0], Dimensions[1] });
	}

	Cylinder.Gender = ReadGender(Values, "gender");
	Cylinder.Centered = GetProperty(Values, "center").toLower() == "true";
	Cylinder.Sliding = GetProperty(Values, "slide").toLower() == "true";
	using Caps = lcCylinderConnection::CylinderCaps;
	const QByteArray Ends = GetProperty(Values, "caps", "one").toLower();
	
	if (Ends == "none")
		Cylinder.Caps = Caps::None;
	else if (Ends == "two")
		Cylinder.Caps = Caps::Both;
	else if (Ends == "a")
		Cylinder.Caps = Caps::A;
	else if (Ends == "b" || (Ends == "one" && Cylinder.Gender == lcConnectionGender::Female))
		Cylinder.Caps = Caps::B;
	else if (Ends != "one")
		return false;

	Item.Data.Shape = std::move(Cylinder);
	return true;
}

bool lcConnectionLoader::ReadGeneric(const QByteArray& Kind, const lcConnectionProperties& Values, lcConnectionRecord& Item)
{
	const bool LegacySphere = Kind == "SNAP_SPH";
	const lcConnectionTokens Bounds = SplitWords(LegacySphere
		? QByteArray("sph ") + GetProperty(Values, "radius")
		: GetProperty(Values, "bounding", "pnt"));
	if (Bounds.empty())
		return false;

	using Shape = lcGenericConnection::Bounds;
	static const std::map<QByteArray, std::pair<Shape, int>> Shapes = {
		{ "pnt", { Shape::Point, 0 } }, { "box", { Shape::Box, 3 } }, { "cube", { Shape::Cube, 1 } },
		{ "cyl", { Shape::Cylinder, 2 } }, { "sph", { Shape::Sphere, 1 } }
	};
	const auto BoundingShape = Shapes.find(Bounds.front().toLower());
	float Dimensions[3]{ 0.0f, 0.0f, 0.0f };
	
	if (BoundingShape == Shapes.end() || Bounds.size() != BoundingShape->second.second + 1 ||
		!ReadNumbers(Bounds, 1, BoundingShape->second.second, Dimensions) ||
		std::any_of(std::begin(Dimensions), std::end(Dimensions), [](float Value) { return Value < 0.0f; }))
		return false;

	lcGenericConnection Generic;
	
	Generic.BoundsShape = BoundingShape->second.first;
	Generic.Dimensions = lcVector3(Dimensions[0], Dimensions[1], Dimensions[2]);
	Generic.Gender = ReadGender(Values, "gender");
	Generic.MatchRule = GetProperty(Values, "match", LegacySphere ? "size" : "shape").toLower().toStdString();
	Generic.PlacementMode = GetProperty(Values, "placement", LegacySphere ? "free" : "").toLower().toStdString();
	Item.Data.Shape = std::move(Generic);
	
	return true;
}

bool lcConnectionLoader::ReadShape(const QByteArray& Kind, const lcConnectionProperties& Values, lcConnectionRecord& Item)
{
	if (Kind == "SNAP_CYL")
		return ReadCylinder(Values, Item);

	if (Kind == "SNAP_CLP")
	{
		lcClipConnection Clip;
		
		if (!ReadNumber(GetProperty(Values, "radius", "4"), Clip.Radius) || Clip.Radius < 0.0f ||
			!ReadNumber(GetProperty(Values, "length", "8"), Clip.Length) || Clip.Length < 0.0f)
			return false;

		Clip.Centered = GetProperty(Values, "center").toLower() == "true";
		Clip.Sliding = GetProperty(Values, "slide").toLower() == "true";
		Item.Data.Shape = Clip;
		return true;
	}

	if (Kind == "SNAP_FGR")
	{
		const lcConnectionTokens Sequence = SplitWords(GetProperty(Values, "seq"));
		lcFingersConnection Fingers;
		
		Fingers.Widths.resize(Sequence.size());
		
		if (Sequence.empty() || !ReadNumbers(Sequence, 0, Sequence.size(), Fingers.Widths.data()) ||
			!ReadNumber(GetProperty(Values, "radius", "0"), Fingers.Radius) || Fingers.Radius < 0.0f ||
			std::any_of(Fingers.Widths.begin(), Fingers.Widths.end(), [](float Width) { return Width <= 0.0f; }))
			return false;

		Fingers.Centered = GetProperty(Values, "center", "true").toLower() == "true";
		Fingers.FirstGender = ReadGender(Values, "genderofs");
		Item.Data.Shape = std::move(Fingers);
		
		return true;
	}

	return (Kind == "SNAP_GEN" || Kind == "SNAP_SPH") && ReadGeneric(Kind, Values, Item);
}

bool lcConnectionLoader::TransformRecord(lcConnectionRecord& Item, const lcMatrix44& Parent, lcConnectionTransformKind Kind)
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

	if (Kind == lcConnectionTransformKind::Declaration)
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
	
	if (Kind == lcConnectionTransformKind::Reference)
	{
		if (ScaledAxially && (ScaledRadially || (Item.Scaling != lcConnectionScalePolicy::Axial && Item.Scaling != lcConnectionScalePolicy::AxialOrRadial)))
			return false;

		if (ScaledRadially && (fabsf(Scale.x - Scale.z) > Tolerance ||
			(Item.Scaling != lcConnectionScalePolicy::Radial && Item.Scaling != lcConnectionScalePolicy::AxialOrRadial)))
			return false;

		if (!ScaledAxially)
			Scale.y = 1.0f;

		if (!ScaledRadially)
			Scale.x = Scale.z = 1.0f;
	}

	// Mirroring policy applies to the parent transform. The authored frame
	// can itself be reflected, including when inherited through an identity.
	if (Kind != lcConnectionTransformKind::Declaration && Parent.Determinant() < 0.0f)
	{
		if (Item.Mirroring == lcConnectionMirrorPolicy::None)
			return false;

		const int Axis = Item.Mirroring == lcConnectionMirrorPolicy::CorrectZ ? 2 : 0;
		Axes[Axis] = Axes[Axis] * -1.0f;
	}

	Item.Data.Frame = lcMatrix44(lcVector4(Axes[0], 0.0f), lcVector4(Axes[1], 0.0f), lcVector4(Axes[2], 0.0f), Frame[3]);
	Item.Data.Scale = lcVector3(Item.Data.Scale.x * Scale.x, Item.Data.Scale.y * Scale.y, Item.Data.Scale.z * Scale.z);
	
	return std::isfinite(Item.Data.Scale.x) && std::isfinite(Item.Data.Scale.y) && std::isfinite(Item.Data.Scale.z);
}

QString lcConnectionLoader::NormalizeReference(QString Path)
{
	Path.replace(QLatin1Char('\\'), QLatin1Char('/'));
	Path = QDir::cleanPath(Path).toUpper();

	if (Path == QLatin1String("..") || Path.startsWith(QLatin1String("../")) || QDir::isAbsolutePath(Path) || Path.contains(QLatin1Char(':')))
		return QString();

	return Path;
}

void lcConnectionLoader::Append(lcConnectionRecords& Result, const lcConnectionRecords& Included, const lcMatrix44& Frame,
	const QByteArray& Id, const QByteArray& Slide, lcConnectionTransformKind Kind)
{
	for (const lcConnectionRecord& Source : Included)
	{
		if (mLibrary.ShouldCancelLoading())
			return;

		lcConnectionRecord Item = Source;
		
		if (!TransformRecord(Item, Frame, Kind))
			continue;

		if (!Id.isEmpty())
			Item.Ids.push_back(Id);

		if (!Slide.isEmpty())
		{
			const bool Sliding = Slide.toLower() == "true";
			
			if (lcCylinderConnection* Cylinder = std::get_if<lcCylinderConnection>(&Item.Data.Shape))
				Cylinder->Sliding = Sliding;
			else if (lcClipConnection* Clip = std::get_if<lcClipConnection>(&Item.Data.Shape))
				Clip->Sliding = Sliding;
		}

		Result.emplace_back(std::move(Item));
	}
}

void lcConnectionLoader::Include(const lcConnectionProperties& Values, const lcMatrix44& Frame, const lcConnectionGrid& Grid, lcConnectionRecords& Result)
{
	if (mLibrary.ShouldCancelLoading())
		return;

	const QString Ref = NormalizeReference(QString::fromLatin1(GetProperty(Values, "ref")));
	
	if (Ref.isEmpty() || Ref == QLatin1String("."))
		return;

	const lcConnectionTokens ScaleWords = SplitWords(GetProperty(Values, "scale", "1 1 1"));
	float Scale[3];
	
	if (ScaleWords.size() != 3 || !ReadNumbers(ScaleWords, 0, 3, Scale))
		return;

	auto [Entry, Inserted] = mConnectionIncludes.try_emplace(Ref);
	
	if (Inserted)
	{
		lcMemFile ConnectionFile;
		
		if (Ref.startsWith(QLatin1String("P/")) || Ref.startsWith(QLatin1String("PARTS/")))
		{
			if (!mLibrary.ReadShadowFile(Ref, ConnectionFile))
				return;
		}
		else if (Ref.startsWith(QLatin1String("S/")))
		{
			if (!mLibrary.ReadShadowFile(QStringLiteral("parts/") + Ref, ConnectionFile))
				return;
		}
		else if (!mLibrary.ReadShadowFile(QStringLiteral("p/") + Ref, ConnectionFile) &&
			!mLibrary.ReadShadowFile(QStringLiteral("parts/") + Ref, ConnectionFile))
			return;

		// Included directives are evaluated in isolation. CLEAR in an included
		// file only affects that file's records, and nested INCL is skipped.
		ReadConnectionDirectives(ConnectionFile, Entry->second, false);
	}

	lcMatrix44 Transform = Frame;
	
	for (int Axis = 0; Axis < 3; Axis++)
		Transform[Axis] = Transform[Axis] * Scale[Axis];

	// Grid positions share the same orientation, scale and mirror eligibility.
	Transform.SetTranslation(lcVector3(0.0f, 0.0f, 0.0f));

	const QByteArray Id = GetProperty(Values, "id");
	const QByteArray Slide = GetProperty(Values, "slide");
	lcConnectionRecords Transformed;
	
	Append(Transformed, Entry->second, Transform, Id, Slide, lcConnectionTransformKind::Include);

	PlaceGrid(Grid, Frame, Transformed.data(), Transformed.size(), Result);
}

void lcConnectionLoader::PlaceGrid(const lcConnectionGrid& Grid, const lcMatrix44& Frame,
	const lcConnectionRecord* Connections, size_t Count, lcConnectionRecords& Result)
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

					const lcConnectionRecord& Source = Connections[Index];
					const lcVector3 Position = Source.Data.Frame.GetTranslation() + Translation;

					if (!std::isfinite(Position.x) || !std::isfinite(Position.y) || !std::isfinite(Position.z))
						continue;

					Result.push_back(Source);
					Result.back().Data.Frame.SetTranslation(Position);
				}
			}
}

void lcConnectionLoader::ReadConnectionDirective(const QByteArray& Line, lcConnectionRecords& Result, bool AllowIncludes)
{
	if (!Line.startsWith("0 ") || Line.left(14).toUpper() != "0 !LDCAD SNAP_")
		return;

	int End = Line.indexOf(' ', 9);
	if (End < 0)
		End = Line.size();

	const QByteArray Kind = Line.mid(9, End - 9).toUpper();
	lcConnectionProperties Values;
	
	if (!ReadProperties(Line, End, Values))
		return;

	const QByteArray Id = GetProperty(Values, "id");
	
	if (Kind == "SNAP_CLEAR")
	{
		if (Id.isEmpty())
			Result.clear();
		else
			Result.erase(std::remove_if(Result.begin(), Result.end(), [&Id](const lcConnectionRecord& Item)
			{
				return std::find(Item.Ids.begin(), Item.Ids.end(), Id) != Item.Ids.end();
			}), Result.end());
		return;
	}

	const bool IsInclude = Kind == "SNAP_INCL";
	lcConnectionRecord Item;

	if ((IsInclude && !AllowIncludes) || (!IsInclude && !ReadShape(Kind, Values, Item)))
		return;

	lcMatrix44 Frame;
	lcConnectionGrid Grid;
	
	if (!ReadFrame(Values, Frame) || !ReadGrid(GetProperty(Values, "grid"), Grid))
		return;

	if (IsInclude)
	{
		Include(Values, Frame, Grid, Result);
		return;
	}

	Item.Data.Group = GetProperty(Values, "group").toStdString();
	
	if (!Id.isEmpty())
		Item.Ids.push_back(Id);

	const QByteArray Scale = GetProperty(Values, "scale").toLower();
	
	if (Scale == "yonly")
		Item.Scaling = lcConnectionScalePolicy::Axial;
	else if (Scale == "ronly")
		Item.Scaling = lcConnectionScalePolicy::Radial;
	else if (Scale == "yandr")
		Item.Scaling = lcConnectionScalePolicy::AxialOrRadial;

	const QByteArray Mirror = GetProperty(Values, "mirror", Kind == "SNAP_CYL" ? "cor" : "none").toLower();
	
	if (Mirror == "cor" || Mirror == "corx")
		Item.Mirroring = lcConnectionMirrorPolicy::CorrectX;
	else if (Mirror == "corz")
		Item.Mirroring = lcConnectionMirrorPolicy::CorrectZ;

	lcMatrix44 Orientation = Frame;
	
	Orientation.SetTranslation(lcVector3(0.0f, 0.0f, 0.0f));
	
	if (TransformRecord(Item, Orientation, lcConnectionTransformKind::Declaration))
		PlaceGrid(Grid, Frame, &Item, 1, Result);
}

void lcConnectionLoader::ReadConnectionDirectives(lcFile& File, lcConnectionRecords& Result, bool AllowIncludes)
{
	char Buffer[4096]; // The longest line measured in the bundled libraries was about 400 bytes.
	
	while (!mLibrary.ShouldCancelLoading() && File.ReadLine(Buffer, sizeof(Buffer)))
		ReadConnectionDirective(QByteArray(Buffer).simplified(), Result, AllowIncludes);
}

std::vector<lcConnectionLoader::lcGeometryLine> lcConnectionLoader::ReadGeometryLines(lcFile& File, lcGeometryTexture InheritedTexture,
	const std::set<std::string>& UnavailableTextures) const
{
	std::vector<lcGeometryLine> Lines;
	std::vector<lcMeshLoaderTextureMap> TextureStack;
	QString TextureError;
	const lcMatrix44 Identity = lcMatrix44Identity();
	char Buffer[4096]; // The longest line measured in the bundled libraries was about 400 bytes.

	while (!mLibrary.ShouldCancelLoading() && File.ReadLine(Buffer, sizeof(Buffer)))
	{
		int LineType;
		if (sscanf(Buffer, "%d", &LineType) != 1)
			continue;

		if (LineType != 0 && LineType != 1 && (TextureStack.empty() || !TextureStack.back().Next))
			continue;

		QByteArray Line = QByteArray(Buffer).simplified();
		if (LineType == 0)
		{
			if (!TextureStack.empty() && TextureStack.back().Next)
				TextureStack.pop_back();

			char Meta[16];
			int Offset = 0;
			if (sscanf(Buffer, "%*d %15s%n", Meta, &Offset) == 1 && !strcmp(Meta, "!TEXMAP"))
			{
				if (!lcMeshLoader::ApplyTextureMapCommand(Buffer + Offset, Identity, TextureStack, 0, UnavailableTextures, TextureError))
					return std::vector<lcGeometryLine>();

				continue;
			}

			if (Line.startsWith("0 !: ") && (!TextureStack.empty() || InheritedTexture != lcGeometryTexture::None))
				Line.remove(0, 5);
			else
			{
				if (Line.left(14).toUpper() == "0 !LDCAD SNAP_" && lcMeshLoader::IsTextureBranchActive(TextureStack, UnavailableTextures))
					Lines.push_back({ std::move(Line), lcGeometryTexture::None });

				continue;
			}
		}

		int Color;
		if (sscanf(Line.constData(), "%d %d", &LineType, &Color) != 2 || LineType < 1 || LineType > 5)
			continue;

		if (LineType == 1 && lcMeshLoader::IsTextureBranchActive(TextureStack, UnavailableTextures))
		{
			lcGeometryTexture Texture = InheritedTexture;
			if (!TextureStack.empty())
				Texture = TextureStack.back().InFallback ? lcGeometryTexture::Fallback : lcGeometryTexture::Mapped;

			Lines.push_back({ std::move(Line), Texture });
		}

		if (!TextureStack.empty() && TextureStack.back().Next)
		{
			if (!lcMeshLoader::FinishTextureMap(TextureStack.back(), UnavailableTextures, TextureError))
				return std::vector<lcGeometryLine>();

			TextureStack.pop_back();
		}
	}

	for (const lcMeshLoaderTextureMap& Map : TextureStack)
		if (!lcMeshLoader::FinishTextureMap(Map, UnavailableTextures, TextureError))
			return std::vector<lcGeometryLine>();

	return Lines;
}

lcConnectionLoader::lcConnectionRecords lcConnectionLoader::ReadGeometry(lcFile& File, const QString& Path, const std::set<std::string>& UnavailableTextures)
{
	std::map<lcGeometryReference, lcConnectionRecords> References;
	std::set<QString> Active{ Path };
	std::vector<lcPieceGeometrySource> Stack;
	
	const lcGeometryReference Root(Path, lcGeometryTexture::None);
	Stack.emplace_back(Path, Root, ReadGeometryLines(File, Root.second, UnavailableTextures), lcMatrix44Identity());

	while (!mLibrary.ShouldCancelLoading() && !Stack.empty())
	{
		lcPieceGeometrySource& Current = Stack.back();

		if (Current.NextLine == Current.Lines.size())
		{
			lcMemFile ConnectionFile;
			if (mLibrary.ReadShadowFile(Current.Path, ConnectionFile))
				ReadConnectionDirectives(ConnectionFile, Current.Connections, true);

			if (Stack.size() == 1)
				return std::move(Current.Connections);

			const lcMatrix44 Transform = Current.Transform;
			const auto Entry = References.try_emplace(Current.Reference, std::move(Current.Connections)).first;
			
			Active.erase(Current.Path);
			Stack.pop_back();
			Append(Stack.back().Connections, Entry->second, Transform, QByteArray(), QByteArray(), lcConnectionTransformKind::Reference);
			
			continue;
		}

		const lcGeometryLine& GeometryLine = Current.Lines[Current.NextLine++];
		const QByteArray& Line = GeometryLine.Text;

		if (!Line.startsWith("1 "))
		{
			ReadConnectionDirective(Line, Current.Connections, true);
			continue;
		}

		const lcConnectionTokens Tokens = SplitWords(Line);
		float Position[3], Orientation[9];
		
		if (Tokens.size() < 15 || !ReadNumbers(Tokens, 2, 3, Position) || !ReadNumbers(Tokens, 5, 9, Orientation))
			continue;

		const QString Reference = NormalizeReference(QString::fromLatin1(Tokens[14]));
		
		if (Reference.isEmpty())
			continue;

		const lcGeometryReference Key(Reference, GeometryLine.Texture);
		const lcMatrix44 Transform = MakeFrame(Position, Orientation);
		const auto Cached = References.find(Key);

		if (Cached != References.end())
		{
			Append(Current.Connections, Cached->second, Transform, QByteArray(), QByteArray(), lcConnectionTransformKind::Reference);
			continue;
		}

		const QByteArray Name = Reference.toLatin1();
		const lcLibraryPrimitive* Primitive = GeometryLine.Texture == lcGeometryTexture::Mapped ? nullptr : mLibrary.FindPrimitive(Name.constData());
		lcResult<lcOpenedLibraryFile> Child = Primitive ? mLibrary.OpenPrimitiveFile(Primitive) : mLibrary.OpenPieceDataFile(Name.constData());

		if (!Child)
		{
			References.try_emplace(Key);
			continue;
		}

		QString ChildPath = NormalizeReference(mLibrary.GetShadowPath(Child.value().Source));
		if (ChildPath.isEmpty())
			ChildPath = (Primitive && !Primitive->mSubFile ? QStringLiteral("P/") : QStringLiteral("PARTS/")) + Reference;

		if (!Active.insert(ChildPath).second)
			continue;

		// Shared primitives are parsed independently of the calling texture scope.
		const lcGeometryTexture ChildTexture = Primitive && !Primitive->mSubFile ? lcGeometryTexture::None : GeometryLine.Texture;
		Stack.emplace_back(ChildPath, Key, ReadGeometryLines(*Child.value().File, ChildTexture, UnavailableTextures), Transform);
	}

	return lcConnectionRecords();
}


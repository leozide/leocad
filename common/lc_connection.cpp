#include "lc_global.h"
#include "lc_connection.h"
#include "lc_file.h"

static constexpr float lcMatingTolerance = 0.01f;

// Ordered so that fit >= AxleInRoundHole means the section engages.
enum class lcMatingSectionFit
{
	None,            // Incompatible profiles or dimensions.
	Clearance,       // Fits without engaging.
	AxleInRoundHole, // An axle profile fits a round hole; this fit is less preferred than a pin fit.
	Contact          // Matching profiles engage.
};

static bool lcMatingMatchDimension(float First, float Second)
{
	return fabsf(First - Second) <= lcMatingTolerance;
}

static bool lcMatingMatchRadius(float First, const lcVector3& FirstScale, float Second, const lcVector3& SecondScale)
{
	return lcMatingMatchDimension(First * FirstScale.x, Second * SecondScale.x) &&
	       lcMatingMatchDimension(First * FirstScale.y, Second * SecondScale.y);
}

template<typename T, typename Length>
static std::vector<float> lcMatingGetBoundaries(const std::vector<T>& Sections, float Scale, bool Centered, Length GetLength)
{
	std::vector<float> Boundaries;

	Boundaries.reserve(Sections.size() + 1);
	Boundaries.push_back(0.0f);

	for (const T& Section : Sections)
		Boundaries.push_back(Boundaries.back() + GetLength(Section) * Scale);

	if (Centered)
	{
		const float Midpoint = Boundaries.back() * 0.5f;

		for (float& Boundary : Boundaries)
			Boundary -= Midpoint;
	}

	return Boundaries;
}

// Try translations that line up profile boundaries in either axial direction.
// A fit must engage the whole shorter profile, including any internal steps.
template<typename Fits>
static void lcMatingMatchProfiles(std::vector<lcConnectionMating>& Matings, const std::vector<float>& First, const std::vector<float>& Second,
                                  bool Reverse, Fits ProfilesFit)
{
	const float Direction = Reverse ? -1.0f : 1.0f;

	auto FirstBoundary = [&](size_t Index)
	{
		return Reverse ? -First[First.size() - 1 - Index] : First[Index];
	};

	const float Length = lcMin(First.back() - First.front(), Second.back() - Second.front());

	if (Length <= lcMatingTolerance)
		return;

	for (size_t FirstIndex = 0; FirstIndex < First.size(); FirstIndex++)
		for (float SecondBoundary : Second)
		{
			const float Offset = SecondBoundary - FirstBoundary(FirstIndex);
			const float Start = FirstBoundary(0) + Offset;
			const float End = FirstBoundary(First.size() - 1) + Offset;

			if (lcMin(End, Second.back()) - lcMax(Start, Second.front()) < Length - lcMatingTolerance)
				continue;

			if (std::any_of(Matings.begin(), Matings.end(), [&](const lcConnectionMating& Mating)
			                {
				                return Mating.Direction == Direction && lcMatingMatchDimension(Mating.SecondOffset, Offset);
			                }))
				continue;

			lcConnectionMating Mating{ 0.0f, Offset, Direction };

			if (ProfilesFit(Mating, FirstBoundary, Start, End))
				Matings.push_back(Mating);
		}
}

static lcMatingSectionFit lcMatingMatchCylinderSection(const lcCylinderConnection& Male, const lcVector3& MaleScale, size_t MaleIndex,
	const lcCylinderConnection& Female, const lcVector3& FemaleScale, size_t FemaleIndex)
{
	const lcCylinderConnection::Section& MaleSection = Male.Sections[MaleIndex];
	const lcCylinderConnection::Section& FemaleSection = Female.Sections[FemaleIndex];
	const lcCylinderConnection::Section& MaleNominal = Male.GetNominalSection(MaleIndex);
	const lcCylinderConnection::Section& FemaleNominal = Female.GetNominalSection(FemaleIndex);

	if (MaleNominal.Profile == lcCylinderConnection::SectionProfile::FlexPrevious || MaleNominal.Profile == lcCylinderConnection::SectionProfile::FlexNext ||
		FemaleNominal.Profile == lcCylinderConnection::SectionProfile::FlexPrevious || FemaleNominal.Profile == lcCylinderConnection::SectionProfile::FlexNext)
		return lcMatingSectionFit::None;

	const bool AxleInRoundHole = MaleNominal.Profile == lcCylinderConnection::SectionProfile::Axle && FemaleNominal.Profile == lcCylinderConnection::SectionProfile::Round;
	const bool StudInSquareReceiver = MaleNominal.Profile == lcCylinderConnection::SectionProfile::Round && FemaleNominal.Profile == lcCylinderConnection::SectionProfile::Square;

	if (MaleNominal.Profile != FemaleNominal.Profile && !AxleInRoundHole && !StudInSquareReceiver)
		return lcMatingSectionFit::None;

	const bool FlexibleTip = &MaleSection != &MaleNominal;
	bool Engages = true;

	for (int Axis = 0; Axis < 2; Axis++)
	{
		const float MaleMin = lcMin(MaleSection.Radius, MaleNominal.Radius) * MaleScale[Axis];
		const float MaleMax = lcMax(MaleSection.Radius, MaleNominal.Radius) * MaleScale[Axis];
		const float FemaleMin = lcMin(FemaleSection.Radius, FemaleNominal.Radius) * FemaleScale[Axis];
		const float FemaleMax = lcMax(FemaleSection.Radius, FemaleNominal.Radius) * FemaleScale[Axis];

		// A pin hole's wider counterbores clear the axle. A narrower
		// section still blocks it, and some section must provide contact.
		if (MaleMin > FemaleMax + lcMatingTolerance || (!FlexibleTip && !AxleInRoundHole && FemaleMin > MaleMax + lcMatingTolerance))
			return lcMatingSectionFit::None;

		Engages &= FemaleMin <= MaleMax + lcMatingTolerance;
	}

	if (!Engages)
		return lcMatingSectionFit::Clearance;

	return AxleInRoundHole ? lcMatingSectionFit::AxleInRoundHole : lcMatingSectionFit::Contact;
}

static std::vector<lcConnectionMating> lcMatingMatchCylinders(const lcCylinderConnection& Male, const lcVector3& MaleScale, const lcCylinderConnection& Female, const lcVector3& FemaleScale)
{
	std::vector<lcConnectionMating> Matings;

	if (Male.Gender == Female.Gender || Female.Caps == lcCylinderConnection::CylinderCaps::Both || Male.Sections.empty() || Female.Sections.empty())
		return Matings;

	const auto SectionLength = [](const lcCylinderConnection::Section& Section) { return Section.Length; };

	// For a single-section stud pair, the male base seats at the female entrance.
	if (Male.Sections.size() == 1 && Female.Sections.size() == 1 && Male.Caps == lcCylinderConnection::CylinderCaps::A && Female.Caps == lcCylinderConnection::CylinderCaps::B)
	{
		const float MaleLength = Male.Sections.front().Length * MaleScale.z;
		const float FemaleLength = Female.Sections.front().Length * FemaleScale.z;
		const lcMatingSectionFit Fit = lcMatingMatchCylinderSection(Male, MaleScale, 0, Female, FemaleScale, 0);

		if (MaleLength > lcMatingTolerance && MaleLength <= FemaleLength + lcMatingTolerance &&
			Fit >= lcMatingSectionFit::AxleInRoundHole)
			Matings.push_back({ Male.Centered ? -MaleLength * 0.5f : 0.0f, Female.Centered ? -FemaleLength * 0.5f : 0.0f,
				1.0f, 0.0f, false, Fit == lcMatingSectionFit::AxleInRoundHole });

		return Matings;
	}

	const std::vector<float> MaleEnds = lcMatingGetBoundaries(Male.Sections, MaleScale.z, Male.Centered, SectionLength);
	const std::vector<float> FemaleEnds = lcMatingGetBoundaries(Female.Sections, FemaleScale.z, Female.Centered, SectionLength);

	const float MaleLength = MaleEnds.back() - MaleEnds.front();
	const float FemaleLength = FemaleEnds.back() - FemaleEnds.front();

	// A uniform shaft can slide through a stepped bore if it fits every
	// section and fully spans the bore throughout the sliding range.
	if (Male.Sections.size() == 1 && Male.Caps == lcCylinderConnection::CylinderCaps::None && Female.Caps == lcCylinderConnection::CylinderCaps::None &&
		(Male.Sliding || Female.Sliding) && (Female.Sections.size() == 1 || MaleLength >= FemaleLength))
	{
		if (lcMin(MaleLength, FemaleLength) <= lcMatingTolerance)
			return Matings;

		bool Engages = false;
		bool AxleInRoundHole = false;

		for (size_t Index = 0; Index < Female.Sections.size(); Index++)
		{
			if (FemaleEnds[Index + 1] - FemaleEnds[Index] <= lcMatingTolerance)
				continue;

			const lcMatingSectionFit Fit = lcMatingMatchCylinderSection(Male, MaleScale, 0, Female, FemaleScale, Index);

			if (Fit == lcMatingSectionFit::None)
				return Matings;

			Engages |= Fit >= lcMatingSectionFit::AxleInRoundHole;
			AxleInRoundHole |= Fit == lcMatingSectionFit::AxleInRoundHole;
		}

		if (Engages)
		{
			const float MaleCenter = (MaleEnds.front() + MaleEnds.back()) * 0.5f;
			const float FemaleCenter = (FemaleEnds.front() + FemaleEnds.back()) * 0.5f;
			const float Travel = fabsf(MaleLength - FemaleLength) * 0.5f;

			Matings.push_back({ MaleCenter, FemaleCenter, 1.0f, Travel, MaleLength > FemaleLength, AxleInRoundHole });
			Matings.push_back({ MaleCenter, FemaleCenter, -1.0f, Travel, MaleLength > FemaleLength, AxleInRoundHole });
		}

		return Matings;
	}

	for (bool Reverse : { false, true })
	{
		const bool MaleStartCapped = Male.IsCapped(Reverse ? lcCylinderConnection::CylinderCaps::B : lcCylinderConnection::CylinderCaps::A);
		const bool MaleEndCapped = Male.IsCapped(Reverse ? lcCylinderConnection::CylinderCaps::A : lcCylinderConnection::CylinderCaps::B);

		if ((MaleStartCapped || Female.Caps == lcCylinderConnection::CylinderCaps::A) && (MaleEndCapped || Female.Caps == lcCylinderConnection::CylinderCaps::B))
			continue;

		lcMatingMatchProfiles(Matings, MaleEnds, FemaleEnds, Reverse, [&](lcConnectionMating& Mating, auto MaleBoundary, float Start, float End)
		{
			const float FemaleStart = FemaleEnds.front(), FemaleEnd = FemaleEnds.back();

			if ((Female.IsCapped(lcCylinderConnection::CylinderCaps::A) && Start < FemaleStart - lcMatingTolerance) ||
				(Female.IsCapped(lcCylinderConnection::CylinderCaps::B) && End > FemaleEnd + lcMatingTolerance) ||
				(MaleStartCapped && Start > FemaleStart + lcMatingTolerance && Start < FemaleEnd - lcMatingTolerance) ||
				(MaleEndCapped && End > FemaleStart + lcMatingTolerance && End < FemaleEnd - lcMatingTolerance))
				return false;

			const float Offset = Mating.SecondOffset;
			bool Engages = false;

			for (size_t M = 0, F = 0; M < Male.Sections.size() && F < Female.Sections.size();)
			{
				const float MaleEnd = MaleBoundary(M + 1) + Offset;

				if (lcMin(MaleEnd, FemaleEnds[F + 1]) - lcMax(MaleBoundary(M) + Offset, FemaleEnds[F]) > lcMatingTolerance)
				{
					const lcMatingSectionFit Fit = lcMatingMatchCylinderSection(Male, MaleScale, Reverse ? Male.Sections.size() - 1 - M : M, Female, FemaleScale, F);

					if (Fit == lcMatingSectionFit::None)
						return false;

					Engages |= Fit >= lcMatingSectionFit::AxleInRoundHole;
					Mating.AxleInRoundHole |= Fit == lcMatingSectionFit::AxleInRoundHole;
				}

				if (MaleEnd <= FemaleEnds[F + 1])
					M++;
				else
					F++;
			}

			return Engages;
		});
	}

	return Matings;
}

static std::vector<lcConnectionMating> lcMatingMatchClip(const lcClipConnection& Clip, const lcVector3& ClipScale, const lcCylinderConnection& Bar, const lcVector3& BarScale)
{
	std::vector<lcConnectionMating> Matings;

	if (Bar.Gender != lcConnectionGender::Male || Bar.Sections.empty())
		return Matings;

	const float Length = Clip.Length * ClipScale.z;
	const std::vector<float> Ends = lcMatingGetBoundaries(Bar.Sections, BarScale.z, Bar.Centered, [](const lcCylinderConnection::Section& Section) { return Section.Length; });

	for (size_t Start = 0; Start < Bar.Sections.size();)
	{
		size_t End = Start;

		while (End < Bar.Sections.size() && Bar.Sections[End].Profile == lcCylinderConnection::SectionProfile::Round &&
			lcMatingMatchRadius(Clip.Radius, ClipScale, Bar.Sections[End].Radius, BarScale))
			End++;

		const float Span = Ends[End] - Ends[Start];

		if (Span > lcMatingTolerance && Span >= Length - lcMatingTolerance)
		{
			const float ClipCenter = Clip.Centered ? 0.0f : Length * 0.5f;
			const float BarCenter = (Ends[Start] + Ends[End]) * 0.5f;
			const float Travel = Clip.Sliding || Bar.Sliding ? lcMax((Span - Length) * 0.5f, 0.0f) : 0.0f;

			Matings.push_back({ ClipCenter, BarCenter, 1.0f, Travel, false, false, Span * 0.5f });
			Matings.push_back({ ClipCenter, BarCenter, -1.0f, Travel, false, false, Span * 0.5f });
		}

		Start = End + 1;
	}

	return Matings;
}

static std::vector<lcConnectionMating> lcMatingMatchFingers(const lcFingersConnection& First, const lcVector3& FirstScale, const lcFingersConnection& Second, const lcVector3& SecondScale)
{
	std::vector<lcConnectionMating> Matings;

	if (First.Widths.empty() || Second.Widths.empty() || !lcMatingMatchRadius(First.Radius, FirstScale, Second.Radius, SecondScale))
		return Matings;

	const auto Width = [](float Value) { return Value; };
	const std::vector<float> FirstEnds = lcMatingGetBoundaries(First.Widths, FirstScale.z, First.Centered, Width);
	const std::vector<float> SecondEnds = lcMatingGetBoundaries(Second.Widths, SecondScale.z, Second.Centered, Width);

	for (bool Reverse : { false, true })
		lcMatingMatchProfiles(Matings, FirstEnds, SecondEnds, Reverse, [&](const lcConnectionMating& Mating, auto FirstBoundary, float, float)
		{
			const float Offset = Mating.SecondOffset;
			for (size_t A = 0, B = 0; A < First.Widths.size() && B < Second.Widths.size();)
			{
				const float FirstEnd = FirstBoundary(A + 1) + Offset;
				const float SecondEnd = SecondEnds[B + 1];
				const float FirstStart = FirstBoundary(A) + Offset;
				const float SecondStart = SecondEnds[B];
				if (lcMin(FirstEnd, SecondEnd) - lcMax(FirstStart, SecondStart) > lcMatingTolerance)
				{
					const size_t Index = Reverse ? First.Widths.size() - 1 - A : A;
					const bool FirstMale = (First.FirstGender == lcConnectionGender::Male) != (Index % 2 != 0);
					const bool SecondMale = (Second.FirstGender == lcConnectionGender::Male) != (B % 2 != 0);
					if (FirstMale == SecondMale || !lcMatingMatchDimension(FirstEnd - FirstStart, SecondEnd - SecondStart))
						return false;
				}

				if (FirstEnd <= SecondEnd)
					A++;
				else
					B++;
			}
			return true;
		});

	return Matings;
}

static bool lcMatingMatchGeneric(const lcGenericConnection& First, const lcVector3& FirstScale, const lcGenericConnection& Second, const lcVector3& SecondScale)
{
	const auto ValidRule = [](const std::string& Rule) { return Rule == "group" || Rule == "shape" || Rule == "size"; };

	if (First.Gender == Second.Gender || !ValidRule(First.MatchRule) || !ValidRule(Second.MatchRule))
		return false;

	if (First.MatchRule == "group" && Second.MatchRule == "group")
		return true;

	if (First.BoundsShape != Second.BoundsShape)
		return false;

	if (First.MatchRule != "size" && Second.MatchRule != "size")
		return true;

	const lcVector3 Delta = lcVector3Abs(First.GetScaledDimensions(FirstScale) - Second.GetScaledDimensions(SecondScale));

	return Delta.x <= lcMatingTolerance && Delta.y <= lcMatingTolerance && Delta.z <= lcMatingTolerance;
}

static std::vector<lcConnectionMating> lcMatingSwapOrder(std::vector<lcConnectionMating> Matings)
{
	for (lcConnectionMating& Mating : Matings)
	{
		std::swap(Mating.FirstOffset, Mating.SecondOffset);
		Mating.TravelOnFirst = !Mating.TravelOnFirst;
	}

	return Matings;
}

bool lcCylinderConnection::IsCapped(CylinderCaps End) const
{
	return Caps == End || Caps == CylinderCaps::Both;
}

const lcCylinderConnection::Section& lcCylinderConnection::GetNominalSection(size_t Index) const
{
	const Section& Current = Sections[Index];

	if (Current.Profile == SectionProfile::FlexPrevious && Index > 0)
		return Sections[Index - 1];

	if (Current.Profile == SectionProfile::FlexNext && Index + 1 < Sections.size())
		return Sections[Index + 1];

	return Current;
}

lcVector3 lcGenericConnection::GetScaledDimensions(const lcVector3& Scale) const
{
	using Bounds = lcGenericConnection::Bounds;
	const lcVector3& Size = Dimensions;

	switch (BoundsShape)
	{
	case Bounds::Point:
		return lcVector3(0, 0, 0);

	case Bounds::Box:
		return lcVector3(Size.x * Scale.x, Size.y * Scale.y, Size.z * Scale.z);

	case Bounds::Sphere:
	case Bounds::Cube:
		return Size.x * Scale;

	case Bounds::Cylinder:
		return lcVector3(Size.x * Scale.x, Size.x * Scale.y, Size.y * Scale.z);
	}

	return lcVector3(0, 0, 0);
}

float lcConnection::GetAxialReach() const
{
	if (const lcCylinderConnection* Cylinder = std::get_if<lcCylinderConnection>(&Shape))
	{
		float Reach = 0.0f;

		for (const lcCylinderConnection::Section& Section : Cylinder->Sections)
			Reach += Section.Length * Scale.z;

		return Cylinder->Centered ? Reach * 0.5f : Reach;
	}

	if (const lcFingersConnection* Fingers = std::get_if<lcFingersConnection>(&Shape))
	{
		float Reach = 0.0f;

		for (float Width : Fingers->Widths)
			Reach += Width * Scale.z;

		return Fingers->Centered ? Reach * 0.5f : Reach;
	}

	if (const lcClipConnection* Clip = std::get_if<lcClipConnection>(&Shape))
		return Clip->Centered ? 0.0f : Clip->Length * Scale.z * 0.5f;

	return 0.0f;
}

std::vector<lcConnectionMating> lcConnection::GetMatings(const lcConnection& Other) const
{
	const lcConnection& First = *this;
	const lcConnection& Second = Other;

	if (First.Group != Second.Group)
		return std::vector<lcConnectionMating>();

	if (const lcCylinderConnection* FirstCylinder = std::get_if<lcCylinderConnection>(&First.Shape))
	{
		if (const lcCylinderConnection* SecondCylinder = std::get_if<lcCylinderConnection>(&Second.Shape))
		{
			if (FirstCylinder->Gender == lcConnectionGender::Male)
				return lcMatingMatchCylinders(*FirstCylinder, First.Scale, *SecondCylinder, Second.Scale);
			return lcMatingSwapOrder(lcMatingMatchCylinders(*SecondCylinder, Second.Scale, *FirstCylinder, First.Scale));
		}
		if (const lcClipConnection* Clip = std::get_if<lcClipConnection>(&Second.Shape))
			return lcMatingSwapOrder(lcMatingMatchClip(*Clip, Second.Scale, *FirstCylinder, First.Scale));
	}

	if (const lcClipConnection* Clip = std::get_if<lcClipConnection>(&First.Shape))
		if (const lcCylinderConnection* Bar = std::get_if<lcCylinderConnection>(&Second.Shape))
			return lcMatingMatchClip(*Clip, First.Scale, *Bar, Second.Scale);

	if (const lcFingersConnection* FirstFingers = std::get_if<lcFingersConnection>(&First.Shape))
		if (const lcFingersConnection* SecondFingers = std::get_if<lcFingersConnection>(&Second.Shape))
			return lcMatingMatchFingers(*FirstFingers, First.Scale, *SecondFingers, Second.Scale);

	if (const lcGenericConnection* FirstGeneric = std::get_if<lcGenericConnection>(&First.Shape))
		if (const lcGenericConnection* SecondGeneric = std::get_if<lcGenericConnection>(&Second.Shape))
			if (lcMatingMatchGeneric(*FirstGeneric, First.Scale, *SecondGeneric, Second.Scale))
				return std::vector<lcConnectionMating>{ lcConnectionMating{ 0.0f, 0.0f } };

	return std::vector<lcConnectionMating>();
}

bool lcConnection::IsCompatibleWith(const lcConnection& Other) const
{
	return !GetMatings(Other).empty();
}

bool lcSaveConnections(lcFile& File, const lcConnections& Connections)
{
	if (Connections.size() > std::numeric_limits<quint32>::max())
		return false;

	File.WriteU32(static_cast<quint32>(Connections.size()));

	for (const lcConnection& Connection : Connections)
	{
		if (File.WriteFloats(Connection.Frame.GetFloats(), 16) != 16 || File.WriteFloats(Connection.Scale.GetFloats(), 3) != 3 ||
		    !File.WriteString(Connection.Group))
			return false;

		const quint8 ShapeType = static_cast<quint8>(Connection.Shape.index());

		File.WriteU8(ShapeType);

		if (const lcCylinderConnection* Cylinder = std::get_if<lcCylinderConnection>(&Connection.Shape))
		{
			if (Cylinder->Sections.size() > std::numeric_limits<quint32>::max())
				return false;

			File.WriteU32(static_cast<quint32>(Cylinder->Sections.size()));

			for (const lcCylinderConnection::Section& Section : Cylinder->Sections)
			{
				const quint8 Profile = static_cast<quint8>(Section.Profile);

				File.WriteU8(Profile);

				if (File.WriteFloats(&Section.Radius, 1) != 1 || File.WriteFloats(&Section.Length, 1) != 1)
					return false;
			}

			const quint8 Gender = static_cast<quint8>(Cylinder->Gender);
			const quint8 Caps = static_cast<quint8>(Cylinder->Caps);
			const quint8 Centered = Cylinder->Centered;
			const quint8 Sliding = Cylinder->Sliding;
			const quint8 StudType = static_cast<quint8>(Cylinder->StudType);

			File.WriteU8(Gender);
			File.WriteU8(Caps);
			File.WriteU8(Centered);
			File.WriteU8(Sliding);
			File.WriteU8(StudType);
		}
		else if (const lcClipConnection* Clip = std::get_if<lcClipConnection>(&Connection.Shape))
		{
			if (File.WriteFloats(&Clip->Radius, 1) != 1 || File.WriteFloats(&Clip->Length, 1) != 1)
				return false;

			const quint8 Centered = Clip->Centered;
			const quint8 Sliding = Clip->Sliding;

			File.WriteU8(Centered);
			File.WriteU8(Sliding);
		}
		else if (const lcFingersConnection* Fingers = std::get_if<lcFingersConnection>(&Connection.Shape))
		{
			if (Fingers->Widths.size() > std::numeric_limits<quint32>::max())
				return false;

			File.WriteU32(static_cast<quint32>(Fingers->Widths.size()));

			if ((!Fingers->Widths.empty() && File.WriteFloats(Fingers->Widths.data(), Fingers->Widths.size()) != Fingers->Widths.size()) ||
			    File.WriteFloats(&Fingers->Radius, 1) != 1)
				return false;

			const quint8 FirstGender = static_cast<quint8>(Fingers->FirstGender);
			const quint8 Centered = Fingers->Centered;

			File.WriteU8(FirstGender);
			File.WriteU8(Centered);
		}
		else if (const lcGenericConnection* Generic = std::get_if<lcGenericConnection>(&Connection.Shape))
		{
			const quint8 Bounds = static_cast<quint8>(Generic->BoundsShape);
			const quint8 Gender = static_cast<quint8>(Generic->Gender);

			File.WriteU8(Bounds);

			if (File.WriteFloats(Generic->Dimensions.GetFloats(), 3) != 3)
				return false;

			File.WriteU8(Gender);

			if (!File.WriteString(Generic->MatchRule) || !File.WriteString(Generic->PlacementMode))
				return false;
		}
		else
			return false;
	}

	return true;
}

bool lcLoadConnections(lcFile& File, lcConnections& Connections)
{
	quint32 Count;
	if (File.ReadU32(&Count, 1) != 1)
		return false;

	const long Position = File.GetPosition();

	constexpr size_t MinimumConnectionSize = 16 * sizeof(float) + 3 * sizeof(float) + sizeof(quint32) + sizeof(quint8);
	if (Position < 0 || static_cast<size_t>(Position) > File.GetLength() ||
		Count > (File.GetLength() - static_cast<size_t>(Position)) / MinimumConnectionSize)
		return false;

	Connections.clear();
	Connections.reserve(Count);

	for (quint32 Index = 0; Index < Count; Index++)
	{
		lcConnection Connection;
		quint8 ShapeType;

		if (File.ReadFloats(Connection.Frame.GetFloats(), 16) != 16 || File.ReadFloats(Connection.Scale.GetFloats(), 3) != 3 ||
		    !File.ReadString(Connection.Group) || File.ReadU8(&ShapeType, 1) != 1)
			return false;

		if (ShapeType == 0)
		{
			lcCylinderConnection Cylinder;
			quint32 SectionCount;
			if (File.ReadU32(&SectionCount, 1) != 1)
				return false;

			const long SectionPosition = File.GetPosition();
			constexpr size_t SectionSize = sizeof(quint8) + 2 * sizeof(float);
			if (SectionPosition < 0 || static_cast<size_t>(SectionPosition) > File.GetLength() ||
			    File.GetLength() - static_cast<size_t>(SectionPosition) < 5 ||
			    SectionCount > (File.GetLength() - static_cast<size_t>(SectionPosition) - 5) / SectionSize)
				return false;

			Cylinder.Sections.resize(SectionCount);
			for (lcCylinderConnection::Section& Section : Cylinder.Sections)
			{
				quint8 Profile;
				if (File.ReadU8(&Profile, 1) != 1 || Profile > static_cast<quint8>(lcCylinderConnection::SectionProfile::FlexNext) ||
				    File.ReadFloats(&Section.Radius, 1) != 1 || File.ReadFloats(&Section.Length, 1) != 1)
					return false;
				Section.Profile = static_cast<lcCylinderConnection::SectionProfile>(Profile);
			}

			quint8 Gender, Caps, Centered, Sliding, StudType;
			if (File.ReadU8(&Gender, 1) != 1 || Gender > static_cast<quint8>(lcConnectionGender::Female) ||
			    File.ReadU8(&Caps, 1) != 1 || Caps > static_cast<quint8>(lcCylinderConnection::CylinderCaps::Both) ||
			    File.ReadU8(&Centered, 1) != 1 || Centered > 1 || File.ReadU8(&Sliding, 1) != 1 || Sliding > 1 ||
			    File.ReadU8(&StudType, 1) != 1 || StudType > static_cast<quint8>(lcStudType::None))
				return false;

			Cylinder.Gender = static_cast<lcConnectionGender>(Gender);
			Cylinder.Caps = static_cast<lcCylinderConnection::CylinderCaps>(Caps);
			Cylinder.Centered = Centered != 0;
			Cylinder.Sliding = Sliding != 0;
			Cylinder.StudType = static_cast<lcStudType>(StudType);
			Connection.Shape = std::move(Cylinder);
		}
		else if (ShapeType == 1)
		{
			lcClipConnection Clip;
			quint8 Centered, Sliding;
			if (File.ReadFloats(&Clip.Radius, 1) != 1 || File.ReadFloats(&Clip.Length, 1) != 1 ||
			    File.ReadU8(&Centered, 1) != 1 || Centered > 1 || File.ReadU8(&Sliding, 1) != 1 || Sliding > 1)
				return false;
			Clip.Centered = Centered != 0;
			Clip.Sliding = Sliding != 0;
			Connection.Shape = Clip;
		}
		else if (ShapeType == 2)
		{
			lcFingersConnection Fingers;
			quint32 WidthCount;
			if (File.ReadU32(&WidthCount, 1) != 1)
				return false;

			const long WidthPosition = File.GetPosition();
			if (WidthPosition < 0 || static_cast<size_t>(WidthPosition) > File.GetLength() ||
			    File.GetLength() - static_cast<size_t>(WidthPosition) < sizeof(float) + 2 ||
			    WidthCount > (File.GetLength() - static_cast<size_t>(WidthPosition) - sizeof(float) - 2) / sizeof(float))
				return false;

			Fingers.Widths.resize(WidthCount);
			quint8 FirstGender, Centered;
			if ((!Fingers.Widths.empty() && File.ReadFloats(Fingers.Widths.data(), Fingers.Widths.size()) != Fingers.Widths.size()) ||
			    File.ReadFloats(&Fingers.Radius, 1) != 1 || File.ReadU8(&FirstGender, 1) != 1 ||
			    FirstGender > static_cast<quint8>(lcConnectionGender::Female) || File.ReadU8(&Centered, 1) != 1 || Centered > 1)
				return false;
			Fingers.FirstGender = static_cast<lcConnectionGender>(FirstGender);
			Fingers.Centered = Centered != 0;
			Connection.Shape = std::move(Fingers);
		}
		else if (ShapeType == 3)
		{
			lcGenericConnection Generic;
			quint8 Bounds, Gender;
			if (File.ReadU8(&Bounds, 1) != 1 || Bounds > static_cast<quint8>(lcGenericConnection::Bounds::Sphere) ||
			    File.ReadFloats(Generic.Dimensions.GetFloats(), 3) != 3 || File.ReadU8(&Gender, 1) != 1 ||
			    Gender > static_cast<quint8>(lcConnectionGender::Female) || !File.ReadString(Generic.MatchRule) ||
			    !File.ReadString(Generic.PlacementMode))
				return false;
			Generic.BoundsShape = static_cast<lcGenericConnection::Bounds>(Bounds);
			Generic.Gender = static_cast<lcConnectionGender>(Gender);
			Connection.Shape = std::move(Generic);
		}
		else
			return false;

		Connections.emplace_back(std::move(Connection));
	}

	return true;
}

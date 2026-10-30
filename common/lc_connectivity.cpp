#include "lc_global.h"
#include "lc_connectivity.h"

float lcConnectivity::GetDeterminant(const lcMatrix44& m)
{
	return lcDot(lcVector3(m.r[0]), lcCross(lcVector3(m.r[1]), lcVector3(m.r[2])));
}

// Whether a male shape fits into a female one running the same way, with its position at Offset
// from the female's: every part of the male inside the female is no wider than the female there,
// and at least one matching pair of sections grips, the shorter one completely inside the longer.
// Radius extensions (_L, L_) may squeeze into narrower sections.
bool lcConnectivity::FitsAt(const lcConnectionCylinder& Male, const lcConnectionCylinder& Female, float Offset)
{
	const auto IsExtension = [](const lcConnectionSection& Section)
	{
		return Section.Shape == QLatin1String("_L") || Section.Shape == QLatin1String("L_");
	};

	bool Grips = false;
	float MaleStart = Male.GetExtent().first + Offset;

	for (const lcConnectionSection& MaleSection : Male.Sections)
	{
		const float MaleEnd = MaleStart + MaleSection.Length;
		float FemaleStart = Female.GetExtent().first;

		for (const lcConnectionSection& FemaleSection : Female.Sections)
		{
			const float FemaleEnd = FemaleStart + FemaleSection.Length;
			const float Overlap = std::min(MaleEnd, FemaleEnd) - std::max(MaleStart, FemaleStart);

			if (Overlap > 0.01f)
			{
				if (MaleSection.Radius > FemaleSection.Radius + 0.01f && !IsExtension(MaleSection) && !IsExtension(FemaleSection))
					return false;

				const bool MaleInside = MaleStart > FemaleStart - 0.01f && MaleEnd < FemaleEnd + 0.01f;
				const bool FemaleInside = FemaleStart > MaleStart - 0.01f && FemaleEnd < MaleEnd + 0.01f;

				if (AreSectionsCompatible(MaleSection, FemaleSection) && (MaleInside || FemaleInside))
					Grips = true;
			}

			FemaleStart = FemaleEnd;
		}

		MaleStart = MaleEnd;
	}

	return Grips;
}

// Where a male shape can sit along a female one running the same way, as ranges of the male's
// offset from the female's position: where it fits without passing a closed end of either.
std::vector<std::pair<float, float>> lcConnectivity::GetInsertRanges(const lcConnectionCylinder& Male, const lcConnectionCylinder& Female)
{
	const auto [MaleStart, MaleEnd] = Male.GetExtent();
	const auto [FemaleStart, FemaleEnd] = Female.GetExtent();
	float Low = FemaleStart - MaleEnd;
	float High = FemaleEnd - MaleStart;

	if (Female.ClosedStart)
		Low = std::max(Low, FemaleStart - MaleStart);

	if (Female.ClosedEnd)
		High = std::min(High, FemaleEnd - MaleEnd);

	if (Male.ClosedStart)
		High = std::min(High, FemaleStart - MaleStart);

	if (Male.ClosedEnd)
		Low = std::max(Low, FemaleEnd - MaleEnd);

	if (Low > High + 0.01f)
		return {};

	// Whether the male fits only changes where a section boundary of one passes one of the other,
	// so testing those offsets and the ones halfway between them finds every range.
	std::vector<float> MaleBoundaries = { MaleStart };
	std::vector<float> FemaleBoundaries = { FemaleStart };

	for (const lcConnectionSection& Section : Male.Sections)
		MaleBoundaries.push_back(MaleBoundaries.back() + Section.Length);

	for (const lcConnectionSection& Section : Female.Sections)
		FemaleBoundaries.push_back(FemaleBoundaries.back() + Section.Length);

	std::vector<float> Offsets = { Low, High };

	for (float FemaleBoundary : FemaleBoundaries)
		for (float MaleBoundary : MaleBoundaries)
			Offsets.push_back(FemaleBoundary - MaleBoundary);

	Offsets.erase(std::remove_if(Offsets.begin(), Offsets.end(), [Low, High](float Offset) { return Offset < Low - 0.01f || Offset > High + 0.01f; }), Offsets.end());
	std::sort(Offsets.begin(), Offsets.end());
	Offsets.erase(std::unique(Offsets.begin(), Offsets.end(), [](float a, float b) { return b - a < 0.01f; }), Offsets.end());

	for (size_t Index = Offsets.size() - 1; Index > 0; Index--)
		Offsets.insert(Offsets.begin() + Index, (Offsets[Index - 1] + Offsets[Index]) / 2.0f);

	std::vector<std::pair<float, float>> Ranges;
	bool Fitted = false;

	for (float Offset : Offsets)
	{
		const bool Fits = FitsAt(Male, Female, Offset);

		if (Fits && Fitted)
			Ranges.back().second = Offset;
		else if (Fits)
			Ranges.emplace_back(Offset, Offset);

		Fitted = Fits;
	}

	return Ranges;
}

bool lcConnectivity::AreCompatible(const lcConnectionCylinder& a, const lcConnectionCylinder& b)
{
	if (a.Gender == b.Gender || a.Group.compare(b.Group, Qt::CaseInsensitive) != 0)
		return false;

	const lcConnectionCylinder& Male = a.Gender == lcConnectionGender::Male ? a : b;
	const lcConnectionCylinder& Female = a.Gender == lcConnectionGender::Male ? b : a;

	for (const lcConnectionSection& MaleSection : Male.Sections)
		for (const lcConnectionSection& FemaleSection : Female.Sections)
			if (AreSectionsCompatible(MaleSection, FemaleSection))
				return true;

	return false;
}

// A round male section also fits a square female one (most anti studs are stored as squares).
bool lcConnectivity::AreSectionsCompatible(const lcConnectionSection& Male, const lcConnectionSection& Female)
{
	return fabsf(Male.Radius - Female.Radius) < 0.01f && (Male.Shape == Female.Shape || (Male.Shape == QLatin1String("R") && Female.Shape == QLatin1String("S")));
}

// Whether two compatible shapes at their frames (LeoCAD coordinates, shapes along +Z) are on the
// same axis and seated as deep as the shapes allow.
bool lcConnectivity::IsSeated(const lcConnectionCylinder& Cylinder, const lcMatrix44& Frame, const lcConnectionCylinder& Other, const lcMatrix44& OtherFrame)
{
	const lcVector3 ShapeAxis(0.0f, 0.0f, 1.0f);
	const lcVector3 Axis = lcNormalize(lcMul30(ShapeAxis, Frame));

	if (lcDot(lcNormalize(lcMul30(ShapeAxis, OtherFrame)), Axis) < 0.99f)
		return false;

	const lcVector3 Offset = Frame.GetTranslation() - OtherFrame.GetTranslation();
	const float Along = lcDot(Offset, Axis);

	if (lcLength(Offset - Axis * Along) > 0.5f)
		return false;

	const bool IsMale = Cylinder.Gender == lcConnectionGender::Male;
	const std::vector<std::pair<float, float>> InsertRanges = IsMale ? GetInsertRanges(Cylinder, Other) : GetInsertRanges(Other, Cylinder);
	const float MaleOffset = IsMale ? Along : -Along;

	return std::any_of(InsertRanges.begin(), InsertRanges.end(), [MaleOffset](const std::pair<float, float>& Range) { return MaleOffset > Range.first - 0.5f && MaleOffset < Range.second + 0.5f; });
}

void lcConnectivity::AddToBoundingBox(const std::vector<lcConnectionCylinder>& Cylinders, const lcMatrix44& Transform, lcBoundingBox& Box)
{
	for (const lcConnectionCylinder& Cylinder : Cylinders)
	{
		const lcMatrix44 Frame = lcMul(lcMatrix44LDrawToLeoCAD(Cylinder.Transform), Transform);
		const lcVector3 Axis = lcNormalize(lcMul30(lcVector3(0.0f, 0.0f, 1.0f), Frame));
		const lcVector3 Radius(Cylinder.GetRadius(), Cylinder.GetRadius(), Cylinder.GetRadius());
		const auto [Start, End] = Cylinder.GetExtent();

		for (float Along : { Start, End })
		{
			const lcVector3 Point = Frame.GetTranslation() + Axis * Along;

			Box.Min = lcMin(Box.Min, Point - Radius);
			Box.Max = lcMax(Box.Max, Point + Radius);
		}
	}
}

bool lcConnectivity::AreConnected(const std::vector<lcConnectionCylinder>& Cylinders, const lcMatrix44& Transform, const std::vector<lcConnectionCylinder>& OtherCylinders, const lcMatrix44& OtherTransform)
{
	for (const lcConnectionCylinder& Cylinder : Cylinders)
	{
		const lcMatrix44 Frame = lcMul(lcMatrix44LDrawToLeoCAD(Cylinder.Transform), Transform);

		for (const lcConnectionCylinder& OtherCylinder : OtherCylinders)
			if (AreCompatible(Cylinder, OtherCylinder) && IsSeated(Cylinder, Frame, OtherCylinder, lcMul(lcMatrix44LDrawToLeoCAD(OtherCylinder.Transform), OtherTransform)))
				return true;
	}

	return false;
}

bool lcConnectivity::AreConnected(const std::vector<std::pair<const std::vector<lcConnectionCylinder>*, lcMatrix44>>& Shapes, const std::vector<std::pair<const std::vector<lcConnectionCylinder>*, lcMatrix44>>& OtherShapes)
{
	for (const auto& [Cylinders, Transform] : Shapes)
		for (const auto& [OtherCylinders, OtherTransform] : OtherShapes)
			if (AreConnected(*Cylinders, Transform, *OtherCylinders, OtherTransform))
				return true;

	return false;
}

#pragma once

#include "lc_math.h"
#include "piece.h"

enum class lcSynthType
{
	HOSE_FLEXIBLE,
	FLEX_SYSTEM_HOSE,
	RIBBED_HOSE,
	FLEXIBLE_AXLE,
	STRING_BRAIDED,
	SHOCK_ABSORBER,
	ACTUATOR
};

class lcLibraryMeshData;

class lcSynthInfo
{
public:
	lcSynthInfo(lcSynthType Type, float Length);
	virtual ~lcSynthInfo() = default;

	bool CanAddControlPoints() const
	{
		return mCurve;
	}

	bool IsCurve() const
	{
		return mCurve;
	}

	virtual void GetDefaultControlPoints(lcArray<lcPieceControlPoint>& ControlPoints) const = 0;
	int InsertControlPoint(lcArray<lcPieceControlPoint>& ControlPoints, const lcVector3& Start, const lcVector3& End) const;
	lcMesh* CreateMesh(const lcArray<lcPieceControlPoint>& ControlPoints) const;

protected:
	using SectionCallbackFunc = std::function<void(const lcVector3& CurvePoint, int SegmentIndex, float t)>;
	virtual void CalculateSections(const lcArray<lcPieceControlPoint>& ControlPoints, lcArray<lcMatrix44>& Sections, SectionCallbackFunc SectionCallback) const = 0;
	virtual void AddParts(lcMemFile& File, lcLibraryMeshData& MeshData, const lcArray<lcMatrix44>& Sections) const = 0;

	lcSynthType mType;
	bool mCurve = false;
	float mLength;
};

void lcSynthInit();


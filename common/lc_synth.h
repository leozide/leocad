#pragma once

#include "lc_math.h"
#include "piece.h"

class lcLibraryMeshData;

class lcSynthInfo
{
public:
	explicit lcSynthInfo(float Length);
	virtual ~lcSynthInfo() = default;

	lcSynthInfo(const lcSynthInfo&) = delete;
	lcSynthInfo(lcSynthInfo&&) = delete;
	lcSynthInfo& operator=(const lcSynthInfo&) = delete;
	lcSynthInfo& operator=(lcSynthInfo&&) = delete;

	bool CanAddControlPoints() const
	{
		return mCurve;
	}

	bool IsCurve() const
	{
		return mCurve;
	}

	bool IsUnidirectional() const
	{
		return mUnidirectional;
	}

	bool IsNondirectional() const
	{
		return mNondirectional;
	}

	virtual void GetDefaultControlPoints(lcArray<lcPieceControlPoint>& ControlPoints) const = 0;
	virtual void VerifyControlPoints(lcArray<lcPieceControlPoint>& ControlPoints) const = 0;
	int InsertControlPoint(lcArray<lcPieceControlPoint>& ControlPoints, const lcVector3& Start, const lcVector3& End) const;
	lcMesh* CreateMesh(const lcArray<lcPieceControlPoint>& ControlPoints) const;

protected:
	using SectionCallbackFunc = std::function<void(const lcVector3& CurvePoint, int SegmentIndex, float t)>;
	virtual void CalculateSections(const lcArray<lcPieceControlPoint>& ControlPoints, lcArray<lcMatrix44>& Sections, SectionCallbackFunc SectionCallback) const = 0;
	virtual void AddParts(lcMemFile& File, lcLibraryMeshData& MeshData, const lcArray<lcMatrix44>& Sections) const = 0;

	bool mCurve = false;
	bool mUnidirectional = false;
	bool mNondirectional = false;
	float mLength;
};

void lcSynthInit();


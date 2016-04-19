#ifndef _LC_SYNTH_H_
#define _LC_SYNTH_H_

#include "lc_math.h"
#include "piece.h"

enum lcSynthType
{
	LC_SYNTH_PIECE_RIBBED_HOSE,
	LC_SYNTH_PIECE_FLEXIBLE_AXLE,
	LC_SYNTH_PIECE_STRING_BRAIDED
};

struct lcSynthComponent
{
	lcMatrix44 Transform;
	float Length;
};

class lcLibraryMeshData;

class lcSynthInfo
{
public:
	lcSynthInfo(lcSynthType Type, float Length, int NumSections);

	void GetDefaultControlPoints(lcArray<lcPieceControlPoint>& ControlPoints) const;
	int InsertControlPoint(lcArray<lcPieceControlPoint>& ControlPoints, const lcVector3& Start, const lcVector3& End) const;
	lcMesh* CreateMesh(const lcArray<lcPieceControlPoint>& ControlPoints) const;

protected:
	float GetSectionTwist(const lcMatrix44& StartTransform, const lcMatrix44& EndTransform) const;
	void CalculateSections(const lcArray<lcPieceControlPoint>& ControlPoints, lcArray<lcMatrix44>& Sections, void(*SectionCallback)(const lcVector3& CurvePoint, int SegmentIndex, float t, void* Param), void* CallbackParam) const;
	void AddRibbedHoseParts(lcMemFile& File, const lcArray<lcMatrix44>& Sections) const;
	void AddFlexibleAxleParts(lcMemFile& File, lcLibraryMeshData& MeshData, const lcArray<lcMatrix44>& Sections) const;
	void AddStringBraidedParts(lcMemFile& File, lcLibraryMeshData& MeshData, lcArray<lcMatrix44>& Sections) const;

	lcSynthType mType;
	lcSynthComponent mStart;
	lcSynthComponent mMiddle;
	lcSynthComponent mEnd;
	float mLength;
	int mNumSections;
	bool mRigidEdges;
};

void lcSynthInit();

#endif // _LC_SYNTH_H_

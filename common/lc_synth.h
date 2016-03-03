#ifndef _LC_SYNTH_H_
#define _LC_SYNTH_H_

#include "lc_math.h"
#include "piece.h"
#include "pieceinf.h"

struct lcSynthComponent
{
	char PartID[LC_PIECE_NAME_LEN];
	lcMatrix44 Transform;
	float Length;
};

struct lcSynthInfo
{
	lcMatrix44 DefaultControlPoints[2];
	lcSynthComponent Components[3];
	float DefaultStiffness;
	int NumSections;
};

void lcSynthInit();
lcMesh* lcSynthCreateMesh(const lcSynthInfo* SynthInfo, const lcArray<lcPieceControlPoint>& ControlPoints);
int lcSynthInsertControlPoint(const lcSynthInfo* SynthInfo, lcArray<lcPieceControlPoint>& ControlPoints, const lcVector3& Start, const lcVector3& End);

#endif // _LC_SYNTH_H_

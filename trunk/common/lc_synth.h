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
lcMesh* lcSynthCreateMesh(lcSynthInfo* SynthInfo, const lcArray<lcPieceControlPoint>& ControlPoints);

#endif // _LC_SYNTH_H_

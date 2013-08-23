#ifndef LC_MODEL_H
#define LC_MODEL_H

#include "lc_array.h"
#include "lc_math.h"

class PieceInfo;

class lcModel
{
public:
	lcModel();
	~lcModel();

	void DeleteContents();

	lcArray<lcPart*> mParts;
	lcArray<lcCamera*> mCameras;
	lcArray<lcLight*> mLights;
};

#endif // LC_MODEL_H

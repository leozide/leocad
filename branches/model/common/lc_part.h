#ifndef LC_PART_H
#define LC_PART_H

#include "lc_object.h"
#include "lc_array.h"
#include "lc_math.h"

class lcPart : public lcObject
{
public:
	lcPart();

	void Update(lcKeyTime Time)
	{

	}

	lcObject* mParent;
	lcArray<lcObject*> mChildren;

	lcPartInfo* mPartInfo;
	int mColorIndex;

//	lcMatrix44 mModelWorld;
//	lcVector3 mPosition;
//	lcVector4 mAxisAngle;

//	lcArray<lcObjectVector3Key> mPositionKeys;
//	lcArray<lcObjectVector4Key> mAxisAngleKeys;
};

#endif // LC_PART_H

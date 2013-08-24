#ifndef LC_OBJECT_H
#define LC_OBJECT_H

#include "lc_array.h"
#include "lc_math.h"

typedef lcuint64 lcKeyTime;

template<typename T>
struct lcObjectPropertyKey
{
	lcKeyTime Time;
	T Property;
	// TODO: key in/out curves
};

typedef lcObjectPropertyKey<float> lcObjectFloatKey;
typedef lcObjectPropertyKey<lcVector3> lcObjectVector3Key;
typedef lcObjectPropertyKey<lcVector4> lcObjectVector4Key;

class lcObject
{
public:
	lcObject();

	template<typename T>
	const T& CalculateKey(const lcArray< lcObjectPropertyKey<T> >& Keys, lcKeyTime Time)
	{
		lcObjectPropertyKey<T>* PreviousKey = &Keys[0];

		for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
		{
			if (Keys[KeyIdx].Time > Time)
				break;

			PreviousKey = &Keys[KeyIdx];
		}

		return PreviousKey->Property;
	}
};

#endif // LC_OBJECT_H

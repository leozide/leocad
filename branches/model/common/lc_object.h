#ifndef LC_OBJECT_H
#define LC_OBJECT_H

typedef lcuint64 lcTime;

template<typename T>
struct lcObjectPropertyKey
{
	lcTime Time;
	T Property;
};

typedef lcObjectPropertyKey<lcVector3> lcObjectVector3Key;
typedef lcObjectPropertyKey<lcVector4> lcObjectVector4Key;

class lcObject
{
public:
	lcObject();
};

#endif // LC_OBJECT_H

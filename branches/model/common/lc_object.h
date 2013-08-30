#ifndef LC_OBJECT_H
#define LC_OBJECT_H

#include "lc_array.h"
#include "lc_math.h"

class View;
//class lcRenderMesh;

enum lcObjectType
{
	LC_OBJECT_TYPE_PART,
	LC_OBJECT_TYPE_CAMERA,
	LC_OBJECT_TYPE_LIGHT
};

template<typename T>
struct lcObjectPropertyKey
{
	lcTime Time;
	T Property;
	// TODO: key in/out curves
};

typedef lcObjectPropertyKey<float> lcObjectFloatKey;
typedef lcObjectPropertyKey<lcVector3> lcObjectVector3Key;
typedef lcObjectPropertyKey<lcVector4> lcObjectVector4Key;

struct lcObjectSection
{
	lcObject* Object;
	lcuint32 Section;
};

struct lcObjectHitTest
{
	lcVector3 Start;
	lcVector3 End;
	float Distance;
	lcObjectSection ObjectSection;
};

class lcObject
{
public:
	lcObject(lcObjectType Type);
	virtual ~lcObject();

	bool IsPart() const
	{
		return mObjectType == LC_OBJECT_TYPE_PART;
	}

	bool IsCamera() const
	{
		return mObjectType == LC_OBJECT_TYPE_CAMERA;
	}

	bool IsLight() const
	{
		return mObjectType == LC_OBJECT_TYPE_LIGHT;
	}

	virtual bool IsSelected() const = 0;
	virtual bool IsSelected(lcuint32 Section) const = 0;
	virtual bool IsFocused() const = 0;
	virtual bool IsFocused(lcuint32 Section) const = 0;
	virtual void ClearSelection() = 0;
	virtual void ClearFocus() = 0;
	virtual void SetSelection(lcuint32 Section, bool Selection) = 0;
	virtual void SetFocus(lcuint32 Section, bool Focus) = 0;
	virtual void ToggleSelection(lcuint32 Section) = 0;
	virtual void SaveSelectionState(lcMemFile& File) const = 0;

	virtual void ClosestHitTest(lcObjectHitTest& HitTest) = 0;
	virtual void BoxTest(const lcVector4* BoxPlanes, lcArray<lcObjectSection>& ObjectSections) = 0;

//	virtual void GetRenderMeshes(View* View, bool PartsOnly, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes) const = 0;
	virtual void RenderExtra(View* View) const = 0;

protected:
	template<typename T>
	const T& CalculateKey(const lcArray< lcObjectPropertyKey<T> >& Keys, lcTime Time)
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

	lcObjectType mObjectType;
};

#endif // LC_OBJECT_H

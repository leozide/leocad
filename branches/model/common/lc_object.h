#ifndef LC_OBJECT_H
#define LC_OBJECT_H

#include "lc_array.h"
#include "lc_math.h"

class View;
class PieceInfo;
struct lcRenderMesh;

enum lcObjectType
{
	LC_OBJECT_TYPE_PIECE,
	LC_OBJECT_TYPE_CAMERA
//	LC_OBJECT_TYPE_LIGHT
};

template<typename T>
struct lcObjectKey
{
	lcTime Time;
	T Value;
	// TODO: key in/out curves
};

typedef lcObjectKey<float> lcObjectFloatKey;
typedef lcObjectKey<lcVector3> lcObjectVector3Key;
typedef lcObjectKey<lcVector4> lcObjectVector4Key;

struct lcObjectSection
{
	lcObject* Object;
	lcuintptr Section;
};

struct lcObjectHitTest
{
	bool PiecesOnly;
	lcCamera* Camera;
	lcVector3 Start;
	lcVector3 End;
	float Distance;
	lcObjectSection ObjectSection;
};

struct lcObjectBoxTest
{
	lcCamera* Camera;
	lcVector4 Planes[6];
	lcArray<lcObjectSection> ObjectSections;
};

struct lcObjectParts
{
	PieceInfo* Info;
	int ColorIndex;
	int Count;
};

class lcObject
{
public:
	lcObject(lcObjectType Type);
	virtual ~lcObject();

	lcObjectType ObjectType() const
	{
		return mObjectType;
	}

	bool IsCamera() const
	{
		return mObjectType == LC_OBJECT_TYPE_CAMERA;
	}

	virtual bool IsVisible() const = 0;
	virtual void SetVisible(bool Visible) = 0;
	virtual bool IsSelected() const = 0;
	virtual bool IsSelected(lcuintptr Section) const = 0;
	virtual bool IsFocused() const = 0;
	virtual bool IsFocused(lcuintptr Section) const = 0;
	virtual void SetSelection(bool Selection) = 0;
	virtual void SetSelection(lcuintptr Section, bool Selection) = 0;
	virtual void ClearFocus() = 0;
	virtual void SetFocus(lcuintptr Section, bool Focus) = 0;
	virtual lcuintptr GetFocusSection() const = 0;
	virtual void InvertSelection() = 0;
	virtual void InvertSelection(lcuintptr Section) = 0;

	virtual void SetCurrentTime(lcTime Time) = 0;

	virtual void Save(lcFile& File) = 0;
	virtual void Load(lcFile& File) = 0;
	virtual void Update() = 0;

	virtual void ClosestHitTest(lcObjectHitTest& HitTest) = 0;
	virtual void BoxTest(lcObjectBoxTest& BoxTest) = 0;

	virtual lcPiece* GetPiece(lcuintptr Section) { return NULL; }
	virtual void GetPartsUsed(lcArray<lcObjectParts>& PartsUsed) const { }
	virtual void GetRenderMeshes(View* View, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes, lcArray<lcObject*>& InterfaceObjects) = 0;
	virtual void RenderInterface(View* View) const = 0;

	virtual lcVector3 GetFocusPosition() const = 0;
	virtual void Move(const lcVector3& Distance, lcTime Time, bool AddKeys) = 0;

protected:
	template<typename T>
	const T& CalculateKey(const lcArray< lcObjectKey<T> >& Keys, lcTime Time)
	{
		lcObjectKey<T>* PreviousKey = &Keys[0];

		for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
		{
			if (Keys[KeyIdx].Time > Time)
				break;

			PreviousKey = &Keys[KeyIdx];
		}

		return PreviousKey->Value;
	}

	template<typename T>
	void ChangeKey(lcArray< lcObjectKey<T> >& Keys, const T& Value, lcTime Time, bool AddKey)
	{
		lcObjectKey<T>* Key;

		for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
		{
			Key = &Keys[KeyIdx];

			if (Key->Time == Time)
			{
				Key->Value = Value;

				return;
			}

			if (Key->Time > Time)
			{
				if (AddKey)
				{
					Key = &Keys.InsertAt(KeyIdx);

					Key->Time = Time;
					Key->Value = Value;
				}
				else if (KeyIdx)
				{
					Key = &Keys[KeyIdx - 1];

					Key->Value = Value;
				}
				else
				{
					Key->Time = Time;
					Key->Value = Value;
				}

				return;
			}
		}

		if (AddKey || Keys.GetSize() == 0)
		{
			Key = &Keys.Add();

			Key->Time = Time;
			Key->Value = Value;
		}
		else
		{
			Key = &Keys[Keys.GetSize() - 1];

			Key->Value = Value;
		}
	}

	lcObjectType mObjectType;
};

#endif // LC_OBJECT_H

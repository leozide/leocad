#pragma once

#include "lc_math.h"
#include "lc_array.h"

typedef quint32 lcStep;
#define LC_STEP_MAX 0xffffffff

enum class lcObjectType
{
	Piece,
	Camera,
	Light
};

template<typename T>
struct lcObjectKey
{
	lcStep Step;
	T Value;
};

struct lcObjectSection
{
	lcObject* Object;
	quint32 Section;
};

struct lcObjectRayTest
{
	lcCamera* ViewCamera;
	bool PiecesOnly;
	bool IgnoreSelected;
	lcVector3 Start;
	lcVector3 End;
	float Distance;
	lcObjectSection ObjectSection;
};

struct lcObjectBoxTest
{
	lcCamera* ViewCamera;
	lcVector4 Planes[6];
	lcArray<lcObject*> Objects;
};

#define LC_OBJECT_TRANSFORM_MOVE_X   0x001
#define LC_OBJECT_TRANSFORM_MOVE_Y   0x002
#define LC_OBJECT_TRANSFORM_MOVE_Z   0x004
#define LC_OBJECT_TRANSFORM_ROTATE_X 0x010
#define LC_OBJECT_TRANSFORM_ROTATE_Y 0x020
#define LC_OBJECT_TRANSFORM_ROTATE_Z 0x040
#define LC_OBJECT_TRANSFORM_SCALE_X  0x100
#define LC_OBJECT_TRANSFORM_SCALE_Y  0x200
#define LC_OBJECT_TRANSFORM_SCALE_Z  0x400

class lcObject
{
public:
	lcObject(lcObjectType ObjectType);
	virtual ~lcObject();

public:
	bool IsPiece() const
	{
		return mObjectType == lcObjectType::Piece;
	}

	bool IsCamera() const
	{
		return mObjectType == lcObjectType::Camera;
	}

	bool IsLight() const
	{
		return mObjectType == lcObjectType::Light;
	}

	lcObjectType GetType() const
	{
		return mObjectType;
	}

	virtual bool IsSelected() const = 0;
	virtual bool IsSelected(quint32 Section) const = 0;
	virtual void SetSelected(bool Selected) = 0;
	virtual void SetSelected(quint32 Section, bool Selected) = 0;
	virtual bool IsFocused() const = 0;
	virtual bool IsFocused(quint32 Section) const = 0;
	virtual void SetFocused(quint32 Section, bool Focused) = 0;
	virtual quint32 GetFocusSection() const = 0;

	virtual quint32 GetAllowedTransforms() const = 0;
	virtual lcVector3 GetSectionPosition(quint32 Section) const = 0;
	virtual void RayTest(lcObjectRayTest& ObjectRayTest) const = 0;
	virtual void BoxTest(lcObjectBoxTest& ObjectBoxTest) const = 0;
	virtual void DrawInterface(lcContext* Context, const lcScene& Scene) const = 0;
	virtual void RemoveKeyFrames() = 0;
	virtual const char* GetName() const = 0;

protected:
	template<typename T>
	void SaveKeysLDraw(QTextStream& Stream, const lcArray<lcObjectKey<T>>& Keys, const char* KeyName) const
	{
		const int Count = sizeof(T) / sizeof(float);
		for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
		{
			const lcObjectKey<T>& Key = Keys[KeyIdx];
			Stream << QLatin1String("0 !LEOCAD ") << KeyName << Key.Step << ' ';
			for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
				Stream << ((float*)&Key.Value)[ValueIdx] << ' ';
			Stream << QLatin1String("\r\n");
		}
	}

	template<typename T>
	void LoadKeysLDraw(QTextStream& Stream, lcArray<lcObjectKey<T>>& Keys)
	{
		QString Token;
		Stream >> Token;

		int Step = Token.toInt();
		T Value;

		const int Count = sizeof(T) / sizeof(float);
		for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
			Stream >> ((float*)&Value)[ValueIdx];

		ChangeKey(Keys, Value, Step, true);
	}

	template<typename T>
	const T& CalculateKey(const lcArray<lcObjectKey<T>>& Keys, lcStep Step)
	{
		const lcObjectKey<T>* PreviousKey = &Keys[0];

		for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
		{
			if (Keys[KeyIdx].Step > Step)
				break;

			PreviousKey = &Keys[KeyIdx];
		}

		return PreviousKey->Value;
	}

	template<typename T>
	void ChangeKey(lcArray<lcObjectKey<T>>& Keys, const T& Value, lcStep Step, bool AddKey)
	{
		for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
		{
			lcObjectKey<T>* Key = &Keys[KeyIdx];

			if (Key->Step == Step)
			{
				Key->Value = Value;

				return;
			}

			if (Key->Step > Step)
			{
				if (AddKey)
				{
					Key = &Keys.InsertAt(KeyIdx);
					Key->Step = Step;
				}
				else if (KeyIdx)
					Key = &Keys[KeyIdx - 1];

				Key->Value = Value;

				return;
			}
		}

		if (AddKey || Keys.GetSize() == 0)
		{
			lcObjectKey<T>* Key = &Keys.Add();

			Key->Step = Step;
			Key->Value = Value;
		}
		else
		{
			lcObjectKey<T>* Key = &Keys[Keys.GetSize() - 1];
			Key->Value = Value;
		}
	}

	template<typename T>
	void InsertTime(lcArray<lcObjectKey<T>>& Keys, lcStep Start, lcStep Time)
	{
		bool EndKey = false;

		for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
		{
			lcObjectKey<T>& Key = Keys[KeyIdx];

			if ((Key.Step < Start) || (Key.Step == 1))
				continue;

			if (EndKey)
			{
				Keys.RemoveIndex(KeyIdx);
				KeyIdx--;
				continue;
			}

			if (Key.Step >= LC_STEP_MAX - Time)
			{
				Key.Step = LC_STEP_MAX;
				EndKey = true;
			}
			else
				Key.Step += Time;
		}
	}

	template<typename T>
	void RemoveTime(lcArray<lcObjectKey<T>>& Keys, lcStep Start, lcStep Time)
	{
		for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
		{
			lcObjectKey<T>& Key = Keys[KeyIdx];

			if ((Key.Step < Start) || (Key.Step == 1))
				continue;

			if (Key.Step < Start + Time)
			{
				Keys.RemoveIndex(KeyIdx);
				KeyIdx--;
				continue;
			}

			Key.Step -= Time;
		}
	}

private:
	lcObjectType mObjectType;
};


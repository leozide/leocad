#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "lc_math.h"
#include "lc_array.h"

typedef lcuint32 lcStep;
#define LC_STEP_MAX 0xffffffff

enum lcObjectType
{
	LC_OBJECT_PIECE,
	LC_OBJECT_CAMERA,
	LC_OBJECT_LIGHT
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
	lcuint32 Section;
};

struct lcObjectRayTest
{
	lcCamera* ViewCamera;
	bool PiecesOnly;
	lcVector3 Start;
	lcVector3 End;
	float Distance;
	lcObjectSection ObjectSection;
};

struct lcObjectBoxTest
{
	lcCamera* ViewCamera;
	lcVector4 Planes[6];
	lcArray<lcObjectSection> ObjectSections;
};

class lcObject
{
public:
	lcObject(lcObjectType ObjectType);
	virtual ~lcObject();

public:
	bool IsPiece() const
	{
		return mObjectType == LC_OBJECT_PIECE;
	}

	bool IsCamera() const
	{
		return mObjectType == LC_OBJECT_CAMERA;
	}

	bool IsLight() const
	{
		return mObjectType == LC_OBJECT_LIGHT;
	}

	lcObjectType GetType() const
	{
		return mObjectType;
	}

	virtual bool IsSelected() const = 0;
	virtual bool IsSelected(lcuint32 Section) const = 0;
	virtual void SetSelected(bool Selected) = 0;
	virtual void SetSelected(lcuint32 Section, bool Selected) = 0;
	virtual bool IsFocused() const = 0;
	virtual bool IsFocused(lcuint32 Section) const = 0;
	virtual void SetFocused(lcuint32 Section, bool Focused) = 0;
	virtual lcuint32 GetFocusSection() const = 0;

	virtual lcVector3 GetSectionPosition(lcuint32 Section) const = 0;
	virtual void Move(lcStep Step, bool AddKey, const lcVector3& Distance) = 0;
	virtual void RayTest(lcObjectRayTest& ObjectRayTest) const = 0;
	virtual void BoxTest(lcObjectBoxTest& ObjectBoxTest) const = 0;
	virtual const char* GetName() const = 0;

protected:
	void SaveKeysLDraw(const lcArray<lcObjectKey<lcVector3>>& Keys, const char* KeyName, lcFile& File) const;
	void SaveKeysLDraw(const lcArray<lcObjectKey<lcVector4>>& Keys, const char* KeyName, lcFile& File) const;

	template<typename T>
	void LoadKeyLDraw(lcArray<lcObjectKey<T>>& Keys, QString& Line)
	{
		QRegExp TokenExp("\\s*(\\w+)\\s+(.*)");

		if (!Line.contains(TokenExp))
			return;

		QString Token = TokenExp.cap(1);
		Line = TokenExp.cap(2);

		int Step = Token.toInt();
		T Value;

		QTextStream Stream(&Line);
		const int Count = sizeof(T) / sizeof(float);
		for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
			Stream >> Value[ValueIdx];

		lcObjectKey<T>& Key = Keys.Add();
		Key.Step = Step;
		Key.Value = Value;
	}

	template<typename T>
	QJsonArray SaveKeysJson(const lcArray<lcObjectKey<T>>& Keys) const
	{
		const int Count = sizeof(T) / sizeof(float);
		QJsonArray KeyArray;
		QString StepString = QStringLiteral("Step");
		QString ValueString = QStringLiteral("Value");

		for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
		{
			lcObjectKey<T>& Key = Keys[KeyIdx];

			QJsonArray ValueArray;
			for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
				ValueArray << Key.Value[ValueIdx];

			QJsonObject KeyObject;
			KeyObject[StepString] = (int)Key.Step;
			KeyObject[ValueString] = ValueArray;
			KeyArray.append(KeyObject);
		}

		return KeyArray;
	}

	template<typename T>
	void LoadKeysJson(lcArray<lcObjectKey<T>>& Keys, const QJsonValue& KeysValue, T& Single, const QJsonValue& SingleValue)
	{
		const int Count = sizeof(T) / sizeof(float);
		Keys.SetSize(0);
		QString StepString = QStringLiteral("Step");
		QString ValueString = QStringLiteral("Value");

		if (!KeysValue.isUndefined())
		{
			QJsonArray KeyArray = KeysValue.toArray();
			Keys.AllocGrow(KeyArray.size());

			for (int KeyIdx = 0; KeyIdx < KeyArray.size(); KeyIdx++)
			{
				QJsonObject KeyObject = KeyArray[KeyIdx].toObject();

				QJsonValue Step = KeyObject.value(StepString);
				QJsonValue Value = KeyObject.value(ValueString);

				if (Step.isUndefined() || Value.isUndefined())
					continue;

				lcStep CurrentStep = Step.toInt();

				if (!Keys.IsEmpty() && Keys[Keys.GetSize() - 1].Step >= CurrentStep)
					continue;

				lcObjectKey<T>& Key = Keys.Add();
				Key.Step = CurrentStep;

				QJsonArray ValueArray = Value.toArray();
				if (ValueArray.size() == Count)
					for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
						Single[ValueIdx] = ValueArray[ValueIdx].toDouble();
			}
		}
		else if (!SingleValue.isUndefined())
		{
			QJsonArray ValueArray = SingleValue.toArray();

			if (ValueArray.size() == Count)
				for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
					Single[ValueIdx] = ValueArray[ValueIdx].toDouble();
		}

		if (Keys.IsEmpty())
			ChangeKey(Keys, Single, 1, true);
	}

	template<typename T>
	const T& CalculateKey(const lcArray<lcObjectKey<T>>& Keys, lcStep Step)
	{
		lcObjectKey<T>* PreviousKey = &Keys[0];

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
		lcObjectKey<T>* Key;

		for (int KeyIdx = 0; KeyIdx < Keys.GetSize(); KeyIdx++)
		{
			Key = &Keys[KeyIdx];

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
			Key = &Keys.Add();

			Key->Step = Step;
			Key->Value = Value;
		}
		else
		{
			Key = &Keys[Keys.GetSize() - 1];
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

#endif

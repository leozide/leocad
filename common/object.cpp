#include "lc_global.h"
#include "object.h"
#include "lc_file.h"

lcObject::lcObject(lcObjectType ObjectType)
	: mObjectType(ObjectType)
{
}

lcObject::~lcObject()
{
}

template class lcObjectKeyArray<float>;
template class lcObjectKeyArray<lcVector3>;
template class lcObjectKeyArray<lcVector4>;
template class lcObjectKeyArray<lcMatrix33>;

template<typename T>
void lcObjectKeyArray<T>::SaveKeysLDraw(QTextStream& Stream, const char* KeyName) const
{
	const int Count = sizeof(T) / sizeof(float);

	for (const lcObjectKey<T>& Key : mKeys)
	{
		Stream << QLatin1String("0 !LEOCAD ") << KeyName << Key.Step << ' ';

		for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
			Stream << ((float*)&Key.Value)[ValueIdx] << ' ';

		Stream << QLatin1String("\r\n");
	}
}

template<typename T>
void lcObjectKeyArray<T>::LoadKeysLDraw(QTextStream& Stream)
{
	QString Token;
	Stream >> Token;

	int Step = Token.toInt();
	T Value;

	const int Count = sizeof(T) / sizeof(float);

	for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
		Stream >> ((float*)&Value)[ValueIdx];

	ChangeKey(Value, Step, true);
}

template<typename T>
const T& lcObjectKeyArray<T>::CalculateKey(lcStep Step) const
{
	const lcObjectKey<T>* PreviousKey = &mKeys[0];

	for (const lcObjectKey<T>& Key : mKeys)
	{
		if (Key.Step > Step)
			break;

		PreviousKey = &Key;
	}

	return PreviousKey->Value;
}

template<typename T>
void lcObjectKeyArray<T>::ChangeKey(const T& Value, lcStep Step, bool AddKey)
{
	for (int KeyIdx = 0; KeyIdx < mKeys.GetSize(); KeyIdx++)
	{
		lcObjectKey<T>* Key = &mKeys[KeyIdx];

		if (Key->Step == Step)
		{
			Key->Value = Value;

			return;
		}

		if (Key->Step > Step)
		{
			if (AddKey)
			{
				Key = &mKeys.InsertAt(KeyIdx);
				Key->Step = Step;
			}
			else if (KeyIdx)
				Key = &mKeys[KeyIdx - 1];

			Key->Value = Value;

			return;
		}
	}

	if (AddKey || mKeys.GetSize() == 0)
	{
		lcObjectKey<T>* Key = &mKeys.Add();

		Key->Step = Step;
		Key->Value = Value;
	}
	else
	{
		lcObjectKey<T>* Key = &mKeys[mKeys.GetSize() - 1];
		Key->Value = Value;
	}
}

template<typename T>
void lcObjectKeyArray<T>::InsertTime(lcStep Start, lcStep Time)
{
	bool EndKey = false;

	for (int KeyIdx = 0; KeyIdx < mKeys.GetSize(); KeyIdx++)
	{
		lcObjectKey<T>& Key = mKeys[KeyIdx];

		if ((Key.Step < Start) || (Key.Step == 1))
			continue;

		if (EndKey)
		{
			mKeys.RemoveIndex(KeyIdx);
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
void lcObjectKeyArray<T>::RemoveTime(lcStep Start, lcStep Time)
{
	for (int KeyIdx = 0; KeyIdx < mKeys.GetSize(); KeyIdx++)
	{
		lcObjectKey<T>& Key = mKeys[KeyIdx];

		if ((Key.Step < Start) || (Key.Step == 1))
			continue;

		if (Key.Step < Start + Time)
		{
			mKeys.RemoveIndex(KeyIdx);
			KeyIdx--;
			continue;
		}

		Key.Step -= Time;
	}
}

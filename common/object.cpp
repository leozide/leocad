#include "lc_global.h"
#include "object.h"

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
	for (typename std::vector<lcObjectKey<T>>::iterator KeyIt = mKeys.begin(); KeyIt != mKeys.end(); KeyIt++)
	{
		if (KeyIt->Step < Step)
			continue;

		if (KeyIt->Step == Step)
			KeyIt->Value = Value;
		else if (AddKey)
			mKeys.insert(KeyIt, lcObjectKey<T>{ Step, Value });
		else if (KeyIt == mKeys.begin())
			KeyIt->Value = Value;
		else
		{
			KeyIt = KeyIt - 1;
			KeyIt->Value = Value;
		}

		return;
	}

	if (AddKey || mKeys.empty())
		mKeys.emplace_back(lcObjectKey<T>{ Step, Value });
	else
		mKeys.back().Value = Value;
}

template<typename T>
void lcObjectKeyArray<T>::InsertTime(lcStep Start, lcStep Time)
{
	bool EndKey = false;

	for (typename std::vector<lcObjectKey<T>>::iterator KeyIt = mKeys.begin(); KeyIt != mKeys.end();)
	{
		if ((KeyIt->Step < Start) || (KeyIt->Step == 1))
		{
			KeyIt++;
			continue;
		}

		if (EndKey)
		{
			KeyIt = mKeys.erase(KeyIt);
			continue;
		}

		if (KeyIt->Step >= LC_STEP_MAX - Time)
		{
			KeyIt->Step = LC_STEP_MAX;
			EndKey = true;
		}
		else
			KeyIt->Step += Time;

		KeyIt++;
	}
}

template<typename T>
void lcObjectKeyArray<T>::RemoveTime(lcStep Start, lcStep Time)
{
	for (typename std::vector<lcObjectKey<T>>::iterator KeyIt = mKeys.begin(); KeyIt != mKeys.end();)
	{
		if ((KeyIt->Step < Start) || (KeyIt->Step == 1))
		{
			KeyIt++;
			continue;
		}

		if (KeyIt->Step < Start + Time)
		{
			KeyIt = mKeys.erase(KeyIt);
			continue;
		}

		KeyIt->Step -= Time;
		KeyIt++;
	}
}

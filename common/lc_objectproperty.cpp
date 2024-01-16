#include "lc_global.h"
#include "lc_objectproperty.h"
#include "lc_math.h"

#define LC_OBJECT_PROPERTY(T) \
	template void lcObjectProperty<T>::SaveKeysLDraw(QTextStream& Stream, const char* ObjectName, const char* VariableName) const; \
	template void lcObjectProperty<T>::LoadKeysLDraw(QTextStream& Stream); \
	template void lcObjectProperty<T>::Update(lcStep Step); \
	template void lcObjectProperty<T>::ChangeKey(const T& Value, lcStep Step, bool AddKey); \
	template void lcObjectProperty<T>::InsertTime(lcStep Start, lcStep Time); \
	template void lcObjectProperty<T>::RemoveTime(lcStep Start, lcStep Time); \
	template bool lcObjectProperty<T>::HasKeyFrame(lcStep Time) const; \
	template void lcObjectProperty<T>::Save(QTextStream& Stream, const char* ObjectName, const char* VariableName) const; \
	template bool lcObjectProperty<T>::Load(QTextStream& Stream, const QString& Token, const char* VariableName);

LC_OBJECT_PROPERTY(float)
LC_OBJECT_PROPERTY(lcVector2i)
LC_OBJECT_PROPERTY(lcVector2)
LC_OBJECT_PROPERTY(lcVector3)
LC_OBJECT_PROPERTY(lcVector4)
LC_OBJECT_PROPERTY(lcMatrix33)

template<typename T>
static void lcObjectPropertySaveValue(QTextStream& Stream, const T& Value)
{
	constexpr int Count = sizeof(T) / sizeof(float);

	for (int ValueIndex = 0; ValueIndex < Count; ValueIndex++)
		Stream << ((const float*)&Value)[ValueIndex] << ' ';
}

template<>
void lcObjectPropertySaveValue(QTextStream& Stream, const lcVector2i& Value)
{
	constexpr int Count = sizeof(lcVector2i) / sizeof(int);

	for (int ValueIndex = 0; ValueIndex < Count; ValueIndex++)
		Stream << ((const int*)&Value)[ValueIndex] << ' ';
}

template<typename T>
static void lcObjectPropertyLoadValue(QTextStream& Stream, T& Value)
{
	constexpr int Count = sizeof(T) / sizeof(float);

	for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
		Stream >> ((float*)&Value)[ValueIdx];
}

template<>
void lcObjectPropertyLoadValue(QTextStream& Stream, lcVector2i& Value)
{
	constexpr int Count = sizeof(lcVector2i) / sizeof(int);

	for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
		Stream >> ((int*)&Value)[ValueIdx];
}

template<typename T>
void lcObjectProperty<T>::SaveKeysLDraw(QTextStream& Stream, const char* ObjectName, const char* VariableName) const
{
	for (const lcObjectPropertyKey<T>& Key : mKeys)
	{
		Stream << QLatin1String("0 !LEOCAD ") << ObjectName << ' ' << VariableName << "_KEY " << Key.Step << ' ';

		lcObjectPropertySaveValue(Stream, Key.Value);

		Stream << QLatin1String("\r\n");
	}
}

template<typename T>
void lcObjectProperty<T>::LoadKeysLDraw(QTextStream& Stream)
{
	QString Token;
	Stream >> Token;

	const int Step = Token.toInt();
	T Value;

	constexpr int Count = sizeof(T) / sizeof(float);

	for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
		Stream >> ((float*)&Value)[ValueIdx];

	ChangeKey(Value, Step, true);
}

template<typename T>
void lcObjectProperty<T>::Update(lcStep Step)
{
	if (mKeys.empty())
		return;

	const lcObjectPropertyKey<T>* PreviousKey = &mKeys[0];

	for (const lcObjectPropertyKey<T>& Key : mKeys)
	{
		if (Key.Step > Step)
			break;

		PreviousKey = &Key;
	}

	mValue = PreviousKey->Value;
}

template<typename T>
void lcObjectProperty<T>::ChangeKey(const T& Value, lcStep Step, bool AddKey)
{
	if (!AddKey && mKeys.empty())
	{
		mValue = Value;

		return;
	}

	for (typename std::vector<lcObjectPropertyKey<T>>::iterator KeyIt = mKeys.begin(); KeyIt != mKeys.end(); KeyIt++)
	{
		if (KeyIt->Step < Step)
			continue;

		if (KeyIt->Step == Step)
			KeyIt->Value = Value;
		else if (AddKey)
			mKeys.insert(KeyIt, lcObjectPropertyKey<T>{ Step, Value });
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
		mKeys.emplace_back(lcObjectPropertyKey<T>{ Step, Value });
	else
		mKeys.back().Value = Value;
}

template<typename T>
void lcObjectProperty<T>::InsertTime(lcStep Start, lcStep Time)
{
	bool EndKey = false;

	for (typename std::vector<lcObjectPropertyKey<T>>::iterator KeyIt = mKeys.begin(); KeyIt != mKeys.end();)
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
void lcObjectProperty<T>::RemoveTime(lcStep Start, lcStep Time)
{
	for (typename std::vector<lcObjectPropertyKey<T>>::iterator KeyIt = mKeys.begin(); KeyIt != mKeys.end();)
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

template<typename T>
bool lcObjectProperty<T>::HasKeyFrame(lcStep Time) const
{
	for (typename std::vector<lcObjectPropertyKey<T>>::const_iterator KeyIt = mKeys.begin(); KeyIt != mKeys.end(); KeyIt++)
	{
		if (KeyIt->Step == Time)
			return true;
		else if (KeyIt->Step > Time)
			return false;
	}

	return false;
}

template<typename T>
void lcObjectProperty<T>::Save(QTextStream& Stream, const char* ObjectName, const char* VariableName) const
{
	if (mKeys.empty())
	{
		Stream << QLatin1String("0 !LEOCAD ") << ObjectName << ' ' << VariableName << ' ';

		lcObjectPropertySaveValue(Stream, mValue);

		Stream << QLatin1String("\r\n");
	}
	else
		SaveKeysLDraw(Stream, ObjectName, VariableName);
}

template<typename T>
bool lcObjectProperty<T>::Load(QTextStream& Stream, const QString& Token, const char* VariableName)
{
	if (Token == VariableName)
	{
		lcObjectPropertyLoadValue(Stream, mValue);

		return true;
	}

	if (Token.endsWith(QLatin1String("_KEY")) && Token.leftRef(Token.size() - 4) == VariableName)
	{
		LoadKeysLDraw(Stream);

		return true;
	}

	return false;
}

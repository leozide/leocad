#include "lc_global.h"
#include "lc_objectproperty.h"
#include "lc_math.h"

#define LC_OBJECT_PROPERTY(T) \
	template void lcObjectProperty<T>::Update(lcStep Step); \
	template bool lcObjectProperty<T>::ChangeKey(const T& Value, lcStep Step, bool AddKey); \
	template void lcObjectProperty<T>::InsertTime(lcStep Start, lcStep Time); \
	template void lcObjectProperty<T>::RemoveTime(lcStep Start, lcStep Time); \
	template bool lcObjectProperty<T>::HasKeyFrame(lcStep Time) const; \
	template bool lcObjectProperty<T>::SetKeyFrame(lcStep Time, bool KeyFrame); \
	template void lcObjectProperty<T>::Save(QTextStream& Stream, const char* ObjectName, const char* VariableName, bool SaveEmpty) const; \
	template bool lcObjectProperty<T>::Load(QTextStream& Stream, const QString& Token, const char* VariableName); \
    template void lcObjectProperty<T>::SaveToDataStream(QDataStream& Stream) const; \
    template bool lcObjectProperty<T>::LoadFromDataStream(QDataStream& Stream);

LC_OBJECT_PROPERTY(float)
LC_OBJECT_PROPERTY(int)
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
void lcObjectPropertySaveValue(QTextStream& Stream, const int& Value)
{
	constexpr int Count = sizeof(int) / sizeof(int);

	for (int ValueIndex = 0; ValueIndex < Count; ValueIndex++)
		Stream << ((const int*)&Value)[ValueIndex] << ' ';
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
void lcObjectPropertyLoadValue(QTextStream& Stream, int& Value)
{
	constexpr int Count = sizeof(int) / sizeof(int);

	for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
		Stream >> ((int*)&Value)[ValueIdx];
}

template<>
void lcObjectPropertyLoadValue(QTextStream& Stream, lcVector2i& Value)
{
	constexpr int Count = sizeof(lcVector2i) / sizeof(int);

	for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
		Stream >> ((int*)&Value)[ValueIdx];
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
bool lcObjectProperty<T>::ChangeKey(const T& Value, lcStep Step, bool AddKey)
{
	if (!AddKey && mKeys.empty())
	{
		if (mValue == Value)
			return false;

		mValue = Value;

		return true;
	}

	for (typename std::vector<lcObjectPropertyKey<T>>::iterator KeyIt = mKeys.begin(); KeyIt != mKeys.end(); KeyIt++)
	{
		if (KeyIt->Step < Step)
			continue;

		if (KeyIt->Step == Step)
		{
			if (KeyIt->Value == Value)
				return false;

			KeyIt->Value = Value;
		}
		else if (AddKey)
			mKeys.insert(KeyIt, lcObjectPropertyKey<T>{ Step, Value });
		else if (KeyIt == mKeys.begin())
		{
			if (KeyIt->Value == Value)
				return false;

			KeyIt->Value = Value;
		}
		else
		{
			KeyIt = KeyIt - 1;

			if (KeyIt->Value == Value)
				return false;

			KeyIt->Value = Value;
		}

		return true;
	}

	if (AddKey || mKeys.empty())
		mKeys.emplace_back(lcObjectPropertyKey<T>{ Step, Value });
	else
		mKeys.back().Value = Value;

	return true;
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
bool lcObjectProperty<T>::SetKeyFrame(lcStep Time, bool KeyFrame)
{
	if (KeyFrame)
	{
		typename std::vector<lcObjectPropertyKey<T>>::const_iterator KeyIt;

		for (KeyIt = mKeys.begin(); KeyIt != mKeys.end(); KeyIt++)
		{
			if (KeyIt->Step == Time)
				return false;
			else if (KeyIt->Step > Time)
				break;
		}

		mKeys.insert(KeyIt, lcObjectPropertyKey<T>{ Time, mValue });

		return true;
	}
	else
	{
		for (typename std::vector<lcObjectPropertyKey<T>>::const_iterator KeyIt = mKeys.begin(); KeyIt != mKeys.end(); KeyIt++)
		{
			if (KeyIt->Step == Time)
			{
				if (mKeys.size() == 1)
					mValue = KeyIt->Value;

				mKeys.erase(KeyIt);

				return true;
			}
			else if (KeyIt->Step > Time)
				return false;
		}
	}

	return false;
}

template<typename T>
void lcObjectProperty<T>::Save(QTextStream& Stream, const char* ObjectName, const char* VariableName, bool SaveEmpty) const
{
	if (mKeys.empty())
	{
		if (SaveEmpty)
		{
			Stream << QLatin1String("0 !LEOCAD ") << ObjectName << ' ' << VariableName << ' ';

			lcObjectPropertySaveValue(Stream, mValue);

			Stream << QLatin1String("\r\n");
		}
	}
	else
	{
		for (const lcObjectPropertyKey<T>& Key : mKeys)
		{
			Stream << QLatin1String("0 !LEOCAD ") << ObjectName << ' ' << VariableName << "_KEY " << Key.Step << ' ';

			lcObjectPropertySaveValue(Stream, Key.Value);

			Stream << QLatin1String("\r\n");
		}
	}
}

template<typename T>
bool lcObjectProperty<T>::Load(QTextStream& Stream, const QString& Token, const char* VariableName)
{
	if (Token == VariableName)
	{
		lcObjectPropertyLoadValue(Stream, mValue);

		return true;
	}

	if (Token.endsWith(QLatin1String("_KEY")) && Token.left(Token.size() - 4) == VariableName)
	{
		QString StepString;
		Stream >> StepString;

		const int Step = StepString.toInt();
		T Value;

		constexpr int Count = sizeof(T) / sizeof(float);

		for (int ValueIdx = 0; ValueIdx < Count; ValueIdx++)
			Stream >> ((float*)&Value)[ValueIdx];

		ChangeKey(Value, Step, true);

		return true;
	}

	return false;
}

template<typename T>
void lcObjectProperty<T>::SaveToDataStream(QDataStream& Stream) const
{
	size_t KeyCount = mKeys.size();
	size_t DataSize = KeyCount * sizeof(lcObjectPropertyKey<T>);
	
	Stream.writeRawData(reinterpret_cast<const char*>(&mValue), sizeof(mValue));
	Stream.writeRawData(reinterpret_cast<const char*>(&KeyCount), sizeof(KeyCount));
	Stream.writeRawData(reinterpret_cast<const char*>(mKeys.data()), DataSize);
}

template<typename T>
bool lcObjectProperty<T>::LoadFromDataStream(QDataStream& Stream)
{
	if (Stream.readRawData(reinterpret_cast<char*>(&mValue), sizeof(mValue)) != sizeof(mValue))
		return false;
	
	size_t KeyCount;
	
	if (Stream.readRawData(reinterpret_cast<char*>(&KeyCount), sizeof(KeyCount)) != sizeof(KeyCount))
		return false;
	
	mKeys.resize(KeyCount);
	
	qsizetype DataSize = KeyCount * sizeof(lcObjectPropertyKey<T>);
	
	if (Stream.readRawData(reinterpret_cast<char*>(mKeys.data()), DataSize) != DataSize)
		return false;
	
	return true;	
}

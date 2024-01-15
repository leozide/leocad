#pragma once

#include "lc_array.h"

enum class lcObjectPropertyId
{
	PieceId,
	PieceColor,
	PieceStepShow,
	PieceStepHide,
	CameraName,
	CameraType,
	CameraFOV,
	CameraNear,
	CameraFar,
	CameraPositionX,
	CameraPositionY,
	CameraPositionZ,
	CameraTargetX,
	CameraTargetY,
	CameraTargetZ,
	CameraUpX,
	CameraUpY,
	CameraUpZ,
	LightName,
	LightType,
	LightColor,
	LightPower,
	LightCastShadow,
	LightAttenuationDistance,
	LightAttenuationPower,
	LightPointSize,
	LightSpotSize,
	LightDirectionalSize,
	LightAreaSize,
	LightAreaSizeX,
	LightAreaSizeY,
	LightSpotConeAngle,
	LightSpotPenumbraAngle,
	LightSpotTightness,
	LightAreaShape,
	LightAreaGridX,
	LightAreaGridY,
	ObjectPositionX,
	ObjectPositionY,
	ObjectPositionZ,
	ObjectRotationX,
	ObjectRotationY,
	ObjectRotationZ,
	Count
};

template<typename T>
struct lcObjectPropertyKey
{
	lcStep Step;
	T Value;
};

template<typename T>
class lcObjectProperty
{
public:
	explicit lcObjectProperty(const T& DefaultValue)
		: mValue(DefaultValue)
	{
		ChangeKey(mValue, 1, true);
	}

	operator const T& () const
	{
		return mValue;
	}

	int GetSize() const
	{
		return static_cast<int>(mKeys.size());
	}

	bool IsEmpty() const
	{
		return mKeys.empty();
	}

	void Update(lcStep Step)
	{
		mValue = CalculateKey(Step);
	}

	void Reset()
	{
		mKeys.clear();
		ChangeKey(mValue, 1, true);
	}

	void Reset(const T& Value)
	{
		mValue = Value;
		Reset();
	}

	void ChangeKey(const T& Value, lcStep Step, bool AddKey);
	void InsertTime(lcStep Start, lcStep Time);
	void RemoveTime(lcStep Start, lcStep Time);
	bool HasKeyFrame(lcStep Time) const;

	void Save(QTextStream& Stream, const char* ObjectName, const char* VariableName) const;
	bool Load(QTextStream& Stream, const QString& Token, const char* VariableName);
	void SaveKeysLDraw(QTextStream& Stream, const char* ObjectName, const char* VariableName) const;
	void LoadKeysLDraw(QTextStream& Stream);

protected:
	const T& CalculateKey(lcStep Step) const;

	T mValue;
	std::vector<lcObjectPropertyKey<T>> mKeys;
};

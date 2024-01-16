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
	}

	operator const T& () const
	{
		return mValue;
	}

	void SetValue(const T& Value)
	{
		mValue = Value;
	}

	void RemoveAllKeys()
	{
		mKeys.clear();
	}

	void Update(lcStep Step);
	void ChangeKey(const T& Value, lcStep Step, bool AddKey);
	void InsertTime(lcStep Start, lcStep Time);
	void RemoveTime(lcStep Start, lcStep Time);
	bool HasKeyFrame(lcStep Time) const;

	void Save(QTextStream& Stream, const char* ObjectName, const char* VariableName, bool SaveEmpty) const;
	bool Load(QTextStream& Stream, const QString& Token, const char* VariableName);

protected:
	T mValue;
	std::vector<lcObjectPropertyKey<T>> mKeys;
};

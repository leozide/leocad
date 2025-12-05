#pragma once

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
	LightBlenderPower,
	LightPOVRayPower,
	LightCastShadow,
	LightPOVRayFadeDistance,
	LightPOVRayFadePower,
	LightBlenderRadius,
	LightBlenderAngle,
	LightAreaSizeX,
	LightAreaSizeY,
	LightSpotConeAngle,
	LightSpotPenumbraAngle,
	LightPOVRaySpotTightness,
	LightAreaShape,
	LightPOVRayAreaGridX,
	LightPOVRayAreaGridY,
	ObjectPositionX,
	ObjectPositionY,
	ObjectPositionZ,
	ObjectRotationX,
	ObjectRotationY,
	ObjectRotationZ,
	Count
};

enum class lcFloatPropertySnap
{
	Auto,
	PiecePositionXY,
	PiecePositionZ,
	Position,
	Rotation
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
	bool ChangeKey(const T& Value, lcStep Step, bool AddKey);
	void InsertTime(lcStep Start, lcStep Time);
	void RemoveTime(lcStep Start, lcStep Time);
	bool HasKeyFrame(lcStep Time) const;
	bool SetKeyFrame(lcStep Time, bool KeyFrame);

	void Save(QTextStream& Stream, const char* ObjectName, const char* VariableName, bool SaveEmpty) const;
	bool Load(QTextStream& Stream, const QString& Token, const char* VariableName);
	bool SaveUndoData(QDataStream& Stream) const;
	bool LoadUndoData(QDataStream& Stream);

protected:
	T mValue;
	std::vector<lcObjectPropertyKey<T>> mKeys;
};

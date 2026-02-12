#pragma once

#include "object.h"
#include "lc_math.h"

enum class lcViewpoint
{
	Front,
	Back,
	Top,
	Bottom,
	Left,
	Right,
	Home,
	Count
};

enum class lcCameraProjection
{
	Perspective,
	Orthographic,
	Count
};

enum lcCameraSection : quint32
{
	LC_CAMERA_SECTION_INVALID = LC_OBJECT_SECTION_INVALID,
	LC_CAMERA_SECTION_POSITION = 0,
	LC_CAMERA_SECTION_TARGET,
	LC_CAMERA_SECTION_UPVECTOR
};

struct lcCameraHistoryState
{
	lcObjectId Id;
	bool Hidden;
	bool Simple;
	float Fovy;
	float NearPlane;
	float FarPlane;
	lcCameraProjection Projection;
	lcObjectProperty<lcVector3> Position;
	lcObjectProperty<lcVector3> TargetPosition;
	lcObjectProperty<lcVector3> UpVector;
	QString Name;
	
	bool operator==(const lcCameraHistoryState& Other) const
	{
		return Id == Other.Id && Hidden == Other.Hidden && Simple == Other.Simple && Fovy == Other.Fovy &&
            NearPlane == Other.NearPlane && FarPlane == Other.FarPlane && Projection == Other.Projection &&
            Position == Other.Position && TargetPosition == Other.TargetPosition && UpVector == Other.UpVector && Name == Other.Name;
	}
};

class lcCamera : public lcObject
{
public:
	lcCamera();
	lcCamera(bool Simple);
	lcCamera(bool Simple, const lcVector3& Position, const lcVector3& TargetPosition);
	virtual ~lcCamera();

	lcCamera(const lcCamera&) = delete;
	lcCamera(lcCamera&&) = delete;
	lcCamera& operator=(const lcCamera&) = delete;
	lcCamera& operator=(lcCamera&&) = delete;

	static QString GetCameraProjectionString(lcCameraProjection CameraProjection);
	static QStringList GetCameraProjectionStrings();
	static lcViewpoint GetViewpoint(const QString& ViewpointName);

	void CopyProperties(const lcCamera& Other);

	QString GetName() const override
	{
		return mName;
	}

	bool SetName(const QString& Name);
	void CreateName(const std::vector<std::unique_ptr<lcCamera>>& Cameras);

	bool IsSimple() const
	{
		return mSimple;
	}

	lcCameraProjection GetProjection() const
	{
		return mProjection;
	}

	bool SetProjection(lcCameraProjection CameraProjection);

	quint32 GetAllowedTransforms() const override
	{
		return LC_OBJECT_TRANSFORM_MOVE_XYZ;
	}

	lcVector3 GetSectionPosition(quint32 Section) const override
	{
		switch (Section)
		{
		case LC_CAMERA_SECTION_POSITION:
			return mPosition;

		case LC_CAMERA_SECTION_TARGET:
			return mTargetPosition;

		case LC_CAMERA_SECTION_UPVECTOR:
			return lcMul31(lcVector3(0, 25, 0), lcMatrix44AffineInverse(mWorldView));
		}

		return lcVector3(0.0f, 0.0f, 0.0f);
	}

	lcMatrix33 GetRelativeRotation() const
	{
		const quint32 Section = GetFocusSection();

		if (Section == LC_CAMERA_SECTION_POSITION)
			return lcMatrix33(mWorldView);
		else
			return lcMatrix33Identity();
	}

	lcVector3 GetRotationCenter() const
	{
		const quint32 Section = GetFocusSection();

		if (Section != LC_CAMERA_SECTION_TARGET)
		{
			return mPosition;
		}
		else
		{
			return mTargetPosition;
		}
	}

	void SaveLDraw(QTextStream& Stream) const;
	bool ParseLDrawLine(QTextStream& Stream);

public:
	bool IsVisible() const
	{
		return !mHidden;
	}

	bool IsHidden() const
	{
		return mHidden;
	}

	void SetHidden(bool Hidden)
	{
		mHidden = Hidden;
	}

	void SetPosition(const lcVector3& Position, lcStep Step, bool AddKey)
	{
		mPosition.ChangeKey(Position, Step, AddKey);
	}

	void SetTargetPosition(const lcVector3& TargetPosition, lcStep Step, bool AddKey)
	{
		mTargetPosition.ChangeKey(TargetPosition, Step, AddKey);
	}

	void SetUpVector(const lcVector3& UpVector, lcStep Step, bool AddKey)
	{
		mUpVector.ChangeKey(UpVector, Step, AddKey);
	}

	float GetOrthoHeight() const
	{
		// Compute the FOV/plane intersection radius.
		//                d               d
		//   a = 2 atan(------) => ~ a = --- => d = af
		//                2f              f
		const float f = (mPosition - mTargetPosition).Length();
		return (m_fovy * f) * (LC_PI / 180.0f);
	}

public:
	void RayTest(lcObjectRayTest& ObjectRayTest) const override;
	void BoxTest(lcObjectBoxTest& ObjectBoxTest) const override;
	void DrawInterface(lcContext* Context, const lcScene& Scene) const override;
	QVariant GetPropertyValue(lcObjectPropertyId PropertyId) const override;
	bool SetPropertyValue(lcObjectPropertyId PropertyId, lcStep Step, bool AddKey, QVariant Value) override;
	bool HasKeyFrame(lcObjectPropertyId PropertyId, lcStep Time) const override;
	bool SetKeyFrame(lcObjectPropertyId PropertyId, lcStep Time, bool KeyFrame) override;
	void RemoveKeyFrames() override;
	lcCameraHistoryState GetHistoryState(const lcModel* Model) const;
	void SetHistoryState(const lcCameraHistoryState& State, const lcModel* Model);

	void InsertTime(lcStep Start, lcStep Time);
	void RemoveTime(lcStep Start, lcStep Time);

	static bool FileLoad(lcFile& file);

	void CompareBoundingBox(lcVector3& Min, lcVector3& Max);
	void UpdatePosition(lcStep Step) override;
	void CopyPosition(const lcCamera* Camera);
	void CopySettings(const lcCamera* Camera);

	void ZoomExtents(float AspectRatio, const lcVector3& Center, const std::vector<lcVector3>& Points, lcStep Step, bool AddKey);
	void ZoomRegion(float AspectRatio, const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3* Corners, lcStep Step, bool AddKey);
	void Zoom(float Distance, lcStep Step, bool AddKey);
	void Pan(const lcVector3& Distance, lcStep Step, bool AddKey);
	void Orbit(float DistanceX, float DistanceY, const lcVector3& CenterPosition, lcStep Step, bool AddKey);
	void Roll(float Distance, lcStep Step, bool AddKey);
	void Center(const lcVector3& NewCenter, lcStep Step, bool AddKey);
	void MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance);
	void MoveRelative(const lcVector3& Distance, lcStep Step, bool AddKey);
	void Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcVector3& Center, const lcMatrix33& RotationFrame);
	void SetViewpoint(lcViewpoint Viewpoint);
	void SetViewpoint(const lcVector3& Position);
	void SetViewpoint(const lcVector3& Position, const lcVector3& Target, const lcVector3& Up);
	void GetAngles(float& Latitude, float& Longitude, float& Distance) const;
	void SetAngles(float Latitude, float Longitude, float Distance);

	float m_fovy = 30.0f;
	float m_zNear = 25.0f;
	float m_zFar = 50000;

	lcMatrix44 mWorldView;
	lcObjectProperty<lcVector3> mPosition = lcObjectProperty<lcVector3>(lcVector3(0.0f, 0.0f, 0.0f));
	lcObjectProperty<lcVector3> mTargetPosition = lcObjectProperty<lcVector3>(lcVector3(0.0f, 0.0f, 0.0f));
	lcObjectProperty<lcVector3> mUpVector = lcObjectProperty<lcVector3>(lcVector3(0.0f, 0.0f, 0.0f));

protected:
	QString mName;
	bool mSimple = true;
	lcCameraProjection mProjection = lcCameraProjection::Perspective;
};

#pragma once

#include "object.h"
#include "lc_math.h"
#include "lc_array.h"

#define LC_CAMERA_HIDDEN            0x0001
#define LC_CAMERA_SIMPLE            0x0002
#define LC_CAMERA_ORTHO             0x0004
#define LC_CAMERA_POSITION_SELECTED 0x0010
#define LC_CAMERA_POSITION_FOCUSED  0x0020
#define LC_CAMERA_TARGET_SELECTED   0x0040
#define LC_CAMERA_TARGET_FOCUSED    0x0080
#define LC_CAMERA_UPVECTOR_SELECTED 0x0100
#define LC_CAMERA_UPVECTOR_FOCUSED  0x0200

#define LC_CAMERA_SELECTION_MASK    (LC_CAMERA_POSITION_SELECTED | LC_CAMERA_TARGET_SELECTED | LC_CAMERA_UPVECTOR_SELECTED)
#define LC_CAMERA_FOCUS_MASK        (LC_CAMERA_POSITION_FOCUSED | LC_CAMERA_TARGET_FOCUSED | LC_CAMERA_UPVECTOR_FOCUSED)

enum class lcViewpoint
{
	Front,
	Back,
	Top,
	Bottom,
	Left,
	Right,
	Home
};

enum lcCameraSection
{
	LC_CAMERA_SECTION_POSITION,
	LC_CAMERA_SECTION_TARGET,
	LC_CAMERA_SECTION_UPVECTOR
};

class lcCamera : public lcObject
{
public:
	lcCamera(bool Simple);
	lcCamera(float ex, float ey, float ez, float tx, float ty, float tz);
	~lcCamera();

	lcCamera(const lcCamera&) = delete;
	lcCamera(lcCamera&&) = delete;
	lcCamera& operator=(const lcCamera&) = delete;
	lcCamera& operator=(lcCamera&&) = delete;

	const char* GetName() const override
	{
		return m_strName;
	}

	void CreateName(const lcArray<lcCamera*>& Cameras);

	bool IsSimple() const
	{
		return (mState & LC_CAMERA_SIMPLE) != 0;
	}

	bool IsOrtho() const
	{
		return (mState & LC_CAMERA_ORTHO) != 0;
	}

	void SetOrtho(bool Ortho)
	{
		if (Ortho)
			mState |= LC_CAMERA_ORTHO;
		else
			mState &= ~LC_CAMERA_ORTHO;
	}

	bool IsSelected() const override
	{
		return (mState & LC_CAMERA_SELECTION_MASK) != 0;
	}

	bool IsSelected(quint32 Section) const override
	{
		switch (Section)
		{
		case LC_CAMERA_SECTION_POSITION:
			return (mState & LC_CAMERA_POSITION_SELECTED) != 0;
			break;

		case LC_CAMERA_SECTION_TARGET:
			return (mState & LC_CAMERA_TARGET_SELECTED) != 0;
			break;

		case LC_CAMERA_SECTION_UPVECTOR:
			return (mState & LC_CAMERA_UPVECTOR_SELECTED) != 0;
			break;
		}
		return false;
	}

	void SetSelected(bool Selected) override
	{
		if (Selected)
			mState |= LC_CAMERA_SELECTION_MASK;
		else
			mState &= ~(LC_CAMERA_SELECTION_MASK | LC_CAMERA_FOCUS_MASK);
	}

	void SetSelected(quint32 Section, bool Selected) override
	{
		switch (Section)
		{
		case LC_CAMERA_SECTION_POSITION:
			if (Selected)
				mState |= LC_CAMERA_POSITION_SELECTED;
			else
				mState &= ~(LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED);
			break;

		case LC_CAMERA_SECTION_TARGET:
			if (Selected)
				mState |= LC_CAMERA_TARGET_SELECTED;
			else
				mState &= ~(LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED);
			break;

		case LC_CAMERA_SECTION_UPVECTOR:
			if (Selected)
				mState |= LC_CAMERA_UPVECTOR_SELECTED;
			else
				mState &= ~(LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED);
			break;
		}
	}

	bool IsFocused() const override
	{
		return (mState & LC_CAMERA_FOCUS_MASK) != 0;
	}

	bool IsFocused(quint32 Section) const override
	{
		switch (Section)
		{
		case LC_CAMERA_SECTION_POSITION:
			return (mState & LC_CAMERA_POSITION_FOCUSED) != 0;
			break;

		case LC_CAMERA_SECTION_TARGET:
			return (mState & LC_CAMERA_TARGET_FOCUSED) != 0;
			break;

		case LC_CAMERA_SECTION_UPVECTOR:
			return (mState & LC_CAMERA_UPVECTOR_FOCUSED) != 0;
			break;
		}
		return false;
	}

	void SetFocused(quint32 Section, bool Focus) override
	{
		switch (Section)
		{
		case LC_CAMERA_SECTION_POSITION:
			if (Focus)
				mState |= LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED;
			else
				mState &= ~(LC_CAMERA_POSITION_SELECTED | LC_CAMERA_POSITION_FOCUSED);
			break;

		case LC_CAMERA_SECTION_TARGET:
			if (Focus)
				mState |= LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED;
			else
				mState &= ~(LC_CAMERA_TARGET_SELECTED | LC_CAMERA_TARGET_FOCUSED);
			break;

		case LC_CAMERA_SECTION_UPVECTOR:
			if (Focus)
				mState |= LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED;
			else
				mState &= ~(LC_CAMERA_UPVECTOR_SELECTED | LC_CAMERA_UPVECTOR_FOCUSED);
			break;
		}
	}

	quint32 GetFocusSection() const override
	{
		if (mState & LC_CAMERA_POSITION_FOCUSED)
			return LC_CAMERA_SECTION_POSITION;

		if (mState & LC_CAMERA_TARGET_FOCUSED)
			return LC_CAMERA_SECTION_TARGET;

		if (mState & LC_CAMERA_UPVECTOR_FOCUSED)
			return LC_CAMERA_SECTION_UPVECTOR;

		return ~0U;
	}

	quint32 GetAllowedTransforms() const override
	{
		return LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z;
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

	void SaveLDraw(QTextStream& Stream) const;
	bool ParseLDrawLine(QTextStream& Stream);

public:
	bool IsVisible() const
		{ return (mState & LC_CAMERA_HIDDEN) == 0; }

	bool IsHidden() const
	{
		return (mState & LC_CAMERA_HIDDEN) != 0;
	}

	void SetHidden(bool Hidden)
	{
		if (Hidden)
			mState |= LC_CAMERA_HIDDEN;
		else
			mState &= ~LC_CAMERA_HIDDEN;
	}

	void SetPosition(const lcVector3& Position, lcStep Step, bool AddKey)
	{
		ChangeKey(mPositionKeys, Position, Step, AddKey);
	}

	void SetTargetPosition(const lcVector3& TargetPosition, lcStep Step, bool AddKey)
	{
		ChangeKey(mTargetPositionKeys, TargetPosition, Step, AddKey);
	}

	void SetUpVector(const lcVector3& UpVector, lcStep Step, bool AddKey)
	{
		ChangeKey(mPositionKeys, UpVector, Step, AddKey);
	}

	float GetOrthoHeight() const
	{
		// Compute the FOV/plane intersection radius.
		//                d               d
		//   a = 2 atan(------) => ~ a = --- => d = af
		//                2f              f
		float f = (mPosition - mTargetPosition).Length();
		return (m_fovy * f) * (LC_PI / 180.0f);
	}

public:
	void RayTest(lcObjectRayTest& ObjectRayTest) const override;
	void BoxTest(lcObjectBoxTest& ObjectBoxTest) const override;
	void DrawInterface(lcContext* Context, const lcScene& Scene) const override;
	void RemoveKeyFrames() override;

	void InsertTime(lcStep Start, lcStep Time);
	void RemoveTime(lcStep Start, lcStep Time);

	bool FileLoad(lcFile& file);

	void CompareBoundingBox(lcVector3& Min, lcVector3& Max);
	void UpdatePosition(lcStep Step);
	void CopyPosition(const lcCamera* Camera);
	void CopySettings(const lcCamera* Camera);

	void ZoomExtents(float AspectRatio, const lcVector3& Center, const lcVector3* Points, int NumPoints, lcStep Step, bool AddKey);
	void ZoomRegion(float AspectRatio, const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3* Corners, lcStep Step, bool AddKey);
	void Zoom(float Distance, lcStep Step, bool AddKey);
	void Pan(const lcVector3& Distance, lcStep Step, bool AddKey);
	void Orbit(float DistanceX, float DistanceY, const lcVector3& CenterPosition, lcStep Step, bool AddKey);
	void Roll(float Distance, lcStep Step, bool AddKey);
	void Center(const lcVector3& NewCenter, lcStep Step, bool AddKey);
	void MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance);
	void MoveRelative(const lcVector3& Distance, lcStep Step, bool AddKey);
	void SetViewpoint(lcViewpoint Viewpoint);
	void SetViewpoint(const lcVector3& Position);
	void SetViewpoint(const lcVector3& Position, const lcVector3& Target, const lcVector3& Up);
	void GetAngles(float& Latitude, float& Longitude, float& Distance) const;
	void SetAngles(float Latitude, float Longitude, float Distance);

	char m_strName[81];

	float m_fovy;
	float m_zNear;
	float m_zFar;

	lcMatrix44 mWorldView;
	lcVector3 mPosition;
	lcVector3 mTargetPosition;
	lcVector3 mUpVector;

protected:
	lcArray<lcObjectKey<lcVector3>> mPositionKeys;
	lcArray<lcObjectKey<lcVector3>> mTargetPositionKeys;
	lcArray<lcObjectKey<lcVector3>> mUpVectorKeys;

	void Initialize();

	quint32 mState;
};


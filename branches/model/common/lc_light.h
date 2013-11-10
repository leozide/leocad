#ifndef _LC_LIGHT_H_
#define _LC_LIGHT_H_

#include "lc_object.h"
#include "lc_math.h"

#define LC_LIGHT_HIDDEN            0x0001
#define LC_LIGHT_DISABLED          0x0002
#define LC_LIGHT_SPOT              0x0004
#define LC_LIGHT_DIRECTIONAL       0x0008
#define LC_LIGHT_POSITION_SELECTED 0x0010
#define LC_LIGHT_POSITION_FOCUSED  0x0020
#define LC_LIGHT_TARGET_SELECTED   0x0040
#define LC_LIGHT_TARGET_FOCUSED    0x0080

#define LC_LIGHT_SELECTION_MASK    (LC_LIGHT_POSITION_SELECTED | LC_LIGHT_TARGET_SELECTED)
#define LC_LIGHT_FOCUS_MASK        (LC_LIGHT_POSITION_FOCUSED | LC_LIGHT_TARGET_FOCUSED)

enum lcLightSection
{
	LC_LIGHT_POSITION,
	LC_LIGHT_TARGET
};

class lcLight : public lcObject
{
public:
	lcLight(const lcVector3& Position);
	virtual ~lcLight();

	bool IsPointLight() const
	{
		return (mState & (LC_LIGHT_SPOT | LC_LIGHT_DIRECTIONAL)) == 0;
	}

	bool IsSpotLight() const
	{
		return (mState & LC_LIGHT_SPOT) != 0;
	}

	bool IsDirectionalLight() const
	{
		return (mState & LC_LIGHT_DIRECTIONAL) != 0;
	}

	virtual bool IsVisible() const
	{
		return (mState & LC_LIGHT_HIDDEN) == 0;
	}

	virtual void SetVisible(bool Visible)
	{
		if (Visible)
			mState &= ~LC_LIGHT_HIDDEN;
		else
		{
			mState |= LC_LIGHT_HIDDEN;
			mState &= ~(LC_LIGHT_SELECTION_MASK | LC_LIGHT_FOCUS_MASK);
		}
	}

	virtual bool IsSelected() const
	{
		return (mState & LC_LIGHT_SELECTION_MASK) != 0;
	}

	virtual bool IsSelected(lcuintptr Section) const
	{
		switch (Section)
		{
		case LC_LIGHT_POSITION:
			return (mState & LC_LIGHT_POSITION_SELECTED) != 0;
			break;

		case LC_LIGHT_TARGET:
			return (mState & LC_LIGHT_TARGET_SELECTED) != 0;
			break;
		}
		return false;
	}

	virtual bool IsFocused() const
	{
		return (mState & LC_LIGHT_FOCUS_MASK) != 0;
	}

	virtual bool IsFocused(lcuintptr Section) const
	{
		switch (Section)
		{
		case LC_LIGHT_POSITION:
			return (mState & LC_LIGHT_POSITION_FOCUSED) != 0;
			break;

		case LC_LIGHT_TARGET:
			return (mState & LC_LIGHT_TARGET_FOCUSED) != 0;
			break;
		}
		return false;
	}

	virtual void SetSelection(bool Selection)
	{
		if (Selection)
		{
			if (IsPointLight())
				mState |= LC_LIGHT_POSITION_SELECTED;
			else
				mState |= LC_LIGHT_SELECTION_MASK;
		}
		else
			mState &= ~(LC_LIGHT_SELECTION_MASK | LC_LIGHT_FOCUS_MASK);
	}

	virtual void SetSelection(lcuintptr Section, bool Selection)
	{
		switch (Section)
		{
		case LC_LIGHT_POSITION:
			if (Selection)
				mState |= LC_LIGHT_POSITION_SELECTED;
			else
				mState &= ~(LC_LIGHT_POSITION_SELECTED | LC_LIGHT_POSITION_FOCUSED);
			break;

		case LC_LIGHT_TARGET:
			if (Selection)
			{
				if (!IsPointLight())
					mState |= LC_LIGHT_TARGET_SELECTED;
			}
			else
				mState &= ~(LC_LIGHT_TARGET_SELECTED | LC_LIGHT_TARGET_FOCUSED);
			break;
		}
	}

	virtual void ClearFocus()
	{
		mState &= ~LC_LIGHT_FOCUS_MASK;
	}

	virtual void SetFocus(lcuintptr Section, bool Focus)
	{
		switch (Section)
		{
		case LC_LIGHT_POSITION:
			if (Focus)
				mState |= LC_LIGHT_POSITION_SELECTED | LC_LIGHT_POSITION_FOCUSED;
			else
				mState &= ~(LC_LIGHT_POSITION_SELECTED | LC_LIGHT_POSITION_FOCUSED);
			break;

		case LC_LIGHT_TARGET:
			if (Focus)
			{
				if (!IsPointLight())
					mState |= LC_LIGHT_TARGET_SELECTED | LC_LIGHT_TARGET_FOCUSED;
			}
			else
				mState &= ~(LC_LIGHT_TARGET_SELECTED | LC_LIGHT_TARGET_FOCUSED);
			break;
		}
	}

	virtual lcuintptr GetFocusSection() const
	{
		if (mState & LC_LIGHT_POSITION_FOCUSED)
			return LC_LIGHT_POSITION;

		if (!IsPointLight() && (mState & LC_LIGHT_TARGET_FOCUSED))
			return LC_LIGHT_TARGET;

		return ~0;
	}

	virtual void InvertSelection()
	{
		if (mState & LC_LIGHT_POSITION_SELECTED)
			mState &= ~(LC_LIGHT_POSITION_SELECTED | LC_LIGHT_POSITION_FOCUSED);
		else
			mState |= LC_LIGHT_POSITION_SELECTED;

		if (mState & LC_LIGHT_TARGET_SELECTED)
			mState &= ~(LC_LIGHT_TARGET_SELECTED | LC_LIGHT_TARGET_FOCUSED);
		else if (!IsPointLight())
			mState |= LC_LIGHT_TARGET_SELECTED;
	}

	virtual void InvertSelection(lcuintptr Section)
	{
		switch (Section)
		{
		case LC_LIGHT_POSITION:
			if (mState & LC_LIGHT_POSITION_SELECTED)
				mState &= ~(LC_LIGHT_POSITION_SELECTED | LC_LIGHT_POSITION_FOCUSED);
			else
				mState |= LC_LIGHT_POSITION_SELECTED;
			break;

		case LC_LIGHT_TARGET:
			if (mState & LC_LIGHT_TARGET_SELECTED)
				mState &= ~(LC_LIGHT_TARGET_SELECTED | LC_LIGHT_TARGET_FOCUSED);
			else if (!IsPointLight())
				mState |= LC_LIGHT_TARGET_SELECTED;
			break;
		}
	}

	virtual void SetCurrentTime(lcTime Time)
	{
		if (mPositionKeys.GetSize())
			mPosition = CalculateKey(mPositionKeys, Time);

		if (mTargetPositionKeys.GetSize())
			mTargetPosition = CalculateKey(mTargetPositionKeys, Time);

		if (mAmbientColorKeys.GetSize())
			mAmbientColor = CalculateKey(mAmbientColorKeys, Time);

		if (mDiffuseColorKeys.GetSize())
			mDiffuseColor = CalculateKey(mDiffuseColorKeys, Time);

		if (mSpecularColorKeys.GetSize())
			mSpecularColor = CalculateKey(mSpecularColorKeys, Time);

		if (mAttenuationKeys.GetSize())
			mAttenuation = CalculateKey(mAttenuationKeys, Time);

		if (mSpotCutoffKeys.GetSize())
			mSpotCutoff = CalculateKey(mSpotCutoffKeys, Time);

		if (mSpotExponentKeys.GetSize())
			mSpotExponent = CalculateKey(mSpotExponentKeys, Time);

		Update();
	}

	virtual void Save(lcFile& File);
	virtual void Load(lcFile& File);
	virtual void Update();

	virtual void ClosestHitTest(lcObjectHitTest& HitTest);
	virtual void BoxTest(lcObjectBoxTest& BoxTest);

	virtual void GetRenderMeshes(View* View, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes, lcArray<lcObject*>& InterfaceObjects);
	virtual void RenderInterface(View* View) const;

	virtual lcVector3 GetFocusPosition() const
	{
		if (mState & LC_LIGHT_POSITION_FOCUSED)
			return mPosition;

		if (!IsPointLight() && (mState & LC_LIGHT_TARGET_FOCUSED))
			return mTargetPosition;

		return lcVector3(0.0f, 0.0f, 0.0f);
	}

	virtual void Move(const lcVector3& Distance, lcTime Time, bool AddKey);

	bool Setup(int LightIndex);

	lcMatrix44 mWorldLight;
 	lcVector3 mPosition;
	lcVector3 mTargetPosition;
	lcVector4 mAmbientColor;
	lcVector4 mDiffuseColor;
	lcVector4 mSpecularColor;
	lcVector3 mAttenuation;
	float mSpotCutoff;
	float mSpotExponent;

	lcuint32 mState;
	char mName[81];

protected:
	void DrawPointLight() const;
	void DrawSpotLight() const;

	lcArray<lcObjectVector3Key> mPositionKeys;
	lcArray<lcObjectVector3Key> mTargetPositionKeys;
	lcArray<lcObjectVector4Key> mAmbientColorKeys;
	lcArray<lcObjectVector4Key> mDiffuseColorKeys;
	lcArray<lcObjectVector4Key> mSpecularColorKeys;
	lcArray<lcObjectVector3Key> mAttenuationKeys;
	lcArray<lcObjectFloatKey> mSpotCutoffKeys;
	lcArray<lcObjectFloatKey> mSpotExponentKeys;
};

#endif // _LC_LIGHT_H_

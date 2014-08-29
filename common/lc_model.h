#ifndef _LC_MODEL_H_
#define _LC_MODEL_H_

#include "lc_file.h"
#include "lc_math.h"
#include "str.h"
#include "object.h"

#define LC_SEL_NO_PIECES     0x01 // No pieces in model
#define LC_SEL_PIECE         0x02 // At last 1 piece selected
#define LC_SEL_SELECTED      0x04 // At last 1 object selected
#define LC_SEL_UNSELECTED    0x08 // At least 1 piece unselected
#define LC_SEL_HIDDEN        0x10 // At least one piece hidden
#define LC_SEL_GROUPED       0x20 // At least one piece selected is grouped
#define LC_SEL_FOCUS_GROUPED 0x40 // Focused piece is grouped
#define LC_SEL_CAN_GROUP     0x80 // Can make a new group

enum lcTransformType
{
	LC_TRANSFORM_ABSOLUTE_TRANSLATION,
	LC_TRANSFORM_RELATIVE_TRANSLATION,
	LC_TRANSFORM_ABSOLUTE_ROTATION,
	LC_TRANSFORM_RELATIVE_ROTATION
};

enum lcBackgroundType
{
	LC_BACKGROUND_SOLID,
	LC_BACKGROUND_GRADIENT,
	LC_BACKGROUND_IMAGE
};

class lcModelProperties
{
public:
	void LoadDefaults();
	void SaveDefaults();

	bool operator==(const lcModelProperties& Properties)
	{
		if (mName != Properties.mName || mAuthor != Properties.mAuthor ||
			mDescription != Properties.mDescription || mComments != Properties.mComments)
			return false;

		if (mBackgroundType != Properties.mBackgroundType || mBackgroundSolidColor != Properties.mBackgroundSolidColor ||
			mBackgroundGradientColor1 != Properties.mBackgroundGradientColor1 || mBackgroundGradientColor2 != Properties.mBackgroundGradientColor2 ||
			mBackgroundImage != Properties.mBackgroundImage || mBackgroundImageTile != Properties.mBackgroundImageTile)
			return false;

		if (mFogEnabled != Properties.mFogEnabled || mFogDensity != Properties.mFogDensity ||
			mFogColor != Properties.mFogColor || mAmbientColor != Properties.mAmbientColor)
			return false;

		return true;
	}

	QJsonObject Save();
	void Load(QJsonObject Properties);

	String mName;
	String mAuthor;
	String mDescription;
	String mComments;

	lcBackgroundType mBackgroundType;
	lcVector3 mBackgroundSolidColor;
	lcVector3 mBackgroundGradientColor1;
	lcVector3 mBackgroundGradientColor2;
	String mBackgroundImage;
	bool mBackgroundImageTile;

	bool mFogEnabled;
	float mFogDensity;
	lcVector3 mFogColor;
	lcVector3 mAmbientColor;
};

enum lcTool
{
	LC_TOOL_INSERT,
	LC_TOOL_LIGHT,
	LC_TOOL_SPOTLIGHT,
	LC_TOOL_CAMERA,
	LC_TOOL_SELECT,
	LC_TOOL_MOVE,
	LC_TOOL_ROTATE,
	LC_TOOL_ERASER,
	LC_TOOL_PAINT,
	LC_TOOL_ZOOM,
	LC_TOOL_PAN,
	LC_TOOL_ROTATE_VIEW,
	LC_TOOL_ROLL,
	LC_TOOL_ZOOM_REGION
};

struct lcModelHistoryEntry
{
	lcMemFile File;
	char Description[64];
};

class lcModel
{
public:
	lcModel();
	~lcModel();

	bool IsModified() const
	{
		return mSavedHistory != mUndoHistory[0];
	}

	const lcArray<lcPiece*>& GetPieces() const
	{
		return mPieces;
	}

	const lcArray<lcCamera*>& GetCameras() const
	{
		return mCameras;
	}

	const lcArray<lcLight*>& GetLights() const
	{
		return mLights;
	}

	const lcArray<lcGroup*>& GetGroups() const
	{
		return mGroups;
	}

	lcStep GetLastStep() const;

	lcStep GetCurrentStep() const
	{
		return mCurrentStep;
	}

	QJsonObject Save();
	void Load(QJsonObject Model);

	lcObject* GetFocusObject() const;
	void FocusOrDeselectObject(const lcObjectSection& ObjectSection);
	void ClearSelectionAndSetFocus(lcObject* Object, lcuint32 Section);
	void ClearSelectionAndSetFocus(const lcObjectSection& ObjectSection);
	void SetSelection(const lcArray<lcObjectSection>& ObjectSections);
	void AddToSelection(const lcArray<lcObjectSection>& ObjectSections);

protected:
	void UpdateSelection() const;
	void SelectGroup(lcGroup* TopGroup, bool Select);
	void ClearSelection(bool UpdateInterface);

	lcModelProperties mProperties;

	lcStep mCurrentStep;

	lcArray<lcPiece*> mPieces;
	lcArray<lcCamera*> mCameras;
	lcArray<lcLight*> mLights;
	lcArray<lcGroup*> mGroups;

	lcModelHistoryEntry* mSavedHistory;
	lcArray<lcModelHistoryEntry*> mUndoHistory;
	lcArray<lcModelHistoryEntry*> mRedoHistory;
};

#endif // _LC_MODEL_H_

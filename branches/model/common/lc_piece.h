#ifndef LC_PIECE_H
#define LC_PIECE_H

#include "lc_object.h"

#define LC_PIECE_HIDDEN             0x01
#define LC_PIECE_POSITION_SELECTED  0x02
#define LC_PIECE_POSITION_FOCUSED   0x04

#define LC_PIECE_SELECTION_MASK     (LC_PIECE_POSITION_SELECTED)
#define LC_PIECE_FOCUS_MASK         (LC_PIECE_POSITION_FOCUSED)

class PieceInfo;

class lcPiece : public lcObject
{
public:
	lcPiece(PieceInfo* Part, int ColorIndex, const lcVector3& Position, const lcVector4& AxisAngle, lcTime Time);
	virtual ~lcPiece();

	virtual bool IsSelected() const
	{
		return (mState & LC_PIECE_SELECTION_MASK) != 0;
	}

	virtual bool IsSelected(lcuint32 Section) const
	{
		return (mState & LC_PIECE_SELECTION_MASK) != 0;
	}

	virtual bool IsFocused() const
	{
		return (mState & LC_PIECE_FOCUS_MASK) != 0;
	}

	virtual bool IsFocused(lcuint32 Section) const
	{
		return (mState & LC_PIECE_FOCUS_MASK) != 0;
	}

	virtual void ClearSelection()
	{
		mState &= ~(LC_PIECE_SELECTION_MASK | LC_PIECE_FOCUS_MASK);
	}

	virtual void ClearFocus()
	{
		mState &= ~LC_PIECE_FOCUS_MASK;
	}

	virtual void SetSelection(lcuint32 Section, bool Selection)
	{
		if (Selection)
			mState |= LC_PIECE_POSITION_SELECTED;
		else
			mState &= ~(LC_PIECE_SELECTION_MASK | LC_PIECE_FOCUS_MASK);
	}

	virtual void SetFocus(lcuint32 Section, bool Focus)
	{
		if (Focus)
			mState |= LC_PIECE_POSITION_FOCUSED;
		else
			mState &= ~LC_PIECE_FOCUS_MASK;
	}

	virtual void ToggleSelection(lcuint32 Section)
	{
		if (mState & (LC_PIECE_POSITION_SELECTED | LC_PIECE_POSITION_FOCUSED))
			mState &= ~(LC_PIECE_POSITION_SELECTED | LC_PIECE_POSITION_FOCUSED);
		else
			mState |= LC_PIECE_POSITION_SELECTED;
	}

	virtual void SetCurrentTime(lcTime Time)
	{
		if (mPositionKeys.GetSize())
			mPosition = CalculateKey(mPositionKeys, Time);

		if (mAxisAngleKeys.GetSize())
			mAxisAngle = CalculateKey(mAxisAngleKeys, Time);
	}

	virtual void Save(lcFile& File);
	virtual void Load(lcFile& File);
	virtual void Update();

	virtual void ClosestHitTest(lcObjectHitTest& HitTest);
	virtual void BoxTest(const lcVector4* BoxPlanes, lcArray<lcObjectSection>& ObjectSections);

	virtual void GetRenderMeshes(View* View, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes, lcArray<lcObject*> InterfaceObjects);
	virtual void RenderInterface(View* View) const;

	virtual void Move(const lcVector3& Distance, lcTime Time, bool AddKey);

public:
	PieceInfo* mPieceInfo;
	int mColorIndex;

	lcMatrix44 mModelWorld;
	lcVector3 mPosition;
	lcVector4 mAxisAngle;

	lcuint32 mState;

protected:
	lcArray<lcObjectVector3Key> mPositionKeys;
	lcArray<lcObjectVector4Key> mAxisAngleKeys;
};

#endif // LC_PIECE_H

#include "lc_global.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "opengl.h"
#include "pieceinf.h"
#include "piece.h"
#include "group.h"
#include "project.h"
#include "lc_model.h"
#include "lc_file.h"
#include "lc_application.h"
#include "lc_library.h"
#include "lc_context.h"
#include "lc_qutils.h"

#define LC_PIECE_SAVE_VERSION 12 // LeoCAD 0.80

/////////////////////////////////////////////////////////////////////////////
// Piece construction/destruction

lcPiece::lcPiece(PieceInfo* pPieceInfo)
	: lcObject (LC_OBJECT_PIECE)
{
	mPieceInfo = pPieceInfo;
	mState = 0;
	mColorIndex = gDefaultColor;
	mColorCode = 16;
	mStepShow = 1;
	mStepHide = LC_STEP_MAX;
	mGroup = NULL;
	mFileLine = -1;

	if (mPieceInfo != NULL)
		mPieceInfo->AddRef();

	ChangeKey(mPositionKeys, lcVector3(0.0f, 0.0f, 0.0f), 1, true);
	ChangeKey(mRotationKeys, lcMatrix33Identity(), 1, true);
}

lcPiece::~lcPiece()
{
	if (mPieceInfo != NULL)
		mPieceInfo->Release();
}

void lcPiece::SaveLDraw(QTextStream& Stream) const
{
	QLatin1String LineEnding("\r\n");

	if (mStepHide != LC_STEP_MAX)
		Stream << QLatin1String("0 !LEOCAD PIECE STEP_HIDE ") << mStepHide << LineEnding;

	if (IsHidden())
		Stream << QLatin1String("0 !LEOCAD PIECE HIDDEN") << LineEnding;

	if (mPositionKeys.GetSize() > 1)
		SaveKeysLDraw(Stream, mPositionKeys, "PIECE POSITION_KEY ");

	if (mRotationKeys.GetSize() > 1)
		SaveKeysLDraw(Stream, mRotationKeys, "PIECE ROTATION_KEY ");

	Stream << "1 " << mColorCode << ' ';

	const float* Matrix = mModelWorld;
	float Numbers[12] = { Matrix[12], -Matrix[14], Matrix[13], Matrix[0], -Matrix[8], Matrix[4], -Matrix[2], Matrix[10], -Matrix[6], Matrix[1], -Matrix[9], Matrix[5] };

	for (int NumberIdx = 0; NumberIdx < 12; NumberIdx++)
		Stream << lcFormatValue(Numbers[NumberIdx]) << ' ';

	Stream << mPieceInfo->GetSaveID() << LineEnding;
}

bool lcPiece::ParseLDrawLine(QTextStream& Stream)
{
	while (!Stream.atEnd())
	{
		QString Token;
		Stream >> Token;

		if (Token == QLatin1String("STEP_HIDE"))
			Stream >> mStepHide;
		else if (Token == QLatin1String("HIDDEN"))
			SetHidden(true);
		else if (Token == QLatin1String("POSITION_KEY"))
			LoadKeysLDraw(Stream, mPositionKeys);
		else if (Token == QLatin1String("ROTATION_KEY"))
			LoadKeysLDraw(Stream, mRotationKeys);
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Piece save/load

// Use only when loading from a file
void lcPiece::SetPieceInfo(PieceInfo* pPieceInfo)
{
	mPieceInfo = pPieceInfo;
	mPieceInfo->AddRef();
}

bool lcPiece::FileLoad(lcFile& file)
{
	lcuint8 version, ch;

	version = file.ReadU8();

	if (version > LC_PIECE_SAVE_VERSION)
		return false;

	if (version > 8)
	{
		if (file.ReadU8() != 1)
			return false;

		lcuint16 time;
		float param[4];
		lcuint8 type;
		lcuint32 n;

		file.ReadU32(&n, 1);
		while (n--)
		{
			file.ReadU16(&time, 1);
			file.ReadFloats(param, 4);
			file.ReadU8(&type, 1);

			if (type == 0)
				ChangeKey(mPositionKeys, lcVector3(param[0], param[1], param[2]), time, true);
			else if (type == 1)
				ChangeKey(mRotationKeys, lcMatrix33FromAxisAngle(lcVector3(param[0], param[1], param[2]), param[3] * LC_DTOR), time, true);
		}

		file.ReadU32(&n, 1);
		while (n--)
		{
			file.ReadU16(&time, 1);
			file.ReadFloats(param, 4);
			file.ReadU8(&type, 1);
		}
	}

  if (version < 9)
  {
    lcuint16 time;
    lcuint8 type;

    if (version > 5)
    {
      lcuint32 keys;
      float param[4];

      file.ReadU32(&keys, 1);
      while (keys--)
      {
        file.ReadFloats(param, 4);
        file.ReadU16(&time, 1);
        file.ReadU8(&type, 1);

		if (type == 0)
			ChangeKey(mPositionKeys, lcVector3(param[0], param[1], param[2]), time, true);
		else if (type == 1)
			ChangeKey(mRotationKeys, lcMatrix33FromAxisAngle(lcVector3(param[0], param[1], param[2]), param[3] * LC_DTOR), time, true);
      }

      file.ReadU32(&keys, 1);
      while (keys--)
      {
        file.ReadFloats(param, 4);
        file.ReadU16(&time, 1);
        file.ReadU8(&type, 1);
      }
    }
    else
    {
      if (version > 2)
      {
        file.ReadU8(&ch, 1);

        while (ch--)
        {
			lcMatrix44 ModelWorld;

			if (version > 3)
			{
				file.ReadFloats(ModelWorld, 16);
			}
			else
			{
				lcVector3 Translation;
				float Rotation[3];
				file.ReadFloats(Translation, 3);
				file.ReadFloats(Rotation, 3);
				ModelWorld = lcMatrix44Translation(Translation);
				ModelWorld = lcMul(lcMatrix44RotationZ(Rotation[2] * LC_DTOR), lcMul(lcMatrix44RotationY(Rotation[1] * LC_DTOR), lcMul(lcMatrix44RotationX(Rotation[0] * LC_DTOR), ModelWorld)));
			}

			lcuint8 b;
			file.ReadU8(&b, 1);
			time = b;

			ChangeKey(mPositionKeys, ModelWorld.GetTranslation(), 1, true);
			ChangeKey(mRotationKeys, lcMatrix33(ModelWorld), time, true);

			lcint32 bl;
			file.ReadS32(&bl, 1);
        }
      }
      else
      {
			lcVector3 Translation;
			float Rotation[3];
			file.ReadFloats(Translation, 3);
			file.ReadFloats(Rotation, 3);
			lcMatrix44 ModelWorld = lcMatrix44Translation(Translation);
			ModelWorld = lcMul(lcMatrix44RotationZ(Rotation[2] * LC_DTOR), lcMul(lcMatrix44RotationY(Rotation[1] * LC_DTOR), lcMul(lcMatrix44RotationX(Rotation[0] * LC_DTOR), ModelWorld)));

			ChangeKey(mPositionKeys, lcVector3(ModelWorld.r[3][0], ModelWorld.r[3][1], ModelWorld.r[3][2]), 1, true);
			ChangeKey(mRotationKeys, lcMatrix33(ModelWorld), 1, true);
	  }
    }
  }

  // Common to all versions.
	char name[LC_PIECE_NAME_LEN];
  if (version < 10)
  {
	  memset(name, 0, LC_PIECE_NAME_LEN);
	  file.ReadBuffer(name, 9);
  }
  else
	  file.ReadBuffer(name, LC_PIECE_NAME_LEN);

	PieceInfo* pInfo = lcGetPiecesLibrary()->FindPiece(name, NULL, true);
	SetPieceInfo(pInfo);

	// 11 (0.77)
	if (version < 11)
	{
		lcuint8 Color;

		file.ReadU8(&Color, 1);

		if (version < 5)
			mColorCode = lcGetColorCodeFromOriginalColor(Color);
		else
			mColorCode = lcGetColorCodeFromExtendedColor(Color);
	}
	else
		file.ReadU32(&mColorCode, 1);
	mColorIndex = lcGetColorIndex(mColorCode);

  lcuint8 Step;
  file.ReadU8(&Step, 1);
  mStepShow = Step;
  if (version > 1)
  {
    file.ReadU8(&Step, 1);
	mStepHide = Step == 255 ? LC_STEP_MAX : Step;
  }
  else
    mStepHide = LC_STEP_MAX;

  if (version > 5)
  {
	file.ReadU16(); // m_nFrameShow
	file.ReadU16(); // m_nFrameHide

    if (version > 7)
    {
      lcuint8 Hidden;
      file.ReadU8(&Hidden, 1);
	  if (Hidden & 1)
		  mState |= LC_PIECE_HIDDEN;
      file.ReadU8(&ch, 1);
	  file.Seek(ch, SEEK_CUR);
    }
    else
    {
      lcint32 hide;
      file.ReadS32(&hide, 1);
      if (hide != 0)
        mState |= LC_PIECE_HIDDEN;
	  file.Seek(81, SEEK_CUR);
    }

    // 7 (0.64)
    lcint32 i = -1;
    if (version > 6)
      file.ReadS32(&i, 1);
    mGroup = (lcGroup*)(long)i;
  }
  else
  {
    file.ReadU8(&ch, 1);
    if (ch == 0)
      mGroup = (lcGroup*)-1;
    else
      mGroup = (lcGroup*)(long)ch;

    file.ReadU8(&ch, 1);
    if (ch & 0x01)
      mState |= LC_PIECE_HIDDEN;
  }

	if (version < 12)
	{
		for (int KeyIdx = 0; KeyIdx < mPositionKeys.GetSize(); KeyIdx++)
			mPositionKeys[KeyIdx].Value *= 25.0f;
	}

	return true;
}

void lcPiece::Initialize(const lcMatrix44& WorldMatrix, lcStep Step)
{
	mStepShow = Step;

	ChangeKey(mPositionKeys, WorldMatrix.GetTranslation(), 1, true);
	ChangeKey(mRotationKeys, lcMatrix33(WorldMatrix), 1, true);

	UpdatePosition(Step);
}

void lcPiece::InsertTime(lcStep Start, lcStep Time)
{
	if (mStepShow >= Start)
	{
		if (mStepShow < LC_STEP_MAX - Time)
			mStepShow += Time;
		else
			mStepShow = LC_STEP_MAX;
	}

	if (mStepHide >= Start)
	{
		if (mStepHide < LC_STEP_MAX - Time)
			mStepHide += Time;
		else
			mStepHide = LC_STEP_MAX;
	}

	if (mStepShow >= mStepHide)
	{
		if (mStepShow != LC_STEP_MAX)
			mStepHide = mStepShow + 1;
		else
		{
			mStepShow = LC_STEP_MAX - 1;
			mStepHide = LC_STEP_MAX;
		}
	}

	lcObject::InsertTime(mPositionKeys, Start, Time);
	lcObject::InsertTime(mRotationKeys, Start, Time);
}

void lcPiece::RemoveTime(lcStep Start, lcStep Time)
{
	if (mStepShow >= Start)
	{
		if (mStepShow > Time)
			mStepShow -= Time;
		else
			mStepShow = 1;
	}

	if (mStepHide != LC_STEP_MAX)
	{
		if (mStepHide > Time)
			mStepHide -= Time;
		else
			mStepHide = 1;
	}

	if (mStepShow >= mStepHide)
	{
		if (mStepShow != LC_STEP_MAX)
			mStepHide = mStepShow + 1;
		else
		{
			mStepShow = LC_STEP_MAX - 1;
			mStepHide = LC_STEP_MAX;
		}
	}

	lcObject::RemoveTime(mPositionKeys, Start, Time);
	lcObject::RemoveTime(mRotationKeys, Start, Time);
}

void lcPiece::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	if (mPieceInfo->MinIntersectDist(mModelWorld, ObjectRayTest.Start, ObjectRayTest.End, ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcPiece*>(this);
		ObjectRayTest.ObjectSection.Section = 0;
	}
}

void lcPiece::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	if (mPieceInfo->BoxTest(mModelWorld, ObjectBoxTest.Planes))
		ObjectBoxTest.Objects.Add(const_cast<lcPiece*>(this));
}

void lcPiece::DrawInterface(lcContext* Context, const lcMatrix44& ViewMatrix) const
{
	float LineWidth = lcGetPreferences().mLineWidth;
	Context->SetLineWidth(2.0f * LineWidth);

	lcVector3 Min(mPieceInfo->m_fDimensions[3], mPieceInfo->m_fDimensions[4], mPieceInfo->m_fDimensions[5]);
	lcVector3 Max(mPieceInfo->m_fDimensions[0], mPieceInfo->m_fDimensions[1], mPieceInfo->m_fDimensions[2]);
	lcVector3 Edge((Max - Min) * 0.33f);

	float Verts[48][3] =
	{
		{ Max[0], Max[1], Max[2] }, { Max[0] - Edge[0], Max[1], Max[2] },
		{ Max[0], Max[1], Max[2] }, { Max[0], Max[1] - Edge[1], Max[2] },
		{ Max[0], Max[1], Max[2] }, { Max[0], Max[1], Max[2] - Edge[2] },

		{ Min[0], Max[1], Max[2] }, { Min[0] + Edge[0], Max[1], Max[2] },
		{ Min[0], Max[1], Max[2] }, { Min[0], Max[1] - Edge[1], Max[2] },
		{ Min[0], Max[1], Max[2] }, { Min[0], Max[1], Max[2] - Edge[2] },

		{ Max[0], Min[1], Max[2] }, { Max[0] - Edge[0], Min[1], Max[2] },
		{ Max[0], Min[1], Max[2] }, { Max[0], Min[1] + Edge[1], Max[2] },
		{ Max[0], Min[1], Max[2] }, { Max[0], Min[1], Max[2] - Edge[2] },

		{ Min[0], Min[1], Max[2] }, { Min[0] + Edge[0], Min[1], Max[2] },
		{ Min[0], Min[1], Max[2] }, { Min[0], Min[1] + Edge[1], Max[2] },
		{ Min[0], Min[1], Max[2] }, { Min[0], Min[1], Max[2] - Edge[2] },

		{ Max[0], Max[1], Min[2] }, { Max[0] - Edge[0], Max[1], Min[2] },
		{ Max[0], Max[1], Min[2] }, { Max[0], Max[1] - Edge[1], Min[2] },
		{ Max[0], Max[1], Min[2] }, { Max[0], Max[1], Min[2] + Edge[2] },

		{ Min[0], Max[1], Min[2] }, { Min[0] + Edge[0], Max[1], Min[2] },
		{ Min[0], Max[1], Min[2] }, { Min[0], Max[1] - Edge[1], Min[2] },
		{ Min[0], Max[1], Min[2] }, { Min[0], Max[1], Min[2] + Edge[2] },

		{ Max[0], Min[1], Min[2] }, { Max[0] - Edge[0], Min[1], Min[2] },
		{ Max[0], Min[1], Min[2] }, { Max[0], Min[1] + Edge[1], Min[2] },
		{ Max[0], Min[1], Min[2] }, { Max[0], Min[1], Min[2] + Edge[2] },

		{ Min[0], Min[1], Min[2] }, { Min[0] + Edge[0], Min[1], Min[2] },
		{ Min[0], Min[1], Min[2] }, { Min[0], Min[1] + Edge[1], Min[2] },
		{ Min[0], Min[1], Min[2] }, { Min[0], Min[1], Min[2] + Edge[2] },
	};

	Context->SetWorldViewMatrix(lcMul(mModelWorld, ViewMatrix));

	if (IsFocused())
		lcSetColorFocused();
	else
		lcSetColorSelected();

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormat(0, 3, 0, 0);

	Context->DrawPrimitives(GL_LINES, 0, 48);
}

void lcPiece::Move(lcStep Step, bool AddKey, const lcVector3& Distance)
{
	lcVector3 Position = mModelWorld.GetTranslation() + Distance;

	ChangeKey(mPositionKeys, Position, Step, AddKey);

	mModelWorld.SetTranslation(Position);
}

const char* lcPiece::GetName() const
{
	return mPieceInfo->m_strDescription;
}

bool lcPiece::IsVisible(lcStep Step)
{
	if (mState & LC_PIECE_HIDDEN)
		return false;

	return (mStepShow <= Step) && (mStepHide > Step);
}

void lcPiece::CompareBoundingBox(float box[6])
{
	lcVector3 Points[8] =
	{
		lcVector3(mPieceInfo->m_fDimensions[0], mPieceInfo->m_fDimensions[1], mPieceInfo->m_fDimensions[5]),
		lcVector3(mPieceInfo->m_fDimensions[3], mPieceInfo->m_fDimensions[1], mPieceInfo->m_fDimensions[5]),
		lcVector3(mPieceInfo->m_fDimensions[0], mPieceInfo->m_fDimensions[1], mPieceInfo->m_fDimensions[2]),
		lcVector3(mPieceInfo->m_fDimensions[3], mPieceInfo->m_fDimensions[4], mPieceInfo->m_fDimensions[5]),
		lcVector3(mPieceInfo->m_fDimensions[3], mPieceInfo->m_fDimensions[4], mPieceInfo->m_fDimensions[2]),
		lcVector3(mPieceInfo->m_fDimensions[0], mPieceInfo->m_fDimensions[4], mPieceInfo->m_fDimensions[2]),
		lcVector3(mPieceInfo->m_fDimensions[0], mPieceInfo->m_fDimensions[4], mPieceInfo->m_fDimensions[5]),
		lcVector3(mPieceInfo->m_fDimensions[3], mPieceInfo->m_fDimensions[1], mPieceInfo->m_fDimensions[2])
	};

	for (int i = 0; i < 8; i++)
	{
		lcVector3 Point = lcMul31(Points[i], mModelWorld);

		if (Point[0] < box[0]) box[0] = Point[0];
		if (Point[1] < box[1]) box[1] = Point[1];
		if (Point[2] < box[2]) box[2] = Point[2];
		if (Point[0] > box[3]) box[3] = Point[0];
		if (Point[1] > box[4]) box[4] = Point[1];
		if (Point[2] > box[5]) box[5] = Point[2];
	}
}

lcGroup* lcPiece::GetTopGroup()
{
	return mGroup ? mGroup->GetTopGroup() : NULL;
}

void lcPiece::UpdatePosition(lcStep Step)
{
	lcVector3 Position = CalculateKey(mPositionKeys, Step);
	lcMatrix33 Rotation = CalculateKey(mRotationKeys, Step);

	mModelWorld = lcMatrix44(Rotation, Position);
}

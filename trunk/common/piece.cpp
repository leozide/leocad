// A piece object in the LeoCAD world.
//

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
#include "lc_application.h"
#include "lc_library.h"

#define LC_PIECE_SAVE_VERSION 11 // LeoCAD 0.77

static LC_OBJECT_KEY_INFO piece_key_info[LC_PK_COUNT] =
{
  { "Position", 3, LC_PK_POSITION },
  { "Rotation", 4, LC_PK_ROTATION }
};

/////////////////////////////////////////////////////////////////////////////
// Piece construction/destruction

lcPiece::lcPiece(PieceInfo* pPieceInfo)
	: Object (LC_OBJECT_PIECE)
{
	mPieceInfo = pPieceInfo;
	mState = 0;
	mColorIndex = 0;
	mColorCode = 0;
	mStepShow = 1;
	mStepHide = LC_STEP_MAX;
	memset(m_strName, 0, sizeof(m_strName));
	mGroup = NULL;

	if (mPieceInfo != NULL)
		mPieceInfo->AddRef();

	float *values[] = { mPosition, mRotation };
	RegisterKeys (values, piece_key_info, LC_PK_COUNT);

	float pos[3] = { 0, 0, 0 }, rot[4] = { 0, 0, 1, 0 };
	ChangeKey(1, true, pos, LC_PK_POSITION);
	ChangeKey(1, true, rot, LC_PK_ROTATION);
}

lcPiece::~lcPiece()
{
	if (mPieceInfo != NULL)
		mPieceInfo->Release();
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
    if (!Object::FileLoad (file))
      return false;

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

		ChangeKey(time, true, param, type);
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

			ChangeKey(1, true, ModelWorld.r[3], LC_PK_POSITION);

			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
			AxisAngle[3] *= LC_RTOD;

			ChangeKey(time, true, AxisAngle, LC_PK_ROTATION);

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

			ChangeKey(1, true, ModelWorld.r[3], LC_PK_POSITION);

			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
			AxisAngle[3] *= LC_RTOD;
			ChangeKey(1, true, AxisAngle, LC_PK_ROTATION);
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

	PieceInfo* pInfo = lcGetPiecesLibrary()->FindPiece(name, true);
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
      file.ReadBuffer(m_strName, ch);
    }
    else
    {
      lcint32 hide;
      file.ReadS32(&hide, 1);
      if (hide != 0)
        mState |= LC_PIECE_HIDDEN;
      file.ReadBuffer(m_strName, 81);
    }

    // 7 (0.64)
    lcint32 i = -1;
    if (version > 6)
      file.ReadS32(&i, 1);
    mGroup = (Group*)(long)i;
  }
  else
  {
    file.ReadU8(&ch, 1);
    if (ch == 0)
      mGroup = (Group*)-1;
    else
      mGroup = (Group*)(long)ch;

    file.ReadU8(&ch, 1);
    if (ch & 0x01)
      mState |= LC_PIECE_HIDDEN;
  }

  return true;
}

void lcPiece::FileSave(lcFile& file) const
{
	file.WriteU8(LC_PIECE_SAVE_VERSION);

	Object::FileSave (file);

	file.WriteBuffer(mPieceInfo->m_strName, LC_PIECE_NAME_LEN);
	file.WriteU32(mColorCode);
	file.WriteU8(lcMin(mStepShow, 254U));
	file.WriteU8(lcMin(mStepHide, 255U));
	file.WriteU16(1); // m_nFrameShow
	file.WriteU16(100); // m_nFrameHide

	// version 8
	file.WriteU8(mState & LC_PIECE_HIDDEN ? 1 : 0);

	lcuint8 Length = strlen(m_strName);
	file.WriteU8(Length);
	file.WriteBuffer(m_strName, Length);

	// version 7
	lcint32 GroupIndex = lcGetActiveProject()->GetGroupIndex(mGroup);
	file.WriteS32(GroupIndex);
}

void lcPiece::Initialize(float x, float y, float z, lcStep Step)
{
	mStepShow = Step;

	float pos[3] = { x, y, z }, rot[4] = { 0, 0, 1, 0 };
	ChangeKey(1, true, pos, LC_PK_POSITION);
	ChangeKey(1, true, rot, LC_PK_ROTATION);

	UpdatePosition(1);
}

void lcPiece::CreateName(const lcArray<Piece*>& Pieces)
{
	int i, max = 0;

	for (int PieceIdx = 0; PieceIdx < Pieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = Pieces[PieceIdx];

		if (strncmp(Piece->m_strName, mPieceInfo->m_strDescription, strlen(mPieceInfo->m_strDescription)) == 0)
			if (sscanf(Piece->m_strName + strlen(mPieceInfo->m_strDescription), " #%d", &i) == 1)
				if (i > max)
					max = i;
	}

	snprintf(m_strName, sizeof(m_strName), "%s #%.2d", mPieceInfo->m_strDescription, max+1);
	m_strName[sizeof(m_strName) - 1] = 0;
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

	Object::InsertTime(Start, Time);
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

	Object::RemoveTime(Start, Time);
}

void lcPiece::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	lcVector3 Min(mPieceInfo->m_fDimensions[3], mPieceInfo->m_fDimensions[4], mPieceInfo->m_fDimensions[5]);
	lcVector3 Max(mPieceInfo->m_fDimensions[0], mPieceInfo->m_fDimensions[1], mPieceInfo->m_fDimensions[2]);

	lcMatrix44 WorldModel = lcMatrix44AffineInverse(mModelWorld);

	lcVector3 Start = lcMul31(ObjectRayTest.Start, WorldModel);
	lcVector3 End = lcMul31(ObjectRayTest.End, WorldModel);

	float Distance;
	if (!lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) || (Distance >= ObjectRayTest.Distance))
		return;

	lcVector3 Intersection;

	if (mPieceInfo->mMesh->MinIntersectDist(Start, End, ObjectRayTest.Distance, Intersection))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<Piece*>(this);
		ObjectRayTest.ObjectSection.Section = 0;
	}
}

void lcPiece::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	lcVector3 Box[8] =
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

	lcMatrix44 WorldToLocal = lcMatrix44FromAxisAngle(lcVector3(mRotation[0], mRotation[1], mRotation[2]), -mRotation[3] * LC_DTOR);
	WorldToLocal.SetTranslation(lcMul31(lcVector3(-mPosition[0], -mPosition[1], -mPosition[2]), WorldToLocal));

	const int NumPlanes = 6;
	lcVector4 LocalPlanes[NumPlanes];
	int i;

	for (i = 0; i < NumPlanes; i++)
	{
		lcVector3 PlaneNormal = lcMul30(ObjectBoxTest.Planes[i], WorldToLocal);
		LocalPlanes[i] = lcVector4(PlaneNormal, ObjectBoxTest.Planes[i][3] - lcDot3(WorldToLocal[3], PlaneNormal));
	}

	int Outcodes[8];

	for (i = 0; i < 8; i++)
	{
		Outcodes[i] = 0;

		for (int j = 0; j < NumPlanes; j++)
		{
			if (lcDot3(Box[i], LocalPlanes[j]) + LocalPlanes[j][3] > 0)
				Outcodes[i] |= 1 << j;
		}
	}

	int OutcodesOR = 0, OutcodesAND = 0x3f;

	for (i = 0; i < 8; i++)
	{
		OutcodesAND &= Outcodes[i];
		OutcodesOR |= Outcodes[i];
	}

	if (OutcodesAND != 0)
		return;

	if (OutcodesOR == 0 || mPieceInfo->mMesh->IntersectsPlanes(LocalPlanes))
	{
		lcObjectSection& ObjectSection = ObjectBoxTest.ObjectSections.Add();
		ObjectSection.Object = const_cast<Piece*>(this);
		ObjectSection.Section = 0;
	}
}

void lcPiece::Move(lcStep Step, bool AddKey, const lcVector3& Distance)
{
	mPosition += Distance;

	ChangeKey(Step, AddKey, mPosition, LC_PK_POSITION);

	mModelWorld.SetTranslation(mPosition);
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
	CalculateKeys(Step);

	mModelWorld = lcMatrix44FromAxisAngle(lcVector3(mRotation[0], mRotation[1], mRotation[2]), mRotation[3] * LC_DTOR);
	mModelWorld.SetTranslation(mPosition);
}

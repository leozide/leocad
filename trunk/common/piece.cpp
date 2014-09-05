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
	memset(m_strName, 0, sizeof(m_strName));
	mGroup = NULL;

	if (mPieceInfo != NULL)
		mPieceInfo->AddRef();

	ChangeKey(mPositionKeys, lcVector3(0.0f, 0.0f, 0.0f), 1, true);
	ChangeKey(mRotationKeys, lcVector4(0.0f, 0.0f, 1.0f, 0.0f), 1, true);
}

lcPiece::~lcPiece()
{
	if (mPieceInfo != NULL)
		mPieceInfo->Release();
}

void lcPiece::SaveLDraw(QTextStream& Stream) const
{
	if (mStepHide != LC_STEP_MAX)
		Stream << QLatin1String("0 !LEOCAD PIECE STEP_HIDE ") << mStepHide << endl;

	Stream << QLatin1String("0 !LEOCAD PIECE NAME ") << m_strName << endl;

	if (IsHidden())
		Stream << QLatin1String("0 !LEOCAD PIECE HIDDEN") << endl;

	if (mGroup)
		Stream << QLatin1String("0 !LEOCAD PIECE GROUP ") << mGroup->m_strName << endl;

	if (mPositionKeys.GetSize() > 1)
		SaveKeysLDraw(Stream, mPositionKeys, "PIECE POSITION_KEY ");

	if (mRotationKeys.GetSize() > 1)
		SaveKeysLDraw(Stream, mRotationKeys, "PIECE ROTATION_KEY ");

	Stream << "1 " << mColorCode << ' ';

	const float* Matrix = mModelWorld;
	float Numbers[12] = { Matrix[12], -Matrix[14], Matrix[13], Matrix[0], -Matrix[8], Matrix[4], -Matrix[2], Matrix[10], -Matrix[6], Matrix[1], -Matrix[9], Matrix[5] };
	for (int NumberIdx = 0; NumberIdx < 12; NumberIdx++)
	{
		char Number[64];
		sprintf(Number, "%f", Numbers[NumberIdx]);
		char* Dot = strchr(Number, '.');
		if (Dot)
		{
			char* Last;

			for (Last = Dot + strlen(Dot) - 1; *Last == '0'; Last--)
				*Last = 0;

			if (Last == Dot)
				*Dot = 0;
		}

		Stream << Number << ' ';
	}

	Stream << mPieceInfo->m_strName << QLatin1String(".DAT") << endl;
}

bool lcPiece::ParseLDrawLine(QTextStream& Stream, lcModel* Model)
{
	QString Token;
	Stream >> Token;

	if (Token == QLatin1String("STEP_HIDE"))
		Stream >> mStepHide;
	else if (Token == QLatin1String("NAME"))
	{
		QString Name = Stream.readAll().trimmed();
		QByteArray NameUtf = Name.toUtf8(); // todo: replace with qstring
		strncpy(m_strName, NameUtf.constData(), sizeof(m_strName));
		m_strName[sizeof(m_strName) - 1] = 0;
	}
	else if (Token == QLatin1String("HIDDEN"))
		SetHidden(true);
	else if (Token == QLatin1String("GROUP"))
	{
		QString Name = Stream.readAll().trimmed();
		QByteArray NameUtf = Name.toUtf8(); // todo: replace with qstring
		mGroup = Model->GetGroup(NameUtf.constData(), true);
	}
	else if (Token == QLatin1String("POSITION_KEY"))
		LoadKeysLDraw(Stream, mPositionKeys);
	else if (Token == QLatin1String("ROTATION_KEY"))
		LoadKeysLDraw(Stream, mRotationKeys);

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
				ChangeKey(mRotationKeys, lcVector4(param[0], param[1], param[2], param[3]), time, true);
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
			ChangeKey(mRotationKeys, lcVector4(param[0], param[1], param[2], param[3]), time, true);
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

			ChangeKey(mPositionKeys, lcVector3(ModelWorld.r[3][0], ModelWorld.r[3][1], ModelWorld.r[3][2]), 1, true);

			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
			AxisAngle[3] *= LC_RTOD;

			ChangeKey(mRotationKeys, AxisAngle, time, true);

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

			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
			AxisAngle[3] *= LC_RTOD;
			ChangeKey(mRotationKeys, AxisAngle, 1, true);
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

void lcPiece::FileSave(lcFile& file) const
{
	file.WriteU8(LC_PIECE_SAVE_VERSION);

	file.WriteU8(1);
	file.WriteU32(mPositionKeys.GetSize() + mRotationKeys.GetSize());

	for (int KeyIdx = 0; KeyIdx < mPositionKeys.GetSize(); KeyIdx++)
	{
		lcObjectKey<lcVector3>& Key = mPositionKeys[KeyIdx];

		lcuint16 Step = lcMin(Key.Step, 0xFFFFU);
		file.WriteU16(Step);
		file.WriteFloats(Key.Value, 3);
		file.WriteFloat(0);
		file.WriteU8(0);
	}

	for (int KeyIdx = 0; KeyIdx < mRotationKeys.GetSize(); KeyIdx++)
	{
		lcObjectKey<lcVector4>& Key = mRotationKeys[KeyIdx];

		lcuint16 Step = lcMin(Key.Step, 0xFFFFU);
		file.WriteU16(Step);
		file.WriteFloats(Key.Value, 4);
		file.WriteU8(1);
	}

	file.WriteU32(0);


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

void lcPiece::Initialize(const lcVector3& Position, const lcVector4& AxisAngle, lcStep Step)
{
	mStepShow = Step;

	ChangeKey(mPositionKeys, Position, 1, true);
	ChangeKey(mRotationKeys, AxisAngle, 1, true);

	UpdatePosition(1);
}

void lcPiece::CreateName(const lcArray<lcPiece*>& Pieces)
{
	if (m_strName[0])
	{
		bool Found = false;

		for (int PieceIdx = 0; PieceIdx < Pieces.GetSize(); PieceIdx++)
		{
			if (!strcmp(Pieces[PieceIdx]->m_strName, m_strName))
			{
				Found = true;
				break;
			}
		}

		if (!Found)
			return;
	}

	const char* Prefix = mPieceInfo->m_strDescription;
	int Length = strlen(Prefix);
	int i, max = 0;

	for (int PieceIdx = 0; PieceIdx < Pieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = Pieces[PieceIdx];

		if (strncmp(Piece->m_strName, Prefix, Length) == 0)
			if (sscanf(Piece->m_strName + Length, " #%d", &i) == 1)
				if (i > max)
					max = i;
	}

	snprintf(m_strName, sizeof(m_strName), "%s #%.2d", Prefix, max + 1);
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
		ObjectRayTest.ObjectSection.Object = const_cast<lcPiece*>(this);
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
		ObjectSection.Object = const_cast<lcPiece*>(this);
		ObjectSection.Section = 0;
	}
}

void lcPiece::Move(lcStep Step, bool AddKey, const lcVector3& Distance)
{
	mPosition += Distance;

	ChangeKey(mPositionKeys, mPosition, Step, AddKey);

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
	mPosition = CalculateKey(mPositionKeys, Step);
	mRotation = CalculateKey(mRotationKeys, Step);

	mModelWorld = lcMatrix44FromAxisAngle(lcVector3(mRotation[0], mRotation[1], mRotation[2]), mRotation[3] * LC_DTOR);
	mModelWorld.SetTranslation(mPosition);
}

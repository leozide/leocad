#include "lc_global.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "pieceinf.h"
#include "piece.h"
#include "group.h"
#include "lc_file.h"
#include "lc_application.h"
#include "lc_library.h"
#include "lc_context.h"
#include "lc_qutils.h"
#include "lc_synth.h"

#define LC_PIECE_CONTROL_POINT_SIZE 10.0f

lcPiece::lcPiece(PieceInfo* Info)
	: lcObject(LC_OBJECT_PIECE)
{
	mMesh = NULL;
	SetPieceInfo(Info);
	mState = 0;
	mColorIndex = gDefaultColor;
	mColorCode = 16;
	mStepShow = 1;
	mStepHide = LC_STEP_MAX;
	mGroup = NULL;
	mFileLine = -1;
	mPivotMatrix = lcMatrix44Identity();

	ChangeKey(mPositionKeys, lcVector3(0.0f, 0.0f, 0.0f), 1, true);
	ChangeKey(mRotationKeys, lcMatrix33Identity(), 1, true);
}

lcPiece::~lcPiece()
{
	if (mPieceInfo)
		mPieceInfo->Release(false);

	delete mMesh;
}

void lcPiece::SetPieceInfo(PieceInfo* Info)
{
	mPieceInfo = Info;
	if (mPieceInfo)
		mPieceInfo->AddRef();

	mControlPoints.RemoveAll();
	delete mMesh;
	mMesh = NULL;

	lcSynthInfo* SynthInfo = mPieceInfo ? mPieceInfo->GetSynthInfo() : NULL;

	if (SynthInfo)
	{
		SynthInfo->GetDefaultControlPoints(mControlPoints);
		UpdateMesh();
	}
}

void lcPiece::SaveLDraw(QTextStream& Stream) const
{
	QLatin1String LineEnding("\r\n");

	if (mStepHide != LC_STEP_MAX)
		Stream << QLatin1String("0 !LEOCAD PIECE STEP_HIDE ") << mStepHide << LineEnding;

	if (IsHidden())
		Stream << QLatin1String("0 !LEOCAD PIECE HIDDEN") << LineEnding;

	if (mState & LC_PIECE_PIVOT_POINT_VALID)
	{
		const float* PivotMatrix = mPivotMatrix;
		float PivotNumbers[12] = { PivotMatrix[12], -PivotMatrix[14], PivotMatrix[13], PivotMatrix[0], -PivotMatrix[8], PivotMatrix[4], -PivotMatrix[2], PivotMatrix[10], -PivotMatrix[6], PivotMatrix[1], -PivotMatrix[9], PivotMatrix[5] };

		Stream << QLatin1String("0 !LEOCAD PIECE PIVOT ");

		for (int NumberIdx = 0; NumberIdx < 12; NumberIdx++)
			Stream << ' ' << lcFormatValue(PivotNumbers[NumberIdx]);

		Stream << LineEnding;
	}

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
		else if (Token == QLatin1String("PIVOT"))
		{
			float PivotNumbers[12];
			for (int TokenIdx = 0; TokenIdx < 12; TokenIdx++)
				Stream >> PivotNumbers[TokenIdx];

			lcMatrix44 PivotMatrix(lcVector4( PivotNumbers[3],  PivotNumbers[9], -PivotNumbers[6], 0.0f), lcVector4(PivotNumbers[5], PivotNumbers[11], -PivotNumbers[8], 0.0f),
								   lcVector4(-PivotNumbers[4], -PivotNumbers[10], PivotNumbers[7], 0.0f), lcVector4(PivotNumbers[0], PivotNumbers[2],  -PivotNumbers[1], 1.0f));

			mPivotMatrix = PivotMatrix;
			mState |= LC_PIECE_PIVOT_POINT_VALID;
		}
		else if (Token == QLatin1String("POSITION_KEY"))
			LoadKeysLDraw(Stream, mPositionKeys);
		else if (Token == QLatin1String("ROTATION_KEY"))
			LoadKeysLDraw(Stream, mRotationKeys);
	}

	return false;
}

bool lcPiece::FileLoad(lcFile& file)
{
	lcuint8 version, ch;

	version = file.ReadU8();

	if (version > 12) // LeoCAD 0.80
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
    mGroup = (lcGroup*)(quintptr)i;
  }
  else
  {
    file.ReadU8(&ch, 1);
    if (ch == 0)
      mGroup = (lcGroup*)-1;
    else
      mGroup = (lcGroup*)(quintptr)ch;

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
	lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(mModelWorld);
	lcVector3 Start = lcMul31(ObjectRayTest.Start, InverseWorldMatrix);
	lcVector3 End = lcMul31(ObjectRayTest.End, InverseWorldMatrix);

	if (mMesh)
	{
		if (mMesh->MinIntersectDist(Start, End, ObjectRayTest.Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcPiece*>(this);
			ObjectRayTest.ObjectSection.Section = LC_PIECE_SECTION_POSITION;
		}
	}
	else if (mPieceInfo->MinIntersectDist(Start, End, ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcPiece*>(this);
		ObjectRayTest.ObjectSection.Section = LC_PIECE_SECTION_POSITION;
	}

	if (AreControlPointsVisible())
	{
		lcVector3 Min(-LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE);
		lcVector3 Max(LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE);

		for (int ControlPointIdx = 0; ControlPointIdx < mControlPoints.GetSize(); ControlPointIdx++)
		{
			lcMatrix44 InverseTransform = lcMatrix44AffineInverse(mControlPoints[ControlPointIdx].Transform);
			lcVector3 PointStart = lcMul31(Start, InverseTransform);
			lcVector3 PointEnd = lcMul31(End, InverseTransform);

			float Distance;
			if (!lcBoundingBoxRayIntersectDistance(Min, Max, PointStart, PointEnd, &Distance, NULL) || (Distance >= ObjectRayTest.Distance))
				continue;

			ObjectRayTest.ObjectSection.Object = const_cast<lcPiece*>(this);
			ObjectRayTest.ObjectSection.Section = LC_PIECE_SECTION_CONTROL_POINT_1 + ControlPointIdx;
			ObjectRayTest.Distance = Distance;
		}
	}
}

void lcPiece::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	if (mPieceInfo->BoxTest(mModelWorld, ObjectBoxTest.Planes))
		ObjectBoxTest.Objects.Add(const_cast<lcPiece*>(this));
}

void lcPiece::DrawInterface(lcContext* Context) const
{
	float LineWidth = lcGetPreferences().mLineWidth;
	Context->SetLineWidth(2.0f * LineWidth);

	const lcBoundingBox& BoundingBox = GetBoundingBox();
	const lcVector3& Min = BoundingBox.Min;
	const lcVector3& Max = BoundingBox.Max;
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

	Context->SetProgram(LC_PROGRAM_SIMPLE);
	Context->SetWorldMatrix(mModelWorld);

	if (IsFocused(LC_PIECE_SECTION_POSITION))
		Context->SetInterfaceColor(LC_COLOR_FOCUSED);
	else
		Context->SetInterfaceColor(LC_COLOR_SELECTED);

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormat(0, 3, 0, 0);

	Context->DrawPrimitives(GL_LINES, 0, 48);

	if (IsPivotPointVisible())
	{
		const float Size = 5.0f;
		const float Verts[8 * 3] =
		{
			-Size, -Size, -Size, -Size,  Size, -Size, Size,  Size, -Size, Size, -Size, -Size,
			-Size, -Size,  Size, -Size,  Size,  Size, Size,  Size,  Size, Size, -Size,  Size
		};

		const GLushort Indices[24] =
		{
			0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7
		};

		Context->SetWorldMatrix(lcMul(mPivotMatrix, mModelWorld));

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormat(0, 3, 0, 0);
		Context->SetIndexBufferPointer(Indices);

		Context->DrawIndexedPrimitives(GL_LINES, 24, GL_UNSIGNED_SHORT, 0);
	}

	if (AreControlPointsVisible())
	{
		float Verts[8 * 3];
		float* CurVert = Verts;

		lcVector3 Min(-LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE);
		lcVector3 Max(LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE);

		*CurVert++ = Min[0]; *CurVert++ = Min[1]; *CurVert++ = Min[2];
		*CurVert++ = Min[0]; *CurVert++ = Max[1]; *CurVert++ = Min[2];
		*CurVert++ = Max[0]; *CurVert++ = Max[1]; *CurVert++ = Min[2];
		*CurVert++ = Max[0]; *CurVert++ = Min[1]; *CurVert++ = Min[2];
		*CurVert++ = Min[0]; *CurVert++ = Min[1]; *CurVert++ = Max[2];
		*CurVert++ = Min[0]; *CurVert++ = Max[1]; *CurVert++ = Max[2];
		*CurVert++ = Max[0]; *CurVert++ = Max[1]; *CurVert++ = Max[2];
		*CurVert++ = Max[0]; *CurVert++ = Min[1]; *CurVert++ = Max[2];

		const GLushort Indices[36] =
		{
			0, 1, 2, 0, 2, 3, 7, 6, 5, 7, 5, 4, 5, 1, 0, 4, 5, 0,
			7, 3, 2, 6, 7, 2, 0, 3, 7, 0, 7, 4, 6, 2, 1, 5, 6, 1
		};

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);

		for (int ControlPointIdx = 0; ControlPointIdx < mControlPoints.GetSize(); ControlPointIdx++)
		{
			Context->SetWorldMatrix(lcMul(mControlPoints[ControlPointIdx].Transform, mModelWorld));

			Context->SetVertexBufferPointer(Verts);
			Context->SetVertexFormat(0, 3, 0, 0);
			Context->SetIndexBufferPointer(Indices);

			if (IsFocused(LC_PIECE_SECTION_CONTROL_POINT_1 + ControlPointIdx))
				Context->SetInterfaceColor(LC_COLOR_CONTROL_POINT_FOCUSED);
			else
				Context->SetInterfaceColor(LC_COLOR_CONTROL_POINT);

			Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
		}

		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
	}
}

void lcPiece::AddRenderMeshes(lcScene& Scene, bool DrawInterface) const
{
	bool Focused, Selected;

	if (DrawInterface)
	{
		Focused = IsFocused();
		Selected = IsSelected();
	}
	else
	{
		Focused = false;
		Selected = false;
	}

	if (!mMesh)
		mPieceInfo->AddRenderMeshes(Scene, mModelWorld, mColorIndex, Focused, Selected);
	else
	{
		lcRenderMesh RenderMesh;

		RenderMesh.WorldMatrix = mModelWorld;
		RenderMesh.Mesh = mMesh;
		RenderMesh.ColorIndex = mColorIndex;
		RenderMesh.State = Focused ? LC_RENDERMESH_FOCUSED : (Selected ? LC_RENDERMESH_SELECTED : LC_RENDERMESH_NONE);
		RenderMesh.Distance = fabsf(lcMul31(mModelWorld[3], Scene.mViewMatrix).z);
		RenderMesh.LodIndex = RenderMesh.Mesh->GetLodIndex(RenderMesh.Distance);

		bool Translucent = lcIsColorTranslucent(mColorIndex);

		if ((mPieceInfo->mFlags & (LC_PIECE_HAS_SOLID | LC_PIECE_HAS_LINES)) || ((mPieceInfo->mFlags & LC_PIECE_HAS_DEFAULT) && !Translucent))
			Scene.mOpaqueMeshes.Add(RenderMesh);

		if ((mPieceInfo->mFlags & LC_PIECE_HAS_TRANSLUCENT) || ((mPieceInfo->mFlags & LC_PIECE_HAS_DEFAULT) && Translucent))
			Scene.mTranslucentMeshes.Add(RenderMesh);
	}

	if (Selected)
		Scene.mInterfaceObjects.Add(this);
}

void lcPiece::Move(lcStep Step, bool AddKey, const lcVector3& Distance)
{
	lcuint32 Section = GetFocusSection();

	if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
	{
		lcVector3 Position = mModelWorld.GetTranslation() + Distance;

		SetPosition(Position, Step, AddKey);

		mModelWorld.SetTranslation(Position);
	}
	else
	{
		int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_1;

		if (ControlPointIndex >= 0 && ControlPointIndex < mControlPoints.GetSize())
		{
			lcMatrix33 InverseWorldMatrix = lcMatrix33AffineInverse(lcMatrix33(mModelWorld));
			lcMatrix44& Transform = mControlPoints[ControlPointIndex].Transform;

			Transform.SetTranslation(Transform.GetTranslation() + lcMul(Distance, InverseWorldMatrix));
		}

		UpdateMesh();
	}
}

void lcPiece::Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcVector3& Center, const lcMatrix33& RotationFrame)
{
	lcuint32 Section = GetFocusSection();

	if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
	{
		lcVector3 Distance = mModelWorld.GetTranslation() - Center;
		lcMatrix33 LocalToWorldMatrix = lcMatrix33(mModelWorld);

		lcMatrix33 LocalToFocusMatrix = lcMul(LocalToWorldMatrix, RotationFrame);
		lcMatrix33 NewLocalToWorldMatrix = lcMul(LocalToFocusMatrix, RotationMatrix);

		lcMatrix33 WorldToLocalMatrix = lcMatrix33AffineInverse(LocalToWorldMatrix);

		Distance = lcMul(Distance, WorldToLocalMatrix);
		Distance = lcMul(Distance, NewLocalToWorldMatrix);

		NewLocalToWorldMatrix.Orthonormalize();

		SetPosition(Center + Distance, Step, AddKey);
		SetRotation(NewLocalToWorldMatrix, Step, AddKey);
	}
	else
	{
		int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_1;

		if (ControlPointIndex >= 0 && ControlPointIndex < mControlPoints.GetSize())
		{
			lcMatrix44& Transform = mControlPoints[ControlPointIndex].Transform;
			lcMatrix33 PieceWorldMatrix(mModelWorld);
			lcMatrix33 LocalToWorldMatrix = lcMul(lcMatrix33(Transform), PieceWorldMatrix);

			lcMatrix33 LocalToFocusMatrix = lcMul(LocalToWorldMatrix, RotationFrame);
			lcMatrix33 NewLocalToWorldMatrix = lcMul(lcMul(LocalToFocusMatrix, RotationMatrix), lcMatrix33AffineInverse(PieceWorldMatrix));

			NewLocalToWorldMatrix.Orthonormalize();
			Transform = lcMatrix44(NewLocalToWorldMatrix, Transform.GetTranslation());
		}

		UpdateMesh();
	}
}

void lcPiece::MovePivotPoint(const lcVector3& Distance)
{
	if (!IsFocused(LC_PIECE_SECTION_POSITION))
		return;

	mPivotMatrix.SetTranslation(mPivotMatrix.GetTranslation() + Distance);
	mState |= LC_PIECE_PIVOT_POINT_VALID;
}

void lcPiece::RotatePivotPoint(const lcMatrix33& RotationMatrix)
{
	if (!IsFocused(LC_PIECE_SECTION_POSITION))
		return;

	lcMatrix33 NewPivotRotationMatrix = lcMul(RotationMatrix, lcMatrix33(mPivotMatrix));
	NewPivotRotationMatrix.Orthonormalize();

	mPivotMatrix = lcMatrix44(NewPivotRotationMatrix, mPivotMatrix.GetTranslation());
	mState |= LC_PIECE_PIVOT_POINT_VALID;
}

lcuint32 lcPiece::GetAllowedTransforms() const
{
	lcuint32 Section = GetFocusSection();

	if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
		return LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z | LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z;

	if (mPieceInfo->GetSynthInfo()->IsCurve())
		return LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z | LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z | LC_OBJECT_TRANSFORM_SCALE_X;
	else
		return LC_OBJECT_TRANSFORM_MOVE_Z;
}

bool lcPiece::InsertControlPoint(const lcVector3& WorldStart, const lcVector3& WorldEnd)
{
	lcSynthInfo* SynthInfo = mPieceInfo->GetSynthInfo();
	if (!SynthInfo || !SynthInfo->CanAddControlPoints())
		return false;

	lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(mModelWorld);
	lcVector3 Start = lcMul31(WorldStart, InverseWorldMatrix);
	lcVector3 End = lcMul31(WorldEnd, InverseWorldMatrix);

	int ControlPointIndex = SynthInfo->InsertControlPoint(mControlPoints, Start, End);
	if (ControlPointIndex)
	{
		SetFocused(GetFocusSection(), false);
		SetFocused(LC_PIECE_SECTION_CONTROL_POINT_1 + ControlPointIndex, true);
		UpdateMesh();
		return true;
	}

	return false;
}

bool lcPiece::RemoveFocusedControlPoint()
{
	int ControlPointIndex = GetFocusSection() - LC_PIECE_SECTION_CONTROL_POINT_1;

	if (ControlPointIndex < 0 || ControlPointIndex >= mControlPoints.GetSize() || mControlPoints.GetSize() <= 2)
		return false;

	SetFocused(GetFocusSection(), false);
	SetFocused(LC_PIECE_SECTION_POSITION, true);
	mControlPoints.RemoveIndex(ControlPointIndex);

	UpdateMesh();

	return true;
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

const lcBoundingBox& lcPiece::GetBoundingBox() const
{
	if (!mMesh)
		return mPieceInfo->GetBoundingBox();
	else
		return mMesh->mBoundingBox;
}

void lcPiece::CompareBoundingBox(lcVector3& Min, lcVector3& Max) const
{
	lcVector3 Points[8];

	if (!mMesh)
		lcGetBoxCorners(GetBoundingBox(), Points);
	else
		lcGetBoxCorners(mMesh->mBoundingBox, Points);

	for (int i = 0; i < 8; i++)
	{
		lcVector3 Point = lcMul31(Points[i], mModelWorld);

		Min = lcMin(Point, Min);
		Max = lcMax(Point, Max);
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

void lcPiece::UpdateMesh()
{
	delete mMesh;
	lcSynthInfo* SynthInfo = mPieceInfo->GetSynthInfo();
	mMesh = SynthInfo ? SynthInfo->CreateMesh(mControlPoints) : NULL;
}

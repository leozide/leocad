#include "lc_global.h"
#include "lc_piece.h"
#include "lc_mesh.h"
#include "lc_file.h"
#include "lc_colors.h"
#include "pieceinf.h"
#include "view.h"
#include "lc_camera.h"

lcPiece::lcPiece()
	: lcObject(LC_OBJECT_TYPE_PIECE)
{
	mPieceInfo = NULL;
	mColorIndex = 0;
	mState = 0;
}

lcPiece::lcPiece(PieceInfo* Part, int ColorIndex, const lcVector3& Position, const lcVector4& AxisAngle, lcTime Time)
	: lcObject(LC_OBJECT_TYPE_PIECE)
{
	mPieceInfo = Part;
	mColorIndex = ColorIndex;
	mState = 0;

	mPosition = Position;
	mAxisAngle = AxisAngle;
//Time
}

lcPiece::~lcPiece()
{
}

void lcPiece::Save(lcFile& File)
{
	File.WriteFloats(mPosition, 3);
	File.WriteFloats(mAxisAngle, 4);
}

void lcPiece::Load(lcFile& File)
{
	File.ReadFloats(mPosition, 3);
	File.ReadFloats(mAxisAngle, 4);
}

void lcPiece::Update()
{
//	if (!IsVisible(nTime, bAnimation))
//		m_nState &= ~(LC_PIECE_SELECTED|LC_PIECE_FOCUSED);

	mModelWorld = lcMatrix44FromAxisAngle(lcVector3(mAxisAngle[0], mAxisAngle[1], mAxisAngle[2]), mAxisAngle[3] * LC_DTOR);
	mModelWorld.SetTranslation(mPosition);
}

void lcPiece::ClosestHitTest(lcObjectHitTest& HitTest)
{
	lcVector3 Min(mPieceInfo->m_fDimensions[3], mPieceInfo->m_fDimensions[4], mPieceInfo->m_fDimensions[5]);
	lcVector3 Max(mPieceInfo->m_fDimensions[0], mPieceInfo->m_fDimensions[1], mPieceInfo->m_fDimensions[2]);

	lcMatrix44 WorldModel = lcMatrix44AffineInverse(mModelWorld);

	lcVector3 Start = lcMul31(HitTest.Start, WorldModel);
	lcVector3 End = lcMul31(HitTest.End, WorldModel);

	float Distance;
	if (!lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) || (Distance >= HitTest.Distance))
		return;

	lcVector3 Intersection;

	if (mPieceInfo->mMesh->MinIntersectDist(Start, End, HitTest.Distance, Intersection))
	{
		HitTest.ObjectSection.Object = this;
		HitTest.ObjectSection.Section = 0;
	}
}

void lcPiece::BoxTest(lcObjectBoxTest& BoxTest)
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

	lcMatrix44 WorldToLocal = lcMatrix44FromAxisAngle(lcVector3(mAxisAngle[0], mAxisAngle[1], mAxisAngle[2]), -mAxisAngle[3] * LC_DTOR);
	WorldToLocal.SetTranslation(lcMul31(lcVector3(-mPosition[0], -mPosition[1], -mPosition[2]), WorldToLocal));

	const int NumPlanes = 6;
	lcVector4 LocalPlanes[NumPlanes];
	int i;

	for (i = 0; i < NumPlanes; i++)
	{
		lcVector3 PlaneNormal = lcMul30(BoxTest.Planes[i], WorldToLocal);
		LocalPlanes[i] = lcVector4(PlaneNormal, BoxTest.Planes[i][3] - lcDot3(WorldToLocal[3], PlaneNormal));
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
		lcObjectSection& ObjectSection = BoxTest.ObjectSections.Add();
		ObjectSection.Object = this;
		ObjectSection.Section = 0;
	}
}

void lcPiece::AddBoundingBox(lcVector3* Min, lcVector3* Max) const
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

	for (int PointIdx = 0; PointIdx < 8; PointIdx++)
	{
		lcVector3 Point = lcMul31(Points[PointIdx], mModelWorld);

		(*Min)[0] = lcMin((*Min)[0], Point[0]);
		(*Min)[1] = lcMin((*Min)[1], Point[1]);
		(*Min)[2] = lcMin((*Min)[2], Point[2]);
		(*Max)[0] = lcMax((*Max)[0], Point[0]);
		(*Max)[1] = lcMax((*Max)[1], Point[1]);
		(*Max)[2] = lcMax((*Max)[2], Point[2]);
	}
}

void lcPiece::GetPartsUsed(lcArray<lcObjectParts>& PartsUsed) const
{
//	if (Piece->mPieceInfo->m_strDescription[0] == '~')
//		continue;

	for (int PartIdx = 0; PartIdx < PartsUsed.GetSize(); PartIdx++)
	{
		lcObjectParts& Entry = PartsUsed[PartIdx];

// replace default color
		if (Entry.Info != mPieceInfo || Entry.ColorIndex != mColorIndex)
			continue;

		Entry.Count++;
		return;
	}

	lcObjectParts& Entry = PartsUsed.Add();

	Entry.Info = mPieceInfo;
	Entry.ColorIndex = mColorIndex;
	Entry.Count = 1;
}

void lcPiece::GetRenderMeshes(View* View, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes, lcArray<lcObject*>& InterfaceObjects)
{
//	if (!IsVisible())
//		return;

	bool Translucent = lcIsColorTranslucent(mColorIndex);

	lcRenderMesh RenderMesh;

	RenderMesh.ModelWorld = &mModelWorld;
	RenderMesh.Mesh = mPieceInfo->mMesh;
	RenderMesh.ColorIndex = mColorIndex;

	if ((mPieceInfo->mFlags & (LC_PIECE_HAS_SOLID | LC_PIECE_HAS_LINES)) || ((mPieceInfo->mFlags & LC_PIECE_HAS_DEFAULT) && !Translucent))
		OpaqueMeshes.Add(RenderMesh);

	if ((mPieceInfo->mFlags & LC_PIECE_HAS_TRANSLUCENT) || ((mPieceInfo->mFlags & LC_PIECE_HAS_DEFAULT) && Translucent))
	{
		lcVector3 Pos = lcMul31(mPosition, View->mCamera->mWorldView);

		RenderMesh.Distance = Pos[2];

		TranslucentMeshes.Add(RenderMesh);
	}

	if (IsSelected())
		InterfaceObjects.Add(this);
}

void lcPiece::RenderInterface(View* View) const
{
	lcVector3 Min(mPieceInfo->m_fDimensions[3], mPieceInfo->m_fDimensions[4], mPieceInfo->m_fDimensions[5]);
	lcVector3 Max(mPieceInfo->m_fDimensions[0], mPieceInfo->m_fDimensions[1], mPieceInfo->m_fDimensions[2]);
	lcVector3 Edge((Max - Min) * 0.33f);
/*
	float Verts[24][3] =
	{
		{ Max[0], Max[1], Max[2] }, { Min[0], Max[1], Max[2] },
		{ Min[0], Max[1], Max[2] }, { Min[0], Min[1], Max[2] },
		{ Min[0], Min[1], Max[2] }, { Max[0], Min[1], Max[2] },
		{ Max[0], Min[1], Max[2] }, { Max[0], Max[1], Max[2] },
		{ Max[0], Max[1], Min[2] }, { Min[0], Max[1], Min[2] },
		{ Min[0], Max[1], Min[2] }, { Min[0], Min[1], Min[2] },
		{ Min[0], Min[1], Min[2] }, { Max[0], Min[1], Min[2] },
		{ Max[0], Min[1], Min[2] }, { Max[0], Max[1], Min[2] },
		{ Max[0], Max[1], Max[2] }, { Max[0], Max[1], Min[2] },
		{ Min[0], Max[1], Max[2] }, { Min[0], Max[1], Min[2] },
		{ Min[0], Min[1], Max[2] }, { Min[0], Min[1], Min[2] },
		{ Max[0], Min[1], Max[2] }, { Max[0], Min[1], Min[2] }
	};
*/
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

	glPushMatrix();
	glMultMatrixf(mModelWorld);

	glLineWidth(2.0f);

	if (IsFocused())
		lcSetColorFocused();
	else
		lcSetColorSelected();

	glVertexPointer(3, GL_FLOAT, 0, Verts);
	glDrawArrays(GL_LINES, 0, 48);

	glLineWidth(1.0f);

	glPopMatrix();
}

void lcPiece::Move(const lcVector3& Distance, lcTime Time, bool AddKeys)
{
	if (mState & LC_PIECE_POSITION_SELECTED)
	{
		mPosition += Distance;

		ChangeKey(mPositionKeys, mPosition, Time, AddKeys);
	}
}

/*
#define LC_PIECE_SAVE_VERSION 11 // LeoCAD 0.77

Piece::Piece(PieceInfo* pPieceInfo)
	: Object (LC_OBJECT_PIECE)
{
	m_pNext = NULL;
	mPieceInfo = pPieceInfo;
	m_nState = 0;
	mColorIndex = 0;
	mColorCode = 0;
	m_nStepShow = 1;
	m_nStepHide = 255;
	m_nFrameHide = 65535;
	memset(m_strName, 0, sizeof(m_strName));
	m_pGroup = NULL;

	if (mPieceInfo != NULL)
		mPieceInfo->AddRef();

	float *values[] = { mPosition, mRotation };
	RegisterKeys (values, piece_key_info, LC_PK_COUNT);

	float pos[3] = { 0, 0, 0 }, rot[4] = { 0, 0, 1, 0 };
	ChangeKey (1, false, true, pos, LC_PK_POSITION);
	ChangeKey (1, false, true, rot, LC_PK_ROTATION);
	ChangeKey (1, true, true, pos, LC_PK_POSITION);
	ChangeKey (1, true, true, rot, LC_PK_ROTATION);
}

Piece::~Piece()
{
	if (mPieceInfo != NULL)
		mPieceInfo->Release();
}

/////////////////////////////////////////////////////////////////////////////
// Piece save/load

// Use only when loading from a file
void Piece::SetPieceInfo(PieceInfo* pPieceInfo)
{
	mPieceInfo = pPieceInfo;
	mPieceInfo->AddRef();
}

bool Piece::FileLoad(lcFile& file)
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

		ChangeKey (time, false, true, param, type);
	  }

	  file.ReadU32(&keys, 1);
	  while (keys--)
	  {
		file.ReadFloats(param, 4);
		file.ReadU16(&time, 1);
		file.ReadU8(&type, 1);

		ChangeKey (time, true, true, param, type);
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

			ChangeKey(1, false, true, ModelWorld.r[3], LC_PK_POSITION);
			ChangeKey(1, true, true, ModelWorld.r[3], LC_PK_POSITION);

			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
			AxisAngle[3] *= LC_RTOD;

			ChangeKey(time, false, true, AxisAngle, LC_PK_ROTATION);
			ChangeKey(time, true, true, AxisAngle, LC_PK_ROTATION);

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

			ChangeKey(1, false, true, ModelWorld.r[3], LC_PK_POSITION);
			ChangeKey(1, true, true, ModelWorld.r[3], LC_PK_POSITION);

			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
			AxisAngle[3] *= LC_RTOD;
			ChangeKey(1, false, true, AxisAngle, LC_PK_ROTATION);
			ChangeKey(1, true, true, AxisAngle, LC_PK_ROTATION);
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

  file.ReadU8(&m_nStepShow, 1);
  if (version > 1)
	file.ReadU8(&m_nStepHide, 1);
  else
	m_nStepHide = 255;

  if (version > 5)
  {
	file.ReadU16(&m_nFrameShow, 1);
	file.ReadU16(&m_nFrameHide, 1);

	if (version > 7)
	{
	  file.ReadU8(&m_nState, 1);
	  Select (false, false, false);
	  file.ReadU8(&ch, 1);
	  file.ReadBuffer(m_strName, ch);
	}
	else
	{
	  lcint32 hide;
	  file.ReadS32(&hide, 1);
	  if (hide != 0)
		m_nState |= LC_PIECE_HIDDEN;
	  file.ReadBuffer(m_strName, 81);
	}

	// 7 (0.64)
	lcint32 i = -1;
	if (version > 6)
	  file.ReadS32(&i, 1);
	m_pGroup = (Group*)(long)i;
  }
  else
  {
	m_nFrameShow = 1;
	m_nFrameHide = 65535;

	file.ReadU8(&ch, 1);
	if (ch == 0)
	  m_pGroup = (Group*)-1;
	else
	  m_pGroup = (Group*)(long)ch;

	file.ReadU8(&ch, 1);
	if (ch & 0x01)
	  m_nState |= LC_PIECE_HIDDEN;
  }

  return true;
}

void Piece::FileSave(lcFile& file) const
{
	file.WriteU8(LC_PIECE_SAVE_VERSION);

	Object::FileSave (file);

	file.WriteBuffer(mPieceInfo->m_strName, LC_PIECE_NAME_LEN);
	file.WriteU32(mColorCode);
	file.WriteU8(m_nStepShow);
	file.WriteU8(m_nStepHide);
	file.WriteU16(m_nFrameShow);
	file.WriteU16(m_nFrameHide);

	// version 8
	file.WriteU8(m_nState);

	lcuint8 Length = strlen(m_strName);
	file.WriteU8(Length);
	file.WriteBuffer(m_strName, Length);

	// version 7
	lcint32 i;

	if (m_pGroup != NULL)
	{
		Group* pGroups = lcGetActiveProject()->m_pGroups;
		for (i = 0; pGroups; pGroups = pGroups->m_pNext)
		{
			if (m_pGroup == pGroups)
				break;
			i++;
		}
	}
	else
		i = -1;

	file.WriteS32(i);
}

void Piece::Initialize(float x, float y, float z, unsigned char nStep, unsigned short nFrame)
{
	m_nFrameShow = nFrame;
	m_nStepShow = nStep;

	float pos[3] = { x, y, z }, rot[4] = { 0, 0, 1, 0 };
	ChangeKey (1, false, true, pos, LC_PK_POSITION);
	ChangeKey (1, false, true, rot, LC_PK_ROTATION);
	ChangeKey (1, true, true, pos, LC_PK_POSITION);
	ChangeKey (1, true, true, rot, LC_PK_ROTATION);

	UpdatePosition (1, false);
}

void Piece::CreateName(Piece* pPiece)
{
	int i, max = 0;

	for (; pPiece; pPiece = pPiece->m_pNext)
		if (strncmp (pPiece->m_strName, mPieceInfo->m_strDescription, strlen(mPieceInfo->m_strDescription)) == 0)
			if (sscanf(pPiece->m_strName + strlen(mPieceInfo->m_strDescription), " #%d", &i) == 1)
				if (i > max)
					max = i;

	sprintf (m_strName, "%s #%.2d", mPieceInfo->m_strDescription, max+1);
}

void Piece::Select (bool bSelecting, bool bFocus, bool bMultiple)
{
  if (bSelecting == true)
  {
	if (bFocus == true)
	  m_nState |= (LC_PIECE_FOCUSED|LC_PIECE_SELECTED);
	else
	  m_nState |= LC_PIECE_SELECTED;
  }
  else
  {
	if (bFocus == true)
	  m_nState &= ~(LC_PIECE_FOCUSED);
	else
	  m_nState &= ~(LC_PIECE_SELECTED|LC_PIECE_FOCUSED);
  }
}

void Piece::InsertTime (unsigned short start, bool animation, unsigned short time)
{
  if (animation)
  {
	if (m_nFrameShow >= start)
	  m_nFrameShow = lcMin(m_nFrameShow + time, lcGetActiveProject()->GetTotalFrames());

	if (m_nFrameHide >= start)
	  m_nFrameHide = lcMin(m_nFrameHide + time, lcGetActiveProject()->GetTotalFrames());

	if (m_nFrameShow > lcGetActiveProject()->GetCurrentTime())
	  Select (false, false, false);
  }
  else
  {
	if (m_nStepShow >= start)
	  m_nStepShow = lcMin(m_nStepShow + time, 255);

	if (m_nStepHide >= start)
	  m_nStepHide = lcMin(m_nStepHide + time, 255);

	if (m_nStepShow > lcGetActiveProject()->GetCurrentTime ())
	  Select (false, false, false);
  }

  Object::InsertTime (start, animation, time);
}

void Piece::RemoveTime (unsigned short start, bool animation, unsigned short time)
{
  if (animation)
  {
	if (m_nFrameShow >= start)
	  m_nFrameShow = lcMax(m_nFrameShow - time, 1);

	if (m_nFrameHide == lcGetActiveProject()->GetTotalFrames())
	  m_nFrameHide = lcGetActiveProject()->GetTotalFrames();
	else
	  m_nFrameHide = lcMax(m_nFrameHide - time, 1);

	if (m_nFrameHide < lcGetActiveProject()->GetCurrentTime())
	  Select (false, false, false);
  }
  else
  {
	if (m_nStepShow >= start)
	  m_nStepShow = lcMax(m_nStepShow - time, 1);

	if (m_nStepHide != 255)
	  m_nStepHide = lcMax(m_nStepHide - time, 1);

	if (m_nStepHide < lcGetActiveProject()->GetCurrentTime())
	  Select (false, false, false);
  }

  Object::RemoveTime (start, animation, time);
}

void Piece::Move(unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz)
{
	mPosition[0] += dx;
	mPosition[1] += dy;
	mPosition[2] += dz;

	ChangeKey(nTime, bAnimation, bAddKey, mPosition, LC_PK_POSITION);

	mModelWorld.SetTranslation(mPosition);
}

bool Piece::IsVisible(unsigned short nTime, bool bAnimation)
{
	if (m_nState & LC_PIECE_HIDDEN)
		return false;

	if (bAnimation)
	{
		if (m_nFrameShow > nTime) return false;
		if (m_nFrameHide < nTime) return false;
		return true;
	}
	else
	{
		if (m_nStepShow > nTime) return false;
		if ((m_nStepHide == 255) || (m_nStepHide > nTime))
			return true;
		return false;
	}
}

void Piece::CompareBoundingBox(float box[6])
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

Group* Piece::GetTopGroup()
{
	return m_pGroup ? m_pGroup->GetTopGroup() : NULL;
}

void Piece::DoGroup(Group* pGroup)
{
	if (m_pGroup != NULL && m_pGroup != (Group*)-1 && m_pGroup > (Group*)0xFFFF)
		m_pGroup->SetGroup(pGroup);
	else
		m_pGroup = pGroup;
}

void Piece::UnGroup(Group* pGroup)
{
	if ((m_pGroup == pGroup) || (pGroup == NULL))
		m_pGroup = NULL;
	else
		if (m_pGroup != NULL)
			m_pGroup->UnGroup(pGroup);
}

// Recalculates current position and connections
void Piece::UpdatePosition(unsigned short nTime, bool bAnimation)
{
	if (!IsVisible(nTime, bAnimation))
		m_nState &= ~(LC_PIECE_SELECTED|LC_PIECE_FOCUSED);

	CalculateKeys (nTime, bAnimation);

	mModelWorld = lcMatrix44FromAxisAngle(lcVector3(mRotation[0], mRotation[1], mRotation[2]), mRotation[3] * LC_DTOR);
	mModelWorld.SetTranslation(mPosition);
}

void Piece::Render(bool bLighting, bool bEdges)
{
	glPushMatrix();
	glMultMatrixf(mModelWorld);
	mPieceInfo->mMesh->Render(mColorIndex, (m_nState & LC_PIECE_SELECTED) != 0, (m_nState & LC_PIECE_FOCUSED) != 0);
	glPopMatrix();
}
*/

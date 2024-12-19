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
#include "lc_scene.h"
#include "lc_qutils.h"
#include "lc_synth.h"
#include "lc_traintrack.h"

constexpr float LC_PIECE_CONTROL_POINT_SIZE = 10.0f;

lcPiece::lcPiece(PieceInfo* Info)
	: lcObject(lcObjectType::Piece)
{
	SetPieceInfo(Info, QString(), true);
	mColorIndex = gDefaultColor;
	mColorCode = 16;
	mStepShow = 1;
	mStepHide = LC_STEP_MAX;
	mGroup = nullptr;
	mPivotMatrix = lcMatrix44Identity();
}

lcPiece::lcPiece(const lcPiece& Other)
	: lcObject(lcObjectType::Piece)
{
	SetPieceInfo(Other.mPieceInfo, Other.mID, true);
	mHidden = Other.mHidden;
	mSelected = Other.mSelected;
	mColorIndex = Other.mColorIndex;
	mColorCode = Other.mColorCode;
	mStepShow = Other.mStepShow;
	mStepHide = Other.mStepHide;
	mGroup = Other.mGroup;

	mPivotMatrix = Other.mPivotMatrix;
	mPivotPointValid = Other.mPivotPointValid;

	mPosition = Other.mPosition;
	mRotation = Other.mRotation;
	mControlPoints = Other.mControlPoints;

	UpdateMesh();
}

lcPiece::~lcPiece()
{
	if (mPieceInfo)
	{
		lcPiecesLibrary* Library = lcGetPiecesLibrary();
		Library->ReleasePieceInfo(mPieceInfo);
	}

	delete mMesh;
}

void lcPiece::SetPieceInfo(PieceInfo* Info, const QString& ID, bool Wait)
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	mPieceInfo = Info;
	if (mPieceInfo)
		Library->LoadPieceInfo(mPieceInfo, Wait, true);

	if (!ID.isEmpty())
		mID = ID;
	else if (mPieceInfo)
		mID = mPieceInfo->mFileName;
	else
		mID.clear();

	mControlPoints.clear();
	delete mMesh;
	mMesh = nullptr;

	const lcSynthInfo* SynthInfo = mPieceInfo ? mPieceInfo->GetSynthInfo() : nullptr;

	if (SynthInfo)
	{
		SynthInfo->GetDefaultControlPoints(mControlPoints);
		UpdateMesh();
	}
}

bool lcPiece::SetPieceId(PieceInfo* Info)
{
	if (mPieceInfo == Info)
		return false;

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	Library->ReleasePieceInfo(mPieceInfo);
	SetPieceInfo(Info, QString(), true);

	return true;
}

void lcPiece::UpdateID()
{
	mID = mPieceInfo->mFileName;
}

void lcPiece::SaveLDraw(QTextStream& Stream) const
{
	const QLatin1String LineEnding("\r\n");

	if (mStepHide != LC_STEP_MAX)
		Stream << QLatin1String("0 !LEOCAD PIECE STEP_HIDE ") << mStepHide << LineEnding;

	if (IsHidden())
		Stream << QLatin1String("0 !LEOCAD PIECE HIDDEN") << LineEnding;

	if (mPivotPointValid)
	{
		const float* PivotMatrix = mPivotMatrix;
		const float PivotNumbers[12] = { PivotMatrix[12], -PivotMatrix[14], PivotMatrix[13], PivotMatrix[0], -PivotMatrix[8], PivotMatrix[4], -PivotMatrix[2], PivotMatrix[10], -PivotMatrix[6], PivotMatrix[1], -PivotMatrix[9], PivotMatrix[5] };

		Stream << QLatin1String("0 !LEOCAD PIECE PIVOT ");

		for (int NumberIdx = 0; NumberIdx < 12; NumberIdx++)
			Stream << ' ' << lcFormatValue(PivotNumbers[NumberIdx], NumberIdx < 3 ? 4 : 6);

		Stream << LineEnding;
	}

	mPosition.Save(Stream, "PIECE", "POSITION", false);
	mRotation.Save(Stream, "PIECE", "ROTATION", false);

	Stream << "1 " << mColorCode << ' ';

	const float* Matrix = mModelWorld;
	const float Numbers[12] = { Matrix[12], -Matrix[14], Matrix[13], Matrix[0], -Matrix[8], Matrix[4], -Matrix[2], Matrix[10], -Matrix[6], Matrix[1], -Matrix[9], Matrix[5] };

	for (int NumberIdx = 0; NumberIdx < 12; NumberIdx++)
		Stream << lcFormatValue(Numbers[NumberIdx], NumberIdx < 3 ? 4 : 6) << ' ';

	Stream << mID << LineEnding;
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

			const lcMatrix44 PivotMatrix(lcVector4( PivotNumbers[3],  PivotNumbers[9], -PivotNumbers[6], 0.0f), lcVector4(PivotNumbers[5], PivotNumbers[11], -PivotNumbers[8], 0.0f),
								         lcVector4(-PivotNumbers[4], -PivotNumbers[10], PivotNumbers[7], 0.0f), lcVector4(PivotNumbers[0], PivotNumbers[2],  -PivotNumbers[1], 1.0f));

			mPivotMatrix = PivotMatrix;
			mPivotPointValid = true;
		}
		else if (mPosition.Load(Stream, Token, "POSITION"))
			continue;
		else if (mRotation.Load(Stream, Token, "ROTATION"))
			continue;
	}

	return false;
}

bool lcPiece::FileLoad(lcFile& file)
{
	quint8 version, ch;

	version = file.ReadU8();

	if (version > 12) // LeoCAD 0.80
		return false;

	const float PositionScale = (version < 12) ? 25.0f : 1.0f;

	if (version > 8)
	{
		if (file.ReadU8() != 1)
			return false;

		quint16 time;
		float param[4];
		quint8 type;
		quint32 n;

		file.ReadU32(&n, 1);
		while (n--)
		{
			file.ReadU16(&time, 1);
			file.ReadFloats(param, 4);
			file.ReadU8(&type, 1);

			if (type == 0)
				mPosition.ChangeKey(lcVector3(param[0], param[1], param[2]) * PositionScale, time, true);
			else if (type == 1)
				mRotation.ChangeKey(lcMatrix33FromAxisAngle(lcVector3(param[0], param[1], param[2]), param[3] * LC_DTOR), time, true);
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
		quint16 time;
		quint8 type;

		if (version > 5)
		{
			quint32 keys;
			float param[4];

			file.ReadU32(&keys, 1);
			while (keys--)
			{
				file.ReadFloats(param, 4);
				file.ReadU16(&time, 1);
				file.ReadU8(&type, 1);

				if (type == 0)
					mPosition.ChangeKey(lcVector3(param[0], param[1], param[2]) * PositionScale, time, true);
				else if (type == 1)
					mRotation.ChangeKey(lcMatrix33FromAxisAngle(lcVector3(param[0], param[1], param[2]), param[3] * LC_DTOR), time, true);
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

					quint8 b;
					file.ReadU8(&b, 1);
					time = b;

					mPosition.ChangeKey(ModelWorld.GetTranslation() * PositionScale, 1, true);
					mRotation.ChangeKey(lcMatrix33(ModelWorld), time, true);

					qint32 bl;
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

				mPosition.ChangeKey(lcVector3(ModelWorld.r[3][0], ModelWorld.r[3][1], ModelWorld.r[3][2]) * PositionScale, 1, true);
				mRotation.ChangeKey(lcMatrix33(ModelWorld), 1, true);
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
	strcat(name, ".dat");

	PieceInfo* pInfo = lcGetPiecesLibrary()->FindPiece(name, nullptr, true, false);
	SetPieceInfo(pInfo, QString(), true);

	// 11 (0.77)
	if (version < 11)
	{
		quint8 Color;

		file.ReadU8(&Color, 1);

		if (version < 5)
			mColorCode = lcGetColorCodeFromOriginalColor(Color);
		else
			mColorCode = lcGetColorCodeFromExtendedColor(Color);
	}
	else
		file.ReadU32(&mColorCode, 1);
	mColorIndex = lcGetColorIndex(mColorCode);

	quint8 Step;
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
			quint8 Hidden;
			file.ReadU8(&Hidden, 1);
			if (Hidden & 1)
				mHidden = true;
			file.ReadU8(&ch, 1);
			file.Seek(ch, SEEK_CUR);
		}
		else
		{
			qint32 hide;
			file.ReadS32(&hide, 1);
			if (hide != 0)
				mHidden = true;
			file.Seek(81, SEEK_CUR);
		}

		// 7 (0.64)
		qint32 i = -1;
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
			mHidden = true;
	}

	return true;
}

void lcPiece::Initialize(const lcMatrix44& WorldMatrix, lcStep Step)
{
	mStepShow = Step;

	mPosition.SetValue(WorldMatrix.GetTranslation());
	mRotation.SetValue(lcMatrix33(WorldMatrix));

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

	mPosition.InsertTime(Start, Time);
	mRotation.InsertTime(Start, Time);
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

	mPosition.RemoveTime(Start, Time);
	mRotation.RemoveTime(Start, Time);
}

void lcPiece::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	const lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(mModelWorld);
	const lcVector3 Start = lcMul31(ObjectRayTest.Start, InverseWorldMatrix);
	const lcVector3 End = lcMul31(ObjectRayTest.End, InverseWorldMatrix);

	if (mMesh)
	{
		if (mMesh->MinIntersectDist(Start, End, ObjectRayTest.Distance, ObjectRayTest.PieceInfoRayTest.Plane))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcPiece*>(this);
			ObjectRayTest.ObjectSection.Section = LC_PIECE_SECTION_POSITION;
			ObjectRayTest.PieceInfoRayTest.Transform = mModelWorld;
		}
	}
	else
	{
		if (mPieceInfo->MinIntersectDist(Start, End, ObjectRayTest.Distance, ObjectRayTest.PieceInfoRayTest))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcPiece*>(this);
			ObjectRayTest.ObjectSection.Section = LC_PIECE_SECTION_POSITION;
			ObjectRayTest.PieceInfoRayTest.Transform = lcMul(ObjectRayTest.PieceInfoRayTest.Transform, mModelWorld);
		}
	}

	if (mPieceInfo->GetSynthInfo() && AreControlPointsVisible())
	{
		const lcVector3 Min(-LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE);
		const lcVector3 Max(LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE);

		for (quint32 ControlPointIndex = 0; ControlPointIndex < mControlPoints.size(); ControlPointIndex++)
		{
			const lcMatrix44 InverseTransform = lcMatrix44AffineInverse(mControlPoints[ControlPointIndex].Transform);
			const lcVector3 PointStart = lcMul31(Start, InverseTransform);
			const lcVector3 PointEnd = lcMul31(End, InverseTransform);

			float Distance;
			lcVector3 Plane;

			if (lcBoundingBoxRayIntersectDistance(Min, Max, PointStart, PointEnd, &Distance, nullptr, &Plane))
			{
				ObjectRayTest.ObjectSection.Object = const_cast<lcPiece*>(this);
				ObjectRayTest.ObjectSection.Section = LC_PIECE_SECTION_CONTROL_POINT_FIRST + ControlPointIndex;
				ObjectRayTest.Distance = Distance;
				ObjectRayTest.PieceInfoRayTest.Plane = Plane;
			}
		}
	}

	if (mPieceInfo->GetTrainTrackInfo() && AreTrainTrackConnectionsVisible())
	{
		const lcVector3 Min(-LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE);
		const lcVector3 Max(LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE);
		const std::vector<lcTrainTrackConnection>& Connections = mPieceInfo->GetTrainTrackInfo()->GetConnections();

		for (quint32 ConnectionIndex = 0; ConnectionIndex < Connections.size(); ConnectionIndex++)
		{
			const lcMatrix44 InverseTransform = lcMatrix44AffineInverse(Connections[ConnectionIndex].Transform);
			const lcVector3 PointStart = lcMul31(Start, InverseTransform);
			const lcVector3 PointEnd = lcMul31(End, InverseTransform);

			float Distance;
			lcVector3 Plane;

			if (lcBoundingBoxRayIntersectDistance(Min, Max, PointStart, PointEnd, &Distance, nullptr, &Plane))
			{
				ObjectRayTest.ObjectSection.Object = const_cast<lcPiece*>(this);
				ObjectRayTest.ObjectSection.Section = LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex;
				ObjectRayTest.Distance = Distance;
				ObjectRayTest.PieceInfoRayTest.Plane = Plane;
			}
		}
	}
}

void lcPiece::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	if (mPieceInfo->BoxTest(mModelWorld, ObjectBoxTest.Planes))
		ObjectBoxTest.Objects.emplace_back(const_cast<lcPiece*>(this));
}

void lcPiece::DrawInterface(lcContext* Context, const lcScene& Scene) const
{
	const lcPreferences& Preferences = lcGetPreferences();
	const float LineWidth = Preferences.mLineWidth;

	Context->SetLineWidth(2.0f * LineWidth);

	const lcBoundingBox& BoundingBox = GetBoundingBox();
	const lcVector3& Min = BoundingBox.Min;
	const lcVector3& Max = BoundingBox.Max;
	lcVector3 Edge((Max - Min) * 0.33f);

	const float LineVerts[48][3] =
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

	const lcMatrix44 WorldMatrix = Scene.ApplyActiveSubmodelTransform(mModelWorld);
	Context->SetMaterial(lcMaterialType::UnlitColor);
	Context->SetWorldMatrix(WorldMatrix);

	if (IsFocused(LC_PIECE_SECTION_POSITION))
	{
		const lcVector4 FocusedColor = lcVector4FromColor(Preferences.mObjectFocusedColor);
		Context->SetColor(FocusedColor);
	}
	else
	{
		const lcVector4 SelectedColor = lcVector4FromColor(Preferences.mObjectSelectedColor);
		Context->SetColor(SelectedColor);
	}

	Context->SetVertexBufferPointer(LineVerts);
	Context->SetVertexFormatPosition(3);

	Context->DrawPrimitives(GL_LINES, 0, 48);

	if (IsPivotPointVisible())
	{
		constexpr float Size = 5.0f;
		constexpr float Verts[8 * 3] =
		{
			-Size, -Size, -Size, -Size,  Size, -Size, Size,  Size, -Size, Size, -Size, -Size,
			-Size, -Size,  Size, -Size,  Size,  Size, Size,  Size,  Size, Size, -Size,  Size
		};

		const GLushort Indices[24] =
		{
			0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7
		};

		Context->SetWorldMatrix(lcMul(mPivotMatrix, WorldMatrix));

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormatPosition(3);
		Context->SetIndexBufferPointer(Indices);

		Context->DrawIndexedPrimitives(GL_LINES, 24, GL_UNSIGNED_SHORT, 0);
	}

	if (mPieceInfo->GetSynthInfo())
		DrawSynthInterface(Context, WorldMatrix);
	else if (mPieceInfo->GetTrainTrackInfo())
		DrawTrainTrackInterface(Context, WorldMatrix);
}

void lcPiece::DrawSynthInterface(lcContext* Context, const lcMatrix44& WorldMatrix) const
{
	if (mControlPoints.empty() || !AreControlPointsVisible())
		return;

	float Verts[8 * 3];
	float* CurVert = Verts;

	lcVector3 CubeMin(-LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE);
	lcVector3 CubeMax(LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE);

	*CurVert++ = CubeMin[0]; *CurVert++ = CubeMin[1]; *CurVert++ = CubeMin[2];
	*CurVert++ = CubeMin[0]; *CurVert++ = CubeMax[1]; *CurVert++ = CubeMin[2];
	*CurVert++ = CubeMax[0]; *CurVert++ = CubeMax[1]; *CurVert++ = CubeMin[2];
	*CurVert++ = CubeMax[0]; *CurVert++ = CubeMin[1]; *CurVert++ = CubeMin[2];
	*CurVert++ = CubeMin[0]; *CurVert++ = CubeMin[1]; *CurVert++ = CubeMax[2];
	*CurVert++ = CubeMin[0]; *CurVert++ = CubeMax[1]; *CurVert++ = CubeMax[2];
	*CurVert++ = CubeMax[0]; *CurVert++ = CubeMax[1]; *CurVert++ = CubeMax[2];
	*CurVert++ = CubeMax[0]; *CurVert++ = CubeMin[1]; *CurVert++ = CubeMax[2];

	const GLushort Indices[36] =
	{
		0, 1, 2, 0, 2, 3, 7, 6, 5, 7, 5, 4, 5, 1, 0, 4, 5, 0,
		7, 3, 2, 6, 7, 2, 0, 3, 7, 0, 7, 4, 6, 2, 1, 5, 6, 1
	};

	Context->EnableColorBlend(true);
	Context->EnableCullFace(true);

	const lcPreferences& Preferences = lcGetPreferences();
	const lcVector4 ControlPointColor = lcVector4FromColor(Preferences.mControlPointColor);
	const lcVector4 ControlPointFocusedColor = lcVector4FromColor(Preferences.mControlPointFocusedColor);

	for (quint32 ControlPointIndex = 0; ControlPointIndex < mControlPoints.size(); ControlPointIndex++)
	{
		Context->SetWorldMatrix(lcMul(mControlPoints[ControlPointIndex].Transform, WorldMatrix));

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormatPosition(3);
		Context->SetIndexBufferPointer(Indices);

		if (IsFocused(LC_PIECE_SECTION_CONTROL_POINT_FIRST + ControlPointIndex))
			Context->SetColor(ControlPointFocusedColor);
		else
			Context->SetColor(ControlPointColor);

		Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
	}

	Context->EnableCullFace(false);
	Context->EnableColorBlend(false);
}

void lcPiece::DrawTrainTrackInterface(lcContext* Context, const lcMatrix44& WorldMatrix) const
{
	if (!AreTrainTrackConnectionsVisible())
		return;

	float Verts[8 * 3];
	float* CurVert = Verts;

	lcVector3 CubeMin(-LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE, -LC_PIECE_CONTROL_POINT_SIZE);
	lcVector3 CubeMax(LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE, LC_PIECE_CONTROL_POINT_SIZE);

	*CurVert++ = CubeMin[0]; *CurVert++ = CubeMin[1]; *CurVert++ = CubeMin[2];
	*CurVert++ = CubeMin[0]; *CurVert++ = CubeMax[1]; *CurVert++ = CubeMin[2];
	*CurVert++ = CubeMax[0]; *CurVert++ = CubeMax[1]; *CurVert++ = CubeMin[2];
	*CurVert++ = CubeMax[0]; *CurVert++ = CubeMin[1]; *CurVert++ = CubeMin[2];
	*CurVert++ = CubeMin[0]; *CurVert++ = CubeMin[1]; *CurVert++ = CubeMax[2];
	*CurVert++ = CubeMin[0]; *CurVert++ = CubeMax[1]; *CurVert++ = CubeMax[2];
	*CurVert++ = CubeMax[0]; *CurVert++ = CubeMax[1]; *CurVert++ = CubeMax[2];
	*CurVert++ = CubeMax[0]; *CurVert++ = CubeMin[1]; *CurVert++ = CubeMax[2];

	const GLushort Indices[36] =
	{
		0, 1, 2, 0, 2, 3, 7, 6, 5, 7, 5, 4, 5, 1, 0, 4, 5, 0,
		7, 3, 2, 6, 7, 2, 0, 3, 7, 0, 7, 4, 6, 2, 1, 5, 6, 1
	};

	const lcPreferences& Preferences = lcGetPreferences();
	const lcVector4 ConnectionColor = lcVector4FromColor(Preferences.mControlPointColor);
	const lcVector4 ConnectionFocusedColor = lcVector4FromColor(Preferences.mControlPointFocusedColor);

	const lcTrainTrackInfo* TrainTrackInfo = mPieceInfo->GetTrainTrackInfo();
	const std::vector<lcTrainTrackConnection>& Connections = TrainTrackInfo->GetConnections();

	for (quint32 ConnectionIndex = 0; ConnectionIndex < Connections.size(); ConnectionIndex++)
	{
		Context->SetWorldMatrix(lcMul(Connections[ConnectionIndex].Transform, WorldMatrix));

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormatPosition(3);
		Context->SetIndexBufferPointer(Indices);

		if (IsFocused(LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex))
			Context->SetColor(ConnectionFocusedColor);
		else
			Context->SetColor(ConnectionColor);

		Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
	}

	mPieceInfo->GetTrainTrackInfo()->GetConnections();
}

QVariant lcPiece::GetPropertyValue(lcObjectPropertyId PropertyId) const
{
	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
		return QVariant::fromValue<void*>(mPieceInfo);

	case lcObjectPropertyId::PieceColor:
		return GetColorIndex();

	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
	case lcObjectPropertyId::CameraName:
	case lcObjectPropertyId::CameraType:
	case lcObjectPropertyId::CameraFOV:
	case lcObjectPropertyId::CameraNear:
	case lcObjectPropertyId::CameraFar:
	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
	case lcObjectPropertyId::LightName:
	case lcObjectPropertyId::LightType:
	case lcObjectPropertyId::LightColor:
	case lcObjectPropertyId::LightBlenderPower:
	case lcObjectPropertyId::LightPOVRayPower:
	case lcObjectPropertyId::LightCastShadow:
	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointBlenderRadius:
	case lcObjectPropertyId::LightSpotBlenderRadius:
	case lcObjectPropertyId::LightDirectionalBlenderAngle:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
	case lcObjectPropertyId::LightAreaShape:
	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
	case lcObjectPropertyId::Count:
		break;
	}

	return QVariant();
}

bool lcPiece::SetPropertyValue(lcObjectPropertyId PropertyId, lcStep Step, bool AddKey, QVariant Value)
{
	Q_UNUSED(Step);
	Q_UNUSED(AddKey);

	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
		return SetPieceId(static_cast<PieceInfo*>(Value.value<void*>()));

	case lcObjectPropertyId::PieceColor:
		return SetColorIndex(Value.toInt());

	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
	case lcObjectPropertyId::CameraName:
	case lcObjectPropertyId::CameraType:
	case lcObjectPropertyId::CameraFOV:
	case lcObjectPropertyId::CameraNear:
	case lcObjectPropertyId::CameraFar:
	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
	case lcObjectPropertyId::LightName:
	case lcObjectPropertyId::LightType:
	case lcObjectPropertyId::LightColor:
	case lcObjectPropertyId::LightBlenderPower:
	case lcObjectPropertyId::LightPOVRayPower:
	case lcObjectPropertyId::LightCastShadow:
	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointBlenderRadius:
	case lcObjectPropertyId::LightSpotBlenderRadius:
	case lcObjectPropertyId::LightDirectionalBlenderAngle:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
	case lcObjectPropertyId::LightAreaShape:
	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
	case lcObjectPropertyId::Count:
		break;
	}

	return false;
}

bool lcPiece::HasKeyFrame(lcObjectPropertyId PropertyId, lcStep Time) const
{
	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
	case lcObjectPropertyId::PieceColor:
	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
	case lcObjectPropertyId::CameraName:
	case lcObjectPropertyId::CameraType:
	case lcObjectPropertyId::CameraFOV:
	case lcObjectPropertyId::CameraNear:
	case lcObjectPropertyId::CameraFar:
	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
	case lcObjectPropertyId::LightName:
	case lcObjectPropertyId::LightType:
	case lcObjectPropertyId::LightColor:
	case lcObjectPropertyId::LightBlenderPower:
	case lcObjectPropertyId::LightPOVRayPower:
	case lcObjectPropertyId::LightCastShadow:
	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointBlenderRadius:
	case lcObjectPropertyId::LightSpotBlenderRadius:
	case lcObjectPropertyId::LightDirectionalBlenderAngle:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
	case lcObjectPropertyId::LightAreaShape:
	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
		return false;

	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
		return mPosition.HasKeyFrame(Time);

	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
		return mRotation.HasKeyFrame(Time);

	case lcObjectPropertyId::Count:
		return false;
	}

	return false;
}

bool lcPiece::SetKeyFrame(lcObjectPropertyId PropertyId, lcStep Time, bool KeyFrame)
{
	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
	case lcObjectPropertyId::PieceColor:
	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
	case lcObjectPropertyId::CameraName:
	case lcObjectPropertyId::CameraType:
	case lcObjectPropertyId::CameraFOV:
	case lcObjectPropertyId::CameraNear:
	case lcObjectPropertyId::CameraFar:
	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
	case lcObjectPropertyId::LightName:
	case lcObjectPropertyId::LightType:
	case lcObjectPropertyId::LightColor:
	case lcObjectPropertyId::LightBlenderPower:
	case lcObjectPropertyId::LightPOVRayPower:
	case lcObjectPropertyId::LightCastShadow:
	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointBlenderRadius:
	case lcObjectPropertyId::LightSpotBlenderRadius:
	case lcObjectPropertyId::LightDirectionalBlenderAngle:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
	case lcObjectPropertyId::LightAreaShape:
	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
		return false;

	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
		return mPosition.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
		return mRotation.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::Count:
		return false;
	}

	return false;
}

void lcPiece::RemoveKeyFrames()
{
	mPosition.RemoveAllKeys();
	mRotation.RemoveAllKeys();
}

void lcPiece::AddMainModelRenderMeshes(lcScene* Scene, bool Highlight, bool Fade) const
{
	lcRenderMeshState RenderMeshState = lcRenderMeshState::Default;
	bool ParentActive = false;

	if (Highlight)
		RenderMeshState = lcRenderMeshState::Highlighted;

	if (Fade)
		RenderMeshState = lcRenderMeshState::Faded;

	if (Scene->GetDrawInterface())
	{
		const lcPiece* ActiveSubmodelInstance = Scene->GetActiveSubmodelInstance();

		if (!ActiveSubmodelInstance)
			RenderMeshState = IsFocused() ? lcRenderMeshState::Focused : (IsSelected() ? lcRenderMeshState::Selected : RenderMeshState);
		else if (ActiveSubmodelInstance == this)
			ParentActive = true;
		else
			RenderMeshState = lcRenderMeshState::Faded;
	}

	if (!mMesh)
		mPieceInfo->AddRenderMeshes(Scene, mModelWorld, mColorIndex, RenderMeshState, ParentActive);
	else
		Scene->AddMesh(mMesh, mModelWorld, mColorIndex, RenderMeshState);

	if (RenderMeshState == lcRenderMeshState::Focused || RenderMeshState == lcRenderMeshState::Selected)
		Scene->AddInterfaceObject(this);
}

void lcPiece::AddSubModelRenderMeshes(lcScene* Scene, const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const
{
	int ColorIndex = mColorIndex;

	if (ColorIndex == gDefaultColor)
		ColorIndex = DefaultColorIndex;

	const lcPiece* ActiveSubmodelInstance = Scene->GetActiveSubmodelInstance();

	if (ActiveSubmodelInstance == this)
		RenderMeshState = lcRenderMeshState::Default;
	else if (ParentActive)
		RenderMeshState = IsFocused() ? lcRenderMeshState::Focused : (IsSelected() ? lcRenderMeshState::Selected : lcRenderMeshState::Default);

	if (!mMesh)
		mPieceInfo->AddRenderMeshes(Scene, lcMul(mModelWorld, WorldMatrix), ColorIndex, RenderMeshState, ActiveSubmodelInstance == this);
	else
		Scene->AddMesh(mMesh, lcMul(mModelWorld, WorldMatrix), ColorIndex, RenderMeshState);

	if (ParentActive && (RenderMeshState == lcRenderMeshState::Focused || RenderMeshState == lcRenderMeshState::Selected))
		Scene->AddInterfaceObject(this);
}

void lcPiece::SubModelCompareBoundingBox(const lcMatrix44& WorldMatrix, lcVector3& Min, lcVector3& Max) const
{
	mPieceInfo->CompareBoundingBox(lcMul(mModelWorld, WorldMatrix), Min, Max);
}

void lcPiece::SubModelAddBoundingBoxPoints(const lcMatrix44& WorldMatrix, std::vector<lcVector3>& Points) const
{
	if (!mMesh)
		mPieceInfo->AddSubModelBoundingBoxPoints(lcMul(mModelWorld, WorldMatrix), Points);
	else
	{
		lcVector3 BoxPoints[8];

		lcGetBoxCorners(mMesh->mBoundingBox, BoxPoints);

		for (int i = 0; i < 8; i++)
			Points.emplace_back(lcMul31(BoxPoints[i], mModelWorld));
	}
}

void lcPiece::MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance)
{
	const quint32 Section = GetFocusSection();

	if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
	{
		const lcVector3 Position = mModelWorld.GetTranslation() + Distance;

		SetPosition(Position, Step, AddKey);

		mModelWorld.SetTranslation(Position);
	}
	else if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST)
	{
		const quint32 ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;

		if (ControlPointIndex < mControlPoints.size())
		{
			const lcMatrix33 InverseWorldMatrix = lcMatrix33AffineInverse(lcMatrix33(mModelWorld));
			lcMatrix44& Transform = mControlPoints[ControlPointIndex].Transform;

			Transform.SetTranslation(Transform.GetTranslation() + lcMul(Distance, InverseWorldMatrix));
		}

		UpdateMesh();
	}
}

void lcPiece::Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcVector3& Center, const lcMatrix33& RotationFrame)
{
	const quint32 Section = GetFocusSection();

	if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
	{
		lcVector3 Distance = mModelWorld.GetTranslation() - Center;
		const lcMatrix33 LocalToWorldMatrix = lcMatrix33(mModelWorld);

		const lcMatrix33 LocalToFocusMatrix = lcMul(LocalToWorldMatrix, RotationFrame);
		lcMatrix33 NewLocalToWorldMatrix = lcMul(LocalToFocusMatrix, RotationMatrix);

		const lcMatrix33 WorldToLocalMatrix = lcMatrix33AffineInverse(LocalToWorldMatrix);

		Distance = lcMul(Distance, WorldToLocalMatrix);
		Distance = lcMul(Distance, NewLocalToWorldMatrix);

		NewLocalToWorldMatrix.Orthonormalize();

		SetPosition(Center + Distance, Step, AddKey);
		SetRotation(NewLocalToWorldMatrix, Step, AddKey);
	}
	else if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST)
	{
		const quint32 ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;

		if (ControlPointIndex < mControlPoints.size())
		{
			lcMatrix44& Transform = mControlPoints[ControlPointIndex].Transform;
			const lcMatrix33 PieceWorldMatrix(mModelWorld);
			const lcMatrix33 LocalToWorldMatrix = lcMul(lcMatrix33(Transform), PieceWorldMatrix);

			const lcMatrix33 LocalToFocusMatrix = lcMul(LocalToWorldMatrix, RotationFrame);
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

	mPivotMatrix.SetTranslation(mPivotMatrix.GetTranslation() + lcMul30(Distance, lcMatrix44AffineInverse(mModelWorld)));
	mPivotPointValid = true;
}

void lcPiece::RotatePivotPoint(const lcMatrix33& RotationMatrix)
{
	if (!IsFocused(LC_PIECE_SECTION_POSITION))
		return;

	lcMatrix33 NewPivotRotationMatrix = lcMul(RotationMatrix, lcMatrix33(mPivotMatrix));
	NewPivotRotationMatrix.Orthonormalize();

	mPivotMatrix = lcMatrix44(NewPivotRotationMatrix, mPivotMatrix.GetTranslation());
	mPivotPointValid = true;
}

quint32 lcPiece::GetAllowedTransforms() const
{
	const quint32 Section = GetFocusSection();

	if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
		return LC_OBJECT_TRANSFORM_MOVE_XYZ | LC_OBJECT_TRANSFORM_ROTATE_XYZ;

	const lcSynthInfo* SynthInfo = mPieceInfo->GetSynthInfo();

	if (SynthInfo)
	{
		if (SynthInfo->IsUnidirectional())
			return LC_OBJECT_TRANSFORM_MOVE_Z;

		if (SynthInfo->IsCurve())
			return LC_OBJECT_TRANSFORM_MOVE_XYZ | LC_OBJECT_TRANSFORM_ROTATE_XYZ | LC_OBJECT_TRANSFORM_SCALE_X;

		if (SynthInfo->IsNondirectional())
			return LC_OBJECT_TRANSFORM_MOVE_XYZ;
	}

	return 0;
}

lcVector3 lcPiece::GetSectionPosition(quint32 Section) const
{
	if (Section == LC_PIECE_SECTION_POSITION)
	{
		if (mPivotPointValid)
			return lcMul(mPivotMatrix, mModelWorld).GetTranslation();
		else
			return mModelWorld.GetTranslation();
	}

	if (mPieceInfo->GetSynthInfo())
	{
		if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST)
		{
			const quint32 ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;

			if (ControlPointIndex < mControlPoints.size())
			{
				const lcMatrix44& Transform = mControlPoints[ControlPointIndex].Transform;
				return lcMul(Transform, mModelWorld).GetTranslation();
			}
		}
	}

	if (mPieceInfo->GetTrainTrackInfo())
	{
		if (Section >= LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
		{
			const quint32 ConnectionIndex = Section - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
			const std::vector<lcTrainTrackConnection>& Connections = mPieceInfo->GetTrainTrackInfo()->GetConnections();

			if (ConnectionIndex < Connections.size())
			{
				const lcMatrix44& Transform = Connections[ConnectionIndex].Transform;
				return lcMul(Transform, mModelWorld).GetTranslation();
			}
		}
	}

	return lcVector3(0.0f, 0.0f, 0.0f);
}

lcVector3 lcPiece::GetRotationCenter() const
{
	const quint32 Section = GetFocusSection();

	if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
	{
		if (mPivotPointValid)
			return lcMul31(mPivotMatrix.GetTranslation(), mModelWorld);
	}
	else if (mPieceInfo->GetSynthInfo())
	{
		if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST)
		{
			const quint32 ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;

			if (ControlPointIndex < mControlPoints.size())
			{
				const lcMatrix44& Transform = mControlPoints[ControlPointIndex].Transform;
				return lcMul31(Transform.GetTranslation(), mModelWorld);
			}
		}
	}
	else if (mPieceInfo->GetTrainTrackInfo())
	{
		if (Section >= LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
		{
			const quint32 ConnectionIndex = Section - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
			const std::vector<lcTrainTrackConnection>& Connections = mPieceInfo->GetTrainTrackInfo()->GetConnections();

			if (ConnectionIndex < Connections.size())
			{
				const lcMatrix44& Transform = Connections[ConnectionIndex].Transform;
				return lcMul(Transform, mModelWorld).GetTranslation();
			}
		}
	}

	return mModelWorld.GetTranslation();
}

lcMatrix33 lcPiece::GetRelativeRotation() const
{
	const quint32 Section = GetFocusSection();

	if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
	{
		if (mPivotPointValid)
			return lcMatrix33(lcMul(mModelWorld, mPivotMatrix));
		else
			return lcMatrix33(mModelWorld);
	}
	else if (mPieceInfo->GetSynthInfo())
	{
		if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST)
		{
			const quint32 ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;

			if (ControlPointIndex < mControlPoints.size())
			{
				const lcMatrix44& Transform = mControlPoints[ControlPointIndex].Transform;
				return lcMatrix33(lcMul(Transform, mModelWorld));
			}
		}
	}
	else if (mPieceInfo->GetTrainTrackInfo())
	{
		if (Section >= LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
		{
			const quint32 ConnectionIndex = Section - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
			const std::vector<lcTrainTrackConnection>& Connections = mPieceInfo->GetTrainTrackInfo()->GetConnections();

			if (ConnectionIndex < Connections.size())
			{
				const lcMatrix44& Transform = Connections[ConnectionIndex].Transform;
				return lcMatrix33(lcMul(Transform, mModelWorld));
			}
		}
	}

	return lcMatrix33Identity();
}

bool lcPiece::CanAddControlPoint() const
{
	if (mControlPoints.size() >= LC_MAX_CONTROL_POINTS)
		return false;

	const lcSynthInfo* SynthInfo = mPieceInfo->GetSynthInfo();
	return SynthInfo && SynthInfo->CanAddControlPoints();
}

bool lcPiece::CanRemoveControlPoint() const
{
	const quint32 Section = GetFocusSection();
	return Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST && Section <= LC_PIECE_SECTION_CONTROL_POINT_LAST && mControlPoints.size() > 2;
}

bool lcPiece::InsertControlPoint(const lcVector3& WorldStart, const lcVector3& WorldEnd)
{
	if (!CanAddControlPoint())
		return false;

	const lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(mModelWorld);
	const lcVector3 Start = lcMul31(WorldStart, InverseWorldMatrix);
	const lcVector3 End = lcMul31(WorldEnd, InverseWorldMatrix);

	const lcSynthInfo* SynthInfo = mPieceInfo->GetSynthInfo();
	const int ControlPointIndex = SynthInfo->InsertControlPoint(mControlPoints, Start, End);

	if (ControlPointIndex)
	{
		SetFocused(GetFocusSection(), false);
		SetFocused(LC_PIECE_SECTION_CONTROL_POINT_FIRST + ControlPointIndex, true);
		UpdateMesh();
		return true;
	}

	return false;
}

bool lcPiece::RemoveFocusedControlPoint()
{
	quint32 Section = GetFocusSection();

	if (Section < LC_PIECE_SECTION_CONTROL_POINT_FIRST)
		return false;

	const quint32 ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;

	if (ControlPointIndex >= mControlPoints.size() || mControlPoints.size() <= 2)
		return false;

	SetFocused(GetFocusSection(), false);
	SetFocused(LC_PIECE_SECTION_POSITION, true);
	mControlPoints.erase(mControlPoints.begin() + ControlPointIndex);

	UpdateMesh();

	return true;
}

void lcPiece::VerifyControlPoints(std::vector<lcPieceControlPoint>& ControlPoints) const
{
	const lcSynthInfo* SynthInfo = mPieceInfo->GetSynthInfo();

	if (!SynthInfo)
	{
		ControlPoints.clear();
	}
	else
	{
		if (ControlPoints.size() > LC_MAX_CONTROL_POINTS)
			ControlPoints.resize(LC_MAX_CONTROL_POINTS);

		SynthInfo->VerifyControlPoints(ControlPoints);
	}
}

QString lcPiece::GetName() const
{
	return QString::fromLatin1(mPieceInfo->m_strDescription);
}

bool lcPiece::IsVisible(lcStep Step) const
{
	return !mHidden && (mStepShow <= Step) && (mStepHide > Step || mStepHide == LC_STEP_MAX);
}

bool lcPiece::IsVisibleInSubModel() const
{
	return (mStepHide == LC_STEP_MAX) && !mHidden;
}

void lcPiece::GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const
{
	if (!IsVisibleInSubModel())
		return;

	int ColorIndex = mColorIndex;

	if (ColorIndex == gDefaultColor)
		ColorIndex = DefaultColorIndex;

	if (!mMesh)
		mPieceInfo->GetModelParts(lcMul(mModelWorld, WorldMatrix), ColorIndex, ModelParts);
	else
		ModelParts.emplace_back(lcModelPartsEntry{ lcMul(mModelWorld, WorldMatrix), mPieceInfo, mMesh, ColorIndex });
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
	if (!mMesh)
		mPieceInfo->CompareBoundingBox(mModelWorld, Min, Max);
	else
	{
		lcVector3 Points[8];

		lcGetBoxCorners(mMesh->mBoundingBox, Points);

		for (int i = 0; i < 8; i++)
		{
			const lcVector3 Point = lcMul31(Points[i], mModelWorld);

			Min = lcMin(Point, Min);
			Max = lcMax(Point, Max);
		}
	}
}

lcGroup* lcPiece::GetTopGroup()
{
	return mGroup ? mGroup->GetTopGroup() : nullptr;
}

void lcPiece::UpdatePosition(lcStep Step)
{
	mPosition.Update(Step);
	mRotation.Update(Step);

	mModelWorld = lcMatrix44(mRotation, mPosition);
}

void lcPiece::UpdateMesh()
{
	delete mMesh;
	const lcSynthInfo* SynthInfo = mPieceInfo->GetSynthInfo();
	mMesh = SynthInfo ? SynthInfo->CreateMesh(mControlPoints) : nullptr;
}

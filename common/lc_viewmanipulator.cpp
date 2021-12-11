#include "lc_global.h"
#include "lc_viewmanipulator.h"
#include "lc_view.h"
#include "lc_model.h"
#include "object.h"
#include "piece.h"
#include "camera.h"
#include "pieceinf.h"
#include "lc_synth.h"
#include "lc_mainwindow.h"
#include "texfont.h"

lcVertexBuffer lcViewManipulator::mRotateMoveVertexBuffer;
lcIndexBuffer lcViewManipulator::mRotateMoveIndexBuffer;

lcViewManipulator::lcViewManipulator(lcView* View)
	: mView(View)
{
}

void lcViewManipulator::CreateResources(lcContext* Context)
{
	float Verts[(51 + 138 + 10) * 3];
	float* CurVert = Verts;

	const float OverlayMovePlaneSize = 0.5f;
	const float OverlayMoveArrowSize = 1.5f;
	const float OverlayMoveArrowCapSize = 0.9f;
	const float OverlayMoveArrowCapRadius = 0.1f;
	const float OverlayMoveArrowBodySize = 1.2f;
	const float OverlayMoveArrowBodyRadius = 0.05f;
	const float OverlayRotateArrowStart = 1.0f;
	const float OverlayRotateArrowEnd = 1.5f;
	const float OverlayRotateArrowCenter = 1.2f;

	*CurVert++ = OverlayMoveArrowSize; *CurVert++ = 0.0f; *CurVert++ = 0.0f;

	for (int EdgeIdx = 0; EdgeIdx < 8; EdgeIdx++)
	{
		*CurVert++ = OverlayMoveArrowCapSize;
		*CurVert++ = cosf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius;
		*CurVert++ = sinf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius;
	}

	*CurVert++ = 0.0f; *CurVert++ = -OverlayMoveArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = OverlayMoveArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = -OverlayMoveArrowBodyRadius;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = OverlayMoveArrowBodyRadius;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = -OverlayMoveArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = OverlayMoveArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = 0.0f; *CurVert++ = -OverlayMoveArrowBodyRadius;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = 0.0f; *CurVert++ = OverlayMoveArrowBodyRadius;

	for (int VertIdx = 0; VertIdx < 17; VertIdx++)
	{
		*CurVert = *(CurVert - 50); CurVert++;
		*CurVert = *(CurVert - 52); CurVert++;
		*CurVert = *(CurVert - 51); CurVert++;
	}

	for (int VertIdx = 0; VertIdx < 17; VertIdx++)
	{
		*CurVert = *(CurVert - 100); CurVert++;
		*CurVert = *(CurVert - 102); CurVert++;
		*CurVert = *(CurVert - 104); CurVert++;
	}

	*CurVert++ = 0.0f; *CurVert++ = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius; *CurVert++ = OverlayRotateArrowStart;

	for (int EdgeIdx = 0; EdgeIdx < 8; EdgeIdx++)
	{
		*CurVert++ = cosf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius;
		*CurVert++ = sinf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius + OverlayRotateArrowEnd - OverlayMoveArrowCapRadius;
		*CurVert++ = OverlayRotateArrowCenter;
	}

	*CurVert++ = 0.0f; *CurVert++ = OverlayRotateArrowStart; *CurVert++ = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius;

	for (int EdgeIdx = 0; EdgeIdx < 8; EdgeIdx++)
	{
		*CurVert++ = cosf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius;
		*CurVert++ = OverlayRotateArrowCenter;
		*CurVert++ = sinf(LC_2PI * EdgeIdx / 8) * OverlayMoveArrowCapRadius + OverlayRotateArrowEnd - OverlayMoveArrowCapRadius;
	}

	for (int EdgeIdx = 0; EdgeIdx < 7; EdgeIdx++)
	{
		const float Radius1 = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter - OverlayMoveArrowBodyRadius;
		const float Radius2 = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter + OverlayMoveArrowBodyRadius;
		float x = cosf(LC_2PI / 4 * EdgeIdx / 6);
		float y = sinf(LC_2PI / 4 * EdgeIdx / 6);

		*CurVert++ = 0.0f;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius1;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius1;
		*CurVert++ = 0.0f;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius2;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius2;
	}

	for (int EdgeIdx = 0; EdgeIdx < 7; EdgeIdx++)
	{
		const float Radius = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter;
		float x = cosf(LC_2PI / 4 * EdgeIdx / 6);
		float y = sinf(LC_2PI / 4 * EdgeIdx / 6);

		*CurVert++ = -OverlayMoveArrowBodyRadius;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius;
		*CurVert++ = OverlayMoveArrowBodyRadius;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius;
	}

	for (int VertIdx = 0; VertIdx < 46; VertIdx++)
	{
		*CurVert = *(CurVert - 137); CurVert++;
		*CurVert = *(CurVert - 139); CurVert++;
		*CurVert = *(CurVert - 138); CurVert++;
	}

	for (int VertIdx = 0; VertIdx < 46; VertIdx++)
	{
		*CurVert = *(CurVert - 274); CurVert++;
		*CurVert = *(CurVert - 276); CurVert++;
		*CurVert = *(CurVert - 278); CurVert++;
	}

	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize; *CurVert++ = OverlayMovePlaneSize;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize;
	*CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize;
	*CurVert++ = 0.0f; *CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMovePlaneSize; *CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMovePlaneSize; *CurVert++ = 0.0f; *CurVert++ = 0.0f;

	const GLushort Indices[108 + 360 + 12] =
	{
		0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7, 0, 7, 8, 0, 8, 1,
		9, 10, 14, 14, 13, 9, 11, 12, 15, 15, 16, 12,
		17, 18, 19, 17, 19, 20, 17, 20, 21, 17, 21, 22, 17, 22, 23, 17, 23, 24, 17, 24, 25, 17, 25, 18,
		26, 27, 31, 31, 30, 26, 28, 29, 32, 32, 33, 29,
		34, 35, 36, 34, 36, 37, 34, 37, 38, 34, 38, 39, 34, 39, 40, 34, 40, 41, 34, 41, 42, 34, 42, 35,
		43, 44, 48, 48, 47, 43, 45, 46, 49, 49, 50, 46,
		51, 52, 53, 51, 53, 54, 51, 54, 55, 51, 55, 56, 51, 56, 57, 51, 57, 58, 51, 58, 59, 51, 59, 52,
		60, 61, 62, 60, 62, 63, 60, 63, 64, 60, 64, 65, 60, 65, 66, 60, 66, 67, 60, 67, 68, 60, 68, 61,
		69, 70, 71, 71, 72, 70, 71, 72, 73, 73, 74, 72, 73, 74, 75, 75, 76, 74, 75, 76, 77, 77, 78, 76, 77, 78, 79, 79, 80, 78, 79, 80, 81, 81, 82, 80,
		83, 84, 85, 85, 86, 84, 85, 86, 87, 87, 88, 86, 87, 88, 89, 89, 90, 88, 89, 90, 91, 91, 92, 90, 91, 92, 93, 93, 94, 92, 93, 94, 95, 95, 96, 94,
		97, 98, 99, 97, 99, 100, 97, 100, 101, 97, 101, 102, 97, 102, 103, 97, 103, 104, 97, 104, 105, 97, 105, 98,
		106, 107, 108, 106, 108, 109, 106, 109, 110, 106, 110, 111, 106, 111, 112, 106, 112, 113, 106, 113, 114, 106, 114, 107,
		115, 116, 117, 117, 118, 116, 117, 118, 119, 119, 120, 118, 119, 120, 121, 121, 122, 120, 121, 122, 123, 123, 124, 122, 123, 124, 125, 125, 126, 124, 125, 126, 127, 127, 128, 126,
		129, 130, 131, 131, 132, 130, 131, 132, 133, 133, 134, 132, 133, 134, 135, 135, 136, 134, 135, 136, 137, 137, 138, 136, 137, 138, 139, 139, 140, 138, 139, 140, 141, 141, 142, 140,
		143, 144, 145, 143, 145, 146, 143, 146, 147, 143, 147, 148, 143, 148, 149, 143, 149, 150, 143, 150, 151, 143, 151, 144,
		152, 153, 154, 152, 154, 155, 152, 155, 156, 152, 156, 157, 152, 157, 158, 152, 158, 159, 152, 159, 160, 152, 160, 153,
		161, 162, 163, 163, 164, 162, 163, 164, 165, 165, 166, 164, 165, 166, 167, 167, 168, 166, 167, 168, 169, 169, 170, 168, 169, 170, 171, 171, 172, 170, 171, 172, 173, 173, 174, 172,
		175, 176, 177, 177, 178, 176, 177, 178, 179, 179, 180, 178, 179, 180, 181, 181, 182, 180, 181, 182, 183, 183, 184, 182, 183, 184, 185, 185, 186, 184, 185, 186, 187, 187, 188, 186,
		189, 190, 191, 192, 189, 193, 194, 195, 189, 196, 197, 198
	};

	mRotateMoveVertexBuffer = Context->CreateVertexBuffer(sizeof(Verts), Verts);
	mRotateMoveIndexBuffer = Context->CreateIndexBuffer(sizeof(Indices), Indices);
}

void lcViewManipulator::DestroyResources(lcContext* Context)
{
	Context->DestroyVertexBuffer(mRotateMoveVertexBuffer);
	Context->DestroyIndexBuffer(mRotateMoveIndexBuffer);
}

void lcViewManipulator::DrawSelectMove(lcTrackButton TrackButton, lcTrackTool TrackTool)
{
	const lcCamera* Camera = mView->GetCamera();
	lcContext* Context = mView->mContext;

	Context->SetMaterial(lcMaterialType::UnlitColor);
	Context->SetViewMatrix(Camera->mWorldView);
	Context->SetProjectionMatrix(mView->GetProjectionMatrix());

	Context->EnableDepthTest(false);

	lcVector3 OverlayCenter;
	lcMatrix33 RelativeRotation;
	lcModel* ActiveModel = mView->GetActiveModel();
	ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation);
	bool AnyPiecesSelected = ActiveModel->AnyPiecesSelected();

	lcMatrix44 WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);

	if (ActiveModel != mView->GetModel())
		WorldMatrix = lcMul(WorldMatrix, mView->GetActiveSubmodelTransform());

	const float OverlayScale = mView->GetOverlayScale();
	WorldMatrix = lcMul(lcMatrix44Scale(lcVector3(OverlayScale, OverlayScale, OverlayScale)), WorldMatrix);

	Context->SetWorldMatrix(WorldMatrix);
	Context->SetLineWidth(1.0f);

	Context->SetIndexBuffer(mRotateMoveIndexBuffer);
	Context->SetVertexBuffer(mRotateMoveVertexBuffer);
	Context->SetVertexFormatPosition(3);

	lcObject* Focus = ActiveModel->GetFocusObject();
	quint32 AllowedTransforms = Focus ? Focus->GetAllowedTransforms() : LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z | LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z;

	if (TrackButton == lcTrackButton::None || (TrackTool >= lcTrackTool::MoveX && TrackTool <= lcTrackTool::MoveXYZ))
	{
		if (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_X)
		{
			if ((TrackTool == lcTrackTool::MoveX) || (TrackTool == lcTrackTool::MoveXY) || (TrackTool == lcTrackTool::MoveXZ))
			{
				Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
			}
			else if (TrackButton == lcTrackButton::None)
			{
				Context->SetColor(0.8f, 0.0f, 0.0f, 1.0f);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
			}
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Y)
		{
			if (((TrackTool == lcTrackTool::MoveY) || (TrackTool == lcTrackTool::MoveXY) || (TrackTool == lcTrackTool::MoveYZ)) && (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Y))
			{
				Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 36 * 2);
			}
			else if (TrackButton == lcTrackButton::None)
			{
				Context->SetColor(0.0f, 0.8f, 0.0f, 1.0f);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 36 * 2);
			}
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Z)
		{
			if (((TrackTool == lcTrackTool::MoveZ) || (TrackTool == lcTrackTool::MoveXZ) || (TrackTool == lcTrackTool::MoveYZ)) && (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Z))
			{
				Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 72 * 2);
			}
			else if (TrackButton == lcTrackButton::None)
			{
				Context->SetColor(0.0f, 0.0f, 0.8f, 1.0f);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 72 * 2);
			}
		}
	}

	if (gMainWindow->GetTool() == lcTool::Select && TrackButton == lcTrackButton::None && AnyPiecesSelected)
	{
		if (AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_X)
		{
			if (TrackTool == lcTrackTool::RotateX)
				Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			else
				Context->SetColor(0.8f, 0.0f, 0.0f, 1.0f);

			Context->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, 108 * 2);
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_Y)
		{
			if (TrackTool == lcTrackTool::RotateY)
				Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			else
				Context->SetColor(0.0f, 0.8f, 0.0f, 1.0f);

			Context->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, (108 + 120) * 2);
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_Z)
		{
			if (TrackTool == lcTrackTool::RotateZ)
				Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			else
				Context->SetColor(0.0f, 0.0f, 0.8f, 1.0f);

			Context->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, (108 + 240) * 2);
		}
	}

	if ((TrackTool == lcTrackTool::MoveXY) || (TrackTool == lcTrackTool::MoveXZ) || (TrackTool == lcTrackTool::MoveYZ))
	{
		Context->EnableColorBlend(true);

		Context->SetColor(0.8f, 0.8f, 0.0f, 0.3f);

		if (TrackTool == lcTrackTool::MoveXY)
			Context->DrawIndexedPrimitives(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, (108 + 360 + 8) * 2);
		else if (TrackTool == lcTrackTool::MoveXZ)
			Context->DrawIndexedPrimitives(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, (108 + 360 + 4) * 2);
		else if (TrackTool == lcTrackTool::MoveYZ)
			Context->DrawIndexedPrimitives(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, (108 + 360) * 2);

		Context->EnableColorBlend(false);
	}

	if (Focus && Focus->IsPiece())
	{
		lcPiece* Piece = (lcPiece*)Focus;
		quint32 Section = Piece->GetFocusSection();

		if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST && Section <= LC_PIECE_SECTION_CONTROL_POINT_LAST && Piece->mPieceInfo->GetSynthInfo() && Piece->mPieceInfo->GetSynthInfo()->IsCurve())
		{
			int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;
			float Strength = Piece->GetControlPoints()[ControlPointIndex].Scale;
			const float ScaleStart = 2.0f;
			float Length = ScaleStart + Strength / OverlayScale;
			const float OverlayScaleInnerRadius = 0.075f;
			const float OverlayScaleRadius = 0.125f;

			lcVector3 Verts[38];
			int NumVerts = 0;

			Verts[NumVerts++] = lcVector3(Length - OverlayScaleRadius, 0.0f, 0.0f);
			Verts[NumVerts++] = lcVector3(OverlayScaleRadius - Length, 0.0f, 0.0f);

			float SinTable[9], CosTable[9];

			for (int Step = 0; Step <= 8; Step++)
			{
				SinTable[Step] = sinf((float)Step / 8.0f * LC_2PI);
				CosTable[Step] = cosf((float)Step / 8.0f * LC_2PI);
			}

			for (int Step = 0; Step <= 8; Step++)
			{
				float x = CosTable[Step];
				float y = SinTable[Step];

				Verts[NumVerts++] = lcVector3(Length + x * OverlayScaleInnerRadius, 0.0f, y * OverlayScaleInnerRadius);
				Verts[NumVerts++] = lcVector3(Length + x * OverlayScaleRadius, 0.0f, y * OverlayScaleRadius);
			}

			for (int Step = 0; Step <= 8; Step++)
			{
				float x = CosTable[Step];
				float y = SinTable[Step];

				Verts[NumVerts++] = lcVector3(-Length + x * OverlayScaleInnerRadius, 0.0f, y * OverlayScaleInnerRadius);
				Verts[NumVerts++] = lcVector3(-Length + x * OverlayScaleRadius, 0.0f, y * OverlayScaleRadius);
			}

			if (TrackTool == lcTrackTool::ScalePlus || TrackTool == lcTrackTool::ScaleMinus)
				Context->SetColor(0.8f, 0.8f, 0.0f, 0.3f);
			else
				Context->SetColor(0.0f, 0.0f, 0.8f, 1.0f);

			Context->SetVertexBufferPointer(Verts);
			Context->ClearIndexBuffer();
			Context->SetVertexFormatPosition(3);

			Context->DrawPrimitives(GL_LINES, 0, 2);
			Context->DrawPrimitives(GL_TRIANGLE_STRIP, 2, 18);
			Context->DrawPrimitives(GL_TRIANGLE_STRIP, 20, 18);
		}
	}

	Context->EnableDepthTest(true);
}

void lcViewManipulator::DrawRotate(lcTrackButton TrackButton, lcTrackTool TrackTool)
{
	const lcCamera* Camera = mView->GetCamera();
	lcContext* Context = mView->mContext;

	const float OverlayScale = mView->GetOverlayScale();
	const float OverlayRotateRadius = 2.0f;

	Context->SetMaterial(lcMaterialType::UnlitColor);
	Context->SetViewMatrix(Camera->mWorldView);
	Context->SetProjectionMatrix(mView->GetProjectionMatrix());
	Context->SetLineWidth(1.0f);

	Context->EnableDepthTest(false);

	int j;

	lcVector3 OverlayCenter;
	lcMatrix33 RelativeRotation;
	lcModel* ActiveModel = mView->GetActiveModel();
	ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation);
	lcVector3 MouseToolDistance = ActiveModel->SnapRotation(ActiveModel->GetMouseToolDistance());
	bool HasAngle = false;

	lcMatrix44 WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);

	if (ActiveModel != mView->GetModel())
		WorldMatrix = lcMul(WorldMatrix, mView->GetActiveSubmodelTransform());

	// Draw a disc showing the rotation amount.
	if (MouseToolDistance.LengthSquared() != 0.0f && (TrackButton != lcTrackButton::None))
	{
		lcVector4 Rotation;
		float Angle, Step;

		HasAngle = true;

		switch (TrackTool)
		{
			case lcTrackTool::RotateX:
				Context->SetColor(0.8f, 0.0f, 0.0f, 0.3f);
				Angle = MouseToolDistance[0];
				Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
				break;
			case lcTrackTool::RotateY:
				Context->SetColor(0.0f, 0.8f, 0.0f, 0.3f);
				Angle = MouseToolDistance[1];
				Rotation = lcVector4(90.0f, 0.0f, 0.0f, 1.0f);
				break;
			case lcTrackTool::RotateZ:
				Context->SetColor(0.0f, 0.0f, 0.8f, 0.3f);
				Angle = MouseToolDistance[2];
				Rotation = lcVector4(90.0f, 0.0f, -1.0f, 0.0f);
				break;
			default:
				Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
				Angle = 0.0f;
				break;
		};

		if (Angle > 0.0f)
		{
			Step = 360.0f / 32;
		}
		else
		{
			Angle = -Angle;
			Step = -360.0f / 32;
		}

		if (fabsf(Angle) >= fabsf(Step))
		{
			lcMatrix44 RotatedWorldMatrix = lcMul(lcMatrix44FromAxisAngle(lcVector3(Rotation[1], Rotation[2], Rotation[3]), Rotation[0] * LC_DTOR), WorldMatrix);

			Context->SetWorldMatrix(RotatedWorldMatrix);

			Context->EnableColorBlend(true);

			lcVector3 Verts[33];
			Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);
			int NumVerts = 1;

			Context->SetVertexBufferPointer(Verts);
			Context->SetVertexFormatPosition(3);

			float StartAngle;
			int i = 0;

			if (Step < 0)
				StartAngle = -Angle;
			else
				StartAngle = Angle;

			do
			{
				float x = cosf((Step * i - StartAngle) * LC_DTOR) * OverlayRotateRadius * OverlayScale;
				float y = sinf((Step * i - StartAngle) * LC_DTOR) * OverlayRotateRadius * OverlayScale;

				Verts[NumVerts++] = lcVector3(0.0f, x, y);

				if (NumVerts == 33)
				{
					Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, NumVerts);
					Verts[1] = Verts[32];
					NumVerts = 2;
				}

				i++;
				if (Step > 0)
					Angle -= Step;
				else
					Angle += Step;

			} while (Angle >= 0.0f);

			if (NumVerts > 2)
				Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, NumVerts);

			Context->EnableColorBlend(false);
		}
	}

	// Draw the circles.
	if (gMainWindow->GetTool() == lcTool::Rotate && !HasAngle && TrackButton == lcTrackButton::None)
	{
		lcMatrix44 Mat = lcMatrix44AffineInverse(Camera->mWorldView);
		Mat.SetTranslation(WorldMatrix.GetTranslation());

		lcVector3 Verts[32];

		for (j = 0; j < 32; j++)
		{
			lcVector3 Pt;

			Pt[0] = cosf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
			Pt[1] = sinf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
			Pt[2] = 0.0f;

			Verts[j] = lcMul31(Pt, Mat);
		}

		Context->SetColor(0.1f, 0.1f, 0.1f, 1.0f);
		Context->SetWorldMatrix(lcMatrix44Identity());

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormatPosition(3);

		Context->DrawPrimitives(GL_LINE_LOOP, 0, 32);
	}

	lcVector3 ViewDir = Camera->mTargetPosition - Camera->mPosition;
	ViewDir.Normalize();

	// Transform ViewDir to local space.
	ViewDir = lcMul(ViewDir, lcMatrix33AffineInverse(lcMatrix33(WorldMatrix)));

	Context->SetWorldMatrix(WorldMatrix);

	// Draw each axis circle.
	for (int i = 0; i < 3; i++)
	{
		if (static_cast<int>(TrackTool) == static_cast<int>(lcTrackTool::RotateX) + i)
		{
			Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
		}
		else
		{
			if (gMainWindow->GetTool() != lcTool::Rotate || HasAngle || TrackButton != lcTrackButton::None)
				continue;

			switch (i)
			{
				case 0:
					Context->SetColor(0.8f, 0.0f, 0.0f, 1.0f);
					break;
				case 1:
					Context->SetColor(0.0f, 0.8f, 0.0f, 1.0f);
					break;
				case 2:
					Context->SetColor(0.0f, 0.0f, 0.8f, 1.0f);
					break;
			}
		}

		lcVector3 Verts[64];
		int NumVerts = 0;

		for (j = 0; j < 32; j++)
		{
			lcVector3 v1, v2;

			switch (i)
			{
				case 0:
					v1 = lcVector3(0.0f, cosf(LC_2PI * j / 32), sinf(LC_2PI * j / 32));
					v2 = lcVector3(0.0f, cosf(LC_2PI * (j + 1) / 32), sinf(LC_2PI * (j + 1) / 32));
					break;

				case 1:
					v1 = lcVector3(cosf(LC_2PI * j / 32), 0.0f, sinf(LC_2PI * j / 32));
					v2 = lcVector3(cosf(LC_2PI * (j + 1) / 32), 0.0f, sinf(LC_2PI * (j + 1) / 32));
					break;

				case 2:
					v1 = lcVector3(cosf(LC_2PI * j / 32), sinf(LC_2PI * j / 32), 0.0f);
					v2 = lcVector3(cosf(LC_2PI * (j + 1) / 32), sinf(LC_2PI * (j + 1) / 32), 0.0f);
					break;
			}

			if (gMainWindow->GetTool() != lcTool::Rotate || HasAngle || TrackButton != lcTrackButton::None || lcDot(ViewDir, v1 + v2) <= 0.0f)
			{
				Verts[NumVerts++] = v1 * (OverlayRotateRadius * OverlayScale);
				Verts[NumVerts++] = v2 * (OverlayRotateRadius * OverlayScale);
			}
		}

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormatPosition(3);

		Context->DrawPrimitives(GL_LINES, 0, NumVerts);
	}

	// Draw tangent vector.
	if (TrackButton != lcTrackButton::None && ((TrackTool == lcTrackTool::RotateX) || (TrackTool == lcTrackTool::RotateY) || (TrackTool == lcTrackTool::RotateZ)))
	{
		const float OverlayRotateArrowSize = 1.5f;
		const float OverlayRotateArrowCapSize = 0.25f;

		lcVector4 Rotation;
		float Angle;

		switch (TrackTool)
		{
			case lcTrackTool::RotateX:
				Angle = MouseToolDistance[0];
				Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
				break;
			case lcTrackTool::RotateY:
				Angle = MouseToolDistance[1];
				Rotation = lcVector4(90.0f, 0.0f, 0.0f, 1.0f);
				break;
			case lcTrackTool::RotateZ:
				Angle = MouseToolDistance[2];
				Rotation = lcVector4(90.0f, 0.0f, -1.0f, 0.0f);
				break;
			default:
				Angle = 0.0f;
				Rotation = lcVector4(0.0f, 0.0f, 1.0f, 0.0f);
				break;
		};

		lcMatrix44 RotatedWorldMatrix = lcMul(lcMatrix44FromAxisAngle(lcVector3(Rotation[1], Rotation[2], Rotation[3]), Rotation[0] * LC_DTOR), WorldMatrix);
		Context->SetWorldMatrix(RotatedWorldMatrix);

		Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);

		if (HasAngle)
		{
			float StartY = OverlayScale * OverlayRotateRadius;
			float EndZ = (Angle > 0.0f) ? OverlayScale * OverlayRotateArrowSize : -OverlayScale * OverlayRotateArrowSize;
			float TipZ = (Angle > 0.0f) ? -OverlayScale * OverlayRotateArrowCapSize : OverlayScale * OverlayRotateArrowCapSize;

			lcVector3 Verts[6];

			Verts[0] = lcVector3(0.0f, StartY, 0.0f);
			Verts[1] = lcVector3(0.0f, StartY, EndZ);

			Verts[2] = lcVector3(0.0f, StartY, EndZ);
			Verts[3] = lcVector3(0.0f, StartY + OverlayScale * OverlayRotateArrowCapSize, EndZ + TipZ);

			Verts[4] = lcVector3(0.0f, StartY, EndZ);
			Verts[5] = lcVector3(0.0f, StartY - OverlayScale * OverlayRotateArrowCapSize, EndZ + TipZ);

			Context->SetVertexBufferPointer(Verts);
			Context->SetVertexFormatPosition(3);

			Context->DrawPrimitives(GL_LINES, 0, 6);
		}

		// Draw text.
		lcVector3 ScreenPos = mView->ProjectPoint(WorldMatrix.GetTranslation());

		Context->SetMaterial(lcMaterialType::UnlitTextureModulate);
		Context->SetWorldMatrix(lcMatrix44Identity());
		Context->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
		Context->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mView->GetWidth(), 0.0f, mView->GetHeight(), -1.0f, 1.0f));
		Context->BindTexture2D(gTexFont.GetTexture());
		Context->EnableColorBlend(true);

		char buf[32];
		sprintf(buf, "[%.2f]", fabsf(Angle));

		int cx, cy;
		gTexFont.GetStringDimensions(&cx, &cy, buf);

		Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
		gTexFont.PrintText(Context, ScreenPos[0] - (cx / 2), ScreenPos[1] + (cy / 2), 0.0f, buf);

		Context->EnableColorBlend(false);
	}

	Context->EnableDepthTest(true);
}

bool lcViewManipulator::IsTrackToolAllowed(lcTrackTool TrackTool, quint32 AllowedTransforms)
{
	switch (TrackTool)
	{
		case lcTrackTool::None:
		case lcTrackTool::Insert:
		case lcTrackTool::PointLight:
		case lcTrackTool::SpotLight:
		case lcTrackTool::Camera:
		case lcTrackTool::Select:
			return true;

		case lcTrackTool::MoveX:
			return AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_X;

		case lcTrackTool::MoveY:
			return AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Y;

		case lcTrackTool::MoveZ:
			return AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Z;

		case lcTrackTool::MoveXY:
			return (AllowedTransforms & (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y)) == (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y);

		case lcTrackTool::MoveXZ:
			return (AllowedTransforms & (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Z)) == (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Z);

		case lcTrackTool::MoveYZ:
			return (AllowedTransforms & (LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z)) == (LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z);

		case lcTrackTool::MoveXYZ:
			return (AllowedTransforms & (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z)) == (LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z);

		case lcTrackTool::RotateX:
			return AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_X;

		case lcTrackTool::RotateY:
			return AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_Y;

		case lcTrackTool::RotateZ:
			return AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_Z;

		case lcTrackTool::RotateXY:
			return (AllowedTransforms & (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y)) == (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y);

		case lcTrackTool::RotateXYZ:
			return (AllowedTransforms & (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z)) == (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z);

		case lcTrackTool::ScalePlus:
		case lcTrackTool::ScaleMinus:
			return AllowedTransforms & (LC_OBJECT_TRANSFORM_SCALE_X | LC_OBJECT_TRANSFORM_SCALE_Y | LC_OBJECT_TRANSFORM_SCALE_Z);

		case lcTrackTool::Eraser:
		case lcTrackTool::Paint:
		case lcTrackTool::ColorPicker:
		case lcTrackTool::Zoom:
		case lcTrackTool::Pan:
		case lcTrackTool::OrbitX:
		case lcTrackTool::OrbitY:
		case lcTrackTool::OrbitXY:
		case lcTrackTool::Roll:
		case lcTrackTool::ZoomRegion:
			return true;

		case lcTrackTool::Count:
			return false;
	}

	return false;
}

lcTrackTool lcViewManipulator::UpdateSelectMove()
{
	lcModel* ActiveModel = mView->GetActiveModel();
	const float OverlayScale = mView->GetOverlayScale();
	const float OverlayMovePlaneSize = 0.5f * OverlayScale;
	const float OverlayMoveArrowSize = 1.5f * OverlayScale;
	const float OverlayMoveArrowCapRadius = 0.1f * OverlayScale;
	const float OverlayRotateArrowStart = 1.0f * OverlayScale;
	const float OverlayRotateArrowEnd = 1.5f * OverlayScale;
	const float OverlayScaleRadius = 0.125f;

	lcTool CurrentTool = gMainWindow->GetTool();
	lcTrackTool NewTrackTool = (CurrentTool == lcTool::Move) ? lcTrackTool::MoveXYZ : lcTrackTool::Select;

	lcVector3 OverlayCenter;
	lcMatrix33 RelativeRotation;

	if (!ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation))
		return NewTrackTool;

	lcMatrix44 WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);

	if (ActiveModel != mView->GetModel())
		WorldMatrix = lcMul(WorldMatrix, mView->GetActiveSubmodelTransform());
	OverlayCenter = WorldMatrix.GetTranslation();

	lcVector3 PlaneNormals[3] =
	{
		lcVector3(1.0f, 0.0f, 0.0f),
		lcVector3(0.0f, 1.0f, 0.0f),
		lcVector3(0.0f, 0.0f, 1.0f),
	};

	for (int i = 0; i < 3; i++)
		PlaneNormals[i] = lcMul30(PlaneNormals[i], WorldMatrix);

	const int x = mView->GetMouseX();
	const int y = mView->GetMouseY();
	lcVector3 StartEnd[2] = { lcVector3((float)x, (float)y, 0.0f), lcVector3((float)x, (float)y, 1.0f) };
	mView->UnprojectPoints(StartEnd, 2);
	const lcVector3& Start = StartEnd[0];
	const lcVector3& End = StartEnd[1];
	float ClosestIntersectionDistance = FLT_MAX;

	lcObject* Focus = ActiveModel->GetFocusObject();
	int ControlPointIndex = -1;
	if (Focus && Focus->IsPiece())
	{
		lcPiece* Piece = (lcPiece*)Focus;
		quint32 Section = Piece->GetFocusSection();

		if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST && Section <= LC_PIECE_SECTION_CONTROL_POINT_LAST)
			ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;
	}

	quint32 AllowedTransforms = Focus ? Focus->GetAllowedTransforms() : LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z | LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z;

	for (int AxisIndex = 0; AxisIndex < 3; AxisIndex++)
	{
		lcVector4 Plane(PlaneNormals[AxisIndex], -lcDot(PlaneNormals[AxisIndex], OverlayCenter));
		lcVector3 Intersection;

		if (!lcLineSegmentPlaneIntersection(&Intersection, Start, End, Plane))
			continue;

		float IntersectionDistance = lcLengthSquared(Intersection - Start);

		if (IntersectionDistance > ClosestIntersectionDistance)
			continue;

		lcVector3 Dir(Intersection - OverlayCenter);

		float Proj1 = lcDot(Dir, PlaneNormals[(AxisIndex + 1) % 3]);
		float Proj2 = lcDot(Dir, PlaneNormals[(AxisIndex + 2) % 3]);

		if (Proj1 > 0.0f && Proj1 < OverlayMovePlaneSize && Proj2 > 0.0f && Proj2 < OverlayMovePlaneSize)
		{
			lcTrackTool PlaneModes[] = { lcTrackTool::MoveYZ, lcTrackTool::MoveXZ, lcTrackTool::MoveXY };

			if (IsTrackToolAllowed(PlaneModes[AxisIndex], AllowedTransforms))
			{
				NewTrackTool = PlaneModes[AxisIndex];
				ClosestIntersectionDistance = IntersectionDistance;
			}
		}

		if (CurrentTool == lcTool::Select && Proj1 > OverlayRotateArrowStart && Proj1 < OverlayRotateArrowEnd && Proj2 > OverlayRotateArrowStart && Proj2 < OverlayRotateArrowEnd && ActiveModel->AnyPiecesSelected())
		{
			lcTrackTool PlaneModes[] = { lcTrackTool::RotateX, lcTrackTool::RotateY, lcTrackTool::RotateZ };

			if (IsTrackToolAllowed(PlaneModes[AxisIndex], AllowedTransforms))
			{
				NewTrackTool = PlaneModes[AxisIndex];
				ClosestIntersectionDistance = IntersectionDistance;
			}
		}

		if (fabs(Proj1) < OverlayMoveArrowCapRadius && Proj2 > 0.0f && Proj2 < OverlayMoveArrowSize)
		{
			lcTrackTool DirModes[] = { lcTrackTool::MoveZ, lcTrackTool::MoveX, lcTrackTool::MoveY };

			if (IsTrackToolAllowed(DirModes[AxisIndex], AllowedTransforms))
			{
				NewTrackTool = DirModes[AxisIndex];
				ClosestIntersectionDistance = IntersectionDistance;
			}
		}

		if (fabs(Proj2) < OverlayMoveArrowCapRadius && Proj1 > 0.0f && Proj1 < OverlayMoveArrowSize)
		{
			lcTrackTool DirModes[] = { lcTrackTool::MoveY, lcTrackTool::MoveZ, lcTrackTool::MoveX };

			if (IsTrackToolAllowed(DirModes[AxisIndex], AllowedTransforms))
			{
				NewTrackTool = DirModes[AxisIndex];
				ClosestIntersectionDistance = IntersectionDistance;
			}
		}

		lcPiece* Piece = (lcPiece*)Focus;

		if (ControlPointIndex != -1 && Piece->mPieceInfo->GetSynthInfo() && Piece->mPieceInfo->GetSynthInfo()->IsCurve())
		{
			float Strength = Piece->GetControlPoints()[ControlPointIndex].Scale;
			const float ScaleStart = (2.0f - OverlayScaleRadius) * OverlayScale + Strength;
			const float ScaleEnd = (2.0f + OverlayScaleRadius) * OverlayScale + Strength;

			if (AxisIndex == 1 && fabs(Proj1) < OverlayScaleRadius * OverlayScale)
			{
				if (Proj2 > ScaleStart && Proj2 < ScaleEnd)
				{
					if (IsTrackToolAllowed(lcTrackTool::ScalePlus, AllowedTransforms))
					{
						NewTrackTool = lcTrackTool::ScalePlus;
						ClosestIntersectionDistance = IntersectionDistance;
					}
				}
				else if (Proj2 < -ScaleStart && Proj2 > -ScaleEnd)
				{
					if (IsTrackToolAllowed(lcTrackTool::ScaleMinus, AllowedTransforms))
					{
						NewTrackTool = lcTrackTool::ScaleMinus;
						ClosestIntersectionDistance = IntersectionDistance;
					}
				}
			}
			else if (AxisIndex == 2 && fabs(Proj2) < OverlayScaleRadius * OverlayScale)
			{
				if (Proj1 > ScaleStart && Proj1 < ScaleEnd)
				{
					if (IsTrackToolAllowed(lcTrackTool::ScalePlus, AllowedTransforms))
					{
						NewTrackTool = lcTrackTool::ScalePlus;
						ClosestIntersectionDistance = IntersectionDistance;
					}
				}
				else if (Proj1 < -ScaleStart && Proj1 > -ScaleEnd)
				{
					if (IsTrackToolAllowed(lcTrackTool::ScaleMinus, AllowedTransforms))
					{
						NewTrackTool = lcTrackTool::ScaleMinus;
						ClosestIntersectionDistance = IntersectionDistance;
					}
				}
			}
		}
	}

	return NewTrackTool;
}

lcTrackTool lcViewManipulator::UpdateRotate()
{
	const lcModel* ActiveModel = mView->GetActiveModel();
	const float OverlayScale = mView->GetOverlayScale();
	const float OverlayRotateRadius = 2.0f;

	lcVector3 OverlayCenter;
	lcMatrix33 RelativeRotation;

	if (!ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation))
		return lcTrackTool::RotateXYZ;

	lcMatrix44 WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);

	if (ActiveModel != mView->GetModel())
		WorldMatrix = lcMul(WorldMatrix, mView->GetActiveSubmodelTransform());
	OverlayCenter = WorldMatrix.GetTranslation();

	const int x = mView->GetMouseX();
	const int y = mView->GetMouseY();
	lcVector3 StartEnd[2] = { lcVector3((float)x, (float)y, 0.0f), lcVector3((float)x, (float)y, 1.0f) };
	mView->UnprojectPoints(StartEnd, 2);

	lcVector3 Intersection;

	if (lcSphereRayIntersection(OverlayCenter, OverlayRotateRadius * OverlayScale, StartEnd[0], StartEnd[1], Intersection))
	{
		const lcVector3 LocalIntersection = lcMul(Intersection - OverlayCenter, lcMatrix33AffineInverse(RelativeRotation));
		const float Epsilon = 0.25f * OverlayScale;
		const float dx = fabsf(LocalIntersection[0]);
		const float dy = fabsf(LocalIntersection[1]);
		const float dz = fabsf(LocalIntersection[2]);

		if (dx < dy)
		{
			if (dx < dz)
			{
				if (dx < Epsilon)
					return lcTrackTool::RotateX;
			}
			else
			{
				if (dz < Epsilon)
					return lcTrackTool::RotateZ;
			}
		}
		else
		{
			if (dy < dz)
			{
				if (dy < Epsilon)
					return lcTrackTool::RotateY;
			}
			else
			{
				if (dz < Epsilon)
					return lcTrackTool::RotateZ;
			}
		}
	}

	return lcTrackTool::RotateXYZ;
}

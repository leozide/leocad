#include "lc_global.h"
#include "lc_viewmanipulator.h"
#include "lc_view.h"
#include "lc_model.h"
#include "object.h"
#include "piece.h"
#include "camera.h"
#include "pieceinf.h"
#include "lc_synth.h"
#include "lc_traintrack.h"
#include "lc_mainwindow.h"
#include "texfont.h"

lcVertexBuffer lcViewManipulator::mRotateMoveVertexBuffer;
lcIndexBuffer lcViewManipulator::mRotateMoveIndexBuffer;
lcVector3 lcViewManipulator::mRotateMoveVertices[51 + 138 + 10 + 74 + 28 + 24];
const GLushort lcViewManipulator::mRotateMoveIndices[mIndexCount] =
{
	// Move X
	0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7, 0, 7, 8, 0, 8, 1,
	9, 10, 14, 14, 13, 9, 11, 12, 15, 15, 16, 12,
	// Move Y
	17, 18, 19, 17, 19, 20, 17, 20, 21, 17, 21, 22, 17, 22, 23, 17, 23, 24, 17, 24, 25, 17, 25, 18,
	26, 27, 31, 31, 30, 26, 28, 29, 32, 32, 33, 29,
	// Move Z
	34, 35, 36, 34, 36, 37, 34, 37, 38, 34, 38, 39, 34, 39, 40, 34, 40, 41, 34, 41, 42, 34, 42, 35,
	43, 44, 48, 48, 47, 43, 45, 46, 49, 49, 50, 46,
	// Rotate X
	51, 52, 53, 51, 53, 54, 51, 54, 55, 51, 55, 56, 51, 56, 57, 51, 57, 58, 51, 58, 59, 51, 59, 52,
	60, 61, 62, 60, 62, 63, 60, 63, 64, 60, 64, 65, 60, 65, 66, 60, 66, 67, 60, 67, 68, 60, 68, 61,
	69, 70, 71, 71, 72, 70, 71, 72, 73, 73, 74, 72, 73, 74, 75, 75, 76, 74, 75, 76, 77, 77, 78, 76, 77, 78, 79, 79, 80, 78, 79, 80, 81, 81, 82, 80,
	83, 84, 85, 85, 86, 84, 85, 86, 87, 87, 88, 86, 87, 88, 89, 89, 90, 88, 89, 90, 91, 91, 92, 90, 91, 92, 93, 93, 94, 92, 93, 94, 95, 95, 96, 94,
	// Rotate Y
	97, 98, 99, 97, 99, 100, 97, 100, 101, 97, 101, 102, 97, 102, 103, 97, 103, 104, 97, 104, 105, 97, 105, 98,
	106, 107, 108, 106, 108, 109, 106, 109, 110, 106, 110, 111, 106, 111, 112, 106, 112, 113, 106, 113, 114, 106, 114, 107,
	115, 116, 117, 117, 118, 116, 117, 118, 119, 119, 120, 118, 119, 120, 121, 121, 122, 120, 121, 122, 123, 123, 124, 122, 123, 124, 125, 125, 126, 124, 125, 126, 127, 127, 128, 126,
	129, 130, 131, 131, 132, 130, 131, 132, 133, 133, 134, 132, 133, 134, 135, 135, 136, 134, 135, 136, 137, 137, 138, 136, 137, 138, 139, 139, 140, 138, 139, 140, 141, 141, 142, 140,
	// Rotate Z
	143, 144, 145, 143, 145, 146, 143, 146, 147, 143, 147, 148, 143, 148, 149, 143, 149, 150, 143, 150, 151, 143, 151, 144,
	152, 153, 154, 152, 154, 155, 152, 155, 156, 152, 156, 157, 152, 157, 158, 152, 158, 159, 152, 159, 160, 152, 160, 153,
	161, 162, 163, 163, 164, 162, 163, 164, 165, 165, 166, 164, 165, 166, 167, 167, 168, 166, 167, 168, 169, 169, 170, 168, 169, 170, 171, 171, 172, 170, 171, 172, 173, 173, 174, 172,
	175, 176, 177, 177, 178, 176, 177, 178, 179, 179, 180, 178, 179, 180, 181, 181, 182, 180, 181, 182, 183, 183, 184, 182, 183, 184, 185, 185, 186, 184, 185, 186, 187, 187, 188, 186,
	// Move Planes
	189, 190, 191, 192, 189, 193, 194, 195, 189, 196, 197, 198,
	// Train Track Rotate Right
	199, 200, 201, 199, 201, 202, 199, 202, 203, 199, 203, 204, 199, 204, 205, 199, 205, 206, 199, 206, 207, 199, 207, 200,
	208, 209, 210, 210, 211, 209, 210, 211, 212, 212, 213, 211, 212, 213, 214, 214, 215, 213, 214, 215, 216, 216, 217, 215, 216, 217, 218, 218, 219, 217, 218, 219, 220, 220, 221, 219,
	222, 223, 224, 224, 225, 223, 224, 225, 226, 226, 227, 225, 226, 227, 228, 228, 229, 227, 228, 229, 230, 230, 231, 229, 230, 231, 232, 232, 233, 231, 232, 233, 234, 234, 235, 233,
	// Train Track Rotate Left
	236, 237, 238, 236, 238, 239, 236, 239, 240, 236, 240, 241, 236, 241, 242, 236, 242, 243, 236, 243, 244, 236, 244, 237,
	245, 246, 247, 247, 248, 246, 247, 248, 249, 249, 250, 248, 249, 250, 251, 251, 252, 250, 251, 252, 253, 253, 254, 252, 253, 254, 255, 255, 256, 254, 255, 256, 257, 257, 258, 256,
	259, 260, 261, 261, 262, 260, 261, 262, 263, 263, 264, 262, 263, 264, 265, 265, 266, 264, 265, 266, 267, 267, 268, 266, 267, 268, 269, 269, 270, 268, 269, 270, 271, 271, 272, 270,
	// Train Track Insert
	273, 274, 275, 275, 276, 274, 275, 276, 277, 277, 278, 276, 277, 278, 279, 279, 280, 278, 279, 280, 281, 281, 282, 280, 281, 282, 283, 283, 284, 282, 283, 284, 285, 285, 286, 284,
	287, 288, 289, 289, 290, 288, 289, 290, 291, 291, 292, 290, 291, 292, 293, 293, 294, 292, 293, 294, 295, 295, 296, 294, 295, 296, 297, 297, 298, 296, 297, 298, 299, 299, 300, 298,
	// Train Track Connection
	301, 302, 303, 301, 303, 304, 308, 307, 306, 308, 306, 305, 306, 302, 301, 305, 306, 301,
	308, 304, 303, 307, 308, 303, 301, 304, 308, 301, 308, 305, 307, 303, 302, 306, 307, 302
};

lcViewManipulator::lcViewManipulator(lcView* View)
	: mView(View)
{
}

void lcViewManipulator::CreateResources(lcContext* Context)
{
	float* CurVert = mRotateMoveVertices[0].GetFloats();

	const float OverlayMovePlaneSize = 0.5f;
	const float OverlayMoveArrowSize = 1.5f;
	const float OverlayMoveArrowCapSize = 0.9f;
	const float OverlayArrowCapRadius = 0.1f;
	const float OverlayMoveArrowBodySize = 1.2f;
	const float OverlayArrowBodyRadius = 0.05f;
	const float OverlayRotateArrowStart = 1.0f;
	const float OverlayRotateArrowEnd = 1.5f;
	const float OverlayRotateArrowCenter = 1.2f;

	*CurVert++ = OverlayMoveArrowSize; *CurVert++ = 0.0f; *CurVert++ = 0.0f;

	for (int EdgeIndex = 0; EdgeIndex < 8; EdgeIndex++)
	{
		*CurVert++ = OverlayMoveArrowCapSize;
		*CurVert++ = cosf(LC_2PI * EdgeIndex / 8) * OverlayArrowCapRadius;
		*CurVert++ = sinf(LC_2PI * EdgeIndex / 8) * OverlayArrowCapRadius;
	}

	*CurVert++ = 0.0f; *CurVert++ = -OverlayArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = OverlayArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = -OverlayArrowBodyRadius;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = OverlayArrowBodyRadius;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = -OverlayArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = OverlayArrowBodyRadius; *CurVert++ = 0.0f;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = 0.0f; *CurVert++ = -OverlayArrowBodyRadius;
	*CurVert++ = OverlayMoveArrowBodySize; *CurVert++ = 0.0f; *CurVert++ = OverlayArrowBodyRadius;

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

	// Rotate X
	*CurVert++ = 0.0f; *CurVert++ = OverlayRotateArrowEnd - OverlayArrowCapRadius; *CurVert++ = OverlayRotateArrowStart;

	for (int EdgeIndex = 0; EdgeIndex < 8; EdgeIndex++)
	{
		*CurVert++ = cosf(LC_2PI * EdgeIndex / 8) * OverlayArrowCapRadius;
		*CurVert++ = sinf(LC_2PI * EdgeIndex / 8) * OverlayArrowCapRadius + OverlayRotateArrowEnd - OverlayArrowCapRadius;
		*CurVert++ = OverlayRotateArrowCenter;
	}

	*CurVert++ = 0.0f; *CurVert++ = OverlayRotateArrowStart; *CurVert++ = OverlayRotateArrowEnd - OverlayArrowCapRadius;

	for (int EdgeIndex = 0; EdgeIndex < 8; EdgeIndex++)
	{
		*CurVert++ = cosf(LC_2PI * EdgeIndex / 8) * OverlayArrowCapRadius;
		*CurVert++ = OverlayRotateArrowCenter;
		*CurVert++ = sinf(LC_2PI * EdgeIndex / 8) * OverlayArrowCapRadius + OverlayRotateArrowEnd - OverlayArrowCapRadius;
	}

	for (int EdgeIndex = 0; EdgeIndex < 7; EdgeIndex++)
	{
		const float Radius1 = OverlayRotateArrowEnd - OverlayArrowCapRadius - OverlayRotateArrowCenter - OverlayArrowBodyRadius;
		const float Radius2 = OverlayRotateArrowEnd - OverlayArrowCapRadius - OverlayRotateArrowCenter + OverlayArrowBodyRadius;
		float x = cosf(LC_2PI / 4 * EdgeIndex / 6);
		float y = sinf(LC_2PI / 4 * EdgeIndex / 6);

		*CurVert++ = 0.0f;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius1;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius1;
		*CurVert++ = 0.0f;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius2;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius2;
	}

	for (int EdgeIndex = 0; EdgeIndex < 7; EdgeIndex++)
	{
		const float Radius = OverlayRotateArrowEnd - OverlayArrowCapRadius - OverlayRotateArrowCenter;
		float x = cosf(LC_2PI / 4 * EdgeIndex / 6);
		float y = sinf(LC_2PI / 4 * EdgeIndex / 6);

		*CurVert++ = -OverlayArrowBodyRadius;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius;
		*CurVert++ = OverlayArrowBodyRadius;
		*CurVert++ = OverlayRotateArrowCenter + x * Radius;
		*CurVert++ = OverlayRotateArrowCenter + y * Radius;
	}

	// Rotate Y
	for (int VertIdx = 0; VertIdx < 46; VertIdx++)
	{
		*CurVert = *(CurVert - 137); CurVert++;
		*CurVert = *(CurVert - 139); CurVert++;
		*CurVert = *(CurVert - 138); CurVert++;
	}

	// Rotate Z
	for (int VertIdx = 0; VertIdx < 46; VertIdx++)
	{
		*CurVert = *(CurVert - 274); CurVert++;
		*CurVert = *(CurVert - 276); CurVert++;
		*CurVert = *(CurVert - 278); CurVert++;
	}

	// Move Planes
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

	const float OverlayTrainTrackStart = 0.0f;
	const float OverlayTrainTrackEnd = 0.5f;
	const float OverlayTrainTrackCenter = 0.2f;
	const float OverlayTrainTrackDistance = 0.5f;

	// Train Track Rotate Right
	*CurVert++ = OverlayTrainTrackStart; *CurVert++ = OverlayTrainTrackEnd + OverlayTrainTrackDistance - OverlayArrowCapRadius; *CurVert++ = 0.0f;

	for (int EdgeIndex = 0; EdgeIndex < 8; EdgeIndex++)
	{
		*CurVert++ = OverlayTrainTrackCenter;
		*CurVert++ = sinf(LC_2PI * EdgeIndex / 8) * OverlayArrowCapRadius + OverlayTrainTrackEnd + OverlayTrainTrackDistance - OverlayArrowCapRadius;
		*CurVert++ = cosf(LC_2PI * EdgeIndex / 8) * OverlayArrowCapRadius;
	}

	for (int EdgeIndex = 0; EdgeIndex < 7; EdgeIndex++)
	{
		const float Radius1 = OverlayTrainTrackEnd - OverlayArrowCapRadius - OverlayTrainTrackCenter - OverlayArrowBodyRadius;
		const float Radius2 = OverlayTrainTrackEnd - OverlayArrowCapRadius - OverlayTrainTrackCenter + OverlayArrowBodyRadius;
		float x = cosf(LC_2PI / 4 * EdgeIndex / 6);
		float y = sinf(LC_2PI / 4 * EdgeIndex / 6);

		*CurVert++ = OverlayTrainTrackCenter + y * Radius1;
		*CurVert++ = OverlayTrainTrackCenter + OverlayTrainTrackDistance + x * Radius1;
		*CurVert++ = 0.0f;
		*CurVert++ = OverlayTrainTrackCenter + y * Radius2;
		*CurVert++ = OverlayTrainTrackCenter + OverlayTrainTrackDistance + x * Radius2;
		*CurVert++ = 0.0f;
	}

	for (int EdgeIndex = 0; EdgeIndex < 7; EdgeIndex++)
	{
		const float Radius = OverlayTrainTrackEnd - OverlayArrowCapRadius - OverlayTrainTrackCenter;
		float x = cosf(LC_2PI / 4 * EdgeIndex / 6);
		float y = sinf(LC_2PI / 4 * EdgeIndex / 6);

		*CurVert++ = OverlayTrainTrackCenter + y * Radius;
		*CurVert++ = OverlayTrainTrackCenter + OverlayTrainTrackDistance + x * Radius;
		*CurVert++ = -OverlayArrowBodyRadius;
		*CurVert++ = OverlayTrainTrackCenter + y * Radius;
		*CurVert++ = OverlayTrainTrackCenter + OverlayTrainTrackDistance + x * Radius;
		*CurVert++ = OverlayArrowBodyRadius;
	}

	// Train Track Rotate Left
	for (int VertIdx = 0; VertIdx < 37; VertIdx++)
	{
		*CurVert = *(CurVert - 111); CurVert++;
		*CurVert = -*(CurVert - 111); CurVert++;
		*CurVert = *(CurVert - 111); CurVert++;
	}

	const float OverlayTrainTrackInsertDistance = OverlayTrainTrackEnd + 0.1f;
	const float OverlayTrainTrackInsertLength = 0.3f;
	const float OverlayTrainTrackInsertWidth = 0.07f;

	// Train Track Insert
	for (int EdgeIndex = 0; EdgeIndex < 7; EdgeIndex++)
	{
		const float Radius = OverlayTrainTrackInsertWidth;
		float x = cosf(LC_2PI * EdgeIndex / 6);
		float y = sinf(LC_2PI * EdgeIndex / 6);

		*CurVert++ = x * Radius + OverlayTrainTrackInsertDistance;
		*CurVert++ = OverlayTrainTrackInsertLength;
		*CurVert++ = y * Radius;

		*CurVert++ = x * Radius + OverlayTrainTrackInsertDistance;
		*CurVert++ = -OverlayTrainTrackInsertLength;
		*CurVert++ = y * Radius;
	}

	for (int EdgeIndex = 0; EdgeIndex < 7; EdgeIndex++)
	{
		const float Radius = OverlayTrainTrackInsertWidth;
		float x = cosf(LC_2PI * EdgeIndex / 6);
		float y = sinf(LC_2PI * EdgeIndex / 6);

		*CurVert++ = OverlayTrainTrackInsertLength + OverlayTrainTrackInsertDistance;
		*CurVert++ = x * Radius;
		*CurVert++ = y * Radius;

		*CurVert++ = -OverlayTrainTrackInsertLength + OverlayTrainTrackInsertDistance;
		*CurVert++ = x * Radius;
		*CurVert++ = y * Radius;
	}

	const float OverlayTrainTrackBoxSize = 0.15f;

	*CurVert++ = -OverlayTrainTrackBoxSize; *CurVert++ = -OverlayTrainTrackBoxSize; *CurVert++ = -OverlayTrainTrackBoxSize;
	*CurVert++ = -OverlayTrainTrackBoxSize; *CurVert++ =  OverlayTrainTrackBoxSize; *CurVert++ = -OverlayTrainTrackBoxSize;
	*CurVert++ =  OverlayTrainTrackBoxSize; *CurVert++ =  OverlayTrainTrackBoxSize; *CurVert++ = -OverlayTrainTrackBoxSize;
	*CurVert++ =  OverlayTrainTrackBoxSize; *CurVert++ = -OverlayTrainTrackBoxSize; *CurVert++ = -OverlayTrainTrackBoxSize;
	*CurVert++ = -OverlayTrainTrackBoxSize; *CurVert++ = -OverlayTrainTrackBoxSize; *CurVert++ =  OverlayTrainTrackBoxSize;
	*CurVert++ = -OverlayTrainTrackBoxSize; *CurVert++ =  OverlayTrainTrackBoxSize; *CurVert++ =  OverlayTrainTrackBoxSize;
	*CurVert++ =  OverlayTrainTrackBoxSize; *CurVert++ =  OverlayTrainTrackBoxSize; *CurVert++ =  OverlayTrainTrackBoxSize;
	*CurVert++ =  OverlayTrainTrackBoxSize; *CurVert++ = -OverlayTrainTrackBoxSize; *CurVert++ =  OverlayTrainTrackBoxSize;

	mRotateMoveVertexBuffer = Context->CreateVertexBuffer(sizeof(mRotateMoveVertices), mRotateMoveVertices[0].GetFloats());
	mRotateMoveIndexBuffer = Context->CreateIndexBuffer(sizeof(mRotateMoveIndices), mRotateMoveIndices);
}

void lcViewManipulator::DestroyResources(lcContext* Context)
{
	Context->DestroyVertexBuffer(mRotateMoveVertexBuffer);
	Context->DestroyIndexBuffer(mRotateMoveIndexBuffer);
}

void lcViewManipulator::DrawSelectMove(lcTrackButton TrackButton, lcTrackTool TrackTool, quint32 TrackToolSection)
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
				Context->SetColor(mColorXAxisSelected);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
			}
			else if (TrackButton == lcTrackButton::None)
			{
				Context->SetColor(mColorXAxis);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
			}
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Y)
		{
			if (((TrackTool == lcTrackTool::MoveY) || (TrackTool == lcTrackTool::MoveXY) || (TrackTool == lcTrackTool::MoveYZ)) && (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Y))
			{
				Context->SetColor(mColorYAxisSelected);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 36 * 2);
			}
			else if (TrackButton == lcTrackButton::None)
			{
				Context->SetColor(mColorYAxis);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 36 * 2);
			}
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Z)
		{
			if (((TrackTool == lcTrackTool::MoveZ) || (TrackTool == lcTrackTool::MoveXZ) || (TrackTool == lcTrackTool::MoveYZ)) && (AllowedTransforms & LC_OBJECT_TRANSFORM_MOVE_Z))
			{
				Context->SetColor(mColorZAxisSelected);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 72 * 2);
			}
			else if (TrackButton == lcTrackButton::None)
			{
				Context->SetColor(mColorZAxis);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 72 * 2);
			}
		}
	}

	if (gMainWindow->GetTool() == lcTool::Select && TrackButton == lcTrackButton::None && AnyPiecesSelected)
	{
		if (AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_X)
		{
			if (TrackTool == lcTrackTool::RotateX)
				Context->SetColor(mColorXAxisSelected);
			else
				Context->SetColor(mColorXAxis);

			Context->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, 108 * 2);
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_Y)
		{
			if (TrackTool == lcTrackTool::RotateY)
				Context->SetColor(mColorYAxisSelected);
			else
				Context->SetColor(mColorYAxis);

			Context->DrawIndexedPrimitives(GL_TRIANGLES, 120, GL_UNSIGNED_SHORT, (108 + 120) * 2);
		}

		if (AllowedTransforms & LC_OBJECT_TRANSFORM_ROTATE_Z)
		{
			if (TrackTool == lcTrackTool::RotateZ)
				Context->SetColor(mColorZAxisSelected);
			else
				Context->SetColor(mColorZAxis);

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
		else if (Piece->mPieceInfo->GetTrainTrackInfo() && TrackButton == lcTrackButton::None)
		{
			DrawTrainTrack(Piece, Context, TrackTool, TrackToolSection);
		}
	}

	Context->EnableDepthTest(true);
}

void lcViewManipulator::DrawTrainTrack(lcPiece* Piece, lcContext* Context, lcTrackTool TrackTool, quint32 TrackToolSection)
{
	const lcPreferences& Preferences = lcGetPreferences();
	const lcVector4 TrainTrackColor = lcVector4FromColor(Preferences.mControlPointColor);

	if (Piece->GetFocusSection() >= LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
	{
		quint32 ConnectionIndex = Piece->GetFocusSection() - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
		bool CanAdd = !Piece->IsTrainTrackConnected(ConnectionIndex);

		Context->SetColor(TrainTrackColor);

		if (TrackTool == lcTrackTool::RotateTrainTrackRight)
		{
			Context->DrawIndexedPrimitives(GL_TRIANGLES, 96, GL_UNSIGNED_SHORT, (108 + 360 + 12 + 96) * 2);

			if (CanAdd)
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 72, GL_UNSIGNED_SHORT, (108 + 360 + 12 + 192) * 2);

			Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			Context->DrawIndexedPrimitives(GL_TRIANGLES, 96, GL_UNSIGNED_SHORT, (108 + 360 + 12) * 2);
		}
		else if (TrackTool == lcTrackTool::RotateTrainTrackLeft)
		{
			Context->DrawIndexedPrimitives(GL_TRIANGLES, 96, GL_UNSIGNED_SHORT, (108 + 360 + 12) * 2);

			if (CanAdd)
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 72, GL_UNSIGNED_SHORT, (108 + 360 + 12 + 192) * 2);

			Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			Context->DrawIndexedPrimitives(GL_TRIANGLES, 96, GL_UNSIGNED_SHORT, (108 + 360 + 12 + 96) * 2);
		}
		else if (TrackTool == lcTrackTool::InsertTrainTrack)
		{
			Context->DrawIndexedPrimitives(GL_TRIANGLES, 192, GL_UNSIGNED_SHORT, (108 + 360 + 12) * 2);

			if (CanAdd)
			{
				Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 72, GL_UNSIGNED_SHORT, (108 + 360 + 12 + 192) * 2);
			}
		}
		else
		{
			Context->DrawIndexedPrimitives(GL_TRIANGLES, 192, GL_UNSIGNED_SHORT, (108 + 360 + 12) * 2);

			if (CanAdd)
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 72, GL_UNSIGNED_SHORT, (108 + 360 + 12 + 192) * 2);
		}
	}
	else
	{
		const lcTrainTrackInfo* TrainTrackInfo = Piece->mPieceInfo->GetTrainTrackInfo();
		const std::vector<lcTrainTrackConnection>& Connections = TrainTrackInfo->GetConnections();

		for (quint32 ConnectionIndex = 0; ConnectionIndex < Connections.size(); ConnectionIndex++)
		{
			if (Piece->IsTrainTrackConnected(ConnectionIndex))
				continue;

			lcMatrix44 WorldMatrix = lcMul(Connections[ConnectionIndex].Transform, Piece->mModelWorld);
			lcModel* ActiveModel = mView->GetActiveModel();

			if (ActiveModel != mView->GetModel())
				WorldMatrix = lcMul(WorldMatrix, mView->GetActiveSubmodelTransform());

			const float OverlayScale = mView->GetOverlayScale();
			WorldMatrix = lcMul(lcMatrix44Scale(lcVector3(OverlayScale, OverlayScale, OverlayScale)), WorldMatrix);

			Context->SetWorldMatrix(WorldMatrix);

			if (TrackToolSection == LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex && TrackTool != lcTrackTool::SelectTrainTrack)
				Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
			else
				Context->SetColor(TrainTrackColor);

			Context->DrawIndexedPrimitives(GL_TRIANGLES, 72, GL_UNSIGNED_SHORT, (108 + 360 + 12 + 192) * 2);
		}
	}

	const lcVector4 ConnectionColor = lcVector4FromColor(Preferences.mControlPointColor);
	const lcVector4 ConnectionFocusedColor = lcVector4FromColor(Preferences.mControlPointFocusedColor);

	const lcTrainTrackInfo* TrainTrackInfo = Piece->mPieceInfo->GetTrainTrackInfo();
	const std::vector<lcTrainTrackConnection>& Connections = TrainTrackInfo->GetConnections();

	for (quint32 ConnectionIndex = 0; ConnectionIndex < Connections.size(); ConnectionIndex++)
	{
		lcMatrix44 WorldMatrix = lcMul(Connections[ConnectionIndex].Transform, Piece->mModelWorld);
		lcModel* ActiveModel = mView->GetActiveModel();

		if (ActiveModel != mView->GetModel())
			WorldMatrix = lcMul(WorldMatrix, mView->GetActiveSubmodelTransform());

		const float OverlayScale = mView->GetOverlayScale();
		WorldMatrix = lcMul(lcMatrix44Scale(lcVector3(OverlayScale, OverlayScale, OverlayScale)), WorldMatrix);

		Context->SetWorldMatrix(WorldMatrix);

		if (Piece->IsFocused(LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex))
			Context->SetColor(ConnectionFocusedColor);
		else if (TrackToolSection == LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex && TrackTool == lcTrackTool::SelectTrainTrack)
			Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);
		else
			Context->SetColor(ConnectionColor);

		Context->DrawIndexedPrimitives(GL_TRIANGLES, mTrainTrackConnectionIndexCount, GL_UNSIGNED_SHORT, mTrainTrackConnectionIndexStart * 2);
	}
}

bool lcViewManipulator::GetRotationWorldMatrix(lcMatrix44& WorldMatrix) const
{
	lcVector3 OverlayCenter;
	lcMatrix33 RelativeRotation;
	lcModel* ActiveModel = mView->GetActiveModel();

	if (!ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation))
		return false;

	WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);

	if (ActiveModel != mView->GetModel())
		WorldMatrix = lcMul(WorldMatrix, mView->GetActiveSubmodelTransform());

	return true;
}

lcTrackTool lcViewManipulator::GetRotationAxis(const lcVector3& LocalIntersection, float Epsilon)
{
	const float dx = fabsf(LocalIntersection[0]);
	const float dy = fabsf(LocalIntersection[1]);
	const float dz = fabsf(LocalIntersection[2]);

	if (dx < dy)
	{
		if (dx < dz)
			return dx < Epsilon ? lcTrackTool::RotateX : lcTrackTool::None;

		return dz < Epsilon ? lcTrackTool::RotateZ : lcTrackTool::None;
	}

	if (dy < dz)
		return dy < Epsilon ? lcTrackTool::RotateY : lcTrackTool::None;

	return dz < Epsilon ? lcTrackTool::RotateZ : lcTrackTool::None;
}

std::optional<lcViewManipulator::lcRotationDiscInfo> lcViewManipulator::GetRotationDiscInfo(lcTrackTool TrackTool)
{
	switch (TrackTool)
	{
	case lcTrackTool::RotateX:
		return lcRotationDiscInfo{ 0, mOverlayRotateRadius, lcVector4(0.0f, 0.0f, 0.0f, 1.0f), mColorXAxis, mColorXAxisSelected, false };

	case lcTrackTool::RotateY:
		return lcRotationDiscInfo{ 1, mOverlayRotateRadius, lcVector4(90.0f, 0.0f, 0.0f, 1.0f), mColorYAxis, mColorYAxisSelected, false };

	case lcTrackTool::RotateZ:
		return lcRotationDiscInfo{ 2, mOverlayRotateRadius, lcVector4(90.0f, 0.0f, -1.0f, 0.0f), mColorZAxis, mColorZAxisSelected, false };

	case lcTrackTool::RotateCamera:
		return lcRotationDiscInfo{ 0, mOverlayRotateCameraRadius, lcVector4(0.0f, 0.0f, 0.0f, 1.0f), mColorCamera, mColorCameraSelected, true };

	default:
		return std::nullopt;
	}
}

lcMatrix44 lcViewManipulator::GetRotationDiscWorldMatrix(const lcRotationDiscInfo& DiscInfo, const lcMatrix44& WorldMatrix) const
{
	if (!DiscInfo.CameraFacing)
		return lcMul(lcMatrix44FromAxisAngle(lcVector3(DiscInfo.Rotation[1], DiscInfo.Rotation[2], DiscInfo.Rotation[3]), DiscInfo.Rotation[0] * LC_DTOR), WorldMatrix);

	lcMatrix44 CameraWorldMatrix = lcMatrix44AffineInverse(mView->GetCamera()->mWorldView);
	CameraWorldMatrix.SetTranslation(WorldMatrix.GetTranslation());
	return CameraWorldMatrix;
}

float lcViewManipulator::GetRotationDiscStartAngle(const lcRotationDiscInfo& DiscInfo, const lcMatrix44& DiscWorldMatrix) const
{
	if (DiscInfo.CameraFacing)
		return mView->GetCameraRotationStartAngle() * LC_RTOD;

	lcVector3 MouseDownRay[2] =
	{
		lcVector3((float)mView->GetMouseDownX(), (float)mView->GetMouseDownY(), 0.0f),
		lcVector3((float)mView->GetMouseDownX(), (float)mView->GetMouseDownY(), 1.0f)
	};
	mView->UnprojectPoints(MouseDownRay, 2);

	const lcVector3 Center = DiscWorldMatrix.GetTranslation();
	const lcVector3 Normal = lcNormalize(lcMul30(lcVector3(1.0f, 0.0f, 0.0f), DiscWorldMatrix));
	const lcVector4 Plane(Normal, -lcDot(Normal, Center));
	lcVector3 Intersection;

	if (!lcLineSegmentPlaneIntersection(&Intersection, MouseDownRay[0], MouseDownRay[1], Plane))
		return 0.0f;

	const lcVector3 LocalPoint = lcMul(Intersection - Center, lcMatrix33AffineInverse(lcMatrix33(DiscWorldMatrix)));
	return -atan2f(LocalPoint[2], LocalPoint[1]) * LC_RTOD;
}

void lcViewManipulator::DrawTrackballHover(const lcMatrix44& WorldMatrix, float OverlayScale) const
{
	lcContext* Context = mView->mContext;
	lcMatrix44 Mat = lcMatrix44AffineInverse(mView->GetCamera()->mWorldView);
	Mat.SetTranslation(WorldMatrix.GetTranslation());

	constexpr int SegmentCount = 32;
	lcVector3 Verts[SegmentCount + 2];
	Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);

	for (int SegmentIndex = 0; SegmentIndex <= SegmentCount; SegmentIndex++)
	{
		const float Angle = LC_2PI * SegmentIndex / SegmentCount;
		Verts[SegmentIndex + 1] = lcVector3(cosf(Angle) * mOverlayRotateRadius * OverlayScale, sinf(Angle) * mOverlayRotateRadius * OverlayScale, 0.0f);
	}

	Context->SetColor(mColorTrackball);
	Context->SetWorldMatrix(Mat);
	Context->EnableColorBlend(true);
	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormatPosition(3);
	Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, SegmentCount + 2);
	Context->EnableColorBlend(false);
}

void lcViewManipulator::DrawRotate(lcTrackButton TrackButton, lcTrackTool TrackTool)
{
	const lcCamera* Camera = mView->GetCamera();
	lcContext* Context = mView->mContext;

	const float OverlayScale = mView->GetOverlayScale();

	Context->SetMaterial(lcMaterialType::UnlitColor);
	Context->SetViewMatrix(Camera->mWorldView);
	Context->SetProjectionMatrix(mView->GetProjectionMatrix());
	Context->SetLineWidth(1.0f);

	Context->EnableDepthTest(false);

	lcModel* ActiveModel = mView->GetActiveModel();
	lcVector3 MouseToolDistance = ActiveModel->SnapRotation(ActiveModel->GetMouseToolDistance());
	bool HasAngle = false;
	lcMatrix44 WorldMatrix;
	if (!GetRotationWorldMatrix(WorldMatrix))
	{
		Context->EnableDepthTest(true);
		return;
	}

	// Show the trackball's active area while the pointer is inside it.
	if (TrackButton == lcTrackButton::None && TrackTool == lcTrackTool::RotateTrackBall)
		DrawTrackballHover(WorldMatrix, OverlayScale);

	// Draw a disc showing the rotation amount.
	const std::optional<lcRotationDiscInfo> RotationDisc = GetRotationDiscInfo(TrackTool);
	if (MouseToolDistance.LengthSquared() != 0.0f && TrackButton != lcTrackButton::None && RotationDisc)
	{
		const lcRotationDiscInfo& DiscInfo = *RotationDisc;
		HasAngle = true;
		const float Radius = DiscInfo.Radius;
		const lcMatrix44 RotatedWorldMatrix = GetRotationDiscWorldMatrix(DiscInfo, WorldMatrix);

		Context->SetColor(DiscInfo.FillColor[0], DiscInfo.FillColor[1], DiscInfo.FillColor[2], 0.3f);
		Context->SetWorldMatrix(RotatedWorldMatrix);

		Context->EnableColorBlend(true);

		lcVector3 Verts[33];
		Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);
		int NumVerts = 1;

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormatPosition(3);

		const float StartAngle = GetRotationDiscStartAngle(DiscInfo, RotatedWorldMatrix);
		const float StartVectorAngle = DiscInfo.CameraFacing ? StartAngle : -StartAngle;
		const float SignedRotationAngle = MouseToolDistance[DiscInfo.AxisIndex];
		int i = 0;
		const int SegmentCount = std::max(1, (int)ceilf(fabsf(SignedRotationAngle) / (360.0f / 32.0f)));

		for (i = 0; i <= SegmentCount; i++)
		{
			const float VertexAngle = StartVectorAngle + (DiscInfo.CameraFacing ? -1.0f : 1.0f) * SignedRotationAngle * i / SegmentCount;

			float x = cosf(VertexAngle * LC_DTOR) * Radius * OverlayScale;
			float y = sinf(VertexAngle * LC_DTOR) * Radius * OverlayScale;

			Verts[NumVerts++] = DiscInfo.CameraFacing ? lcVector3(x, y, 0.0f) : lcVector3(0.0f, x, y);

			if (NumVerts == 33)
			{
				Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, NumVerts);
				Verts[1] = Verts[32];
				NumVerts = 2;
			}

		}

		if (NumVerts > 2)
			Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, NumVerts);

		// Draw thin triangular lines for the mouse-down and current vectors.
		const float VectorRadius = Radius * OverlayScale;
		const float VectorWidth = OverlayScale * 0.035f;
		auto DrawVector = [&](float VectorAngle)
		{
			const float AngleRadians = VectorAngle * LC_DTOR;
			const float Cos = cosf(AngleRadians);
			const float Sin = sinf(AngleRadians);
			lcVector3 Direction = DiscInfo.CameraFacing ? lcVector3(Cos, Sin, 0.0f) : lcVector3(0.0f, Cos, Sin);
			lcVector3 Perpendicular = DiscInfo.CameraFacing ? lcVector3(-Sin, Cos, 0.0f) : lcVector3(0.0f, -Sin, Cos);
			Direction *= VectorRadius;
			Perpendicular *= VectorWidth;
			lcVector3 VectorVerts[6] =
			{
				Perpendicular, Direction + Perpendicular, Direction - Perpendicular,
				Perpendicular, Direction - Perpendicular, -Perpendicular
			};
			Context->SetVertexBufferPointer(VectorVerts);
			Context->SetVertexFormatPosition(3);
			Context->DrawPrimitives(GL_TRIANGLES, 0, 6);
		};

		Context->SetColor(DiscInfo.HighlightColor);

		const float CurrentVectorAngle = DiscInfo.CameraFacing ? StartVectorAngle - SignedRotationAngle : StartVectorAngle + SignedRotationAngle;
		DrawVector(StartVectorAngle);
		DrawVector(CurrentVectorAngle);

		Context->EnableColorBlend(false);
	}

	// Draw the camera circle.
	if (gMainWindow->GetTool() == lcTool::Rotate && (TrackButton == lcTrackButton::None || TrackTool == lcTrackTool::RotateCamera))
	{
		lcMatrix44 Mat = lcMatrix44AffineInverse(Camera->mWorldView);
		Mat.SetTranslation(WorldMatrix.GetTranslation());

		const float HalfWidth = OverlayScale * 0.035f;
		constexpr int SegmentCount = 48;
		lcVector3 Verts[(SegmentCount + 1) * 2];
		int NumVerts = 0;

		for (int SegmentIndex = 0; SegmentIndex <= SegmentCount; SegmentIndex++)
		{
			const float Sin = sinf(LC_2PI * SegmentIndex / SegmentCount);
			const float Cos = cosf(LC_2PI * SegmentIndex / SegmentCount);

			Verts[NumVerts++] = lcMul31(lcVector3(Sin * (mOverlayRotateCameraRadius * OverlayScale - HalfWidth), Cos * (mOverlayRotateCameraRadius * OverlayScale - HalfWidth), 0.0f), Mat);
			Verts[NumVerts++] = lcMul31(lcVector3(Sin * (mOverlayRotateCameraRadius * OverlayScale + HalfWidth), Cos * (mOverlayRotateCameraRadius * OverlayScale + HalfWidth), 0.0f), Mat);
		}

		if (TrackTool == lcTrackTool::RotateCamera)
			Context->SetColor(mColorCameraSelected);
		else
			Context->SetColor(mColorCamera);

		Context->SetWorldMatrix(lcMatrix44Identity());

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormatPosition(3);

		Context->DrawPrimitives(GL_TRIANGLE_STRIP, 0, NumVerts);
	}

	lcVector3 ViewDir = Camera->mTargetPosition - Camera->mPosition;
	ViewDir.Normalize();

	// Transform ViewDir to local space.
	const lcMatrix33 WorldToLocalMatrix = lcMatrix33AffineInverse(lcMatrix33(WorldMatrix));
	ViewDir = lcMul(ViewDir, WorldToLocalMatrix);
	const lcVector3 FrontVector = lcMul(lcNormalize(Camera->mTargetPosition - Camera->mPosition), WorldToLocalMatrix);

	Context->SetWorldMatrix(WorldMatrix);

	// Draw each axis circle.
	for (int PlaneIndex = 0; PlaneIndex < 3; PlaneIndex++)
	{
		if (static_cast<int>(TrackTool) == static_cast<int>(lcTrackTool::RotateX) + PlaneIndex)
		{
			switch (PlaneIndex)
			{
			case 0:
				Context->SetColor(mColorXAxisSelected);
				break;
			case 1:
				Context->SetColor(mColorYAxisSelected);
				break;
			case 2:
				Context->SetColor(mColorZAxisSelected);
				break;
			}
		}
		else
		{
			if (gMainWindow->GetTool() != lcTool::Rotate || HasAngle || TrackButton != lcTrackButton::None)
				continue;

			switch (PlaneIndex)
			{
			case 0:
				Context->SetColor(mColorXAxis);
				break;
			case 1:
				Context->SetColor(mColorYAxis);
				break;
			case 2:
				Context->SetColor(mColorZAxis);
				break;
			}
		}

		const float HalfWidth = OverlayScale * 0.035f;
		constexpr int SegmentCount = 32;
		lcVector3 Verts[SegmentCount * 6];
		int NumVerts = 0;

		for (int SegmentIndex = 0; SegmentIndex < SegmentCount; SegmentIndex++)
		{
			lcVector3 v1, v2, t1, t2;
			const float Sin1 = sinf(LC_2PI * SegmentIndex / SegmentCount);
			const float Cos1 = cosf(LC_2PI * SegmentIndex / SegmentCount);
			const float Sin2 = sinf(LC_2PI * (SegmentIndex + 1) / SegmentCount);
			const float Cos2 = cosf(LC_2PI * (SegmentIndex + 1) / SegmentCount);

			switch (PlaneIndex)
			{
			case 0:
				v1 = lcVector3(0.0f,  Cos1, Sin1);
				v2 = lcVector3(0.0f,  Cos2, Sin2);
				t1 = lcVector3(0.0f, -Sin1, Cos1);
				t2 = lcVector3(0.0f, -Sin2, Cos2);
				break;

			case 1:
				v1 = lcVector3( Cos1, 0.0f, Sin1);
				v2 = lcVector3( Cos2, 0.0f, Sin2);
				t1 = lcVector3(-Sin1, 0.0f, Cos1);
				t2 = lcVector3(-Sin2, 0.0f, Cos2);
				break;

			case 2:
				v1 = lcVector3( Cos1, Sin1, 0.0f);
				v2 = lcVector3( Cos2, Sin2, 0.0f);
				t1 = lcVector3(-Sin1, Cos1, 0.0f);
				t2 = lcVector3(-Sin2, Cos2, 0.0f);
				break;
			}

			if (gMainWindow->GetTool() != lcTool::Rotate || HasAngle || TrackButton != lcTrackButton::None || lcDot(ViewDir, v1 + v2) <= 0.0f)
			{
				lcVector3 NodeCenter1 = v1 * (mOverlayRotateRadius * OverlayScale);
				lcVector3 NodeCenter2 = v2 * (mOverlayRotateRadius * OverlayScale);

				lcVector3 ScreenPerpendicular1 = lcNormalize(lcCross(FrontVector, t1));
				lcVector3 Left1 = NodeCenter1 - (ScreenPerpendicular1 * HalfWidth);
				lcVector3 Right1 = NodeCenter1 + (ScreenPerpendicular1 * HalfWidth);

				lcVector3 ScreenPerpendicular2 = lcNormalize(lcCross(FrontVector, t2));
				lcVector3 Left2 = NodeCenter2 - (ScreenPerpendicular2 * HalfWidth);
				lcVector3 Right2 = NodeCenter2 + (ScreenPerpendicular2 * HalfWidth);

				Verts[NumVerts++] = Left1;
				Verts[NumVerts++] = Left2;
				Verts[NumVerts++] = Right1;

				Verts[NumVerts++] = Right1;
				Verts[NumVerts++] = Left2;
				Verts[NumVerts++] = Right2;
			}
		}

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormatPosition(3);

		Context->DrawPrimitives(GL_TRIANGLES, 0, NumVerts);
	}

	// Draw rotation and text.
	if (TrackButton != lcTrackButton::None && RotationDisc)
	{
		const lcRotationDiscInfo& DiscInfo = *RotationDisc;
		const float Angle = MouseToolDistance[DiscInfo.AxisIndex];
		const lcMatrix44 RotatedWorldMatrix = GetRotationDiscWorldMatrix(DiscInfo, WorldMatrix);
		Context->SetWorldMatrix(RotatedWorldMatrix);

		Context->SetColor(0.8f, 0.8f, 0.0f, 1.0f);

		// Draw text.
		const float StartAngle = GetRotationDiscStartAngle(DiscInfo, RotatedWorldMatrix);
		const float StartVectorAngle = DiscInfo.CameraFacing ? StartAngle : -StartAngle;
		const float MidAngle = StartVectorAngle + (DiscInfo.CameraFacing ? -0.5f : 0.5f) * Angle;
		const float Radius = DiscInfo.Radius * OverlayScale * 0.5f;
		const float MidAngleRadians = MidAngle * LC_DTOR;
		const lcVector3 TextPosition = DiscInfo.CameraFacing ? lcVector3(cosf(MidAngleRadians) * Radius, sinf(MidAngleRadians) * Radius, 0.0f) : lcVector3(0.0f, cosf(MidAngleRadians) * Radius, sinf(MidAngleRadians) * Radius);
		const lcVector3 ScreenPos = mView->ProjectPoint(lcMul31(TextPosition, RotatedWorldMatrix));
		const float UIScale = mView->GetUIScale();

		Context->SetMaterial(lcMaterialType::UnlitTextureModulate);
		Context->SetWorldMatrix(lcMatrix44Identity());
		Context->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
		Context->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mView->GetWidth() / UIScale, 0.0f, mView->GetHeight() / UIScale, -1.0f, 1.0f));
		Context->BindTexture2D(gTexFont.GetTexture());
		Context->EnableColorBlend(true);

		char buf[32];
		snprintf(buf, sizeof(buf), "%.2f", fabsf(Angle));

		int cx, cy;
		gTexFont.GetStringDimensions(&cx, &cy, buf);

		Context->SetColor(0.9f, 0.9f, 0.9f, 1.0f);
		gTexFont.PrintText(Context, ScreenPos[0] / UIScale - (cx / 2), ScreenPos[1] / UIScale + (cy / 2), 0.0f, buf);

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
		case lcTrackTool::DirectionalLight:
		case lcTrackTool::AreaLight:
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

		case lcTrackTool::RotateTrackBall:
			return (AllowedTransforms & (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z)) == (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z);

		case lcTrackTool::RotateCamera:
			return (AllowedTransforms & (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z)) == (LC_OBJECT_TRANSFORM_ROTATE_X | LC_OBJECT_TRANSFORM_ROTATE_Y | LC_OBJECT_TRANSFORM_ROTATE_Z);

		case lcTrackTool::RotateTrainTrackRight:
		case lcTrackTool::RotateTrainTrackLeft:
		case lcTrackTool::InsertTrainTrack:
		case lcTrackTool::SelectTrainTrack:
			return true;

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

std::pair<lcTrackTool, quint32> lcViewManipulator::UpdateSelectMove(lcTrackButton TrackButton)
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
	quint32 NewTrackSection = ~0U;

	lcVector3 OverlayCenter;
	lcMatrix33 RelativeRotation;

	if (!ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation))
		return { NewTrackTool, NewTrackSection };

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

		if (Piece->mPieceInfo->GetSynthInfo())
		{
			quint32 Section = Piece->GetFocusSection();

			if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST && Section <= LC_PIECE_SECTION_CONTROL_POINT_LAST)
				ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;
		}
	}

	quint32 AllowedTransforms = Focus ? Focus->GetAllowedTransforms() : LC_OBJECT_TRANSFORM_MOVE_XYZ | LC_OBJECT_TRANSFORM_ROTATE_XYZ;

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

		if (ControlPointIndex != -1 && Piece->mPieceInfo->GetSynthInfo()->IsCurve())
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

	if (CurrentTool == lcTool::Select && Focus && Focus->IsPiece() && TrackButton == lcTrackButton::None)
	{
		auto [TrainTrackTool, TrainTrackSection, TrainDistance] = UpdateSelectMoveTrainTrack((lcPiece*)Focus, OverlayScale, Start, End);

		if (TrainDistance < ClosestIntersectionDistance)
		{
			NewTrackTool = TrainTrackTool;
			NewTrackSection = TrainTrackSection;
		}
	}

	return { NewTrackTool, NewTrackSection };
}

std::tuple<lcTrackTool, quint32, float> lcViewManipulator::UpdateSelectMoveTrainTrack(lcPiece* Piece, float OverlayScale, const lcVector3& Start, const lcVector3& End) const
{
	lcTrackTool NewTrackTool = lcTrackTool::Select;
	quint32 NewTrackSection = ~0U;
	float ClosestIntersectionDistance = FLT_MAX;

	const lcTrainTrackInfo* TrainTrackInfo = Piece->mPieceInfo->GetTrainTrackInfo();

	if (!TrainTrackInfo)
		return { NewTrackTool, NewTrackSection, ClosestIntersectionDistance };

	const std::vector<lcTrainTrackConnection>& Connections = TrainTrackInfo->GetConnections();

	auto MinIntersectDist=[&ClosestIntersectionDistance](int FirstIndex, int IndexCount, const lcVector3& LocalStart, const lcVector3& LocalEnd)
	{
		lcVector3 Intersection;
		bool Hit = false;

		for (int Index = FirstIndex; Index < FirstIndex + IndexCount; Index += 3)
		{
			const lcVector3& v1 = mRotateMoveVertices[mRotateMoveIndices[Index]];
			const lcVector3& v2 = mRotateMoveVertices[mRotateMoveIndices[Index + 1]];
			const lcVector3& v3 = mRotateMoveVertices[mRotateMoveIndices[Index + 2]];

			if (lcLineTriangleMinIntersection(v1, v2, v3, LocalStart, LocalEnd, &ClosestIntersectionDistance, &Intersection))
				Hit = true;
		}

		return Hit;
	};

	for (quint32 ConnectionIndex = 0; ConnectionIndex < Connections.size(); ConnectionIndex++)
	{
		lcMatrix44 WorldMatrix = lcMul(Connections[ConnectionIndex].Transform, Piece->mModelWorld);
		lcModel* ActiveModel = mView->GetActiveModel();

		if (ActiveModel != mView->GetModel())
			WorldMatrix = lcMul(WorldMatrix, mView->GetActiveSubmodelTransform());

		lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(WorldMatrix);
		InverseWorldMatrix = lcMul(InverseWorldMatrix, lcMatrix44Scale(lcVector3(1.0f / OverlayScale, 1.0f / OverlayScale, 1.0f / OverlayScale)));

		lcVector3 LocalStart = lcMul31(Start, InverseWorldMatrix);
		lcVector3 LocalEnd = lcMul31(End, InverseWorldMatrix);

		if (!Piece->IsTrainTrackConnected(ConnectionIndex))
		{
			if (MinIntersectDist(mTrainTrackInsertIndexStart, mTrainTrackInsertIndexCount, LocalStart, LocalEnd))
			{
				NewTrackTool = lcTrackTool::InsertTrainTrack;
				NewTrackSection = LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex;
			}
		}

		if (Piece->GetFocusSection() >= LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST && ConnectionIndex == Piece->GetFocusSection() - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
		{
			if (MinIntersectDist(mTrainTrackRotateIndexStart, mTrainTrackRotateIndexCount, LocalStart, LocalEnd))
			{
				NewTrackTool = lcTrackTool::RotateTrainTrackRight;
				NewTrackSection = LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex;
			}

			if (MinIntersectDist(mTrainTrackRotateIndexStart + mTrainTrackRotateIndexCount, mTrainTrackRotateIndexCount, LocalStart, LocalEnd))
			{
				NewTrackTool = lcTrackTool::RotateTrainTrackLeft;
				NewTrackSection = LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex;
			}
		}

		if (MinIntersectDist(mTrainTrackConnectionIndexStart, mTrainTrackConnectionIndexCount, LocalStart, LocalEnd))
		{
			NewTrackTool = lcTrackTool::SelectTrainTrack;
			NewTrackSection = LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex;
		}
	}

	return { NewTrackTool, NewTrackSection, ClosestIntersectionDistance };
}

lcTrackTool lcViewManipulator::UpdateRotate()
{
	const float OverlayScale = mView->GetOverlayScale();
	lcMatrix44 WorldMatrix;
	if (!GetRotationWorldMatrix(WorldMatrix))
		return lcTrackTool::None;
	const lcVector3 OverlayCenter = WorldMatrix.GetTranslation();

	const int x = mView->GetMouseX();
	const int y = mView->GetMouseY();
	lcVector3 StartEnd[2] = { lcVector3((float)x, (float)y, 0.0f), lcVector3((float)x, (float)y, 1.0f) };
	mView->UnprojectPoints(StartEnd, 2);

	lcVector3 Intersection;
	const lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(WorldMatrix);
	const lcVector3 LocalStart = lcMul31(StartEnd[0], InverseWorldMatrix);
	const lcVector3 LocalEnd = lcMul31(StartEnd[1], InverseWorldMatrix);
	lcVector3 LocalIntersection;
	const bool TrackballHit = lcSphereRayIntersection(lcVector3(0.0f, 0.0f, 0.0f), mOverlayRotateRadius * OverlayScale, LocalStart, LocalEnd, LocalIntersection);
	if (TrackballHit)
	{
		const float Epsilon = 0.25f * OverlayScale;
		const lcTrackTool Axis = GetRotationAxis(LocalIntersection, Epsilon);
		if (Axis != lcTrackTool::None)
			return Axis;
	}

	// Test the camera-facing ring itself, rather than its bounding sphere, so the
	// inner gizmo does not accidentally activate this mode.
	const lcCamera* Camera = mView->GetCamera();
	const lcVector3 CameraAxis = lcNormalize(Camera->mTargetPosition - Camera->mPosition);
	const lcVector4 CameraPlane(CameraAxis, -lcDot(CameraAxis, OverlayCenter));

	if (lcLineSegmentPlaneIntersection(&Intersection, StartEnd[0], StartEnd[1], CameraPlane))
	{
		const float RingRadius = mOverlayRotateCameraRadius * OverlayScale;
		const float HalfWidth = OverlayScale * 0.075f;
		const float Distance = lcLength(Intersection - OverlayCenter);

		if (fabsf(Distance - RingRadius) <= HalfWidth)
			return lcTrackTool::RotateCamera;
	}

	return TrackballHit ? lcTrackTool::RotateTrackBall : lcTrackTool::None;
}

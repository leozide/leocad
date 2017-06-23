#include "lc_global.h"
#include "lc_scene.h"
#include "lc_context.h"
#include "pieceinf.h"
#include "lc_texture.h"
#include "lc_library.h"
#include "lc_application.h"
#include "object.h"

lcScene::lcScene()
	: mRenderMeshes(0, 1024), mOpaqueMeshes(0, 1024), mTranslucentMeshes(0, 1024), mInterfaceObjects(0, 1024)
{
}

void lcScene::Begin(const lcMatrix44& ViewMatrix)
{
	mViewMatrix = ViewMatrix;
	mRenderMeshes.RemoveAll();
	mOpaqueMeshes.RemoveAll();
	mTranslucentMeshes.RemoveAll();
	mInterfaceObjects.RemoveAll();
	mHasTexture = false;
}

void lcScene::End()
{
	auto OpaqueMeshCompare = [this](int Index1, int Index2)
	{
		return mRenderMeshes[Index1].Mesh <  mRenderMeshes[Index2].Mesh;
	};

	std::sort(&mOpaqueMeshes[0], &mOpaqueMeshes[0] + mOpaqueMeshes.GetSize(), OpaqueMeshCompare);

	auto TranslucentMeshCompare = [this](int Index1, int Index2)
	{
		return mRenderMeshes[Index1].Distance <  mRenderMeshes[Index2].Distance;
	};

	std::sort(&mTranslucentMeshes[0], &mTranslucentMeshes[0] + mTranslucentMeshes.GetSize(), TranslucentMeshCompare);
}

void lcScene::AddMesh(lcMesh* Mesh, const lcMatrix44& WorldMatrix, int ColorIndex, lcRenderMeshState State, int Flags)
{
	lcRenderMesh& RenderMesh = mRenderMeshes.Add();

	RenderMesh.WorldMatrix = WorldMatrix;
	RenderMesh.Mesh = Mesh;
	RenderMesh.ColorIndex = ColorIndex;
	RenderMesh.State = State;
	RenderMesh.Distance = fabsf(lcMul31(WorldMatrix[3], mViewMatrix).z);
	RenderMesh.LodIndex = RenderMesh.Mesh->GetLodIndex(RenderMesh.Distance);

	bool Translucent = lcIsColorTranslucent(ColorIndex);

	if ((Flags & (LC_PIECE_HAS_SOLID | LC_PIECE_HAS_LINES)) || ((Flags & LC_PIECE_HAS_DEFAULT) && !Translucent))
		mOpaqueMeshes.Add(mRenderMeshes.GetSize() - 1);

	if ((Flags & LC_PIECE_HAS_TRANSLUCENT) || ((Flags & LC_PIECE_HAS_DEFAULT) && Translucent))
		mTranslucentMeshes.Add(mRenderMeshes.GetSize() - 1);

	if (Flags & LC_PIECE_HAS_TEXTURE)
		mHasTexture = true;
}

void lcScene::DrawRenderMeshes(lcContext* Context, int PrimitiveTypes, bool EnableNormals, bool DrawTranslucent, bool DrawTextured) const
{
	const lcArray<int>& Meshes = DrawTranslucent ? mTranslucentMeshes : mOpaqueMeshes;

	for (int MeshIndex : Meshes)
	{
		const lcRenderMesh& RenderMesh = mRenderMeshes[MeshIndex];
		const lcMesh* Mesh = RenderMesh.Mesh;
		int LodIndex = RenderMesh.LodIndex;

		Context->BindMesh(Mesh);
		Context->SetWorldMatrix(RenderMesh.WorldMatrix);

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LodIndex].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LodIndex].Sections[SectionIdx];

			if ((Section->PrimitiveType & PrimitiveTypes) == 0 || (Section->Texture != nullptr) != DrawTextured)
				continue;

			int ColorIndex = Section->ColorIndex;

			if (Section->PrimitiveType & (LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES))
			{
				if (ColorIndex == gDefaultColor)
					ColorIndex = RenderMesh.ColorIndex;

				if (lcIsColorTranslucent(ColorIndex) != DrawTranslucent)
					continue;

				switch (RenderMesh.State)
				{
				case LC_RENDERMESH_NONE:
				case LC_RENDERMESH_HIGHLIGHT:
					Context->SetColorIndex(ColorIndex);
					break;

				case LC_RENDERMESH_SELECTED:
					Context->SetColorIndexTinted(ColorIndex, LC_COLOR_SELECTED);
					break;

				case LC_RENDERMESH_FOCUSED:
					Context->SetColorIndexTinted(ColorIndex, LC_COLOR_FOCUSED);
					break;
				}
			}
			else if (Section->PrimitiveType & (LC_MESH_LINES | LC_MESH_TEXTURED_LINES))
			{
				switch (RenderMesh.State)
				{
				case LC_RENDERMESH_NONE:
					if (ColorIndex == gEdgeColor)
						Context->SetEdgeColorIndex(RenderMesh.ColorIndex);
					else
						Context->SetColorIndex(ColorIndex);
					break;

				case LC_RENDERMESH_SELECTED:
					Context->SetInterfaceColor(LC_COLOR_SELECTED);
					break;

				case LC_RENDERMESH_FOCUSED:
					Context->SetInterfaceColor(LC_COLOR_FOCUSED);
					break;

				case LC_RENDERMESH_HIGHLIGHT:
					Context->SetInterfaceColor(LC_COLOR_HIGHLIGHT);
					break;
				}
			}
			else if (Section->PrimitiveType == LC_MESH_CONDITIONAL_LINES)
			{
				lcMatrix44 WorldViewProjectionMatrix = lcMul(RenderMesh.WorldMatrix, lcMul(mViewMatrix, Context->GetProjectionMatrix()));
				lcVertex* VertexBuffer = (lcVertex*)Mesh->mVertexData;
				int IndexBufferOffset = Mesh->mIndexCacheOffset != -1 ? Mesh->mIndexCacheOffset : 0;

				int VertexBufferOffset = Mesh->mVertexCacheOffset != -1 ? Mesh->mVertexCacheOffset : 0;
				Context->SetVertexFormat(VertexBufferOffset, 3, 1, 0, 0, EnableNormals);

				if (Mesh->mIndexType == GL_UNSIGNED_SHORT)
				{
					lcuint16* Indices = (lcuint16*)((char*)Mesh->mIndexData + Section->IndexOffset);

					for (int i = 0; i < Section->NumIndices; i += 4)
					{
						lcVector3 p1 = lcMul31(VertexBuffer[Indices[i + 0]].Position, WorldViewProjectionMatrix);
						lcVector3 p2 = lcMul31(VertexBuffer[Indices[i + 1]].Position, WorldViewProjectionMatrix);
						lcVector3 p3 = lcMul31(VertexBuffer[Indices[i + 2]].Position, WorldViewProjectionMatrix);
						lcVector3 p4 = lcMul31(VertexBuffer[Indices[i + 3]].Position, WorldViewProjectionMatrix);

						if (((p1.y - p2.y) * (p3.x - p1.x) + (p2.x - p1.x) * (p3.y - p1.y)) * ((p1.y - p2.y) * (p4.x - p1.x) + (p2.x - p1.x) * (p4.y - p1.y)) >= 0)
							Context->DrawIndexedPrimitives(GL_LINES, 2, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset + i * sizeof(lcuint16));
					}
				}
				else
				{
					lcuint32* Indices = (lcuint32*)((char*)Mesh->mIndexData + Section->IndexOffset);

					for (int i = 0; i < Section->NumIndices; i += 4)
					{
						lcVector3 p1 = lcMul31(VertexBuffer[Indices[i + 0]].Position, WorldViewProjectionMatrix);
						lcVector3 p2 = lcMul31(VertexBuffer[Indices[i + 1]].Position, WorldViewProjectionMatrix);
						lcVector3 p3 = lcMul31(VertexBuffer[Indices[i + 2]].Position, WorldViewProjectionMatrix);
						lcVector3 p4 = lcMul31(VertexBuffer[Indices[i + 3]].Position, WorldViewProjectionMatrix);

						if (((p1.y - p2.y) * (p3.x - p1.x) + (p2.x - p1.x) * (p3.y - p1.y)) * ((p1.y - p2.y) * (p4.x - p1.x) + (p2.x - p1.x) * (p4.y - p1.y)) >= 0)
							Context->DrawIndexedPrimitives(GL_LINES, 2, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset + i * sizeof(lcuint32));
					}
				}

				continue;
			}

			lcTexture* Texture = Section->Texture;
			int VertexBufferOffset = Mesh->mVertexCacheOffset != -1 ? Mesh->mVertexCacheOffset : 0;
			int IndexBufferOffset = Mesh->mIndexCacheOffset != -1 ? Mesh->mIndexCacheOffset : 0;

			if (!Texture)
			{
				Context->SetVertexFormat(VertexBufferOffset, 3, 1, 0, 0, EnableNormals);
			}
			else
			{
				VertexBufferOffset += Mesh->mNumVertices * sizeof(lcVertex);
				Context->SetVertexFormat(VertexBufferOffset, 3, 1, 2, 0, EnableNormals);
				Context->BindTexture(Texture->mTexture);
			}

			GLenum DrawPrimitiveType = Section->PrimitiveType & (LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES) ? GL_TRIANGLES : GL_LINES;
			Context->DrawIndexedPrimitives(DrawPrimitiveType, Section->NumIndices, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset);
		}

#ifdef QT_DEBUG
		const bool DrawNormals = false;

		if (DrawNormals)
		{
			lcVertex* VertexBuffer = (lcVertex*)Mesh->mVertexData;
			lcVector3* Vertices = (lcVector3*)malloc(Mesh->mNumVertices * 2 * sizeof(lcVector3));

			for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
			{
				Vertices[VertexIdx * 2] = VertexBuffer[VertexIdx].Position;
				Vertices[VertexIdx * 2 + 1] = VertexBuffer[VertexIdx].Position + lcUnpackNormal(VertexBuffer[VertexIdx].Normal);
			}

			Context->SetVertexBufferPointer(Vertices);
			Context->SetVertexFormatPosition(3);
			Context->DrawPrimitives(GL_LINES, 0, Mesh->mNumVertices * 2);
			free(Vertices);
		}
#endif
	}
}

void lcScene::Draw(lcContext* Context) const
{
	lcGetPiecesLibrary()->UpdateBuffers(Context); // TODO: find a better place for this update

	Context->SetViewMatrix(mViewMatrix);

	const bool DrawConditional = false;

	if (lcGetPreferences().mLightingMode == LC_LIGHTING_UNLIT)
	{
		bool DrawLines = lcGetPreferences().mDrawEdgeLines;
		Context->BindTexture(0);

		Context->SetMaterial(LC_MATERIAL_UNLIT_COLOR);

		int UntexturedPrimitives = LC_MESH_TRIANGLES;

		if (DrawLines)
		{
			UntexturedPrimitives |= LC_MESH_LINES;

			if (DrawConditional)
				UntexturedPrimitives |= LC_MESH_CONDITIONAL_LINES;
		}

		DrawRenderMeshes(Context, UntexturedPrimitives, false, false, false);

		if (!mTranslucentMeshes.IsEmpty())
		{
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);

			DrawRenderMeshes(Context, LC_MESH_TRIANGLES, false, true, false);

			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
		}

		if (mHasTexture)
		{
			Context->SetMaterial(LC_MATERIAL_UNLIT_TEXTURE_DECAL);

			if (DrawLines)
				DrawRenderMeshes(Context, LC_MESH_TEXTURED_LINES | LC_MESH_TEXTURED_TRIANGLES, false, false, false);
			else
				DrawRenderMeshes(Context, LC_MESH_TEXTURED_TRIANGLES, false, false, false);

			if (!mTranslucentMeshes.IsEmpty())
			{
				glEnable(GL_BLEND); // todo: remove GL calls
				glDepthMask(GL_FALSE);

				DrawRenderMeshes(Context, LC_MESH_TEXTURED_TRIANGLES, false, true, false);

				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);
			}

			Context->BindTexture(0);
		}
	}
	else
	{
		bool DrawLines = lcGetPreferences().mDrawEdgeLines;
		Context->BindTexture(0);

		if (DrawLines)
		{
			int LinePrimitives = LC_MESH_LINES;

			if (DrawConditional)
				LinePrimitives |= LC_MESH_CONDITIONAL_LINES;

			Context->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
			DrawRenderMeshes(Context, LinePrimitives, false, false, false);
		}

		Context->SetMaterial(LC_MATERIAL_FAKELIT_COLOR);
		DrawRenderMeshes(Context, LC_MESH_TRIANGLES, true, false, false);

		if (!mTranslucentMeshes.IsEmpty())
		{
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);

			DrawRenderMeshes(Context, LC_MESH_TRIANGLES, true, true, false);

			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
		}

		if (mHasTexture)
		{
			if (DrawLines)
			{
				Context->SetMaterial(LC_MATERIAL_UNLIT_TEXTURE_DECAL);
				DrawRenderMeshes(Context, LC_MESH_TEXTURED_LINES, false, false, true);
			}

			Context->SetMaterial(LC_MATERIAL_FAKELIT_TEXTURE_DECAL);
			DrawRenderMeshes(Context, LC_MESH_TEXTURED_TRIANGLES, true, false, true);

			if (!mTranslucentMeshes.IsEmpty())
			{
				glEnable(GL_BLEND);
				glDepthMask(GL_FALSE);

				DrawRenderMeshes(Context, LC_MESH_TEXTURED_TRIANGLES, true, true, true);

				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);
			}

			Context->BindTexture(0);
		}
	}
}

void lcScene::DrawInterfaceObjects(lcContext* Context) const
{
	for (const lcObject* Object : mInterfaceObjects)
		Object->DrawInterface(Context);
}

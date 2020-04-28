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
	mActiveSubmodelInstance = nullptr;
	mDrawInterface = false;
	mAllowWireframe = true;
	mAllowLOD = true;
	mHasFadedParts = false;
	mPreTranslucentCallback = nullptr;
}

void lcScene::Begin(const lcMatrix44& ViewMatrix)
{
	mViewMatrix = ViewMatrix;
	mActiveSubmodelInstance = nullptr;
	mPreTranslucentCallback = nullptr;
	mRenderMeshes.RemoveAll();
	mOpaqueMeshes.RemoveAll();
	mTranslucentMeshes.RemoveAll();
	mInterfaceObjects.RemoveAll();

	const lcPreferences& Preferences = lcGetPreferences();
	mHighlightColor = lcVector4FromColor(Preferences.mHighlightNewPartsColor);
	mFadeColor = lcVector4FromColor(Preferences.mFadeStepsColor);
	mHasFadedParts = false;
	mTranslucentFade = mFadeColor.w != 1.0f;
}

void lcScene::End()
{
	const auto OpaqueMeshCompare = [this](int Index1, int Index2)
	{
		const lcMesh* Mesh1 = mRenderMeshes[Index1].Mesh;
		const lcMesh* Mesh2 = mRenderMeshes[Index2].Mesh;

		const int Texture1 = Mesh1->mFlags & lcMeshFlag::HasTexture;
		const int Texture2 = Mesh2->mFlags & lcMeshFlag::HasTexture;

		if (Texture1 == Texture2)
			return Mesh1 < Mesh2;

		return Texture1 ? false : true;
	};

	std::sort(mOpaqueMeshes.begin(), mOpaqueMeshes.end(), OpaqueMeshCompare);

	auto TranslucentMeshCompare = [](const lcTranslucentMeshInstance& Mesh1, const lcTranslucentMeshInstance& Mesh2)
	{
		return Mesh1.Distance > Mesh2.Distance;
	};

	std::sort(mTranslucentMeshes.begin(), mTranslucentMeshes.end(), TranslucentMeshCompare);
}

void lcScene::AddMesh(lcMesh* Mesh, const lcMatrix44& WorldMatrix, int ColorIndex, lcRenderMeshState State)
{
	lcRenderMesh& RenderMesh = mRenderMeshes.Add();

	RenderMesh.WorldMatrix = WorldMatrix;
	RenderMesh.Mesh = Mesh;
	RenderMesh.ColorIndex = ColorIndex;
	RenderMesh.State = State;
	const float Distance = fabsf(lcMul31(WorldMatrix[3], mViewMatrix).z);
	RenderMesh.LodIndex = mAllowLOD ? RenderMesh.Mesh->GetLodIndex(Distance) : LC_MESH_LOD_HIGH;

	const bool ForceTranslucent = (mTranslucentFade && State == lcRenderMeshState::Faded);
	const bool Translucent = lcIsColorTranslucent(ColorIndex) || ForceTranslucent;
	const lcMeshFlags Flags = Mesh->mFlags;
	mHasFadedParts |= State == lcRenderMeshState::Faded;

	if ((Flags & (lcMeshFlag::HasSolid | lcMeshFlag::HasLines)) || ((Flags & lcMeshFlag::HasDefault) && !Translucent))
		mOpaqueMeshes.Add(mRenderMeshes.GetSize() - 1);

	if ((Flags & lcMeshFlag::HasTranslucent) || ((Flags & lcMeshFlag::HasDefault) && Translucent))
	{
		const lcMeshLod& Lod = Mesh->mLods[RenderMesh.LodIndex];

		for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
		{
			const lcMeshSection* Section = &Lod.Sections[SectionIdx];

			if ((Section->PrimitiveType & (LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES)) == 0)
				continue;

			int SectionColorIndex = Section->ColorIndex;

			if (SectionColorIndex == gDefaultColor)
				SectionColorIndex = RenderMesh.ColorIndex;

			if (!lcIsColorTranslucent(SectionColorIndex) && !ForceTranslucent)
				continue;

			const lcVector3 Center = (Section->BoundingBox.Min + Section->BoundingBox.Max) / 2;
			const float InstanceDistance = fabsf(lcMul31(lcMul31(Center, WorldMatrix), mViewMatrix).z);

			lcTranslucentMeshInstance& Instance = mTranslucentMeshes.Add();
			Instance.Section = Section;
			Instance.Distance = InstanceDistance;
			Instance.RenderMeshIndex = mRenderMeshes.GetSize() - 1;
		}
	}
}

void lcScene::DrawDebugNormals(lcContext* Context, const lcMesh* Mesh) const
{
	const lcVertex* const VertexBuffer = (lcVertex*)Mesh->mVertexData;
	lcVector3* const Vertices = (lcVector3*)malloc(Mesh->mNumVertices * 2 * sizeof(lcVector3));

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

void lcScene::DrawOpaqueMeshes(lcContext* Context, bool DrawLit, int PrimitiveTypes, bool DrawFaded, bool DrawNonFaded) const
{
	if (mOpaqueMeshes.IsEmpty())
		return;

	lcMaterialType FlatMaterial, TexturedMaterial;

	if (DrawLit)
	{
		FlatMaterial = lcMaterialType::FakeLitColor;
		TexturedMaterial = lcMaterialType::FakeLitTextureDecal;
	}
	else
	{
		FlatMaterial = lcMaterialType::UnlitColor;
		TexturedMaterial = lcMaterialType::UnlitTextureDecal;
	}

	Context->SetPolygonOffset(lcPolygonOffset::Opaque);

	for (const int MeshIndex : mOpaqueMeshes)
	{
		const lcRenderMesh& RenderMesh = mRenderMeshes[MeshIndex];
		const lcMesh* Mesh = RenderMesh.Mesh;
		const int LodIndex = RenderMesh.LodIndex;

		if (!DrawFaded && RenderMesh.State == lcRenderMeshState::Faded)
			continue;

		if (!DrawNonFaded && RenderMesh.State != lcRenderMeshState::Faded)
			continue;

		Context->BindMesh(Mesh);
		Context->SetWorldMatrix(RenderMesh.WorldMatrix);

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LodIndex].NumSections; SectionIdx++)
		{
			const lcMeshSection* const Section = &Mesh->mLods[LodIndex].Sections[SectionIdx];

			if ((Section->PrimitiveType & PrimitiveTypes) == 0)
				continue;

			int ColorIndex = Section->ColorIndex;

			if (Section->PrimitiveType & (LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES))
			{
				if (ColorIndex == gDefaultColor)
					ColorIndex = RenderMesh.ColorIndex;

				if (lcIsColorTranslucent(ColorIndex))
					continue;

				switch (RenderMesh.State)
				{
				case lcRenderMeshState::Default:
				case lcRenderMeshState::Highlighted:
					Context->SetColorIndex(ColorIndex);
					break;

				case lcRenderMeshState::Selected:
					Context->SetColorIndexTinted(ColorIndex, LC_COLOR_SELECTED, 0.5f);
					break;

				case lcRenderMeshState::Focused:
					Context->SetColorIndexTinted(ColorIndex, LC_COLOR_FOCUSED, 0.5f);
					break;

				case lcRenderMeshState::Faded:
					if (mTranslucentFade)
						continue;
					Context->SetColorIndexTinted(ColorIndex, mFadeColor);
					break;
				}
			}
			else if (Section->PrimitiveType & LC_MESH_LINES)
			{
				switch (RenderMesh.State)
				{
				case lcRenderMeshState::Default:
					if (ColorIndex == gEdgeColor)
						Context->SetEdgeColorIndex(RenderMesh.ColorIndex);
					else
						Context->SetColorIndex(ColorIndex);
					break;

				case lcRenderMeshState::Selected:
					Context->SetInterfaceColor(LC_COLOR_SELECTED);
					break;

				case lcRenderMeshState::Focused:
					Context->SetInterfaceColor(LC_COLOR_FOCUSED);
					break;

				case lcRenderMeshState::Highlighted:
					Context->SetColor(mHighlightColor);
					break;

				case lcRenderMeshState::Faded:
					Context->SetEdgeColorIndexTinted(ColorIndex, mFadeColor);
					break;
				}
			}
			else if (Section->PrimitiveType == LC_MESH_CONDITIONAL_LINES)
			{
				const lcMatrix44 WorldViewProjectionMatrix = lcMul(RenderMesh.WorldMatrix, lcMul(mViewMatrix, Context->GetProjectionMatrix()));
				const lcVertex* const VertexBuffer = (lcVertex*)Mesh->mVertexData;
				const int IndexBufferOffset = Mesh->mIndexCacheOffset != -1 ? Mesh->mIndexCacheOffset : 0;

				const int VertexBufferOffset = Mesh->mVertexCacheOffset != -1 ? Mesh->mVertexCacheOffset : 0;
				Context->SetVertexFormat(VertexBufferOffset, 3, 1, 0, 0, DrawLit);

				if (Mesh->mIndexType == GL_UNSIGNED_SHORT)
				{
					const quint16* const Indices = (quint16*)((char*)Mesh->mIndexData + Section->IndexOffset);

					for (int i = 0; i < Section->NumIndices; i += 4)
					{
						const lcVector3 p1 = lcMul31(VertexBuffer[Indices[i + 0]].Position, WorldViewProjectionMatrix);
						const lcVector3 p2 = lcMul31(VertexBuffer[Indices[i + 1]].Position, WorldViewProjectionMatrix);
						const lcVector3 p3 = lcMul31(VertexBuffer[Indices[i + 2]].Position, WorldViewProjectionMatrix);
						const lcVector3 p4 = lcMul31(VertexBuffer[Indices[i + 3]].Position, WorldViewProjectionMatrix);

						if (((p1.y - p2.y) * (p3.x - p1.x) + (p2.x - p1.x) * (p3.y - p1.y)) * ((p1.y - p2.y) * (p4.x - p1.x) + (p2.x - p1.x) * (p4.y - p1.y)) >= 0)
							Context->DrawIndexedPrimitives(GL_LINES, 2, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset + i * sizeof(quint16));
					}
				}
				else
				{
					const quint32* const Indices = (quint32*)((char*)Mesh->mIndexData + Section->IndexOffset);

					for (int i = 0; i < Section->NumIndices; i += 4)
					{
						const lcVector3 p1 = lcMul31(VertexBuffer[Indices[i + 0]].Position, WorldViewProjectionMatrix);
						const lcVector3 p2 = lcMul31(VertexBuffer[Indices[i + 1]].Position, WorldViewProjectionMatrix);
						const lcVector3 p3 = lcMul31(VertexBuffer[Indices[i + 2]].Position, WorldViewProjectionMatrix);
						const lcVector3 p4 = lcMul31(VertexBuffer[Indices[i + 3]].Position, WorldViewProjectionMatrix);

						if (((p1.y - p2.y) * (p3.x - p1.x) + (p2.x - p1.x) * (p3.y - p1.y)) * ((p1.y - p2.y) * (p4.x - p1.x) + (p2.x - p1.x) * (p4.y - p1.y)) >= 0)
							Context->DrawIndexedPrimitives(GL_LINES, 2, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset + i * sizeof(quint32));
					}
				}

				continue;
			}

			const lcTexture* const Texture = Section->Texture;
			int VertexBufferOffset = Mesh->mVertexCacheOffset != -1 ? Mesh->mVertexCacheOffset : 0;
			const int IndexBufferOffset = Mesh->mIndexCacheOffset != -1 ? Mesh->mIndexCacheOffset : 0;

			if (!Texture)
			{
				Context->SetMaterial(FlatMaterial);
				Context->SetVertexFormat(VertexBufferOffset, 3, 1, 0, 0, DrawLit);
			}
			else
			{
				Context->SetMaterial(TexturedMaterial);
				VertexBufferOffset += Mesh->mNumVertices * sizeof(lcVertex);
				Context->SetVertexFormat(VertexBufferOffset, 3, 1, 2, 0, DrawLit);
				Context->BindTexture2D(Texture->mTexture);
			}

			const GLenum DrawPrimitiveType = Section->PrimitiveType & (LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES) ? GL_TRIANGLES : GL_LINES;
			Context->DrawIndexedPrimitives(DrawPrimitiveType, Section->NumIndices, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset);
		}

#ifdef LC_DEBUG_NORMALS
		DrawDebugNormals(Context, Mesh);
#endif
	}

	Context->BindTexture2D(0);
	Context->SetPolygonOffset(lcPolygonOffset::None);
}

void lcScene::DrawTranslucentMeshes(lcContext* Context, bool DrawLit, bool DrawFadePrepass, bool DrawFaded, bool DrawNonFaded) const
{
	if (mTranslucentMeshes.IsEmpty())
		return;

	lcMaterialType FlatMaterial, TexturedMaterial;

	if (DrawLit)
	{
		FlatMaterial = lcMaterialType::FakeLitColor;
		TexturedMaterial = lcMaterialType::FakeLitTextureDecal;
	}
	else
	{
		FlatMaterial = lcMaterialType::UnlitColor;
		TexturedMaterial = lcMaterialType::UnlitTextureDecal;
	}

	if (!DrawFadePrepass)
	{
		glEnable(GL_BLEND);
		Context->SetDepthWrite(false);
	}
	else
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	Context->SetPolygonOffset(lcPolygonOffset::Translucent);

	for (const lcTranslucentMeshInstance& MeshInstance : mTranslucentMeshes)
	{
		const lcRenderMesh& RenderMesh = mRenderMeshes[MeshInstance.RenderMeshIndex];
		const lcMesh* Mesh = RenderMesh.Mesh;

		if (!DrawFaded && RenderMesh.State == lcRenderMeshState::Faded)
			continue;

		if (!DrawNonFaded && RenderMesh.State != lcRenderMeshState::Faded)
			continue;

		Context->BindMesh(Mesh);
		Context->SetWorldMatrix(RenderMesh.WorldMatrix);

		const lcMeshSection* Section = MeshInstance.Section;

		int ColorIndex = Section->ColorIndex;

		if (ColorIndex == gDefaultColor)
			ColorIndex = RenderMesh.ColorIndex;

		if (DrawFadePrepass && lcIsColorTranslucent(ColorIndex))
			continue;

		switch (RenderMesh.State)
		{
		case lcRenderMeshState::Default:
		case lcRenderMeshState::Highlighted:
			Context->SetColorIndex(ColorIndex);
			break;

		case lcRenderMeshState::Selected:
			Context->SetColorIndexTinted(ColorIndex, LC_COLOR_SELECTED, 0.5f);
			break;

		case lcRenderMeshState::Focused:
			Context->SetColorIndexTinted(ColorIndex, LC_COLOR_FOCUSED, 0.5f);
			break;

		case lcRenderMeshState::Faded:
			Context->SetColorIndexTinted(ColorIndex, mFadeColor);
			break;
		}

		const lcTexture* Texture = Section->Texture;
		int VertexBufferOffset = Mesh->mVertexCacheOffset != -1 ? Mesh->mVertexCacheOffset : 0;
		const int IndexBufferOffset = Mesh->mIndexCacheOffset != -1 ? Mesh->mIndexCacheOffset : 0;

		if (!Texture)
		{
			Context->SetMaterial(FlatMaterial);
			Context->SetVertexFormat(VertexBufferOffset, 3, 1, 0, 0, DrawLit);
		}
		else
		{
			Context->SetMaterial(TexturedMaterial);
			VertexBufferOffset += Mesh->mNumVertices * sizeof(lcVertex);
			Context->SetVertexFormat(VertexBufferOffset, 3, 1, 2, 0, DrawLit);
			Context->BindTexture2D(Texture->mTexture);
		}

		const GLenum DrawPrimitiveType = Section->PrimitiveType & (LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES) ? GL_TRIANGLES : GL_LINES;
		Context->DrawIndexedPrimitives(DrawPrimitiveType, Section->NumIndices, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset);

#ifdef LC_DEBUG_NORMALS
		DrawDebugNormals(Context, Mesh);
#endif
	}

	Context->BindTexture2D(0);
	Context->SetPolygonOffset(lcPolygonOffset::None);

	if (!DrawFadePrepass)
	{
		Context->SetDepthWrite(true);
		glDisable(GL_BLEND);
	}
	else
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void lcScene::Draw(lcContext* Context) const
{
	// TODO: find a better place for these updates
	lcGetPiecesLibrary()->UpdateBuffers(Context);
	lcGetPiecesLibrary()->UploadTextures(Context);

	Context->SetViewMatrix(mViewMatrix);

	constexpr bool DrawConditional = false;
	const lcPreferences& Preferences = lcGetPreferences();

	lcShadingMode ShadingMode = Preferences.mShadingMode;
	if (ShadingMode == lcShadingMode::Wireframe && !mAllowWireframe)
		ShadingMode = lcShadingMode::Flat;

	if (ShadingMode == lcShadingMode::Wireframe)
	{
		int PrimitiveTypes = LC_MESH_LINES;

		if (DrawConditional)
			PrimitiveTypes |= LC_MESH_CONDITIONAL_LINES;

		DrawOpaqueMeshes(Context, false, PrimitiveTypes, true, true);

		if (mPreTranslucentCallback)
			mPreTranslucentCallback();
	}
	else if (ShadingMode == lcShadingMode::Flat)
	{
		const bool DrawLines = Preferences.mDrawEdgeLines && Preferences.mLineWidth != 0.0f;

		int LinePrimitiveTypes = 0;

		if (DrawLines)
		{
			LinePrimitiveTypes |= LC_MESH_LINES;

			if (DrawConditional)
				LinePrimitiveTypes |= LC_MESH_CONDITIONAL_LINES;
		}

		const int SolidPrimitiveTypes = LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES;

		if (mTranslucentFade && mHasFadedParts)
		{
			DrawOpaqueMeshes(Context, false, SolidPrimitiveTypes | LinePrimitiveTypes, false, true);

			if (mPreTranslucentCallback)
				mPreTranslucentCallback();

			DrawTranslucentMeshes(Context, false, true, true, false);

			if (DrawLines)
				DrawOpaqueMeshes(Context, false, LinePrimitiveTypes, true, false);

			DrawTranslucentMeshes(Context, true, false, true, true);
		}
		else
		{
			DrawOpaqueMeshes(Context, false, SolidPrimitiveTypes | LinePrimitiveTypes, true, true);

			if (mPreTranslucentCallback)
				mPreTranslucentCallback();

			DrawTranslucentMeshes(Context, true, false, true, true);
		}
	}
	else
	{
		const bool DrawLines = Preferences.mDrawEdgeLines && Preferences.mLineWidth != 0.0f;

		int LinePrimitiveTypes = LC_MESH_LINES;

		if (DrawConditional)
			LinePrimitiveTypes |= LC_MESH_CONDITIONAL_LINES;

		if (mTranslucentFade && mHasFadedParts)
		{
			DrawOpaqueMeshes(Context, true, LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES, false, true);

			if (DrawLines)
				DrawOpaqueMeshes(Context, false, LinePrimitiveTypes, false, true);

			if (mPreTranslucentCallback)
				mPreTranslucentCallback();

			DrawTranslucentMeshes(Context, false, true, true, false);

			if (DrawLines)
				DrawOpaqueMeshes(Context, false, LinePrimitiveTypes, true, false);

			DrawTranslucentMeshes(Context, true, false, true, true);
		}
		else
		{
			if (DrawLines)
				DrawOpaqueMeshes(Context, false, LinePrimitiveTypes, true, true);

			DrawOpaqueMeshes(Context, true, LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES, true, true);

			if (mPreTranslucentCallback)
				mPreTranslucentCallback();

			DrawTranslucentMeshes(Context, true, false, true, true);
		}
	}
}

void lcScene::DrawInterfaceObjects(lcContext* Context) const
{
	for (const lcObject* Object : mInterfaceObjects)
		Object->DrawInterface(Context, *this);
}

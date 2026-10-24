#include "lc_global.h"
#include "lc_scene.h"
#include "lc_context.h"
#include "lc_piecedata.h"
#include "lc_texture.h"
#include "lc_library.h"
#include "lc_application.h"
#include "object.h"
#include "lc_colors.h"

lcScene::lcScene()
{
	mHighlightRenderMode = lcHighlightRenderMode::Tint;
	mShadingMode = lcShadingMode::DefaultLights;
	mMeshLODDistance = 250.0f;
}

void lcScene::Begin(const lcMatrix44& ViewMatrix)
{
	mMissingAssets = false;
	mHighlightRenderMode = lcHighlightRenderMode::Tint;
	mViewMatrix = ViewMatrix;
	mActiveSubmodelInstance = nullptr;
	mRenderMeshes.clear();
	mOpaqueMeshes.clear();
	mTranslucentMeshes.clear();
	mInterfaceObjects.clear();

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
	lcRenderMesh& RenderMesh = mRenderMeshes.emplace_back();

	RenderMesh.WorldMatrix = WorldMatrix;
	RenderMesh.Mesh = Mesh;
	RenderMesh.ColorIndex = ColorIndex;
	RenderMesh.State = State;
	const float Distance = fabsf(lcMul31(WorldMatrix[3], mViewMatrix).z) - mMeshLODDistance;
	RenderMesh.LodIndex = mAllowLOD ? RenderMesh.Mesh->GetLodIndex(Distance) : LC_MESH_LOD_HIGH;

	const bool ForceTranslucent = (mTranslucentFade && State == lcRenderMeshState::Faded);
	const bool Translucent = lcIsColorTranslucent(ColorIndex) || ForceTranslucent;
	const lcMeshFlags Flags = Mesh->mFlags;
	mHasFadedParts |= State == lcRenderMeshState::Faded;

	if ((Flags & (lcMeshFlag::HasSolid | lcMeshFlag::HasLines)) || ((Flags & lcMeshFlag::HasDefault) && !Translucent))
		mOpaqueMeshes.emplace_back(static_cast<int>(mRenderMeshes.size()) - 1);

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

			lcTranslucentMeshInstance& Instance = mTranslucentMeshes.emplace_back();
			Instance.Section = Section;
			Instance.Distance = InstanceDistance;
			Instance.RenderMeshIndex = static_cast<int>(mRenderMeshes.size()) - 1;
		}
	}
}

void lcScene::DrawDebugNormals(lcContext* Context, const lcMesh* Mesh) const
{
	const lcVertex* const VertexBuffer = Mesh->GetVertexData();
	const lcVertexTextured* const TexturedVertexBuffer = Mesh->GetTexturedVertexData();
	lcVector3* const Vertices = (lcVector3*)malloc((Mesh->mNumVertices + Mesh->mNumTexturedVertices) * 2 * sizeof(lcVector3));

	for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
	{
		Vertices[VertexIdx * 2] = VertexBuffer[VertexIdx].Position;
		Vertices[VertexIdx * 2 + 1] = VertexBuffer[VertexIdx].Position + lcUnpackNormal(VertexBuffer[VertexIdx].Normal);
	}

	for (int VertexIdx = 0; VertexIdx < Mesh->mNumTexturedVertices; VertexIdx++)
	{
		Vertices[(Mesh->mNumVertices + VertexIdx) * 2] = TexturedVertexBuffer[VertexIdx].Position;
		Vertices[(Mesh->mNumVertices + VertexIdx) * 2 + 1] = TexturedVertexBuffer[VertexIdx].Position + lcUnpackNormal(TexturedVertexBuffer[VertexIdx].Normal);
	}

	Context->SetVertexBufferPointer(Vertices);
	Context->SetVertexFormatPosition(3);
	Context->DrawPrimitives(GL_LINES, 0, (Mesh->mNumVertices + Mesh->mNumTexturedVertices) * 2);
	free(Vertices);
}

void lcScene::DrawOpaqueMeshes(lcContext* Context, bool DrawLit, int PrimitiveTypes, bool DrawFaded, bool DrawNonFaded) const
{
	if (mOpaqueMeshes.empty())
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

	const lcPreferences& Preferences = lcGetPreferences();
	const lcVector4 FocusedColor = lcVector4FromColor(Preferences.mObjectFocusedColor);
	const lcVector4 SelectedColor = lcVector4FromColor(Preferences.mObjectSelectedColor);

	for (const int MeshIndex : mOpaqueMeshes)
	{
		const lcRenderMesh& RenderMesh = mRenderMeshes[MeshIndex];
		const lcMesh* Mesh = RenderMesh.Mesh;
		const int LodIndex = RenderMesh.LodIndex;
		lcRenderMeshState State = RenderMesh.State;
		const bool HasTriangles = (Mesh->mFlags & (lcMeshFlag::HasDefault | lcMeshFlag::HasSolid | lcMeshFlag::HasTranslucent)) != 0;

		if (mHighlightRenderMode == lcHighlightRenderMode::Outline && HasTriangles &&
			(State == lcRenderMeshState::Selected || State == lcRenderMeshState::Focused || State == lcRenderMeshState::Highlighted))
			State = lcRenderMeshState::Default;

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

				switch (State)
				{
				case lcRenderMeshState::Default:
				case lcRenderMeshState::Highlighted:
					Context->SetColorIndex(ColorIndex);
					break;

				case lcRenderMeshState::Selected:
					Context->SetColorIndexTinted(ColorIndex, SelectedColor, 0.5f);
					break;

				case lcRenderMeshState::Focused:
				case lcRenderMeshState::Preview:
					Context->SetColorIndexTinted(ColorIndex, FocusedColor, 0.5f);
					break;

				case lcRenderMeshState::Faded:
					if (mTranslucentFade)
						continue;
					Context->SetColorIndexTinted(ColorIndex, mFadeColor);
					break;
				}
			}
			else if (Section->PrimitiveType & (LC_MESH_LINES | LC_MESH_CONDITIONAL_LINES))
			{
				switch (State)
				{
				case lcRenderMeshState::Default:
					if (mShadingMode != lcShadingMode::Wireframe)
					{
						if (ColorIndex != gEdgeColor)
							Context->SetColorIndex(ColorIndex);
						else
							Context->SetEdgeColorIndex(RenderMesh.ColorIndex);
					}
					else
					{
						if (ColorIndex == gEdgeColor)
							ColorIndex = RenderMesh.ColorIndex;

						Context->SetColorIndex(ColorIndex);
					}
					break;

				case lcRenderMeshState::Selected:
					Context->SetColor(SelectedColor);
					break;

				case lcRenderMeshState::Focused:
				case lcRenderMeshState::Preview:
					Context->SetColor(FocusedColor);
					break;

				case lcRenderMeshState::Highlighted:
					Context->SetColor(mHighlightColor);
					break;

				case lcRenderMeshState::Faded:
					Context->SetEdgeColorIndexTinted(ColorIndex, mFadeColor);
					break;
				}

				if (Section->PrimitiveType == LC_MESH_CONDITIONAL_LINES)
				{
					int VertexBufferOffset = Mesh->mVertexCacheOffset != -1 ? Mesh->mVertexCacheOffset : 0;
					VertexBufferOffset += Mesh->mNumVertices * sizeof(lcVertex) + Mesh->mNumTexturedVertices * sizeof(lcVertexTextured);
					Context->SetMaterial(lcMaterialType::UnlitColorConditional);
					Context->SetVertexFormatConditional(VertexBufferOffset);

					Context->DrawPrimitives(GL_LINES, Section->DrawOffset, Section->DrawCount);

					continue;
				}
			}

			int VertexBufferOffset = Mesh->mVertexCacheOffset != -1 ? Mesh->mVertexCacheOffset : 0;
			const int IndexBufferOffset = Mesh->mIndexCacheOffset != -1 ? Mesh->mIndexCacheOffset : 0;

			if (Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
			{
				Context->SetMaterial(FlatMaterial);
				Context->SetVertexFormat(VertexBufferOffset, 3, 1, 0, 0, DrawLit);
			}
			else
			{
				lcTexture* const Texture = Section->Texture.Get();

				if (Texture)
				{
					if (Texture->NeedsUpload())
						Texture->Upload(Context);

					Context->SetMaterial(TexturedMaterial);
					Context->BindTexture2D(Texture);
				}
				else
				{
					Context->SetMaterial(FlatMaterial);
				}

				VertexBufferOffset += Mesh->mNumVertices * sizeof(lcVertex);
				Context->SetVertexFormat(VertexBufferOffset, 3, 1, 2, 0, DrawLit);
			}

			const GLenum DrawPrimitiveType = Section->PrimitiveType & (LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES) ? GL_TRIANGLES : GL_LINES;
			Context->DrawIndexedPrimitives(DrawPrimitiveType, Section->DrawCount, Mesh->mIndexType, IndexBufferOffset + Section->DrawOffset);
		}

#ifdef LC_DEBUG_NORMALS
		DrawDebugNormals(Context, Mesh);
#endif
	}

	Context->ClearTexture2D();
	Context->SetPolygonOffset(lcPolygonOffset::None);
}

void lcScene::DrawTranslucentMeshes(lcContext* Context, bool DrawLit, bool DrawFadePrepass, bool DrawFaded, bool DrawNonFaded) const
{
	if (mTranslucentMeshes.empty())
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
		Context->EnableColorBlend(true);
		Context->SetDepthWrite(false);
	}
	else
		Context->EnableColorWrite(false);

	Context->SetPolygonOffset(lcPolygonOffset::Translucent);

	const lcPreferences& Preferences = lcGetPreferences();
	const lcVector4 FocusedColor = lcVector4FromColor(Preferences.mObjectFocusedColor);
	const lcVector4 SelectedColor = lcVector4FromColor(Preferences.mObjectSelectedColor);

	for (const lcTranslucentMeshInstance& MeshInstance : mTranslucentMeshes)
	{
		const lcRenderMesh& RenderMesh = mRenderMeshes[MeshInstance.RenderMeshIndex];
		const lcMesh* Mesh = RenderMesh.Mesh;
		lcRenderMeshState State = RenderMesh.State;

		if (mHighlightRenderMode == lcHighlightRenderMode::Outline && (State == lcRenderMeshState::Selected || State == lcRenderMeshState::Focused))
			State = lcRenderMeshState::Default;

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

		switch (State)
		{
		case lcRenderMeshState::Default:
		case lcRenderMeshState::Highlighted:
			Context->SetColorIndex(ColorIndex);
			break;

		case lcRenderMeshState::Selected:
			Context->SetColorIndexTinted(ColorIndex, SelectedColor, 0.5f);
			break;

		case lcRenderMeshState::Focused:
		case lcRenderMeshState::Preview:
			Context->SetColorIndexTinted(ColorIndex, FocusedColor, 0.5f);
			break;

		case lcRenderMeshState::Faded:
			Context->SetColorIndexTinted(ColorIndex, mFadeColor);
			break;
		}

		lcTexture* const Texture = Section->Texture.Get();
		int VertexBufferOffset = Mesh->mVertexCacheOffset != -1 ? Mesh->mVertexCacheOffset : 0;
		const int IndexBufferOffset = Mesh->mIndexCacheOffset != -1 ? Mesh->mIndexCacheOffset : 0;

		if (!Texture)
		{
			Context->SetMaterial(FlatMaterial);
			Context->SetVertexFormat(VertexBufferOffset, 3, 1, 0, 0, DrawLit);
		}
		else
		{
			if (Texture->NeedsUpload())
				Texture->Upload(Context);
			Context->SetMaterial(TexturedMaterial);
			VertexBufferOffset += Mesh->mNumVertices * sizeof(lcVertex);
			Context->SetVertexFormat(VertexBufferOffset, 3, 1, 2, 0, DrawLit);
			Context->BindTexture2D(Texture);
		}

		const GLenum DrawPrimitiveType = Section->PrimitiveType & (LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES) ? GL_TRIANGLES : GL_LINES;
		Context->DrawIndexedPrimitives(DrawPrimitiveType, Section->DrawCount, Mesh->mIndexType, IndexBufferOffset + Section->DrawOffset);

#ifdef LC_DEBUG_NORMALS
		DrawDebugNormals(Context, Mesh);
#endif
	}

	Context->ClearTexture2D();
	Context->SetPolygonOffset(lcPolygonOffset::None);

	if (!DrawFadePrepass)
	{
		Context->SetDepthWrite(true);
		Context->EnableColorBlend(false);
	}
	else
		Context->EnableColorWrite(true);
}

void lcScene::Draw(lcContext* Context, const std::function<void()>& PreTranslucentCallback) const
{
	// TODO: find a better place for these updates
	lcGetPiecesLibrary()->UpdateBuffers(Context);

	Context->SetViewMatrix(mViewMatrix);

	const bool DrawLines = mDrawEdgeLines && mLineWidth > 0.0f;
	const bool DrawConditional = mDrawConditionalLines && mLineWidth > 0.0f && lcContext::SupportsMaterial(lcMaterialType::UnlitColorConditional);

	lcShadingMode ShadingMode = mShadingMode;

	if (ShadingMode != lcShadingMode::Wireframe && ShadingMode != lcShadingMode::Flat && !lcContext::SupportsDefaultLights())
		ShadingMode = lcShadingMode::Flat;

	if (ShadingMode == lcShadingMode::Wireframe)
	{
		DrawOpaqueMeshes(Context, false, LC_MESH_LINES, true, true);

		if (DrawConditional)
			DrawOpaqueMeshes(Context, false, LC_MESH_CONDITIONAL_LINES, true, true);

		if (PreTranslucentCallback)
			PreTranslucentCallback();
	}
	else if (ShadingMode == lcShadingMode::Flat)
	{
		const int SolidPrimitiveTypes = LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES | (DrawLines ? LC_MESH_LINES : 0);

		if (mTranslucentFade && mHasFadedParts)
		{
			DrawOpaqueMeshes(Context, false, SolidPrimitiveTypes, false, true);

			if (PreTranslucentCallback)
				PreTranslucentCallback();

			DrawTranslucentMeshes(Context, false, true, true, false);

			if (DrawLines)
				DrawOpaqueMeshes(Context, false, LC_MESH_LINES, true, false);

			if (DrawConditional)
				DrawOpaqueMeshes(Context, false, LC_MESH_CONDITIONAL_LINES, true, false);

			DrawTranslucentMeshes(Context, false, false, true, true);
		}
		else
		{
			DrawOpaqueMeshes(Context, false, SolidPrimitiveTypes, true, true);

			if (DrawConditional)
				DrawOpaqueMeshes(Context, false, LC_MESH_CONDITIONAL_LINES, true, true);

			if (PreTranslucentCallback)
				PreTranslucentCallback();

			DrawTranslucentMeshes(Context, false, false, true, true);
		}
	}
	else
	{
		if (mTranslucentFade && mHasFadedParts)
		{
			DrawOpaqueMeshes(Context, true, LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES, false, true);

			if (DrawLines)
				DrawOpaqueMeshes(Context, false, LC_MESH_LINES, false, true);

			if (DrawConditional)
				DrawOpaqueMeshes(Context, false, LC_MESH_CONDITIONAL_LINES, false, true);

			if (PreTranslucentCallback)
				PreTranslucentCallback();

			DrawTranslucentMeshes(Context, false, true, true, false);

			if (DrawLines)
				DrawOpaqueMeshes(Context, false, LC_MESH_LINES, true, false);

			if (DrawConditional)
				DrawOpaqueMeshes(Context, false, LC_MESH_CONDITIONAL_LINES, true, false);

			DrawTranslucentMeshes(Context, true, false, true, true);
		}
		else
		{
			if (DrawLines)
				DrawOpaqueMeshes(Context, false, LC_MESH_LINES, true, true);

			if (DrawConditional)
				DrawOpaqueMeshes(Context, false, LC_MESH_CONDITIONAL_LINES, true, true);

			DrawOpaqueMeshes(Context, true, LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES, true, true);

			if (PreTranslucentCallback)
				PreTranslucentCallback();

			DrawTranslucentMeshes(Context, true, false, true, true);
		}
	}
}

bool lcScene::HasSelectedMeshes() const
{
	return std::any_of(mRenderMeshes.begin(), mRenderMeshes.end(), [](const lcRenderMesh& Mesh)
	{
		return Mesh.State == lcRenderMeshState::Selected || Mesh.State == lcRenderMeshState::Focused;
	});
}

bool lcScene::CanReuseSceneDepth() const
{
	// Faded depth prepasses and insertion previews should not obscure selection.
	if (mTranslucentFade && mHasFadedParts)
		return false;

	return std::none_of(mRenderMeshes.begin(), mRenderMeshes.end(), [](const lcRenderMesh& Mesh)
	{
		return Mesh.State == lcRenderMeshState::Preview;
	});
}

bool lcScene::HasHighlightedMeshes() const
{
	return std::any_of(mRenderMeshes.begin(), mRenderMeshes.end(), [](const lcRenderMesh& Mesh)
	{
		return Mesh.State == lcRenderMeshState::Highlighted;
	});
}

void lcScene::DrawSelectionMask(lcContext* Context) const
{
	DrawOutlineGeometry(Context, lcOutlineGeometry::Selection);
}

void lcScene::DrawHighlightMask(lcContext* Context) const
{
	DrawOutlineGeometry(Context, lcOutlineGeometry::Highlight);
}

void lcScene::DrawOutlineOccluders(lcContext* Context) const
{
	DrawOutlineGeometry(Context, lcOutlineGeometry::Occluders);
}

void lcScene::DrawOutlineGeometry(lcContext* Context, lcOutlineGeometry Geometry) const
{
	Context->SetViewMatrix(mViewMatrix);
	Context->SetMaterial(lcMaterialType::UnlitColor);
	Context->ClearTexture2D();

	for (const lcRenderMesh& RenderMesh : mRenderMeshes)
	{
		const bool Selected = RenderMesh.State == lcRenderMeshState::Selected || RenderMesh.State == lcRenderMeshState::Focused;
		const bool Highlighted = RenderMesh.State == lcRenderMeshState::Highlighted;

		if (RenderMesh.State == lcRenderMeshState::Preview ||
			(Geometry == lcOutlineGeometry::Selection && !Selected) ||
			(Geometry == lcOutlineGeometry::Highlight && !Highlighted))
			continue;

		if (Geometry == lcOutlineGeometry::Occluders && mTranslucentFade && RenderMesh.State == lcRenderMeshState::Faded)
			continue;

		const lcMesh* Mesh = RenderMesh.Mesh;
		Context->BindMesh(Mesh);
		Context->SetWorldMatrix(RenderMesh.WorldMatrix);
		Context->SetColor(lcVector4(Selected ? 1.0f : 0.0f, RenderMesh.State == lcRenderMeshState::Focused ? 1.0f : 0.0f, Highlighted ? 1.0f : 0.0f, 1.0f));

		const lcMeshLod& Lod = Mesh->mLods[RenderMesh.LodIndex];
		for (int SectionIndex = 0; SectionIndex < Lod.NumSections; SectionIndex++)
		{
			const lcMeshSection& Section = Lod.Sections[SectionIndex];
			if (!(Section.PrimitiveType & (LC_MESH_TRIANGLES | LC_MESH_TEXTURED_TRIANGLES)))
				continue;

			const int ColorIndex = Section.ColorIndex == gDefaultColor ? RenderMesh.ColorIndex : Section.ColorIndex;
			if (Geometry == lcOutlineGeometry::Occluders && !Selected && lcIsColorTranslucent(ColorIndex))
				continue;

			int VertexOffset = Mesh->mVertexCacheOffset != -1 ? Mesh->mVertexCacheOffset : 0;
			if (Section.PrimitiveType == LC_MESH_TEXTURED_TRIANGLES)
			{
				VertexOffset += Mesh->mNumVertices * sizeof(lcVertex);
				Context->SetVertexFormat(VertexOffset, 3, 1, 2, 0, false);
			}
			else
				Context->SetVertexFormat(VertexOffset, 3, 1, 0, 0, false);

			const int IndexOffset = Mesh->mIndexCacheOffset != -1 ? Mesh->mIndexCacheOffset : 0;
			Context->DrawIndexedPrimitives(GL_TRIANGLES, Section.DrawCount, Mesh->mIndexType, IndexOffset + Section.DrawOffset);
		}
	}
}

void lcScene::DrawInterfaceObjects(lcContext* Context) const
{
	for (const lcObject* Object : mInterfaceObjects)
		Object->DrawInterface(Context, *this);
}

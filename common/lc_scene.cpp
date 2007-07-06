#include "lc_global.h"
#include "lc_scene.h"

#include "lc_mesh.h"
#include "lc_pieceobj.h"
#include "globals.h"

int SortRenderSectionsCallback(const lcRenderSection& a, const lcRenderSection& b, void* SortData)
{
	if (a.Distance < b.Distance)
		return -1;
	else if (a.Distance > b.Distance)
		return 1;
	else
		return 0;
}

lcScene::lcScene(int OpaqueSize, int OpaqueGrow, int TranslucentSize, int TranslucentGrow)
	: m_OpaqueSections(OpaqueSize, OpaqueGrow),
	m_TranslucentSections(TranslucentSize, TranslucentGrow)
{
}

lcScene::~lcScene()
{
}

void lcScene::Render()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	lcVertexBuffer* LastVertexBuffer = NULL;
	lcIndexBuffer* LastIndexBuffer = NULL;
	lcPieceObject* LastPiece = NULL;
	lcMesh* LastMesh;

	glPushMatrix();

	// Render opaque sections.
	for (int i = 0; i < m_OpaqueSections.GetSize(); i++)
	{
		lcRenderSection& RenderSection = m_OpaqueSections[i];
		lcMesh* Mesh = RenderSection.Mesh;

		if (Mesh->m_VertexBuffer != LastVertexBuffer)
		{
			LastVertexBuffer = Mesh->m_VertexBuffer;
			LastVertexBuffer->BindBuffer();
		}

		if (Mesh->m_IndexBuffer != LastIndexBuffer)
		{
			LastIndexBuffer = Mesh->m_IndexBuffer;
			LastIndexBuffer->BindBuffer();
		}

		// FIXME: remove Owner
		if (LastPiece != RenderSection.Owner || LastMesh != Mesh)
		{
			LastPiece = RenderSection.Owner;
			LastMesh = Mesh;
			glPopMatrix();
			glPushMatrix();
			glMultMatrixf(RenderSection.ModelWorld);

			if (LastPiece->IsSelected())
				glLineWidth(2.0f);
			else
				glLineWidth(1.0f);
		}

		glColor3ubv(FlatColorArray[RenderSection.Color]);

		lcMeshSection* Section = RenderSection.Section;

#ifdef LC_PROFILE
		if (Section->PrimitiveType == GL_QUADS)
			g_RenderStats.QuadCount += Section->IndexCount / 4;
		else if (Section->PrimitiveType == GL_TRIANGLES)
			g_RenderStats.TriCount += Section->IndexCount / 3;
		if (Section->PrimitiveType == GL_LINES)
			g_RenderStats.LineCount += Section->IndexCount / 2;
#endif

		glDrawElements(Section->PrimitiveType, Section->IndexCount, Mesh->m_IndexType, (char*)LastIndexBuffer->GetDrawElementsOffset() + Section->IndexOffset);
	}

	// Render translucent sections.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);

	for (int j = 0; j < m_TranslucentSections.GetSize(); j++)
	{
		lcRenderSection& RenderSection = m_TranslucentSections[j];
		lcMesh* Mesh = RenderSection.Mesh;

		if (Mesh->m_VertexBuffer != LastVertexBuffer)
		{
			LastVertexBuffer = Mesh->m_VertexBuffer;
			LastVertexBuffer->BindBuffer();
		}

		if (Mesh->m_IndexBuffer != LastIndexBuffer)
		{
			LastIndexBuffer = Mesh->m_IndexBuffer;
			LastIndexBuffer->BindBuffer();
		}

		if (LastPiece != RenderSection.Owner)
		{
			LastPiece = RenderSection.Owner;
			glPopMatrix();
			glPushMatrix();
			glMultMatrixf(RenderSection.ModelWorld);
		}

		glColor4ubv(ColorArray[RenderSection.Color]);

		lcMeshSection* Section = RenderSection.Section;

#ifdef LC_PROFILE
		if (Section->PrimitiveType == GL_QUADS)
			g_RenderStats.QuadCount += Section->IndexCount / 4;
		else if (Section->PrimitiveType == GL_TRIANGLES)
			g_RenderStats.TriCount += Section->IndexCount / 3;
		if (Section->PrimitiveType == GL_LINES)
			g_RenderStats.LineCount += Section->IndexCount / 2;
#endif

		glDrawElements(Section->PrimitiveType, Section->IndexCount, Mesh->m_IndexType, (char*)LastIndexBuffer->GetDrawElementsOffset() + Section->IndexOffset);
	}

	glPopMatrix();

	// Reset states.
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	if (LastVertexBuffer)
		LastVertexBuffer->UnbindBuffer();

	if (LastIndexBuffer)
		LastIndexBuffer->UnbindBuffer();
}

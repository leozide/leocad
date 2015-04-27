#include "lc_global.h"
#include "lc_context.h"
#include "lc_mesh.h"
#include "lc_texture.h"
#include "lc_colors.h"
#include "lc_mainwindow.h"
#include "lc_library.h"

static int lcOpaqueRenderMeshCompare(const void* Elem1, const void* Elem2)
{
	lcRenderMesh* Mesh1 = (lcRenderMesh*)Elem1;
	lcRenderMesh* Mesh2 = (lcRenderMesh*)Elem2;

	if (Mesh1->Mesh < Mesh2->Mesh)
		return -1;

	if (Mesh1->Mesh > Mesh2->Mesh)
		return 1;

	return 0;
}

static int lcTranslucentRenderMeshCompare(const void* Elem1, const void* Elem2)
{
	lcRenderMesh* Mesh1 = (lcRenderMesh*)Elem1;
	lcRenderMesh* Mesh2 = (lcRenderMesh*)Elem2;

	if (Mesh1->Distance < Mesh2->Distance)
		return -1;

	if (Mesh1->Distance > Mesh2->Distance)
		return 1;

	return 0;
}

lcScene::lcScene()
	: mOpaqueMeshes(0, 1024), mTranslucentMeshes(0, 1024), mInterfaceObjects(0, 1024)
{
}

void lcScene::Begin(const lcMatrix44& ViewMatrix)
{
	mOpaqueMeshes.RemoveAll();
	mTranslucentMeshes.RemoveAll();
	mInterfaceObjects.RemoveAll();
}

void lcScene::End()
{
	qsort(&mOpaqueMeshes[0], mOpaqueMeshes.GetSize(), sizeof(mOpaqueMeshes[0]), lcOpaqueRenderMeshCompare);
	qsort(&mTranslucentMeshes[0], mTranslucentMeshes.GetSize(), sizeof(mTranslucentMeshes[0]), lcTranslucentRenderMeshCompare);
}

lcContext::lcContext()
{
	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = NULL;
	mIndexBufferPointer = NULL;
	mVertexBufferOffset = (char*)~0;

	mTexCoordEnabled = false;
	mColorEnabled = false;

	mTexture = NULL;
	mLineWidth = 1.0f;
	mMatrixMode = GL_MODELVIEW;

	mFramebufferObject = 0;
	mFramebufferTexture = 0;
	mDepthRenderbufferObject = 0;

    mViewportX = 0;
    mViewportY = 0;
    mViewportWidth = 1;
    mViewportHeight = 1;
}

lcContext::~lcContext()
{
}

void lcContext::SetDefaultState()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	if (GL_HasVertexBufferObject())
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}

	glVertexPointer(3, GL_FLOAT, 0, NULL);
	glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	glColorPointer(4, GL_FLOAT, 0, NULL);
	mTexCoordEnabled = false;
	mColorEnabled = false;

	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = NULL;
	mIndexBufferPointer = NULL;
	mVertexBufferOffset = (char*)~0;

	glDisable(GL_TEXTURE_2D);
	mTexture = NULL;

	glLineWidth(1.0f);
	mLineWidth = 1.0f;

	glMatrixMode(GL_MODELVIEW);
	mMatrixMode = GL_MODELVIEW;
}

void lcContext::SetViewport(int x, int y, int Width, int Height)
{
	if (mViewportX == x && mViewportY == y && mViewportWidth == Width && mViewportHeight == Height)
		return;

	glViewport(x, y, Width, Height);

	mViewportX = x;
	mViewportY = y;
	mViewportWidth = Width;
	mViewportHeight = Height;
}

void lcContext::SetWorldViewMatrix(const lcMatrix44& WorldViewMatrix)
{
	if (mMatrixMode != GL_MODELVIEW)
	{
		glMatrixMode(GL_MODELVIEW);
		mMatrixMode = GL_MODELVIEW;
	}

	glLoadMatrixf(WorldViewMatrix);
}

void lcContext::SetProjectionMatrix(const lcMatrix44& ProjectionMatrix)
{
	if (mMatrixMode != GL_PROJECTION)
	{
		glMatrixMode(GL_PROJECTION);
		mMatrixMode = GL_PROJECTION;
	}

	glLoadMatrixf(ProjectionMatrix);
}

void lcContext::SetLineWidth(float LineWidth)
{
	if (LineWidth == mLineWidth)
		return;

	glLineWidth(LineWidth);
	mLineWidth = LineWidth;
}

bool lcContext::BeginRenderToTexture(int Width, int Height)
{
	if (GL_SupportsFramebufferObjectARB)
	{
		glGenFramebuffers(1, &mFramebufferObject);
		glGenTextures(1, &mFramebufferTexture);
		glGenRenderbuffers(1, &mDepthRenderbufferObject);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFramebufferObject);

		glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFramebufferTexture, 0);

		glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbufferObject);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, Width, Height);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbufferObject);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferObject);

		if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			EndRenderToTexture();
			return false;
		}

		return true;
	}

	if (GL_SupportsFramebufferObjectEXT)
	{
		glGenFramebuffersEXT(1, &mFramebufferObject); 
		glGenTextures(1, &mFramebufferTexture); 

		glBindTexture(GL_TEXTURE_2D, mFramebufferTexture); 
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); 

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFramebufferObject); 
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, mFramebufferTexture, 0); 

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFramebufferObject);

		glGenRenderbuffersEXT(1, &mDepthRenderbufferObject); 
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mDepthRenderbufferObject); 
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, Width, Height);

		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepthRenderbufferObject); 

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFramebufferObject);

		if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
		{
			EndRenderToTexture();
			return false;
		}

		return true;
	}

	return false;
}

void lcContext::EndRenderToTexture()
{
	if (GL_SupportsFramebufferObjectARB)
	{
		glDeleteFramebuffers(1, &mFramebufferObject);
		mFramebufferObject = 0;
		glDeleteTextures(1, &mFramebufferTexture);
		mFramebufferTexture = 0;
		glDeleteRenderbuffers(1, &mDepthRenderbufferObject);
		mDepthRenderbufferObject = 0;

		return;
	}

	if (GL_SupportsFramebufferObjectEXT)
	{
		glDeleteFramebuffersEXT(1, &mFramebufferObject);
		mFramebufferObject = 0;
		glDeleteTextures(1, &mFramebufferTexture);
		mFramebufferTexture = 0;
		glDeleteRenderbuffersEXT(1, &mDepthRenderbufferObject);
		mDepthRenderbufferObject = 0;
	}
}

bool lcContext::SaveRenderToTextureImage(const QString& FileName, int Width, int Height)
{
	lcuint8* Buffer = (lcuint8*)malloc(Width * Height * 4);

	glFinish();
	glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Buffer);

	for (int y = 0; y < (Height + 1) / 2; y++)
	{
		lcuint8* Top = Buffer + ((Height - y - 1) * Width * 4);
		lcuint8* Bottom = Buffer + y * Width * 4;

		for (int x = 0; x < Width; x++)
		{
			lcuint8 Red = Top[0];
			lcuint8 Green = Top[1];
			lcuint8 Blue = Top[2];
			lcuint8 Alpha = Top[3];

			Top[0] = Bottom[2];
			Top[1] = Bottom[1];
			Top[2] = Bottom[0];
			Top[3] = Bottom[3];

			Bottom[0] = Blue;
			Bottom[1] = Green;
			Bottom[2] = Red;
			Bottom[3] = Alpha;

			Top += 4;
			Bottom +=4;
		}
	}

    QImageWriter Writer(FileName);
    bool Result = Writer.write(QImage(Buffer, Width, Height, QImage::Format_ARGB32));

	if (!Result)
		QMessageBox::information(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));

	free(Buffer);

	return Result;
}

lcVertexBuffer lcContext::CreateVertexBuffer(int Size, const void* Data)
{
	if (GL_HasVertexBufferObject())
	{
		lcVertexBuffer VertexBuffer;

		glGenBuffers(1, &VertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER_ARB, VertexBuffer);
		glBufferData(GL_ARRAY_BUFFER_ARB, Size, Data, GL_STATIC_DRAW_ARB);

		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0); // context remove
		mVertexBufferObject = 0;

		return VertexBuffer;
	}
	else
	{
		void* VertexBuffer = malloc(Size);
		memcpy(VertexBuffer, Data, Size);
		return (lcVertexBuffer)VertexBuffer;
	}
}

void lcContext::DestroyVertexBuffer(lcVertexBuffer& VertexBuffer)
{
	if (!VertexBuffer)
		return;

	if (GL_HasVertexBufferObject())
	{
		if (mVertexBufferObject == VertexBuffer)
		{
			glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
			mVertexBufferObject = 0;
		}

		glDeleteBuffers(1, &VertexBuffer);
	}
	else
	{
		free((void*)VertexBuffer);
	}
}

lcIndexBuffer lcContext::CreateIndexBuffer(int Size, const void* Data)
{
	if (GL_HasVertexBufferObject())
	{
		lcIndexBuffer IndexBuffer;

		glGenBuffers(1, &IndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, IndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER_ARB, Size, Data, GL_STATIC_DRAW_ARB);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0); // context remove
		mIndexBufferObject = 0;

		return IndexBuffer;
	}
	else
	{
		void* IndexBuffer = malloc(Size);
		memcpy(IndexBuffer, Data, Size);
		return (lcIndexBuffer)IndexBuffer;
	}
}

void lcContext::DestroyIndexBuffer(lcIndexBuffer& IndexBuffer)
{
	if (!IndexBuffer)
		return;

	if (GL_HasVertexBufferObject())
	{
		if (mIndexBufferObject == IndexBuffer)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			mIndexBufferObject = 0;
		}

		glDeleteBuffers(1, &IndexBuffer);
	}
	else
	{
		free((void*)IndexBuffer);
	}
}

void lcContext::ClearVertexBuffer()
{
	mVertexBufferPointer = NULL;
	mVertexBufferOffset = (char*)~0;

	if (mVertexBufferObject)
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		mVertexBufferObject = 0;
	}

	if (mTexCoordEnabled)
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (mColorEnabled)
		glDisableClientState(GL_COLOR_ARRAY);
}

void lcContext::SetVertexBuffer(lcVertexBuffer VertexBuffer)
{
	if (GL_HasVertexBufferObject())
	{
		GLuint VertexBufferObject = VertexBuffer;
		mVertexBufferPointer = NULL;

		if (VertexBufferObject != mVertexBufferObject)
		{
			glBindBuffer(GL_ARRAY_BUFFER_ARB, VertexBufferObject);
			mVertexBufferObject = VertexBufferObject;
			mVertexBufferOffset = (char*)~0;
		}
	}
	else
	{
		mVertexBufferPointer = (char*)VertexBuffer;
		mVertexBufferOffset = (char*)~0;
	}
}

void lcContext::SetVertexBufferPointer(const void* VertexBuffer)
{
	if (GL_HasVertexBufferObject() && mVertexBufferObject)
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		mVertexBufferObject = 0;
	}

	mVertexBufferPointer = (char*)VertexBuffer;
	mVertexBufferOffset = (char*)~0;
}

void lcContext::SetVertexFormat(int BufferOffset, int PositionSize, int TexCoordSize, int ColorSize)
{
	int VertexSize = (PositionSize + TexCoordSize + ColorSize) * sizeof(float);
	char* VertexBufferPointer = mVertexBufferPointer + BufferOffset;

	if (mVertexBufferOffset != VertexBufferPointer)
	{
		glVertexPointer(PositionSize, GL_FLOAT, VertexSize, VertexBufferPointer);
		mVertexBufferOffset = VertexBufferPointer;
	}

	if (TexCoordSize)
	{
		glTexCoordPointer(TexCoordSize, GL_FLOAT, VertexSize, VertexBufferPointer + PositionSize * sizeof(float));

		if (!mTexCoordEnabled)
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			mTexCoordEnabled = true;
		}
	}
	else if (mTexCoordEnabled)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		mTexCoordEnabled = false;
	}

	if (ColorSize)
	{
		glColorPointer(ColorSize, GL_FLOAT, VertexSize, VertexBufferPointer + (PositionSize + TexCoordSize) * sizeof(float));

		if (!mColorEnabled)
		{
			glEnableClientState(GL_COLOR_ARRAY);
			mColorEnabled = true;
		}
	}
	else if (mColorEnabled)
	{
		glDisableClientState(GL_COLOR_ARRAY);
		mColorEnabled = false;
	}
}

void lcContext::ClearIndexBuffer()
{
	if (mIndexBufferObject)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		mIndexBufferObject = 0;
	}
}

void lcContext::SetIndexBuffer(lcIndexBuffer IndexBuffer)
{
	if (GL_HasVertexBufferObject())
	{
		GLuint IndexBufferObject = IndexBuffer;
		mIndexBufferPointer = NULL;

		if (IndexBufferObject != mIndexBufferObject)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, IndexBufferObject);
			mIndexBufferObject = IndexBufferObject;
		}
	}
	else
	{
		mIndexBufferPointer = (char*)IndexBuffer;
	}
}

void lcContext::SetIndexBufferPointer(const void* IndexBuffer)
{
	if (mIndexBufferObject)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		mIndexBufferObject = 0;
	}

	mIndexBufferPointer = (char*)IndexBuffer;
}

void lcContext::BindMesh(lcMesh* Mesh)
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	if (Mesh->mVertexCacheOffset != -1)
	{
		GLuint VertexBufferObject = Library->mVertexBuffer;
		mVertexBufferPointer = (char*)Mesh->mVertexCacheOffset;
		GLuint IndexBufferObject = Library->mIndexBuffer;
		mIndexBufferPointer = (char*)Mesh->mIndexCacheOffset;

		if (VertexBufferObject != mVertexBufferObject)
		{
			glBindBuffer(GL_ARRAY_BUFFER_ARB, VertexBufferObject);
			mVertexBufferObject = VertexBufferObject;
			mVertexBufferOffset = (char*)~0;
		}

		if (IndexBufferObject != mIndexBufferObject)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, IndexBufferObject);
			mIndexBufferObject = IndexBufferObject;
		}
	}
	else
	{
		if (mVertexBufferObject)
		{
			glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
			mVertexBufferObject = 0;
		}

		if (mIndexBufferObject)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			mIndexBufferObject = 0;
		}

		mVertexBufferPointer = (char*)Mesh->mVertexData;
		mIndexBufferPointer = (char*)Mesh->mIndexData;
		mVertexBufferOffset = (char*)~0;
	}
}

void lcContext::UnbindMesh()
{
	if (mTexture)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
		mTexture = NULL;
	}

	if (GL_HasVertexBufferObject())
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		mVertexBufferObject = 0;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		mIndexBufferObject = 0;
	}

	mVertexBufferPointer = NULL;
	mIndexBufferPointer = NULL;

	glVertexPointer(3, GL_FLOAT, 0, NULL);
	glTexCoordPointer(2, GL_FLOAT, 0, NULL);
}

void lcContext::DrawPrimitives(GLenum Mode, GLint First, GLsizei Count)
{
	glDrawArrays(Mode, First, Count);
}

void lcContext::DrawIndexedPrimitives(GLenum Mode, GLsizei Count, GLenum Type, int Offset)
{
	glDrawElements(Mode, Count, Type, mIndexBufferPointer + Offset);
}

void lcContext::DrawMeshSection(lcMesh* Mesh, lcMeshSection* Section)
{
	char* BufferOffset = mVertexBufferPointer;
	lcTexture* Texture = Section->Texture;

	if (!Texture)
	{
		if (mTexture)
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisable(GL_TEXTURE_2D);

			mTexture = NULL;
		}
	}
	else
	{
		BufferOffset += Mesh->mNumVertices * sizeof(lcVertex);

		if (Texture != mTexture)
		{
			glBindTexture(GL_TEXTURE_2D, Texture->mTexture);

			if (!mTexture)
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
				glEnable(GL_TEXTURE_2D);
			}

			mTexture = Texture;
		}
	}

	if (mVertexBufferOffset != BufferOffset)
	{
		if (!Texture)
			glVertexPointer(3, GL_FLOAT, 0, BufferOffset);
		else
		{
			glVertexPointer(3, GL_FLOAT, sizeof(lcVertexTextured), BufferOffset);
			glTexCoordPointer(2, GL_FLOAT, sizeof(lcVertexTextured), BufferOffset + sizeof(lcVector3));
		}

		mVertexBufferOffset = BufferOffset;
	}

	glDrawElements(Section->PrimitiveType, Section->NumIndices, Mesh->mIndexType, mIndexBufferPointer + Section->IndexOffset);
}

void lcContext::DrawOpaqueMeshes(const lcMatrix44& ViewMatrix, const lcArray<lcRenderMesh>& OpaqueMeshes)
{
	bool DrawLines = lcGetPreferences().mDrawEdgeLines;

	lcGetPiecesLibrary()->UpdateBuffers(this); // TODO: find a better place for this update

	for (int MeshIdx = 0; MeshIdx < OpaqueMeshes.GetSize(); MeshIdx++)
	{
		lcRenderMesh& RenderMesh = OpaqueMeshes[MeshIdx];
		lcMesh* Mesh = RenderMesh.Mesh;

		BindMesh(Mesh);
		SetWorldViewMatrix(lcMul(RenderMesh.WorldMatrix, ViewMatrix));

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];
			int ColorIndex = Section->ColorIndex;

			if (Section->PrimitiveType == GL_TRIANGLES)
			{
				if (ColorIndex == gDefaultColor)
					ColorIndex = RenderMesh.ColorIndex;

				if (lcIsColorTranslucent(ColorIndex))
					continue;

				if (RenderMesh.Focused)
				{
					float* Color = gColorList[ColorIndex].Value;
					glColor4fv(lcVector4(Color[0] * 0.5f + 0.4000f * 0.5f, Color[1] * 0.5f + 0.2980f * 0.5f, Color[2] * 0.5f + 0.8980f * 0.5f, Color[3]));
				}
				else if (RenderMesh.Selected)
				{
					float* Color = gColorList[ColorIndex].Value;
					glColor4fv(lcVector4(Color[0] * 0.5f + 0.8980f * 0.5f, Color[1] * 0.5f + 0.2980f * 0.5f, Color[2] * 0.5f + 0.4000f * 0.5f, Color[3]));
				}
				else
					lcSetColor(ColorIndex);
			}
			else
			{
				if (RenderMesh.Focused)
					lcSetColorFocused();
				else if (RenderMesh.Selected)
					lcSetColorSelected();
				else if (DrawLines)
				{
					if (ColorIndex == gEdgeColor)
						lcSetEdgeColor(RenderMesh.ColorIndex);
					else
						lcSetColor(ColorIndex);
				}
				else
					continue;
			}

			DrawMeshSection(Mesh, Section);
		}
	}
}

void lcContext::DrawTranslucentMeshes(const lcMatrix44& ViewMatrix, const lcArray<lcRenderMesh>& TranslucentMeshes)
{
	if (TranslucentMeshes.IsEmpty())
		return;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);

	for (int MeshIdx = 0; MeshIdx < TranslucentMeshes.GetSize(); MeshIdx++)
	{
		lcRenderMesh& RenderMesh = TranslucentMeshes[MeshIdx];
		lcMesh* Mesh = RenderMesh.Mesh;

		BindMesh(Mesh);
		SetWorldViewMatrix(lcMul(RenderMesh.WorldMatrix, ViewMatrix));

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];
			int ColorIndex = Section->ColorIndex;

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			if (ColorIndex == gDefaultColor)
				ColorIndex = RenderMesh.ColorIndex;

			if (!lcIsColorTranslucent(ColorIndex))
				continue;

			if (RenderMesh.Focused)
			{
				float* Color = gColorList[ColorIndex].Value;
				glColor4fv(lcVector4(Color[0] * 0.5f + 0.4000f * 0.5f, Color[1] * 0.5f + 0.2980f * 0.5f, Color[2] * 0.5f + 0.8980f * 0.5f, Color[3]));
			}
			else if (RenderMesh.Selected)
			{
				float* Color = gColorList[ColorIndex].Value;
				glColor4fv(lcVector4(Color[0] * 0.5f + 0.8980f * 0.5f, Color[1] * 0.5f + 0.2980f * 0.5f, Color[2] * 0.5f + 0.4000f * 0.5f, Color[3]));
			}
			else
				lcSetColor(ColorIndex);

			DrawMeshSection(Mesh, Section);
		}
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void lcContext::DrawInterfaceObjects(const lcMatrix44& ViewMatrix, const lcArray<lcObject*>& InterfaceObjects)
{
	for (int ObjectIdx = 0; ObjectIdx < InterfaceObjects.GetSize(); ObjectIdx++)
		InterfaceObjects[ObjectIdx]->DrawInterface(this, ViewMatrix);
}

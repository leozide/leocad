#include "lc_global.h"
#include "lc_context.h"
#include "lc_mesh.h"
#include "lc_texture.h"
#include "lc_colors.h"
#include "lc_mainwindow.h"

lcContext::lcContext()
{
	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = NULL;
	mIndexBufferPointer = NULL;
	mVertexBufferOffset = (char*)~0;

	mTexture = NULL;
	mLineWidth = 1.0f;
	mMatrixMode = GL_MODELVIEW;

	mFramebufferObject = 0;
	mFramebufferTexture = 0;
	mDepthRenderbufferObject = 0;
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

	if (GL_HasVertexBufferObject())
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}

	glVertexPointer(3, GL_FLOAT, 0, NULL);
	glTexCoordPointer(2, GL_FLOAT, 0, NULL);

	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = NULL;
	mIndexBufferPointer = NULL;
	mVertexBufferOffset = (char*)~0;

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
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
		QMessageBox::information(gMainWindow->mHandle, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));

	free(Buffer);

	return Result;
}

void lcContext::BindMesh(lcMesh* Mesh)
{
	if (GL_HasVertexBufferObject())
	{
		GLuint VertexBufferObject = Mesh->mVertexBuffer.mBuffer;
		mVertexBufferPointer = NULL;
		GLuint IndexBufferObject = Mesh->mIndexBuffer.mBuffer;
		mIndexBufferPointer = NULL;

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
		mVertexBufferPointer = (char*)Mesh->mVertexBuffer.mData;
		mIndexBufferPointer = (char*)Mesh->mIndexBuffer.mData;
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
				else if (ColorIndex == gEdgeColor)
					lcSetEdgeColor(RenderMesh.ColorIndex);
				else
					lcSetColor(ColorIndex);
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

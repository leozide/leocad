#include "lc_global.h"
#include "lc_context.h"
#include "lc_mesh.h"
#include "lc_texture.h"

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
}

lcContext::~lcContext()
{
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

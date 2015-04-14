#ifndef _LC_CONTEXT_H_
#define _LC_CONTEXT_H_

#include "lc_array.h"
#include "lc_math.h"

class lcScene
{
public:
	lcScene();

	void Begin(const lcMatrix44& ViewMatrix);
	void End();

	lcMatrix44 mViewMatrix;
	lcArray<lcRenderMesh> mOpaqueMeshes;
	lcArray<lcRenderMesh> mTranslucentMeshes;
	lcArray<lcObject*> mInterfaceObjects;
};

class lcContext
{
public:
	lcContext();
	~lcContext();

	int GetViewportWidth() const
	{
		return mViewportWidth;
	}

	int GetViewportHeight() const
	{
		return mViewportHeight;
	}

	void SetDefaultState();

	void SetViewport(int x, int y, int Width, int Height);
	void SetWorldViewMatrix(const lcMatrix44& WorldViewMatrix);
	void SetProjectionMatrix(const lcMatrix44& ProjectionMatrix);
//	void SetColor(const lcVector4& Color);
	void SetLineWidth(float LineWidth);

	bool BeginRenderToTexture(int Width, int Height);
	void EndRenderToTexture();
	bool SaveRenderToTextureImage(const QString& FileName, int Width, int Height);

	void ClearVertexBuffer();
	void SetVertexBuffer(const lcVertexBuffer* VertexBuffer);
	void SetVertexBufferPointer(const void* VertexBuffer);
	void SetVertexFormat(int BufferOffset, int PositionSize, int TexCoordSize, int ColorSize);
	void DrawPrimitives(GLenum Mode, GLint First, GLsizei Count);

	void BindMesh(lcMesh* Mesh);
	void UnbindMesh();
	void DrawMeshSection(lcMesh* Mesh, lcMeshSection* Section);
	void DrawOpaqueMeshes(const lcMatrix44& ViewMatrix, const lcArray<lcRenderMesh>& OpaqueMeshes);
	void DrawTranslucentMeshes(const lcMatrix44& ViewMatrix, const lcArray<lcRenderMesh>& TranslucentMeshes);
	void DrawInterfaceObjects(const lcMatrix44& ViewMatrix, const lcArray<lcObject*>& InterfaceObjects);

protected:
	GLuint mVertexBufferObject;
	GLuint mIndexBufferObject;
	char* mVertexBufferPointer;
	char* mIndexBufferPointer;
	char* mVertexBufferOffset;

	bool mTexCoordEnabled;
	bool mColorEnabled;

	lcTexture* mTexture;
	float mLineWidth;
	int mMatrixMode;

	int mViewportX;
	int mViewportY;
	int mViewportWidth;
	int mViewportHeight;

	GLuint mFramebufferObject;
	GLuint mFramebufferTexture;
	GLuint mDepthRenderbufferObject;

	Q_DECLARE_TR_FUNCTIONS(lcContext);
};

#endif // _LC_CONTEXT_H_

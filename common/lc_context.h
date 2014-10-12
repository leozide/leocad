#ifndef _LC_CONTEXT_H_
#define _LC_CONTEXT_H_

#include "lc_array.h"

class lcContext
{
public:
	lcContext();
	~lcContext();

	void SetDefaultState();

	void SetWorldViewMatrix(const lcMatrix44& WorldViewMatrix);
	void SetProjectionMatrix(const lcMatrix44& ProjectionMatrix);
//	void SetColor(const lcVector4& Color);
	void SetLineWidth(float LineWidth);

	bool BeginRenderToTexture(int Width, int Height);
	void EndRenderToTexture();
	bool SaveRenderToTextureImage(const QString& FileName, int Width, int Height);

	void BindMesh(lcMesh* Mesh);
	void UnbindMesh();
	void DrawMeshSection(lcMesh* Mesh, lcMeshSection* Section);
	void DrawOpaqueMeshes(const lcMatrix44& ViewMatrix, const lcArray<lcRenderMesh>& OpaqueMeshes);
	void DrawTranslucentMeshes(const lcMatrix44& ViewMatrix, const lcArray<lcRenderMesh>& TranslucentMeshes);

protected:
	GLuint mVertexBufferObject;
	GLuint mIndexBufferObject;
	char* mVertexBufferPointer;
	char* mIndexBufferPointer;
	char* mVertexBufferOffset;

	lcTexture* mTexture;
	float mLineWidth;
	int mMatrixMode;

	GLuint mFramebufferObject;
	GLuint mFramebufferTexture;
	GLuint mDepthRenderbufferObject;

	Q_DECLARE_TR_FUNCTIONS(lcContext);
};

#endif // _LC_CONTEXT_H_

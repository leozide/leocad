#ifndef _LC_CONTEXT_H_
#define _LC_CONTEXT_H_

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

	void BindMesh(lcMesh* Mesh);
	void UnbindMesh();
	void DrawMeshSection(lcMesh* Mesh, lcMeshSection* Section);

protected:
	GLuint mVertexBufferObject;
	GLuint mIndexBufferObject;
	char* mVertexBufferPointer;
	char* mIndexBufferPointer;
	char* mVertexBufferOffset;

	lcTexture* mTexture;
	float mLineWidth;
	int mMatrixMode;
};

#endif // _LC_CONTEXT_H_

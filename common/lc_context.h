#ifndef _LC_CONTEXT_H_
#define _LC_CONTEXT_H_

#include "lc_array.h"
#include "lc_math.h"
#include "lc_colors.h"
#include "lc_mesh.h"

class lcScene
{
public:
	lcScene();

	void Begin(const lcMatrix44& ViewMatrix);
	void End();
	void AddMesh(lcMesh* Mesh, const lcMatrix44& WorldMatrix, int ColorIndex, lcRenderMeshState State, int Flags);

	lcMatrix44 mViewMatrix;
	lcArray<lcRenderMesh> mRenderMeshes;
	lcArray<int> mOpaqueMeshes;
	lcArray<int> mTranslucentMeshes;
	lcArray<const lcObject*> mInterfaceObjects;
	bool mHasTexture;
};

class lcVertexBuffer
{
public:
	lcVertexBuffer()
		: Pointer(NULL)
	{
	}

	bool IsValid() const
	{
		return Pointer != NULL;
	}

	union
	{
		GLuint Object;
		void* Pointer;
	};
};

class lcIndexBuffer
{
public:
	lcIndexBuffer()
		: Pointer(NULL)
	{
	}

	bool IsValid() const
	{
		return Pointer != NULL;
	}

	union
	{
		GLuint Object;
		void* Pointer;
	};
};

enum lcMaterialType
{
	LC_MATERIAL_UNLIT_COLOR,
	LC_MATERIAL_UNLIT_TEXTURE_MODULATE,
	LC_MATERIAL_UNLIT_TEXTURE_DECAL,
	LC_MATERIAL_UNLIT_VERTEX_COLOR,
	LC_MATERIAL_FAKELIT_COLOR,
	LC_MATERIAL_FAKELIT_TEXTURE_DECAL,
	LC_NUM_MATERIALS
};

enum lcProgramAttrib
{
	LC_ATTRIB_POSITION,
	LC_ATTRIB_NORMAL,
	LC_ATTRIB_TEXCOORD,
	LC_ATTRIB_COLOR
};

struct lcProgram
{
	GLuint Object;
	GLint WorldViewProjectionMatrixLocation;
	GLint WorldMatrixLocation;
	GLint MaterialColorLocation;
	GLint LightPositionLocation;
	GLint EyePositionLocation;
};

class lcContext
{
public:
	lcContext();
	~lcContext();

	static void CreateResources();
	static void DestroyResources();

	void SetDefaultState();
	void ClearResources();

	void SetWorldMatrix(const lcMatrix44& WorldMatrix)
	{
		mWorldMatrix = WorldMatrix;
		mWorldMatrixDirty = true;
	}

	void SetViewMatrix(const lcMatrix44& ViewMatrix)
	{
		mViewMatrix = ViewMatrix;
		mViewMatrixDirty = true;
		mViewProjectionMatrixDirty = true;
	}

	void SetProjectionMatrix(const lcMatrix44& ProjectionMatrix)
	{
		mProjectionMatrix = ProjectionMatrix;
		mProjectionMatrixDirty = true;
		mViewProjectionMatrixDirty = true;
	}

	void SetMaterial(lcMaterialType MaterialType);
	void SetViewport(int x, int y, int Width, int Height);
	void SetLineWidth(float LineWidth);
	void SetTexture(GLuint Texture);

	void SetColor(const lcVector4& Color)
	{
		mColor = Color;
		mColorDirty = true;
	}

	void SetColor(float Red, float Green, float Blue, float Alpha);
	void SetColorIndex(int ColorIndex);
	void SetColorIndexTinted(int ColorIndex, lcInterfaceColor InterfaceColor);
	void SetEdgeColorIndex(int ColorIndex);
	void SetInterfaceColor(lcInterfaceColor InterfaceColor);

	bool BeginRenderToTexture(int Width, int Height);
	void EndRenderToTexture();
	QImage GetRenderToTextureImage(int Width, int Height);
	bool SaveRenderToTextureImage(const QString& FileName, int Width, int Height);

	lcVertexBuffer CreateVertexBuffer(int Size, const void* Data);
	void DestroyVertexBuffer(lcVertexBuffer& VertexBuffer);
	lcIndexBuffer CreateIndexBuffer(int Size, const void* Data);
	void DestroyIndexBuffer(lcIndexBuffer& IndexBuffer);

	void ClearVertexBuffer();
	void SetVertexBuffer(lcVertexBuffer VertexBuffer);
	void SetVertexBufferPointer(const void* VertexBuffer);

	void ClearIndexBuffer();
	void SetIndexBuffer(lcIndexBuffer IndexBuffer);
	void SetIndexBufferPointer(const void* IndexBuffer);

	void SetVertexFormat(int BufferOffset, int PositionSize, int NormalSize, int TexCoordSize, int ColorSize, bool EnableNormals);
	void SetVertexFormatPosition(int PositionSize);
	void DrawPrimitives(GLenum Mode, GLint First, GLsizei Count);
	void DrawIndexedPrimitives(GLenum Mode, GLsizei Count, GLenum Type, int Offset);

	void DrawScene(const lcScene& Scene);
	void DrawInterfaceObjects(const lcArray<const lcObject*>& InterfaceObjects);

protected:
	static void CreateShaderPrograms();
	void BindMesh(lcMesh* Mesh);
	void FlushState();

	void DrawMeshSection(lcMesh* Mesh, lcMeshSection* Section);
	void DrawOpaqueMeshes(const lcScene& Scene);
	void DrawTranslucentMeshes(const lcScene& Scene);
	void DrawRenderMeshes(const lcArray<lcRenderMesh>& RenderMeshes, const lcArray<int>& Meshes, lcMeshPrimitiveType PrimitiveType, bool EnableNormals, bool DrawTranslucent, bool DrawTextured);

	GLuint mVertexBufferObject;
	GLuint mIndexBufferObject;
	char* mVertexBufferPointer;
	char* mIndexBufferPointer;
	char* mVertexBufferOffset;

	lcMaterialType mMaterialType;
	bool mNormalEnabled;
	bool mTexCoordEnabled;
	bool mColorEnabled;

	GLuint mTexture;
	float mLineWidth;
	int mMatrixMode;
	bool mTextureEnabled;

	lcVector4 mColor;
	lcMatrix44 mWorldMatrix;
	lcMatrix44 mViewMatrix;
	lcMatrix44 mProjectionMatrix;
	lcMatrix44 mViewProjectionMatrix;
	bool mColorDirty;
	bool mWorldMatrixDirty;
	bool mViewMatrixDirty;
	bool mProjectionMatrixDirty;
	bool mViewProjectionMatrixDirty;

	GLuint mFramebufferObject;
	GLuint mFramebufferTexture;
	GLuint mDepthRenderbufferObject;

	static lcProgram mPrograms[LC_NUM_MATERIALS];

	Q_DECLARE_TR_FUNCTIONS(lcContext);
};

#endif // _LC_CONTEXT_H_

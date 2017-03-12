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

	lcMatrix44 mViewMatrix;
	lcArray<lcRenderMesh> mOpaqueMeshes;
	lcArray<lcRenderMesh> mTranslucentMeshes;
	lcArray<const lcObject*> mInterfaceObjects;
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

enum lcLightingMode : int
{
	LC_LIGHTING_UNLIT,
	LC_LIGHTING_FAKE,
	LC_LIGHTING_FULL,
	LC_NUM_LIGHTING_MODES
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

enum lcTextureMode
{
	LC_TEXTURE_DECAL,
	LC_TEXTURE_MODULATE
};

class lcContext
{
public:
	lcContext();
	~lcContext();

	static void CreateResources();
	static void DestroyResources();

	void SetDefaultState();

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

	void SetLightingMode(lcLightingMode LightingMode);
	void SetMaterial(lcMaterialType MaterialType);
	void SetViewport(int x, int y, int Width, int Height);
	void SetLineWidth(float LineWidth);
	void SetTextureMode(lcTextureMode TextureMode);

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

	void SetVertexFormat(int BufferOffset, int PositionSize, int NormalSize, int TexCoordSize, int ColorSize);
	void DrawPrimitives(GLenum Mode, GLint First, GLsizei Count);
	void DrawIndexedPrimitives(GLenum Mode, GLsizei Count, GLenum Type, int Offset);

	void UnbindMesh();
	void DrawMeshSection(lcMesh* Mesh, lcMeshSection* Section);
	void DrawOpaqueMeshes(const lcArray<lcRenderMesh>& OpaqueMeshes);
	void DrawTranslucentMeshes(const lcArray<lcRenderMesh>& TranslucentMeshes);
	void DrawInterfaceObjects(const lcArray<const lcObject*>& InterfaceObjects);

protected:
	static void CreateShaderPrograms();
	void BindMesh(lcMesh* Mesh);
	void FlushState();

	GLuint mVertexBufferObject;
	GLuint mIndexBufferObject;
	char* mVertexBufferPointer;
	char* mIndexBufferPointer;
	char* mVertexBufferOffset;

	lcLightingMode mLightingMode;
	lcMaterialType mMaterialType;
	bool mNormalEnabled;
	bool mTexCoordEnabled;
	bool mColorEnabled;

	lcTexture* mTexture;
	float mLineWidth;
	int mMatrixMode;

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

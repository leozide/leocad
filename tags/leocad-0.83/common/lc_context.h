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

enum lcProgramType
{
	LC_PROGRAM_SIMPLE,
	LC_PROGRAM_TEXTURE,
	LC_PROGRAM_VERTEX_COLOR,
	LC_NUM_PROGRAMS
};

enum lcProgramAttrib
{
	LC_ATTRIB_POSITION, 
	LC_ATTRIB_TEXCOORD, 
	LC_ATTRIB_COLOR
};

struct lcProgram
{
	GLuint Object;
	GLint MatrixLocation;
	GLint ColorLocation;
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

	void SetProgram(lcProgramType ProgramType);
	void SetViewport(int x, int y, int Width, int Height);
	void SetLineWidth(float LineWidth);

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

	void SetVertexFormat(int BufferOffset, int PositionSize, int TexCoordSize, int ColorSize);
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

	lcProgramType mProgramType; 
	bool mTexCoordEnabled;
	bool mColorEnabled;

	lcTexture* mTexture;
	float mLineWidth;
	int mMatrixMode;

	int mViewportX;
	int mViewportY;
	int mViewportWidth;
	int mViewportHeight;

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

	static lcProgram mPrograms[LC_NUM_PROGRAMS];

	Q_DECLARE_TR_FUNCTIONS(lcContext);
};

#endif // _LC_CONTEXT_H_

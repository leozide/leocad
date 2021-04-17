#pragma once

#include "lc_array.h"
#include "lc_math.h"
#include "lc_colors.h"
#include "lc_mesh.h"

class lcVertexBuffer
{
public:
	constexpr lcVertexBuffer()
		: Pointer(nullptr)
	{
	}

	bool IsValid() const
	{
		return Pointer != nullptr;
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
	constexpr lcIndexBuffer()
		: Pointer(nullptr)
	{
	}

	bool IsValid() const
	{
		return Pointer != nullptr;
	}

	union
	{
		GLuint Object;
		void* Pointer;
	};
};

enum class lcMaterialType
{
	UnlitColor,
	UnlitColorConditional,
	UnlitTextureModulate,
	UnlitTextureDecal,
	UnlitVertexColor,
	UnlitViewSphere,
	FakeLitColor,
	FakeLitTextureDecal,
	Count
};

enum class lcProgramAttrib
{
	Position,
	Normal,
	TexCoord,
	Color,
	ControlPoint1 = 0,
	ControlPoint2,
	ControlPoint3,
	ControlPoint4,
	Count
};

struct lcProgram
{
	GLuint Object;
	GLint WorldViewProjectionMatrixLocation;
	GLint WorldMatrixLocation;
	GLint MaterialColorLocation;
	GLint LightPositionLocation;
	GLint EyePositionLocation;
	GLint HighlightParamsLocation;
};

enum class lcPolygonOffset
{
	None,
	Opaque,
	Translucent
};

enum class lcDepthFunction
{
	LessEqual,
	Always
};

struct lcVertexAttribState
{
	GLint Size = 0;
	GLenum Type = 0;
	GLboolean Normalized = 0;
	bool Enabled = 0;
	GLsizei Stride = 0;
	const void* Pointer = nullptr;
	GLuint VertexBufferObject = 0;
};

class lcContext : protected QOpenGLFunctions
{
public:
	lcContext();
	~lcContext();

	lcContext(const lcContext&) = delete;
	lcContext& operator=(const lcContext&) = delete;

	static bool InitializeRenderer();
	static void ShutdownRenderer();
	static lcContext* GetGlobalOffscreenContext();

	void CreateResources();
	void DestroyResources();

	void SetDefaultState();
	void ClearResources();

	void MakeCurrent();

	void SetGLContext(QOpenGLContext* GLContext, QOpenGLWidget* Widget);
	void SetOffscreenContext();

	void ClearColorAndDepth(const lcVector4& ClearColor);
	void ClearDepth();

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

	const lcMatrix44& GetProjectionMatrix()
	{
		return mProjectionMatrix;
	}

	void SetMaterial(lcMaterialType MaterialType);
	void SetViewport(int x, int y, int Width, int Height);
	void SetPolygonOffset(lcPolygonOffset PolygonOffset);
	void SetDepthWrite(bool Enable);
	void SetDepthFunction(lcDepthFunction DepthFunction);
	void EnableCullFace(bool Enable);
	void SetLineWidth(float LineWidth);
	void SetSmoothShading(bool Smooth);
	void BindTexture2D(GLuint Texture);
	void BindTextureCubeMap(GLuint Texture);
	void UnbindTexture2D(GLuint Texture);
	void UnbindTextureCubeMap(GLuint Texture);

	void SetColor(const lcVector4& Color)
	{
		mColor = Color;
		mColorDirty = true;
	}

	void SetHighlightParams(const lcVector4& HighlightPosition, const lcVector4& TextColor, const lcVector4& BackgroundColor, const lcVector4& HighlightColor)
	{
		mHighlightParams[0] = HighlightPosition;
		mHighlightParams[1] = TextColor;
		mHighlightParams[2] = BackgroundColor;
		mHighlightParams[3] = HighlightColor;
		mHighlightParamsDirty = true;
	}

	void SetColor(float Red, float Green, float Blue, float Alpha);
	void SetColorIndex(int ColorIndex);
	void SetColorIndexTinted(int ColorIndex, lcInterfaceColor InterfaceColor, float Weight);
	void SetColorIndexTinted(int ColorIndex, const lcVector4& Tint);
	void SetEdgeColorIndex(int ColorIndex);
	void SetEdgeColorIndexTinted(int ColorIndex, const lcVector4& Tint);
	void SetInterfaceColor(lcInterfaceColor InterfaceColor);

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
	void SetVertexFormatConditional(int BufferOffset);
	void DrawPrimitives(GLenum Mode, GLint First, GLsizei Count);
	void DrawIndexedPrimitives(GLenum Mode, GLsizei Count, GLenum Type, int Offset);

	void BindMesh(const lcMesh* Mesh);

protected:
	static bool CreateOffscreenContext();
	static void DestroyOffscreenContext();

	void CreateShaderPrograms();
	void FlushState();

	void SetVertexAttribPointer(lcProgramAttrib Attrib, GLint Size, GLenum Type, GLboolean Normalized, GLsizei Stride, const void* Pointer);
	void EnableVertexAttrib(lcProgramAttrib Attrib);
	void DisableVertexAttrib(lcProgramAttrib Attrib);

	QOpenGLWidget* mWidget = nullptr;
	QOpenGLContext* mContext = nullptr;

	GLuint mVertexBufferObject;
	GLuint mIndexBufferObject;
	const char* mVertexBufferPointer;
	const char* mIndexBufferPointer;
	const char* mVertexBufferOffset;

	lcMaterialType mMaterialType;
	bool mNormalEnabled;
	bool mTexCoordEnabled;
	bool mColorEnabled;

	lcVertexAttribState mVertexAttribState[static_cast<int>(lcProgramAttrib::Count)];

	GLuint mTexture2D;
	GLuint mTextureCubeMap;
	lcPolygonOffset mPolygonOffset;
	bool mDepthWrite;
	lcDepthFunction mDepthFunction;
	bool mCullFace;
	float mLineWidth;
	int mMatrixMode;
	bool mTextureEnabled;

	lcVector4 mColor;
	lcMatrix44 mWorldMatrix;
	lcMatrix44 mViewMatrix;
	lcMatrix44 mProjectionMatrix;
	lcMatrix44 mViewProjectionMatrix;
	lcVector4 mHighlightParams[4];
	bool mColorDirty;
	bool mWorldMatrixDirty;
	bool mViewMatrixDirty;
	bool mProjectionMatrixDirty;
	bool mViewProjectionMatrixDirty;
	bool mHighlightParamsDirty;

	static std::unique_ptr<QOpenGLContext> mOffscreenContext;
	static std::unique_ptr<QOffscreenSurface> mOffscreenSurface;
	static std::unique_ptr<lcContext> mGlobalOffscreenContext;

	static lcProgram mPrograms[static_cast<int>(lcMaterialType::Count)];

	Q_DECLARE_TR_FUNCTIONS(lcContext);
};


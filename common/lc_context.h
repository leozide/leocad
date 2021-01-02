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
	UnlitTextureModulate,
	UnlitTextureDecal,
	UnlitVertexColor,
	UnlitViewSphere,
	FakeLitColor,
	FakeLitTextureDecal,
	Count
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
	GLint HighlightParamsLocation;
};

class lcFramebuffer
{
public:
	lcFramebuffer()
	{
	}

	lcFramebuffer(int Width, int Height)
		: mWidth(Width), mHeight(Height)
	{
	}

	bool IsValid() const
	{
		return mObject != 0;
	}

	GLuint mObject = 0;
	GLuint mColorTexture = 0;
	GLuint mDepthRenderbuffer = 0;
	int mWidth = 0;
	int mHeight = 0;
};

enum class lcPolygonOffset
{
	None,
	Opaque,
	Translucent
};

#ifdef LC_USE_QOPENGLWIDGET
class lcContext : protected QOpenGLFunctions
#else
class lcContext
#endif
{
public:
	lcContext();
	~lcContext();

	lcContext(const lcContext&) = delete;
	lcContext& operator=(const lcContext&) = delete;

#ifdef LC_USE_QOPENGLWIDGET
	static bool CreateOffscreenContext();
	static void DestroyOffscreenContext();
#endif

	void CreateResources();
	void DestroyResources();

	void SetDefaultState();
	void ClearResources();

	void MakeCurrent();
#ifdef LC_USE_QOPENGLWIDGET
	void SetGLContext(QOpenGLContext* GLContext, QOpenGLWidget* Widget);
	void SetOffscreenContext();
#else
	void SetGLContext(const QGLContext* GLContext, QGLWidget* Widget);
#endif

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
	void SetLineWidth(float LineWidth);
	void SetSmoothShading(bool Smooth);
	void BindTexture2D(GLuint Texture);
	void BindTexture2DMS(GLuint Texture);
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

	void ClearFramebuffer();
	lcFramebuffer CreateFramebuffer(int Width, int Height, bool Depth, bool Multisample);
	void DestroyFramebuffer(lcFramebuffer& Framebuffer);
	void BindFramebuffer(GLuint FramebufferObject);
	void BindFramebuffer(const lcFramebuffer& Framebuffer)
	{
		BindFramebuffer(Framebuffer.mObject);
	}

	std::pair<lcFramebuffer, lcFramebuffer> CreateRenderFramebuffer(int Width, int Height);
	void DestroyRenderFramebuffer(std::pair<lcFramebuffer, lcFramebuffer>& RenderFramebuffer);
	QImage GetRenderFramebufferImage(const std::pair<lcFramebuffer, lcFramebuffer>& RenderFramebuffer);
	void GetRenderFramebufferImage(const std::pair<lcFramebuffer, lcFramebuffer>& RenderFramebuffer, quint8* Buffer);

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

	void BindMesh(const lcMesh* Mesh);

protected:
	void CreateShaderPrograms();
	void FlushState();

#ifdef LC_USE_QOPENGLWIDGET
	QOpenGLWidget* mWidget = nullptr;
	QOpenGLContext* mContext = nullptr;
#endif
	bool mValid = false;

	GLuint mVertexBufferObject;
	GLuint mIndexBufferObject;
	char* mVertexBufferPointer;
	char* mIndexBufferPointer;
	char* mVertexBufferOffset;

	lcMaterialType mMaterialType;
	bool mNormalEnabled;
	bool mTexCoordEnabled;
	bool mColorEnabled;

	GLuint mTexture2D;
	GLuint mTexture2DMS;
	GLuint mTextureCubeMap;
	lcPolygonOffset mPolygonOffset;
	bool mDepthWrite;
	float mLineWidth;
	int mMatrixMode;
	bool mTextureEnabled;

	lcVector4 mColor;
	lcMatrix44 mWorldMatrix;
	lcMatrix44 mViewMatrix;
	lcMatrix44 mProjectionMatrix;
	lcMatrix44 mViewProjectionMatrix;
	lcVector4 mHighlightParams[5];
	bool mColorDirty;
	bool mWorldMatrixDirty;
	bool mViewMatrixDirty;
	bool mProjectionMatrixDirty;
	bool mViewProjectionMatrixDirty;
	bool mHighlightParamsDirty;

	GLuint mFramebufferObject;

#ifdef LC_USE_QOPENGLWIDGET
	static std::unique_ptr<QOpenGLContext> mOffscreenContext;
	static std::unique_ptr<QOffscreenSurface> mOffscreenSurface;
#endif

	static lcProgram mPrograms[static_cast<int>(lcMaterialType::Count)];
	static int mValidContexts;

	Q_DECLARE_TR_FUNCTIONS(lcContext);
};


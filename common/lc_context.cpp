#include "lc_global.h"
#include "lc_context.h"
#include "lc_glextensions.h"
#include "lc_mesh.h"
#include "lc_texture.h"
#include "lc_colors.h"
#include "lc_mainwindow.h"
#include "lc_library.h"
#include "texfont.h"
#include "lc_view.h"
#include "lc_viewsphere.h"
#include "lc_viewmanipulator.h"
#include "lc_stringcache.h"
#include "lc_partselectionwidget.h"
#include <QOpenGLFunctions_3_2_Core>

#ifdef LC_OPENGLES
#define glEnableClientState(...)
#define glDisableClientState(...)
#define glVertexPointer(...)
#define glTexCoordPointer(...)
#define glColorPointer(...)
#define GL_ARRAY_BUFFER_ARB GL_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER_ARB GL_ELEMENT_ARRAY_BUFFER
#define GL_STATIC_DRAW_ARB GL_STATIC_DRAW
#endif

std::unique_ptr<QOpenGLContext> lcContext::mOffscreenContext;
std::unique_ptr<QOffscreenSurface> lcContext::mOffscreenSurface;
std::unique_ptr<lcContext> lcContext::mGlobalOffscreenContext;
lcProgram lcContext::mPrograms[static_cast<int>(lcMaterialType::Count)];

lcContext::lcContext()
{
	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = nullptr;
	mIndexBufferPointer = nullptr;
	mVertexBufferOffset = (char*)~0;

	mNormalEnabled = false;
	mTexCoordEnabled = false;
	mColorEnabled = false;

	mTexture2D = 0;
	mTextureCubeMap = 0;
	mPolygonOffset = lcPolygonOffset::None;
	mDepthWrite = true;
	mDepthFunction = lcDepthFunction::LessEqual;
	mDepthTest = true;
	mColorWrite = true;
	mColorBlend = false;
	mCullFace = false;
	mLineWidth = 1.0f;
#if LC_FIXED_FUNCTION
	mMatrixMode = GL_MODELVIEW;
	mTextureEnabled = false;
#endif

	mColor = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mWorldMatrix = lcMatrix44Identity();
	mViewMatrix = lcMatrix44Identity();
	mProjectionMatrix = lcMatrix44Identity();
	mViewProjectionMatrix = lcMatrix44Identity();
	mHighlightParams[0] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mHighlightParams[1] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mHighlightParams[2] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mHighlightParams[3] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mColorDirty = false;
	mWorldMatrixDirty = false;
	mViewMatrixDirty = false;
	mProjectionMatrixDirty = false;
	mViewProjectionMatrixDirty = false;
	mHighlightParamsDirty = false;

	mMaterialType = lcMaterialType::Count;
}

lcContext::~lcContext()
{
}

bool lcContext::InitializeRenderer()
{
	if (!CreateOffscreenContext())
		return false;

	mGlobalOffscreenContext = std::unique_ptr<lcContext>(new(lcContext));

	lcContext* Context = mGlobalOffscreenContext.get();
	Context->SetOffscreenContext();

	lcInitializeGLExtensions(mOffscreenContext.get());

	gStringCache.Initialize(Context);
	gTexFont.Initialize(Context);

	Context->CreateResources();
	lcView::CreateResources(Context);
	lcViewManipulator::CreateResources(Context);
	lcViewSphere::CreateResources(Context);

	if (!gSupportsShaderObjects && lcGetPreferences().mShadingMode == lcShadingMode::DefaultLights)
		lcGetPreferences().mShadingMode = lcShadingMode::Flat;

	if (!gSupportsShaderObjects && lcGetPreferences().mDrawConditionalLines)
		lcGetPreferences().mDrawConditionalLines = false;

	if (!gSupportsFramebufferObject)
		gMainWindow->GetPartSelectionWidget()->DisableIconMode();

	return true;
}

void lcContext::ShutdownRenderer()
{
	if (!mGlobalOffscreenContext)
		return;

	mGlobalOffscreenContext->MakeCurrent();
	lcContext* Context = mGlobalOffscreenContext.get();

	gStringCache.Reset();
	gTexFont.Reset();

	lcView::DestroyResources(Context);
	Context->DestroyResources();
	lcViewManipulator::DestroyResources(Context);
	lcViewSphere::DestroyResources(Context);

	mGlobalOffscreenContext.reset();

	lcContext::DestroyOffscreenContext();
}

lcContext* lcContext::GetGlobalOffscreenContext()
{
	return mGlobalOffscreenContext.get();
}

bool lcContext::CreateOffscreenContext()
{
	std::unique_ptr<QOpenGLContext> OffscreenContext(new QOpenGLContext());

	if (!OffscreenContext)
		return false;

	OffscreenContext->setShareContext(QOpenGLContext::globalShareContext());

	if (!OffscreenContext->create() || !OffscreenContext->isValid())
		return false;

	std::unique_ptr<QOffscreenSurface> OffscreenSurface(new QOffscreenSurface());

	if (!OffscreenSurface)
		return false;

	OffscreenSurface->create();

	if (!OffscreenSurface->isValid())
		return false;

	if (!OffscreenContext->makeCurrent(OffscreenSurface.get()))
		return false;

	mOffscreenContext = std::move(OffscreenContext);
	mOffscreenSurface = std::move(OffscreenSurface);

	return true;
}

void lcContext::DestroyOffscreenContext()
{
	mOffscreenSurface.reset();
	mOffscreenContext.reset();
}

void lcContext::CreateShaderPrograms()
{
	const char* ShaderPrefix =
	{
#ifndef LC_OPENGLES
"#version 110\n"
"#define mediump\n"
"#define LC_VERTEX_INPUT attribute\n"
"#define LC_VERTEX_OUTPUT varying\n"
"#define LC_PIXEL_INPUT varying\n"
"#define LC_PIXEL_OUTPUT\n"
"#define LC_SHADER_PRECISION\n"
#else
"#version 300 es\n"
"#define texture2D texture\n"
"#define LC_VERTEX_INPUT in\n"
"#define LC_VERTEX_OUTPUT out\n"
"#define LC_PIXEL_INPUT in mediump\n"
"#define gl_FragColor FragColor\n"
"#define LC_PIXEL_OUTPUT out mediump vec4 gl_FragColor;\n"
"#define LC_SHADER_PRECISION mediump\n"
#endif

"#define LC_PIXEL_FAKE_LIGHTING \\\n"
"		LC_SHADER_PRECISION vec3 Normal = normalize(PixelNormal); \\\n"
"		LC_SHADER_PRECISION vec3 LightDirection = normalize(PixelPosition - LightPosition); \\\n"
"		LC_SHADER_PRECISION vec3 VertexToEye = normalize(EyePosition - PixelPosition); \\\n"
"		LC_SHADER_PRECISION vec3 LightReflect = normalize(reflect(-LightDirection, Normal)); \\\n"
"		LC_SHADER_PRECISION float Specular = abs(dot(VertexToEye, LightReflect)); \\\n"
"		Specular = min(pow(Specular, 8.0), 1.0) * 0.25; \\\n"
"		LC_SHADER_PRECISION vec3 SpecularColor = vec3(Specular, Specular, Specular); \\\n"
"		LC_SHADER_PRECISION float Diffuse = min(abs(dot(Normal, LightDirection)) * 0.6 + 0.65, 1.0);\n"
	};

	const char* const VertexShaders[] =
	{
		":/resources/shaders/unlit_color_vs.glsl",             // UnlitColor
		":/resources/shaders/unlit_color_conditional_vs.glsl", // UnlitColorConditional
		":/resources/shaders/unlit_texture_modulate_vs.glsl",  // UnlitTextureModulate
		":/resources/shaders/unlit_texture_decal_vs.glsl",     // UnlitTextureDecal
		":/resources/shaders/unlit_vertex_color_vs.glsl",      // UnlitVertexColor
		":/resources/shaders/unlit_view_sphere_vs.glsl",       // UnlitViewSphere
		":/resources/shaders/fakelit_color_vs.glsl",           // FakeLitColor
		":/resources/shaders/fakelit_texture_decal_vs.glsl"    // FakeLitTextureDecal
	};

	LC_ARRAY_SIZE_CHECK(VertexShaders, lcMaterialType::Count);

	const char* const FragmentShaders[] =
	{
		":/resources/shaders/unlit_color_ps.glsl",             // UnlitColor
		":/resources/shaders/unlit_color_conditional_ps.glsl", // UnlitColorConditional
		":/resources/shaders/unlit_texture_modulate_ps.glsl",  // UnlitTextureModulate
		":/resources/shaders/unlit_texture_decal_ps.glsl",     // UnlitTextureDecal
		":/resources/shaders/unlit_vertex_color_ps.glsl",      // UnlitVertexColor
		":/resources/shaders/unlit_view_sphere_ps.glsl",       // UnlitViewSphere
		":/resources/shaders/fakelit_color_ps.glsl",           // FakeLitColor
		":/resources/shaders/fakelit_texture_decal_ps.glsl"    // FakeLitTextureDecal
	};

	LC_ARRAY_SIZE_CHECK(FragmentShaders, lcMaterialType::Count);

	const auto LoadShader = [this, ShaderPrefix](const char* FileName, GLuint ShaderType) -> GLuint
	{
		QFile ShaderFile(FileName);

		if (!ShaderFile.open(QIODevice::ReadOnly))
			return 0;

		QByteArray Data = ShaderPrefix + ShaderFile.readAll();
		const char* Source = Data.constData();

		const GLuint Shader = glCreateShader(ShaderType);
		glShaderSource(Shader, 1, &Source, nullptr);
		glCompileShader(Shader);

#ifndef QT_NO_DEBUG
		GLint ShaderCompiled = 0;
		glGetShaderiv(Shader, GL_COMPILE_STATUS, &ShaderCompiled);

		if (ShaderCompiled == GL_FALSE)
		{
			GLint Length = 0;
			glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &Length);

			QByteArray InfoLog;
			InfoLog.resize(Length);
			glGetShaderInfoLog(Shader, Length, &Length, InfoLog.data());

			qDebug() << InfoLog;
		}
#endif

		return Shader;
	};

	for (int MaterialType = 0; MaterialType < static_cast<int>(lcMaterialType::Count); MaterialType++)
	{
		const GLuint VertexShader = LoadShader(VertexShaders[MaterialType], GL_VERTEX_SHADER);
		const GLuint FragmentShader = LoadShader(FragmentShaders[MaterialType], GL_FRAGMENT_SHADER);

		GLuint Program = glCreateProgram();

		glAttachShader(Program, VertexShader);
		glAttachShader(Program, FragmentShader);

		glBindAttribLocation(Program, static_cast<int>(lcProgramAttrib::Position), "VertexPosition");
		glBindAttribLocation(Program, static_cast<int>(lcProgramAttrib::Normal), "VertexNormal");
		glBindAttribLocation(Program, static_cast<int>(lcProgramAttrib::TexCoord), "VertexTexCoord");
		glBindAttribLocation(Program, static_cast<int>(lcProgramAttrib::Color), "VertexColor");

		glBindAttribLocation(Program, static_cast<int>(lcProgramAttrib::ControlPoint1), "VertexPosition1");
		glBindAttribLocation(Program, static_cast<int>(lcProgramAttrib::ControlPoint2), "VertexPosition2");
		glBindAttribLocation(Program, static_cast<int>(lcProgramAttrib::ControlPoint3), "VertexPosition3");
		glBindAttribLocation(Program, static_cast<int>(lcProgramAttrib::ControlPoint4), "VertexPosition4");

		glLinkProgram(Program);

		glDetachShader(Program, VertexShader);
		glDetachShader(Program, FragmentShader);
		glDeleteShader(VertexShader);
		glDeleteShader(FragmentShader);

		GLint IsLinked = 0;
		glGetProgramiv(Program, GL_LINK_STATUS, &IsLinked);

		if (IsLinked == GL_FALSE)
		{
			GLint Length = 0;
			glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &Length);

			QByteArray InfoLog;
			InfoLog.resize(Length);
			glGetProgramInfoLog(Program, Length, &Length, InfoLog.data());

			glDeleteProgram(Program);
			Program = 0;
		}

		mPrograms[MaterialType].Object = Program;
		mPrograms[MaterialType].WorldViewProjectionMatrixLocation = glGetUniformLocation(Program, "WorldViewProjectionMatrix");
		mPrograms[MaterialType].WorldMatrixLocation = glGetUniformLocation(Program, "WorldMatrix");
		mPrograms[MaterialType].MaterialColorLocation = glGetUniformLocation(Program, "MaterialColor");
		mPrograms[MaterialType].LightPositionLocation = glGetUniformLocation(Program, "LightPosition");
		mPrograms[MaterialType].EyePositionLocation = glGetUniformLocation(Program, "EyePosition");
		mPrograms[MaterialType].HighlightParamsLocation = glGetUniformLocation(Program, "HighlightParams");

		const GLint TextureLocation = glGetUniformLocation(Program, "Texture");

		if (TextureLocation != -1)
		{
			glUseProgram(Program);
			glUniform1i(TextureLocation, 0);
			glUseProgram(0);
		}
	}
}

void lcContext::CreateResources()
{
	if (!gSupportsShaderObjects)
		return;

	CreateShaderPrograms();
}

void lcContext::DestroyResources()
{
	if (!gSupportsShaderObjects)
		return;

	for (int MaterialType = 0; MaterialType < static_cast<int>(lcMaterialType::Count); MaterialType++)
	{
		glDeleteProgram(mPrograms[MaterialType].Object);
		mPrograms[MaterialType].Object = 0;
	}
}

void lcContext::MakeCurrent()
{
	if (mWidget)
		mWidget->makeCurrent();
	else
		mOffscreenContext->makeCurrent(mOffscreenSurface.get());
}

void lcContext::SetGLContext(QOpenGLContext* Context, QOpenGLWidget* Widget)
{
	mContext = Context;
	mWidget = Widget;

	MakeCurrent();
	initializeOpenGLFunctions();
}

void lcContext::SetOffscreenContext()
{
	SetGLContext(mOffscreenContext.get(), nullptr);
}

void lcContext::SetDefaultState()
{
#ifndef LC_OPENGLES
	if (QSurfaceFormat::defaultFormat().samples() > 1)
		glEnable(GL_LINE_SMOOTH);
#endif

	glEnable(GL_DEPTH_TEST);
	mDepthTest = true;

	glDepthFunc(GL_LEQUAL);
	mDepthFunction = lcDepthFunction::LessEqual;

	mColorWrite = true;
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	mColorBlend = false;
	glDisable(GL_BLEND);

	if (gSupportsBlendFuncSeparate)
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
	else
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (gSupportsVertexBufferObject)
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}

	if (gSupportsShaderObjects)
	{
		SetVertexAttribPointer(lcProgramAttrib::Position, 3, GL_FLOAT, false, 0, nullptr);
		EnableVertexAttrib(lcProgramAttrib::Position);

		DisableVertexAttrib(lcProgramAttrib::Normal);
		SetVertexAttribPointer(lcProgramAttrib::Normal, 4, GL_BYTE, true, 0, nullptr);
		DisableVertexAttrib(lcProgramAttrib::TexCoord);
		SetVertexAttribPointer(lcProgramAttrib::TexCoord, 2, GL_FLOAT, false, 0, nullptr);
		DisableVertexAttrib(lcProgramAttrib::Color);
		SetVertexAttribPointer(lcProgramAttrib::Color, 4, GL_FLOAT, false, 0, nullptr);
	}
	else
	{
#if LC_FIXED_FUNCTION
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		glVertexPointer(3, GL_FLOAT, 0, nullptr);
		glNormalPointer(GL_BYTE, 0, nullptr);
		glTexCoordPointer(2, GL_FLOAT, 0, nullptr);
		glColorPointer(4, GL_FLOAT, 0, nullptr);

		mNormalEnabled = false;
		mTexCoordEnabled = false;
		mColorEnabled = false;
#endif
	}

	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = nullptr;
	mIndexBufferPointer = nullptr;
	mVertexBufferOffset = (char*)~0;

	for (int AttribIndex = 0; AttribIndex < static_cast<int>(lcProgramAttrib::Count); AttribIndex++)
		mVertexAttribState[AttribIndex] = lcVertexAttribState();

	glBindTexture(GL_TEXTURE_2D, 0);
	mTexture2D = 0;
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	mTextureCubeMap = 0;

	glDisable(GL_POLYGON_OFFSET_FILL);
	mPolygonOffset = lcPolygonOffset::None;

	mDepthWrite = true;
	glDepthMask(GL_TRUE);

	glDisable(GL_CULL_FACE);
	mCullFace = false;

	glLineWidth(1.0f);
	mLineWidth = 1.0f;

	if (gSupportsShaderObjects)
	{
		glUseProgram(0);
		mMaterialType = lcMaterialType::Count;
	}
	else
	{
#if LC_FIXED_FUNCTION
		glMatrixMode(GL_MODELVIEW);
		mMatrixMode = GL_MODELVIEW;
		glShadeModel(GL_FLAT);

		glDisable(GL_TEXTURE_2D);
		mTextureEnabled = false;
#endif
	}
}

void lcContext::ClearColorAndDepth(const lcVector4& ClearColor)
{
	glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void lcContext::ClearDepth()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void lcContext::ClearResources()
{
	ClearVertexBuffer();
	ClearIndexBuffer();
	ClearTexture2D();
}

void lcContext::SetMaterial(lcMaterialType MaterialType)
{
	if (MaterialType == mMaterialType)
		return;

	mMaterialType = MaterialType;

	if (gSupportsShaderObjects)
	{
		glUseProgram(mPrograms[static_cast<int>(MaterialType)].Object);
		mColorDirty = true;
		mWorldMatrixDirty = true; // todo: change dirty to a bitfield and set the lighting constants dirty here
		mViewMatrixDirty = true;
		mHighlightParamsDirty = true;
	}
	else
	{
#if LC_FIXED_FUNCTION
		switch (MaterialType)
		{
		case lcMaterialType::UnlitTextureModulate:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			if (!mTextureEnabled)
			{
				glEnable(GL_TEXTURE_2D);
				mTextureEnabled = true;
			}
			break;

		case lcMaterialType::FakeLitTextureDecal:
		case lcMaterialType::UnlitTextureDecal:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

			if (!mTextureEnabled)
			{
				glEnable(GL_TEXTURE_2D);
				mTextureEnabled = true;
			}
			break;

		case lcMaterialType::UnlitColor:
		case lcMaterialType::UnlitColorConditional:
		case lcMaterialType::UnlitVertexColor:
		case lcMaterialType::FakeLitColor:
			if (mTextureEnabled)
			{
				glDisable(GL_TEXTURE_2D);
				mTextureEnabled = false;
			}
			break;

		case lcMaterialType::UnlitViewSphere:
		case lcMaterialType::Count:
			break;
		}
#endif
	}
}

void lcContext::SetViewport(int x, int y, int Width, int Height)
{
	glViewport(x, y, Width, Height);
}

void lcContext::SetPolygonOffset(lcPolygonOffset PolygonOffset)
{
	if (mPolygonOffset == PolygonOffset)
		return;

	switch (PolygonOffset)
	{
	case lcPolygonOffset::None:
		glDisable(GL_POLYGON_OFFSET_FILL);
		break;

	case lcPolygonOffset::Opaque:
		glPolygonOffset(0.5f, 0.1f);
		glEnable(GL_POLYGON_OFFSET_FILL);
		break;

	case lcPolygonOffset::Translucent:
		glPolygonOffset(0.25f, 0.1f);
		glEnable(GL_POLYGON_OFFSET_FILL);
		break;
	}

	mPolygonOffset = PolygonOffset;
}

void lcContext::SetDepthWrite(bool Enable)
{
	if (Enable == mDepthWrite)
		return;

	glDepthMask(Enable ? GL_TRUE : GL_FALSE);
	mDepthWrite = Enable;
}

void lcContext::SetDepthFunction(lcDepthFunction DepthFunction)
{
	if (DepthFunction == mDepthFunction)
		return;

	switch (DepthFunction)
	{
	case lcDepthFunction::Always:
		glDepthFunc(GL_ALWAYS);
		break;

	case lcDepthFunction::LessEqual:
		glDepthFunc(GL_LEQUAL);
		break;
	}

	mDepthFunction = DepthFunction;
}

void lcContext::EnableDepthTest(bool Enable)
{
	if (Enable == mDepthTest)
		return;

	if (Enable)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	mDepthTest = Enable;
}

void lcContext::EnableColorWrite(bool Enable)
{
	if (Enable == mColorWrite)
		return;

	if (Enable)
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	else
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	mColorWrite = Enable;
}

void lcContext::EnableColorBlend(bool Enable)
{
	if (Enable == mColorBlend)
		return;

	if (Enable)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	mColorBlend = Enable;
}

void lcContext::EnableCullFace(bool Enable)
{
	if (Enable == mCullFace)
		return;

	if (Enable)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	mCullFace = Enable;
}

void lcContext::SetLineWidth(float LineWidth)
{
	if (LineWidth == mLineWidth)
		return;

	glLineWidth(LineWidth);
	mLineWidth = LineWidth;
}

void lcContext::BindTexture2D(const lcTexture* Texture)
{
	GLuint TextureObject = Texture->mTexture;

	if (mTexture2D == TextureObject)
		return;

	glBindTexture(GL_TEXTURE_2D, TextureObject);
	mTexture2D = TextureObject;
}

void lcContext::BindTextureCubeMap(const lcTexture* Texture)
{
	GLuint TextureObject = Texture->mTexture;

	if (mTextureCubeMap == TextureObject)
		return;

	glBindTexture(GL_TEXTURE_CUBE_MAP, TextureObject);
	mTextureCubeMap = TextureObject;
}

void lcContext::ClearTexture2D()
{
	if (mTexture2D == 0)
		return;

	glBindTexture(GL_TEXTURE_2D, 0);
	mTexture2D = 0;
}

void lcContext::ClearTextureCubeMap()
{
	if (mTexture2D == 0)
		return;

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	mTextureCubeMap = 0;
}

void lcContext::UploadTexture(lcTexture* Texture)
{
	if (!Texture->mTexture)
		glGenTextures(1, &Texture->mTexture);

	constexpr int Filters[2][5] =
	{
		{ GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR },
		{ GL_NEAREST, GL_LINEAR, GL_LINEAR, GL_LINEAR, GL_LINEAR  },
	};

	const int Flags = Texture->GetFlags();
	const int FilterFlags = Flags & LC_TEXTURE_FILTER_MASK;
	const int FilterIndex = FilterFlags >> LC_TEXTURE_FILTER_SHIFT;
	const int MipIndex = Flags & LC_TEXTURE_MIPMAPS ? 0 : 1;

	unsigned int Faces, Target;

	if ((Flags & LC_TEXTURE_CUBEMAP) == 0)
	{
		Faces = 1;
		Target = GL_TEXTURE_2D;
		BindTexture2D(Texture);
	}
	else
	{
		Faces = 6;
		Target = GL_TEXTURE_CUBE_MAP;
		BindTextureCubeMap(Texture);
	}

	glTexParameteri(Target, GL_TEXTURE_WRAP_S, (Flags & LC_TEXTURE_WRAPU) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(Target, GL_TEXTURE_WRAP_T, (Flags & LC_TEXTURE_WRAPV) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(Target, GL_TEXTURE_MIN_FILTER, Filters[MipIndex][FilterIndex]);
	glTexParameteri(Target, GL_TEXTURE_MAG_FILTER, Filters[1][FilterIndex]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	if (gSupportsAnisotropic && FilterFlags == LC_TEXTURE_ANISOTROPIC)
		glTexParameterf(Target, GL_TEXTURE_MAX_ANISOTROPY_EXT, lcMin(4.0f, gMaxAnisotropy));

	int Format;
	switch (Texture->GetImage(0).mFormat)
	{
	default:
	case lcPixelFormat::Invalid:
		Format = 0;
		break;
	case lcPixelFormat::A8:
		Format = GL_ALPHA;
		break;
	case lcPixelFormat::L8A8:
		Format = GL_LUMINANCE_ALPHA;
		break;
	case lcPixelFormat::R8G8B8:
		Format = GL_RGB;
		break;
	case lcPixelFormat::R8G8B8A8:
		Format = GL_RGBA;
		break;
	}

	int CurrentImage = 0;
	if (Flags & LC_TEXTURE_CUBEMAP)
		Target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;

	for (size_t FaceIdx = 0; FaceIdx < Faces; FaceIdx++)
	{
		void* Data = Texture->GetImage(CurrentImage).mData;
		glTexImage2D(Target, 0, Format, Texture->mWidth, Texture->mHeight, 0, Format, GL_UNSIGNED_BYTE, Data);

		if (Flags & LC_TEXTURE_MIPMAPS || FilterFlags >= LC_TEXTURE_BILINEAR)
		{
			int Width = Texture->mWidth;
			int Height = Texture->mHeight;
			int Components = Texture->GetImage(CurrentImage).GetBPP();

			for (int Level = 1; ((Width != 1) || (Height != 1)); Level++)
			{
				int RowStride = Width * Components;

				Width = lcMax(1, Width >> 1);
				Height = lcMax(1, Height >> 1);

				if (Texture->GetImageCount() == Faces)
				{
					GLubyte* Out, * In;

					In = Out = (GLubyte*)Data;

					for (int y = 0; y < Height; y++, In += RowStride)
						for (int x = 0; x < Width; x++, Out += Components, In += 2 * Components)
							for (int c = 0; c < Components; c++)
								Out[c] = (In[c] + In[c + Components] + In[RowStride] + In[c + RowStride + Components]) / 4;
				}
				else
					Data = Texture->GetImage(++CurrentImage).mData;

				glTexImage2D(Target, Level, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Data);
			}

			if (Texture->GetImageCount() == Faces)
				CurrentImage++;
		}
		else
			CurrentImage++;

		Target++;
	}

	if ((Flags & LC_TEXTURE_CUBEMAP) == 0)
		ClearTexture2D();
	else
		ClearTextureCubeMap();
}

void lcContext::SetColor(float Red, float Green, float Blue, float Alpha)
{
	SetColor(lcVector4(Red, Green, Blue, Alpha));
}

void lcContext::SetColorIndex(int ColorIndex)
{
	SetColor(gColorList[ColorIndex].Value);
}

void lcContext::SetColorIndexTinted(int ColorIndex, const lcVector4& Tint, float Weight)
{
	const lcVector3 Color(gColorList[ColorIndex].Value * Weight + Tint * (1.0f - Weight));
	SetColor(lcVector4(Color, gColorList[ColorIndex].Value.w));
}

void lcContext::SetColorIndexTinted(int ColorIndex, const lcVector4& Tint)
{
	SetColor(gColorList[ColorIndex].Value * Tint);
}

void lcContext::SetEdgeColorIndex(int ColorIndex)
{
	SetColor(gColorList[ColorIndex].Edge);
}

void lcContext::SetEdgeColorIndexTinted(int ColorIndex, const lcVector4& Tint)
{
	SetColor(gColorList[ColorIndex].Edge * Tint);
}

lcVertexBuffer lcContext::CreateVertexBuffer(int Size, const void* Data)
{
	lcVertexBuffer VertexBuffer;

	if (gSupportsVertexBufferObject)
	{
		glGenBuffers(1, &VertexBuffer.Object);
		glBindBuffer(GL_ARRAY_BUFFER_ARB, VertexBuffer.Object);
		glBufferData(GL_ARRAY_BUFFER_ARB, Size, Data, GL_STATIC_DRAW_ARB);

		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0); // context remove
		mVertexBufferObject = 0;
	}
	else
	{
		VertexBuffer.Pointer = malloc(Size);
		if (VertexBuffer.Pointer)
			memcpy(VertexBuffer.Pointer, Data, Size);
	}

	return VertexBuffer;
}

void lcContext::DestroyVertexBuffer(lcVertexBuffer& VertexBuffer)
{
	if (!VertexBuffer.IsValid())
		return;

	if (gSupportsVertexBufferObject)
	{
		if (mVertexBufferObject == VertexBuffer.Object)
		{
			glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
			mVertexBufferObject = 0;
		}

		glDeleteBuffers(1, &VertexBuffer.Object);
	}
	else
	{
		free(VertexBuffer.Pointer);
	}

	VertexBuffer.Pointer = nullptr;
}

lcIndexBuffer lcContext::CreateIndexBuffer(int Size, const void* Data)
{
	lcIndexBuffer IndexBuffer;

	if (gSupportsVertexBufferObject)
	{
		glGenBuffers(1, &IndexBuffer.Object);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, IndexBuffer.Object);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER_ARB, Size, Data, GL_STATIC_DRAW_ARB);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0); // context remove
		mIndexBufferObject = 0;
	}
	else
	{
		IndexBuffer.Pointer = malloc(Size);
		if (IndexBuffer.Pointer)
			memcpy(IndexBuffer.Pointer, Data, Size);
	}

	return IndexBuffer;
}

void lcContext::DestroyIndexBuffer(lcIndexBuffer& IndexBuffer)
{
	if (!IndexBuffer.IsValid())
		return;

	if (gSupportsVertexBufferObject)
	{
		if (mIndexBufferObject == IndexBuffer.Object)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			mIndexBufferObject = 0;
		}

		glDeleteBuffers(1, &IndexBuffer.Object);
	}
	else
	{
		free(IndexBuffer.Pointer);
	}

	IndexBuffer.Pointer = nullptr;
}

void lcContext::ClearVertexBuffer()
{
	mVertexBufferPointer = nullptr;
	mVertexBufferOffset = (char*)~0;

	if (mVertexBufferObject)
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		mVertexBufferObject = 0;
	}

	if (gSupportsShaderObjects)
	{
		SetVertexAttribPointer(lcProgramAttrib::Position, 3, GL_FLOAT, false, 0, nullptr);

		DisableVertexAttrib(lcProgramAttrib::Normal);
		SetVertexAttribPointer(lcProgramAttrib::Normal, 4, GL_BYTE, true, 0, nullptr);
		DisableVertexAttrib(lcProgramAttrib::TexCoord);
		SetVertexAttribPointer(lcProgramAttrib::TexCoord, 2, GL_FLOAT, false, 0, nullptr);
		DisableVertexAttrib(lcProgramAttrib::Color);
		SetVertexAttribPointer(lcProgramAttrib::Color, 4, GL_FLOAT, false, 0, nullptr);
	}
	else
	{
#if LC_FIXED_FUNCTION
		if (mNormalEnabled)
			glDisableClientState(GL_NORMAL_ARRAY);

		if (mTexCoordEnabled)
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		if (mColorEnabled)
			glDisableClientState(GL_COLOR_ARRAY);

		glVertexPointer(3, GL_FLOAT, 0, nullptr);
		glNormalPointer(GL_BYTE, 0, nullptr);
		glTexCoordPointer(2, GL_FLOAT, 0, nullptr);
		glColorPointer(4, GL_FLOAT, 0, nullptr);
#endif
	}
}

void lcContext::SetVertexBuffer(lcVertexBuffer VertexBuffer)
{
	if (gSupportsVertexBufferObject)
	{
		const GLuint VertexBufferObject = VertexBuffer.Object;
		mVertexBufferPointer = nullptr;

		if (VertexBufferObject != mVertexBufferObject)
		{
			glBindBuffer(GL_ARRAY_BUFFER_ARB, VertexBufferObject);
			mVertexBufferObject = VertexBufferObject;
			mVertexBufferOffset = (char*)~0;
		}
	}
	else
	{
		mVertexBufferPointer = (char*)VertexBuffer.Pointer;
		mVertexBufferOffset = (char*)~0;
	}
}

void lcContext::SetVertexBufferPointer(const void* VertexBuffer)
{
	if (gSupportsVertexBufferObject && mVertexBufferObject)
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		mVertexBufferObject = 0;
	}

	mVertexBufferPointer = (char*)VertexBuffer;
	mVertexBufferOffset = (char*)~0;
}

void lcContext::SetVertexAttribPointer(lcProgramAttrib Attrib, GLint Size, GLenum Type, GLboolean Normalized, GLsizei Stride, const void* Pointer)
{
	const int Index = static_cast<int>(Attrib);
	lcVertexAttribState& State = mVertexAttribState[Index];

	if (State.Size != Size || State.Type != Type || State.Normalized != Normalized || State.Stride != Stride || State.Pointer != Pointer || State.VertexBufferObject != mVertexBufferObject)
	{
		glVertexAttribPointer(Index, Size, Type, Normalized, Stride, Pointer);

		State.Size = Size;
		State.Type = Type;
		State.Normalized = Normalized;
		State.Stride = Stride;
		State.Pointer = Pointer;
		State.VertexBufferObject = mVertexBufferObject;
	}
}

void lcContext::EnableVertexAttrib(lcProgramAttrib Attrib)
{
	const int Index = static_cast<int>(Attrib);
	lcVertexAttribState& State = mVertexAttribState[Index];

	if (!State.Enabled)
	{
		glEnableVertexAttribArray(Index);
		State.Enabled = true;
	}
}

void lcContext::DisableVertexAttrib(lcProgramAttrib Attrib)
{
	const int Index = static_cast<int>(Attrib);
	lcVertexAttribState& State = mVertexAttribState[Index];

	if (State.Enabled)
	{
		glDisableVertexAttribArray(Index);
		State.Enabled = false;
	}
}

void lcContext::SetVertexFormatPosition(int PositionSize)
{
	const int VertexSize = PositionSize * sizeof(float);
	const char* VertexBufferPointer = mVertexBufferPointer;

	if (gSupportsShaderObjects)
	{
		SetVertexAttribPointer(lcProgramAttrib::Position, PositionSize, GL_FLOAT, false, VertexSize, VertexBufferPointer);
		DisableVertexAttrib(lcProgramAttrib::Normal);
		DisableVertexAttrib(lcProgramAttrib::TexCoord);
		DisableVertexAttrib(lcProgramAttrib::Color);
	}
	else
	{
#if LC_FIXED_FUNCTION
		if (mVertexBufferOffset != mVertexBufferPointer)
		{
			glVertexPointer(PositionSize, GL_FLOAT, VertexSize, VertexBufferPointer);
			mVertexBufferOffset = VertexBufferPointer;
		}

		if (mNormalEnabled)
		{
			glDisableClientState(GL_NORMAL_ARRAY);
			mNormalEnabled = false;
		}

		if (mTexCoordEnabled)
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			mTexCoordEnabled = false;
		}

		if (mColorEnabled)
		{
			glDisableClientState(GL_COLOR_ARRAY);
			mColorEnabled = false;
		}
#endif
	}
}

void lcContext::SetVertexFormatConditional(int BufferOffset)
{
	constexpr int VertexSize = 12 * sizeof(float);
	const char* VertexBufferPointer = mVertexBufferPointer + BufferOffset;

	if (gSupportsShaderObjects)
	{
		SetVertexAttribPointer(lcProgramAttrib::ControlPoint1, 3, GL_FLOAT, false, VertexSize, VertexBufferPointer);
		EnableVertexAttrib(lcProgramAttrib::ControlPoint1);
		SetVertexAttribPointer(lcProgramAttrib::ControlPoint2, 3, GL_FLOAT, false, VertexSize, VertexBufferPointer + 3 * sizeof(float));
		EnableVertexAttrib(lcProgramAttrib::ControlPoint2);
		SetVertexAttribPointer(lcProgramAttrib::ControlPoint3, 3, GL_FLOAT, false, VertexSize, VertexBufferPointer + 6 * sizeof(float));
		EnableVertexAttrib(lcProgramAttrib::ControlPoint3);
		SetVertexAttribPointer(lcProgramAttrib::ControlPoint4, 3, GL_FLOAT, false, VertexSize, VertexBufferPointer + 9 * sizeof(float));
		EnableVertexAttrib(lcProgramAttrib::ControlPoint4);
	}
}

void lcContext::SetVertexFormat(int BufferOffset, int PositionSize, int NormalSize, int TexCoordSize, int ColorSize, bool EnableNormals)
{
	const int VertexSize = (PositionSize + TexCoordSize) * sizeof(float) + NormalSize * sizeof(quint32) + ColorSize;
	const char* VertexBufferPointer = mVertexBufferPointer + BufferOffset;

	if (gSupportsShaderObjects)
	{
		int Offset = 0;

		SetVertexAttribPointer(lcProgramAttrib::Position, PositionSize, GL_FLOAT, false, VertexSize, VertexBufferPointer);
		EnableVertexAttrib(lcProgramAttrib::Position);

		Offset += PositionSize * sizeof(float);

		if (NormalSize && EnableNormals)
		{
			SetVertexAttribPointer(lcProgramAttrib::Normal, 4, GL_BYTE, true, VertexSize, VertexBufferPointer + Offset);
			EnableVertexAttrib(lcProgramAttrib::Normal);
		}
		else
			DisableVertexAttrib(lcProgramAttrib::Normal);

		Offset += NormalSize * sizeof(quint32);

		if (TexCoordSize)
		{
			SetVertexAttribPointer(lcProgramAttrib::TexCoord, TexCoordSize, GL_FLOAT, false, VertexSize, VertexBufferPointer + Offset);
			EnableVertexAttrib(lcProgramAttrib::TexCoord);
		}
		else
			DisableVertexAttrib(lcProgramAttrib::TexCoord);

		Offset += TexCoordSize * sizeof(float);

		if (ColorSize)
		{
			SetVertexAttribPointer(lcProgramAttrib::Color, ColorSize, GL_UNSIGNED_BYTE, true, VertexSize, VertexBufferPointer + Offset);
			EnableVertexAttrib(lcProgramAttrib::Color);
		}
		else
			DisableVertexAttrib(lcProgramAttrib::Color);
	}
	else
	{
#if LC_FIXED_FUNCTION
		if (mVertexBufferOffset != VertexBufferPointer)
		{
			glVertexPointer(PositionSize, GL_FLOAT, VertexSize, VertexBufferPointer);
			mVertexBufferOffset = VertexBufferPointer;
		}

		int Offset = PositionSize * sizeof(float);

		if (NormalSize && EnableNormals)
		{
			glNormalPointer(GL_BYTE, VertexSize, VertexBufferPointer + Offset);

			if (!mNormalEnabled)
			{
				glEnableClientState(GL_NORMAL_ARRAY);
				mNormalEnabled = true;
			}
		}
		else if (mNormalEnabled)
		{
			glDisableClientState(GL_NORMAL_ARRAY);
			mNormalEnabled = false;
		}

		Offset += NormalSize * sizeof(quint32);

		if (TexCoordSize)
		{
			glTexCoordPointer(TexCoordSize, GL_FLOAT, VertexSize, VertexBufferPointer + Offset);

			if (!mTexCoordEnabled)
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				mTexCoordEnabled = true;
			}

			Offset += 2 * sizeof(float);
		}
		else if (mTexCoordEnabled)
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			mTexCoordEnabled = false;
		}

		if (ColorSize)
		{
			glColorPointer(ColorSize, GL_FLOAT, VertexSize, VertexBufferPointer + Offset);

			if (!mColorEnabled)
			{
				glEnableClientState(GL_COLOR_ARRAY);
				mColorEnabled = true;
			}
		}
		else if (mColorEnabled)
		{
			glDisableClientState(GL_COLOR_ARRAY);
			mColorEnabled = false;
		}
#endif
	}
}

void lcContext::ClearIndexBuffer()
{
	if (mIndexBufferObject)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		mIndexBufferObject = 0;
	}
}

void lcContext::SetIndexBuffer(lcIndexBuffer IndexBuffer)
{
	if (gSupportsVertexBufferObject)
	{
		const GLuint IndexBufferObject = IndexBuffer.Object;
		mIndexBufferPointer = nullptr;

		if (IndexBufferObject != mIndexBufferObject)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, IndexBufferObject);
			mIndexBufferObject = IndexBufferObject;
		}
	}
	else
	{
		mIndexBufferPointer = (char*)IndexBuffer.Pointer;
	}
}

void lcContext::SetIndexBufferPointer(const void* IndexBuffer)
{
	if (mIndexBufferObject)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		mIndexBufferObject = 0;
	}

	mIndexBufferPointer = (char*)IndexBuffer;
}

void lcContext::BindMesh(const lcMesh* Mesh)
{
	const lcPiecesLibrary* const Library = lcGetPiecesLibrary();

	if (Mesh->mVertexCacheOffset != -1)
	{
		const GLuint VertexBufferObject = Library->mVertexBuffer.Object;
		const GLuint IndexBufferObject = Library->mIndexBuffer.Object;

		if (VertexBufferObject != mVertexBufferObject)
		{
			glBindBuffer(GL_ARRAY_BUFFER_ARB, VertexBufferObject);
			mVertexBufferObject = VertexBufferObject;
			mVertexBufferPointer = nullptr;
			mVertexBufferOffset = (char*)~0;
		}

		if (IndexBufferObject != mIndexBufferObject)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, IndexBufferObject);
			mIndexBufferObject = IndexBufferObject;
			mIndexBufferPointer = nullptr;
		}
	}
	else
	{
		if (mVertexBufferObject)
		{
			glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
			mVertexBufferObject = 0;
		}

		if (mIndexBufferObject)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			mIndexBufferObject = 0;
		}

		mVertexBufferPointer = (char*)Mesh->mVertexData;
		mIndexBufferPointer = (char*)Mesh->mIndexData;
		mVertexBufferOffset = (char*)~0;
	}
}

void lcContext::FlushState()
{
	if (gSupportsShaderObjects)
	{
		const lcProgram& Program = mPrograms[static_cast<int>(mMaterialType)];

		if (mWorldMatrixDirty || mViewMatrixDirty || mProjectionMatrixDirty)
		{
			if (mViewProjectionMatrixDirty)
			{
				mViewProjectionMatrix = lcMul(mViewMatrix, mProjectionMatrix);
				mViewProjectionMatrixDirty = false;
			}

			if (mWorldMatrixDirty)
			{
				if (Program.WorldMatrixLocation != -1)
					glUniformMatrix4fv(Program.WorldMatrixLocation, 1, false, mWorldMatrix);
			}

			if (mViewMatrixDirty)
			{
				const lcMatrix44 InverseViewMatrix = lcMatrix44AffineInverse(mViewMatrix);
				lcVector3 ViewPosition = lcMul30(-mViewMatrix.GetTranslation(), InverseViewMatrix);

				if (Program.LightPositionLocation != -1)
				{
					lcVector3 LightPosition = ViewPosition + lcMul30(lcVector3(300.0f, 300.0f, 0.0f), InverseViewMatrix);
					glUniform3fv(Program.LightPositionLocation, 1, LightPosition);
				}

				if (Program.EyePositionLocation != -1)
					glUniform3fv(Program.EyePositionLocation, 1, ViewPosition);
			}

			glUniformMatrix4fv(Program.WorldViewProjectionMatrixLocation, 1, false, lcMul(mWorldMatrix, mViewProjectionMatrix));
			mWorldMatrixDirty = false;
			mViewMatrixDirty = false;
			mProjectionMatrixDirty = false;
		}

		if (mColorDirty && Program.MaterialColorLocation != -1)
		{
			glUniform4fv(Program.MaterialColorLocation, 1, mColor);
			mColorDirty = false;
		}

		if (mHighlightParamsDirty && Program.HighlightParamsLocation != -1)
		{
			glUniform4fv(Program.HighlightParamsLocation, 4, mHighlightParams[0]);
			mHighlightParamsDirty = false;
		}
	}
	else
	{
#if LC_FIXED_FUNCTION
		glColor4fv(mColor);

		if (mWorldMatrixDirty || mViewMatrixDirty)
		{
			if (mMatrixMode != GL_MODELVIEW)
			{
				glMatrixMode(GL_MODELVIEW);
				mMatrixMode = GL_MODELVIEW;
			}

			glLoadMatrixf(lcMul(mWorldMatrix, mViewMatrix));
			mWorldMatrixDirty = false;
			mViewMatrixDirty = false;
		}

		if (mProjectionMatrixDirty)
		{
			if (mMatrixMode != GL_PROJECTION)
			{
				glMatrixMode(GL_PROJECTION);
				mMatrixMode = GL_PROJECTION;
			}

			glLoadMatrixf(mProjectionMatrix);
			mProjectionMatrixDirty = false;
		}
#endif
	}
}

void lcContext::DrawPrimitives(GLenum Mode, GLint First, GLsizei Count)
{
	FlushState();
	glDrawArrays(Mode, First, Count);
}

void lcContext::DrawIndexedPrimitives(GLenum Mode, GLsizei Count, GLenum Type, int Offset)
{
	FlushState();
	glDrawElements(Mode, Count, Type, mIndexBufferPointer + Offset);
}

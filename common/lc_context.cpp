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
#include "lc_scene.h"

#ifdef LC_OPENGLES
#define GL_ARRAY_BUFFER_ARB GL_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER_ARB GL_ELEMENT_ARRAY_BUFFER
#define GL_STATIC_DRAW_ARB GL_STATIC_DRAW
#endif

std::unique_ptr<QOpenGLContext> lcContext::mOffscreenContext;
std::unique_ptr<QOffscreenSurface> lcContext::mOffscreenSurface;
std::unique_ptr<lcContext> lcContext::mGlobalOffscreenContext;
bool lcContext::mRenderResourcesCreated = false;
lcProgram lcContext::mPrograms[static_cast<int>(lcMaterialType::Count)];

lcScopedGLContextState::lcScopedGLContextState()
	: mContext(QOpenGLContext::currentContext()), mSurface(mContext ? mContext->surface() : nullptr)
{
	if (!mContext)
		return;

	QOpenGLFunctions* Functions = mContext->functions();
	mSeparateFramebuffers = QOpenGLFramebufferObject::hasOpenGLFramebufferBlit();

	if (mSeparateFramebuffers)
	{
		Functions->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &mDrawFramebuffer);
		Functions->glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &mReadFramebuffer);
	}
	else
		Functions->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mDrawFramebuffer);
}

lcScopedGLContextState::~lcScopedGLContextState()
{
	QOpenGLContext* CurrentContext = QOpenGLContext::currentContext();

	if (!mContext)
	{
		if (CurrentContext)
			CurrentContext->doneCurrent();

		return;
	}

	if (CurrentContext != mContext || mContext->surface() != mSurface)
		if (!mContext->makeCurrent(mSurface))
			return;

	QOpenGLFunctions* Functions = mContext->functions();

	if (mSeparateFramebuffers)
	{
		Functions->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mDrawFramebuffer);
		Functions->glBindFramebuffer(GL_READ_FRAMEBUFFER, mReadFramebuffer);
	}
	else
		Functions->glBindFramebuffer(GL_FRAMEBUFFER, mDrawFramebuffer);
}

lcContext::lcContext()
{
	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = nullptr;
	mIndexBufferPointer = nullptr;

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

	mColor = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mTextHaloColor = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mWorldMatrix = lcMatrix44Identity();
	mViewMatrix = lcMatrix44Identity();
	mProjectionMatrix = lcMatrix44Identity();
	mViewProjectionMatrix = lcMatrix44Identity();
	mHighlightParams[0] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mHighlightParams[1] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mHighlightParams[2] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mHighlightParams[3] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mOutlineParams[0] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mOutlineParams[1] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mOutlineParams[2] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mOutlineParams[3] = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mColorDirty = false;
	mTextHaloColorDirty = false;
	mWorldMatrixDirty = false;
	mViewMatrixDirty = false;
	mProjectionMatrixDirty = false;
	mViewProjectionMatrixDirty = false;
	mHighlightParamsDirty = false;
	mOutlineParamsDirty = false;

	mMaterialType = lcMaterialType::Count;
}

lcContext::~lcContext()
{
}

lcResult<void> lcContext::InitializeRenderer(bool CreateRenderResources)
{
	const lcResult<void> OffscreenResult = CreateOffscreenContext();

	if (!OffscreenResult)
		return lcUnexpected(OffscreenResult.error());

	mGlobalOffscreenContext = std::unique_ptr<lcContext>(new(lcContext));

	lcContext* Context = mGlobalOffscreenContext.get();

	Context->SetOffscreenContext();

	lcInitializeGLExtensions(mOffscreenContext.get());

	if (!CreateRenderResources)
		return lcResult<void>();

	if (!Context->hasOpenGLFeature(QOpenGLFunctions::Shaders))
	{
		mGlobalOffscreenContext.reset();
		DestroyOffscreenContext();

		return lcUnexpected(QCoreApplication::translate("lcContext", "OpenGL shader support is required."));
	}

	const lcResult<void> ResourcesResult = Context->CreateResources();

	if (!ResourcesResult)
	{
		Context->DestroyResources();
		mGlobalOffscreenContext.reset();
		DestroyOffscreenContext();

		return lcUnexpected(ResourcesResult.error());
	}

	gStringCache.Initialize(Context);

	if (SupportsMaterial(lcMaterialType::TextSDF))
		gTexFont.Initialize(Context);

	lcView::CreateResources(Context);
	lcViewManipulator::CreateResources(Context);

	if (SupportsMaterial(lcMaterialType::UnlitViewSphere))
		lcViewSphere::CreateResources(Context);

	mRenderResourcesCreated = true;

	if (!SupportsSelectionOutline())
		lcGetPreferences().mHighlightRenderMode = lcHighlightRenderMode::Tint;

	return lcResult<void>();
}

void lcContext::ShutdownRenderer()
{
	if (!mGlobalOffscreenContext)
		return;

	mGlobalOffscreenContext->MakeCurrent();
	lcContext* Context = mGlobalOffscreenContext.get();

	if (mRenderResourcesCreated)
	{
		gStringCache.Reset();
		gTexFont.Reset();

		lcView::DestroyResources(Context);
		Context->DestroyResources();
		lcViewManipulator::DestroyResources(Context);
		lcViewSphere::DestroyResources(Context);

		mRenderResourcesCreated = false;
	}

	mGlobalOffscreenContext.reset();

	lcContext::DestroyOffscreenContext();
}

lcContext* lcContext::GetGlobalOffscreenContext()
{
	return mGlobalOffscreenContext.get();
}

lcResult<void> lcContext::CreateOffscreenContext()
{
	std::unique_ptr<QOpenGLContext> OffscreenContext(new QOpenGLContext());

	OffscreenContext->setShareContext(QOpenGLContext::globalShareContext());

	if (!OffscreenContext->create() || !OffscreenContext->isValid())
		return lcUnexpected(QCoreApplication::translate("lcContext", "Could not create the OpenGL context."));

	std::unique_ptr<QOffscreenSurface> OffscreenSurface(new QOffscreenSurface());

	OffscreenSurface->create();

	if (!OffscreenSurface->isValid())
		return lcUnexpected(QCoreApplication::translate("lcContext", "Could not create the offscreen OpenGL surface."));

	if (!OffscreenContext->makeCurrent(OffscreenSurface.get()))
		return lcUnexpected(QCoreApplication::translate("lcContext", "Could not make the offscreen OpenGL context current."));

	mOffscreenContext = std::move(OffscreenContext);
	mOffscreenSurface = std::move(OffscreenSurface);

	return lcResult<void>();
}

void lcContext::DestroyOffscreenContext()
{
	mOffscreenSurface.reset();
	mOffscreenContext.reset();
}

lcResult<void> lcContext::CreateShaderPrograms()
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
		":/resources/shaders/unlit_texture_modulate_vs.glsl",  // TextSDF
		":/resources/shaders/unlit_texture_decal_vs.glsl",     // UnlitTextureDecal
		":/resources/shaders/unlit_vertex_color_vs.glsl",      // UnlitVertexColor
		":/resources/shaders/unlit_view_sphere_vs.glsl",       // UnlitViewSphere
		":/resources/shaders/fakelit_color_vs.glsl",           // FakeLitColor
		":/resources/shaders/fakelit_texture_decal_vs.glsl",   // FakeLitTextureDecal
		":/resources/shaders/unlit_texture_modulate_vs.glsl"   // OutlineComposite
	};

	LC_ARRAY_SIZE_CHECK(VertexShaders, lcMaterialType::Count);

	const char* const FragmentShaders[] =
	{
		":/resources/shaders/unlit_color_ps.glsl",             // UnlitColor
		":/resources/shaders/unlit_color_conditional_ps.glsl", // UnlitColorConditional
		":/resources/shaders/unlit_texture_modulate_ps.glsl",  // UnlitTextureModulate
		":/resources/shaders/text_sdf_ps.glsl",                // TextSDF
		":/resources/shaders/unlit_texture_decal_ps.glsl",     // UnlitTextureDecal
		":/resources/shaders/unlit_vertex_color_ps.glsl",      // UnlitVertexColor
		":/resources/shaders/unlit_view_sphere_ps.glsl",       // UnlitViewSphere
		":/resources/shaders/fakelit_color_ps.glsl",           // FakeLitColor
		":/resources/shaders/fakelit_texture_decal_ps.glsl",   // FakeLitTextureDecal
		":/resources/shaders/outline_composite_ps.glsl"      // OutlineComposite
	};

	LC_ARRAY_SIZE_CHECK(FragmentShaders, lcMaterialType::Count);

	const auto LoadShader = [this, ShaderPrefix](const char* FileName, GLuint ShaderType) -> lcResult<GLuint>
	{
		QFile ShaderFile(FileName);

		if (!ShaderFile.open(QIODevice::ReadOnly))
			return lcUnexpected(QCoreApplication::translate("lcContext", "Could not read shader %1: %2").arg(QString::fromUtf8(FileName), ShaderFile.errorString()));

		QByteArray Data = ShaderPrefix + ShaderFile.readAll();
		const char* Source = Data.constData();

		const GLuint Shader = glCreateShader(ShaderType);

		if (!Shader)
			return lcUnexpected(QCoreApplication::translate("lcContext", "Could not create shader %1.").arg(QString::fromUtf8(FileName)));

		glShaderSource(Shader, 1, &Source, nullptr);
		glCompileShader(Shader);

		GLint ShaderCompiled = 0;
		glGetShaderiv(Shader, GL_COMPILE_STATUS, &ShaderCompiled);

		if (ShaderCompiled == GL_FALSE)
		{
			GLint Length = 0;
			glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &Length);

			QByteArray InfoLog(Length, '\0');

			if (Length > 0)
				glGetShaderInfoLog(Shader, Length, nullptr, InfoLog.data());

			glDeleteShader(Shader);

			return lcUnexpected(QCoreApplication::translate("lcContext", "Could not compile shader %1:\n%2").arg(QString::fromUtf8(FileName), QString::fromUtf8(InfoLog.constData())));
		}

		return Shader;
	};

	for (int MaterialType = 0; MaterialType < static_cast<int>(lcMaterialType::Count); MaterialType++)
	{
		bool Optional = false;

		switch (static_cast<lcMaterialType>(MaterialType))
		{
		case lcMaterialType::UnlitColorConditional:
		case lcMaterialType::TextSDF:
		case lcMaterialType::UnlitViewSphere:
		case lcMaterialType::FakeLitColor:
		case lcMaterialType::FakeLitTextureDecal:
		case lcMaterialType::OutlineComposite:
			Optional = true;
			break;

		case lcMaterialType::UnlitColor:
		case lcMaterialType::UnlitTextureModulate:
		case lcMaterialType::UnlitTextureDecal:
		case lcMaterialType::UnlitVertexColor:
		case lcMaterialType::Count:
			break;
		}

		const lcResult<GLuint> VertexResult = LoadShader(VertexShaders[MaterialType], GL_VERTEX_SHADER);

		if (!VertexResult)
		{
			if (Optional)
			{
				qWarning().noquote() << VertexResult.error();
				continue;
			}

			return lcUnexpected(VertexResult.error());
		}

		const GLuint VertexShader = VertexResult.value();
		const lcResult<GLuint> FragmentResult = LoadShader(FragmentShaders[MaterialType], GL_FRAGMENT_SHADER);

		if (!FragmentResult)
		{
			glDeleteShader(VertexShader);

			if (Optional)
			{
				qWarning().noquote() << FragmentResult.error();
				continue;
			}

			return lcUnexpected(FragmentResult.error());
		}

		const GLuint FragmentShader = FragmentResult.value();
		const GLuint Program = glCreateProgram();

		if (!Program)
		{
			glDeleteShader(VertexShader);
			glDeleteShader(FragmentShader);

			const QString Error = QCoreApplication::translate("lcContext", "Could not create the shader program for %1 and %2.").arg(QString::fromUtf8(VertexShaders[MaterialType]), QString::fromUtf8(FragmentShaders[MaterialType]));

			if (Optional)
			{
				qWarning().noquote() << Error;
				continue;
			}

			return lcUnexpected(Error);
		}

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

			QByteArray InfoLog(Length, '\0');

			if (Length > 0)
				glGetProgramInfoLog(Program, Length, nullptr, InfoLog.data());

			glDeleteProgram(Program);

			const QString Error = QCoreApplication::translate("lcContext", "Could not link the shader program for %1 and %2:\n%3").arg(QString::fromUtf8(VertexShaders[MaterialType]), QString::fromUtf8(FragmentShaders[MaterialType]), QString::fromUtf8(InfoLog.constData()));

			if (Optional)
			{
				qWarning().noquote() << Error;
				continue;
			}

			return lcUnexpected(Error);
		}

		mPrograms[MaterialType].Object = Program;

		mPrograms[MaterialType].WorldViewProjectionMatrixLocation = glGetUniformLocation(Program, "WorldViewProjectionMatrix");
		mPrograms[MaterialType].WorldMatrixLocation = glGetUniformLocation(Program, "WorldMatrix");
		mPrograms[MaterialType].MaterialColorLocation = glGetUniformLocation(Program, "MaterialColor");
		mPrograms[MaterialType].LightPositionLocation = glGetUniformLocation(Program, "LightPosition");
		mPrograms[MaterialType].EyePositionLocation = glGetUniformLocation(Program, "EyePosition");
		mPrograms[MaterialType].HighlightParamsLocation = glGetUniformLocation(Program, "HighlightParams");
		mPrograms[MaterialType].OutlineParamsLocation = glGetUniformLocation(Program, "OutlineParams");
		mPrograms[MaterialType].TextHaloColorLocation = glGetUniformLocation(Program, "TextHaloColor");

		const GLint TextureLocation = glGetUniformLocation(Program, "Texture");

		if (TextureLocation != -1)
		{
			glUseProgram(Program);
			glUniform1i(TextureLocation, 0);
			glUseProgram(0);
		}

		const GLint VisibleTextureLocation = glGetUniformLocation(Program, "VisibleTexture");

		if (VisibleTextureLocation != -1)
		{
			glUseProgram(Program);
			glUniform1i(VisibleTextureLocation, 1);
			glUseProgram(0);
		}
	}

	return lcResult<void>();
}

lcResult<void> lcContext::CreateResources()
{
	return CreateShaderPrograms();
}

void lcContext::DestroyResources()
{
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

	SetVertexAttribPointer(lcProgramAttrib::Position, 3, GL_FLOAT, false, 0, nullptr);
	EnableVertexAttrib(lcProgramAttrib::Position);

	DisableVertexAttrib(lcProgramAttrib::Normal);
	SetVertexAttribPointer(lcProgramAttrib::Normal, 4, GL_BYTE, true, 0, nullptr);
	DisableVertexAttrib(lcProgramAttrib::TexCoord);
	SetVertexAttribPointer(lcProgramAttrib::TexCoord, 2, GL_FLOAT, false, 0, nullptr);
	DisableVertexAttrib(lcProgramAttrib::Color);
	SetVertexAttribPointer(lcProgramAttrib::Color, 4, GL_FLOAT, false, 0, nullptr);

	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = nullptr;
	mIndexBufferPointer = nullptr;

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

	glUseProgram(0);
	mMaterialType = lcMaterialType::Count;
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

bool lcContext::SupportsSelectionOutline()
{
	return gSupportsFramebufferObject &&
		mPrograms[static_cast<int>(lcMaterialType::UnlitColor)].Object &&
		mPrograms[static_cast<int>(lcMaterialType::OutlineComposite)].Object;
}

int lcContext::GetMaxTextureSize()
{
	GLint MaxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);
	return MaxTextureSize;
}

std::unique_ptr<QOpenGLFramebufferObject> lcContext::CreateFramebuffer(int Width, int Height)
{
	if (!gSupportsFramebufferObject || Width <= 0 || Height <= 0)
		return nullptr;

	const int MaxTextureSize = GetMaxTextureSize();
	GLint MaxRenderbufferSize = 0;

	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &MaxRenderbufferSize);

	if (Width > MaxTextureSize || Height > MaxTextureSize || Width > MaxRenderbufferSize || Height > MaxRenderbufferSize)
		return nullptr;

	lcScopedGLContextState GLState;
	QOpenGLFramebufferObjectFormat Format;

	Format.setAttachment(QOpenGLFramebufferObject::Depth);

	std::unique_ptr<QOpenGLFramebufferObject> Framebuffer = std::make_unique<QOpenGLFramebufferObject>(QSize(Width, Height), Format);

	if (!Framebuffer->isValid())
	{
		glBindTexture(GL_TEXTURE_2D, mTexture2D);

		return nullptr;
	}

	glBindTexture(GL_TEXTURE_2D, Framebuffer->texture());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, mTexture2D);

	return Framebuffer;
}

bool lcContext::TryShareDepthAttachment(GLuint SourceFramebuffer, QOpenGLFramebufferObject& TargetFramebuffer)
{
	// Share single-sample depth directly; other framebuffers use a depth prepass.
	if (!SourceFramebuffer)
		return false;

	glBindFramebuffer(GL_FRAMEBUFFER, SourceFramebuffer);

	GLint Samples = 0;

	glGetIntegerv(GL_SAMPLES, &Samples);

	if (Samples != 0)
		return false;

	GLint ObjectType = 0;
	GLint DepthBuffer = 0;

	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &ObjectType);

	if (ObjectType != GL_RENDERBUFFER)
		return false;

	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &DepthBuffer);

	GLint PreviousRenderbuffer = 0;
	GLint Width = 0;
	GLint Height = 0;

	glGetIntegerv(GL_RENDERBUFFER_BINDING, &PreviousRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, DepthBuffer);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &Width);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &Height);
	glBindRenderbuffer(GL_RENDERBUFFER, PreviousRenderbuffer);

	if (QSize(Width, Height) != TargetFramebuffer.size())
		return false;

	TargetFramebuffer.bind();
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, DepthBuffer);

	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

bool lcContext::GenerateOutlineMasks(const lcScene& Scene, QOpenGLFramebufferObject& FullFramebuffer, QOpenGLFramebufferObject& VisibleFramebuffer)
{
	lcScopedGLContextState GLState;
	GLint SceneFramebuffer = 0;
	GLint Viewport[4];

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &SceneFramebuffer);
	glGetIntegerv(GL_VIEWPORT, Viewport);

	const lcMatrix44 WorldMatrix = mWorldMatrix;
	const lcMatrix44 ViewMatrix = mViewMatrix;
	const lcMatrix44 ProjectionMatrix = mProjectionMatrix;
	const float LineWidth = mLineWidth;

	if (!FullFramebuffer.bind() || glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	SetDefaultState();
	SetViewport(0, 0, FullFramebuffer.width(), FullFramebuffer.height());
	ClearColorAndDepth(lcVector4(0.0f, 0.0f, 0.0f, 0.0f));
	Scene.DrawSelectionMask(this);

	if (Scene.HasHighlightedMeshes())
	{
		// Keep highlight coverage independent of selection, including when they overlap.
		EnableDepthTest(false);
		glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);

		Scene.DrawHighlightMask(this);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		EnableDepthTest(true);
	}

	const bool Success = VisibleFramebuffer.bind() && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

	if (Success && Scene.HasSelectedMeshes())
	{
		GLint OwnDepthBuffer = 0;

		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &OwnDepthBuffer);

		const bool SharedDepth = Scene.CanReuseSceneDepth() && TryShareDepthAttachment(SceneFramebuffer, VisibleFramebuffer);

		VisibleFramebuffer.bind();

		if (!SharedDepth)
		{
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, OwnDepthBuffer);

			ClearColorAndDepth(lcVector4(0.0f, 0.0f, 0.0f, 0.0f));

			EnableColorWrite(false);
			SetPolygonOffset(lcPolygonOffset::Opaque);

			Scene.DrawOutlineOccluders(this);

			SetPolygonOffset(lcPolygonOffset::None);
			EnableColorWrite(true);
		}
		else
		{
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		SetDepthWrite(false);

		Scene.DrawSelectionMask(this);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, OwnDepthBuffer);
	}
	else if (Success)
		ClearColorAndDepth(lcVector4(0.0f, 0.0f, 0.0f, 0.0f));

	glBindFramebuffer(GL_FRAMEBUFFER, SceneFramebuffer);

	SetDefaultState();
	SetWorldMatrix(WorldMatrix);
	SetViewMatrix(ViewMatrix);
	SetProjectionMatrix(ProjectionMatrix);
	SetViewport(Viewport[0], Viewport[1], Viewport[2], Viewport[3]);
	SetLineWidth(LineWidth);

	return Success;
}

void lcContext::DrawOutline(QOpenGLFramebufferObject& FullFramebuffer, QOpenGLFramebufferObject& VisibleFramebuffer, float DeviceScale)
{
	const lcMatrix44 WorldMatrix = mWorldMatrix;
	const lcMatrix44 ViewMatrix = mViewMatrix;
	const lcMatrix44 ProjectionMatrix = mProjectionMatrix;
	const float LineWidth = mLineWidth;

	SetDepthWrite(false);
	EnableDepthTest(false);
	EnableColorBlend(true);
	SetWorldMatrix(lcMatrix44Identity());
	SetViewMatrix(lcMatrix44Identity());
	SetProjectionMatrix(lcMatrix44Identity());
	SetMaterial(lcMaterialType::OutlineComposite);

	const lcPreferences& Preferences = lcGetPreferences();

	SetOutlineParams(lcVector4(1.0f / FullFramebuffer.width(), 1.0f / FullFramebuffer.height(), 4.0f * DeviceScale, 0.3f),
		lcVector4FromColor(Preferences.mObjectSelectedColor), lcVector4FromColor(Preferences.mObjectFocusedColor), lcVector4FromColor(Preferences.mHighlightNewPartsColor));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, VisibleFramebuffer.texture());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, FullFramebuffer.texture());
	mTexture2D = FullFramebuffer.texture();

	const float Vertices[] =
	{
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f
	};

	SetVertexBufferPointer(Vertices);
	SetVertexFormat(0, 3, 0, 2, 0, false);

	DrawPrimitives(GL_TRIANGLE_STRIP, 0, 4);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

	SetDefaultState();
	SetWorldMatrix(WorldMatrix);
	SetViewMatrix(ViewMatrix);
	SetProjectionMatrix(ProjectionMatrix);
	SetLineWidth(LineWidth);
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

	glUseProgram(mPrograms[static_cast<int>(MaterialType)].Object);
	mColorDirty = true;
	mWorldMatrixDirty = true; // todo: change dirty to a bitfield and set the lighting constants dirty here
	mViewMatrixDirty = true;
	mHighlightParamsDirty = true;
	mOutlineParamsDirty = true;
	mTextHaloColorDirty = true;
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
	if (mTextureCubeMap == 0)
		return;

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	mTextureCubeMap = 0;
}

void lcContext::UploadTextureImage(GLenum Target, int Level, GLenum Format, const QImage& Image)
{
	const size_t RowBytes = static_cast<size_t>(Image.width()) * (Image.depth() / 8);
	const size_t AlignedRowBytes = (RowBytes + 3) & ~size_t(3);

	if (static_cast<size_t>(Image.bytesPerLine()) == AlignedRowBytes)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glTexImage2D(Target, Level, Format, Image.width(), Image.height(), 0, Format, GL_UNSIGNED_BYTE, Image.constBits());
	}
	else
	{
		std::vector<quint8> PackedPixels(RowBytes * Image.height());
		for (int Row = 0; Row < Image.height(); Row++)
			memcpy(PackedPixels.data() + Row * RowBytes, Image.constScanLine(Row), RowBytes);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(Target, Level, Format, Image.width(), Image.height(), 0, Format, GL_UNSIGNED_BYTE, PackedPixels.data());
	}
}

bool lcContext::UploadTexture(lcTexture* Texture)
{
	for (int Error = 0; Error < 16 && glGetError() != GL_NO_ERROR; Error++)
	{
	}

	GLint UnpackAlignment;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &UnpackAlignment);

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

	const auto FinishUpload = [this, Flags, UnpackAlignment](bool Success)
	{
		if ((Flags & LC_TEXTURE_CUBEMAP) == 0)
			ClearTexture2D();
		else
			ClearTextureCubeMap();

		glPixelStorei(GL_UNPACK_ALIGNMENT, UnpackAlignment);

		const bool NoError = glGetError() == GL_NO_ERROR;
		return Success && NoError;
	};

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

	if (Flags & LC_TEXTURE_CUBEMAP)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	if (gSupportsAnisotropic && FilterFlags == LC_TEXTURE_ANISOTROPIC)
		glTexParameterf(Target, GL_TEXTURE_MAX_ANISOTROPY_EXT, lcMin(4.0f, gMaxAnisotropy));

	int Format;
	switch (Texture->GetImage(0).format())
	{
	default:
		return FinishUpload(false);
	case QImage::Format_Alpha8:
		Format = GL_ALPHA;
		break;
	case QImage::Format_RGB888:
		Format = GL_RGB;
		break;
	case QImage::Format_RGBA8888:
		Format = GL_RGBA;
		break;
	}

	if (Flags & LC_TEXTURE_CUBEMAP)
		Target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;

	const bool LoadMipmaps = (Flags & LC_TEXTURE_MIPMAPS) || FilterFlags >= LC_TEXTURE_BILINEAR;
	const bool HasMipmaps = Texture->GetImageCount() > Faces;
	int CurrentImage = 0;

	for (size_t FaceIdx = 0; FaceIdx < Faces; FaceIdx++)
	{
		const QImage& Image = Texture->GetImage(CurrentImage);

		UploadTextureImage(Target, 0, Format, Image);

		if (LoadMipmaps)
		{
			int Width = Texture->mWidth;
			int Height = Texture->mHeight;
			int Components = Image.depth() / 8;
			GLubyte* MipBuffer = nullptr;

			if (!HasMipmaps)
			{
				MipBuffer = new GLubyte[Width * Height * Components];

				if (!MipBuffer)
					return FinishUpload(false);

				const size_t RowBytes = static_cast<size_t>(Width) * Components;
				for (int Row = 0; Row < Height; Row++)
					memcpy(MipBuffer + Row * RowBytes, Image.constScanLine(Row), RowBytes);
			}

			for (int Level = 1; ((Width != 1) || (Height != 1)); Level++)
			{
				int PrevRowStride = Width * Components;
				int PrevWidth = Width;
				int PrevHeight = Height;

				Width = lcMax(1, Width >> 1);
				Height = lcMax(1, Height >> 1);
				int RowStride = Width * Components;

				if (!HasMipmaps)
				{
					for (int y = 0; y < Height; y++)
					{
						int SrcY0 = y * 2;
						int SrcY1 = lcMin(SrcY0 + 1, PrevHeight - 1);

						for (int x = 0; x < Width; x++)
						{
							int SrcX0 = x * 2;
							int SrcX1 = lcMin(SrcX0 + 1, PrevWidth - 1);

							for (int c = 0; c < Components; c++)
							{
								GLubyte c00 = MipBuffer[SrcX0 * Components + SrcY0 * PrevRowStride + c];
								GLubyte c01 = MipBuffer[SrcX0 * Components + SrcY1 * PrevRowStride + c];
								GLubyte c10 = MipBuffer[SrcX1 * Components + SrcY0 * PrevRowStride + c];
								GLubyte c11 = MipBuffer[SrcX1 * Components + SrcY1 * PrevRowStride + c];

								MipBuffer[x * Components + y * RowStride + c] = (c00 + c01 + c10 + c11) / 4;
							}
						}
					}

					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glTexImage2D(Target, Level, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, MipBuffer);
				}
				else
				{
					const QImage& MipImage = Texture->GetImage(++CurrentImage);

					UploadTextureImage(Target, Level, Format, MipImage);
				}
			}

			delete[] MipBuffer;
		}

		CurrentImage++;
		Target++;
	}

	return FinishUpload(true);
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

	if (mVertexBufferObject)
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		mVertexBufferObject = 0;
	}

	SetVertexAttribPointer(lcProgramAttrib::Position, 3, GL_FLOAT, false, 0, nullptr);

	DisableVertexAttrib(lcProgramAttrib::Normal);
	SetVertexAttribPointer(lcProgramAttrib::Normal, 4, GL_BYTE, true, 0, nullptr);
	DisableVertexAttrib(lcProgramAttrib::TexCoord);
	SetVertexAttribPointer(lcProgramAttrib::TexCoord, 2, GL_FLOAT, false, 0, nullptr);
	DisableVertexAttrib(lcProgramAttrib::Color);
	SetVertexAttribPointer(lcProgramAttrib::Color, 4, GL_FLOAT, false, 0, nullptr);
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
		}
	}
	else
	{
		mVertexBufferPointer = (char*)VertexBuffer.Pointer;
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

	SetVertexAttribPointer(lcProgramAttrib::Position, PositionSize, GL_FLOAT, false, VertexSize, VertexBufferPointer);
	DisableVertexAttrib(lcProgramAttrib::Normal);
	DisableVertexAttrib(lcProgramAttrib::TexCoord);
	DisableVertexAttrib(lcProgramAttrib::Color);
}

void lcContext::SetVertexFormatConditional(int BufferOffset)
{
	constexpr int VertexSize = 12 * sizeof(float);
	const char* VertexBufferPointer = mVertexBufferPointer + BufferOffset;

	SetVertexAttribPointer(lcProgramAttrib::ControlPoint1, 3, GL_FLOAT, false, VertexSize, VertexBufferPointer);
	EnableVertexAttrib(lcProgramAttrib::ControlPoint1);
	SetVertexAttribPointer(lcProgramAttrib::ControlPoint2, 3, GL_FLOAT, false, VertexSize, VertexBufferPointer + 3 * sizeof(float));
	EnableVertexAttrib(lcProgramAttrib::ControlPoint2);
	SetVertexAttribPointer(lcProgramAttrib::ControlPoint3, 3, GL_FLOAT, false, VertexSize, VertexBufferPointer + 6 * sizeof(float));
	EnableVertexAttrib(lcProgramAttrib::ControlPoint3);
	SetVertexAttribPointer(lcProgramAttrib::ControlPoint4, 3, GL_FLOAT, false, VertexSize, VertexBufferPointer + 9 * sizeof(float));
	EnableVertexAttrib(lcProgramAttrib::ControlPoint4);
}

void lcContext::SetVertexFormat(int BufferOffset, int PositionSize, int NormalSize, int TexCoordSize, int ColorSize, bool EnableNormals)
{
	const int VertexSize = (PositionSize + TexCoordSize) * sizeof(float) + NormalSize * sizeof(quint32) + ColorSize;
	const char* VertexBufferPointer = mVertexBufferPointer + BufferOffset;

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
	if (Mesh->mVertexCacheOffset != -1)
	{
		const auto [VertexBuffer, IndexBuffer] = lcGetPiecesLibrary()->GetBuffers();
		const GLuint VertexBufferObject = VertexBuffer.Object;
		const GLuint IndexBufferObject = IndexBuffer.Object;

		if (VertexBufferObject != mVertexBufferObject)
		{
			glBindBuffer(GL_ARRAY_BUFFER_ARB, VertexBufferObject);
			mVertexBufferObject = VertexBufferObject;
			mVertexBufferPointer = nullptr;
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
	}
}

void lcContext::FlushState()
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
				glUniformMatrix4fv(Program.WorldMatrixLocation, 1, false, mWorldMatrix.GetFloats());
		}

		if (mViewMatrixDirty)
		{
			const lcMatrix44 InverseViewMatrix = lcMatrix44AffineInverse(mViewMatrix);
			lcVector3 ViewPosition = lcMul30(-mViewMatrix.GetTranslation(), InverseViewMatrix);

			if (Program.LightPositionLocation != -1)
			{
				lcVector3 LightPosition = ViewPosition + lcMul30(lcVector3(300.0f, 300.0f, 0.0f), InverseViewMatrix);
				glUniform3fv(Program.LightPositionLocation, 1, LightPosition.GetFloats());
			}

			if (Program.EyePositionLocation != -1)
				glUniform3fv(Program.EyePositionLocation, 1, ViewPosition.GetFloats());
		}

		glUniformMatrix4fv(Program.WorldViewProjectionMatrixLocation, 1, false, lcMul(mWorldMatrix, mViewProjectionMatrix).GetFloats());
		mWorldMatrixDirty = false;
		mViewMatrixDirty = false;
		mProjectionMatrixDirty = false;
	}

	if (mColorDirty && Program.MaterialColorLocation != -1)
	{
		glUniform4fv(Program.MaterialColorLocation, 1, mColor.GetFloats());
		mColorDirty = false;
	}

	if (mTextHaloColorDirty && Program.TextHaloColorLocation != -1)
	{
		glUniform4fv(Program.TextHaloColorLocation, 1, mTextHaloColor.GetFloats());
		mTextHaloColorDirty = false;
	}

	if (mHighlightParamsDirty && Program.HighlightParamsLocation != -1)
	{
		glUniform4fv(Program.HighlightParamsLocation, 4, mHighlightParams[0].GetFloats());
		mHighlightParamsDirty = false;
	}

	if (mOutlineParamsDirty && Program.OutlineParamsLocation != -1)
	{
		glUniform4fv(Program.OutlineParamsLocation, 4, mOutlineParams[0].GetFloats());
		mOutlineParamsDirty = false;
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

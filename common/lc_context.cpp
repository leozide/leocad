#include "lc_global.h"
#include "lc_context.h"
#include "lc_glextensions.h"
#include "lc_mesh.h"
#include "lc_texture.h"
#include "lc_colors.h"
#include "lc_mainwindow.h"
#include "lc_library.h"

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

lcProgram lcContext::mPrograms[LC_NUM_MATERIALS];

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
	mTexture2DMS = 0;
	mLineWidth = 1.0f;
#ifndef LC_OPENGLES
	mMatrixMode = GL_MODELVIEW;
	mTextureEnabled = false;
#endif

	mFramebufferObject = 0;
	mFramebufferObjectMS = 0;
	mFramebufferTexture = 0;
	mFramebufferTextureMS = 0;
	mDepthRenderbufferObject = 0;

	mColor = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);
	mWorldMatrix = lcMatrix44Identity();
	mViewMatrix = lcMatrix44Identity();
	mProjectionMatrix = lcMatrix44Identity();
	mViewProjectionMatrix = lcMatrix44Identity();
	mColorDirty = false;
	mWorldMatrixDirty = false;
	mViewMatrixDirty = false;
	mProjectionMatrixDirty = false;
	mViewProjectionMatrixDirty = false;

	mMaterialType = LC_NUM_MATERIALS;
}

lcContext::~lcContext()
{
}

void lcContext::CreateShaderPrograms()
{
#ifndef LC_OPENGLES
#define LC_SHADER_VERSION "#version 110\n#define mediump\n"
#define LC_VERTEX_INPUT "attribute "
#define LC_VERTEX_OUTPUT "varying "
#define LC_PIXEL_INPUT "varying "
#define LC_PIXEL_OUTPUT
#else
#define LC_SHADER_VERSION "#version 300 es\n#define texture2D texture\n"
#define LC_VERTEX_INPUT "in "
#define LC_VERTEX_OUTPUT "out "
#define LC_PIXEL_INPUT "in mediump "
#define LC_PIXEL_OUTPUT "#define gl_FragColor FragColor\nout mediump vec4 gl_FragColor;\n"
#endif
#define LC_PIXEL_FAKE_LIGHTING \
	"	vec3 Normal = normalize(PixelNormal);\n" \
	"	vec3 LightDirection = normalize(PixelPosition - LightPosition);" \
	"	vec3 VertexToEye = normalize(EyePosition - PixelPosition);\n" \
	"	vec3 LightReflect = normalize(reflect(-LightDirection, Normal));\n" \
	"	float Specular = abs(dot(VertexToEye, LightReflect));\n" \
	"	Specular = min(pow(Specular, 8.0), 1.0) * 0.25;\n" \
	"	vec3 SpecularColor = vec3(Specular, Specular, Specular);\n" \
	"	float Diffuse = min(abs(dot(Normal, LightDirection)) * 0.6 + 0.65, 1.0);\n"

	const char* VertexShaders[LC_NUM_MATERIALS] =
	{
		// LC_MATERIAL_UNLIT_COLOR
		LC_SHADER_VERSION
		LC_VERTEX_INPUT "vec3 VertexPosition;\n"
		"uniform mat4 WorldViewProjectionMatrix;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);\n"
		"}\n",
		// LC_MATERIAL_UNLIT_TEXTURE_MODULATE
		LC_SHADER_VERSION
		LC_VERTEX_INPUT "vec3 VertexPosition;\n"
		LC_VERTEX_INPUT "vec2 VertexTexCoord;\n"
		LC_VERTEX_OUTPUT "vec2 PixelTexCoord;\n"
		"uniform mat4 WorldViewProjectionMatrix;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);\n"
		"	PixelTexCoord = VertexTexCoord;\n"
		"}\n",
		// LC_MATERIAL_UNLIT_TEXTURE_DECAL
		LC_SHADER_VERSION
		LC_VERTEX_INPUT "vec3 VertexPosition;\n"
		LC_VERTEX_INPUT "vec2 VertexTexCoord;\n"
		LC_VERTEX_OUTPUT "vec2 PixelTexCoord;\n"
		"uniform mat4 WorldViewProjectionMatrix;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);\n"
		"	PixelTexCoord = VertexTexCoord;\n"
		"}\n",
		// LC_MATERIAL_UNLIT_VERTEX_COLOR
		LC_SHADER_VERSION
		LC_VERTEX_INPUT "vec3 VertexPosition;\n"
		LC_VERTEX_INPUT "vec4 VertexColor;\n"
		LC_VERTEX_OUTPUT "vec4 PixelColor;\n"
		"uniform mat4 WorldViewProjectionMatrix;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);\n"
		"	PixelColor = VertexColor;\n"
		"}\n",
		// LC_MATERIAL_FAKELIT_COLOR
		LC_SHADER_VERSION
		LC_VERTEX_INPUT "vec3 VertexPosition;\n"
		LC_VERTEX_INPUT "vec3 VertexNormal;\n"
		LC_VERTEX_OUTPUT "vec3 PixelPosition;\n"
		LC_VERTEX_OUTPUT "vec3 PixelNormal;\n"
		"uniform mat4 WorldViewProjectionMatrix;\n"
		"uniform mat4 WorldMatrix;\n"
		"void main()\n"
		"{\n"
		"	PixelPosition = (WorldMatrix * vec4(VertexPosition, 1.0)).xyz;\n"
		"   PixelNormal = (WorldMatrix * vec4(VertexNormal, 0.0)).xyz;\n"
		"	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);\n"
		"}\n",
		// LC_MATERIAL_FAKELIT_TEXTURE_DECAL
		LC_SHADER_VERSION
		LC_VERTEX_INPUT "vec3 VertexPosition;\n"
		LC_VERTEX_INPUT "vec3 VertexNormal;\n"
		LC_VERTEX_INPUT "vec2 VertexTexCoord;\n"
		LC_VERTEX_OUTPUT "vec3 PixelPosition;\n"
		LC_VERTEX_OUTPUT "vec3 PixelNormal;\n"
		LC_VERTEX_OUTPUT "vec2 PixelTexCoord;\n"
		"uniform mat4 WorldViewProjectionMatrix;\n"
		"uniform mat4 WorldMatrix;\n"
		"void main()\n"
		"{\n"
		"	PixelPosition = (WorldMatrix * vec4(VertexPosition, 1.0)).xyz;\n"
		"   PixelNormal = (WorldMatrix * vec4(VertexNormal, 0.0)).xyz;\n"
		"	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);\n"
		"	PixelTexCoord = VertexTexCoord;\n"
		"}\n"
	};

	const char* FragmentShaders[LC_NUM_MATERIALS] =
	{
		// LC_MATERIAL_UNLIT_COLOR
		LC_SHADER_VERSION
		LC_PIXEL_OUTPUT
		"uniform mediump vec4 MaterialColor;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = MaterialColor;\n"
		"}\n",
		// LC_MATERIAL_UNLIT_TEXTURE_MODULATE
		LC_SHADER_VERSION
		LC_PIXEL_INPUT "vec2 PixelTexCoord;\n"
		LC_PIXEL_OUTPUT
		"uniform mediump vec4 MaterialColor;\n"
		"uniform sampler2D Texture;\n"
		"void main()\n"
		"{\n"
		"	vec4 TexelColor = texture2D(Texture, PixelTexCoord);"
		"	gl_FragColor = vec4(MaterialColor.rgb, TexelColor.a * MaterialColor.a);\n"
		"}\n",
		// LC_MATERIAL_UNLIT_TEXTURE_DECAL
		LC_SHADER_VERSION
		LC_PIXEL_INPUT "vec2 PixelTexCoord;\n"
		LC_PIXEL_OUTPUT
		"uniform mediump vec4 MaterialColor;\n"
		"uniform sampler2D Texture;\n"
		"void main()\n"
		"{\n"
		"	vec4 TexelColor = texture2D(Texture, PixelTexCoord);"
		"	gl_FragColor = vec4(mix(MaterialColor.xyz, TexelColor.xyz, TexelColor.a), MaterialColor.a);\n"
		"}\n",
		// LC_MATERIAL_UNLIT_VERTEX_COLOR
		LC_SHADER_VERSION
		LC_PIXEL_INPUT "vec4 PixelColor;\n"
		LC_PIXEL_OUTPUT
		"void main()\n"
		"{\n"
		"	gl_FragColor = PixelColor;\n"
		"}\n",
		// LC_MATERIAL_FAKELIT_COLOR
		LC_SHADER_VERSION
		LC_PIXEL_INPUT "vec3 PixelPosition;\n"
		LC_PIXEL_INPUT "vec3 PixelNormal;\n"
		LC_PIXEL_OUTPUT
		"uniform mediump vec4 MaterialColor;\n"
		"uniform mediump vec3 LightPosition;\n"
		"uniform mediump vec3 EyePosition;\n"
		"void main()\n"
		"{\n"
		LC_PIXEL_FAKE_LIGHTING
		"	vec3 DiffuseColor = MaterialColor.rgb * Diffuse;\n"
		"	gl_FragColor = vec4(DiffuseColor + SpecularColor, MaterialColor.a);\n"
		"}\n",
		// LC_MATERIAL_FAKELIT_TEXTURE_DECAL
		LC_SHADER_VERSION
		LC_PIXEL_INPUT "vec3 PixelPosition;\n"
		LC_PIXEL_INPUT "vec3 PixelNormal;\n"
		LC_PIXEL_INPUT "vec2 PixelTexCoord;\n"
		LC_PIXEL_OUTPUT
		"uniform mediump vec4 MaterialColor;\n"
		"uniform mediump vec3 LightPosition;\n"
		"uniform mediump vec3 EyePosition;\n"
		"uniform sampler2D Texture;\n"
		"void main()\n"
		"{\n"
		LC_PIXEL_FAKE_LIGHTING
		"	vec4 TexelColor = texture2D(Texture, PixelTexCoord);"
		"	vec3 DiffuseColor = mix(MaterialColor.xyz, TexelColor.xyz, TexelColor.a) * Diffuse;\n"
		"	gl_FragColor = vec4(DiffuseColor + SpecularColor, MaterialColor.a);\n"
		"}\n"
	};

	for (int MaterialType = 0; MaterialType < LC_NUM_MATERIALS; MaterialType++)
	{
		GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(VertexShader, 1, &VertexShaders[MaterialType], nullptr);
		glCompileShader(VertexShader);

#ifndef QT_NO_DEBUG
		GLint VertexShaderCompiled = 0;
		glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &VertexShaderCompiled);

		if (VertexShaderCompiled == GL_FALSE)
		{
			GLint Length = 0;
			glGetShaderiv(VertexShader, GL_INFO_LOG_LENGTH, &Length);

			QByteArray InfoLog;
			InfoLog.resize(Length);
			glGetShaderInfoLog(VertexShader, Length, &Length, InfoLog.data());

			qDebug() << InfoLog;
		}
#endif

		GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(FragmentShader, 1, &FragmentShaders[MaterialType], nullptr);
		glCompileShader(FragmentShader);

#ifndef QT_NO_DEBUG
		GLint FragmentShaderCompiled = 0;
		glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &FragmentShaderCompiled);

		if (FragmentShaderCompiled == GL_FALSE)
		{
			GLint Length = 0;
			glGetShaderiv(FragmentShader, GL_INFO_LOG_LENGTH, &Length);

			QByteArray InfoLog;
			InfoLog.resize(Length);
			glGetShaderInfoLog(FragmentShader, Length, &Length, InfoLog.data());

			qDebug() << InfoLog;
		}
#endif

		GLuint Program = glCreateProgram();

		glAttachShader(Program, VertexShader);
		glAttachShader(Program, FragmentShader);

		glBindAttribLocation(Program, LC_ATTRIB_POSITION, "VertexPosition");
		glBindAttribLocation(Program, LC_ATTRIB_NORMAL, "VertexNormal");
		glBindAttribLocation(Program, LC_ATTRIB_TEXCOORD, "VertexTexCoord");
		glBindAttribLocation(Program, LC_ATTRIB_COLOR, "VertexColor");

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

	for (int MaterialType = 0; MaterialType < LC_NUM_MATERIALS; MaterialType++)
	{
		glDeleteProgram(mPrograms[MaterialType].Object);
		mPrograms[MaterialType].Object = 0;
	}
}

void lcContext::SetDefaultState()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);

	if (gSupportsVertexBufferObject)
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}

	if (gSupportsShaderObjects)
	{
		glEnableVertexAttribArray(LC_ATTRIB_POSITION);
		glDisableVertexAttribArray(LC_ATTRIB_NORMAL);
		glDisableVertexAttribArray(LC_ATTRIB_TEXCOORD);
		glDisableVertexAttribArray(LC_ATTRIB_COLOR);
	}
	else
	{
#ifndef LC_OPENGLES
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		glVertexPointer(3, GL_FLOAT, 0, nullptr);
		glNormalPointer(GL_BYTE, 0, nullptr);
		glTexCoordPointer(2, GL_FLOAT, 0, nullptr);
		glColorPointer(4, GL_FLOAT, 0, nullptr);
#endif
	}

	mNormalEnabled = false;
	mTexCoordEnabled = false;
	mColorEnabled = false;

	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = nullptr;
	mIndexBufferPointer = nullptr;
	mVertexBufferOffset = (char*)~0;

	glBindTexture(GL_TEXTURE_2D, 0);
	mTexture2D = 0;
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	mTexture2DMS = 0;

	glLineWidth(1.0f);
	mLineWidth = 1.0f;

	if (gSupportsShaderObjects)
	{
		glUseProgram(0);
		mMaterialType = LC_NUM_MATERIALS;
	}
	else
	{
#ifndef LC_OPENGLES
		glMatrixMode(GL_MODELVIEW);
		mMatrixMode = GL_MODELVIEW;
		glShadeModel(GL_FLAT);

		glDisable(GL_TEXTURE_2D);
		mTextureEnabled = false;
#endif
	}
}

void lcContext::ClearResources()
{
	ClearVertexBuffer();
	ClearIndexBuffer();
	BindTexture2D(0);
	BindTexture2DMS(0);
}

void lcContext::SetMaterial(lcMaterialType MaterialType)
{
	if (MaterialType == mMaterialType)
		return;

	mMaterialType = MaterialType;

	if (gSupportsShaderObjects)
	{
		glUseProgram(mPrograms[MaterialType].Object);
		mColorDirty = true;
		mWorldMatrixDirty = true; // todo: change dirty to a bitfield and set the lighting constants dirty here
		mViewMatrixDirty = true;
	}
	else
	{
#ifndef LC_OPENGLES
		switch (MaterialType)
		{
		case LC_MATERIAL_UNLIT_TEXTURE_MODULATE:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			if (!mTextureEnabled)
			{
				glEnable(GL_TEXTURE_2D);
				mTextureEnabled = true;
			}
			break;

		case LC_MATERIAL_FAKELIT_TEXTURE_DECAL:
		case LC_MATERIAL_UNLIT_TEXTURE_DECAL:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

			if (!mTextureEnabled)
			{
				glEnable(GL_TEXTURE_2D);
				mTextureEnabled = true;
			}
			break;

		case LC_MATERIAL_UNLIT_COLOR:
		case LC_MATERIAL_UNLIT_VERTEX_COLOR:
		case LC_MATERIAL_FAKELIT_COLOR:
			if (mTextureEnabled)
			{
				glDisable(GL_TEXTURE_2D);
				mTextureEnabled = false;
			}
			break;

		case LC_NUM_MATERIALS:
			break;
		}
#endif
	}
}

void lcContext::SetViewport(int x, int y, int Width, int Height)
{
	glViewport(x, y, Width, Height);
}

void lcContext::SetLineWidth(float LineWidth)
{
	if (LineWidth == mLineWidth)
		return;

	glLineWidth(LineWidth);
	mLineWidth = LineWidth;
}

void lcContext::SetSmoothShading(bool Smooth)
{
#ifndef LC_OPENGLES
	if (gSupportsShaderObjects)
		glShadeModel(Smooth ? GL_SMOOTH : GL_FLAT);
#endif
}

void lcContext::BindTexture2D(GLuint Texture)
{
	if (mTexture2D == Texture)
		return;

	glBindTexture(GL_TEXTURE_2D, Texture);
	mTexture2D = Texture;
}

void lcContext::BindTexture2DMS(GLuint Texture)
{
	if (mTexture2DMS == Texture)
		return;

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, Texture);
	mTexture2DMS = Texture;
}

void lcContext::SetColor(float Red, float Green, float Blue, float Alpha)
{
	SetColor(lcVector4(Red, Green, Blue, Alpha));
}

void lcContext::SetColorIndex(int ColorIndex)
{
	SetColor(gColorList[ColorIndex].Value);
}

void lcContext::SetColorIndexTinted(int ColorIndex, lcInterfaceColor InterfaceColor)
{
	SetColor((gColorList[ColorIndex].Value + gInterfaceColors[InterfaceColor]) * 0.5f);
}

void lcContext::SetEdgeColorIndex(int ColorIndex)
{
	SetColor(gColorList[ColorIndex].Edge);
}

void lcContext::SetInterfaceColor(lcInterfaceColor InterfaceColor)
{
	SetColor(gInterfaceColors[InterfaceColor]);
}

bool lcContext::BeginRenderToTexture(int Width, int Height)
{
	if (gSupportsFramebufferObjectARB)
	{
		int Samples = gSupportsTexImage2DMultisample ? QGLFormat::defaultFormat().samples() : 1;

		glGenFramebuffers(1, &mFramebufferObject);
		glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferObject);

		glGenTextures(1, &mFramebufferTexture);
		BindTexture2D(mFramebufferTexture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		BindTexture2D(0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFramebufferTexture, 0);

		glGenRenderbuffers(1, &mDepthRenderbufferObject);

		if (Samples == 1)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbufferObject);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, Width, Height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbufferObject);
		}
		else
		{
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				EndRenderToTexture();
				return false;
			}

			glGenFramebuffers(1, &mFramebufferObjectMS);
			glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferObjectMS);

			glGenTextures(1, &mFramebufferTextureMS);
			BindTexture2DMS(mFramebufferTextureMS);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, GL_RGBA, Width, Height, GL_TRUE);
			BindTexture2DMS(0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, mFramebufferTextureMS, 0);

			glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbufferObject);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, Samples, GL_DEPTH_COMPONENT24, Width, Height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbufferObject);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			EndRenderToTexture();
			return false;
		}

		return true;
	}

#ifndef LC_OPENGLES
	if (gSupportsFramebufferObjectEXT)
	{
		glGenFramebuffersEXT(1, &mFramebufferObject); 
		glGenTextures(1, &mFramebufferTexture); 

		BindTexture2D(mFramebufferTexture); 
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFramebufferObject); 
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, mFramebufferTexture, 0); 

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFramebufferObject);

		glGenRenderbuffersEXT(1, &mDepthRenderbufferObject); 
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mDepthRenderbufferObject); 
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, Width, Height);

		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepthRenderbufferObject); 

		BindTexture2D(0);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFramebufferObject);

		if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
		{
			EndRenderToTexture();
			return false;
		}

		return true;
	}
#endif
	
	return false;
}

void lcContext::EndRenderToTexture()
{
	if (gSupportsFramebufferObjectARB)
	{
		glDeleteFramebuffers(1, &mFramebufferObject);
		mFramebufferObject = 0;
		glDeleteTextures(1, &mFramebufferTexture);
		mFramebufferTexture = 0;
		glDeleteFramebuffers(1, &mFramebufferObjectMS);
		mFramebufferObjectMS = 0;
		glDeleteTextures(1, &mFramebufferTextureMS);
		mFramebufferTextureMS = 0;
		glDeleteRenderbuffers(1, &mDepthRenderbufferObject);
		mDepthRenderbufferObject = 0;

		return;
	}

#ifndef LC_OPENGLES
	if (gSupportsFramebufferObjectEXT)
	{
		glDeleteFramebuffersEXT(1, &mFramebufferObject);
		mFramebufferObject = 0;
		glDeleteTextures(1, &mFramebufferTexture);
		mFramebufferTexture = 0;
		glDeleteRenderbuffersEXT(1, &mDepthRenderbufferObject);
		mDepthRenderbufferObject = 0;
	}
#endif
}

QImage lcContext::GetRenderToTextureImage(int Width, int Height)
{
	QImage Image(Width, Height, QImage::Format_ARGB32);

	GetRenderToTextureImage(Width, Height, Image.bits());

	return Image;
}

void lcContext::GetRenderToTextureImage(int Width, int Height, quint8* Buffer)
{
	if (mFramebufferTextureMS)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mFramebufferObjectMS);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFramebufferObject);

		glBlitFrameBuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferObject);
	}

	glFinish();
	glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Buffer);

	if (mFramebufferTextureMS)
		glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferObjectMS);

	for (int y = 0; y < (Height + 1) / 2; y++)
	{
		quint8* Top = Buffer + ((Height - y - 1) * Width * 4);
		quint8* Bottom = Buffer + y * Width * 4;

		for (int x = 0; x < Width; x++)
		{
			QRgb TopColor = qRgba(Top[0], Top[1], Top[2], Top[3]);
			QRgb BottomColor = qRgba(Bottom[0], Bottom[1], Bottom[2], Bottom[3]);

			*(QRgb*)Top = BottomColor;
			*(QRgb*)Bottom = TopColor;

			Top += 4;
			Bottom += 4;
		}
	}
}

bool lcContext::SaveRenderToTextureImage(const QString& FileName, int Width, int Height)
{
	QImage Image = GetRenderToTextureImage(Width, Height);
    QImageWriter Writer(FileName);

	bool Result = Writer.write(Image);

	if (!Result)
		QMessageBox::information(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));

	return Result;
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
		if (mNormalEnabled)
			glDisableVertexAttribArray(LC_ATTRIB_NORMAL);

		if (mTexCoordEnabled)
			glDisableVertexAttribArray(LC_ATTRIB_TEXCOORD);

		if (mColorEnabled)
			glDisableVertexAttribArray(LC_ATTRIB_COLOR);

		glVertexAttribPointer(LC_ATTRIB_POSITION, 3, GL_FLOAT, false, 0, nullptr);
		glVertexAttribPointer(LC_ATTRIB_NORMAL, 4, GL_FLOAT, false, 0, nullptr);
		glVertexAttribPointer(LC_ATTRIB_TEXCOORD, 2, GL_FLOAT, false, 0, nullptr);
		glVertexAttribPointer(LC_ATTRIB_COLOR, 4, GL_FLOAT, false, 0, nullptr);
	}
	else
	{
#ifndef LC_OPENGLES
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
		GLuint VertexBufferObject = VertexBuffer.Object;
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

void lcContext::SetVertexFormatPosition(int PositionSize)
{
	int VertexSize = PositionSize * sizeof(float);
	char* VertexBufferPointer = mVertexBufferPointer;

	if (gSupportsShaderObjects)
	{
		if (mVertexBufferOffset != mVertexBufferPointer)
		{
			glVertexAttribPointer(LC_ATTRIB_POSITION, PositionSize, GL_FLOAT, false, VertexSize, VertexBufferPointer);
			mVertexBufferOffset = VertexBufferPointer;
		}

		if (mNormalEnabled)
		{
			glDisableVertexAttribArray(LC_ATTRIB_NORMAL);
			mNormalEnabled = false;
		}

		if (mTexCoordEnabled)
		{
			glDisableVertexAttribArray(LC_ATTRIB_TEXCOORD);
			mTexCoordEnabled = false;
		}

		if (mColorEnabled)
		{
			glDisableVertexAttribArray(LC_ATTRIB_COLOR);
			mColorEnabled = false;
		}
	}
	else
	{
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
	}
}

void lcContext::SetVertexFormat(int BufferOffset, int PositionSize, int NormalSize, int TexCoordSize, int ColorSize, bool EnableNormals)
{
	int VertexSize = (PositionSize + TexCoordSize + ColorSize) * sizeof(float) + NormalSize * sizeof(quint32);
	char* VertexBufferPointer = mVertexBufferPointer + BufferOffset;

	if (gSupportsShaderObjects)
	{
		if (mVertexBufferOffset != VertexBufferPointer)
		{
			glVertexAttribPointer(LC_ATTRIB_POSITION, PositionSize, GL_FLOAT, false, VertexSize, VertexBufferPointer);
			mVertexBufferOffset = VertexBufferPointer;
		}

		int Offset = PositionSize * sizeof(float);

		if (NormalSize && EnableNormals)
		{
			glVertexAttribPointer(LC_ATTRIB_NORMAL, 4, GL_BYTE, true, VertexSize, VertexBufferPointer + Offset);

			if (!mNormalEnabled)
			{
				glEnableVertexAttribArray(LC_ATTRIB_NORMAL);
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
			glVertexAttribPointer(LC_ATTRIB_TEXCOORD, TexCoordSize, GL_FLOAT, false, VertexSize, VertexBufferPointer + Offset);

			if (!mTexCoordEnabled)
			{
				glEnableVertexAttribArray(LC_ATTRIB_TEXCOORD);
				mTexCoordEnabled = true;
			}

			Offset += 2 * sizeof(float);
		}
		else if (mTexCoordEnabled)
		{
			glDisableVertexAttribArray(LC_ATTRIB_TEXCOORD);
			mTexCoordEnabled = false;
		}

		if (ColorSize)
		{
			glVertexAttribPointer(LC_ATTRIB_COLOR, ColorSize, GL_FLOAT, false, VertexSize, VertexBufferPointer + Offset);

			if (!mColorEnabled)
			{
				glEnableVertexAttribArray(LC_ATTRIB_COLOR);
				mColorEnabled = true;
			}
		}
		else if (mColorEnabled)
		{
			glDisableVertexAttribArray(LC_ATTRIB_COLOR);
			mColorEnabled = false;
		}
	}
	else
	{
#ifndef LC_OPENGLES
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
		GLuint IndexBufferObject = IndexBuffer.Object;
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
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	if (Mesh->mVertexCacheOffset != -1)
	{
		GLuint VertexBufferObject = Library->mVertexBuffer.Object;
		GLuint IndexBufferObject = Library->mIndexBuffer.Object;

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
		const lcProgram& Program = mPrograms[mMaterialType];

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
				lcMatrix44 InverseViewMatrix = lcMatrix44AffineInverse(mViewMatrix);
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
	}
	else
	{
#ifndef LC_OPENGLES
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

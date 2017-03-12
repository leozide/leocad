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

static int lcOpaqueRenderMeshCompare(const void* Elem1, const void* Elem2)
{
	lcRenderMesh* Mesh1 = (lcRenderMesh*)Elem1;
	lcRenderMesh* Mesh2 = (lcRenderMesh*)Elem2;

	if (Mesh1->Mesh < Mesh2->Mesh)
		return -1;

	if (Mesh1->Mesh > Mesh2->Mesh)
		return 1;

	return 0;
}

static int lcTranslucentRenderMeshCompare(const void* Elem1, const void* Elem2)
{
	lcRenderMesh* Mesh1 = (lcRenderMesh*)Elem1;
	lcRenderMesh* Mesh2 = (lcRenderMesh*)Elem2;

	if (Mesh1->Distance < Mesh2->Distance)
		return 1;

	if (Mesh1->Distance > Mesh2->Distance)
		return -1;

	return 0;
}

lcScene::lcScene()
	: mOpaqueMeshes(0, 1024), mTranslucentMeshes(0, 1024), mInterfaceObjects(0, 1024)
{
}

void lcScene::Begin(const lcMatrix44& ViewMatrix)
{
	mViewMatrix = ViewMatrix;
	mOpaqueMeshes.RemoveAll();
	mTranslucentMeshes.RemoveAll();
	mInterfaceObjects.RemoveAll();
}

void lcScene::End()
{
	qsort(&mOpaqueMeshes[0], mOpaqueMeshes.GetSize(), sizeof(mOpaqueMeshes[0]), lcOpaqueRenderMeshCompare);
	qsort(&mTranslucentMeshes[0], mTranslucentMeshes.GetSize(), sizeof(mTranslucentMeshes[0]), lcTranslucentRenderMeshCompare);
}

lcContext::lcContext()
{
	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = NULL;
	mIndexBufferPointer = NULL;
	mVertexBufferOffset = (char*)~0;

	mNormalEnabled = false;
	mTexCoordEnabled = false;
	mColorEnabled = false;

	mTexture = NULL;
	mLineWidth = 1.0f;
#ifndef LC_OPENGLES
	mMatrixMode = GL_MODELVIEW;
#endif

	mFramebufferObject = 0;
	mFramebufferTexture = 0;
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

	mLightingMode = LC_NUM_LIGHTING_MODES;
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
		glShaderSource(VertexShader, 1, &VertexShaders[MaterialType], NULL);
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
		glShaderSource(FragmentShader, 1, &FragmentShaders[MaterialType], NULL);
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
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		glVertexPointer(3, GL_FLOAT, 0, NULL);
		glNormalPointer(GL_BYTE, 0, NULL);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);
		glColorPointer(4, GL_FLOAT, 0, NULL);
	}

	mNormalEnabled = false;
	mTexCoordEnabled = false;
	mColorEnabled = false;

	mVertexBufferObject = 0;
	mIndexBufferObject = 0;
	mVertexBufferPointer = NULL;
	mIndexBufferPointer = NULL;
	mVertexBufferOffset = (char*)~0;

	glDisable(GL_TEXTURE_2D);
	mTexture = NULL;

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
#endif
	}
}

void lcContext::SetLightingMode(lcLightingMode LightingMode)
{
	if (mLightingMode == LightingMode)
		return;

	mLightingMode = LightingMode;
	mMaterialType = LC_NUM_MATERIALS;
}

void lcContext::SetMaterial(lcMaterialType MaterialType)
{
	if (!gSupportsShaderObjects || mMaterialType == MaterialType)
		return;

	glUseProgram(mPrograms[MaterialType].Object);
	mMaterialType = MaterialType;
	mColorDirty = true;
	mWorldMatrixDirty = true; // todo: change dirty to a bitfield and set the lighting constants dirty here
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

void lcContext::SetTextureMode(lcTextureMode TextureMode)
{
#ifndef LC_OPENGLES
	if (!gSupportsShaderObjects)
	{
		const GLenum ModeTable[] = { GL_DECAL, GL_REPLACE, GL_MODULATE };
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, ModeTable[TextureMode]);
	}
#endif
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
		glGenFramebuffers(1, &mFramebufferObject);
		glGenTextures(1, &mFramebufferTexture);
		glGenRenderbuffers(1, &mDepthRenderbufferObject);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFramebufferObject);

		glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFramebufferTexture, 0);

		glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbufferObject);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, Width, Height);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbufferObject);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferObject);

		if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
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

		glBindTexture(GL_TEXTURE_2D, mFramebufferTexture); 
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); 

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFramebufferObject); 
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, mFramebufferTexture, 0); 

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFramebufferObject);

		glGenRenderbuffersEXT(1, &mDepthRenderbufferObject); 
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mDepthRenderbufferObject); 
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, Width, Height);

		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepthRenderbufferObject); 

		glBindTexture(GL_TEXTURE_2D, 0);
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
	quint8* Buffer = Image.bits();

	glFinish();
	glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Buffer);

	for (int y = 0; y < (Height + 1) / 2; y++)
	{
		quint8* Top = Buffer + ((Height - y - 1) * Width * 4);
		quint8* Bottom = Buffer + y * Width * 4;

		for (int x = 0; x < Width; x++)
		{
			quint8 Red = Top[0];
			quint8 Green = Top[1];
			quint8 Blue = Top[2];
			quint8 Alpha = Top[3];

			Top[0] = Bottom[2];
			Top[1] = Bottom[1];
			Top[2] = Bottom[0];
			Top[3] = Bottom[3];

			Bottom[0] = Blue;
			Bottom[1] = Green;
			Bottom[2] = Red;
			Bottom[3] = Alpha;

			Top += 4;
			Bottom += 4;
		}
	}

	return Image;
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

	VertexBuffer.Pointer = NULL;
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

	IndexBuffer.Pointer = NULL;
}

void lcContext::ClearVertexBuffer()
{
	mVertexBufferPointer = NULL;
	mVertexBufferOffset = (char*)~0;

	if (mVertexBufferObject)
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		mVertexBufferObject = 0;
	}

	if (mNormalEnabled)
		glDisableClientState(GL_NORMAL_ARRAY);

	if (mTexCoordEnabled)
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (mColorEnabled)
		glDisableClientState(GL_COLOR_ARRAY);
}

void lcContext::SetVertexBuffer(lcVertexBuffer VertexBuffer)
{
	if (gSupportsVertexBufferObject)
	{
		GLuint VertexBufferObject = VertexBuffer.Object;
		mVertexBufferPointer = NULL;

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

void lcContext::SetVertexFormat(int BufferOffset, int PositionSize, int NormalSize, int TexCoordSize, int ColorSize)
{
	int VertexSize = (PositionSize + TexCoordSize + ColorSize) * sizeof(float) + NormalSize * sizeof(quint32);
	char* VertexBufferPointer = mVertexBufferPointer + BufferOffset;

	if (mVertexBufferOffset != VertexBufferPointer)
	{
		if (gSupportsShaderObjects)
			glVertexAttribPointer(LC_ATTRIB_POSITION, PositionSize, GL_FLOAT, false, VertexSize, VertexBufferPointer);
		else
			glVertexPointer(PositionSize, GL_FLOAT, VertexSize, VertexBufferPointer);

		mVertexBufferOffset = VertexBufferPointer;
	}

	int Offset = PositionSize * sizeof(float);

	if (NormalSize && mLightingMode != LC_LIGHTING_UNLIT)
	{
		if (gSupportsShaderObjects)
		{
			glVertexAttribPointer(LC_ATTRIB_NORMAL, 4, GL_BYTE, true, VertexSize, VertexBufferPointer + Offset);

			if (!mNormalEnabled)
			{
				glEnableVertexAttribArray(LC_ATTRIB_NORMAL);
				mNormalEnabled = true;
			}
		}
		else
		{
			glNormalPointer(GL_BYTE, VertexSize, VertexBufferPointer + Offset);

			if (!mNormalEnabled)
			{
				glEnableClientState(GL_NORMAL_ARRAY);
				mNormalEnabled = true;
			}
		}
	}
	else
	{
		if (gSupportsShaderObjects)
			glDisableVertexAttribArray(LC_ATTRIB_NORMAL);
		else
			glDisableClientState(GL_NORMAL_ARRAY);
		mNormalEnabled = false;
	}

	Offset += NormalSize * sizeof(quint32);

	if (TexCoordSize)
	{
		if (gSupportsShaderObjects)
		{
			glVertexAttribPointer(LC_ATTRIB_TEXCOORD, TexCoordSize, GL_FLOAT, false, VertexSize, VertexBufferPointer + Offset);

			if (!mTexCoordEnabled)
			{
				glEnableVertexAttribArray(LC_ATTRIB_TEXCOORD);
				mTexCoordEnabled = true;
			}
		}
		else
		{
			glTexCoordPointer(TexCoordSize, GL_FLOAT, VertexSize, VertexBufferPointer + Offset);

			if (!mTexCoordEnabled)
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				mTexCoordEnabled = true;
			}
		}

		Offset += 2 * sizeof(float);
	}
	else if (mTexCoordEnabled)
	{
		if (gSupportsShaderObjects)
			glDisableVertexAttribArray(LC_ATTRIB_TEXCOORD);
		else
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		mTexCoordEnabled = false;
	}

	if (ColorSize)
	{
		if (gSupportsShaderObjects)
		{
			glVertexAttribPointer(LC_ATTRIB_COLOR, ColorSize, GL_FLOAT, false, VertexSize, VertexBufferPointer + Offset);

			if (!mColorEnabled)
			{
				glEnableVertexAttribArray(LC_ATTRIB_COLOR);
				mColorEnabled = true;
			}
		}
		else
		{
			glColorPointer(ColorSize, GL_FLOAT, VertexSize, VertexBufferPointer + Offset);

			if (!mColorEnabled)
			{
				glEnableClientState(GL_COLOR_ARRAY);
				mColorEnabled = true;
			}
		}
	}
	else if (mColorEnabled)
	{
		if (gSupportsShaderObjects)
			glDisableVertexAttribArray(LC_ATTRIB_COLOR);
		else
			glDisableClientState(GL_COLOR_ARRAY);
		mColorEnabled = false;
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
		mIndexBufferPointer = NULL;

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

void lcContext::BindMesh(lcMesh* Mesh)
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
			mVertexBufferPointer = NULL;
			mVertexBufferOffset = (char*)~0;
		}

		if (IndexBufferObject != mIndexBufferObject)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, IndexBufferObject);
			mIndexBufferObject = IndexBufferObject;
			mIndexBufferPointer = NULL;
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

void lcContext::UnbindMesh()
{
	if (mTexture)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
		mTexture = NULL;
	}

	if (gSupportsVertexBufferObject)
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		mVertexBufferObject = 0;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		mIndexBufferObject = 0;
	}

	mVertexBufferPointer = NULL;
	mIndexBufferPointer = NULL;

	if (gSupportsShaderObjects)
	{
		glDisableVertexAttribArray(LC_ATTRIB_TEXCOORD);
		glDisableVertexAttribArray(LC_ATTRIB_NORMAL);
		glDisableVertexAttribArray(LC_ATTRIB_COLOR);
	}
	else
	{
		glVertexPointer(3, GL_FLOAT, 0, NULL);
		glNormalPointer(GL_BYTE, 0, NULL);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	}

	mNormalEnabled = false;
	mTexCoordEnabled = false;
	mColorEnabled = false;
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

void lcContext::DrawMeshSection(lcMesh* Mesh, lcMeshSection* Section)
{
	lcTexture* Texture = Section->Texture;
	int VertexBufferOffset = Mesh->mVertexCacheOffset != -1 ? Mesh->mVertexCacheOffset : 0;
	int IndexBufferOffset = Mesh->mIndexCacheOffset != -1 ? Mesh->mIndexCacheOffset : 0;

	if (!Texture)
	{
		SetMaterial(mLightingMode == LC_LIGHTING_UNLIT ? LC_MATERIAL_UNLIT_COLOR : LC_MATERIAL_FAKELIT_COLOR);
		SetVertexFormat(VertexBufferOffset, 3, 1, 0, 0);

		if (mTexture)
		{
			glDisable(GL_TEXTURE_2D);
			mTexture = NULL;
		}
	}
	else
	{
		VertexBufferOffset += Mesh->mNumVertices * sizeof(lcVertex);
		SetMaterial(mLightingMode == LC_LIGHTING_UNLIT ? LC_MATERIAL_UNLIT_TEXTURE_DECAL : LC_MATERIAL_FAKELIT_TEXTURE_DECAL);
		SetVertexFormat(VertexBufferOffset, 3, 1, 2, 0);

		if (Texture != mTexture)
		{
			glBindTexture(GL_TEXTURE_2D, Texture->mTexture);

			if (!mTexture)
			{
				SetTextureMode(LC_TEXTURE_DECAL);
				glEnable(GL_TEXTURE_2D);
			}

			mTexture = Texture;
		}
	}

	const bool DrawConditional = false;

	if (Section->PrimitiveType != LC_MESH_CONDITIONAL_LINES)
	{
		GLenum PrimitiveType = (Section->PrimitiveType == LC_MESH_TRIANGLES || Section->PrimitiveType == LC_MESH_TEXTURED_TRIANGLES) ? GL_TRIANGLES : GL_LINES;
		DrawIndexedPrimitives(PrimitiveType, Section->NumIndices, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset);
	}
	else if (DrawConditional)
	{
		FlushState();
		lcMatrix44 WorldViewProjectionMatrix = lcMul(mWorldMatrix, mViewProjectionMatrix);
		lcVertex* VertexBuffer = (lcVertex*)Mesh->mVertexData;

		if (Mesh->mIndexType == GL_UNSIGNED_SHORT)
		{
			lcuint16* Indices = (lcuint16*)((char*)Mesh->mIndexData + Section->IndexOffset);

			for (int i = 0; i < Section->NumIndices; i += 4)
			{
				lcVector3 p1 = lcMul31(VertexBuffer[Indices[i + 0]].Position, WorldViewProjectionMatrix);
				lcVector3 p2 = lcMul31(VertexBuffer[Indices[i + 1]].Position, WorldViewProjectionMatrix);
				lcVector3 p3 = lcMul31(VertexBuffer[Indices[i + 2]].Position, WorldViewProjectionMatrix);
				lcVector3 p4 = lcMul31(VertexBuffer[Indices[i + 3]].Position, WorldViewProjectionMatrix);

				if (((p1.y - p2.y) * (p3.x - p1.x) + (p2.x - p1.x) * (p3.y - p1.y)) * ((p1.y - p2.y) * (p4.x - p1.x) + (p2.x - p1.x) * (p4.y - p1.y)) >= 0)
					DrawIndexedPrimitives(GL_LINES, 2, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset + i * sizeof(lcuint16));
			}
		}
		else
		{
			lcuint32* Indices = (lcuint32*)((char*)Mesh->mIndexData + Section->IndexOffset);

			for (int i = 0; i < Section->NumIndices; i += 4)
			{
				lcVector3 p1 = lcMul31(VertexBuffer[Indices[i + 0]].Position, WorldViewProjectionMatrix);
				lcVector3 p2 = lcMul31(VertexBuffer[Indices[i + 1]].Position, WorldViewProjectionMatrix);
				lcVector3 p3 = lcMul31(VertexBuffer[Indices[i + 2]].Position, WorldViewProjectionMatrix);
				lcVector3 p4 = lcMul31(VertexBuffer[Indices[i + 3]].Position, WorldViewProjectionMatrix);

				if (((p1.y - p2.y) * (p3.x - p1.x) + (p2.x - p1.x) * (p3.y - p1.y)) * ((p1.y - p2.y) * (p4.x - p1.x) + (p2.x - p1.x) * (p4.y - p1.y)) >= 0)
					DrawIndexedPrimitives(GL_LINES, 2, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset + i * sizeof(lcuint32));
			}
		}
	}
}

void lcContext::DrawOpaqueMeshes(const lcArray<lcRenderMesh>& OpaqueMeshes)
{
	bool DrawLines = lcGetPreferences().mDrawEdgeLines;

	lcGetPiecesLibrary()->UpdateBuffers(this); // TODO: find a better place for this update

	for (int MeshIdx = 0; MeshIdx < OpaqueMeshes.GetSize(); MeshIdx++)
	{
		const lcRenderMesh& RenderMesh = OpaqueMeshes[MeshIdx];
		lcMesh* Mesh = RenderMesh.Mesh;
		int LodIndex = RenderMesh.LodIndex;

		BindMesh(Mesh);
		SetWorldMatrix(RenderMesh.WorldMatrix);

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LodIndex].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LodIndex].Sections[SectionIdx];
			int ColorIndex = Section->ColorIndex;

			if (Section->PrimitiveType == LC_MESH_TRIANGLES || Section->PrimitiveType == LC_MESH_TEXTURED_TRIANGLES)
			{
				if (ColorIndex == gDefaultColor)
					ColorIndex = RenderMesh.ColorIndex;

				if (lcIsColorTranslucent(ColorIndex))
					continue;

				switch (RenderMesh.State)
				{
				case LC_RENDERMESH_NONE:
					SetColorIndex(ColorIndex);
					break;

				case LC_RENDERMESH_SELECTED:
					SetColorIndexTinted(ColorIndex, LC_COLOR_SELECTED);
					break;

				case LC_RENDERMESH_FOCUSED:
					SetColorIndexTinted(ColorIndex, LC_COLOR_FOCUSED);
					break;
				}
			}
			else
			{
				switch (RenderMesh.State)
				{
				case LC_RENDERMESH_NONE:
					if (DrawLines)
					{
						if (ColorIndex == gEdgeColor)
							SetEdgeColorIndex(RenderMesh.ColorIndex);
						else
							SetColorIndex(ColorIndex);
					}
					else
						continue;
					break;

				case LC_RENDERMESH_SELECTED:
					SetInterfaceColor(LC_COLOR_SELECTED);
					break;

				case LC_RENDERMESH_FOCUSED:
					SetInterfaceColor(LC_COLOR_FOCUSED);
					break;
				}
			}

			DrawMeshSection(Mesh, Section);
		}

		const bool DrawNormals = false;

		if (DrawNormals)
		{
			lcVertex* VertexBuffer = (lcVertex*)Mesh->mVertexData;
			lcVector3* Vertices = (lcVector3*)malloc(Mesh->mNumVertices * 2 * sizeof(lcVector3));

			for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
			{
				Vertices[VertexIdx * 2] = VertexBuffer[VertexIdx].Position;
				Vertices[VertexIdx * 2 + 1] = VertexBuffer[VertexIdx].Position + lcUnpackNormal(VertexBuffer[VertexIdx].Normal);
			}

			SetVertexBufferPointer(Vertices);
			SetVertexFormat(0, 3, 0, 0, 0);
			DrawPrimitives(GL_LINES, 0, Mesh->mNumVertices * 2);
			free(Vertices);
		}
	}
}

void lcContext::DrawTranslucentMeshes(const lcArray<lcRenderMesh>& TranslucentMeshes)
{
	if (TranslucentMeshes.IsEmpty())
		return;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);

	for (int MeshIdx = 0; MeshIdx < TranslucentMeshes.GetSize(); MeshIdx++)
	{
		const lcRenderMesh& RenderMesh = TranslucentMeshes[MeshIdx];
		lcMesh* Mesh = RenderMesh.Mesh;
		int LodIndex = RenderMesh.LodIndex;

		BindMesh(Mesh);
		SetWorldMatrix(RenderMesh.WorldMatrix);

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LodIndex].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LodIndex].Sections[SectionIdx];
			int ColorIndex = Section->ColorIndex;

			if (Section->PrimitiveType != LC_MESH_TRIANGLES)
				continue;

			if (ColorIndex == gDefaultColor)
				ColorIndex = RenderMesh.ColorIndex;

			if (!lcIsColorTranslucent(ColorIndex))
				continue;

			switch (RenderMesh.State)
			{
			case LC_RENDERMESH_NONE:
				SetColorIndex(ColorIndex);
				break;

			case LC_RENDERMESH_SELECTED:
				SetColorIndexTinted(ColorIndex, LC_COLOR_SELECTED);
				break;

			case LC_RENDERMESH_FOCUSED:
				SetColorIndexTinted(ColorIndex, LC_COLOR_FOCUSED);
				break;
			}

			DrawMeshSection(Mesh, Section);
		}
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void lcContext::DrawInterfaceObjects(const lcArray<const lcObject*>& InterfaceObjects)
{
	for (int ObjectIdx = 0; ObjectIdx < InterfaceObjects.GetSize(); ObjectIdx++)
		InterfaceObjects[ObjectIdx]->DrawInterface(this);
}

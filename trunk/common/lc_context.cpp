#include "lc_global.h"
#include "lc_context.h"
#include "lc_glextensions.h"
#include "lc_mesh.h"
#include "lc_texture.h"
#include "lc_colors.h"
#include "lc_mainwindow.h"
#include "lc_library.h"

lcProgram lcContext::mPrograms[LC_NUM_PROGRAMS];

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

	mTexCoordEnabled = false;
	mColorEnabled = false;

	mTexture = NULL;
	mLineWidth = 1.0f;
	mMatrixMode = GL_MODELVIEW;

	mFramebufferObject = 0;
	mFramebufferTexture = 0;
	mDepthRenderbufferObject = 0;

	mViewportX = 0;
	mViewportY = 0;
	mViewportWidth = 1;
	mViewportHeight = 1;

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

	mProgramType = LC_NUM_PROGRAMS;
}

lcContext::~lcContext()
{
}

void lcContext::CreateShaderPrograms()
{
	const char* VertexShaders[LC_NUM_PROGRAMS] =
	{
		// LC_PROGRAM_SIMPLE
		"#version 110\n"
		"attribute vec3 VertexPosition;\n"
		"uniform mat4 WorldViewProjectionMatrix;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);\n"
		"}\n",
		// LC_PROGRAM_TEXTURE
		"#version 110\n"
		"attribute vec3 VertexPosition;\n"
		"attribute vec2 VertexTexCoord;\n"
		"varying vec2 PixelTexCoord;\n"
		"uniform mat4 WorldViewProjectionMatrix;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);\n"
		"	PixelTexCoord = VertexTexCoord;\n"
		"}\n",
		// LC_PROGRAM_VERTEX_COLOR
		"#version 110\n"
		"attribute vec3 VertexPosition;\n"
		"attribute vec4 VertexColor;\n"
		"varying vec4 PixelColor;\n"
		"uniform mat4 WorldViewProjectionMatrix;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);\n"
		"	PixelColor = VertexColor;\n"
		"}\n"
	};

	const char* FragmentShaders[LC_NUM_PROGRAMS] =
	{
		// LC_PROGRAM_SIMPLE
		"#version 110\n"
		"uniform vec4 Color;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = Color;\n"
		"}\n",
		// LC_PROGRAM_TEXTURE
		"#version 110\n"
		"varying vec2 PixelTexCoord;\n"
		"uniform vec4 Color;\n"
		"uniform sampler2D Texture;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(Texture, PixelTexCoord) * Color;\n"
		"}\n",
		// LC_PROGRAM_VERTEX_COLOR
		"#version 110\n"
		"varying vec4 PixelColor;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = PixelColor;\n"
		"}\n"
	};

	for (int ProgramIdx = 0; ProgramIdx < LC_NUM_PROGRAMS; ProgramIdx++)
	{
		GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(VertexShader, 1, &VertexShaders[ProgramIdx], NULL);
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
		glShaderSource(FragmentShader, 1, &FragmentShaders[ProgramIdx], NULL);
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

		mPrograms[ProgramIdx].Object = Program;
		mPrograms[ProgramIdx].MatrixLocation = glGetUniformLocation(Program, "WorldViewProjectionMatrix");
		mPrograms[ProgramIdx].ColorLocation = glGetUniformLocation(Program, "Color");
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

	for (int ProgramIdx = 0; ProgramIdx < LC_NUM_PROGRAMS; ProgramIdx++)
		glDeleteProgram(mPrograms[ProgramIdx].Object);
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
		glDisableVertexAttribArray(LC_ATTRIB_TEXCOORD);
		glDisableVertexAttribArray(LC_ATTRIB_COLOR);
	}
	else
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		glVertexPointer(3, GL_FLOAT, 0, NULL);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);
		glColorPointer(4, GL_FLOAT, 0, NULL);
	}

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
		mProgramType = LC_NUM_PROGRAMS;
	}
	else
	{
		glMatrixMode(GL_MODELVIEW);
		mMatrixMode = GL_MODELVIEW;
	}
}

void lcContext::SetProgram(lcProgramType ProgramType)
{
	if (!gSupportsShaderObjects || mProgramType == ProgramType)
		return;

	glUseProgram(mPrograms[ProgramType].Object);
	mProgramType = ProgramType;
	mColorDirty = true;
	mWorldMatrixDirty = true;
}

void lcContext::SetViewport(int x, int y, int Width, int Height)
{
	if (mViewportX == x && mViewportY == y && mViewportWidth == Width && mViewportHeight == Height)
		return;

	glViewport(x, y, Width, Height);

	mViewportX = x;
	mViewportY = y;
	mViewportWidth = Width;
	mViewportHeight = Height;
}

void lcContext::SetLineWidth(float LineWidth)
{
	if (LineWidth == mLineWidth)
		return;

	glLineWidth(LineWidth);
	mLineWidth = LineWidth;
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

	if (gSupportsFramebufferObjectEXT)
	{
		glDeleteFramebuffersEXT(1, &mFramebufferObject);
		mFramebufferObject = 0;
		glDeleteTextures(1, &mFramebufferTexture);
		mFramebufferTexture = 0;
		glDeleteRenderbuffersEXT(1, &mDepthRenderbufferObject);
		mDepthRenderbufferObject = 0;
	}
}

bool lcContext::SaveRenderToTextureImage(const QString& FileName, int Width, int Height)
{
	lcuint8* Buffer = (lcuint8*)malloc(Width * Height * 4);

	glFinish();
	glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Buffer);

	for (int y = 0; y < (Height + 1) / 2; y++)
	{
		lcuint8* Top = Buffer + ((Height - y - 1) * Width * 4);
		lcuint8* Bottom = Buffer + y * Width * 4;

		for (int x = 0; x < Width; x++)
		{
			lcuint8 Red = Top[0];
			lcuint8 Green = Top[1];
			lcuint8 Blue = Top[2];
			lcuint8 Alpha = Top[3];

			Top[0] = Bottom[2];
			Top[1] = Bottom[1];
			Top[2] = Bottom[0];
			Top[3] = Bottom[3];

			Bottom[0] = Blue;
			Bottom[1] = Green;
			Bottom[2] = Red;
			Bottom[3] = Alpha;

			Top += 4;
			Bottom +=4;
		}
	}

    QImageWriter Writer(FileName);
    bool Result = Writer.write(QImage(Buffer, Width, Height, QImage::Format_ARGB32));

	if (!Result)
		QMessageBox::information(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));

	free(Buffer);

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

void lcContext::SetVertexFormat(int BufferOffset, int PositionSize, int TexCoordSize, int ColorSize)
{
	int VertexSize = (PositionSize + TexCoordSize + ColorSize) * sizeof(float);
	char* VertexBufferPointer = mVertexBufferPointer + BufferOffset;

	if (mVertexBufferOffset != VertexBufferPointer)
	{
		if (gSupportsShaderObjects)
			glVertexAttribPointer(LC_ATTRIB_POSITION, PositionSize, GL_FLOAT, false, VertexSize, VertexBufferPointer);
		else
			glVertexPointer(PositionSize, GL_FLOAT, VertexSize, VertexBufferPointer);

		mVertexBufferOffset = VertexBufferPointer;
	}

	if (TexCoordSize)
	{
		if (gSupportsShaderObjects)
		{
			glVertexAttribPointer(LC_ATTRIB_TEXCOORD, TexCoordSize, GL_FLOAT, false, VertexSize, VertexBufferPointer + PositionSize * sizeof(float));

			if (!mTexCoordEnabled)
			{
				glEnableVertexAttribArray(LC_ATTRIB_TEXCOORD);
				mTexCoordEnabled = true;
			}
		}
		else
		{
			glTexCoordPointer(TexCoordSize, GL_FLOAT, VertexSize, VertexBufferPointer + PositionSize * sizeof(float));

			if (!mTexCoordEnabled)
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				mTexCoordEnabled = true;
			}
		}
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
			glVertexAttribPointer(LC_ATTRIB_COLOR, ColorSize, GL_FLOAT, false, VertexSize, VertexBufferPointer + (PositionSize + TexCoordSize) * sizeof(float));

			if (!mColorEnabled)
			{
				glEnableVertexAttribArray(LC_ATTRIB_COLOR);
				mColorEnabled = true;
			}
		}
		else
		{
			glColorPointer(ColorSize, GL_FLOAT, VertexSize, VertexBufferPointer + (PositionSize + TexCoordSize) * sizeof(float));

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
		glDisableVertexAttribArray(LC_ATTRIB_COLOR);
	}
	else
	{
		glVertexPointer(3, GL_FLOAT, 0, NULL);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	}
	mTexCoordEnabled = false;
	mColorEnabled = false;
}

void lcContext::FlushState()
{
	if (gSupportsShaderObjects)
	{
		if (mWorldMatrixDirty || mViewMatrixDirty || mProjectionMatrixDirty)
		{
			if (mViewProjectionMatrixDirty)
			{
				mViewProjectionMatrix = lcMul(mViewMatrix, mProjectionMatrix);
				mViewProjectionMatrixDirty = false;
			}

			glUniformMatrix4fv(mPrograms[mProgramType].MatrixLocation, 1, false, lcMul(mWorldMatrix, mViewProjectionMatrix));
			mWorldMatrixDirty = false;
			mViewMatrixDirty = false;
			mProjectionMatrixDirty = false;
		}

		if (mColorDirty && mPrograms[mProgramType].ColorLocation != -1)
		{
			glUniform4fv(mPrograms[mProgramType].ColorLocation, 1, mColor);
			mColorDirty = false;
		}
	}
	else
	{
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
		SetProgram(LC_PROGRAM_SIMPLE);
		SetVertexFormat(VertexBufferOffset, 3, 0, 0);

		if (mTexture)
		{
			glDisable(GL_TEXTURE_2D);
			mTexture = NULL;
		}
	}
	else
	{
		VertexBufferOffset += Mesh->mNumVertices * sizeof(lcVertex);
		SetProgram(LC_PROGRAM_TEXTURE);
		SetVertexFormat(VertexBufferOffset, 3, 2, 0);

		if (Texture != mTexture)
		{
			glBindTexture(GL_TEXTURE_2D, Texture->mTexture);

			if (!mTexture)
			{
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
				glEnable(GL_TEXTURE_2D);
			}

			mTexture = Texture;
		}
	}

	DrawIndexedPrimitives(Section->PrimitiveType, Section->NumIndices, Mesh->mIndexType, IndexBufferOffset + Section->IndexOffset);
}

void lcContext::DrawOpaqueMeshes(const lcArray<lcRenderMesh>& OpaqueMeshes)
{
	bool DrawLines = lcGetPreferences().mDrawEdgeLines;

	lcGetPiecesLibrary()->UpdateBuffers(this); // TODO: find a better place for this update

	for (int MeshIdx = 0; MeshIdx < OpaqueMeshes.GetSize(); MeshIdx++)
	{
		lcRenderMesh& RenderMesh = OpaqueMeshes[MeshIdx];
		lcMesh* Mesh = RenderMesh.Mesh;
		int LodIndex = RenderMesh.LodIndex;

		BindMesh(Mesh);
		SetWorldMatrix(RenderMesh.WorldMatrix);

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LodIndex].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LodIndex].Sections[SectionIdx];
			int ColorIndex = Section->ColorIndex;

			if (Section->PrimitiveType == GL_TRIANGLES)
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
		lcRenderMesh& RenderMesh = TranslucentMeshes[MeshIdx];
		lcMesh* Mesh = RenderMesh.Mesh;
		int LodIndex = RenderMesh.LodIndex;

		BindMesh(Mesh);
		SetWorldMatrix(RenderMesh.WorldMatrix);

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LodIndex].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LodIndex].Sections[SectionIdx];
			int ColorIndex = Section->ColorIndex;

			if (Section->PrimitiveType != GL_TRIANGLES)
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

#include "lc_global.h"
#include <string.h>
#include <stdio.h>
#include "opengl.h"
#include "lc_glwidget.h"
#include "lc_mainwindow.h"
#include "preview.h"

GLBINDBUFFERARBPROC lcBindBufferARB;
GLDELETEBUFFERSARBPROC lcDeleteBuffersARB;
GLGENBUFFERSARBPROC lcGenBuffersARB;
GLISBUFFERARBPROC lcIsBufferARB;
GLBUFFERDATAARBPROC lcBufferDataARB;
GLBUFFERSUBDATAARBPROC lcBufferSubDataARB;
GLGETBUFFERSUBDATAARBPROC lcGetBufferSubDataARB;
GLMAPBUFFERARBPROC lcMapBufferARB;
GLUNMAPBUFFERARBPROC lcUnmapBufferARB;
GLGETBUFFERPARAMETERIVARBPROC lcGetBufferParameterivARB;
GLGETBUFFERPOINTERVARBPROC lcGetBufferPointervARB;

GLISRENDERBUFFERARBPROC lcIsRenderbufferARB;
GLBINDRENDERBUFFERARBPROC lcBindRenderbufferARB;
GLDELETERENDERBUFFERSARBPROC lcDeleteRenderbuffersARB;
GLGENRENDERBUFFERSARBPROC lcGenRenderbuffersARB;
GLRENDERBUFFERSTORAGEARBPROC lcRenderbufferStorageARB;
GLGETRENDERBUFFERPARAMETERIVARBPROC lcGetRenderbufferParameterivARB;
GLISFRAMEBUFFERARBPROC lcIsFramebufferARB;
GLBINDFRAMEBUFFERARBPROC lcBindFramebufferARB;
GLDELETEFRAMEBUFFERSARBPROC lcDeleteFramebuffersARB;
GLGENFRAMEBUFFERSARBPROC lcGenFramebuffersARB;
GLCHECKFRAMEBUFFERSTATUSARBPROC lcCheckFramebufferStatusARB;
GLFRAMEBUFFERTEXTURE1DARBPROC lcFramebufferTexture1DARB;
GLFRAMEBUFFERTEXTURE2DARBPROC lcFramebufferTexture2DARB;
GLFRAMEBUFFERTEXTURE3DARBPROC lcFramebufferTexture3DARB;
GLFRAMEBUFFERRENDERBUFFERARBPROC lcFramebufferRenderbufferARB;
GLGETFRAMEBUFFERATTACHMENTPARAMETERIVARBPROC lcGetFramebufferAttachmentParameterivARB;
GLGENERATEMIPMAPARBPROC lcGenerateMipmapARB;
GLBLITFRAMEBUFFERARBPROC lcBlitFramebufferARB;
GLRENDERBUFFERSTORAGEMULTISAMPLEARBPROC lcRenderbufferStorageMultisampleARB;
GLFRAMEBUFFERTEXTURELAYERARBPROC lcFramebufferTextureLayerARB;

GLISRENDERBUFFEREXTPROC lcIsRenderbufferEXT;
GLBINDRENDERBUFFEREXTPROC lcBindRenderbufferEXT;
GLDELETERENDERBUFFERSEXTPROC lcDeleteRenderbuffersEXT;
GLGENRENDERBUFFERSEXTPROC lcGenRenderbuffersEXT;
GLRENDERBUFFERSTORAGEEXTPROC lcRenderbufferStorageEXT;
GLGETRENDERBUFFERPARAMETERIVEXTPROC lcGetRenderbufferParameterivEXT;
GLISFRAMEBUFFEREXTPROC lcIsFramebufferEXT;
GLBINDFRAMEBUFFEREXTPROC lcBindFramebufferEXT;
GLDELETEFRAMEBUFFERSEXTPROC lcDeleteFramebuffersEXT;
GLGENFRAMEBUFFERSEXTPROC lcGenFramebuffersEXT;
GLCHECKFRAMEBUFFERSTATUSEXTPROC lcCheckFramebufferStatusEXT;
GLFRAMEBUFFERTEXTURE1DEXTPROC lcFramebufferTexture1DEXT;
GLFRAMEBUFFERTEXTURE2DEXTPROC lcFramebufferTexture2DEXT;
GLFRAMEBUFFERTEXTURE3DEXTPROC lcFramebufferTexture3DEXT;
GLFRAMEBUFFERRENDERBUFFEREXTPROC lcFramebufferRenderbufferEXT;
GLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC lcGetFramebufferAttachmentParameterivEXT;
GLGENERATEMIPMAPEXTPROC lcGenerateMipmapEXT;

bool GL_SupportsVertexBufferObject;
bool GL_UseVertexBufferObject;
bool GL_SupportsFramebufferObjectARB;
bool GL_SupportsFramebufferObjectEXT;
bool GL_SupportsAnisotropic;
GLfloat GL_MaxAnisotropy;

bool GL_ExtensionSupported(const GLubyte* Extensions, const char* Name)
{
	const GLubyte *start;
	GLubyte *where, *terminator;

	where = (GLubyte*)strchr(Name, ' ');
	if (where || *Name == '\0')
		return false;

	if (!Extensions)
		return false;

	for (start = Extensions; ;)
	{
		where = (GLubyte*)strstr((const char*)start, Name);
		if (!where)
			break;

		terminator = where + strlen(Name);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return true;

		start = terminator;
	}

	return false;
}

void GL_InitializeSharedExtensions(lcGLWidget* Window)
{
	const GLubyte* Extensions = glGetString(GL_EXTENSIONS);

	if (GL_ExtensionSupported(Extensions, "GL_EXT_texture_filter_anisotropic"))
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &GL_MaxAnisotropy);

		GL_SupportsAnisotropic = true;
	}

	if (GL_ExtensionSupported(Extensions, "GL_ARB_vertex_buffer_object"))
	{
		lcBindBufferARB = (GLBINDBUFFERARBPROC)Window->GetExtensionAddress("glBindBufferARB");
		lcDeleteBuffersARB = (GLDELETEBUFFERSARBPROC)Window->GetExtensionAddress("glDeleteBuffersARB");
		lcGenBuffersARB = (GLGENBUFFERSARBPROC)Window->GetExtensionAddress("glGenBuffersARB");
		lcIsBufferARB = (GLISBUFFERARBPROC)Window->GetExtensionAddress("glIsBufferARB");
		lcBufferDataARB = (GLBUFFERDATAARBPROC)Window->GetExtensionAddress("glBufferDataARB");
		lcBufferSubDataARB = (GLBUFFERSUBDATAARBPROC)Window->GetExtensionAddress("glBufferSubDataARB");
		lcGetBufferSubDataARB = (GLGETBUFFERSUBDATAARBPROC)Window->GetExtensionAddress("glGetBufferSubDataARB");
		lcMapBufferARB = (GLMAPBUFFERARBPROC)Window->GetExtensionAddress("glMapBufferARB");
		lcUnmapBufferARB = (GLUNMAPBUFFERARBPROC)Window->GetExtensionAddress("glUnmapBufferARB");
		lcGetBufferParameterivARB = (GLGETBUFFERPARAMETERIVARBPROC)Window->GetExtensionAddress("glGetBufferParameterivARB");
		lcGetBufferPointervARB = (GLGETBUFFERPOINTERVARBPROC)Window->GetExtensionAddress("glGetBufferPointervARB");

		GL_UseVertexBufferObject = true;
		GL_SupportsVertexBufferObject = true;
	}

	if (GL_ExtensionSupported(Extensions, "GL_ARB_framebuffer_object"))
	{
		lcIsRenderbufferARB = (GLISRENDERBUFFERARBPROC)Window->GetExtensionAddress("glIsRenderbuffer");
		lcBindRenderbufferARB = (GLBINDRENDERBUFFERARBPROC)Window->GetExtensionAddress("glBindRenderbuffer");
		lcDeleteRenderbuffersARB = (GLDELETERENDERBUFFERSARBPROC)Window->GetExtensionAddress("glDeleteRenderbuffers");
		lcGenRenderbuffersARB = (GLGENRENDERBUFFERSARBPROC)Window->GetExtensionAddress("glGenRenderbuffers");
		lcRenderbufferStorageARB = (GLRENDERBUFFERSTORAGEARBPROC)Window->GetExtensionAddress("glRenderbufferStorage");
		lcGetRenderbufferParameterivARB = (GLGETRENDERBUFFERPARAMETERIVARBPROC)Window->GetExtensionAddress("glGetRenderbufferParameteriv");
		lcIsFramebufferARB = (GLISFRAMEBUFFERARBPROC)Window->GetExtensionAddress("glIsFramebuffer");
		lcBindFramebufferARB = (GLBINDFRAMEBUFFERARBPROC)Window->GetExtensionAddress("glBindFramebuffer");
		lcDeleteFramebuffersARB = (GLDELETEFRAMEBUFFERSARBPROC)Window->GetExtensionAddress("glDeleteFramebuffers");
		lcGenFramebuffersARB = (GLGENFRAMEBUFFERSARBPROC)Window->GetExtensionAddress("glGenFramebuffers");
		lcCheckFramebufferStatusARB = (GLCHECKFRAMEBUFFERSTATUSARBPROC)Window->GetExtensionAddress("glCheckFramebufferStatus");
		lcFramebufferTexture1DARB = (GLFRAMEBUFFERTEXTURE1DARBPROC)Window->GetExtensionAddress("glFramebufferTexture1D");
		lcFramebufferTexture2DARB = (GLFRAMEBUFFERTEXTURE2DARBPROC)Window->GetExtensionAddress("glFramebufferTexture2D");
		lcFramebufferTexture3DARB = (GLFRAMEBUFFERTEXTURE3DARBPROC)Window->GetExtensionAddress("glFramebufferTexture3D");
		lcFramebufferRenderbufferARB = (GLFRAMEBUFFERRENDERBUFFERARBPROC)Window->GetExtensionAddress("glFramebufferRenderbuffer");
		lcGetFramebufferAttachmentParameterivARB = (GLGETFRAMEBUFFERATTACHMENTPARAMETERIVARBPROC)Window->GetExtensionAddress("glGetFramebufferAttachmentParameteriv");
		lcGenerateMipmapARB = (GLGENERATEMIPMAPARBPROC)Window->GetExtensionAddress("glGenerateMipmap");
		lcBlitFramebufferARB = (GLBLITFRAMEBUFFERARBPROC)Window->GetExtensionAddress("glBlitFramebuffer");
		lcRenderbufferStorageMultisampleARB = (GLRENDERBUFFERSTORAGEMULTISAMPLEARBPROC)Window->GetExtensionAddress("glRenderbufferStorageMultisample");
		lcFramebufferTextureLayerARB = (GLFRAMEBUFFERTEXTURELAYERARBPROC)Window->GetExtensionAddress("glFramebufferTextureLayer");

		GL_SupportsFramebufferObjectARB = true;
	}

	if (GL_ExtensionSupported(Extensions, "GL_EXT_framebuffer_object"))
	{
		lcIsRenderbufferEXT = (GLISRENDERBUFFEREXTPROC)Window->GetExtensionAddress("glIsRenderbufferEXT");
		lcBindRenderbufferEXT = (GLBINDRENDERBUFFEREXTPROC)Window->GetExtensionAddress("glBindRenderbufferEXT");
		lcDeleteRenderbuffersEXT = (GLDELETERENDERBUFFERSEXTPROC)Window->GetExtensionAddress("glDeleteRenderbuffersEXT");
		lcGenRenderbuffersEXT = (GLGENRENDERBUFFERSEXTPROC)Window->GetExtensionAddress("glGenRenderbuffersEXT");
		lcRenderbufferStorageEXT = (GLRENDERBUFFERSTORAGEEXTPROC)Window->GetExtensionAddress("glRenderbufferStorageEXT");
		lcGetRenderbufferParameterivEXT = (GLGETRENDERBUFFERPARAMETERIVEXTPROC)Window->GetExtensionAddress("glGetRenderbufferParameterivEXT");
		lcIsFramebufferEXT = (GLISFRAMEBUFFEREXTPROC)Window->GetExtensionAddress("glIsFramebufferEXT");
		lcBindFramebufferEXT = (GLBINDFRAMEBUFFEREXTPROC)Window->GetExtensionAddress("glBindFramebufferEXT");
		lcDeleteFramebuffersEXT = (GLDELETEFRAMEBUFFERSEXTPROC)Window->GetExtensionAddress("glDeleteFramebuffersEXT");
		lcGenFramebuffersEXT = (GLGENFRAMEBUFFERSEXTPROC)Window->GetExtensionAddress("glGenFramebuffersEXT");
		lcCheckFramebufferStatusEXT = (GLCHECKFRAMEBUFFERSTATUSEXTPROC)Window->GetExtensionAddress("glCheckFramebufferStatusEXT");
		lcFramebufferTexture1DEXT = (GLFRAMEBUFFERTEXTURE1DEXTPROC)Window->GetExtensionAddress("glFramebufferTexture1DEXT");
		lcFramebufferTexture2DEXT = (GLFRAMEBUFFERTEXTURE2DEXTPROC)Window->GetExtensionAddress("glFramebufferTexture2DEXT");
		lcFramebufferTexture3DEXT = (GLFRAMEBUFFERTEXTURE3DEXTPROC)Window->GetExtensionAddress("glFramebufferTexture3DEXT");
		lcFramebufferRenderbufferEXT = (GLFRAMEBUFFERRENDERBUFFEREXTPROC)Window->GetExtensionAddress("glFramebufferRenderbufferEXT");
		lcGetFramebufferAttachmentParameterivEXT = (GLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)Window->GetExtensionAddress("glGetFramebufferAttachmentParameterivEXT");
		lcGenerateMipmapEXT = (GLGENERATEMIPMAPEXTPROC)Window->GetExtensionAddress("glGenerateMipmapEXT");

		GL_SupportsFramebufferObjectEXT = true;
	}
}

static GLuint gFramebufferObject;
static GLuint gFramebufferTexture;
static GLuint gDepthRenderbufferObject;

bool GL_BeginRenderToTexture(int Width, int Height)
{
	if (GL_SupportsFramebufferObjectARB)
	{
		gMainWindow->mPreviewWidget->MakeCurrent();

		glGenFramebuffers(1, &gFramebufferObject);
		glGenTextures(1, &gFramebufferTexture);
		glGenRenderbuffers(1, &gDepthRenderbufferObject);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gFramebufferObject);

		glBindTexture(GL_TEXTURE_2D, gFramebufferTexture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gFramebufferTexture, 0);

		glBindRenderbuffer(GL_RENDERBUFFER, gDepthRenderbufferObject);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, Width, Height);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gDepthRenderbufferObject);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, gFramebufferObject);

		if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GL_EndRenderToTexture();
			return false;
		}

		return true;
	}

	if (GL_SupportsFramebufferObjectEXT)
	{
		gMainWindow->mPreviewWidget->MakeCurrent();

		glGenFramebuffersEXT(1, &gFramebufferObject); 
		glGenTextures(1, &gFramebufferTexture); 

		glBindTexture(GL_TEXTURE_2D, gFramebufferTexture); 
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); 

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gFramebufferObject); 
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, gFramebufferTexture, 0); 

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gFramebufferObject);

		glGenRenderbuffersEXT(1, &gDepthRenderbufferObject); 
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, gDepthRenderbufferObject); 
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, Width, Height);

		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, gDepthRenderbufferObject); 

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gFramebufferObject);

		if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
		{
			GL_EndRenderToTexture();
			return false;
		}

		return true;
	}

	return false;
}

void GL_EndRenderToTexture()
{
	if (GL_SupportsFramebufferObjectARB)
	{
		glDeleteFramebuffers(1, &gFramebufferObject);
		gFramebufferObject = 0;
		glDeleteTextures(1, &gFramebufferTexture);
		gFramebufferTexture = 0;
		glDeleteRenderbuffers(1, &gDepthRenderbufferObject);
		gDepthRenderbufferObject = 0;

		return;
	}

	if (GL_SupportsFramebufferObjectEXT)
	{
		glDeleteFramebuffersEXT(1, &gFramebufferObject);
		gFramebufferObject = 0;
		glDeleteTextures(1, &gFramebufferTexture);
		gFramebufferTexture = 0;
		glDeleteRenderbuffersEXT(1, &gDepthRenderbufferObject);
		gDepthRenderbufferObject = 0;
	}
}

#include "lc_global.h"
#include "lc_glextensions.h"

bool gSupportsShaderObjects;
bool gSupportsVertexBufferObject;
bool gSupportsFramebufferObjectARB;
bool gSupportsFramebufferObjectEXT;
bool gSupportsAnisotropic;
GLfloat gMaxAnisotropy;

PFNGLBINDBUFFERARBPROC lcBindBufferARB;
PFNGLDELETEBUFFERSARBPROC lcDeleteBuffersARB;
PFNGLGENBUFFERSARBPROC lcGenBuffersARB;
PFNGLISBUFFERARBPROC lcIsBufferARB;
PFNGLBUFFERDATAARBPROC lcBufferDataARB;
PFNGLBUFFERSUBDATAARBPROC lcBufferSubDataARB;
PFNGLGETBUFFERSUBDATAARBPROC lcGetBufferSubDataARB;
PFNGLMAPBUFFERARBPROC lcMapBufferARB;
PFNGLUNMAPBUFFERARBPROC lcUnmapBufferARB;
PFNGLGETBUFFERPARAMETERIVARBPROC lcGetBufferParameterivARB;
PFNGLGETBUFFERPOINTERVARBPROC lcGetBufferPointervARB;

PFNGLISRENDERBUFFERPROC lcIsRenderbuffer;
PFNGLBINDRENDERBUFFERPROC lcBindRenderbuffer;
PFNGLDELETERENDERBUFFERSPROC lcDeleteRenderbuffers;
PFNGLGENRENDERBUFFERSPROC lcGenRenderbuffers;
PFNGLRENDERBUFFERSTORAGEPROC lcRenderbufferStorage;
PFNGLGETRENDERBUFFERPARAMETERIVPROC lcGetRenderbufferParameteriv;
PFNGLISFRAMEBUFFERPROC lcIsFramebuffer;
PFNGLBINDFRAMEBUFFERPROC lcBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC lcDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC lcGenFramebuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC lcCheckFramebufferStatus;
PFNGLFRAMEBUFFERTEXTURE1DPROC lcFramebufferTexture1D;
PFNGLFRAMEBUFFERTEXTURE2DPROC lcFramebufferTexture2D;
PFNGLFRAMEBUFFERTEXTURE3DPROC lcFramebufferTexture3D;
PFNGLFRAMEBUFFERRENDERBUFFERPROC lcFramebufferRenderbuffer;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC lcGetFramebufferAttachmentParameteriv;
PFNGLGENERATEMIPMAPPROC lcGenerateMipmap;
PFNGLBLITFRAMEBUFFERPROC lcBlitFramebuffer;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC lcRenderbufferStorageMultisample;
PFNGLFRAMEBUFFERTEXTURELAYERPROC lcFramebufferTextureLayer;

PFNGLISRENDERBUFFEREXTPROC lcIsRenderbufferEXT;
PFNGLBINDRENDERBUFFEREXTPROC lcBindRenderbufferEXT;
PFNGLDELETERENDERBUFFERSEXTPROC lcDeleteRenderbuffersEXT;
PFNGLGENRENDERBUFFERSEXTPROC lcGenRenderbuffersEXT;
PFNGLRENDERBUFFERSTORAGEEXTPROC lcRenderbufferStorageEXT;
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC lcGetRenderbufferParameterivEXT;
PFNGLISFRAMEBUFFEREXTPROC lcIsFramebufferEXT;
PFNGLBINDFRAMEBUFFEREXTPROC lcBindFramebufferEXT;
PFNGLDELETEFRAMEBUFFERSEXTPROC lcDeleteFramebuffersEXT;
PFNGLGENFRAMEBUFFERSEXTPROC lcGenFramebuffersEXT;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC lcCheckFramebufferStatusEXT;
PFNGLFRAMEBUFFERTEXTURE1DEXTPROC lcFramebufferTexture1DEXT;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC lcFramebufferTexture2DEXT;
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC lcFramebufferTexture3DEXT;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC lcFramebufferRenderbufferEXT;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC lcGetFramebufferAttachmentParameterivEXT;
PFNGLGENERATEMIPMAPEXTPROC lcGenerateMipmapEXT;

PFNGLDELETEOBJECTARBPROC lcDeleteObjectARB;
PFNGLGETHANDLEARBPROC lcGetHandleARB;
PFNGLDETACHOBJECTARBPROC lcDetachObjectARB;
PFNGLCREATESHADEROBJECTARBPROC lcCreateShaderObjectARB;
PFNGLSHADERSOURCEARBPROC lcShaderSourceARB;
PFNGLCOMPILESHADERARBPROC lcCompileShaderARB;
PFNGLCREATEPROGRAMOBJECTARBPROC lcCreateProgramObjectARB;
PFNGLATTACHOBJECTARBPROC lcAttachObjectARB;
PFNGLLINKPROGRAMARBPROC lcLinkProgramARB;
PFNGLUSEPROGRAMOBJECTARBPROC lcUseProgramObjectARB;
PFNGLVALIDATEPROGRAMARBPROC lcValidateProgramARB;
PFNGLUNIFORM1FARBPROC lcUniform1fARB;
PFNGLUNIFORM2FARBPROC lcUniform2fARB;
PFNGLUNIFORM3FARBPROC lcUniform3fARB;
PFNGLUNIFORM4FARBPROC lcUniform4fARB;
PFNGLUNIFORM1IARBPROC lcUniform1iARB;
PFNGLUNIFORM2IARBPROC lcUniform2iARB;
PFNGLUNIFORM3IARBPROC lcUniform3iARB;
PFNGLUNIFORM4IARBPROC lcUniform4iARB;
PFNGLUNIFORM1FVARBPROC lcUniform1fvARB;
PFNGLUNIFORM2FVARBPROC lcUniform2fvARB;
PFNGLUNIFORM3FVARBPROC lcUniform3fvARB;
PFNGLUNIFORM4FVARBPROC lcUniform4fvARB;
PFNGLUNIFORM1IVARBPROC lcUniform1ivARB;
PFNGLUNIFORM2IVARBPROC lcUniform2ivARB;
PFNGLUNIFORM3IVARBPROC lcUniform3ivARB;
PFNGLUNIFORM4IVARBPROC lcUniform4ivARB;
PFNGLUNIFORMMATRIX2FVARBPROC lcUniformMatrix2fvARB;
PFNGLUNIFORMMATRIX3FVARBPROC lcUniformMatrix3fvARB;
PFNGLUNIFORMMATRIX4FVARBPROC lcUniformMatrix4fvARB;
PFNGLGETOBJECTPARAMETERFVARBPROC lcGetObjectParameterfvARB;
PFNGLGETOBJECTPARAMETERIVARBPROC lcGetObjectParameterivARB;
PFNGLGETINFOLOGARBPROC lcGetInfoLogARB;
PFNGLGETATTACHEDOBJECTSARBPROC lcGetAttachedObjectsARB;
PFNGLGETUNIFORMLOCATIONARBPROC lcGetUniformLocationARB;
PFNGLGETACTIVEUNIFORMARBPROC lcGetActiveUniformARB;
PFNGLGETUNIFORMFVARBPROC lcGetUniformfvARB;
PFNGLGETUNIFORMIVARBPROC lcGetUniformivARB;
PFNGLGETSHADERSOURCEARBPROC lcGetShaderSourceARB;

PFNGLBINDATTRIBLOCATIONARBPROC lcBindAttribLocationARB;
PFNGLGETACTIVEATTRIBARBPROC lcGetActiveAttribARB;
PFNGLGETATTRIBLOCATIONARBPROC lcGetAttribLocationARB;

static bool lcIsGLExtensionSupported(const GLubyte* Extensions, const char* Name)
{
	const GLubyte* Start;
	GLubyte* Where;
	GLubyte* Terminator;

	Where = (GLubyte*)strchr(Name, ' ');
	if (Where || *Name == '\0')
		return false;

	if (!Extensions)
		return false;

	for (Start = Extensions; ;)
	{
		Where = (GLubyte*)strstr((const char*)Start, Name);
		if (!Where)
			break;

		Terminator = Where + strlen(Name);
		if (Where == Start || *(Where - 1) == ' ')
			if (*Terminator == ' ' || *Terminator == '\0')
				return true;

		Start = Terminator;
	}

	return false;
}

#ifndef QT_NO_DEBUG

static void APIENTRY lcGLDebugCallback(GLenum Source, GLenum Type, GLuint Id, GLenum Severity, GLsizei Length, const GLchar* Message, GLvoid* UserParam)
{
	qDebug() << Message;
}

#endif

void lcInitializeGLExtensions(const QGLContext* Context)
{
	const GLubyte* Extensions = glGetString(GL_EXTENSIONS);

#ifndef QT_NO_DEBUG
	if (lcIsGLExtensionSupported(Extensions, "GL_KHR_debug"))
	{
		PFNGLDEBUGMESSAGECALLBACKARBPROC DebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKARBPROC)Context->getProcAddress("glDebugMessageCallback");

		if (DebugMessageCallback)
		{
			DebugMessageCallback((GLDEBUGPROCARB)&lcGLDebugCallback, NULL);
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		}
	}
#endif

	if (lcIsGLExtensionSupported(Extensions, "GL_EXT_texture_filter_anisotropic"))
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gMaxAnisotropy);

		gSupportsAnisotropic = true;
	}

	if (lcIsGLExtensionSupported(Extensions, "GL_ARB_vertex_buffer_object"))
	{
		lcBindBufferARB = (PFNGLBINDBUFFERARBPROC)Context->getProcAddress("glBindBufferARB");
		lcDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)Context->getProcAddress("glDeleteBuffersARB");
		lcGenBuffersARB = (PFNGLGENBUFFERSARBPROC)Context->getProcAddress("glGenBuffersARB");
		lcIsBufferARB = (PFNGLISBUFFERARBPROC)Context->getProcAddress("glIsBufferARB");
		lcBufferDataARB = (PFNGLBUFFERDATAARBPROC)Context->getProcAddress("glBufferDataARB");
		lcBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)Context->getProcAddress("glBufferSubDataARB");
		lcGetBufferSubDataARB = (PFNGLGETBUFFERSUBDATAARBPROC)Context->getProcAddress("glGetBufferSubDataARB");
		lcMapBufferARB = (PFNGLMAPBUFFERARBPROC)Context->getProcAddress("glMapBufferARB");
		lcUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)Context->getProcAddress("glUnmapBufferARB");
		lcGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)Context->getProcAddress("glGetBufferParameterivARB");
		lcGetBufferPointervARB = (PFNGLGETBUFFERPOINTERVARBPROC)Context->getProcAddress("glGetBufferPointervARB");

		gSupportsVertexBufferObject = true;
	}

	if (lcIsGLExtensionSupported(Extensions, "GL_ARB_framebuffer_object"))
	{
		lcIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)Context->getProcAddress("glIsRenderbuffer");
		lcBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)Context->getProcAddress("glBindRenderbuffer");
		lcDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)Context->getProcAddress("glDeleteRenderbuffers");
		lcGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)Context->getProcAddress("glGenRenderbuffers");
		lcRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)Context->getProcAddress("glRenderbufferStorage");
		lcGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)Context->getProcAddress("glGetRenderbufferParameteriv");
		lcIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)Context->getProcAddress("glIsFramebuffer");
		lcBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)Context->getProcAddress("glBindFramebuffer");
		lcDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)Context->getProcAddress("glDeleteFramebuffers");
		lcGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)Context->getProcAddress("glGenFramebuffers");
		lcCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)Context->getProcAddress("glCheckFramebufferStatus");
		lcFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)Context->getProcAddress("glFramebufferTexture1D");
		lcFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)Context->getProcAddress("glFramebufferTexture2D");
		lcFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)Context->getProcAddress("glFramebufferTexture3D");
		lcFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)Context->getProcAddress("glFramebufferRenderbuffer");
		lcGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)Context->getProcAddress("glGetFramebufferAttachmentParameteriv");
		lcGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)Context->getProcAddress("glGenerateMipmap");
		lcBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)Context->getProcAddress("glBlitFramebuffer");
		lcRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)Context->getProcAddress("glRenderbufferStorageMultisample");
		lcFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERARBPROC)Context->getProcAddress("glFramebufferTextureLayer");

		gSupportsFramebufferObjectARB = true;
	}

	if (lcIsGLExtensionSupported(Extensions, "GL_EXT_framebuffer_object"))
	{
		lcIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)Context->getProcAddress("glIsRenderbufferEXT");
		lcBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)Context->getProcAddress("glBindRenderbufferEXT");
		lcDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)Context->getProcAddress("glDeleteRenderbuffersEXT");
		lcGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)Context->getProcAddress("glGenRenderbuffersEXT");
		lcRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)Context->getProcAddress("glRenderbufferStorageEXT");
		lcGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)Context->getProcAddress("glGetRenderbufferParameterivEXT");
		lcIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)Context->getProcAddress("glIsFramebufferEXT");
		lcBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)Context->getProcAddress("glBindFramebufferEXT");
		lcDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)Context->getProcAddress("glDeleteFramebuffersEXT");
		lcGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)Context->getProcAddress("glGenFramebuffersEXT");
		lcCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)Context->getProcAddress("glCheckFramebufferStatusEXT");
		lcFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)Context->getProcAddress("glFramebufferTexture1DEXT");
		lcFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)Context->getProcAddress("glFramebufferTexture2DEXT");
		lcFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)Context->getProcAddress("glFramebufferTexture3DEXT");
		lcFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)Context->getProcAddress("glFramebufferRenderbufferEXT");
		lcGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)Context->getProcAddress("glGetFramebufferAttachmentParameterivEXT");
		lcGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)Context->getProcAddress("glGenerateMipmapEXT");

		gSupportsFramebufferObjectEXT = true;
	}

	if (lcIsGLExtensionSupported(Extensions, "GL_ARB_shader_objects") && lcIsGLExtensionSupported(Extensions, "GL_ARB_shading_language_100") &&
	    lcIsGLExtensionSupported(Extensions, "GL_ARB_vertex_shader") && lcIsGLExtensionSupported(Extensions, "GL_ARB_fragment_shader"))
	{
		lcDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC)Context->getProcAddress("glDeleteObjectARB");
		lcGetHandleARB = (PFNGLGETHANDLEARBPROC)Context->getProcAddress("glGetHandleARB");
		lcDetachObjectARB = (PFNGLDETACHOBJECTARBPROC)Context->getProcAddress("glDetachObjectARB");
		lcCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)Context->getProcAddress("glCreateShaderObjectARB");
		lcShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)Context->getProcAddress("glShaderSourceARB");
		lcCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)Context->getProcAddress("glCompileShaderARB");
		lcCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)Context->getProcAddress("glCreateProgramObjectARB");
		lcAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)Context->getProcAddress("glAttachObjectARB");
		lcLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)Context->getProcAddress("glLinkProgramARB");
		lcUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)Context->getProcAddress("glUseProgramObjectARB");
		lcValidateProgramARB = (PFNGLVALIDATEPROGRAMARBPROC)Context->getProcAddress("glValidateProgramARB");
		lcUniform1fARB = (PFNGLUNIFORM1FARBPROC)Context->getProcAddress("glUniform1fARB");
		lcUniform2fARB = (PFNGLUNIFORM2FARBPROC)Context->getProcAddress("glUniform2fARB");
		lcUniform3fARB = (PFNGLUNIFORM3FARBPROC)Context->getProcAddress("glUniform3fARB");
		lcUniform4fARB = (PFNGLUNIFORM4FARBPROC)Context->getProcAddress("glUniform4fARB");
		lcUniform1iARB = (PFNGLUNIFORM1IARBPROC)Context->getProcAddress("glUniform1iARB");
		lcUniform2iARB = (PFNGLUNIFORM2IARBPROC)Context->getProcAddress("glUniform2iARB");
		lcUniform3iARB = (PFNGLUNIFORM3IARBPROC)Context->getProcAddress("glUniform3iARB");
		lcUniform4iARB = (PFNGLUNIFORM4IARBPROC)Context->getProcAddress("glUniform4iARB");
		lcUniform1fvARB = (PFNGLUNIFORM1FVARBPROC)Context->getProcAddress("glUniform1fvARB");
		lcUniform2fvARB = (PFNGLUNIFORM2FVARBPROC)Context->getProcAddress("glUniform2fvARB");
		lcUniform3fvARB = (PFNGLUNIFORM3FVARBPROC)Context->getProcAddress("glUniform3fvARB");
		lcUniform4fvARB = (PFNGLUNIFORM4FVARBPROC)Context->getProcAddress("glUniform4fvARB");
		lcUniform1ivARB = (PFNGLUNIFORM1IVARBPROC)Context->getProcAddress("glUniform1ivARB");
		lcUniform2ivARB = (PFNGLUNIFORM2IVARBPROC)Context->getProcAddress("glUniform2ivARB");
		lcUniform3ivARB = (PFNGLUNIFORM3IVARBPROC)Context->getProcAddress("glUniform3ivARB");
		lcUniform4ivARB = (PFNGLUNIFORM4IVARBPROC)Context->getProcAddress("glUniform4ivARB");
		lcUniformMatrix2fvARB = (PFNGLUNIFORMMATRIX2FVARBPROC)Context->getProcAddress("glUniformMatrix2fvARB");
		lcUniformMatrix3fvARB = (PFNGLUNIFORMMATRIX3FVARBPROC)Context->getProcAddress("glUniformMatrix3fvARB");
		lcUniformMatrix4fvARB = (PFNGLUNIFORMMATRIX4FVARBPROC)Context->getProcAddress("glUniformMatrix4fvARB");
		lcGetObjectParameterfvARB = (PFNGLGETOBJECTPARAMETERFVARBPROC)Context->getProcAddress("glGetObjectParameterfvARB");
		lcGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)Context->getProcAddress("glGetObjectParameterivARB");
		lcGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)Context->getProcAddress("glGetInfoLogARB");
		lcGetAttachedObjectsARB = (PFNGLGETATTACHEDOBJECTSARBPROC)Context->getProcAddress("glGetAttachedObjectsARB");
		lcGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)Context->getProcAddress("glGetUniformLocationARB");
		lcGetActiveUniformARB = (PFNGLGETACTIVEUNIFORMARBPROC)Context->getProcAddress("glGetActiveUniformARB");
		lcGetUniformfvARB = (PFNGLGETUNIFORMFVARBPROC)Context->getProcAddress("glGetUniformfvARB");
		lcGetUniformivARB = (PFNGLGETUNIFORMIVARBPROC)Context->getProcAddress("glGetUniformivARB");
		lcGetShaderSourceARB = (PFNGLGETSHADERSOURCEARBPROC)Context->getProcAddress("glGetShaderSourceARB");

		lcBindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC)Context->getProcAddress("glBindAttribLocationARB");
		lcGetActiveAttribARB = (PFNGLGETACTIVEATTRIBARBPROC)Context->getProcAddress("glGetActiveAttribARB");
		lcGetAttribLocationARB = (PFNGLGETATTRIBLOCATIONARBPROC)Context->getProcAddress("glGetAttribLocationARB");

		gSupportsShaderObjects = true;
	}
}

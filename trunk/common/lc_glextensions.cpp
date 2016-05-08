#include "lc_global.h"
#include "lc_glextensions.h"

bool gSupportsShaderObjects;
bool gSupportsVertexBufferObject;
bool gSupportsFramebufferObjectARB;
bool gSupportsFramebufferObjectEXT;
bool gSupportsAnisotropic;
GLfloat gMaxAnisotropy;

#ifdef LC_LOAD_GLEXTENSIONS

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

PFNGLATTACHSHADERPROC lcAttachShader;
PFNGLBINDATTRIBLOCATIONPROC lcBindAttribLocation;
PFNGLCOMPILESHADERPROC lcCompileShader;
PFNGLCREATEPROGRAMPROC lcCreateProgram;
PFNGLCREATESHADERPROC lcCreateShader;
PFNGLDELETEPROGRAMPROC lcDeleteProgram;
PFNGLDELETESHADERPROC lcDeleteShader;
PFNGLDETACHSHADERPROC lcDetachShader;
PFNGLDISABLEVERTEXATTRIBARRAYPROC lcDisableVertexAttribArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC lcEnableVertexAttribArray;
PFNGLGETACTIVEATTRIBPROC lcGetActiveAttrib;
PFNGLGETACTIVEUNIFORMPROC lcGetActiveUniform;
PFNGLGETATTACHEDSHADERSPROC lcGetAttachedShaders;
PFNGLGETATTRIBLOCATIONPROC lcGetAttribLocation;
PFNGLGETPROGRAMIVPROC lcGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC lcGetProgramInfoLog;
PFNGLGETSHADERIVPROC lcGetShaderiv;
PFNGLGETSHADERINFOLOGPROC lcGetShaderInfoLog;
PFNGLGETSHADERSOURCEPROC lcGetShaderSource;
PFNGLGETUNIFORMLOCATIONPROC lcGetUniformLocation;
PFNGLGETUNIFORMFVPROC lcGetUniformfv;
PFNGLGETUNIFORMIVPROC lcGetUniformiv;
PFNGLGETVERTEXATTRIBDVPROC lcGetVertexAttribdv;
PFNGLGETVERTEXATTRIBFVPROC lcGetVertexAttribfv;
PFNGLGETVERTEXATTRIBIVPROC lcGetVertexAttribiv;
PFNGLGETVERTEXATTRIBPOINTERVPROC lcGetVertexAttribPointerv;
PFNGLISPROGRAMPROC lcIsProgram;
PFNGLISSHADERPROC lcIsShader;
PFNGLLINKPROGRAMPROC lcLinkProgram;
PFNGLSHADERSOURCEPROC lcShaderSource;
PFNGLUSEPROGRAMPROC lcUseProgram;
PFNGLUNIFORM1FPROC lcUniform1f;
PFNGLUNIFORM2FPROC lcUniform2f;
PFNGLUNIFORM3FPROC lcUniform3f;
PFNGLUNIFORM4FPROC lcUniform4f;
PFNGLUNIFORM1IPROC lcUniform1i;
PFNGLUNIFORM2IPROC lcUniform2i;
PFNGLUNIFORM3IPROC lcUniform3i;
PFNGLUNIFORM4IPROC lcUniform4i;
PFNGLUNIFORM1FVPROC lcUniform1fv;
PFNGLUNIFORM2FVPROC lcUniform2fv;
PFNGLUNIFORM3FVPROC lcUniform3fv;
PFNGLUNIFORM4FVPROC lcUniform4fv;
PFNGLUNIFORM1IVPROC lcUniform1iv;
PFNGLUNIFORM2IVPROC lcUniform2iv;
PFNGLUNIFORM3IVPROC lcUniform3iv;
PFNGLUNIFORM4IVPROC lcUniform4iv;
PFNGLUNIFORMMATRIX2FVPROC lcUniformMatrix2fv;
PFNGLUNIFORMMATRIX3FVPROC lcUniformMatrix3fv;
PFNGLUNIFORMMATRIX4FVPROC lcUniformMatrix4fv;
PFNGLVALIDATEPROGRAMPROC lcValidateProgram;
PFNGLVERTEXATTRIBPOINTERPROC lcVertexAttribPointer;

#endif

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

#if !defined(QT_NO_DEBUG) && defined(GL_ARB_debug_output)

static void APIENTRY lcGLDebugCallback(GLenum Source, GLenum Type, GLuint Id, GLenum Severity, GLsizei Length, const GLchar* Message, GLvoid* UserParam)
{
	Q_UNUSED(Source);
	Q_UNUSED(Type);
	Q_UNUSED(Id);
	Q_UNUSED(Severity);
	Q_UNUSED(Length);
	Q_UNUSED(UserParam);

	qDebug() << Message;
}

#endif

void lcInitializeGLExtensions(const QGLContext* Context)
{
	const GLubyte* Extensions = glGetString(GL_EXTENSIONS);
	const GLubyte* Version = glGetString(GL_VERSION);
	int VersionMajor = 0, VersionMinor = 0;

	if (Version)
		sscanf((const char*)Version, "%d.%d", &VersionMajor, &VersionMinor);

#if !defined(QT_NO_DEBUG) && defined(GL_ARB_debug_output)
	if (lcIsGLExtensionSupported(Extensions, "GL_KHR_debug"))
	{
		PFNGLDEBUGMESSAGECALLBACKARBPROC DebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKARBPROC)Context->getProcAddress("glDebugMessageCallback");

#ifndef GL_DEBUG_OUTPUT
#define GL_DEBUG_OUTPUT 0x92E0
#endif

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

	// todo: check gl version and use core functions instead
	if (lcIsGLExtensionSupported(Extensions, "GL_ARB_vertex_buffer_object"))
	{
#ifdef LC_LOAD_GLEXTENSIONS
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
#endif
		gSupportsVertexBufferObject = true;
	}

	// todo: check gl version
	if (lcIsGLExtensionSupported(Extensions, "GL_ARB_framebuffer_object"))
	{
#ifdef LC_LOAD_GLEXTENSIONS
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
#endif
		gSupportsFramebufferObjectARB = true;
	}

	if (lcIsGLExtensionSupported(Extensions, "GL_EXT_framebuffer_object"))
	{
#ifdef LC_LOAD_GLEXTENSIONS
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
#endif
		gSupportsFramebufferObjectEXT = true;
	}

	const GLubyte* GLSLVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
	int GLSLMajor = 0, GLSLMinor = 0;

	if (GLSLVersion)
		sscanf((const char*)GLSLVersion, "%d.%d", &GLSLMajor, &GLSLMinor);

	if (VersionMajor >= 2 && (GLSLMajor > 1 || (GLSLMajor == 1 && GLSLMinor >= 10)))
	{
#ifdef LC_LOAD_GLEXTENSIONS
		lcAttachShader = (PFNGLATTACHSHADERPROC)Context->getProcAddress("glAttachShader");
		lcBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)Context->getProcAddress("glBindAttribLocation");
		lcCompileShader = (PFNGLCOMPILESHADERPROC)Context->getProcAddress("glCompileShader");
		lcCreateProgram = (PFNGLCREATEPROGRAMPROC)Context->getProcAddress("glCreateProgram");
		lcCreateShader = (PFNGLCREATESHADERPROC)Context->getProcAddress("glCreateShader");
		lcDeleteProgram = (PFNGLDELETEPROGRAMPROC)Context->getProcAddress("glDeleteProgram");
		lcDeleteShader = (PFNGLDELETESHADERPROC)Context->getProcAddress("glDeleteShader");
		lcDetachShader = (PFNGLDETACHSHADERPROC)Context->getProcAddress("glDetachShader");
		lcDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)Context->getProcAddress("glDisableVertexAttribArray");
		lcEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)Context->getProcAddress("glEnableVertexAttribArray");
		lcGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)Context->getProcAddress("glGetActiveAttrib");
		lcGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)Context->getProcAddress("glGetActiveUniform");
		lcGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)Context->getProcAddress("glGetAttachedShaders");
		lcGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)Context->getProcAddress("glGetAttribLocation");
		lcGetProgramiv = (PFNGLGETPROGRAMIVPROC)Context->getProcAddress("glGetProgramiv");
		lcGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)Context->getProcAddress("glGetProgramInfoLog");
		lcGetShaderiv = (PFNGLGETSHADERIVPROC)Context->getProcAddress("glGetShaderiv");
		lcGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)Context->getProcAddress("glGetShaderInfoLog");
		lcGetShaderSource = (PFNGLGETSHADERSOURCEPROC)Context->getProcAddress("glGetShaderSource");
		lcGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)Context->getProcAddress("glGetUniformLocation");
		lcGetUniformfv = (PFNGLGETUNIFORMFVPROC)Context->getProcAddress("glGetUniformfv");
		lcGetUniformiv = (PFNGLGETUNIFORMIVPROC)Context->getProcAddress("glGetUniformiv");
		lcGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)Context->getProcAddress("glGetVertexAttribdv");
		lcGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)Context->getProcAddress("glGetVertexAttribfv");
		lcGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)Context->getProcAddress("glGetVertexAttribiv");
		lcGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)Context->getProcAddress("glGetVertexAttribPointerv");
		lcIsProgram = (PFNGLISPROGRAMPROC)Context->getProcAddress("glIsProgram");
		lcIsShader = (PFNGLISSHADERPROC)Context->getProcAddress("glIsShader");
		lcLinkProgram = (PFNGLLINKPROGRAMPROC)Context->getProcAddress("glLinkProgram");
		lcShaderSource = (PFNGLSHADERSOURCEPROC)Context->getProcAddress("glShaderSource");
		lcUseProgram = (PFNGLUSEPROGRAMPROC)Context->getProcAddress("glUseProgram");
		lcUniform1f = (PFNGLUNIFORM1FPROC)Context->getProcAddress("glUniform1f");
		lcUniform2f = (PFNGLUNIFORM2FPROC)Context->getProcAddress("glUniform2f");
		lcUniform3f = (PFNGLUNIFORM3FPROC)Context->getProcAddress("glUniform3f");
		lcUniform4f = (PFNGLUNIFORM4FPROC)Context->getProcAddress("glUniform4f");
		lcUniform1i = (PFNGLUNIFORM1IPROC)Context->getProcAddress("glUniform1i");
		lcUniform2i = (PFNGLUNIFORM2IPROC)Context->getProcAddress("glUniform2i");
		lcUniform3i = (PFNGLUNIFORM3IPROC)Context->getProcAddress("glUniform3i");
		lcUniform4i = (PFNGLUNIFORM4IPROC)Context->getProcAddress("glUniform4i");
		lcUniform1fv = (PFNGLUNIFORM1FVPROC)Context->getProcAddress("glUniform1fv");
		lcUniform2fv = (PFNGLUNIFORM2FVPROC)Context->getProcAddress("glUniform2fv");
		lcUniform3fv = (PFNGLUNIFORM3FVPROC)Context->getProcAddress("glUniform3fv");
		lcUniform4fv = (PFNGLUNIFORM4FVPROC)Context->getProcAddress("glUniform4fv");
		lcUniform1iv = (PFNGLUNIFORM1IVPROC)Context->getProcAddress("glUniform1iv");
		lcUniform2iv = (PFNGLUNIFORM2IVPROC)Context->getProcAddress("glUniform2iv");
		lcUniform3iv = (PFNGLUNIFORM3IVPROC)Context->getProcAddress("glUniform3iv");
		lcUniform4iv = (PFNGLUNIFORM4IVPROC)Context->getProcAddress("glUniform4iv");
		lcUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)Context->getProcAddress("glUniformMatrix2fv");
		lcUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)Context->getProcAddress("glUniformMatrix3fv");
		lcUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)Context->getProcAddress("glUniformMatrix4fv");
		lcValidateProgram = (PFNGLVALIDATEPROGRAMPROC)Context->getProcAddress("glValidateProgram");
		lcVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)Context->getProcAddress("glVertexAttribPointer");
#endif
		gSupportsShaderObjects = true;
	}
}

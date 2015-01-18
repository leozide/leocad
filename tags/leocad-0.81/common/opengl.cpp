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

GLDELETEOBJECTARBPROC lcDeleteObjectARB;
GLGETHANDLEARBPROC lcGetHandleARB;
GLDETACHOBJECTARBPROC lcDetachObjectARB;
GLCREATESHADEROBJECTARBPROC lcCreateShaderObjectARB;
GLSHADERSOURCEARBPROC lcShaderSourceARB;
GLCOMPILESHADERARBPROC lcCompileShaderARB;
GLCREATEPROGRAMOBJECTARBPROC lcCreateProgramObjectARB;
GLATTACHOBJECTARBPROC lcAttachObjectARB;
GLLINKPROGRAMARBPROC lcLinkProgramARB;
GLUSEPROGRAMOBJECTARBPROC lcUseProgramObjectARB;
GLVALIDATEPROGRAMARBPROC lcValidateProgramARB;
GLUNIFORM1FARBPROC lcUniform1fARB;
GLUNIFORM2FARBPROC lcUniform2fARB;
GLUNIFORM3FARBPROC lcUniform3fARB;
GLUNIFORM4FARBPROC lcUniform4fARB;
GLUNIFORM1IARBPROC lcUniform1iARB;
GLUNIFORM2IARBPROC lcUniform2iARB;
GLUNIFORM3IARBPROC lcUniform3iARB;
GLUNIFORM4IARBPROC lcUniform4iARB;
GLUNIFORM1FVARBPROC lcUniform1fvARB;
GLUNIFORM2FVARBPROC lcUniform2fvARB;
GLUNIFORM3FVARBPROC lcUniform3fvARB;
GLUNIFORM4FVARBPROC lcUniform4fvARB;
GLUNIFORM1IVARBPROC lcUniform1ivARB;
GLUNIFORM2IVARBPROC lcUniform2ivARB;
GLUNIFORM3IVARBPROC lcUniform3ivARB;
GLUNIFORM4IVARBPROC lcUniform4ivARB;
GLUNIFORMMATRIX2FVARBPROC lcUniformMatrix2fvARB;
GLUNIFORMMATRIX3FVARBPROC lcUniformMatrix3fvARB;
GLUNIFORMMATRIX4FVARBPROC lcUniformMatrix4fvARB;
GLGETOBJECTPARAMETERFVARBPROC lcGetObjectParameterfvARB;
GLGETOBJECTPARAMETERIVARBPROC lcGetObjectParameterivARB;
GLGETINFOLOGARBPROC lcGetInfoLogARB;
GLGETATTACHEDOBJECTSARBPROC lcGetAttachedObjectsARB;
GLGETUNIFORMLOCATIONARBPROC lcGetUniformLocationARB;
GLGETACTIVEUNIFORMARBPROC lcGetActiveUniformARB;
GLGETUNIFORMFVARBPROC lcGetUniformfvARB;
GLGETUNIFORMIVARBPROC lcGetUniformivARB;
GLGETSHADERSOURCEARBPROC lcGetShaderSourceARB;

GLBINDATTRIBLOCATIONARBPROC lcBindAttribLocationARB;
GLGETACTIVEATTRIBARBPROC lcGetActiveAttribARB;
GLGETATTRIBLOCATIONARBPROC lcGetAttribLocationARB;

bool GL_SupportsShaderObjects;
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

	if (GL_ExtensionSupported(Extensions, "GL_ARB_shader_objects") && GL_ExtensionSupported(Extensions, "GL_ARB_shading_language_100") &&
	    GL_ExtensionSupported(Extensions, "GL_ARB_vertex_shader") && GL_ExtensionSupported(Extensions, "GL_ARB_fragment_shader"))
	{
		lcDeleteObjectARB = (GLDELETEOBJECTARBPROC)Window->GetExtensionAddress("glDeleteObjectARB");
		lcGetHandleARB = (GLGETHANDLEARBPROC)Window->GetExtensionAddress("glGetHandleARB");
		lcDetachObjectARB = (GLDETACHOBJECTARBPROC)Window->GetExtensionAddress("glDetachObjectARB");
		lcCreateShaderObjectARB = (GLCREATESHADEROBJECTARBPROC)Window->GetExtensionAddress("glCreateShaderObjectARB");
		lcShaderSourceARB = (GLSHADERSOURCEARBPROC)Window->GetExtensionAddress("glShaderSourceARB");
		lcCompileShaderARB = (GLCOMPILESHADERARBPROC)Window->GetExtensionAddress("glCompileShaderARB");
		lcCreateProgramObjectARB = (GLCREATEPROGRAMOBJECTARBPROC)Window->GetExtensionAddress("glCreateProgramObjectARB");
		lcAttachObjectARB = (GLATTACHOBJECTARBPROC)Window->GetExtensionAddress("glAttachObjectARB");
		lcLinkProgramARB = (GLLINKPROGRAMARBPROC)Window->GetExtensionAddress("glLinkProgramARB");
		lcUseProgramObjectARB = (GLUSEPROGRAMOBJECTARBPROC)Window->GetExtensionAddress("glUseProgramObjectARB");
		lcValidateProgramARB = (GLVALIDATEPROGRAMARBPROC)Window->GetExtensionAddress("glValidateProgramARB");
		lcUniform1fARB = (GLUNIFORM1FARBPROC)Window->GetExtensionAddress("glUniform1fARB");
		lcUniform2fARB = (GLUNIFORM2FARBPROC)Window->GetExtensionAddress("glUniform2fARB");
		lcUniform3fARB = (GLUNIFORM3FARBPROC)Window->GetExtensionAddress("glUniform3fARB");
		lcUniform4fARB = (GLUNIFORM4FARBPROC)Window->GetExtensionAddress("glUniform4fARB");
		lcUniform1iARB = (GLUNIFORM1IARBPROC)Window->GetExtensionAddress("glUniform1iARB");
		lcUniform2iARB = (GLUNIFORM2IARBPROC)Window->GetExtensionAddress("glUniform2iARB");
		lcUniform3iARB = (GLUNIFORM3IARBPROC)Window->GetExtensionAddress("glUniform3iARB");
		lcUniform4iARB = (GLUNIFORM4IARBPROC)Window->GetExtensionAddress("glUniform4iARB");
		lcUniform1fvARB = (GLUNIFORM1FVARBPROC)Window->GetExtensionAddress("glUniform1fvARB");
		lcUniform2fvARB = (GLUNIFORM2FVARBPROC)Window->GetExtensionAddress("glUniform2fvARB");
		lcUniform3fvARB = (GLUNIFORM3FVARBPROC)Window->GetExtensionAddress("glUniform3fvARB");
		lcUniform4fvARB = (GLUNIFORM4FVARBPROC)Window->GetExtensionAddress("glUniform4fvARB");
		lcUniform1ivARB = (GLUNIFORM1IVARBPROC)Window->GetExtensionAddress("glUniform1ivARB");
		lcUniform2ivARB = (GLUNIFORM2IVARBPROC)Window->GetExtensionAddress("glUniform2ivARB");
		lcUniform3ivARB = (GLUNIFORM3IVARBPROC)Window->GetExtensionAddress("glUniform3ivARB");
		lcUniform4ivARB = (GLUNIFORM4IVARBPROC)Window->GetExtensionAddress("glUniform4ivARB");
		lcUniformMatrix2fvARB = (GLUNIFORMMATRIX2FVARBPROC)Window->GetExtensionAddress("glUniformMatrix2fvARB");
		lcUniformMatrix3fvARB = (GLUNIFORMMATRIX3FVARBPROC)Window->GetExtensionAddress("glUniformMatrix3fvARB");
		lcUniformMatrix4fvARB = (GLUNIFORMMATRIX4FVARBPROC)Window->GetExtensionAddress("glUniformMatrix4fvARB");
		lcGetObjectParameterfvARB = (GLGETOBJECTPARAMETERFVARBPROC)Window->GetExtensionAddress("glGetObjectParameterfvARB");
		lcGetObjectParameterivARB = (GLGETOBJECTPARAMETERIVARBPROC)Window->GetExtensionAddress("glGetObjectParameterivARB");
		lcGetInfoLogARB = (GLGETINFOLOGARBPROC)Window->GetExtensionAddress("glGetInfoLogARB");
		lcGetAttachedObjectsARB = (GLGETATTACHEDOBJECTSARBPROC)Window->GetExtensionAddress("glGetAttachedObjectsARB");
		lcGetUniformLocationARB = (GLGETUNIFORMLOCATIONARBPROC)Window->GetExtensionAddress("glGetUniformLocationARB");
		lcGetActiveUniformARB = (GLGETACTIVEUNIFORMARBPROC)Window->GetExtensionAddress("glGetActiveUniformARB");
		lcGetUniformfvARB = (GLGETUNIFORMFVARBPROC)Window->GetExtensionAddress("glGetUniformfvARB");
		lcGetUniformivARB = (GLGETUNIFORMIVARBPROC)Window->GetExtensionAddress("glGetUniformivARB");
		lcGetShaderSourceARB = (GLGETSHADERSOURCEARBPROC)Window->GetExtensionAddress("glGetShaderSourceARB");

		lcBindAttribLocationARB = (GLBINDATTRIBLOCATIONARBPROC)Window->GetExtensionAddress("glBindAttribLocationARB");
		lcGetActiveAttribARB = (GLGETACTIVEATTRIBARBPROC)Window->GetExtensionAddress("glGetActiveAttribARB");
		lcGetAttribLocationARB = (GLGETATTRIBLOCATIONARBPROC)Window->GetExtensionAddress("glGetAttribLocationARB");

		GL_SupportsShaderObjects = true;
	}
}

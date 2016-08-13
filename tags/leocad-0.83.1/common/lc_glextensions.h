#ifndef _LC_GLEXTENSIONS_H_
#define _LC_GLEXTENSIONS_H_

void lcInitializeGLExtensions(const QGLContext* Context);

extern bool gSupportsShaderObjects;
extern bool gSupportsVertexBufferObject;
extern bool gSupportsFramebufferObjectARB;
extern bool gSupportsFramebufferObjectEXT;
extern bool gSupportsAnisotropic;
extern GLfloat gMaxAnisotropy;

#ifndef Q_OS_MAC
#define LC_LOAD_GLEXTENSIONS
#endif

#ifdef LC_LOAD_GLEXTENSIONS

extern PFNGLBINDBUFFERARBPROC lcBindBufferARB;
extern PFNGLDELETEBUFFERSARBPROC lcDeleteBuffersARB;
extern PFNGLGENBUFFERSARBPROC lcGenBuffersARB;
extern PFNGLISBUFFERARBPROC lcIsBufferARB;
extern PFNGLBUFFERDATAARBPROC lcBufferDataARB;
extern PFNGLBUFFERSUBDATAARBPROC lcBufferSubDataARB;
extern PFNGLGETBUFFERSUBDATAARBPROC lcGetBufferSubDataARB;
extern PFNGLMAPBUFFERARBPROC lcMapBufferARB;
extern PFNGLUNMAPBUFFERARBPROC lcUnmapBufferARB;
extern PFNGLGETBUFFERPARAMETERIVARBPROC lcGetBufferParameterivARB;
extern PFNGLGETBUFFERPOINTERVARBPROC lcGetBufferPointervARB;

extern PFNGLISRENDERBUFFERPROC lcIsRenderbuffer;
extern PFNGLBINDRENDERBUFFERPROC lcBindRenderbuffer;
extern PFNGLDELETERENDERBUFFERSPROC lcDeleteRenderbuffers;
extern PFNGLGENRENDERBUFFERSPROC lcGenRenderbuffers;
extern PFNGLRENDERBUFFERSTORAGEPROC lcRenderbufferStorage;
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC lcGetRenderbufferParameteriv;
extern PFNGLISFRAMEBUFFERPROC lcIsFramebuffer;
extern PFNGLBINDFRAMEBUFFERPROC lcBindFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC lcDeleteFramebuffers;
extern PFNGLGENFRAMEBUFFERSPROC lcGenFramebuffers;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC lcCheckFramebufferStatus;
extern PFNGLFRAMEBUFFERTEXTURE1DPROC lcFramebufferTexture1D;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC lcFramebufferTexture2D;
extern PFNGLFRAMEBUFFERTEXTURE3DPROC lcFramebufferTexture3D;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC lcFramebufferRenderbuffer;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC lcGetFramebufferAttachmentParameteriv;
extern PFNGLGENERATEMIPMAPPROC lcGenerateMipmap;
extern PFNGLBLITFRAMEBUFFERPROC lcBlitFramebuffer;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC lcRenderbufferStorageMultisample;
extern PFNGLFRAMEBUFFERTEXTURELAYERPROC lcFramebufferTextureLayer;

extern PFNGLISRENDERBUFFEREXTPROC lcIsRenderbufferEXT;
extern PFNGLBINDRENDERBUFFEREXTPROC lcBindRenderbufferEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC lcDeleteRenderbuffersEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC lcGenRenderbuffersEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC lcRenderbufferStorageEXT;
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC lcGetRenderbufferParameterivEXT;
extern PFNGLISFRAMEBUFFEREXTPROC lcIsFramebufferEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC lcBindFramebufferEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC lcDeleteFramebuffersEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC lcGenFramebuffersEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC lcCheckFramebufferStatusEXT;
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC lcFramebufferTexture1DEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC lcFramebufferTexture2DEXT;
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC lcFramebufferTexture3DEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC lcFramebufferRenderbufferEXT;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC lcGetFramebufferAttachmentParameterivEXT;
extern PFNGLGENERATEMIPMAPEXTPROC lcGenerateMipmapEXT;

extern PFNGLATTACHSHADERPROC lcAttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC lcBindAttribLocation;
extern PFNGLCOMPILESHADERPROC lcCompileShader;
extern PFNGLCREATEPROGRAMPROC lcCreateProgram;
extern PFNGLCREATESHADERPROC lcCreateShader;
extern PFNGLDELETEPROGRAMPROC lcDeleteProgram;
extern PFNGLDELETESHADERPROC lcDeleteShader;
extern PFNGLDETACHSHADERPROC lcDetachShader;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC lcDisableVertexAttribArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC lcEnableVertexAttribArray;
extern PFNGLGETACTIVEATTRIBPROC lcGetActiveAttrib;
extern PFNGLGETACTIVEUNIFORMPROC lcGetActiveUniform;
extern PFNGLGETATTACHEDSHADERSPROC lcGetAttachedShaders;
extern PFNGLGETATTRIBLOCATIONPROC lcGetAttribLocation;
extern PFNGLGETPROGRAMIVPROC lcGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC lcGetProgramInfoLog;
extern PFNGLGETSHADERIVPROC lcGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC lcGetShaderInfoLog;
extern PFNGLGETSHADERSOURCEPROC lcGetShaderSource;
extern PFNGLGETUNIFORMLOCATIONPROC lcGetUniformLocation;
extern PFNGLGETUNIFORMFVPROC lcGetUniformfv;
extern PFNGLGETUNIFORMIVPROC lcGetUniformiv;
extern PFNGLGETVERTEXATTRIBDVPROC lcGetVertexAttribdv;
extern PFNGLGETVERTEXATTRIBFVPROC lcGetVertexAttribfv;
extern PFNGLGETVERTEXATTRIBIVPROC lcGetVertexAttribiv;
extern PFNGLGETVERTEXATTRIBPOINTERVPROC lcGetVertexAttribPointerv;
extern PFNGLISPROGRAMPROC lcIsProgram;
extern PFNGLISSHADERPROC lcIsShader;
extern PFNGLLINKPROGRAMPROC lcLinkProgram;
extern PFNGLSHADERSOURCEPROC lcShaderSource;
extern PFNGLUSEPROGRAMPROC lcUseProgram;
extern PFNGLUNIFORM1FPROC lcUniform1f;
extern PFNGLUNIFORM2FPROC lcUniform2f;
extern PFNGLUNIFORM3FPROC lcUniform3f;
extern PFNGLUNIFORM4FPROC lcUniform4f;
extern PFNGLUNIFORM1IPROC lcUniform1i;
extern PFNGLUNIFORM2IPROC lcUniform2i;
extern PFNGLUNIFORM3IPROC lcUniform3i;
extern PFNGLUNIFORM4IPROC lcUniform4i;
extern PFNGLUNIFORM1FVPROC lcUniform1fv;
extern PFNGLUNIFORM2FVPROC lcUniform2fv;
extern PFNGLUNIFORM3FVPROC lcUniform3fv;
extern PFNGLUNIFORM4FVPROC lcUniform4fv;
extern PFNGLUNIFORM1IVPROC lcUniform1iv;
extern PFNGLUNIFORM2IVPROC lcUniform2iv;
extern PFNGLUNIFORM3IVPROC lcUniform3iv;
extern PFNGLUNIFORM4IVPROC lcUniform4iv;
extern PFNGLUNIFORMMATRIX2FVPROC lcUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX3FVPROC lcUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVPROC lcUniformMatrix4fv;
extern PFNGLVALIDATEPROGRAMPROC lcValidateProgram;
extern PFNGLVERTEXATTRIBPOINTERPROC lcVertexAttribPointer;

#define glBindBuffer lcBindBufferARB
#define glDeleteBuffers lcDeleteBuffersARB
#define glGenBuffers lcGenBuffersARB
#define glIsBuffer lcIsBufferARB
#define glBufferData lcBufferDataARB
#define glBufferSubData lcBufferSubDataARB
#define glGetBufferSubData lcGetBufferSubDataARB
#define glMapBuffer lcMapBufferARB
#define glUnmapBuffer lcUnmapBufferARB
#define glGetBufferParameteriv lcGetBufferParameterivARB
#define glGetBufferPointerv lcGetBufferPointervARB

#define glIsRenderbuffer lcIsRenderbuffer
#define glBindRenderbuffer lcBindRenderbuffer
#define glDeleteRenderbuffers lcDeleteRenderbuffers
#define glGenRenderbuffers lcGenRenderbuffers
#define glRenderbufferStorage lcRenderbufferStorage
#define glGetRenderbufferParameteriv lcGetRenderbufferParameteriv
#define glIsFramebuffer lcIsFramebuffer
#define glBindFramebuffer lcBindFramebuffer
#define glDeleteFramebuffers lcDeleteFramebuffers
#define glGenFramebuffers lcGenFramebuffers
#define glCheckFramebufferStatus lcCheckFramebufferStatus
#define glFramebufferTexture1D lcFramebufferTexture1D
#define glFramebufferTexture2D lcFramebufferTexture2D
#define glFramebufferTexture3D lcFramebufferTexture3D
#define glFramebufferRenderbuffer lcFramebufferRenderbuffer
#define glGetFramebufferAttachmentParameteriv lcGetFramebufferAttachmentParameteriv
#define glGenerateMipmap lcGenerateMipmap
#define glBlitFramebuffer lcBlitFramebuffer
#define glRenderbufferStorageMultisample lcRenderbufferStorageMultisample
#define glFramebufferTextureLayer lcFramebufferTextureLayer

#define glIsRenderbufferEXT lcIsRenderbufferEXT
#define glBindRenderbufferEXT lcBindRenderbufferEXT
#define glDeleteRenderbuffersEXT lcDeleteRenderbuffersEXT
#define glGenRenderbuffersEXT lcGenRenderbuffersEXT
#define glRenderbufferStorageEXT lcRenderbufferStorageEXT
#define glGetRenderbufferParameterivEXT lcGetRenderbufferParameterivEXT
#define glIsFramebufferEXT lcIsFramebufferEXT
#define glBindFramebufferEXT lcBindFramebufferEXT
#define glDeleteFramebuffersEXT lcDeleteFramebuffersEXT
#define glGenFramebuffersEXT lcGenFramebuffersEXT
#define glCheckFramebufferStatusEXT lcCheckFramebufferStatusEXT
#define glFramebufferTexture1DEXT lcFramebufferTexture1DEXT
#define glFramebufferTexture2DEXT lcFramebufferTexture2DEXT
#define glFramebufferTexture3DEXT lcFramebufferTexture3DEXT
#define glFramebufferRenderbufferEXT lcFramebufferRenderbufferEXT
#define glGetFramebufferAttachmentParameterivEXT lcGetFramebufferAttachmentParameterivEXT
#define glGenerateMipmapEXT lcGenerateMipmapEXT

#define glAttachShader lcAttachShader
#define glBindAttribLocation lcBindAttribLocation
#define glCompileShader lcCompileShader
#define glCreateProgram lcCreateProgram
#define glCreateShader lcCreateShader
#define glDeleteProgram lcDeleteProgram
#define glDeleteShader lcDeleteShader
#define glDetachShader lcDetachShader
#define glDisableVertexAttribArray lcDisableVertexAttribArray
#define glEnableVertexAttribArray lcEnableVertexAttribArray
#define glGetActiveAttrib lcGetActiveAttrib
#define glGetActiveUniform lcGetActiveUniform
#define glGetAttachedShaders lcGetAttachedShaders
#define glGetAttribLocation lcGetAttribLocation
#define glGetProgramiv lcGetProgramiv
#define glGetProgramInfoLog lcGetProgramInfoLog
#define glGetShaderiv lcGetShaderiv
#define glGetShaderInfoLog lcGetShaderInfoLog
#define glGetShaderSource lcGetShaderSource
#define glGetUniformLocation lcGetUniformLocation
#define glGetUniformfv lcGetUniformfv
#define glGetUniformiv lcGetUniformiv
#define glGetVertexAttribdv lcGetVertexAttribdv
#define glGetVertexAttribfv lcGetVertexAttribfv
#define glGetVertexAttribiv lcGetVertexAttribiv
#define glGetVertexAttribPointerv lcGetVertexAttribPointerv
#define glIsProgram lcIsProgram
#define glIsShader lcIsShader
#define glLinkProgram lcLinkProgram
#define glShaderSource lcShaderSource
#define glUseProgram lcUseProgram
#define glUniform1f lcUniform1f
#define glUniform2f lcUniform2f
#define glUniform3f lcUniform3f
#define glUniform4f lcUniform4f
#define glUniform1i lcUniform1i
#define glUniform2i lcUniform2i
#define glUniform3i lcUniform3i
#define glUniform4i lcUniform4i
#define glUniform1fv lcUniform1fv
#define glUniform2fv lcUniform2fv
#define glUniform3fv lcUniform3fv
#define glUniform4fv lcUniform4fv
#define glUniform1iv lcUniform1iv
#define glUniform2iv lcUniform2iv
#define glUniform3iv lcUniform3iv
#define glUniform4iv lcUniform4iv
#define glUniformMatrix2fv lcUniformMatrix2fv
#define glUniformMatrix3fv lcUniformMatrix3fv
#define glUniformMatrix4fv lcUniformMatrix4fv
#define glValidateProgram lcValidateProgram
#define glVertexAttribPointer lcVertexAttribPointer

#endif

#endif

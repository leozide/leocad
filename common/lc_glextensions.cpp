#include "lc_global.h"
#include "lc_glextensions.h"
#include <QOpenGLFunctions_3_2_Core>

bool gSupportsShaderObjects;
bool gSupportsVertexBufferObject;
bool gSupportsFramebufferObject;
bool gSupportsTexImage2DMultisample;
bool gSupportsBlendFuncSeparate;
bool gSupportsAnisotropic;
GLfloat gMaxAnisotropy;

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

void lcInitializeGLExtensions(const QOpenGLContext* Context)
{
	const QOpenGLFunctions* Functions = Context->functions();

#if !defined(QT_NO_DEBUG) && defined(GL_ARB_debug_output)
	if (Context->hasExtension("GL_KHR_debug"))
	{
		PFNGLDEBUGMESSAGECALLBACKARBPROC DebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKARBPROC)Context->getProcAddress("glDebugMessageCallback");

#ifndef GL_DEBUG_OUTPUT
#define GL_DEBUG_OUTPUT 0x92E0
#endif

		if (DebugMessageCallback)
		{
			DebugMessageCallback((GLDEBUGPROCARB)&lcGLDebugCallback, nullptr);
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		}
	}
#endif

	if (Context->hasExtension("GL_EXT_texture_filter_anisotropic"))
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gMaxAnisotropy);

		gSupportsAnisotropic = true;
	}

	gSupportsVertexBufferObject = Functions->hasOpenGLFeature(QOpenGLFunctions::Buffers);
	gSupportsFramebufferObject = Functions->hasOpenGLFeature(QOpenGLFunctions::Framebuffers);
	gSupportsBlendFuncSeparate = Functions->hasOpenGLFeature(QOpenGLFunctions::BlendFuncSeparate);
	gSupportsShaderObjects = Functions->hasOpenGLFeature(QOpenGLFunctions::Shaders);

	QOpenGLFunctions_3_2_Core* Funcs = Context->versionFunctions<QOpenGLFunctions_3_2_Core>();

	if (Funcs)
		gSupportsTexImage2DMultisample = true;
}

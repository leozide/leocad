#pragma once

void lcInitializeGLExtensions(const QOpenGLContext* Context);

extern bool gSupportsShaderObjects;
extern bool gSupportsVertexBufferObject;
extern bool gSupportsFramebufferObject;
extern bool gSupportsTexImage2DMultisample;
extern bool gSupportsBlendFuncSeparate;
extern bool gSupportsAnisotropic;
extern GLfloat gMaxAnisotropy;

#ifndef _OPENGL_H_
#define _OPENGL_H_

void GL_InitializeSharedExtensions(void* Data);
bool GL_ExtensionSupported(const GLubyte* Extensions, const char* Name);

extern bool GL_SupportsVertexBufferObject;
extern bool GL_UseVertexBufferObject;

inline void GL_DisableVertexBufferObject()
{
	GL_UseVertexBufferObject = false;
}

inline void GL_EnableVertexBufferObject()
{
	GL_UseVertexBufferObject = GL_SupportsVertexBufferObject;
}

inline bool GL_HasVertexBufferObject()
{
	return GL_UseVertexBufferObject;
}

#include <stddef.h>
#ifndef GL_VERSION_1_5
// GL types for handling large vertex buffer objects
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
#endif

#ifndef GL_ARB_vertex_buffer_object
// GL types for handling large vertex buffer objects
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
#endif

#ifndef GL_ARB_vertex_buffer_object
#define GL_BUFFER_SIZE_ARB                           0x8764
#define GL_BUFFER_USAGE_ARB                          0x8765
#define GL_ARRAY_BUFFER_ARB                          0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB                  0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB                  0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB          0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB           0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB           0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB            0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB            0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB    0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB        0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB  0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB   0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB           0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB    0x889F
#define GL_READ_ONLY_ARB                             0x88B8
#define GL_WRITE_ONLY_ARB                            0x88B9
#define GL_READ_WRITE_ARB                            0x88BA
#define GL_BUFFER_ACCESS_ARB                         0x88BB
#define GL_BUFFER_MAPPED_ARB                         0x88BC
#define GL_BUFFER_MAP_POINTER_ARB                    0x88BD
#define GL_STREAM_DRAW_ARB                           0x88E0
#define GL_STREAM_READ_ARB                           0x88E1
#define GL_STREAM_COPY_ARB                           0x88E2
#define GL_STATIC_DRAW_ARB                           0x88E4
#define GL_STATIC_READ_ARB                           0x88E5
#define GL_STATIC_COPY_ARB                           0x88E6
#define GL_DYNAMIC_DRAW_ARB                          0x88E8
#define GL_DYNAMIC_READ_ARB                          0x88E9
#define GL_DYNAMIC_COPY_ARB                          0x88EA
#endif

// GL_ARB_vertex_buffer_object
typedef void (APIENTRY *GLBINDBUFFERARBPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRY *GLDELETEBUFFERSARBPROC) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *GLGENBUFFERSARBPROC) (GLsizei n, GLuint *buffers);
typedef GLboolean (APIENTRY *GLISBUFFERARBPROC) (GLuint buffer);
typedef void (APIENTRY *GLBUFFERDATAARBPROC) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
typedef void (APIENTRY *GLBUFFERSUBDATAARBPROC) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
typedef void (APIENTRY *GLGETBUFFERSUBDATAARBPROC) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
typedef GLvoid* (APIENTRY *GLMAPBUFFERARBPROC) (GLenum target, GLenum access);
typedef GLboolean (APIENTRY *GLUNMAPBUFFERARBPROC) (GLenum target);
typedef void (APIENTRY *GLGETBUFFERPARAMETERIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY *GLGETBUFFERPOINTERVARBPROC) (GLenum target, GLenum pname, GLvoid* *params);

#ifndef GL_ARB_multisample
#define GL_ARB_multisample 1

#define GL_MULTISAMPLE_ARB 0x809D

#endif

extern GLBINDBUFFERARBPROC glBindBufferARB;
extern GLDELETEBUFFERSARBPROC glDeleteBuffersARB;
extern GLGENBUFFERSARBPROC glGenBuffersARB;
extern GLISBUFFERARBPROC glIsBufferARB;
extern GLBUFFERDATAARBPROC glBufferDataARB;
extern GLBUFFERSUBDATAARBPROC glBufferSubDataARB;
extern GLGETBUFFERSUBDATAARBPROC glGetBufferSubDataARB;
extern GLMAPBUFFERARBPROC glMapBufferARB;
extern GLUNMAPBUFFERARBPROC glUnmapBufferARB;
extern GLGETBUFFERPARAMETERIVARBPROC glGetBufferParameterivARB;
extern GLGETBUFFERPOINTERVARBPROC glGetBufferPointervARB;

#endif // _OPENGL_H_

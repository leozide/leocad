#ifndef _OPENGL_H_
#define _OPENGL_H_

class lcGLWidget;

void GL_InitializeSharedExtensions(lcGLWidget* Window);
bool GL_ExtensionSupported(const GLubyte* Extensions, const char* Name);

extern bool GL_SupportsVertexBufferObject;
extern bool GL_UseVertexBufferObject;
extern bool GL_SupportsFramebufferObjectARB;
extern bool GL_SupportsFramebufferObjectEXT;
extern bool GL_SupportsAnisotropic;
extern GLfloat GL_MaxAnisotropy;

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

inline bool GL_HasFramebufferObjectARB()
{
	return GL_SupportsFramebufferObjectARB;
}

inline bool GL_HasFramebufferObjectEXT()
{
	return GL_SupportsFramebufferObjectEXT;
}

bool GL_BeginRenderToTexture(int Width, int Height);
void GL_EndRenderToTexture();

#ifndef GL_VERSION_1_4
#define GL_BLEND_DST_RGB                  0x80C8
#define GL_BLEND_SRC_RGB                  0x80C9
#define GL_BLEND_DST_ALPHA                0x80CA
#define GL_BLEND_SRC_ALPHA                0x80CB
#define GL_POINT_FADE_THRESHOLD_SIZE      0x8128
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_MIRRORED_REPEAT                0x8370
#define GL_MAX_TEXTURE_LOD_BIAS           0x84FD
#define GL_TEXTURE_LOD_BIAS               0x8501
#define GL_INCR_WRAP                      0x8507
#define GL_DECR_WRAP                      0x8508
#define GL_TEXTURE_DEPTH_SIZE             0x884A
#define GL_TEXTURE_COMPARE_MODE           0x884C
#define GL_TEXTURE_COMPARE_FUNC           0x884D
#define GL_POINT_SIZE_MIN                 0x8126
#define GL_POINT_SIZE_MAX                 0x8127
#define GL_POINT_DISTANCE_ATTENUATION     0x8129
#define GL_GENERATE_MIPMAP                0x8191
#define GL_GENERATE_MIPMAP_HINT           0x8192
#define GL_FOG_COORDINATE_SOURCE          0x8450
#define GL_FOG_COORDINATE                 0x8451
#define GL_FRAGMENT_DEPTH                 0x8452
#define GL_CURRENT_FOG_COORDINATE         0x8453
#define GL_FOG_COORDINATE_ARRAY_TYPE      0x8454
#define GL_FOG_COORDINATE_ARRAY_STRIDE    0x8455
#define GL_FOG_COORDINATE_ARRAY_POINTER   0x8456
#define GL_FOG_COORDINATE_ARRAY           0x8457
#define GL_COLOR_SUM                      0x8458
#define GL_CURRENT_SECONDARY_COLOR        0x8459
#define GL_SECONDARY_COLOR_ARRAY_SIZE     0x845A
#define GL_SECONDARY_COLOR_ARRAY_TYPE     0x845B
#define GL_SECONDARY_COLOR_ARRAY_STRIDE   0x845C
#define GL_SECONDARY_COLOR_ARRAY_POINTER  0x845D
#define GL_SECONDARY_COLOR_ARRAY          0x845E
#define GL_TEXTURE_FILTER_CONTROL         0x8500
#define GL_DEPTH_TEXTURE_MODE             0x884B
#define GL_COMPARE_R_TO_TEXTURE           0x884E
#endif

#include <stddef.h>

#ifndef GL_ARB_vertex_buffer_object
// GL types for handling large vertex buffer objects
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
#endif

#ifndef GL_ARB_shader_objects
// GL types for program/shader text and shader object handles
typedef char GLcharARB;
typedef unsigned int GLhandleARB;
#endif

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT                0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT            0x84FF
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

#ifndef APIENTRY
#define APIENTRY
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

#ifndef GL_ARB_framebuffer_object
#define GL_INVALID_FRAMEBUFFER_OPERATION  0x0506
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING 0x8210
#define GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE 0x8211
#define GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE 0x8212
#define GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE 0x8213
#define GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE 0x8214
#define GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE 0x8215
#define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE 0x8216
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE 0x8217
#define GL_FRAMEBUFFER_DEFAULT            0x8218
#define GL_FRAMEBUFFER_UNDEFINED          0x8219
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_MAX_RENDERBUFFER_SIZE          0x84E8
#define GL_DEPTH_STENCIL                  0x84F9
#define GL_UNSIGNED_INT_24_8              0x84FA
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_TEXTURE_STENCIL_SIZE           0x88F1
#define GL_TEXTURE_RED_TYPE               0x8C10
#define GL_TEXTURE_GREEN_TYPE             0x8C11
#define GL_TEXTURE_BLUE_TYPE              0x8C12
#define GL_TEXTURE_ALPHA_TYPE             0x8C13
#define GL_TEXTURE_DEPTH_TYPE             0x8C16
#define GL_UNSIGNED_NORMALIZED            0x8C17
#define GL_FRAMEBUFFER_BINDING            0x8CA6
#define GL_DRAW_FRAMEBUFFER_BINDING       GL_FRAMEBUFFER_BINDING
#define GL_RENDERBUFFER_BINDING           0x8CA7
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING       0x8CAA
#define GL_RENDERBUFFER_SAMPLES           0x8CAB
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED        0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS          0x8CDF
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_COLOR_ATTACHMENT1              0x8CE1
#define GL_COLOR_ATTACHMENT2              0x8CE2
#define GL_COLOR_ATTACHMENT3              0x8CE3
#define GL_COLOR_ATTACHMENT4              0x8CE4
#define GL_COLOR_ATTACHMENT5              0x8CE5
#define GL_COLOR_ATTACHMENT6              0x8CE6
#define GL_COLOR_ATTACHMENT7              0x8CE7
#define GL_COLOR_ATTACHMENT8              0x8CE8
#define GL_COLOR_ATTACHMENT9              0x8CE9
#define GL_COLOR_ATTACHMENT10             0x8CEA
#define GL_COLOR_ATTACHMENT11             0x8CEB
#define GL_COLOR_ATTACHMENT12             0x8CEC
#define GL_COLOR_ATTACHMENT13             0x8CED
#define GL_COLOR_ATTACHMENT14             0x8CEE
#define GL_COLOR_ATTACHMENT15             0x8CEF
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_STENCIL_ATTACHMENT             0x8D20
#define GL_FRAMEBUFFER                    0x8D40
#define GL_RENDERBUFFER                   0x8D41
#define GL_RENDERBUFFER_WIDTH             0x8D42
#define GL_RENDERBUFFER_HEIGHT            0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT   0x8D44
#define GL_STENCIL_INDEX1                 0x8D46
#define GL_STENCIL_INDEX4                 0x8D47
#define GL_STENCIL_INDEX8                 0x8D48
#define GL_STENCIL_INDEX16                0x8D49
#define GL_RENDERBUFFER_RED_SIZE          0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE        0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE         0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE        0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE        0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE      0x8D55
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
#define GL_MAX_SAMPLES                    0x8D57
#endif

// GL_ARB_framebuffer_object
typedef GLboolean (APIENTRY *GLISRENDERBUFFERARBPROC) (GLuint renderbuffer);
typedef void (APIENTRY *GLBINDRENDERBUFFERARBPROC) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRY *GLDELETERENDERBUFFERSARBPROC) (GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY *GLGENRENDERBUFFERSARBPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY *GLRENDERBUFFERSTORAGEARBPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY *GLGETRENDERBUFFERPARAMETERIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef GLboolean (APIENTRY *GLISFRAMEBUFFERARBPROC) (GLuint framebuffer);
typedef void (APIENTRY *GLBINDFRAMEBUFFERARBPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRY *GLDELETEFRAMEBUFFERSARBPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *GLGENFRAMEBUFFERSARBPROC) (GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRY *GLCHECKFRAMEBUFFERSTATUSARBPROC) (GLenum target);
typedef void (APIENTRY *GLFRAMEBUFFERTEXTURE1DARBPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY *GLFRAMEBUFFERTEXTURE2DARBPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY *GLFRAMEBUFFERTEXTURE3DARBPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
typedef void (APIENTRY *GLFRAMEBUFFERRENDERBUFFERARBPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRY *GLGETFRAMEBUFFERATTACHMENTPARAMETERIVARBPROC) (GLenum target, GLenum attachment, GLenum pname, GLint *params);
typedef void (APIENTRY *GLGENERATEMIPMAPARBPROC) (GLenum target);
typedef void (APIENTRY *GLBLITFRAMEBUFFERARBPROC) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef void (APIENTRY *GLRENDERBUFFERSTORAGEMULTISAMPLEARBPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY *GLFRAMEBUFFERTEXTURELAYERARBPROC) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);

#ifndef GL_EXT_framebuffer_object
#define GL_EXT_framebuffer_object 1
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT 0x0506
#define GL_MAX_RENDERBUFFER_SIZE_EXT      0x84E8
#define GL_FRAMEBUFFER_BINDING_EXT        0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT       0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE_EXT       0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT 0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT 0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT    0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS_EXT      0x8CDF
#define GL_COLOR_ATTACHMENT0_EXT          0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT          0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT          0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT          0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT          0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT          0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT          0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT          0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT          0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT          0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT         0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT         0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT         0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT         0x8CED
#define GL_COLOR_ATTACHMENT14_EXT         0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT         0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT           0x8D00
#define GL_STENCIL_ATTACHMENT_EXT         0x8D20
#define GL_FRAMEBUFFER_EXT                0x8D40
#define GL_RENDERBUFFER_EXT               0x8D41
#define GL_RENDERBUFFER_WIDTH_EXT         0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT        0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT 0x8D44
#define GL_STENCIL_INDEX1_EXT             0x8D46
#define GL_STENCIL_INDEX4_EXT             0x8D47
#define GL_STENCIL_INDEX8_EXT             0x8D48
#define GL_STENCIL_INDEX16_EXT            0x8D49
#define GL_RENDERBUFFER_RED_SIZE_EXT      0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT    0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT     0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT    0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT    0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT  0x8D55
#endif

// GL_EXT_framebuffer_object
typedef GLboolean (APIENTRY *GLISRENDERBUFFEREXTPROC) (GLuint renderbuffer);
typedef void (APIENTRY *GLBINDRENDERBUFFEREXTPROC) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRY *GLDELETERENDERBUFFERSEXTPROC) (GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY *GLGENRENDERBUFFERSEXTPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY *GLRENDERBUFFERSTORAGEEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY *GLGETRENDERBUFFERPARAMETERIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef GLboolean (APIENTRY *GLISFRAMEBUFFEREXTPROC) (GLuint framebuffer);
typedef void (APIENTRY *GLBINDFRAMEBUFFEREXTPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRY *GLDELETEFRAMEBUFFERSEXTPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *GLGENFRAMEBUFFERSEXTPROC) (GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRY *GLCHECKFRAMEBUFFERSTATUSEXTPROC) (GLenum target);
typedef void (APIENTRY *GLFRAMEBUFFERTEXTURE1DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY *GLFRAMEBUFFERTEXTURE2DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY *GLFRAMEBUFFERTEXTURE3DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
typedef void (APIENTRY *GLFRAMEBUFFERRENDERBUFFEREXTPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRY *GLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) (GLenum target, GLenum attachment, GLenum pname, GLint *params);
typedef void (APIENTRY *GLGENERATEMIPMAPEXTPROC) (GLenum target);

#ifndef GL_ARB_shader_objects
#define GL_PROGRAM_OBJECT_ARB                        0x8B40
#define GL_SHADER_OBJECT_ARB                         0x8B48
#define GL_OBJECT_TYPE_ARB                           0x8B4E
#define GL_OBJECT_SUBTYPE_ARB                        0x8B4F
#define GL_FLOAT_VEC2_ARB                            0x8B50
#define GL_FLOAT_VEC3_ARB                            0x8B51
#define GL_FLOAT_VEC4_ARB                            0x8B52
#define GL_INT_VEC2_ARB                              0x8B53
#define GL_INT_VEC3_ARB                              0x8B54
#define GL_INT_VEC4_ARB                              0x8B55
#define GL_BOOL_ARB                                  0x8B56
#define GL_BOOL_VEC2_ARB                             0x8B57
#define GL_BOOL_VEC3_ARB                             0x8B58
#define GL_BOOL_VEC4_ARB                             0x8B59
#define GL_FLOAT_MAT2_ARB                            0x8B5A
#define GL_FLOAT_MAT3_ARB                            0x8B5B
#define GL_FLOAT_MAT4_ARB                            0x8B5C
#define GL_SAMPLER_1D_ARB                            0x8B5D
#define GL_SAMPLER_2D_ARB                            0x8B5E
#define GL_SAMPLER_3D_ARB                            0x8B5F
#define GL_SAMPLER_CUBE_ARB                          0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB                     0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB                     0x8B62
#define GL_SAMPLER_2D_RECT_ARB                       0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB                0x8B64
#define GL_OBJECT_DELETE_STATUS_ARB                  0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB                 0x8B81
#define GL_OBJECT_LINK_STATUS_ARB                    0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB                0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB                0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB               0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB                0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB      0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB           0x8B88
#endif

// GL_ARB_shader_objects
typedef void (APIENTRY *GLDELETEOBJECTARBPROC) (GLhandleARB obj);
typedef GLhandleARB (APIENTRY *GLGETHANDLEARBPROC) (GLenum pname);
typedef void (APIENTRY *GLDETACHOBJECTARBPROC) (GLhandleARB containerObj, GLhandleARB attachedObj);
typedef GLhandleARB (APIENTRY *GLCREATESHADEROBJECTARBPROC) (GLenum shaderType);
typedef void (APIENTRY *GLSHADERSOURCEARBPROC) (GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length);
typedef void (APIENTRY *GLCOMPILESHADERARBPROC) (GLhandleARB shaderObj);
typedef GLhandleARB (APIENTRY *GLCREATEPROGRAMOBJECTARBPROC) (void);
typedef void (APIENTRY *GLATTACHOBJECTARBPROC) (GLhandleARB containerObj, GLhandleARB obj);
typedef void (APIENTRY *GLLINKPROGRAMARBPROC) (GLhandleARB programObj);
typedef void (APIENTRY *GLUSEPROGRAMOBJECTARBPROC) (GLhandleARB programObj);
typedef void (APIENTRY *GLVALIDATEPROGRAMARBPROC) (GLhandleARB programObj);
typedef void (APIENTRY *GLUNIFORM1FARBPROC) (GLint location, GLfloat v0);
typedef void (APIENTRY *GLUNIFORM2FARBPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY *GLUNIFORM3FARBPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY *GLUNIFORM4FARBPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY *GLUNIFORM1IARBPROC) (GLint location, GLint v0);
typedef void (APIENTRY *GLUNIFORM2IARBPROC) (GLint location, GLint v0, GLint v1);
typedef void (APIENTRY *GLUNIFORM3IARBPROC) (GLint location, GLint v0, GLint v1, GLint v2);
typedef void (APIENTRY *GLUNIFORM4IARBPROC) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void (APIENTRY *GLUNIFORM1FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *GLUNIFORM2FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *GLUNIFORM3FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *GLUNIFORM4FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *GLUNIFORM1IVARBPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *GLUNIFORM2IVARBPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *GLUNIFORM3IVARBPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *GLUNIFORM4IVARBPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *GLUNIFORMMATRIX2FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY *GLUNIFORMMATRIX3FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY *GLUNIFORMMATRIX4FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY *GLGETOBJECTPARAMETERFVARBPROC) (GLhandleARB obj, GLenum pname, GLfloat *params);
typedef void (APIENTRY *GLGETOBJECTPARAMETERIVARBPROC) (GLhandleARB obj, GLenum pname, GLint *params);
typedef void (APIENTRY *GLGETINFOLOGARBPROC) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
typedef void (APIENTRY *GLGETATTACHEDOBJECTSARBPROC) (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
typedef GLint (APIENTRY *GLGETUNIFORMLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);
typedef void (APIENTRY *GLGETACTIVEUNIFORMARBPROC) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
typedef void (APIENTRY *GLGETUNIFORMFVARBPROC) (GLhandleARB programObj, GLint location, GLfloat *params);
typedef void (APIENTRY *GLGETUNIFORMIVARBPROC) (GLhandleARB programObj, GLint location, GLint *params);
typedef void (APIENTRY *GLGETSHADERSOURCEARBPROC) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);

#ifndef GL_ARB_vertex_shader
#define GL_VERTEX_SHADER_ARB                         0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB         0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB                    0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB        0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB      0x8B4D
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB              0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB    0x8B8A
#endif

// GL_ARB_vertex_shader
typedef void (APIENTRY *GLBINDATTRIBLOCATIONARBPROC) (GLhandleARB programObj, GLuint index, const GLcharARB *name);
typedef void (APIENTRY *GLGETACTIVEATTRIBARBPROC) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
typedef GLint (APIENTRY *GLGETATTRIBLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);

#ifndef GL_ARB_fragment_shader
#define GL_FRAGMENT_SHADER_ARB                       0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB       0x8B49
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB       0x8B8B
#endif

#ifndef GL_ARB_fragment_shader
#define GL_ARB_fragment_shader 1
#endif

extern GLBINDBUFFERARBPROC lcBindBufferARB;
extern GLDELETEBUFFERSARBPROC lcDeleteBuffersARB;
extern GLGENBUFFERSARBPROC lcGenBuffersARB;
extern GLISBUFFERARBPROC lcIsBufferARB;
extern GLBUFFERDATAARBPROC lcBufferDataARB;
extern GLBUFFERSUBDATAARBPROC lcBufferSubDataARB;
extern GLGETBUFFERSUBDATAARBPROC lcGetBufferSubDataARB;
extern GLMAPBUFFERARBPROC lcMapBufferARB;
extern GLUNMAPBUFFERARBPROC lcUnmapBufferARB;
extern GLGETBUFFERPARAMETERIVARBPROC lcGetBufferParameterivARB;
extern GLGETBUFFERPOINTERVARBPROC lcGetBufferPointervARB;

extern GLISRENDERBUFFERARBPROC lcIsRenderbufferARB;
extern GLBINDRENDERBUFFERARBPROC lcBindRenderbufferARB;
extern GLDELETERENDERBUFFERSARBPROC lcDeleteRenderbuffersARB;
extern GLGENRENDERBUFFERSARBPROC lcGenRenderbuffersARB;
extern GLRENDERBUFFERSTORAGEARBPROC lcRenderbufferStorageARB;
extern GLGETRENDERBUFFERPARAMETERIVARBPROC lcGetRenderbufferParameterivARB;
extern GLISFRAMEBUFFERARBPROC lcIsFramebufferARB;
extern GLBINDFRAMEBUFFERARBPROC lcBindFramebufferARB;
extern GLDELETEFRAMEBUFFERSARBPROC lcDeleteFramebuffersARB;
extern GLGENFRAMEBUFFERSARBPROC lcGenFramebuffersARB;
extern GLCHECKFRAMEBUFFERSTATUSARBPROC lcCheckFramebufferStatusARB;
extern GLFRAMEBUFFERTEXTURE1DARBPROC lcFramebufferTexture1DARB;
extern GLFRAMEBUFFERTEXTURE2DARBPROC lcFramebufferTexture2DARB;
extern GLFRAMEBUFFERTEXTURE3DARBPROC lcFramebufferTexture3DARB;
extern GLFRAMEBUFFERRENDERBUFFERARBPROC lcFramebufferRenderbufferARB;
extern GLGETFRAMEBUFFERATTACHMENTPARAMETERIVARBPROC lcGetFramebufferAttachmentParameterivARB;
extern GLGENERATEMIPMAPARBPROC lcGenerateMipmapARB;
extern GLBLITFRAMEBUFFERARBPROC lcBlitFramebufferARB;
extern GLRENDERBUFFERSTORAGEMULTISAMPLEARBPROC lcRenderbufferStorageMultisampleARB;
extern GLFRAMEBUFFERTEXTURELAYERARBPROC lcFramebufferTextureLayerARB;

extern GLISRENDERBUFFEREXTPROC lcIsRenderbufferEXT;
extern GLBINDRENDERBUFFEREXTPROC lcBindRenderbufferEXT;
extern GLDELETERENDERBUFFERSEXTPROC lcDeleteRenderbuffersEXT;
extern GLGENRENDERBUFFERSEXTPROC lcGenRenderbuffersEXT;
extern GLRENDERBUFFERSTORAGEEXTPROC lcRenderbufferStorageEXT;
extern GLGETRENDERBUFFERPARAMETERIVEXTPROC lcGetRenderbufferParameterivEXT;
extern GLISFRAMEBUFFEREXTPROC lcIsFramebufferEXT;
extern GLBINDFRAMEBUFFEREXTPROC lcBindFramebufferEXT;
extern GLDELETEFRAMEBUFFERSEXTPROC lcDeleteFramebuffersEXT;
extern GLGENFRAMEBUFFERSEXTPROC lcGenFramebuffersEXT;
extern GLCHECKFRAMEBUFFERSTATUSEXTPROC lcCheckFramebufferStatusEXT;
extern GLFRAMEBUFFERTEXTURE1DEXTPROC lcFramebufferTexture1DEXT;
extern GLFRAMEBUFFERTEXTURE2DEXTPROC lcFramebufferTexture2DEXT;
extern GLFRAMEBUFFERTEXTURE3DEXTPROC lcFramebufferTexture3DEXT;
extern GLFRAMEBUFFERRENDERBUFFEREXTPROC lcFramebufferRenderbufferEXT;
extern GLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC lcGetFramebufferAttachmentParameterivEXT;
extern GLGENERATEMIPMAPEXTPROC lcGenerateMipmapEXT;

extern GLDELETEOBJECTARBPROC lcDeleteObjectARB;
extern GLGETHANDLEARBPROC lcGetHandleARB;
extern GLDETACHOBJECTARBPROC lcDetachObjectARB;
extern GLCREATESHADEROBJECTARBPROC lcCreateShaderObjectARB;
extern GLSHADERSOURCEARBPROC lcShaderSourceARB;
extern GLCOMPILESHADERARBPROC lcCompileShaderARB;
extern GLCREATEPROGRAMOBJECTARBPROC lcCreateProgramObjectARB;
extern GLATTACHOBJECTARBPROC lcAttachObjectARB;
extern GLLINKPROGRAMARBPROC lcLinkProgramARB;
extern GLUSEPROGRAMOBJECTARBPROC lcUseProgramObjectARB;
extern GLVALIDATEPROGRAMARBPROC lcValidateProgramARB;
extern GLUNIFORM1FARBPROC lcUniform1fARB;
extern GLUNIFORM2FARBPROC lcUniform2fARB;
extern GLUNIFORM3FARBPROC lcUniform3fARB;
extern GLUNIFORM4FARBPROC lcUniform4fARB;
extern GLUNIFORM1IARBPROC lcUniform1iARB;
extern GLUNIFORM2IARBPROC lcUniform2iARB;
extern GLUNIFORM3IARBPROC lcUniform3iARB;
extern GLUNIFORM4IARBPROC lcUniform4iARB;
extern GLUNIFORM1FVARBPROC lcUniform1fvARB;
extern GLUNIFORM2FVARBPROC lcUniform2fvARB;
extern GLUNIFORM3FVARBPROC lcUniform3fvARB;
extern GLUNIFORM4FVARBPROC lcUniform4fvARB;
extern GLUNIFORM1IVARBPROC lcUniform1ivARB;
extern GLUNIFORM2IVARBPROC lcUniform2ivARB;
extern GLUNIFORM3IVARBPROC lcUniform3ivARB;
extern GLUNIFORM4IVARBPROC lcUniform4ivARB;
extern GLUNIFORMMATRIX2FVARBPROC lcUniformMatrix2fvARB;
extern GLUNIFORMMATRIX3FVARBPROC lcUniformMatrix3fvARB;
extern GLUNIFORMMATRIX4FVARBPROC lcUniformMatrix4fvARB;
extern GLGETOBJECTPARAMETERFVARBPROC lcGetObjectParameterfvARB;
extern GLGETOBJECTPARAMETERIVARBPROC lcGetObjectParameterivARB;
extern GLGETINFOLOGARBPROC lcGetInfoLogARB;
extern GLGETATTACHEDOBJECTSARBPROC lcGetAttachedObjectsARB;
extern GLGETUNIFORMLOCATIONARBPROC lcGetUniformLocationARB;
extern GLGETACTIVEUNIFORMARBPROC lcGetActiveUniformARB;
extern GLGETUNIFORMFVARBPROC lcGetUniformfvARB;
extern GLGETUNIFORMIVARBPROC lcGetUniformivARB;
extern GLGETSHADERSOURCEARBPROC lcGetShaderSourceARB;

extern GLBINDATTRIBLOCATIONARBPROC lcBindAttribLocationARB;
extern GLGETACTIVEATTRIBARBPROC lcGetActiveAttribARB;
extern GLGETATTRIBLOCATIONARBPROC lcGetAttribLocationARB;

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

#define glIsRenderbuffer lcIsRenderbufferARB
#define glBindRenderbuffer lcBindRenderbufferARB
#define glDeleteRenderbuffers lcDeleteRenderbuffersARB
#define glGenRenderbuffers lcGenRenderbuffersARB
#define glRenderbufferStorage lcRenderbufferStorageARB
#define glGetRenderbufferParameteriv lcGetRenderbufferParameterivARB
#define glIsFramebuffer lcIsFramebufferARB
#define glBindFramebuffer lcBindFramebufferARB
#define glDeleteFramebuffers lcDeleteFramebuffersARB
#define glGenFramebuffers lcGenFramebuffersARB
#define glCheckFramebufferStatus lcCheckFramebufferStatusARB
#define glFramebufferTexture1D lcFramebufferTexture1DARB
#define glFramebufferTexture2D lcFramebufferTexture2DARB
#define glFramebufferTexture3D lcFramebufferTexture3DARB
#define glFramebufferRenderbuffer lcFramebufferRenderbufferARB
#define glGetFramebufferAttachmentParameteriv lcGetFramebufferAttachmentParameterivARB
#define glGenerateMipmap lcGenerateMipmapARB
#define glBlitFramebuffer lcBlitFramebufferARB
#define glRenderbufferStorageMultisample lcRenderbufferStorageMultisampleARB
#define glFramebufferTextureLayer lcFramebufferTextureLayerARB

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

#define glDeleteObjectARB lcDeleteObjectARB
#define glGetHandleARB lcGetHandleARB
#define glDetachObjectARB lcDetachObjectARB
#define glCreateShaderObjectARB lcCreateShaderObjectARB
#define glShaderSourceARB lcShaderSourceARB
#define glCompileShaderARB lcCompileShaderARB
#define glCreateProgramObjectARB lcCreateProgramObjectARB
#define glAttachObjectARB lcAttachObjectARB
#define glLinkProgramARB lcLinkProgramARB
#define glUseProgramObjectARB lcUseProgramObjectARB
#define glValidateProgramARB lcValidateProgramARB
#define glUniform1fARB lcUniform1fARB
#define glUniform2fARB lcUniform2fARB
#define glUniform3fARB lcUniform3fARB
#define glUniform4fARB lcUniform4fARB
#define glUniform1iARB lcUniform1iARB
#define glUniform2iARB lcUniform2iARB
#define glUniform3iARB lcUniform3iARB
#define glUniform4iARB lcUniform4iARB
#define glUniform1fvARB lcUniform1fvARB
#define glUniform2fvARB lcUniform2fvARB
#define glUniform3fvARB lcUniform3fvARB
#define glUniform4fvARB lcUniform4fvARB
#define glUniform1ivARB lcUniform1ivARB
#define glUniform2ivARB lcUniform2ivARB
#define glUniform3ivARB lcUniform3ivARB
#define glUniform4ivARB lcUniform4ivARB
#define glUniformMatrix2fvARB lcUniformMatrix2fvARB
#define glUniformMatrix3fvARB lcUniformMatrix3fvARB
#define glUniformMatrix4fvARB lcUniformMatrix4fvARB
#define glGetObjectParameterfvARB lcGetObjectParameterfvARB
#define glGetObjectParameterivARB lcGetObjectParameterivARB
#define glGetInfoLogARB lcGetInfoLogARB
#define glGetAttachedObjectsARB lcGetAttachedObjectsARB
#define glGetUniformLocationARB lcGetUniformLocationARB
#define glGetActiveUniformARB lcGetActiveUniformARB
#define glGetUniformfvARB lcGetUniformfvARB
#define glGetUniformivARB lcGetUniformivARB
#define glGetShaderSourceARB lcGetShaderSourceARB

#define glBindAttribLocationARB lcBindAttribLocationARB
#define glGetActiveAttribARB lcGetActiveAttribARB
#define glGetAttribLocationARB lcGetAttribLocationARB

#endif // _OPENGL_H_

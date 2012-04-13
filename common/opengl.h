#ifndef _OPENGL_H_
#define _OPENGL_H_

#ifdef LC_LINUX
#include <GL/gl.h>
#include <GL/glx.h>
#include "linux_gl.h"
#define LC_OPENGL_DYNAMIC 1
#endif 

#ifdef LC_MACOSX
#include <OpenGL/gl.h>
#include <AGL/agl.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

void gluLookAt (GLdouble eyex, GLdouble eyey, GLdouble eyez,
		GLdouble centerx, GLdouble centery, GLdouble centerz,
		GLdouble upx, GLdouble upy, GLdouble upz);
void gluPerspective (GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

bool GL_Initialize(const char* libname);
void GL_Shutdown();
bool GL_InitializeExtensions();

inline bool GL_HasVertexBufferObject()
{
	extern bool GL_VertexBufferObject;
	return GL_VertexBufferObject;
}

// =============================================================================
// OpenGL functions typedefs

#ifdef LC_OPENGL_DYNAMIC

// Miscellaneous
typedef void (APIENTRY *PFNGLCLEARINDEX) (GLfloat c);
typedef void (APIENTRY *PFNGLCLEARCOLOR) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (APIENTRY *PFNGLCLEAR) (GLbitfield mask);
typedef void (APIENTRY *PFNGLINDEXMASK) (GLuint mask);
typedef void (APIENTRY *PFNGLCOLORMASK) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
typedef void (APIENTRY *PFNGLALPHAFUNC) (GLenum func, GLclampf ref);
typedef void (APIENTRY *PFNGLBLENDFUNC) (GLenum sfactor, GLenum dfactor);
typedef void (APIENTRY *PFNGLLOGICOP) (GLenum opcode);
typedef void (APIENTRY *PFNGLCULLFACE) (GLenum mode);
typedef void (APIENTRY *PFNGLFRONTFACE) (GLenum mode);
typedef void (APIENTRY *PFNGLPOINTSIZE) (GLfloat size);
typedef void (APIENTRY *PFNGLLINEWIDTH) (GLfloat width);
typedef void (APIENTRY *PFNGLLINESTIPPLE) (GLint factor, GLushort pattern);
typedef void (APIENTRY *PFNGLPOLYGONMODE) (GLenum face, GLenum mode);
typedef void (APIENTRY *PFNGLPOLYGONOFFSET) (GLfloat factor, GLfloat units);
typedef void (APIENTRY *PFNGLPOLYGONSTIPPLE) (const GLubyte *mask);
typedef void (APIENTRY *PFNGLGETPOLYGONSTIPPLE) (GLubyte *mask);
typedef void (APIENTRY *PFNGLEDGEFLAG) (GLboolean flag);
typedef void (APIENTRY *PFNGLEDGEFLAGV) (const GLboolean *flag);
typedef void (APIENTRY *PFNGLSCISSOR) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRY *PFNGLCLIPPLANE) (GLenum plane, const GLdouble *equation);
typedef void (APIENTRY *PFNGLGETCLIPPLANE) (GLenum plane, GLdouble *equation);
typedef void (APIENTRY *PFNGLDRAWBUFFER) (GLenum mode);
typedef void (APIENTRY *PFNGLREADBUFFER) (GLenum mode);
typedef void (APIENTRY *PFNGLENABLE) (GLenum cap);
typedef void (APIENTRY *PFNGLDISABLE) (GLenum cap);
typedef GLboolean (APIENTRY *PFNGLISENABLED) (GLenum cap);
typedef void (APIENTRY *PFNGLENABLECLIENTSTATE) (GLenum cap);
typedef void (APIENTRY *PFNGLDISABLECLIENTSTATE) (GLenum cap);
typedef void (APIENTRY *PFNGLGETBOOLEANV) (GLenum pname, GLboolean *params);
typedef void (APIENTRY *PFNGLGETDOUBLEV) (GLenum pname, GLdouble *params);
typedef void (APIENTRY *PFNGLGETFLOATV) (GLenum pname, GLfloat *params);
typedef void (APIENTRY *PFNGLGETINTEGERV) (GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLPUSHATTRIB) (GLbitfield mask);
typedef void (APIENTRY *PFNGLPOPATTRIB) (void);
typedef void (APIENTRY *PFNGLPUSHCLIENTATTRIB) (GLbitfield mask);
typedef void (APIENTRY *PFNGLPOPCLIENTATTRIB) (void);
typedef GLint (APIENTRY *PFNGLRENDERMODE) (GLenum mode);
typedef GLenum (APIENTRY *PFNGLGETERROR) (void);
typedef const GLubyte* (APIENTRY *PFNGLGETSTRING) (GLenum name);
typedef void (APIENTRY *PFNGLFINISH) (void);
typedef void (APIENTRY *PFNGLFLUSH) (void);
typedef void (APIENTRY *PFNGLHINT) (GLenum target, GLenum mode);

// Depth Buffer
typedef void (APIENTRY *PFNGLCLEARDEPTH) (GLclampd depth);
typedef void (APIENTRY *PFNGLDEPTHFUNC) (GLenum func);
typedef void (APIENTRY *PFNGLDEPTHMASK) (GLboolean flag);
typedef void (APIENTRY *PFNGLDEPTHRANGE) (GLclampd near_val, GLclampd far_val);

// Accumulation Buffer
typedef void (APIENTRY *PFNGLCLEARACCUM) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (APIENTRY *PFNGLACCUM) (GLenum op, GLfloat value);

// Transformation
typedef void (APIENTRY *PFNGLMATRIXMODE) (GLenum mode);
typedef void (APIENTRY *PFNGLORTHO) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
typedef void (APIENTRY *PFNGLFRUSTUM) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
typedef void (APIENTRY *PFNGLVIEWPORT) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRY *PFNGLPUSHMATRIX) (void);
typedef void (APIENTRY *PFNGLPOPMATRIX) (void);
typedef void (APIENTRY *PFNGLLOADIDENTITY) (void);
typedef void (APIENTRY *PFNGLLOADMATRIXD) (const GLdouble *m);
typedef void (APIENTRY *PFNGLLOADMATRIXF) (const GLfloat *m);
typedef void (APIENTRY *PFNGLMULTMATRIXD) (const GLdouble *m);
typedef void (APIENTRY *PFNGLMULTMATRIXF) (const GLfloat *m);
typedef void (APIENTRY *PFNGLROTATED) (GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY *PFNGLROTATEF) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY *PFNGLSCALED) (GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY *PFNGLSCALEF) (GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY *PFNGLTRANSLATED) (GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY *PFNGLTRANSLATEF) (GLfloat x, GLfloat y, GLfloat z);

// Display Lists
typedef GLboolean (APIENTRY *PFNGLISLIST) (GLuint list);
typedef void (APIENTRY *PFNGLDELETELISTS) (GLuint list, GLsizei range);
typedef GLuint (APIENTRY *PFNGLGENLISTS) (GLsizei range);
typedef void (APIENTRY *PFNGLNEWLIST) (GLuint list, GLenum mode);
typedef void (APIENTRY *PFNGLENDLIST) (void);
typedef void (APIENTRY *PFNGLCALLLIST) (GLuint list);
typedef void (APIENTRY *PFNGLCALLLISTS) (GLsizei n, GLenum type, const GLvoid *lists);
typedef void (APIENTRY *PFNGLLISTBASE) (GLuint base);

// Drawing Functions
typedef void (APIENTRY *PFNGLBEGIN) (GLenum mode);
typedef void (APIENTRY *PFNGLEND) (void);
typedef void (APIENTRY *PFNGLVERTEX2D) (GLdouble x, GLdouble y);
typedef void (APIENTRY *PFNGLVERTEX2F) (GLfloat x, GLfloat y);
typedef void (APIENTRY *PFNGLVERTEX2I) (GLint x, GLint y);
typedef void (APIENTRY *PFNGLVERTEX2S) (GLshort x, GLshort y);
typedef void (APIENTRY *PFNGLVERTEX3D) (GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY *PFNGLVERTEX3F) (GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY *PFNGLVERTEX3I) (GLint x, GLint y, GLint z);
typedef void (APIENTRY *PFNGLVERTEX3S) (GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY *PFNGLVERTEX4D) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY *PFNGLVERTEX4F) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY *PFNGLVERTEX4I) (GLint x, GLint y, GLint z, GLint w);
typedef void (APIENTRY *PFNGLVERTEX4S) (GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY *PFNGLVERTEX2DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLVERTEX2FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLVERTEX2IV) (const GLint *v);
typedef void (APIENTRY *PFNGLVERTEX2SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLVERTEX3DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLVERTEX3FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLVERTEX3IV) (const GLint *v);
typedef void (APIENTRY *PFNGLVERTEX3SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLVERTEX4DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLVERTEX4FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLVERTEX4IV) (const GLint *v);
typedef void (APIENTRY *PFNGLVERTEX4SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLNORMAL3B) (GLbyte nx, GLbyte ny, GLbyte nz);
typedef void (APIENTRY *PFNGLNORMAL3D) (GLdouble nx, GLdouble ny, GLdouble nz);
typedef void (APIENTRY *PFNGLNORMAL3F) (GLfloat nx, GLfloat ny, GLfloat nz);
typedef void (APIENTRY *PFNGLNORMAL3I) (GLint nx, GLint ny, GLint nz);
typedef void (APIENTRY *PFNGLNORMAL3S) (GLshort nx, GLshort ny, GLshort nz);
typedef void (APIENTRY *PFNGLNORMAL3BV) (const GLbyte *v);
typedef void (APIENTRY *PFNGLNORMAL3DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLNORMAL3FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLNORMAL3IV) (const GLint *v);
typedef void (APIENTRY *PFNGLNORMAL3SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLINDEXD) (GLdouble c);
typedef void (APIENTRY *PFNGLINDEXF) (GLfloat c);
typedef void (APIENTRY *PFNGLINDEXI) (GLint c);
typedef void (APIENTRY *PFNGLINDEXS) (GLshort c);
typedef void (APIENTRY *PFNGLINDEXUB) (GLubyte c);
typedef void (APIENTRY *PFNGLINDEXDV) (const GLdouble *c);
typedef void (APIENTRY *PFNGLINDEXFV) (const GLfloat *c);
typedef void (APIENTRY *PFNGLINDEXIV) (const GLint *c);
typedef void (APIENTRY *PFNGLINDEXSV) (const GLshort *c);
typedef void (APIENTRY *PFNGLINDEXUBV) (const GLubyte *c);
typedef void (APIENTRY *PFNGLCOLOR3B) (GLbyte red, GLbyte green, GLbyte blue);
typedef void (APIENTRY *PFNGLCOLOR3D) (GLdouble red, GLdouble green, GLdouble blue);
typedef void (APIENTRY *PFNGLCOLOR3F) (GLfloat red, GLfloat green, GLfloat blue);
typedef void (APIENTRY *PFNGLCOLOR3I) (GLint red, GLint green, GLint blue);
typedef void (APIENTRY *PFNGLCOLOR3S) (GLshort red, GLshort green, GLshort blue);
typedef void (APIENTRY *PFNGLCOLOR3UB) (GLubyte red, GLubyte green, GLubyte blue);
typedef void (APIENTRY *PFNGLCOLOR3UI) (GLuint red, GLuint green, GLuint blue);
typedef void (APIENTRY *PFNGLCOLOR3US) (GLushort red, GLushort green, GLushort blue);
typedef void (APIENTRY *PFNGLCOLOR4B) (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
typedef void (APIENTRY *PFNGLCOLOR4D) (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
typedef void (APIENTRY *PFNGLCOLOR4F) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (APIENTRY *PFNGLCOLOR4I) (GLint red, GLint green, GLint blue, GLint alpha);
typedef void (APIENTRY *PFNGLCOLOR4S) (GLshort red, GLshort green, GLshort blue, GLshort alpha);
typedef void (APIENTRY *PFNGLCOLOR4UB) (GLubyte red, GLubyte green,GLubyte blue, GLubyte alpha);
typedef void (APIENTRY *PFNGLCOLOR4UI) (GLuint red, GLuint green, GLuint blue, GLuint alpha);
typedef void (APIENTRY *PFNGLCOLOR4US) (GLushort red, GLushort green, GLushort blue, GLushort alpha);
typedef void (APIENTRY *PFNGLCOLOR3BV) (const GLbyte *v);
typedef void (APIENTRY *PFNGLCOLOR3DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLCOLOR3FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLCOLOR3IV) (const GLint *v);
typedef void (APIENTRY *PFNGLCOLOR3SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLCOLOR3UBV) (const GLubyte *v);
typedef void (APIENTRY *PFNGLCOLOR3UIV) (const GLuint *v);
typedef void (APIENTRY *PFNGLCOLOR3USV) (const GLushort *v);
typedef void (APIENTRY *PFNGLCOLOR4BV) (const GLbyte *v);
typedef void (APIENTRY *PFNGLCOLOR4DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLCOLOR4FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLCOLOR4IV) (const GLint *v);
typedef void (APIENTRY *PFNGLCOLOR4SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLCOLOR4UBV) (const GLubyte *v);
typedef void (APIENTRY *PFNGLCOLOR4UIV) (const GLuint *v);
typedef void (APIENTRY *PFNGLCOLOR4USV) (const GLushort *v);
typedef void (APIENTRY *PFNGLTEXCOORD1D) (GLdouble s);
typedef void (APIENTRY *PFNGLTEXCOORD1F) (GLfloat s);
typedef void (APIENTRY *PFNGLTEXCOORD1I) (GLint s);
typedef void (APIENTRY *PFNGLTEXCOORD1S) (GLshort s);
typedef void (APIENTRY *PFNGLTEXCOORD2D) (GLdouble s, GLdouble t);
typedef void (APIENTRY *PFNGLTEXCOORD2F) (GLfloat s, GLfloat t);
typedef void (APIENTRY *PFNGLTEXCOORD2I) (GLint s, GLint t);
typedef void (APIENTRY *PFNGLTEXCOORD2S) (GLshort s, GLshort t);
typedef void (APIENTRY *PFNGLTEXCOORD3D) (GLdouble s, GLdouble t, GLdouble r);
typedef void (APIENTRY *PFNGLTEXCOORD3F) (GLfloat s, GLfloat t, GLfloat r);
typedef void (APIENTRY *PFNGLTEXCOORD3I) (GLint s, GLint t, GLint r);
typedef void (APIENTRY *PFNGLTEXCOORD3S) (GLshort s, GLshort t, GLshort r);
typedef void (APIENTRY *PFNGLTEXCOORD4D) (GLdouble s, GLdouble t, GLdouble r, GLdouble q);
typedef void (APIENTRY *PFNGLTEXCOORD4F) (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void (APIENTRY *PFNGLTEXCOORD4I) (GLint s, GLint t, GLint r, GLint q);
typedef void (APIENTRY *PFNGLTEXCOORD4S) (GLshort s, GLshort t, GLshort r, GLshort q);
typedef void (APIENTRY *PFNGLTEXCOORD1DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLTEXCOORD1FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLTEXCOORD1IV) (const GLint *v);
typedef void (APIENTRY *PFNGLTEXCOORD1SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLTEXCOORD2DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLTEXCOORD2FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLTEXCOORD2IV) (const GLint *v);
typedef void (APIENTRY *PFNGLTEXCOORD2SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLTEXCOORD3DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLTEXCOORD3FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLTEXCOORD3IV) (const GLint *v);
typedef void (APIENTRY *PFNGLTEXCOORD3SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLTEXCOORD4DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLTEXCOORD4FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLTEXCOORD4IV) (const GLint *v);
typedef void (APIENTRY *PFNGLTEXCOORD4SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLRASTERPOS2D) (GLdouble x, GLdouble y);
typedef void (APIENTRY *PFNGLRASTERPOS2F) (GLfloat x, GLfloat y);
typedef void (APIENTRY *PFNGLRASTERPOS2I) (GLint x, GLint y);
typedef void (APIENTRY *PFNGLRASTERPOS2S) (GLshort x, GLshort y);
typedef void (APIENTRY *PFNGLRASTERPOS3D) (GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY *PFNGLRASTERPOS3F) (GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY *PFNGLRASTERPOS3I) (GLint x, GLint y, GLint z);
typedef void (APIENTRY *PFNGLRASTERPOS3S) (GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY *PFNGLRASTERPOS4D) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY *PFNGLRASTERPOS4F) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY *PFNGLRASTERPOS4I) (GLint x, GLint y, GLint z, GLint w);
typedef void (APIENTRY *PFNGLRASTERPOS4S) (GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY *PFNGLRASTERPOS2DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLRASTERPOS2FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLRASTERPOS2IV) (const GLint *v);
typedef void (APIENTRY *PFNGLRASTERPOS2SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLRASTERPOS3DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLRASTERPOS3FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLRASTERPOS3IV) (const GLint *v);
typedef void (APIENTRY *PFNGLRASTERPOS3SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLRASTERPOS4DV) (const GLdouble *v);
typedef void (APIENTRY *PFNGLRASTERPOS4FV) (const GLfloat *v);
typedef void (APIENTRY *PFNGLRASTERPOS4IV) (const GLint *v);
typedef void (APIENTRY *PFNGLRASTERPOS4SV) (const GLshort *v);
typedef void (APIENTRY *PFNGLRECTD) (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
typedef void (APIENTRY *PFNGLRECTF) (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
typedef void (APIENTRY *PFNGLRECTI) (GLint x1, GLint y1, GLint x2, GLint y2);
typedef void (APIENTRY *PFNGLRECTS) (GLshort x1, GLshort y1, GLshort x2, GLshort y2);
typedef void (APIENTRY *PFNGLRECTDV) (const GLdouble *v1, const GLdouble *v2);
typedef void (APIENTRY *PFNGLRECTFV) (const GLfloat *v1, const GLfloat *v2);
typedef void (APIENTRY *PFNGLRECTIV) (const GLint *v1, const GLint *v2);
typedef void (APIENTRY *PFNGLRECTSV) (const GLshort *v1, const GLshort *v2);

// Vertex Arrays
typedef void (APIENTRY *PFNGLVERTEXPOINTER) (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (APIENTRY *PFNGLNORMALPOINTER) (GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (APIENTRY *PFNGLCOLORPOINTER) (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (APIENTRY *PFNGLINDEXPOINTER) (GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (APIENTRY *PFNGLTEXCOORDPOINTER) (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (APIENTRY *PFNGLEDGEFLAGPOINTER) (GLsizei stride, const GLvoid *ptr);
typedef void (APIENTRY *PFNGLGETPOINTERV) (GLenum pname, void **params);
typedef void (APIENTRY *PFNGLARRAYELEMENT) (GLint i);
typedef void (APIENTRY *PFNGLDRAWARRAYS) (GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRY *PFNGLDRAWELEMENTS) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
typedef void (APIENTRY *PFNGLINTERLEAVEDARRAYS) (GLenum format, GLsizei stride, const GLvoid *pointer);

// Lighting
typedef void (APIENTRY *PFNGLSHADEMODEL) (GLenum mode);
typedef void (APIENTRY *PFNGLLIGHTF) (GLenum light, GLenum pname, GLfloat param);
typedef void (APIENTRY *PFNGLLIGHTI) (GLenum light, GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLLIGHTFV) (GLenum light, GLenum pname, const GLfloat *params);
typedef void (APIENTRY *PFNGLLIGHTIV) (GLenum light, GLenum pname, const GLint *params);
typedef void (APIENTRY *PFNGLGETLIGHTFV) (GLenum light, GLenum pname, GLfloat *params);
typedef void (APIENTRY *PFNGLGETLIGHTIV) (GLenum light, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLLIGHTMODELF) (GLenum pname, GLfloat param);
typedef void (APIENTRY *PFNGLLIGHTMODELI) (GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLLIGHTMODELFV) (GLenum pname, const GLfloat *params);
typedef void (APIENTRY *PFNGLLIGHTMODELIV) (GLenum pname, const GLint *params);
typedef void (APIENTRY *PFNGLMATERIALF) (GLenum face, GLenum pname, GLfloat param);
typedef void (APIENTRY *PFNGLMATERIALI) (GLenum face, GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLMATERIALFV) (GLenum face, GLenum pname, const GLfloat *params);
typedef void (APIENTRY *PFNGLMATERIALIV) (GLenum face, GLenum pname, const GLint *params);
typedef void (APIENTRY *PFNGLGETMATERIALFV) (GLenum face, GLenum pname, GLfloat *params);
typedef void (APIENTRY *PFNGLGETMATERIALIV) (GLenum face, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLCOLORMATERIAL) (GLenum face, GLenum mode);

// Raster functions
typedef void (APIENTRY *PFNGLPIXELZOOM) (GLfloat xfactor, GLfloat yfactor);
typedef void (APIENTRY *PFNGLPIXELSTOREF) (GLenum pname, GLfloat param);
typedef void (APIENTRY *PFNGLPIXELSTOREI) (GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLPIXELTRANSFERF) (GLenum pname, GLfloat param);
typedef void (APIENTRY *PFNGLPIXELTRANSFERI) (GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLPIXELMAPFV) (GLenum map, GLint mapsize, const GLfloat *values);
typedef void (APIENTRY *PFNGLPIXELMAPUIV) (GLenum map, GLint mapsize, const GLuint *values);
typedef void (APIENTRY *PFNGLPIXELMAPUSV) (GLenum map, GLint mapsize, const GLushort *values);
typedef void (APIENTRY *PFNGLGETPIXELMAPFV) (GLenum map, GLfloat *values);
typedef void (APIENTRY *PFNGLGETPIXELMAPUIV) (GLenum map, GLuint *values);
typedef void (APIENTRY *PFNGLGETPIXELMAPUSV) (GLenum map, GLushort *values);
typedef void (APIENTRY *PFNGLBITMAP) (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap );
typedef void (APIENTRY *PFNGLREADPIXELS) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
typedef void (APIENTRY *PFNGLDRAWPIXELS) (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY *PFNGLCOPYPIXELS) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);

// Stenciling
typedef void (APIENTRY *PFNGLSTENCILFUNC) (GLenum func, GLint ref, GLuint mask);
typedef void (APIENTRY *PFNGLSTENCILMASK) (GLuint mask);
typedef void (APIENTRY *PFNGLSTENCILOP) (GLenum fail, GLenum zfail, GLenum zpass);
typedef void (APIENTRY *PFNGLCLEARSTENCIL) (GLint s);

// Texture mapping
typedef void (APIENTRY *PFNGLTEXGEND) (GLenum coord, GLenum pname, GLdouble param);
typedef void (APIENTRY *PFNGLTEXGENF) (GLenum coord, GLenum pname, GLfloat param);
typedef void (APIENTRY *PFNGLTEXGENI) (GLenum coord, GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLTEXGENDV) (GLenum coord, GLenum pname, const GLdouble *params);
typedef void (APIENTRY *PFNGLTEXGENFV) (GLenum coord, GLenum pname, const GLfloat *params);
typedef void (APIENTRY *PFNGLTEXGENIV) (GLenum coord, GLenum pname, const GLint *params);
typedef void (APIENTRY *PFNGLGETTEXGENDV) (GLenum coord, GLenum pname, GLdouble *params);
typedef void (APIENTRY *PFNGLGETTEXGENFV) (GLenum coord, GLenum pname, GLfloat *params);
typedef void (APIENTRY *PFNGLGETTEXGENIV) (GLenum coord, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLTEXENVF) (GLenum target, GLenum pname, GLfloat param);
typedef void (APIENTRY *PFNGLTEXENVI) (GLenum target, GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLTEXENVFV) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (APIENTRY *PFNGLTEXENVIV) (GLenum target, GLenum pname, const GLint *params);
typedef void (APIENTRY *PFNGLGETTEXENVFV) (GLenum target, GLenum pname, GLfloat *params);
typedef void (APIENTRY *PFNGLGETTEXENVIV) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLTEXPARAMETERF) (GLenum target, GLenum pname, GLfloat param);
typedef void (APIENTRY *PFNGLTEXPARAMETERI) (GLenum target, GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLTEXPARAMETERFV) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (APIENTRY *PFNGLTEXPARAMETERIV) (GLenum target, GLenum pname, const GLint *params);
typedef void (APIENTRY *PFNGLGETTEXPARAMETERFV) (GLenum target, GLenum pname, GLfloat *params);
typedef void (APIENTRY *PFNGLGETTEXPARAMETERIV) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLGETTEXLEVELPARAMETERFV) (GLenum target, GLint level, GLenum pname, GLfloat *params);
typedef void (APIENTRY *PFNGLGETTEXLEVELPARAMETERIV) (GLenum target, GLint level, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLTEXIMAGE1D) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY *PFNGLTEXIMAGE2D) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY *PFNGLGETTEXIMAGE) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
typedef void (APIENTRY *PFNGLGENTEXTURES) (GLsizei n, GLuint *textures);
typedef void (APIENTRY *PFNGLDELETETEXTURES) (GLsizei n, const GLuint *textures);
typedef void (APIENTRY *PFNGLBINDTEXTURE) (GLenum target, GLuint texture);
typedef void (APIENTRY *PFNGLPRIORITIZETEXTURES) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
typedef GLboolean (APIENTRY *PFNGLARETEXTURESRESIDENT) (GLsizei n, const GLuint *textures, GLboolean *residences);
typedef GLboolean (APIENTRY *PFNGLISTEXTURE) (GLuint texture);
typedef void (APIENTRY *PFNGLTEXSUBIMAGE1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY *PFNGLTEXSUBIMAGE2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY *PFNGLCOPYTEXIMAGE1D) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
typedef void (APIENTRY *PFNGLCOPYTEXIMAGE2D) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
typedef void (APIENTRY *PFNGLCOPYTEXSUBIMAGE1D) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
typedef void (APIENTRY *PFNGLCOPYTEXSUBIMAGE2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);

// Evaluators
typedef void (APIENTRY *PFNGLMAP1D) (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
typedef void (APIENTRY *PFNGLMAP1F) (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
typedef void (APIENTRY *PFNGLMAP2D) (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
typedef void (APIENTRY *PFNGLMAP2F) (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
typedef void (APIENTRY *PFNGLGETMAPDV) (GLenum target, GLenum query, GLdouble *v);
typedef void (APIENTRY *PFNGLGETMAPFV) (GLenum target, GLenum query, GLfloat *v);
typedef void (APIENTRY *PFNGLGETMAPIV) (GLenum target, GLenum query, GLint *v);
typedef void (APIENTRY *PFNGLEVALCOORD1D) (GLdouble u);
typedef void (APIENTRY *PFNGLEVALCOORD1F) (GLfloat u);
typedef void (APIENTRY *PFNGLEVALCOORD1DV) (const GLdouble *u);
typedef void (APIENTRY *PFNGLEVALCOORD1FV) (const GLfloat *u);
typedef void (APIENTRY *PFNGLEVALCOORD2D) (GLdouble u, GLdouble v);
typedef void (APIENTRY *PFNGLEVALCOORD2F) (GLfloat u, GLfloat v);
typedef void (APIENTRY *PFNGLEVALCOORD2DV) (const GLdouble *u);
typedef void (APIENTRY *PFNGLEVALCOORD2FV) (const GLfloat *u);
typedef void (APIENTRY *PFNGLMAPGRID1D) (GLint un, GLdouble u1, GLdouble u2);
typedef void (APIENTRY *PFNGLMAPGRID1F) (GLint un, GLfloat u1, GLfloat u2);
typedef void (APIENTRY *PFNGLMAPGRID2D) (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
typedef void (APIENTRY *PFNGLMAPGRID2F) (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
typedef void (APIENTRY *PFNGLEVALPOINT1) (GLint i);
typedef void (APIENTRY *PFNGLEVALPOINT2) (GLint i, GLint j);
typedef void (APIENTRY *PFNGLEVALMESH1) (GLenum mode, GLint i1, GLint i2);
typedef void (APIENTRY *PFNGLEVALMESH2) (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);

// Fog
typedef void (APIENTRY *PFNGLFOGF) (GLenum pname, GLfloat param);
typedef void (APIENTRY *PFNGLFOGI) (GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLFOGFV) (GLenum pname, const GLfloat *params);
typedef void (APIENTRY *PFNGLFOGIV) (GLenum pname, const GLint *params);

// Selection and Feedback
typedef void (APIENTRY *PFNGLFEEDBACKBUFFER) (GLsizei size, GLenum type, GLfloat *buffer);
typedef void (APIENTRY *PFNGLPASSTHROUGH) (GLfloat token);
typedef void (APIENTRY *PFNGLSELECTBUFFER) (GLsizei size, GLuint *buffer);
typedef void (APIENTRY *PFNGLINITNAMES) (void);
typedef void (APIENTRY *PFNGLLOADNAME) (GLuint name);
typedef void (APIENTRY *PFNGLPUSHNAME) (GLuint name);
typedef void (APIENTRY *PFNGLPOPNAME) (void);

#endif // LC_OPENGL_DYNAMIC

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
#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object 1
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
#endif

// =============================================================================
// OpenGL extern declarations

#ifdef LC_OPENGL_DYNAMIC

extern PFNGLCLEARINDEX pfnglClearIndex;
extern PFNGLCLEARCOLOR pfnglClearColor;
extern PFNGLCLEAR pfnglClear;
extern PFNGLINDEXMASK pfnglIndexMask;
extern PFNGLCOLORMASK pfnglColorMask;
extern PFNGLALPHAFUNC pfnglAlphaFunc;
extern PFNGLBLENDFUNC pfnglBlendFunc;
extern PFNGLLOGICOP pfnglLogicOp;
extern PFNGLCULLFACE pfnglCullFace;
extern PFNGLFRONTFACE pfnglFrontFace;
extern PFNGLPOINTSIZE pfnglPointSize;
extern PFNGLLINEWIDTH pfnglLineWidth;
extern PFNGLLINESTIPPLE pfnglLineStipple;
extern PFNGLPOLYGONMODE pfnglPolygonMode;
extern PFNGLPOLYGONOFFSET pfnglPolygonOffset;
extern PFNGLPOLYGONSTIPPLE pfnglPolygonStipple;
extern PFNGLGETPOLYGONSTIPPLE pfnglGetPolygonStipple;
extern PFNGLEDGEFLAG pfnglEdgeFlag;
extern PFNGLEDGEFLAGV pfnglEdgeFlagv;
extern PFNGLSCISSOR pfnglScissor;
extern PFNGLCLIPPLANE pfnglClipPlane;
extern PFNGLGETCLIPPLANE pfnglGetClipPlane;
extern PFNGLDRAWBUFFER pfnglDrawBuffer;
extern PFNGLREADBUFFER pfnglReadBuffer;
extern PFNGLENABLE pfnglEnable;
extern PFNGLDISABLE pfnglDisable;
extern PFNGLISENABLED pfnglIsEnabled;
extern PFNGLENABLECLIENTSTATE pfnglEnableClientState;
extern PFNGLDISABLECLIENTSTATE pfnglDisableClientState;
extern PFNGLGETBOOLEANV pfnglGetBooleanv;
extern PFNGLGETDOUBLEV pfnglGetDoublev;
extern PFNGLGETFLOATV pfnglGetFloatv;
extern PFNGLGETINTEGERV pfnglGetIntegerv;
extern PFNGLPUSHATTRIB pfnglPushAttrib;
extern PFNGLPOPATTRIB pfnglPopAttrib;
extern PFNGLPUSHCLIENTATTRIB pfnglPushClientAttrib;
extern PFNGLPOPCLIENTATTRIB pfnglPopClientAttrib;
extern PFNGLRENDERMODE pfnglRenderMode;
extern PFNGLGETERROR pfnglGetError;
extern PFNGLGETSTRING pfnglGetString;
extern PFNGLFINISH pfnglFinish;
extern PFNGLFLUSH pfnglFlush;
extern PFNGLHINT pfnglHint;
extern PFNGLCLEARDEPTH pfnglClearDepth;
extern PFNGLDEPTHFUNC pfnglDepthFunc;
extern PFNGLDEPTHMASK pfnglDepthMask;
extern PFNGLDEPTHRANGE pfnglDepthRange;
extern PFNGLCLEARACCUM pfnglClearAccum;
extern PFNGLACCUM pfnglAccum;
extern PFNGLMATRIXMODE pfnglMatrixMode;
extern PFNGLORTHO pfnglOrtho;
extern PFNGLFRUSTUM pfnglFrustum;
extern PFNGLVIEWPORT pfnglViewport;
extern PFNGLPUSHMATRIX pfnglPushMatrix;
extern PFNGLPOPMATRIX pfnglPopMatrix;
extern PFNGLLOADIDENTITY pfnglLoadIdentity;
extern PFNGLLOADMATRIXD pfnglLoadMatrixd;
extern PFNGLLOADMATRIXF pfnglLoadMatrixf;
extern PFNGLMULTMATRIXD pfnglMultMatrixd;
extern PFNGLMULTMATRIXF pfnglMultMatrixf;
extern PFNGLROTATED pfnglRotated;
extern PFNGLROTATEF pfnglRotatef;
extern PFNGLSCALED pfnglScaled;
extern PFNGLSCALEF pfnglScalef;
extern PFNGLTRANSLATED pfnglTranslated;
extern PFNGLTRANSLATEF pfnglTranslatef;
extern PFNGLISLIST pfnglIsList;
extern PFNGLDELETELISTS pfnglDeleteLists;
extern PFNGLGENLISTS pfnglGenLists;
extern PFNGLNEWLIST pfnglNewList;
extern PFNGLENDLIST pfnglEndList;
extern PFNGLCALLLIST pfnglCallList;
extern PFNGLCALLLISTS pfnglCallLists;
extern PFNGLLISTBASE pfnglListBase;
extern PFNGLBEGIN pfnglBegin;
extern PFNGLEND pfnglEnd;
extern PFNGLVERTEX2D pfnglVertex2d;
extern PFNGLVERTEX2F pfnglVertex2f;
extern PFNGLVERTEX2I pfnglVertex2i;
extern PFNGLVERTEX2S pfnglVertex2s;
extern PFNGLVERTEX3D pfnglVertex3d;
extern PFNGLVERTEX3F pfnglVertex3f;
extern PFNGLVERTEX3I pfnglVertex3i;
extern PFNGLVERTEX3S pfnglVertex3s;
extern PFNGLVERTEX4D pfnglVertex4d;
extern PFNGLVERTEX4F pfnglVertex4f;
extern PFNGLVERTEX4I pfnglVertex4i;
extern PFNGLVERTEX4S pfnglVertex4s;
extern PFNGLVERTEX2DV pfnglVertex2dv;
extern PFNGLVERTEX2FV pfnglVertex2fv;
extern PFNGLVERTEX2IV pfnglVertex2iv;
extern PFNGLVERTEX2SV pfnglVertex2sv;
extern PFNGLVERTEX3DV pfnglVertex3dv;
extern PFNGLVERTEX3FV pfnglVertex3fv;
extern PFNGLVERTEX3IV pfnglVertex3iv;
extern PFNGLVERTEX3SV pfnglVertex3sv;
extern PFNGLVERTEX4DV pfnglVertex4dv;
extern PFNGLVERTEX4FV pfnglVertex4fv;
extern PFNGLVERTEX4IV pfnglVertex4iv;
extern PFNGLVERTEX4SV pfnglVertex4sv;
extern PFNGLNORMAL3B pfnglNormal3b;
extern PFNGLNORMAL3D pfnglNormal3d;
extern PFNGLNORMAL3F pfnglNormal3f;
extern PFNGLNORMAL3I pfnglNormal3i;
extern PFNGLNORMAL3S pfnglNormal3s;
extern PFNGLNORMAL3BV pfnglNormal3bv;
extern PFNGLNORMAL3DV pfnglNormal3dv;
extern PFNGLNORMAL3FV pfnglNormal3fv;
extern PFNGLNORMAL3IV pfnglNormal3iv;
extern PFNGLNORMAL3SV pfnglNormal3sv;
extern PFNGLINDEXD pfnglIndexd;
extern PFNGLINDEXF pfnglIndexf;
extern PFNGLINDEXI pfnglIndexi;
extern PFNGLINDEXS pfnglIndexs;
extern PFNGLINDEXUB pfnglIndexub;
extern PFNGLINDEXDV pfnglIndexdv;
extern PFNGLINDEXFV pfnglIndexfv;
extern PFNGLINDEXIV pfnglIndexiv;
extern PFNGLINDEXSV pfnglIndexsv;
extern PFNGLINDEXUBV pfnglIndexubv;
extern PFNGLCOLOR3B pfnglColor3b;
extern PFNGLCOLOR3D pfnglColor3d;
extern PFNGLCOLOR3F pfnglColor3f;
extern PFNGLCOLOR3I pfnglColor3i;
extern PFNGLCOLOR3S pfnglColor3s;
extern PFNGLCOLOR3UB pfnglColor3ub;
extern PFNGLCOLOR3UI pfnglColor3ui;
extern PFNGLCOLOR3US pfnglColor3us;
extern PFNGLCOLOR4B pfnglColor4b;
extern PFNGLCOLOR4D pfnglColor4d;
extern PFNGLCOLOR4F pfnglColor4f;
extern PFNGLCOLOR4I pfnglColor4i;
extern PFNGLCOLOR4S pfnglColor4s;
extern PFNGLCOLOR4UB pfnglColor4ub;
extern PFNGLCOLOR4UI pfnglColor4ui;
extern PFNGLCOLOR4US pfnglColor4us;
extern PFNGLCOLOR3BV pfnglColor3bv;
extern PFNGLCOLOR3DV pfnglColor3dv;
extern PFNGLCOLOR3FV pfnglColor3fv;
extern PFNGLCOLOR3IV pfnglColor3iv;
extern PFNGLCOLOR3SV pfnglColor3sv;
extern PFNGLCOLOR3UBV pfnglColor3ubv;
extern PFNGLCOLOR3UIV pfnglColor3uiv;
extern PFNGLCOLOR3USV pfnglColor3usv;
extern PFNGLCOLOR4BV pfnglColor4bv;
extern PFNGLCOLOR4DV pfnglColor4dv;
extern PFNGLCOLOR4FV pfnglColor4fv;
extern PFNGLCOLOR4IV pfnglColor4iv;
extern PFNGLCOLOR4SV pfnglColor4sv;
extern PFNGLCOLOR4UBV pfnglColor4ubv;
extern PFNGLCOLOR4UIV pfnglColor4uiv;
extern PFNGLCOLOR4USV pfnglColor4usv;
extern PFNGLTEXCOORD1D pfnglTexCoord1d;
extern PFNGLTEXCOORD1F pfnglTexCoord1f;
extern PFNGLTEXCOORD1I pfnglTexCoord1i;
extern PFNGLTEXCOORD1S pfnglTexCoord1s;
extern PFNGLTEXCOORD2D pfnglTexCoord2d;
extern PFNGLTEXCOORD2F pfnglTexCoord2f;
extern PFNGLTEXCOORD2I pfnglTexCoord2i;
extern PFNGLTEXCOORD2S pfnglTexCoord2s;
extern PFNGLTEXCOORD3D pfnglTexCoord3d;
extern PFNGLTEXCOORD3F pfnglTexCoord3f;
extern PFNGLTEXCOORD3I pfnglTexCoord3i;
extern PFNGLTEXCOORD3S pfnglTexCoord3s;
extern PFNGLTEXCOORD4D pfnglTexCoord4d;
extern PFNGLTEXCOORD4F pfnglTexCoord4f;
extern PFNGLTEXCOORD4I pfnglTexCoord4i;
extern PFNGLTEXCOORD4S pfnglTexCoord4s;
extern PFNGLTEXCOORD1DV pfnglTexCoord1dv;
extern PFNGLTEXCOORD1FV pfnglTexCoord1fv;
extern PFNGLTEXCOORD1IV pfnglTexCoord1iv;
extern PFNGLTEXCOORD1SV pfnglTexCoord1sv;
extern PFNGLTEXCOORD2DV pfnglTexCoord2dv;
extern PFNGLTEXCOORD2FV pfnglTexCoord2fv;
extern PFNGLTEXCOORD2IV pfnglTexCoord2iv;
extern PFNGLTEXCOORD2SV pfnglTexCoord2sv;
extern PFNGLTEXCOORD3DV pfnglTexCoord3dv;
extern PFNGLTEXCOORD3FV pfnglTexCoord3fv;
extern PFNGLTEXCOORD3IV pfnglTexCoord3iv;
extern PFNGLTEXCOORD3SV pfnglTexCoord3sv;
extern PFNGLTEXCOORD4DV pfnglTexCoord4dv;
extern PFNGLTEXCOORD4FV pfnglTexCoord4fv;
extern PFNGLTEXCOORD4IV pfnglTexCoord4iv;
extern PFNGLTEXCOORD4SV pfnglTexCoord4sv;
extern PFNGLRASTERPOS2D pfnglRasterPos2d;
extern PFNGLRASTERPOS2F pfnglRasterPos2f;
extern PFNGLRASTERPOS2I pfnglRasterPos2i;
extern PFNGLRASTERPOS2S pfnglRasterPos2s;
extern PFNGLRASTERPOS3D pfnglRasterPos3d;
extern PFNGLRASTERPOS3F pfnglRasterPos3f;
extern PFNGLRASTERPOS3I pfnglRasterPos3i;
extern PFNGLRASTERPOS3S pfnglRasterPos3s;
extern PFNGLRASTERPOS4D pfnglRasterPos4d;
extern PFNGLRASTERPOS4F pfnglRasterPos4f;
extern PFNGLRASTERPOS4I pfnglRasterPos4i;
extern PFNGLRASTERPOS4S pfnglRasterPos4s;
extern PFNGLRASTERPOS2DV pfnglRasterPos2dv;
extern PFNGLRASTERPOS2FV pfnglRasterPos2fv;
extern PFNGLRASTERPOS2IV pfnglRasterPos2iv;
extern PFNGLRASTERPOS2SV pfnglRasterPos2sv;
extern PFNGLRASTERPOS3DV pfnglRasterPos3dv;
extern PFNGLRASTERPOS3FV pfnglRasterPos3fv;
extern PFNGLRASTERPOS3IV pfnglRasterPos3iv;
extern PFNGLRASTERPOS3SV pfnglRasterPos3sv;
extern PFNGLRASTERPOS4DV pfnglRasterPos4dv;
extern PFNGLRASTERPOS4FV pfnglRasterPos4fv;
extern PFNGLRASTERPOS4IV pfnglRasterPos4iv;
extern PFNGLRASTERPOS4SV pfnglRasterPos4sv;
extern PFNGLRECTD pfnglRectd;
extern PFNGLRECTF pfnglRectf;
extern PFNGLRECTI pfnglRecti;
extern PFNGLRECTS pfnglRects;
extern PFNGLRECTDV pfnglRectdv;
extern PFNGLRECTFV pfnglRectfv;
extern PFNGLRECTIV pfnglRectiv;
extern PFNGLRECTSV pfnglRectsv;
extern PFNGLVERTEXPOINTER pfnglVertexPointer;
extern PFNGLNORMALPOINTER pfnglNormalPointer;
extern PFNGLCOLORPOINTER pfnglColorPointer;
extern PFNGLINDEXPOINTER pfnglIndexPointer;
extern PFNGLTEXCOORDPOINTER pfnglTexCoordPointer;
extern PFNGLEDGEFLAGPOINTER pfnglEdgeFlagPointer;
extern PFNGLGETPOINTERV pfnglGetPointerv;
extern PFNGLARRAYELEMENT pfnglArrayElement;
extern PFNGLDRAWARRAYS pfnglDrawArrays;
extern PFNGLDRAWELEMENTS pfnglDrawElements;
extern PFNGLINTERLEAVEDARRAYS pfnglInterleavedArrays;
extern PFNGLSHADEMODEL pfnglShadeModel;
extern PFNGLLIGHTF pfnglLightf;
extern PFNGLLIGHTI pfnglLighti;
extern PFNGLLIGHTFV pfnglLightfv;
extern PFNGLLIGHTIV pfnglLightiv;
extern PFNGLGETLIGHTFV pfnglGetLightfv;
extern PFNGLGETLIGHTIV pfnglGetLightiv;
extern PFNGLLIGHTMODELF pfnglLightModelf;
extern PFNGLLIGHTMODELI pfnglLightModeli;
extern PFNGLLIGHTMODELFV pfnglLightModelfv;
extern PFNGLLIGHTMODELIV pfnglLightModeliv;
extern PFNGLMATERIALF pfnglMaterialf;
extern PFNGLMATERIALI pfnglMateriali;
extern PFNGLMATERIALFV pfnglMaterialfv;
extern PFNGLMATERIALIV pfnglMaterialiv;
extern PFNGLGETMATERIALFV pfnglGetMaterialfv;
extern PFNGLGETMATERIALIV pfnglGetMaterialiv;
extern PFNGLCOLORMATERIAL pfnglColorMaterial;
extern PFNGLPIXELZOOM pfnglPixelZoom;
extern PFNGLPIXELSTOREF pfnglPixelStoref;
extern PFNGLPIXELSTOREI pfnglPixelStorei;
extern PFNGLPIXELTRANSFERF pfnglPixelTransferf;
extern PFNGLPIXELTRANSFERI pfnglPixelTransferi;
extern PFNGLPIXELMAPFV pfnglPixelMapfv;
extern PFNGLPIXELMAPUIV pfnglPixelMapuiv;
extern PFNGLPIXELMAPUSV pfnglPixelMapusv;
extern PFNGLGETPIXELMAPFV pfnglGetPixelMapfv;
extern PFNGLGETPIXELMAPUIV pfnglGetPixelMapuiv;
extern PFNGLGETPIXELMAPUSV pfnglGetPixelMapusv;
extern PFNGLBITMAP pfnglBitmap;
extern PFNGLREADPIXELS pfnglReadPixels;
extern PFNGLDRAWPIXELS pfnglDrawPixels;
extern PFNGLCOPYPIXELS pfnglCopyPixels;
extern PFNGLSTENCILFUNC pfnglStencilFunc;
extern PFNGLSTENCILMASK pfnglStencilMask;
extern PFNGLSTENCILOP pfnglStencilOp;
extern PFNGLCLEARSTENCIL pfnglClearStencil;
extern PFNGLTEXGEND pfnglTexGend;
extern PFNGLTEXGENF pfnglTexGenf;
extern PFNGLTEXGENI pfnglTexGeni;
extern PFNGLTEXGENDV pfnglTexGendv;
extern PFNGLTEXGENFV pfnglTexGenfv;
extern PFNGLTEXGENIV pfnglTexGeniv;
extern PFNGLGETTEXGENDV pfnglGetTexGendv;
extern PFNGLGETTEXGENFV pfnglGetTexGenfv;
extern PFNGLGETTEXGENIV pfnglGetTexGeniv;
extern PFNGLTEXENVF pfnglTexEnvf;
extern PFNGLTEXENVI pfnglTexEnvi;
extern PFNGLTEXENVFV pfnglTexEnvfv;
extern PFNGLTEXENVIV pfnglTexEnviv;
extern PFNGLGETTEXENVFV pfnglGetTexEnvfv;
extern PFNGLGETTEXENVIV pfnglGetTexEnviv;
extern PFNGLTEXPARAMETERF pfnglTexParameterf;
extern PFNGLTEXPARAMETERI pfnglTexParameteri;
extern PFNGLTEXPARAMETERFV pfnglTexParameterfv;
extern PFNGLTEXPARAMETERIV pfnglTexParameteriv;
extern PFNGLGETTEXPARAMETERFV pfnglGetTexParameterfv;
extern PFNGLGETTEXPARAMETERIV pfnglGetTexParameteriv;
extern PFNGLGETTEXLEVELPARAMETERFV pfnglGetTexLevelParameterfv;
extern PFNGLGETTEXLEVELPARAMETERIV pfnglGetTexLevelParameteriv;
extern PFNGLTEXIMAGE1D pfnglTexImage1D;
extern PFNGLTEXIMAGE2D pfnglTexImage2D;
extern PFNGLGETTEXIMAGE pfnglGetTexImage;
extern PFNGLGENTEXTURES pfnglGenTextures;
extern PFNGLDELETETEXTURES pfnglDeleteTextures;
extern PFNGLBINDTEXTURE pfnglBindTexture;
extern PFNGLPRIORITIZETEXTURES pfnglPrioritizeTextures;
extern PFNGLARETEXTURESRESIDENT pfnglAreTexturesResident;
extern PFNGLISTEXTURE pfnglIsTexture;
extern PFNGLTEXSUBIMAGE1D pfnglTexSubImage1D;
extern PFNGLTEXSUBIMAGE2D pfnglTexSubImage2D;
extern PFNGLCOPYTEXIMAGE1D pfnglCopyTexImage1D;
extern PFNGLCOPYTEXIMAGE2D pfnglCopyTexImage2D;
extern PFNGLCOPYTEXSUBIMAGE1D pfnglCopyTexSubImage1D;
extern PFNGLCOPYTEXSUBIMAGE2D pfnglCopyTexSubImage2D;
extern PFNGLMAP1D pfnglMap1d;
extern PFNGLMAP1F pfnglMap1f;
extern PFNGLMAP2D pfnglMap2d;
extern PFNGLMAP2F pfnglMap2f;
extern PFNGLGETMAPDV pfnglGetMapdv;
extern PFNGLGETMAPFV pfnglGetMapfv;
extern PFNGLGETMAPIV pfnglGetMapiv;
extern PFNGLEVALCOORD1D pfnglEvalCoord1d;
extern PFNGLEVALCOORD1F pfnglEvalCoord1f;
extern PFNGLEVALCOORD1DV pfnglEvalCoord1dv;
extern PFNGLEVALCOORD1FV pfnglEvalCoord1fv;
extern PFNGLEVALCOORD2D pfnglEvalCoord2d;
extern PFNGLEVALCOORD2F pfnglEvalCoord2f;
extern PFNGLEVALCOORD2DV pfnglEvalCoord2dv;
extern PFNGLEVALCOORD2FV pfnglEvalCoord2fv;
extern PFNGLMAPGRID1D pfnglMapGrid1d;
extern PFNGLMAPGRID1F pfnglMapGrid1f;
extern PFNGLMAPGRID2D pfnglMapGrid2d;
extern PFNGLMAPGRID2F pfnglMapGrid2f;
extern PFNGLEVALPOINT1 pfnglEvalPoint1;
extern PFNGLEVALPOINT2 pfnglEvalPoint2;
extern PFNGLEVALMESH1 pfnglEvalMesh1;
extern PFNGLEVALMESH2 pfnglEvalMesh2;
extern PFNGLFOGF pfnglFogf;
extern PFNGLFOGI pfnglFogi;
extern PFNGLFOGFV pfnglFogfv;
extern PFNGLFOGIV pfnglFogiv;
extern PFNGLFEEDBACKBUFFER pfnglFeedbackBuffer;
extern PFNGLPASSTHROUGH pfnglPassThrough;
extern PFNGLSELECTBUFFER pfnglSelectBuffer;
extern PFNGLINITNAMES pfnglInitNames;
extern PFNGLLOADNAME pfnglLoadName;
extern PFNGLPUSHNAME pfnglPushName;
extern PFNGLPOPNAME pfnglPopName;

#endif // LC_OPENGL_DYNAMIC

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

// =============================================================================
// Replace OpenGL function names with the dynamic functions

#ifdef LC_OPENGL_DYNAMIC

#define glClearIndex pfnglClearIndex
#define glClearColor pfnglClearColor
#define glClear pfnglClear
#define glIndexMask pfnglIndexMask
#define glColorMask pfnglColorMask
#define glAlphaFunc pfnglAlphaFunc
#define glBlendFunc pfnglBlendFunc
#define glLogicOp pfnglLogicOp
#define glCullFace pfnglCullFace
#define glFrontFace pfnglFrontFace
#define glPointSize pfnglPointSize
#define glLineWidth pfnglLineWidth
#define glLineStipple pfnglLineStipple
#define glPolygonMode pfnglPolygonMode
#define glPolygonOffset pfnglPolygonOffset
#define glPolygonStipple pfnglPolygonStipple
#define glGetPolygonStipple pfnglGetPolygonStipple
#define glEdgeFlag pfnglEdgeFlag
#define glEdgeFlagv pfnglEdgeFlagv
#define glScissor pfnglScissor
#define glClipPlane pfnglClipPlane
#define glGetClipPlane pfnglGetClipPlane
#define glDrawBuffer pfnglDrawBuffer
#define glReadBuffer pfnglReadBuffer
#define glEnable pfnglEnable
#define glDisable pfnglDisable
#define glIsEnabled pfnglIsEnabled
#define glEnableClientState pfnglEnableClientState
#define glDisableClientState pfnglDisableClientState
#define glGetBooleanv pfnglGetBooleanv
#define glGetDoublev pfnglGetDoublev
#define glGetFloatv pfnglGetFloatv
#define glGetIntegerv pfnglGetIntegerv
#define glPushAttrib pfnglPushAttrib
#define glPopAttrib pfnglPopAttrib
#define glPushClientAttrib pfnglPushClientAttrib
#define glPopClientAttrib pfnglPopClientAttrib
#define glRenderMode pfnglRenderMode
#define glGetError pfnglGetError
#define glGetString pfnglGetString
#define glFinish pfnglFinish
#define glFlush pfnglFlush
#define glHint pfnglHint
#define glClearDepth pfnglClearDepth
#define glDepthFunc pfnglDepthFunc
#define glDepthMask pfnglDepthMask
#define glDepthRange pfnglDepthRange
#define glClearAccum pfnglClearAccum
#define glAccum pfnglAccum
#define glMatrixMode pfnglMatrixMode
#define glOrtho pfnglOrtho
#define glFrustum pfnglFrustum
#define glViewport pfnglViewport
#define glPushMatrix pfnglPushMatrix
#define glPopMatrix pfnglPopMatrix
#define glLoadIdentity pfnglLoadIdentity
#define glLoadMatrixd pfnglLoadMatrixd
#define glLoadMatrixf pfnglLoadMatrixf
#define glMultMatrixd pfnglMultMatrixd
#define glMultMatrixf pfnglMultMatrixf
#define glRotated pfnglRotated
#define glRotatef pfnglRotatef
#define glScaled pfnglScaled
#define glScalef pfnglScalef
#define glTranslated pfnglTranslated
#define glTranslatef pfnglTranslatef
#define glIsList pfnglIsList
#define glDeleteLists pfnglDeleteLists
#define glGenLists pfnglGenLists
#define glNewList pfnglNewList
#define glEndList pfnglEndList
#define glCallList pfnglCallList
#define glCallLists pfnglCallLists
#define glListBase pfnglListBase
#define glBegin pfnglBegin
#define glEnd pfnglEnd
#define glVertex2d pfnglVertex2d
#define glVertex2f pfnglVertex2f
#define glVertex2i pfnglVertex2i
#define glVertex2s pfnglVertex2s
#define glVertex3d pfnglVertex3d
#define glVertex3f pfnglVertex3f
#define glVertex3i pfnglVertex3i
#define glVertex3s pfnglVertex3s
#define glVertex4d pfnglVertex4d
#define glVertex4f pfnglVertex4f
#define glVertex4i pfnglVertex4i
#define glVertex4s pfnglVertex4s
#define glVertex2dv pfnglVertex2dv
#define glVertex2fv pfnglVertex2fv
#define glVertex2iv pfnglVertex2iv
#define glVertex2sv pfnglVertex2sv
#define glVertex3dv pfnglVertex3dv
#define glVertex3fv pfnglVertex3fv
#define glVertex3iv pfnglVertex3iv
#define glVertex3sv pfnglVertex3sv
#define glVertex4dv pfnglVertex4dv
#define glVertex4fv pfnglVertex4fv
#define glVertex4iv pfnglVertex4iv
#define glVertex4sv pfnglVertex4sv
#define glNormal3b pfnglNormal3b
#define glNormal3d pfnglNormal3d
#define glNormal3f pfnglNormal3f
#define glNormal3i pfnglNormal3i
#define glNormal3s pfnglNormal3s
#define glNormal3bv pfnglNormal3bv
#define glNormal3dv pfnglNormal3dv
#define glNormal3fv pfnglNormal3fv
#define glNormal3iv pfnglNormal3iv
#define glNormal3sv pfnglNormal3sv
#define glIndexd pfnglIndexd
#define glIndexf pfnglIndexf
#define glIndexi pfnglIndexi
#define glIndexs pfnglIndexs
#define glIndexub pfnglIndexub
#define glIndexdv pfnglIndexdv
#define glIndexfv pfnglIndexfv
#define glIndexiv pfnglIndexiv
#define glIndexsv pfnglIndexsv
#define glIndexubv pfnglIndexubv
#define glColor3b pfnglColor3b
#define glColor3d pfnglColor3d
#define glColor3f pfnglColor3f
#define glColor3i pfnglColor3i
#define glColor3s pfnglColor3s
#define glColor3ub pfnglColor3ub
#define glColor3ui pfnglColor3ui
#define glColor3us pfnglColor3us
#define glColor4b pfnglColor4b
#define glColor4d pfnglColor4d
#define glColor4f pfnglColor4f
#define glColor4i pfnglColor4i
#define glColor4s pfnglColor4s
#define glColor4ub pfnglColor4ub
#define glColor4ui pfnglColor4ui
#define glColor4us pfnglColor4us
#define glColor3bv pfnglColor3bv
#define glColor3dv pfnglColor3dv
#define glColor3fv pfnglColor3fv
#define glColor3iv pfnglColor3iv
#define glColor3sv pfnglColor3sv
#define glColor3ubv pfnglColor3ubv
#define glColor3uiv pfnglColor3uiv
#define glColor3usv pfnglColor3usv
#define glColor4bv pfnglColor4bv
#define glColor4dv pfnglColor4dv
#define glColor4fv pfnglColor4fv
#define glColor4iv pfnglColor4iv
#define glColor4sv pfnglColor4sv
#define glColor4ubv pfnglColor4ubv
#define glColor4uiv pfnglColor4uiv
#define glColor4usv pfnglColor4usv
#define glTexCoord1d pfnglTexCoord1d
#define glTexCoord1f pfnglTexCoord1f
#define glTexCoord1i pfnglTexCoord1i
#define glTexCoord1s pfnglTexCoord1s
#define glTexCoord2d pfnglTexCoord2d
#define glTexCoord2f pfnglTexCoord2f
#define glTexCoord2i pfnglTexCoord2i
#define glTexCoord2s pfnglTexCoord2s
#define glTexCoord3d pfnglTexCoord3d
#define glTexCoord3f pfnglTexCoord3f
#define glTexCoord3i pfnglTexCoord3i
#define glTexCoord3s pfnglTexCoord3s
#define glTexCoord4d pfnglTexCoord4d
#define glTexCoord4f pfnglTexCoord4f
#define glTexCoord4i pfnglTexCoord4i
#define glTexCoord4s pfnglTexCoord4s
#define glTexCoord1dv pfnglTexCoord1dv
#define glTexCoord1fv pfnglTexCoord1fv
#define glTexCoord1iv pfnglTexCoord1iv
#define glTexCoord1sv pfnglTexCoord1sv
#define glTexCoord2dv pfnglTexCoord2dv
#define glTexCoord2fv pfnglTexCoord2fv
#define glTexCoord2iv pfnglTexCoord2iv
#define glTexCoord2sv pfnglTexCoord2sv
#define glTexCoord3dv pfnglTexCoord3dv
#define glTexCoord3fv pfnglTexCoord3fv
#define glTexCoord3iv pfnglTexCoord3iv
#define glTexCoord3sv pfnglTexCoord3sv
#define glTexCoord4dv pfnglTexCoord4dv
#define glTexCoord4fv pfnglTexCoord4fv
#define glTexCoord4iv pfnglTexCoord4iv
#define glTexCoord4sv pfnglTexCoord4sv
#define glRasterPos2d pfnglRasterPos2d
#define glRasterPos2f pfnglRasterPos2f
#define glRasterPos2i pfnglRasterPos2i
#define glRasterPos2s pfnglRasterPos2s
#define glRasterPos3d pfnglRasterPos3d
#define glRasterPos3f pfnglRasterPos3f
#define glRasterPos3i pfnglRasterPos3i
#define glRasterPos3s pfnglRasterPos3s
#define glRasterPos4d pfnglRasterPos4d
#define glRasterPos4f pfnglRasterPos4f
#define glRasterPos4i pfnglRasterPos4i
#define glRasterPos4s pfnglRasterPos4s
#define glRasterPos2dv pfnglRasterPos2dv
#define glRasterPos2fv pfnglRasterPos2fv
#define glRasterPos2iv pfnglRasterPos2iv
#define glRasterPos2sv pfnglRasterPos2sv
#define glRasterPos3dv pfnglRasterPos3dv
#define glRasterPos3fv pfnglRasterPos3fv
#define glRasterPos3iv pfnglRasterPos3iv
#define glRasterPos3sv pfnglRasterPos3sv
#define glRasterPos4dv pfnglRasterPos4dv
#define glRasterPos4fv pfnglRasterPos4fv
#define glRasterPos4iv pfnglRasterPos4iv
#define glRasterPos4sv pfnglRasterPos4sv
#define glRectd pfnglRectd
#define glRectf pfnglRectf
#define glRecti pfnglRecti
#define glRects pfnglRects
#define glRectdv pfnglRectdv
#define glRectfv pfnglRectfv
#define glRectiv pfnglRectiv
#define glRectsv pfnglRectsv
#define glVertexPointer pfnglVertexPointer
#define glNormalPointer pfnglNormalPointer
#define glColorPointer pfnglColorPointer
#define glIndexPointer pfnglIndexPointer
#define glTexCoordPointer pfnglTexCoordPointer
#define glEdgeFlagPointer pfnglEdgeFlagPointer
#define glGetPointerv pfnglGetPointerv
#define glArrayElement pfnglArrayElement
#define glDrawArrays pfnglDrawArrays
#define glDrawElements pfnglDrawElements
#define glInterleavedArrays pfnglInterleavedArrays
#define glShadeModel pfnglShadeModel
#define glLightf pfnglLightf
#define glLighti pfnglLighti
#define glLightfv pfnglLightfv
#define glLightiv pfnglLightiv
#define glGetLightfv pfnglGetLightfv
#define glGetLightiv pfnglGetLightiv
#define glLightModelf pfnglLightModelf
#define glLightModeli pfnglLightModeli
#define glLightModelfv pfnglLightModelfv
#define glLightModeliv pfnglLightModeliv
#define glMaterialf pfnglMaterialf
#define glMateriali pfnglMateriali
#define glMaterialfv pfnglMaterialfv
#define glMaterialiv pfnglMaterialiv
#define glGetMaterialfv pfnglGetMaterialfv
#define glGetMaterialiv pfnglGetMaterialiv
#define glColorMaterial pfnglColorMaterial
#define glPixelZoom pfnglPixelZoom
#define glPixelStoref pfnglPixelStoref
#define glPixelStorei pfnglPixelStorei
#define glPixelTransferf pfnglPixelTransferf
#define glPixelTransferi pfnglPixelTransferi
#define glPixelMapfv pfnglPixelMapfv
#define glPixelMapuiv pfnglPixelMapuiv
#define glPixelMapusv pfnglPixelMapusv
#define glGetPixelMapfv pfnglGetPixelMapfv
#define glGetPixelMapuiv pfnglGetPixelMapuiv
#define glGetPixelMapusv pfnglGetPixelMapusv
#define glBitmap pfnglBitmap
#define glReadPixels pfnglReadPixels
#define glDrawPixels pfnglDrawPixels
#define glCopyPixels pfnglCopyPixels
#define glStencilFunc pfnglStencilFunc
#define glStencilMask pfnglStencilMask
#define glStencilOp pfnglStencilOp
#define glClearStencil pfnglClearStencil
#define glTexGend pfnglTexGend
#define glTexGenf pfnglTexGenf
#define glTexGeni pfnglTexGeni
#define glTexGendv pfnglTexGendv
#define glTexGenfv pfnglTexGenfv
#define glTexGeniv pfnglTexGeniv
#define glGetTexGendv pfnglGetTexGendv
#define glGetTexGenfv pfnglGetTexGenfv
#define glGetTexGeniv pfnglGetTexGeniv
#define glTexEnvf pfnglTexEnvf
#define glTexEnvi pfnglTexEnvi
#define glTexEnvfv pfnglTexEnvfv
#define glTexEnviv pfnglTexEnviv
#define glGetTexEnvfv pfnglGetTexEnvfv
#define glGetTexEnviv pfnglGetTexEnviv
#define glTexParameterf pfnglTexParameterf
#define glTexParameteri pfnglTexParameteri
#define glTexParameterfv pfnglTexParameterfv
#define glTexParameteriv pfnglTexParameteriv
#define glGetTexParameterfv pfnglGetTexParameterfv
#define glGetTexParameteriv pfnglGetTexParameteriv
#define glGetTexLevelParameterfv pfnglGetTexLevelParameterfv
#define glGetTexLevelParameteriv pfnglGetTexLevelParameteriv
#define glTexImage1D pfnglTexImage1D
#define glTexImage2D pfnglTexImage2D
#define glGetTexImage pfnglGetTexImage
#define glGenTextures pfnglGenTextures
#define glDeleteTextures pfnglDeleteTextures
#define glBindTexture pfnglBindTexture
#define glPrioritizeTextures pfnglPrioritizeTextures
#define glAreTexturesResident pfnglAreTexturesResident
#define glIsTexture pfnglIsTexture
#define glTexSubImage1D pfnglTexSubImage1D
#define glTexSubImage2D pfnglTexSubImage2D
#define glCopyTexImage1D pfnglCopyTexImage1D
#define glCopyTexImage2D pfnglCopyTexImage2D
#define glCopyTexSubImage1D pfnglCopyTexSubImage1D
#define glCopyTexSubImage2D pfnglCopyTexSubImage2D
#define glMap1d pfnglMap1d
#define glMap1f pfnglMap1f
#define glMap2d pfnglMap2d
#define glMap2f pfnglMap2f
#define glGetMapdv pfnglGetMapdv
#define glGetMapfv pfnglGetMapfv
#define glGetMapiv pfnglGetMapiv
#define glEvalCoord1d pfnglEvalCoord1d
#define glEvalCoord1f pfnglEvalCoord1f
#define glEvalCoord1dv pfnglEvalCoord1dv
#define glEvalCoord1fv pfnglEvalCoord1fv
#define glEvalCoord2d pfnglEvalCoord2d
#define glEvalCoord2f pfnglEvalCoord2f
#define glEvalCoord2dv pfnglEvalCoord2dv
#define glEvalCoord2fv pfnglEvalCoord2fv
#define glMapGrid1d pfnglMapGrid1d
#define glMapGrid1f pfnglMapGrid1f
#define glMapGrid2d pfnglMapGrid2d
#define glMapGrid2f pfnglMapGrid2f
#define glEvalPoint1 pfnglEvalPoint1
#define glEvalPoint2 pfnglEvalPoint2
#define glEvalMesh1 pfnglEvalMesh1
#define glEvalMesh2 pfnglEvalMesh2
#define glFogf pfnglFogf
#define glFogi pfnglFogi
#define glFogfv pfnglFogfv
#define glFogiv pfnglFogiv
#define glFeedbackBuffer pfnglFeedbackBuffer
#define glPassThrough pfnglPassThrough
#define glSelectBuffer pfnglSelectBuffer
#define glInitNames pfnglInitNames
#define glLoadName pfnglLoadName
#define glPushName pfnglPushName
#define glPopName pfnglPopName

#endif // LC_OPENGL_DYNAMIC

#endif // _OPENGL_H_

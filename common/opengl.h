#ifndef _OPENGL_H_
#define _OPENGL_H_

#ifdef LC_WINDOWS
#include "stdafx.h"
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#ifdef LC_LINUX
#include <GL/glx.h>
#include "linux_gl.h"
#endif 


bool InitializeOpenGL (const char* libname);
void ShutdownOpenGL ();


// =============================================================================
// OpenGL functions typedefs

// Miscellaneous
typedef void (*PFNGLCLEARINDEX) (GLfloat c);
typedef void (*PFNGLCLEARCOLOR) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (*PFNGLCLEAR) (GLbitfield mask);
typedef void (*PFNGLINDEXMASK) (GLuint mask);
typedef void (*PFNGLCOLORMASK) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
typedef void (*PFNGLALPHAFUNC) (GLenum func, GLclampf ref);
typedef void (*PFNGLBLENDFUNC) (GLenum sfactor, GLenum dfactor);
typedef void (*PFNGLLOGICOP) (GLenum opcode);
typedef void (*PFNGLCULLFACE) (GLenum mode);
typedef void (*PFNGLFRONTFACE) (GLenum mode);
typedef void (*PFNGLPOINTSIZE) (GLfloat size);
typedef void (*PFNGLLINEWIDTH) (GLfloat width);
typedef void (*PFNGLLINESTIPPLE) (GLint factor, GLushort pattern);
typedef void (*PFNGLPOLYGONMODE) (GLenum face, GLenum mode);
typedef void (*PFNGLPOLYGONOFFSET) (GLfloat factor, GLfloat units);
typedef void (*PFNGLPOLYGONSTIPPLE) (const GLubyte *mask);
typedef void (*PFNGLGETPOLYGONSTIPPLE) (GLubyte *mask);
typedef void (*PFNGLEDGEFLAG) (GLboolean flag);
typedef void (*PFNGLEDGEFLAGV) (const GLboolean *flag);
typedef void (*PFNGLSCISSOR) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (*PFNGLCLIPPLANE) (GLenum plane, const GLdouble *equation);
typedef void (*PFNGLGETCLIPPLANE) (GLenum plane, GLdouble *equation);
typedef void (*PFNGLDRAWBUFFER) (GLenum mode);
typedef void (*PFNGLREADBUFFER) (GLenum mode);
typedef void (*PFNGLENABLE) (GLenum cap);
typedef void (*PFNGLDISABLE) (GLenum cap);
typedef GLboolean (*PFNGLISENABLED) (GLenum cap);
typedef void (*PFNGLENABLECLIENTSTATE) (GLenum cap);
typedef void (*PFNGLDISABLECLIENTSTATE) (GLenum cap);
typedef void (*PFNGLGETBOOLEANV) (GLenum pname, GLboolean *params);
typedef void (*PFNGLGETDOUBLEV) (GLenum pname, GLdouble *params);
typedef void (*PFNGLGETFLOATV) (GLenum pname, GLfloat *params);
typedef void (*PFNGLGETINTEGERV) (GLenum pname, GLint *params);
typedef void (*PFNGLPUSHATTRIB) (GLbitfield mask);
typedef void (*PFNGLPOPATTRIB) (void);
typedef void (*PFNGLPUSHCLIENTATTRIB) (GLbitfield mask);
typedef void (*PFNGLPOPCLIENTATTRIB) (void);
typedef GLint (*PFNGLRENDERMODE) (GLenum mode);
typedef GLenum (*PFNGLGETERROR) (void);
typedef const GLubyte* (*PFNGLGETSTRING) (GLenum name);
typedef void (*PFNGLFINISH) (void);
typedef void (*PFNGLFLUSH) (void);
typedef void (*PFNGLHINT) (GLenum target, GLenum mode);

// Depth Buffer
typedef void (*PFNGLCLEARDEPTH) (GLclampd depth);
typedef void (*PFNGLDEPTHFUNC) (GLenum func);
typedef void (*PFNGLDEPTHMASK) (GLboolean flag);
typedef void (*PFNGLDEPTHRANGE) (GLclampd near_val, GLclampd far_val);

// Accumulation Buffer
typedef void (*PFNGLCLEARACCUM) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (*PFNGLACCUM) (GLenum op, GLfloat value);

// Transformation
typedef void (*PFNGLMATRIXMODE) (GLenum mode);
typedef void (*PFNGLORTHO) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
typedef void (*PFNGLFRUSTUM) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
typedef void (*PFNGLVIEWPORT) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (*PFNGLPUSHMATRIX) (void);
typedef void (*PFNGLPOPMATRIX) (void);
typedef void (*PFNGLLOADIDENTITY) (void);
typedef void (*PFNGLLOADMATRIXD) (const GLdouble *m);
typedef void (*PFNGLLOADMATRIXF) (const GLfloat *m);
typedef void (*PFNGLMULTMATRIXD) (const GLdouble *m);
typedef void (*PFNGLMULTMATRIXF) (const GLfloat *m);
typedef void (*PFNGLROTATED) (GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
typedef void (*PFNGLROTATEF) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
typedef void (*PFNGLSCALED) (GLdouble x, GLdouble y, GLdouble z);
typedef void (*PFNGLSCALEF) (GLfloat x, GLfloat y, GLfloat z);
typedef void (*PFNGLTRANSLATED) (GLdouble x, GLdouble y, GLdouble z);
typedef void (*PFNGLTRANSLATEF) (GLfloat x, GLfloat y, GLfloat z);

// Display Lists
typedef GLboolean (*PFNGLISLIST) (GLuint list);
typedef void (*PFNGLDELETELISTS) (GLuint list, GLsizei range);
typedef GLuint (*PFNGLGENLISTS) (GLsizei range);
typedef void (*PFNGLNEWLIST) (GLuint list, GLenum mode);
typedef void (*PFNGLENDLIST) (void);
typedef void (*PFNGLCALLLIST) (GLuint list);
typedef void (*PFNGLCALLLISTS) (GLsizei n, GLenum type, const GLvoid *lists);
typedef void (*PFNGLLISTBASE) (GLuint base);

// Drawing Functions
typedef void (*PFNGLBEGIN) (GLenum mode);
typedef void (*PFNGLEND) (void);
typedef void (*PFNGLVERTEX2D) (GLdouble x, GLdouble y);
typedef void (*PFNGLVERTEX2F) (GLfloat x, GLfloat y);
typedef void (*PFNGLVERTEX2I) (GLint x, GLint y);
typedef void (*PFNGLVERTEX2S) (GLshort x, GLshort y);
typedef void (*PFNGLVERTEX3D) (GLdouble x, GLdouble y, GLdouble z);
typedef void (*PFNGLVERTEX3F) (GLfloat x, GLfloat y, GLfloat z);
typedef void (*PFNGLVERTEX3I) (GLint x, GLint y, GLint z);
typedef void (*PFNGLVERTEX3S) (GLshort x, GLshort y, GLshort z);
typedef void (*PFNGLVERTEX4D) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (*PFNGLVERTEX4F) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (*PFNGLVERTEX4I) (GLint x, GLint y, GLint z, GLint w);
typedef void (*PFNGLVERTEX4S) (GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (*PFNGLVERTEX2DV) (const GLdouble *v);
typedef void (*PFNGLVERTEX2FV) (const GLfloat *v);
typedef void (*PFNGLVERTEX2IV) (const GLint *v);
typedef void (*PFNGLVERTEX2SV) (const GLshort *v);
typedef void (*PFNGLVERTEX3DV) (const GLdouble *v);
typedef void (*PFNGLVERTEX3FV) (const GLfloat *v);
typedef void (*PFNGLVERTEX3IV) (const GLint *v);
typedef void (*PFNGLVERTEX3SV) (const GLshort *v);
typedef void (*PFNGLVERTEX4DV) (const GLdouble *v);
typedef void (*PFNGLVERTEX4FV) (const GLfloat *v);
typedef void (*PFNGLVERTEX4IV) (const GLint *v);
typedef void (*PFNGLVERTEX4SV) (const GLshort *v);
typedef void (*PFNGLNORMAL3B) (GLbyte nx, GLbyte ny, GLbyte nz);
typedef void (*PFNGLNORMAL3D) (GLdouble nx, GLdouble ny, GLdouble nz);
typedef void (*PFNGLNORMAL3F) (GLfloat nx, GLfloat ny, GLfloat nz);
typedef void (*PFNGLNORMAL3I) (GLint nx, GLint ny, GLint nz);
typedef void (*PFNGLNORMAL3S) (GLshort nx, GLshort ny, GLshort nz);
typedef void (*PFNGLNORMAL3BV) (const GLbyte *v);
typedef void (*PFNGLNORMAL3DV) (const GLdouble *v);
typedef void (*PFNGLNORMAL3FV) (const GLfloat *v);
typedef void (*PFNGLNORMAL3IV) (const GLint *v);
typedef void (*PFNGLNORMAL3SV) (const GLshort *v);
typedef void (*PFNGLINDEXD) (GLdouble c);
typedef void (*PFNGLINDEXF) (GLfloat c);
typedef void (*PFNGLINDEXI) (GLint c);
typedef void (*PFNGLINDEXS) (GLshort c);
typedef void (*PFNGLINDEXUB) (GLubyte c);
typedef void (*PFNGLINDEXDV) (const GLdouble *c);
typedef void (*PFNGLINDEXFV) (const GLfloat *c);
typedef void (*PFNGLINDEXIV) (const GLint *c);
typedef void (*PFNGLINDEXSV) (const GLshort *c);
typedef void (*PFNGLINDEXUBV) (const GLubyte *c);
typedef void (*PFNGLCOLOR3B) (GLbyte red, GLbyte green, GLbyte blue);
typedef void (*PFNGLCOLOR3D) (GLdouble red, GLdouble green, GLdouble blue);
typedef void (*PFNGLCOLOR3F) (GLfloat red, GLfloat green, GLfloat blue);
typedef void (*PFNGLCOLOR3I) (GLint red, GLint green, GLint blue);
typedef void (*PFNGLCOLOR3S) (GLshort red, GLshort green, GLshort blue);
typedef void (*PFNGLCOLOR3UB) (GLubyte red, GLubyte green, GLubyte blue);
typedef void (*PFNGLCOLOR3UI) (GLuint red, GLuint green, GLuint blue);
typedef void (*PFNGLCOLOR3US) (GLushort red, GLushort green, GLushort blue);
typedef void (*PFNGLCOLOR4B) (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
typedef void (*PFNGLCOLOR4D) (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
typedef void (*PFNGLCOLOR4F) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (*PFNGLCOLOR4I) (GLint red, GLint green, GLint blue, GLint alpha);
typedef void (*PFNGLCOLOR4S) (GLshort red, GLshort green, GLshort blue, GLshort alpha);
typedef void (*PFNGLCOLOR4UB) (GLubyte red, GLubyte green,GLubyte blue, GLubyte alpha);
typedef void (*PFNGLCOLOR4UI) (GLuint red, GLuint green, GLuint blue, GLuint alpha);
typedef void (*PFNGLCOLOR4US) (GLushort red, GLushort green, GLushort blue, GLushort alpha);
typedef void (*PFNGLCOLOR3BV) (const GLbyte *v);
typedef void (*PFNGLCOLOR3DV) (const GLdouble *v);
typedef void (*PFNGLCOLOR3FV) (const GLfloat *v);
typedef void (*PFNGLCOLOR3IV) (const GLint *v);
typedef void (*PFNGLCOLOR3SV) (const GLshort *v);
typedef void (*PFNGLCOLOR3UBV) (const GLubyte *v);
typedef void (*PFNGLCOLOR3UIV) (const GLuint *v);
typedef void (*PFNGLCOLOR3USV) (const GLushort *v);
typedef void (*PFNGLCOLOR4BV) (const GLbyte *v);
typedef void (*PFNGLCOLOR4DV) (const GLdouble *v);
typedef void (*PFNGLCOLOR4FV) (const GLfloat *v);
typedef void (*PFNGLCOLOR4IV) (const GLint *v);
typedef void (*PFNGLCOLOR4SV) (const GLshort *v);
typedef void (*PFNGLCOLOR4UBV) (const GLubyte *v);
typedef void (*PFNGLCOLOR4UIV) (const GLuint *v);
typedef void (*PFNGLCOLOR4USV) (const GLushort *v);
typedef void (*PFNGLTEXCOORD1D) (GLdouble s);
typedef void (*PFNGLTEXCOORD1F) (GLfloat s);
typedef void (*PFNGLTEXCOORD1I) (GLint s);
typedef void (*PFNGLTEXCOORD1S) (GLshort s);
typedef void (*PFNGLTEXCOORD2D) (GLdouble s, GLdouble t);
typedef void (*PFNGLTEXCOORD2F) (GLfloat s, GLfloat t);
typedef void (*PFNGLTEXCOORD2I) (GLint s, GLint t);
typedef void (*PFNGLTEXCOORD2S) (GLshort s, GLshort t);
typedef void (*PFNGLTEXCOORD3D) (GLdouble s, GLdouble t, GLdouble r);
typedef void (*PFNGLTEXCOORD3F) (GLfloat s, GLfloat t, GLfloat r);
typedef void (*PFNGLTEXCOORD3I) (GLint s, GLint t, GLint r);
typedef void (*PFNGLTEXCOORD3S) (GLshort s, GLshort t, GLshort r);
typedef void (*PFNGLTEXCOORD4D) (GLdouble s, GLdouble t, GLdouble r, GLdouble q);
typedef void (*PFNGLTEXCOORD4F) (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void (*PFNGLTEXCOORD4I) (GLint s, GLint t, GLint r, GLint q);
typedef void (*PFNGLTEXCOORD4S) (GLshort s, GLshort t, GLshort r, GLshort q);
typedef void (*PFNGLTEXCOORD1DV) (const GLdouble *v);
typedef void (*PFNGLTEXCOORD1FV) (const GLfloat *v);
typedef void (*PFNGLTEXCOORD1IV) (const GLint *v);
typedef void (*PFNGLTEXCOORD1SV) (const GLshort *v);
typedef void (*PFNGLTEXCOORD2DV) (const GLdouble *v);
typedef void (*PFNGLTEXCOORD2FV) (const GLfloat *v);
typedef void (*PFNGLTEXCOORD2IV) (const GLint *v);
typedef void (*PFNGLTEXCOORD2SV) (const GLshort *v);
typedef void (*PFNGLTEXCOORD3DV) (const GLdouble *v);
typedef void (*PFNGLTEXCOORD3FV) (const GLfloat *v);
typedef void (*PFNGLTEXCOORD3IV) (const GLint *v);
typedef void (*PFNGLTEXCOORD3SV) (const GLshort *v);
typedef void (*PFNGLTEXCOORD4DV) (const GLdouble *v);
typedef void (*PFNGLTEXCOORD4FV) (const GLfloat *v);
typedef void (*PFNGLTEXCOORD4IV) (const GLint *v);
typedef void (*PFNGLTEXCOORD4SV) (const GLshort *v);
typedef void (*PFNGLRASTERPOS2D) (GLdouble x, GLdouble y);
typedef void (*PFNGLRASTERPOS2F) (GLfloat x, GLfloat y);
typedef void (*PFNGLRASTERPOS2I) (GLint x, GLint y);
typedef void (*PFNGLRASTERPOS2S) (GLshort x, GLshort y);
typedef void (*PFNGLRASTERPOS3D) (GLdouble x, GLdouble y, GLdouble z);
typedef void (*PFNGLRASTERPOS3F) (GLfloat x, GLfloat y, GLfloat z);
typedef void (*PFNGLRASTERPOS3I) (GLint x, GLint y, GLint z);
typedef void (*PFNGLRASTERPOS3S) (GLshort x, GLshort y, GLshort z);
typedef void (*PFNGLRASTERPOS4D) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (*PFNGLRASTERPOS4F) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (*PFNGLRASTERPOS4I) (GLint x, GLint y, GLint z, GLint w);
typedef void (*PFNGLRASTERPOS4S) (GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (*PFNGLRASTERPOS2DV) (const GLdouble *v);
typedef void (*PFNGLRASTERPOS2FV) (const GLfloat *v);
typedef void (*PFNGLRASTERPOS2IV) (const GLint *v);
typedef void (*PFNGLRASTERPOS2SV) (const GLshort *v);
typedef void (*PFNGLRASTERPOS3DV) (const GLdouble *v);
typedef void (*PFNGLRASTERPOS3FV) (const GLfloat *v);
typedef void (*PFNGLRASTERPOS3IV) (const GLint *v);
typedef void (*PFNGLRASTERPOS3SV) (const GLshort *v);
typedef void (*PFNGLRASTERPOS4DV) (const GLdouble *v);
typedef void (*PFNGLRASTERPOS4FV) (const GLfloat *v);
typedef void (*PFNGLRASTERPOS4IV) (const GLint *v);
typedef void (*PFNGLRASTERPOS4SV) (const GLshort *v);
typedef void (*PFNGLRECTD) (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
typedef void (*PFNGLRECTF) (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
typedef void (*PFNGLRECTI) (GLint x1, GLint y1, GLint x2, GLint y2);
typedef void (*PFNGLRECTS) (GLshort x1, GLshort y1, GLshort x2, GLshort y2);
typedef void (*PFNGLRECTDV) (const GLdouble *v1, const GLdouble *v2);
typedef void (*PFNGLRECTFV) (const GLfloat *v1, const GLfloat *v2);
typedef void (*PFNGLRECTIV) (const GLint *v1, const GLint *v2);
typedef void (*PFNGLRECTSV) (const GLshort *v1, const GLshort *v2);

// Vertex Arrays
typedef void (*PFNGLVERTEXPOINTER) (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (*PFNGLNORMALPOINTER) (GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (*PFNGLCOLORPOINTER) (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (*PFNGLINDEXPOINTER) (GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (*PFNGLTEXCOORDPOINTER) (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (*PFNGLEDGEFLAGPOINTER) (GLsizei stride, const GLvoid *ptr);
typedef void (*PFNGLGETPOINTERV) (GLenum pname, void **params);
typedef void (*PFNGLARRAYELEMENT) (GLint i);
typedef void (*PFNGLDRAWARRAYS) (GLenum mode, GLint first, GLsizei count);
typedef void (*PFNGLDRAWELEMENTS) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
typedef void (*PFNGLINTERLEAVEDARRAYS) (GLenum format, GLsizei stride, const GLvoid *pointer);

// Lighting
typedef void (*PFNGLSHADEMODEL) (GLenum mode);
typedef void (*PFNGLLIGHTF) (GLenum light, GLenum pname, GLfloat param);
typedef void (*PFNGLLIGHTI) (GLenum light, GLenum pname, GLint param);
typedef void (*PFNGLLIGHTFV) (GLenum light, GLenum pname, const GLfloat *params);
typedef void (*PFNGLLIGHTIV) (GLenum light, GLenum pname, const GLint *params);
typedef void (*PFNGLGETLIGHTFV) (GLenum light, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETLIGHTIV) (GLenum light, GLenum pname, GLint *params);
typedef void (*PFNGLLIGHTMODELF) (GLenum pname, GLfloat param);
typedef void (*PFNGLLIGHTMODELI) (GLenum pname, GLint param);
typedef void (*PFNGLLIGHTMODELFV) (GLenum pname, const GLfloat *params);
typedef void (*PFNGLLIGHTMODELIV) (GLenum pname, const GLint *params);
typedef void (*PFNGLMATERIALF) (GLenum face, GLenum pname, GLfloat param);
typedef void (*PFNGLMATERIALI) (GLenum face, GLenum pname, GLint param);
typedef void (*PFNGLMATERIALFV) (GLenum face, GLenum pname, const GLfloat *params);
typedef void (*PFNGLMATERIALIV) (GLenum face, GLenum pname, const GLint *params);
typedef void (*PFNGLGETMATERIALFV) (GLenum face, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETMATERIALIV) (GLenum face, GLenum pname, GLint *params);
typedef void (*PFNGLCOLORMATERIAL) (GLenum face, GLenum mode);

// Raster functions
typedef void (*PFNGLPIXELZOOM) (GLfloat xfactor, GLfloat yfactor);
typedef void (*PFNGLPIXELSTOREF) (GLenum pname, GLfloat param);
typedef void (*PFNGLPIXELSTOREI) (GLenum pname, GLint param);
typedef void (*PFNGLPIXELTRANSFERF) (GLenum pname, GLfloat param);
typedef void (*PFNGLPIXELTRANSFERI) (GLenum pname, GLint param);
typedef void (*PFNGLPIXELMAPFV) (GLenum map, GLint mapsize, const GLfloat *values);
typedef void (*PFNGLPIXELMAPUIV) (GLenum map, GLint mapsize, const GLuint *values);
typedef void (*PFNGLPIXELMAPUSV) (GLenum map, GLint mapsize, const GLushort *values);
typedef void (*PFNGLGETPIXELMAPFV) (GLenum map, GLfloat *values);
typedef void (*PFNGLGETPIXELMAPUIV) (GLenum map, GLuint *values);
typedef void (*PFNGLGETPIXELMAPUSV) (GLenum map, GLushort *values);
typedef void (*PFNGLBITMAP) (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap );
typedef void (*PFNGLREADPIXELS) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
typedef void (*PFNGLDRAWPIXELS) (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (*PFNGLCOPYPIXELS) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);

// Stenciling
typedef void (*PFNGLSTENCILFUNC) (GLenum func, GLint ref, GLuint mask);
typedef void (*PFNGLSTENCILMASK) (GLuint mask);
typedef void (*PFNGLSTENCILOP) (GLenum fail, GLenum zfail, GLenum zpass);
typedef void (*PFNGLCLEARSTENCIL) (GLint s);

// Texture mapping
typedef void (*PFNGLTEXGEND) (GLenum coord, GLenum pname, GLdouble param);
typedef void (*PFNGLTEXGENF) (GLenum coord, GLenum pname, GLfloat param);
typedef void (*PFNGLTEXGENI) (GLenum coord, GLenum pname, GLint param);
typedef void (*PFNGLTEXGENDV) (GLenum coord, GLenum pname, const GLdouble *params);
typedef void (*PFNGLTEXGENFV) (GLenum coord, GLenum pname, const GLfloat *params);
typedef void (*PFNGLTEXGENIV) (GLenum coord, GLenum pname, const GLint *params);
typedef void (*PFNGLGETTEXGENDV) (GLenum coord, GLenum pname, GLdouble *params);
typedef void (*PFNGLGETTEXGENFV) (GLenum coord, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETTEXGENIV) (GLenum coord, GLenum pname, GLint *params);
typedef void (*PFNGLTEXENVF) (GLenum target, GLenum pname, GLfloat param);
typedef void (*PFNGLTEXENVI) (GLenum target, GLenum pname, GLint param);
typedef void (*PFNGLTEXENVFV) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (*PFNGLTEXENVIV) (GLenum target, GLenum pname, const GLint *params);
typedef void (*PFNGLGETTEXENVFV) (GLenum target, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETTEXENVIV) (GLenum target, GLenum pname, GLint *params);
typedef void (*PFNGLTEXPARAMETERF) (GLenum target, GLenum pname, GLfloat param);
typedef void (*PFNGLTEXPARAMETERI) (GLenum target, GLenum pname, GLint param);
typedef void (*PFNGLTEXPARAMETERFV) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (*PFNGLTEXPARAMETERIV) (GLenum target, GLenum pname, const GLint *params);
typedef void (*PFNGLGETTEXPARAMETERFV) (GLenum target, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETTEXPARAMETERIV) (GLenum target, GLenum pname, GLint *params);
typedef void (*PFNGLGETTEXLEVELPARAMETERFV) (GLenum target, GLint level, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETTEXLEVELPARAMETERIV) (GLenum target, GLint level, GLenum pname, GLint *params);
typedef void (*PFNGLTEXIMAGE1D) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (*PFNGLTEXIMAGE2D) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (*PFNGLGETTEXIMAGE) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
typedef void (*PFNGLGENTEXTURES) (GLsizei n, GLuint *textures);
typedef void (*PFNGLDELETETEXTURES) (GLsizei n, const GLuint *textures);
typedef void (*PFNGLBINDTEXTURE) (GLenum target, GLuint texture);
typedef void (*PFNGLPRIORITIZETEXTURES) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
typedef GLboolean (*PFNGLARETEXTURESRESIDENT) (GLsizei n, const GLuint *textures, GLboolean *residences);
typedef GLboolean (*PFNGLISTEXTURE) (GLuint texture);
typedef void (*PFNGLTEXSUBIMAGE1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (*PFNGLTEXSUBIMAGE2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (*PFNGLCOPYTEXIMAGE1D) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
typedef void (*PFNGLCOPYTEXIMAGE2D) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
typedef void (*PFNGLCOPYTEXSUBIMAGE1D) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
typedef void (*PFNGLCOPYTEXSUBIMAGE2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);

// Evaluators
typedef void (*PFNGLMAP1D) (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
typedef void (*PFNGLMAP1F) (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
typedef void (*PFNGLMAP2D) (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
typedef void (*PFNGLMAP2F) (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
typedef void (*PFNGLGETMAPDV) (GLenum target, GLenum query, GLdouble *v);
typedef void (*PFNGLGETMAPFV) (GLenum target, GLenum query, GLfloat *v);
typedef void (*PFNGLGETMAPIV) (GLenum target, GLenum query, GLint *v);
typedef void (*PFNGLEVALCOORD1D) (GLdouble u);
typedef void (*PFNGLEVALCOORD1F) (GLfloat u);
typedef void (*PFNGLEVALCOORD1DV) (const GLdouble *u);
typedef void (*PFNGLEVALCOORD1FV) (const GLfloat *u);
typedef void (*PFNGLEVALCOORD2D) (GLdouble u, GLdouble v);
typedef void (*PFNGLEVALCOORD2F) (GLfloat u, GLfloat v);
typedef void (*PFNGLEVALCOORD2DV) (const GLdouble *u);
typedef void (*PFNGLEVALCOORD2FV) (const GLfloat *u);
typedef void (*PFNGLMAPGRID1D) (GLint un, GLdouble u1, GLdouble u2);
typedef void (*PFNGLMAPGRID1F) (GLint un, GLfloat u1, GLfloat u2);
typedef void (*PFNGLMAPGRID2D) (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
typedef void (*PFNGLMAPGRID2F) (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
typedef void (*PFNGLEVALPOINT1) (GLint i);
typedef void (*PFNGLEVALPOINT2) (GLint i, GLint j);
typedef void (*PFNGLEVALMESH1) (GLenum mode, GLint i1, GLint i2);
typedef void (*PFNGLEVALMESH2) (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);

// Fog
typedef void (*PFNGLFOGF) (GLenum pname, GLfloat param);
typedef void (*PFNGLFOGI) (GLenum pname, GLint param);
typedef void (*PFNGLFOGFV) (GLenum pname, const GLfloat *params);
typedef void (*PFNGLFOGIV) (GLenum pname, const GLint *params);

// Selection and Feedback
typedef void (*PFNGLFEEDBACKBUFFER) (GLsizei size, GLenum type, GLfloat *buffer);
typedef void (*PFNGLPASSTHROUGH) (GLfloat token);
typedef void (*PFNGLSELECTBUFFER) (GLsizei size, GLuint *buffer);
typedef void (*PFNGLINITNAMES) (void);
typedef void (*PFNGLLOADNAME) (GLuint name);
typedef void (*PFNGLPUSHNAME) (GLuint name);
typedef void (*PFNGLPOPNAME) (void);

// Extensions
typedef void (*PFNGLBLENDEQUATIONEXT) (GLenum mode);
typedef void (*PFNGLBLENDCOLOREXT) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (*PFNGLPOLYGONOFFSETEXT) (GLfloat factor, GLfloat bias);

// GL_EXT_vertex_array
typedef void (*PFNGLVERTEXPOINTEREXT) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr);
typedef void (*PFNGLNORMALPOINTEREXT) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr);
typedef void (*PFNGLCOLORPOINTEREXT) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr);
typedef void (*PFNGLINDEXPOINTEREXT) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr);
typedef void (*PFNGLTEXCOORDPOINTEREXT) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr);
typedef void (*PFNGLEDGEFLAGPOINTEREXT) (GLsizei stride, GLsizei count, const GLboolean *ptr);
typedef void (*PFNGLGETPOINTERVEXT) (GLenum pname, void **params);
typedef void (*PFNGLARRAYELEMENTEXT) (GLint i);
typedef void (*PFNGLDRAWARRAYSEXT) (GLenum mode, GLint first, GLsizei count);

// GL_EXT_texture_object
typedef void (*PFNGLGENTEXTURESEXT) (GLsizei n, GLuint *textures);
typedef void (*PFNGLDELETETEXTURESEXT) (GLsizei n, const GLuint *textures);
typedef void (*PFNGLBINDTEXTUREEXT) (GLenum target, GLuint texture);
typedef void (*PFNGLPRIORITIZETEXTURESEXT) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
typedef GLboolean (*PFNGLARETEXTURESRESIDENTEXT) (GLsizei n, const GLuint *textures, GLboolean *residences);
typedef GLboolean (*PFNGLISTEXTUREEXT) (GLuint texture);

// GL_EXT_texture3D
typedef void (*PFNGLTEXIMAGE3DEXT) (GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (*PFNGLTEXSUBIMAGE3DEXT) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (*PFNGLCOPYTEXSUBIMAGE3DEXT) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);

// GL_EXT_color_table
typedef void (*PFNGLCOLORTABLEEXT) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
typedef void (*PFNGLCOLORSUBTABLEEXT) (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
typedef void (*PFNGLGETCOLORTABLEEXT) (GLenum target, GLenum format, GLenum type, GLvoid *table);
typedef void (*PFNGLGETCOLORTABLEPARAMETERFVEXT) (GLenum target, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETCOLORTABLEPARAMETERIVEXT) (GLenum target, GLenum pname, GLint *params);

// GL_ARB_multitexture
typedef void (*PFNGLACTIVETEXTUREARB) (GLenum texture);
typedef void (*PFNGLCLIENTACTIVETEXTUREARB) (GLenum texture);
typedef void (*PFNGLMULTITEXCOORD1DARB) (GLenum target, GLdouble s);
typedef void (*PFNGLMULTITEXCOORD1DVARB) (GLenum target, const GLdouble *v);
typedef void (*PFNGLMULTITEXCOORD1FARB) (GLenum target, GLfloat s);
typedef void (*PFNGLMULTITEXCOORD1FVARB) (GLenum target, const GLfloat *v);
typedef void (*PFNGLMULTITEXCOORD1IARB) (GLenum target, GLint s);
typedef void (*PFNGLMULTITEXCOORD1IVARB) (GLenum target, const GLint *v);
typedef void (*PFNGLMULTITEXCOORD1SARB) (GLenum target, GLshort s);
typedef void (*PFNGLMULTITEXCOORD1SVARB) (GLenum target, const GLshort *v);
typedef void (*PFNGLMULTITEXCOORD2DARB) (GLenum target, GLdouble s, GLdouble t);
typedef void (*PFNGLMULTITEXCOORD2DVARB) (GLenum target, const GLdouble *v);
typedef void (*PFNGLMULTITEXCOORD2FARB) (GLenum target, GLfloat s, GLfloat t);
typedef void (*PFNGLMULTITEXCOORD2FVARB) (GLenum target, const GLfloat *v);
typedef void (*PFNGLMULTITEXCOORD2IARB) (GLenum target, GLint s, GLint t);
typedef void (*PFNGLMULTITEXCOORD2IVARB) (GLenum target, const GLint *v);
typedef void (*PFNGLMULTITEXCOORD2SARB) (GLenum target, GLshort s, GLshort t);
typedef void (*PFNGLMULTITEXCOORD2SVARB) (GLenum target, const GLshort *v);
typedef void (*PFNGLMULTITEXCOORD3DARB) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
typedef void (*PFNGLMULTITEXCOORD3DVARB) (GLenum target, const GLdouble *v);
typedef void (*PFNGLMULTITEXCOORD3FARB) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
typedef void (*PFNGLMULTITEXCOORD3FVARB) (GLenum target, const GLfloat *v);
typedef void (*PFNGLMULTITEXCOORD3IARB) (GLenum target, GLint s, GLint t, GLint r);
typedef void (*PFNGLMULTITEXCOORD3IVARB) (GLenum target, const GLint *v);
typedef void (*PFNGLMULTITEXCOORD3SARB) (GLenum target, GLshort s, GLshort t, GLshort r);
typedef void (*PFNGLMULTITEXCOORD3SVARB) (GLenum target, const GLshort *v);
typedef void (*PFNGLMULTITEXCOORD4DARB) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
typedef void (*PFNGLMULTITEXCOORD4DVARB) (GLenum target, const GLdouble *v);
typedef void (*PFNGLMULTITEXCOORD4FARB) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void (*PFNGLMULTITEXCOORD4FVARB) (GLenum target, const GLfloat *v);
typedef void (*PFNGLMULTITEXCOORD4IARB) (GLenum target, GLint s, GLint t, GLint r, GLint q);
typedef void (*PFNGLMULTITEXCOORD4IVARB) (GLenum target, const GLint *v);
typedef void (*PFNGLMULTITEXCOORD4SARB) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
typedef void (*PFNGLMULTITEXCOORD4SVARB) (GLenum target, const GLshort *v);

// GL_EXT_point_parameters
typedef void (*PFNGLPOINTPARAMETERFEXT) (GLenum pname, GLfloat param);
typedef void (*PFNGLPOINTPARAMETERFVEXT) (GLenum pname, const GLfloat *params);

// GL_INGR_blend_func_separate
typedef void (*PFNGLBLENDFUNCSEPARATEINGR) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

// GL_MESA_window_pos
typedef void (*PFNGLWINDOWPOS2IMESA) (GLint x, GLint y);
typedef void (*PFNGLWINDOWPOS2SMESA) (GLshort x, GLshort y);
typedef void (*PFNGLWINDOWPOS2FMESA) (GLfloat x, GLfloat y);
typedef void (*PFNGLWINDOWPOS2DMESA) (GLdouble x, GLdouble y);
typedef void (*PFNGLWINDOWPOS2IVMESA) (const GLint *p);
typedef void (*PFNGLWINDOWPOS2SVMESA) (const GLshort *p);
typedef void (*PFNGLWINDOWPOS2FVMESA) (const GLfloat *p);
typedef void (*PFNGLWINDOWPOS2DVMESA) (const GLdouble *p);
typedef void (*PFNGLWINDOWPOS3IMESA) (GLint x, GLint y, GLint z);
typedef void (*PFNGLWINDOWPOS3SMESA) (GLshort x, GLshort y, GLshort z);
typedef void (*PFNGLWINDOWPOS3FMESA) (GLfloat x, GLfloat y, GLfloat z);
typedef void (*PFNGLWINDOWPOS3DMESA) (GLdouble x, GLdouble y, GLdouble z);
typedef void (*PFNGLWINDOWPOS3IVMESA) (const GLint *p);
typedef void (*PFNGLWINDOWPOS3SVMESA) (const GLshort *p);
typedef void (*PFNGLWINDOWPOS3FVMESA) (const GLfloat *p);
typedef void (*PFNGLWINDOWPOS3DVMESA) (const GLdouble *p);
typedef void (*PFNGLWINDOWPOS4IMESA) (GLint x, GLint y, GLint z, GLint w);
typedef void (*PFNGLWINDOWPOS4SMESA) (GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (*PFNGLWINDOWPOS4FMESA) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (*PFNGLWINDOWPOS4DMESA) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (*PFNGLWINDOWPOS4IVMESA) (const GLint *p);
typedef void (*PFNGLWINDOWPOS4SVMESA) (const GLshort *p);
typedef void (*PFNGLWINDOWPOS4FVMESA) (const GLfloat *p);
typedef void (*PFNGLWINDOWPOS4DVMESA) (const GLdouble *p);

// GL_MESA_resize_buffers
typedef void (*PFNGLRESIZEBUFFERSMESA) (void);

// 1.2 functions
typedef void (*PFNGLDRAWRANGEELEMENTS) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
typedef void (*PFNGLTEXIMAGE3D) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (*PFNGLTEXSUBIMAGE3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (*PFNGLCOPYTEXSUBIMAGE3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);

// 1.2 imaging extension functions
typedef void (*PFNGLCOLORTABLE) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
typedef void (*PFNGLCOLORSUBTABLE) (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
typedef void (*PFNGLCOLORTABLEPARAMETERIV) (GLenum target, GLenum pname, const GLint *params);
typedef void (*PFNGLCOLORTABLEPARAMETERFV) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (*PFNGLCOPYCOLORSUBTABLE) (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
typedef void (*PFNGLCOPYCOLORTABLE) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
typedef void (*PFNGLGETCOLORTABLE) (GLenum target, GLenum format, GLenum type, GLvoid *table);
typedef void (*PFNGLGETCOLORTABLEPARAMETERFV) (GLenum target, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETCOLORTABLEPARAMETERIV) (GLenum target, GLenum pname, GLint *params);
typedef void (*PFNGLBLENDEQUATION) (GLenum mode);
typedef void (*PFNGLBLENDCOLOR) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (*PFNGLHISTOGRAM) (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
typedef void (*PFNGLRESETHISTOGRAM) (GLenum target);
typedef void (*PFNGLGETHISTOGRAM) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
typedef void (*PFNGLGETHISTOGRAMPARAMETERFV) (GLenum target, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETHISTOGRAMPARAMETERIV) (GLenum target, GLenum pname, GLint *params);
typedef void (*PFNGLMINMAX) (GLenum target, GLenum internalformat, GLboolean sink);
typedef void (*PFNGLRESETMINMAX) (GLenum target);
typedef void (*PFNGLGETMINMAX) (GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values);
typedef void (*PFNGLGETMINMAXPARAMETERFV) (GLenum target, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETMINMAXPARAMETERIV) (GLenum target, GLenum pname, GLint *params);
typedef void (*PFNGLCONVOLUTIONFILTER1D) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
typedef void (*PFNGLCONVOLUTIONFILTER2D) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
typedef void (*PFNGLCONVOLUTIONPARAMETERF) (GLenum target, GLenum pname, GLfloat params);
typedef void (*PFNGLCONVOLUTIONPARAMETERFV) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (*PFNGLCONVOLUTIONPARAMETERI) (GLenum target, GLenum pname, GLint params);
typedef void (*PFNGLCONVOLUTIONPARAMETERIV) (GLenum target, GLenum pname, const GLint *params);
typedef void (*PFNGLCOPYCONVOLUTIONFILTER1D) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
typedef void (*PFNGLCOPYCONVOLUTIONFILTER2D) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (*PFNGLGETCONVOLUTIONFILTER) (GLenum target, GLenum format, GLenum type, GLvoid *image);
typedef void (*PFNGLGETCONVOLUTIONPARAMETERFV) (GLenum target, GLenum pname, GLfloat *params);
typedef void (*PFNGLGETCONVOLUTIONPARAMETERIV) (GLenum target, GLenum pname, GLint *params);
typedef void (*PFNGLSEPARABLEFILTER2D) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column);
typedef void (*PFNGLGETSEPARABLEFILTER) (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span);

// GL_EXT_compiled_vertex_array
typedef void (*PFNGLLOCKARRAYSEXT) (GLint first, GLsizei count);
typedef void (*PFNGLUNLOCKARRAYSEXT) (void);


// =============================================================================
// OpenGL extern declarations

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
extern PFNGLBLENDEQUATIONEXT pfnglBlendEquationEXT;
extern PFNGLBLENDCOLOREXT pfnglBlendColorEXT;
extern PFNGLPOLYGONOFFSETEXT pfnglPolygonOffsetEXT;
extern PFNGLVERTEXPOINTEREXT pfnglVertexPointerEXT;
extern PFNGLNORMALPOINTEREXT pfnglNormalPointerEXT;
extern PFNGLCOLORPOINTEREXT pfnglColorPointerEXT;
extern PFNGLINDEXPOINTEREXT pfnglIndexPointerEXT;
extern PFNGLTEXCOORDPOINTEREXT pfnglTexCoordPointerEXT;
extern PFNGLEDGEFLAGPOINTEREXT pfnglEdgeFlagPointerEXT;
extern PFNGLGETPOINTERVEXT pfnglGetPointervEXT;
extern PFNGLARRAYELEMENTEXT pfnglArrayElementEXT;
extern PFNGLDRAWARRAYSEXT pfnglDrawArraysEXT;
extern PFNGLGENTEXTURESEXT pfnglGenTexturesEXT;
extern PFNGLDELETETEXTURESEXT pfnglDeleteTexturesEXT;
extern PFNGLBINDTEXTUREEXT pfnglBindTextureEXT;
extern PFNGLPRIORITIZETEXTURESEXT pfnglPrioritizeTexturesEXT;
extern PFNGLARETEXTURESRESIDENTEXT pfnglAreTexturesResidentEXT;
extern PFNGLISTEXTUREEXT pfnglIsTextureEXT;
extern PFNGLTEXIMAGE3DEXT pfnglTexImage3DEXT;
extern PFNGLTEXSUBIMAGE3DEXT pfnglTexSubImage3DEXT;
extern PFNGLCOPYTEXSUBIMAGE3DEXT pfnglCopyTexSubImage3DEXT;
extern PFNGLCOLORTABLEEXT pfnglColorTableEXT;
extern PFNGLCOLORSUBTABLEEXT pfnglColorSubTableEXT;
extern PFNGLGETCOLORTABLEEXT pfnglGetColorTableEXT;
extern PFNGLGETCOLORTABLEPARAMETERFVEXT pfnglGetColorTableParameterfvEXT;
extern PFNGLGETCOLORTABLEPARAMETERIVEXT pfnglGetColorTableParameterivEXT;
extern PFNGLACTIVETEXTUREARB pfnglActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARB pfnglClientActiveTextureARB;
extern PFNGLMULTITEXCOORD1DARB pfnglMultiTexCoord1dARB;
extern PFNGLMULTITEXCOORD1DVARB pfnglMultiTexCoord1dvARB;
extern PFNGLMULTITEXCOORD1FARB pfnglMultiTexCoord1fARB;
extern PFNGLMULTITEXCOORD1FVARB pfnglMultiTexCoord1fvARB;
extern PFNGLMULTITEXCOORD1IARB pfnglMultiTexCoord1iARB;
extern PFNGLMULTITEXCOORD1IVARB pfnglMultiTexCoord1ivARB;
extern PFNGLMULTITEXCOORD1SARB pfnglMultiTexCoord1sARB;
extern PFNGLMULTITEXCOORD1SVARB pfnglMultiTexCoord1svARB;
extern PFNGLMULTITEXCOORD2DARB pfnglMultiTexCoord2dARB;
extern PFNGLMULTITEXCOORD2DVARB pfnglMultiTexCoord2dvARB;
extern PFNGLMULTITEXCOORD2FARB pfnglMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD2FVARB pfnglMultiTexCoord2fvARB;
extern PFNGLMULTITEXCOORD2IARB pfnglMultiTexCoord2iARB;
extern PFNGLMULTITEXCOORD2IVARB pfnglMultiTexCoord2ivARB;
extern PFNGLMULTITEXCOORD2SARB pfnglMultiTexCoord2sARB;
extern PFNGLMULTITEXCOORD2SVARB pfnglMultiTexCoord2svARB;
extern PFNGLMULTITEXCOORD3DARB pfnglMultiTexCoord3dARB;
extern PFNGLMULTITEXCOORD3DVARB pfnglMultiTexCoord3dvARB;
extern PFNGLMULTITEXCOORD3FARB pfnglMultiTexCoord3fARB;
extern PFNGLMULTITEXCOORD3FVARB pfnglMultiTexCoord3fvARB;
extern PFNGLMULTITEXCOORD3IARB pfnglMultiTexCoord3iARB;
extern PFNGLMULTITEXCOORD3IVARB pfnglMultiTexCoord3ivARB;
extern PFNGLMULTITEXCOORD3SARB pfnglMultiTexCoord3sARB;
extern PFNGLMULTITEXCOORD3SVARB pfnglMultiTexCoord3svARB;
extern PFNGLMULTITEXCOORD4DARB pfnglMultiTexCoord4dARB;
extern PFNGLMULTITEXCOORD4DVARB pfnglMultiTexCoord4dvARB;
extern PFNGLMULTITEXCOORD4FARB pfnglMultiTexCoord4fARB;
extern PFNGLMULTITEXCOORD4FVARB pfnglMultiTexCoord4fvARB;
extern PFNGLMULTITEXCOORD4IARB pfnglMultiTexCoord4iARB;
extern PFNGLMULTITEXCOORD4IVARB pfnglMultiTexCoord4ivARB;
extern PFNGLMULTITEXCOORD4SARB pfnglMultiTexCoord4sARB;
extern PFNGLMULTITEXCOORD4SVARB pfnglMultiTexCoord4svARB;
extern PFNGLPOINTPARAMETERFEXT pfnglPointParameterfEXT;
extern PFNGLPOINTPARAMETERFVEXT pfnglPointParameterfvEXT;
extern PFNGLBLENDFUNCSEPARATEINGR pfnglBlendFuncSeparateINGR;
extern PFNGLWINDOWPOS2IMESA pfnglWindowPos2iMESA;
extern PFNGLWINDOWPOS2SMESA pfnglWindowPos2sMESA;
extern PFNGLWINDOWPOS2FMESA pfnglWindowPos2fMESA;
extern PFNGLWINDOWPOS2DMESA pfnglWindowPos2dMESA;
extern PFNGLWINDOWPOS2IVMESA pfnglWindowPos2ivMESA;
extern PFNGLWINDOWPOS2SVMESA pfnglWindowPos2svMESA;
extern PFNGLWINDOWPOS2FVMESA pfnglWindowPos2fvMESA;
extern PFNGLWINDOWPOS2DVMESA pfnglWindowPos2dvMESA;
extern PFNGLWINDOWPOS3IMESA pfnglWindowPos3iMESA;
extern PFNGLWINDOWPOS3SMESA pfnglWindowPos3sMESA;
extern PFNGLWINDOWPOS3FMESA pfnglWindowPos3fMESA;
extern PFNGLWINDOWPOS3DMESA pfnglWindowPos3dMESA;
extern PFNGLWINDOWPOS3IVMESA pfnglWindowPos3ivMESA;
extern PFNGLWINDOWPOS3SVMESA pfnglWindowPos3svMESA;
extern PFNGLWINDOWPOS3FVMESA pfnglWindowPos3fvMESA;
extern PFNGLWINDOWPOS3DVMESA pfnglWindowPos3dvMESA;
extern PFNGLWINDOWPOS4IMESA pfnglWindowPos4iMESA;
extern PFNGLWINDOWPOS4SMESA pfnglWindowPos4sMESA;
extern PFNGLWINDOWPOS4FMESA pfnglWindowPos4fMESA;
extern PFNGLWINDOWPOS4DMESA pfnglWindowPos4dMESA;
extern PFNGLWINDOWPOS4IVMESA pfnglWindowPos4ivMESA;
extern PFNGLWINDOWPOS4SVMESA pfnglWindowPos4svMESA;
extern PFNGLWINDOWPOS4FVMESA pfnglWindowPos4fvMESA;
extern PFNGLWINDOWPOS4DVMESA pfnglWindowPos4dvMESA;
extern PFNGLRESIZEBUFFERSMESA pfnglResizeBuffersMESA;
extern PFNGLDRAWRANGEELEMENTS pfnglDrawRangeElements;
extern PFNGLTEXIMAGE3D pfnglTexImage3D;
extern PFNGLTEXSUBIMAGE3D pfnglTexSubImage3D;
extern PFNGLCOPYTEXSUBIMAGE3D pfnglCopyTexSubImage3D;
extern PFNGLCOLORTABLE pfnglColorTable;
extern PFNGLCOLORSUBTABLE pfnglColorSubTable;
extern PFNGLCOLORTABLEPARAMETERIV pfnglColorTableParameteriv;
extern PFNGLCOLORTABLEPARAMETERFV pfnglColorTableParameterfv;
extern PFNGLCOPYCOLORSUBTABLE pfnglCopyColorSubTable;
extern PFNGLCOPYCOLORTABLE pfnglCopyColorTable;
extern PFNGLGETCOLORTABLE pfnglGetColorTable;
extern PFNGLGETCOLORTABLEPARAMETERFV pfnglGetColorTableParameterfv;
extern PFNGLGETCOLORTABLEPARAMETERIV pfnglGetColorTableParameteriv;
extern PFNGLBLENDEQUATION pfnglBlendEquation;
extern PFNGLBLENDCOLOR pfnglBlendColor;
extern PFNGLHISTOGRAM pfnglHistogram;
extern PFNGLRESETHISTOGRAM pfnglResetHistogram;
extern PFNGLGETHISTOGRAM pfnglGetHistogram;
extern PFNGLGETHISTOGRAMPARAMETERFV pfnglGetHistogramParameterfv;
extern PFNGLGETHISTOGRAMPARAMETERIV pfnglGetHistogramParameteriv;
extern PFNGLMINMAX pfnglMinmax;
extern PFNGLRESETMINMAX pfnglResetMinmax;
extern PFNGLGETMINMAX pfnglGetMinmax;
extern PFNGLGETMINMAXPARAMETERFV pfnglGetMinmaxParameterfv;
extern PFNGLGETMINMAXPARAMETERIV pfnglGetMinmaxParameteriv;
extern PFNGLCONVOLUTIONFILTER1D pfnglConvolutionFilter1D;
extern PFNGLCONVOLUTIONFILTER2D pfnglConvolutionFilter2D;
extern PFNGLCONVOLUTIONPARAMETERF pfnglConvolutionParameterf;
extern PFNGLCONVOLUTIONPARAMETERFV pfnglConvolutionParameterfv;
extern PFNGLCONVOLUTIONPARAMETERI pfnglConvolutionParameteri;
extern PFNGLCONVOLUTIONPARAMETERIV pfnglConvolutionParameteriv;
extern PFNGLCOPYCONVOLUTIONFILTER1D pfnglCopyConvolutionFilter1D;
extern PFNGLCOPYCONVOLUTIONFILTER2D pfnglCopyConvolutionFilter2D;
extern PFNGLGETCONVOLUTIONFILTER pfnglGetConvolutionFilter;
extern PFNGLGETCONVOLUTIONPARAMETERFV pfnglGetConvolutionParameterfv;
extern PFNGLGETCONVOLUTIONPARAMETERIV pfnglGetConvolutionParameteriv;
extern PFNGLSEPARABLEFILTER2D pfnglSeparableFilter2D;
extern PFNGLGETSEPARABLEFILTER pfnglGetSeparableFilter;
extern PFNGLLOCKARRAYSEXT pfnglLockArraysEXT;
extern PFNGLUNLOCKARRAYSEXT pfnglUnlockArraysEXT;

// =============================================================================
// Replace OpenGL function names with the dynamic functions

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
#define glBlendEquationEXT pfnglBlendEquationEXT
#define glBlendColorEXT pfnglBlendColorEXT
#define glPolygonOffsetEXT pfnglPolygonOffsetEXT
#define glVertexPointerEXT pfnglVertexPointerEXT
#define glNormalPointerEXT pfnglNormalPointerEXT
#define glColorPointerEXT pfnglColorPointerEXT
#define glIndexPointerEXT pfnglIndexPointerEXT
#define glTexCoordPointerEXT pfnglTexCoordPointerEXT
#define glEdgeFlagPointerEXT pfnglEdgeFlagPointerEXT
#define glGetPointervEXT pfnglGetPointervEXT
#define glArrayElementEXT pfnglArrayElementEXT
#define glDrawArraysEXT pfnglDrawArraysEXT
#define glGenTexturesEXT pfnglGenTexturesEXT
#define glDeleteTexturesEXT pfnglDeleteTexturesEXT
#define glBindTextureEXT pfnglBindTextureEXT
#define glPrioritizeTexturesEXT pfnglPrioritizeTexturesEXT
#define glAreTexturesResidentEXT pfnglAreTexturesResidentEXT
#define glIsTextureEXT pfnglIsTextureEXT
#define glTexImage3DEXT pfnglTexImage3DEXT
#define glTexSubImage3DEXT pfnglTexSubImage3DEXT
#define glCopyTexSubImage3DEXT pfnglCopyTexSubImage3DEXT
#define glColorTableEXT pfnglColorTableEXT
#define glColorSubTableEXT pfnglColorSubTableEXT
#define glGetColorTableEXT pfnglGetColorTableEXT
#define glGetColorTableParameterfvEXT pfnglGetColorTableParameterfvEXT
#define glGetColorTableParameterivEXT pfnglGetColorTableParameterivEXT
#define glActiveTextureARB pfnglActiveTextureARB
#define glClientActiveTextureARB pfnglClientActiveTextureARB
#define glMultiTexCoord1dARB pfnglMultiTexCoord1dARB
#define glMultiTexCoord1dvARB pfnglMultiTexCoord1dvARB
#define glMultiTexCoord1fARB pfnglMultiTexCoord1fARB
#define glMultiTexCoord1fvARB pfnglMultiTexCoord1fvARB
#define glMultiTexCoord1iARB pfnglMultiTexCoord1iARB
#define glMultiTexCoord1ivARB pfnglMultiTexCoord1ivARB
#define glMultiTexCoord1sARB pfnglMultiTexCoord1sARB
#define glMultiTexCoord1svARB pfnglMultiTexCoord1svARB
#define glMultiTexCoord2dARB pfnglMultiTexCoord2dARB
#define glMultiTexCoord2dvARB pfnglMultiTexCoord2dvARB
#define glMultiTexCoord2fARB pfnglMultiTexCoord2fARB
#define glMultiTexCoord2fvARB pfnglMultiTexCoord2fvARB
#define glMultiTexCoord2iARB pfnglMultiTexCoord2iARB
#define glMultiTexCoord2ivARB pfnglMultiTexCoord2ivARB
#define glMultiTexCoord2sARB pfnglMultiTexCoord2sARB
#define glMultiTexCoord2svARB pfnglMultiTexCoord2svARB
#define glMultiTexCoord3dARB pfnglMultiTexCoord3dARB
#define glMultiTexCoord3dvARB pfnglMultiTexCoord3dvARB
#define glMultiTexCoord3fARB pfnglMultiTexCoord3fARB
#define glMultiTexCoord3fvARB pfnglMultiTexCoord3fvARB
#define glMultiTexCoord3iARB pfnglMultiTexCoord3iARB
#define glMultiTexCoord3ivARB pfnglMultiTexCoord3ivARB
#define glMultiTexCoord3sARB pfnglMultiTexCoord3sARB
#define glMultiTexCoord3svARB pfnglMultiTexCoord3svARB
#define glMultiTexCoord4dARB pfnglMultiTexCoord4dARB
#define glMultiTexCoord4dvARB pfnglMultiTexCoord4dvARB
#define glMultiTexCoord4fARB pfnglMultiTexCoord4fARB
#define glMultiTexCoord4fvARB pfnglMultiTexCoord4fvARB
#define glMultiTexCoord4iARB pfnglMultiTexCoord4iARB
#define glMultiTexCoord4ivARB pfnglMultiTexCoord4ivARB
#define glMultiTexCoord4sARB pfnglMultiTexCoord4sARB
#define glMultiTexCoord4svARB pfnglMultiTexCoord4svARB
#define glPointParameterfEXT pfnglPointParameterfEXT
#define glPointParameterfvEXT pfnglPointParameterfvEXT
#define glBlendFuncSeparateINGR pfnglBlendFuncSeparateINGR
#define glWindowPos2iMESA pfnglWindowPos2iMESA
#define glWindowPos2sMESA pfnglWindowPos2sMESA
#define glWindowPos2fMESA pfnglWindowPos2fMESA
#define glWindowPos2dMESA pfnglWindowPos2dMESA
#define glWindowPos2ivMESA pfnglWindowPos2ivMESA
#define glWindowPos2svMESA pfnglWindowPos2svMESA
#define glWindowPos2fvMESA pfnglWindowPos2fvMESA
#define glWindowPos2dvMESA pfnglWindowPos2dvMESA
#define glWindowPos3iMESA pfnglWindowPos3iMESA
#define glWindowPos3sMESA pfnglWindowPos3sMESA
#define glWindowPos3fMESA pfnglWindowPos3fMESA
#define glWindowPos3dMESA pfnglWindowPos3dMESA
#define glWindowPos3ivMESA pfnglWindowPos3ivMESA
#define glWindowPos3svMESA pfnglWindowPos3svMESA
#define glWindowPos3fvMESA pfnglWindowPos3fvMESA
#define glWindowPos3dvMESA pfnglWindowPos3dvMESA
#define glWindowPos4iMESA pfnglWindowPos4iMESA
#define glWindowPos4sMESA pfnglWindowPos4sMESA
#define glWindowPos4fMESA pfnglWindowPos4fMESA
#define glWindowPos4dMESA pfnglWindowPos4dMESA
#define glWindowPos4ivMESA pfnglWindowPos4ivMESA
#define glWindowPos4svMESA pfnglWindowPos4svMESA
#define glWindowPos4fvMESA pfnglWindowPos4fvMESA
#define glWindowPos4dvMESA pfnglWindowPos4dvMESA
#define glResizeBuffersMESA pfnglResizeBuffersMESA
#define glDrawRangeElements pfnglDrawRangeElements
#define glTexImage3D pfnglTexImage3D
#define glTexSubImage3D pfnglTexSubImage3D
#define glCopyTexSubImage3D pfnglCopyTexSubImage3D
#define glColorTable pfnglColorTable
#define glColorSubTable pfnglColorSubTable
#define glColorTableParameteriv pfnglColorTableParameteriv
#define glColorTableParameterfv pfnglColorTableParameterfv
#define glCopyColorSubTable pfnglCopyColorSubTable
#define glCopyColorTable pfnglCopyColorTable
#define glGetColorTable pfnglGetColorTable
#define glGetColorTableParameterfv pfnglGetColorTableParameterfv
#define glGetColorTableParameteriv pfnglGetColorTableParameteriv
#define glBlendEquation pfnglBlendEquation
#define glBlendColor pfnglBlendColor
#define glHistogram pfnglHistogram
#define glResetHistogram pfnglResetHistogram
#define glGetHistogram pfnglGetHistogram
#define glGetHistogramParameterfv pfnglGetHistogramParameterfv
#define glGetHistogramParameteriv pfnglGetHistogramParameteriv
#define glMinmax pfnglMinmax
#define glResetMinmax pfnglResetMinmax
#define glGetMinmax pfnglGetMinmax
#define glGetMinmaxParameterfv pfnglGetMinmaxParameterfv
#define glGetMinmaxParameteriv pfnglGetMinmaxParameteriv
#define glConvolutionFilter1D pfnglConvolutionFilter1D
#define glConvolutionFilter2D pfnglConvolutionFilter2D
#define glConvolutionParameterf pfnglConvolutionParameterf
#define glConvolutionParameterfv pfnglConvolutionParameterfv
#define glConvolutionParameteri pfnglConvolutionParameteri
#define glConvolutionParameteriv pfnglConvolutionParameteriv
#define glCopyConvolutionFilter1D pfnglCopyConvolutionFilter1D
#define glCopyConvolutionFilter2D pfnglCopyConvolutionFilter2D
#define glGetConvolutionFilter pfnglGetConvolutionFilter
#define glGetConvolutionParameterfv pfnglGetConvolutionParameterfv
#define glGetConvolutionParameteriv pfnglGetConvolutionParameteriv
#define glSeparableFilter2D pfnglSeparableFilter2D
#define glGetSeparableFilter pfnglGetSeparableFilter
#define glLockArraysEXT pfnglLockArraysEXT
#define glUnlockArraysEXT pfnglUnlockArraysEXT

#endif // _OPENGL_H_

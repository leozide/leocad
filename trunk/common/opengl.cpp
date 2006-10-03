//
// Routines to load the OpenGL libraries dynamically
//

#include <string.h>
#include <stdio.h>
#include "opengl.h"

// These functions should be defined in (system)_gl.cpp
bool Sys_GLOpenLibrary(const char* libname);
void Sys_GLCloseLibrary();
void* Sys_GLGetProc(const char *symbol);
void* Sys_GLGetExtension(const char *symbol);

// =============================================================================
// OpenGL Function pointers

PFNGLCLEARINDEX pfnglClearIndex;
PFNGLCLEARCOLOR pfnglClearColor;
PFNGLCLEAR pfnglClear;
PFNGLINDEXMASK pfnglIndexMask;
PFNGLCOLORMASK pfnglColorMask;
PFNGLALPHAFUNC pfnglAlphaFunc;
PFNGLBLENDFUNC pfnglBlendFunc;
PFNGLLOGICOP pfnglLogicOp;
PFNGLCULLFACE pfnglCullFace;
PFNGLFRONTFACE pfnglFrontFace;
PFNGLPOINTSIZE pfnglPointSize;
PFNGLLINEWIDTH pfnglLineWidth;
PFNGLLINESTIPPLE pfnglLineStipple;
PFNGLPOLYGONMODE pfnglPolygonMode;
PFNGLPOLYGONOFFSET pfnglPolygonOffset;
PFNGLPOLYGONSTIPPLE pfnglPolygonStipple;
PFNGLGETPOLYGONSTIPPLE pfnglGetPolygonStipple;
PFNGLEDGEFLAG pfnglEdgeFlag;
PFNGLEDGEFLAGV pfnglEdgeFlagv;
PFNGLSCISSOR pfnglScissor;
PFNGLCLIPPLANE pfnglClipPlane;
PFNGLGETCLIPPLANE pfnglGetClipPlane;
PFNGLDRAWBUFFER pfnglDrawBuffer;
PFNGLREADBUFFER pfnglReadBuffer;
PFNGLENABLE pfnglEnable;
PFNGLDISABLE pfnglDisable;
PFNGLISENABLED pfnglIsEnabled;
PFNGLENABLECLIENTSTATE pfnglEnableClientState;
PFNGLDISABLECLIENTSTATE pfnglDisableClientState;
PFNGLGETBOOLEANV pfnglGetBooleanv;
PFNGLGETDOUBLEV pfnglGetDoublev;
PFNGLGETFLOATV pfnglGetFloatv;
PFNGLGETINTEGERV pfnglGetIntegerv;
PFNGLPUSHATTRIB pfnglPushAttrib;
PFNGLPOPATTRIB pfnglPopAttrib;
PFNGLPUSHCLIENTATTRIB pfnglPushClientAttrib;
PFNGLPOPCLIENTATTRIB pfnglPopClientAttrib;
PFNGLRENDERMODE pfnglRenderMode;
PFNGLGETERROR pfnglGetError;
PFNGLGETSTRING pfnglGetString;
PFNGLFINISH pfnglFinish;
PFNGLFLUSH pfnglFlush;
PFNGLHINT pfnglHint;
PFNGLCLEARDEPTH pfnglClearDepth;
PFNGLDEPTHFUNC pfnglDepthFunc;
PFNGLDEPTHMASK pfnglDepthMask;
PFNGLDEPTHRANGE pfnglDepthRange;
PFNGLCLEARACCUM pfnglClearAccum;
PFNGLACCUM pfnglAccum;
PFNGLMATRIXMODE pfnglMatrixMode;
PFNGLORTHO pfnglOrtho;
PFNGLFRUSTUM pfnglFrustum;
PFNGLVIEWPORT pfnglViewport;
PFNGLPUSHMATRIX pfnglPushMatrix;
PFNGLPOPMATRIX pfnglPopMatrix;
PFNGLLOADIDENTITY pfnglLoadIdentity;
PFNGLLOADMATRIXD pfnglLoadMatrixd;
PFNGLLOADMATRIXF pfnglLoadMatrixf;
PFNGLMULTMATRIXD pfnglMultMatrixd;
PFNGLMULTMATRIXF pfnglMultMatrixf;
PFNGLROTATED pfnglRotated;
PFNGLROTATEF pfnglRotatef;
PFNGLSCALED pfnglScaled;
PFNGLSCALEF pfnglScalef;
PFNGLTRANSLATED pfnglTranslated;
PFNGLTRANSLATEF pfnglTranslatef;
PFNGLISLIST pfnglIsList;
PFNGLDELETELISTS pfnglDeleteLists;
PFNGLGENLISTS pfnglGenLists;
PFNGLNEWLIST pfnglNewList;
PFNGLENDLIST pfnglEndList;
PFNGLCALLLIST pfnglCallList;
PFNGLCALLLISTS pfnglCallLists;
PFNGLLISTBASE pfnglListBase;
PFNGLBEGIN pfnglBegin;
PFNGLEND pfnglEnd;
PFNGLVERTEX2D pfnglVertex2d;
PFNGLVERTEX2F pfnglVertex2f;
PFNGLVERTEX2I pfnglVertex2i;
PFNGLVERTEX2S pfnglVertex2s;
PFNGLVERTEX3D pfnglVertex3d;
PFNGLVERTEX3F pfnglVertex3f;
PFNGLVERTEX3I pfnglVertex3i;
PFNGLVERTEX3S pfnglVertex3s;
PFNGLVERTEX4D pfnglVertex4d;
PFNGLVERTEX4F pfnglVertex4f;
PFNGLVERTEX4I pfnglVertex4i;
PFNGLVERTEX4S pfnglVertex4s;
PFNGLVERTEX2DV pfnglVertex2dv;
PFNGLVERTEX2FV pfnglVertex2fv;
PFNGLVERTEX2IV pfnglVertex2iv;
PFNGLVERTEX2SV pfnglVertex2sv;
PFNGLVERTEX3DV pfnglVertex3dv;
PFNGLVERTEX3FV pfnglVertex3fv;
PFNGLVERTEX3IV pfnglVertex3iv;
PFNGLVERTEX3SV pfnglVertex3sv;
PFNGLVERTEX4DV pfnglVertex4dv;
PFNGLVERTEX4FV pfnglVertex4fv;
PFNGLVERTEX4IV pfnglVertex4iv;
PFNGLVERTEX4SV pfnglVertex4sv;
PFNGLNORMAL3B pfnglNormal3b;
PFNGLNORMAL3D pfnglNormal3d;
PFNGLNORMAL3F pfnglNormal3f;
PFNGLNORMAL3I pfnglNormal3i;
PFNGLNORMAL3S pfnglNormal3s;
PFNGLNORMAL3BV pfnglNormal3bv;
PFNGLNORMAL3DV pfnglNormal3dv;
PFNGLNORMAL3FV pfnglNormal3fv;
PFNGLNORMAL3IV pfnglNormal3iv;
PFNGLNORMAL3SV pfnglNormal3sv;
PFNGLINDEXD pfnglIndexd;
PFNGLINDEXF pfnglIndexf;
PFNGLINDEXI pfnglIndexi;
PFNGLINDEXS pfnglIndexs;
PFNGLINDEXUB pfnglIndexub;
PFNGLINDEXDV pfnglIndexdv;
PFNGLINDEXFV pfnglIndexfv;
PFNGLINDEXIV pfnglIndexiv;
PFNGLINDEXSV pfnglIndexsv;
PFNGLINDEXUBV pfnglIndexubv;
PFNGLCOLOR3B pfnglColor3b;
PFNGLCOLOR3D pfnglColor3d;
PFNGLCOLOR3F pfnglColor3f;
PFNGLCOLOR3I pfnglColor3i;
PFNGLCOLOR3S pfnglColor3s;
PFNGLCOLOR3UB pfnglColor3ub;
PFNGLCOLOR3UI pfnglColor3ui;
PFNGLCOLOR3US pfnglColor3us;
PFNGLCOLOR4B pfnglColor4b;
PFNGLCOLOR4D pfnglColor4d;
PFNGLCOLOR4F pfnglColor4f;
PFNGLCOLOR4I pfnglColor4i;
PFNGLCOLOR4S pfnglColor4s;
PFNGLCOLOR4UB pfnglColor4ub;
PFNGLCOLOR4UI pfnglColor4ui;
PFNGLCOLOR4US pfnglColor4us;
PFNGLCOLOR3BV pfnglColor3bv;
PFNGLCOLOR3DV pfnglColor3dv;
PFNGLCOLOR3FV pfnglColor3fv;
PFNGLCOLOR3IV pfnglColor3iv;
PFNGLCOLOR3SV pfnglColor3sv;
PFNGLCOLOR3UBV pfnglColor3ubv;
PFNGLCOLOR3UIV pfnglColor3uiv;
PFNGLCOLOR3USV pfnglColor3usv;
PFNGLCOLOR4BV pfnglColor4bv;
PFNGLCOLOR4DV pfnglColor4dv;
PFNGLCOLOR4FV pfnglColor4fv;
PFNGLCOLOR4IV pfnglColor4iv;
PFNGLCOLOR4SV pfnglColor4sv;
PFNGLCOLOR4UBV pfnglColor4ubv;
PFNGLCOLOR4UIV pfnglColor4uiv;
PFNGLCOLOR4USV pfnglColor4usv;
PFNGLTEXCOORD1D pfnglTexCoord1d;
PFNGLTEXCOORD1F pfnglTexCoord1f;
PFNGLTEXCOORD1I pfnglTexCoord1i;
PFNGLTEXCOORD1S pfnglTexCoord1s;
PFNGLTEXCOORD2D pfnglTexCoord2d;
PFNGLTEXCOORD2F pfnglTexCoord2f;
PFNGLTEXCOORD2I pfnglTexCoord2i;
PFNGLTEXCOORD2S pfnglTexCoord2s;
PFNGLTEXCOORD3D pfnglTexCoord3d;
PFNGLTEXCOORD3F pfnglTexCoord3f;
PFNGLTEXCOORD3I pfnglTexCoord3i;
PFNGLTEXCOORD3S pfnglTexCoord3s;
PFNGLTEXCOORD4D pfnglTexCoord4d;
PFNGLTEXCOORD4F pfnglTexCoord4f;
PFNGLTEXCOORD4I pfnglTexCoord4i;
PFNGLTEXCOORD4S pfnglTexCoord4s;
PFNGLTEXCOORD1DV pfnglTexCoord1dv;
PFNGLTEXCOORD1FV pfnglTexCoord1fv;
PFNGLTEXCOORD1IV pfnglTexCoord1iv;
PFNGLTEXCOORD1SV pfnglTexCoord1sv;
PFNGLTEXCOORD2DV pfnglTexCoord2dv;
PFNGLTEXCOORD2FV pfnglTexCoord2fv;
PFNGLTEXCOORD2IV pfnglTexCoord2iv;
PFNGLTEXCOORD2SV pfnglTexCoord2sv;
PFNGLTEXCOORD3DV pfnglTexCoord3dv;
PFNGLTEXCOORD3FV pfnglTexCoord3fv;
PFNGLTEXCOORD3IV pfnglTexCoord3iv;
PFNGLTEXCOORD3SV pfnglTexCoord3sv;
PFNGLTEXCOORD4DV pfnglTexCoord4dv;
PFNGLTEXCOORD4FV pfnglTexCoord4fv;
PFNGLTEXCOORD4IV pfnglTexCoord4iv;
PFNGLTEXCOORD4SV pfnglTexCoord4sv;
PFNGLRASTERPOS2D pfnglRasterPos2d;
PFNGLRASTERPOS2F pfnglRasterPos2f;
PFNGLRASTERPOS2I pfnglRasterPos2i;
PFNGLRASTERPOS2S pfnglRasterPos2s;
PFNGLRASTERPOS3D pfnglRasterPos3d;
PFNGLRASTERPOS3F pfnglRasterPos3f;
PFNGLRASTERPOS3I pfnglRasterPos3i;
PFNGLRASTERPOS3S pfnglRasterPos3s;
PFNGLRASTERPOS4D pfnglRasterPos4d;
PFNGLRASTERPOS4F pfnglRasterPos4f;
PFNGLRASTERPOS4I pfnglRasterPos4i;
PFNGLRASTERPOS4S pfnglRasterPos4s;
PFNGLRASTERPOS2DV pfnglRasterPos2dv;
PFNGLRASTERPOS2FV pfnglRasterPos2fv;
PFNGLRASTERPOS2IV pfnglRasterPos2iv;
PFNGLRASTERPOS2SV pfnglRasterPos2sv;
PFNGLRASTERPOS3DV pfnglRasterPos3dv;
PFNGLRASTERPOS3FV pfnglRasterPos3fv;
PFNGLRASTERPOS3IV pfnglRasterPos3iv;
PFNGLRASTERPOS3SV pfnglRasterPos3sv;
PFNGLRASTERPOS4DV pfnglRasterPos4dv;
PFNGLRASTERPOS4FV pfnglRasterPos4fv;
PFNGLRASTERPOS4IV pfnglRasterPos4iv;
PFNGLRASTERPOS4SV pfnglRasterPos4sv;
PFNGLRECTD pfnglRectd;
PFNGLRECTF pfnglRectf;
PFNGLRECTI pfnglRecti;
PFNGLRECTS pfnglRects;
PFNGLRECTDV pfnglRectdv;
PFNGLRECTFV pfnglRectfv;
PFNGLRECTIV pfnglRectiv;
PFNGLRECTSV pfnglRectsv;
PFNGLVERTEXPOINTER pfnglVertexPointer;
PFNGLNORMALPOINTER pfnglNormalPointer;
PFNGLCOLORPOINTER pfnglColorPointer;
PFNGLINDEXPOINTER pfnglIndexPointer;
PFNGLTEXCOORDPOINTER pfnglTexCoordPointer;
PFNGLEDGEFLAGPOINTER pfnglEdgeFlagPointer;
PFNGLGETPOINTERV pfnglGetPointerv;
PFNGLARRAYELEMENT pfnglArrayElement;
PFNGLDRAWARRAYS pfnglDrawArrays;
PFNGLDRAWELEMENTS pfnglDrawElements;
PFNGLINTERLEAVEDARRAYS pfnglInterleavedArrays;
PFNGLSHADEMODEL pfnglShadeModel;
PFNGLLIGHTF pfnglLightf;
PFNGLLIGHTI pfnglLighti;
PFNGLLIGHTFV pfnglLightfv;
PFNGLLIGHTIV pfnglLightiv;
PFNGLGETLIGHTFV pfnglGetLightfv;
PFNGLGETLIGHTIV pfnglGetLightiv;
PFNGLLIGHTMODELF pfnglLightModelf;
PFNGLLIGHTMODELI pfnglLightModeli;
PFNGLLIGHTMODELFV pfnglLightModelfv;
PFNGLLIGHTMODELIV pfnglLightModeliv;
PFNGLMATERIALF pfnglMaterialf;
PFNGLMATERIALI pfnglMateriali;
PFNGLMATERIALFV pfnglMaterialfv;
PFNGLMATERIALIV pfnglMaterialiv;
PFNGLGETMATERIALFV pfnglGetMaterialfv;
PFNGLGETMATERIALIV pfnglGetMaterialiv;
PFNGLCOLORMATERIAL pfnglColorMaterial;
PFNGLPIXELZOOM pfnglPixelZoom;
PFNGLPIXELSTOREF pfnglPixelStoref;
PFNGLPIXELSTOREI pfnglPixelStorei;
PFNGLPIXELTRANSFERF pfnglPixelTransferf;
PFNGLPIXELTRANSFERI pfnglPixelTransferi;
PFNGLPIXELMAPFV pfnglPixelMapfv;
PFNGLPIXELMAPUIV pfnglPixelMapuiv;
PFNGLPIXELMAPUSV pfnglPixelMapusv;
PFNGLGETPIXELMAPFV pfnglGetPixelMapfv;
PFNGLGETPIXELMAPUIV pfnglGetPixelMapuiv;
PFNGLGETPIXELMAPUSV pfnglGetPixelMapusv;
PFNGLBITMAP pfnglBitmap;
PFNGLREADPIXELS pfnglReadPixels;
PFNGLDRAWPIXELS pfnglDrawPixels;
PFNGLCOPYPIXELS pfnglCopyPixels;
PFNGLSTENCILFUNC pfnglStencilFunc;
PFNGLSTENCILMASK pfnglStencilMask;
PFNGLSTENCILOP pfnglStencilOp;
PFNGLCLEARSTENCIL pfnglClearStencil;
PFNGLTEXGEND pfnglTexGend;
PFNGLTEXGENF pfnglTexGenf;
PFNGLTEXGENI pfnglTexGeni;
PFNGLTEXGENDV pfnglTexGendv;
PFNGLTEXGENFV pfnglTexGenfv;
PFNGLTEXGENIV pfnglTexGeniv;
PFNGLGETTEXGENDV pfnglGetTexGendv;
PFNGLGETTEXGENFV pfnglGetTexGenfv;
PFNGLGETTEXGENIV pfnglGetTexGeniv;
PFNGLTEXENVF pfnglTexEnvf;
PFNGLTEXENVI pfnglTexEnvi;
PFNGLTEXENVFV pfnglTexEnvfv;
PFNGLTEXENVIV pfnglTexEnviv;
PFNGLGETTEXENVFV pfnglGetTexEnvfv;
PFNGLGETTEXENVIV pfnglGetTexEnviv;
PFNGLTEXPARAMETERF pfnglTexParameterf;
PFNGLTEXPARAMETERI pfnglTexParameteri;
PFNGLTEXPARAMETERFV pfnglTexParameterfv;
PFNGLTEXPARAMETERIV pfnglTexParameteriv;
PFNGLGETTEXPARAMETERFV pfnglGetTexParameterfv;
PFNGLGETTEXPARAMETERIV pfnglGetTexParameteriv;
PFNGLGETTEXLEVELPARAMETERFV pfnglGetTexLevelParameterfv;
PFNGLGETTEXLEVELPARAMETERIV pfnglGetTexLevelParameteriv;
PFNGLTEXIMAGE1D pfnglTexImage1D;
PFNGLTEXIMAGE2D pfnglTexImage2D;
PFNGLGETTEXIMAGE pfnglGetTexImage;
PFNGLGENTEXTURES pfnglGenTextures;
PFNGLDELETETEXTURES pfnglDeleteTextures;
PFNGLBINDTEXTURE pfnglBindTexture;
PFNGLPRIORITIZETEXTURES pfnglPrioritizeTextures;
PFNGLARETEXTURESRESIDENT pfnglAreTexturesResident;
PFNGLISTEXTURE pfnglIsTexture;
PFNGLTEXSUBIMAGE1D pfnglTexSubImage1D;
PFNGLTEXSUBIMAGE2D pfnglTexSubImage2D;
PFNGLCOPYTEXIMAGE1D pfnglCopyTexImage1D;
PFNGLCOPYTEXIMAGE2D pfnglCopyTexImage2D;
PFNGLCOPYTEXSUBIMAGE1D pfnglCopyTexSubImage1D;
PFNGLCOPYTEXSUBIMAGE2D pfnglCopyTexSubImage2D;
PFNGLMAP1D pfnglMap1d;
PFNGLMAP1F pfnglMap1f;
PFNGLMAP2D pfnglMap2d;
PFNGLMAP2F pfnglMap2f;
PFNGLGETMAPDV pfnglGetMapdv;
PFNGLGETMAPFV pfnglGetMapfv;
PFNGLGETMAPIV pfnglGetMapiv;
PFNGLEVALCOORD1D pfnglEvalCoord1d;
PFNGLEVALCOORD1F pfnglEvalCoord1f;
PFNGLEVALCOORD1DV pfnglEvalCoord1dv;
PFNGLEVALCOORD1FV pfnglEvalCoord1fv;
PFNGLEVALCOORD2D pfnglEvalCoord2d;
PFNGLEVALCOORD2F pfnglEvalCoord2f;
PFNGLEVALCOORD2DV pfnglEvalCoord2dv;
PFNGLEVALCOORD2FV pfnglEvalCoord2fv;
PFNGLMAPGRID1D pfnglMapGrid1d;
PFNGLMAPGRID1F pfnglMapGrid1f;
PFNGLMAPGRID2D pfnglMapGrid2d;
PFNGLMAPGRID2F pfnglMapGrid2f;
PFNGLEVALPOINT1 pfnglEvalPoint1;
PFNGLEVALPOINT2 pfnglEvalPoint2;
PFNGLEVALMESH1 pfnglEvalMesh1;
PFNGLEVALMESH2 pfnglEvalMesh2;
PFNGLFOGF pfnglFogf;
PFNGLFOGI pfnglFogi;
PFNGLFOGFV pfnglFogfv;
PFNGLFOGIV pfnglFogiv;
PFNGLFEEDBACKBUFFER pfnglFeedbackBuffer;
PFNGLPASSTHROUGH pfnglPassThrough;
PFNGLSELECTBUFFER pfnglSelectBuffer;
PFNGLINITNAMES pfnglInitNames;
PFNGLLOADNAME pfnglLoadName;
PFNGLPUSHNAME pfnglPushName;
PFNGLPOPNAME pfnglPopName;

PFNGLACTIVETEXTUREARB pfnglActiveTextureARB;
PFNGLCLIENTACTIVETEXTUREARB pfnglClientActiveTextureARB;
PFNGLMULTITEXCOORD1DARB pfnglMultiTexCoord1dARB;
PFNGLMULTITEXCOORD1DVARB pfnglMultiTexCoord1dvARB;
PFNGLMULTITEXCOORD1FARB pfnglMultiTexCoord1fARB;
PFNGLMULTITEXCOORD1FVARB pfnglMultiTexCoord1fvARB;
PFNGLMULTITEXCOORD1IARB pfnglMultiTexCoord1iARB;
PFNGLMULTITEXCOORD1IVARB pfnglMultiTexCoord1ivARB;
PFNGLMULTITEXCOORD1SARB pfnglMultiTexCoord1sARB;
PFNGLMULTITEXCOORD1SVARB pfnglMultiTexCoord1svARB;
PFNGLMULTITEXCOORD2DARB pfnglMultiTexCoord2dARB;
PFNGLMULTITEXCOORD2DVARB pfnglMultiTexCoord2dvARB;
PFNGLMULTITEXCOORD2FARB pfnglMultiTexCoord2fARB;
PFNGLMULTITEXCOORD2FVARB pfnglMultiTexCoord2fvARB;
PFNGLMULTITEXCOORD2IARB pfnglMultiTexCoord2iARB;
PFNGLMULTITEXCOORD2IVARB pfnglMultiTexCoord2ivARB;
PFNGLMULTITEXCOORD2SARB pfnglMultiTexCoord2sARB;
PFNGLMULTITEXCOORD2SVARB pfnglMultiTexCoord2svARB;
PFNGLMULTITEXCOORD3DARB pfnglMultiTexCoord3dARB;
PFNGLMULTITEXCOORD3DVARB pfnglMultiTexCoord3dvARB;
PFNGLMULTITEXCOORD3FARB pfnglMultiTexCoord3fARB;
PFNGLMULTITEXCOORD3FVARB pfnglMultiTexCoord3fvARB;
PFNGLMULTITEXCOORD3IARB pfnglMultiTexCoord3iARB;
PFNGLMULTITEXCOORD3IVARB pfnglMultiTexCoord3ivARB;
PFNGLMULTITEXCOORD3SARB pfnglMultiTexCoord3sARB;
PFNGLMULTITEXCOORD3SVARB pfnglMultiTexCoord3svARB;
PFNGLMULTITEXCOORD4DARB pfnglMultiTexCoord4dARB;
PFNGLMULTITEXCOORD4DVARB pfnglMultiTexCoord4dvARB;
PFNGLMULTITEXCOORD4FARB pfnglMultiTexCoord4fARB;
PFNGLMULTITEXCOORD4FVARB pfnglMultiTexCoord4fvARB;
PFNGLMULTITEXCOORD4IARB pfnglMultiTexCoord4iARB;
PFNGLMULTITEXCOORD4IVARB pfnglMultiTexCoord4ivARB;
PFNGLMULTITEXCOORD4SARB pfnglMultiTexCoord4sARB;
PFNGLMULTITEXCOORD4SVARB pfnglMultiTexCoord4svARB;
PFNGLPOINTPARAMETERFEXT pfnglPointParameterfEXT;
PFNGLPOINTPARAMETERFVEXT pfnglPointParameterfvEXT;
PFNGLLOCKARRAYSEXT pfnglLockArraysEXT;
PFNGLUNLOCKARRAYSEXT pfnglUnlockArraysEXT;
PFNGLBINDBUFFERARB pfnglBindBufferARB;
PFNGLDELETEBUFFERSARB pfnglDeleteBuffersARB;
PFNGLGENBUFFERSARB pfnglGenBuffersARB;
PFNGLISBUFFERARB pfnglIsBufferARB;
PFNGLBUFFERDATAARB pfnglBufferDataARB;
PFNGLBUFFERSUBDATAARB pfnglBufferSubDataARB;
PFNGLGETBUFFERSUBDATAARB pfnglGetBufferSubDataARB;
PFNGLMAPBUFFERARB pfnglMapBufferARB;
PFNGLUNMAPBUFFERARB pfnglUnmapBufferARB;
PFNGLGETBUFFERPARAMETERIVARB pfnglGetBufferParameterivARB;
PFNGLGETBUFFERPOINTERVARB pfnglGetBufferPointervARB;

// =============================================================================
// Initialization functions

void GL_Shutdown()
{
	Sys_GLCloseLibrary();

	pfnglClearIndex = NULL;
	pfnglClearColor = NULL;
	pfnglClear = NULL;
	pfnglIndexMask = NULL;
	pfnglColorMask = NULL;
	pfnglAlphaFunc = NULL;
	pfnglBlendFunc = NULL;
	pfnglLogicOp = NULL;
	pfnglCullFace = NULL;
	pfnglFrontFace = NULL;
	pfnglPointSize = NULL;
	pfnglLineWidth = NULL;
	pfnglLineStipple = NULL;
	pfnglPolygonMode = NULL;
	pfnglPolygonOffset = NULL;
	pfnglPolygonStipple = NULL;
	pfnglGetPolygonStipple = NULL;
	pfnglEdgeFlag = NULL;
	pfnglEdgeFlagv = NULL;
	pfnglScissor = NULL;
	pfnglClipPlane = NULL;
	pfnglGetClipPlane = NULL;
	pfnglDrawBuffer = NULL;
	pfnglReadBuffer = NULL;
	pfnglEnable = NULL;
	pfnglDisable = NULL;
	pfnglIsEnabled = NULL;
	pfnglEnableClientState = NULL;
	pfnglDisableClientState = NULL;
	pfnglGetBooleanv = NULL;
	pfnglGetDoublev = NULL;
	pfnglGetFloatv = NULL;
	pfnglGetIntegerv = NULL;
	pfnglPushAttrib = NULL;
	pfnglPopAttrib = NULL;
	pfnglPushClientAttrib = NULL;
	pfnglPopClientAttrib = NULL;
	pfnglRenderMode = NULL;
	pfnglGetError = NULL;
	pfnglGetString = NULL;
	pfnglFinish = NULL;
	pfnglFlush = NULL;
	pfnglHint = NULL;
	pfnglClearDepth = NULL;
	pfnglDepthFunc = NULL;
	pfnglDepthMask = NULL;
	pfnglDepthRange = NULL;
	pfnglClearAccum = NULL;
	pfnglAccum = NULL;
	pfnglMatrixMode = NULL;
	pfnglOrtho = NULL;
	pfnglFrustum = NULL;
	pfnglViewport = NULL;
	pfnglPushMatrix = NULL;
	pfnglPopMatrix = NULL;
	pfnglLoadIdentity = NULL;
	pfnglLoadMatrixd = NULL;
	pfnglLoadMatrixf = NULL;
	pfnglMultMatrixd = NULL;
	pfnglMultMatrixf = NULL;
	pfnglRotated = NULL;
	pfnglRotatef = NULL;
	pfnglScaled = NULL;
	pfnglScalef = NULL;
	pfnglTranslated = NULL;
	pfnglTranslatef = NULL;
	pfnglIsList = NULL;
	pfnglDeleteLists = NULL;
	pfnglGenLists = NULL;
	pfnglNewList = NULL;
	pfnglEndList = NULL;
	pfnglCallList = NULL;
	pfnglCallLists = NULL;
	pfnglListBase = NULL;
	pfnglBegin = NULL;
	pfnglEnd = NULL;
	pfnglVertex2d = NULL;
	pfnglVertex2f = NULL;
	pfnglVertex2i = NULL;
	pfnglVertex2s = NULL;
	pfnglVertex3d = NULL;
	pfnglVertex3f = NULL;
	pfnglVertex3i = NULL;
	pfnglVertex3s = NULL;
	pfnglVertex4d = NULL;
	pfnglVertex4f = NULL;
	pfnglVertex4i = NULL;
	pfnglVertex4s = NULL;
	pfnglVertex2dv = NULL;
	pfnglVertex2fv = NULL;
	pfnglVertex2iv = NULL;
	pfnglVertex2sv = NULL;
	pfnglVertex3dv = NULL;
	pfnglVertex3fv = NULL;
	pfnglVertex3iv = NULL;
	pfnglVertex3sv = NULL;
	pfnglVertex4dv = NULL;
	pfnglVertex4fv = NULL;
	pfnglVertex4iv = NULL;
	pfnglVertex4sv = NULL;
	pfnglNormal3b = NULL;
	pfnglNormal3d = NULL;
	pfnglNormal3f = NULL;
	pfnglNormal3i = NULL;
	pfnglNormal3s = NULL;
	pfnglNormal3bv = NULL;
	pfnglNormal3dv = NULL;
	pfnglNormal3fv = NULL;
	pfnglNormal3iv = NULL;
	pfnglNormal3sv = NULL;
	pfnglIndexd = NULL;
	pfnglIndexf = NULL;
	pfnglIndexi = NULL;
	pfnglIndexs = NULL;
	pfnglIndexub = NULL;
	pfnglIndexdv = NULL;
	pfnglIndexfv = NULL;
	pfnglIndexiv = NULL;
	pfnglIndexsv = NULL;
	pfnglIndexubv = NULL;
	pfnglColor3b = NULL;
	pfnglColor3d = NULL;
	pfnglColor3f = NULL;
	pfnglColor3i = NULL;
	pfnglColor3s = NULL;
	pfnglColor3ub = NULL;
	pfnglColor3ui = NULL;
	pfnglColor3us = NULL;
	pfnglColor4b = NULL;
	pfnglColor4d = NULL;
	pfnglColor4f = NULL;
	pfnglColor4i = NULL;
	pfnglColor4s = NULL;
	pfnglColor4ub = NULL;
	pfnglColor4ui = NULL;
	pfnglColor4us = NULL;
	pfnglColor3bv = NULL;
	pfnglColor3dv = NULL;
	pfnglColor3fv = NULL;
	pfnglColor3iv = NULL;
	pfnglColor3sv = NULL;
	pfnglColor3ubv = NULL;
	pfnglColor3uiv = NULL;
	pfnglColor3usv = NULL;
	pfnglColor4bv = NULL;
	pfnglColor4dv = NULL;
	pfnglColor4fv = NULL;
	pfnglColor4iv = NULL;
	pfnglColor4sv = NULL;
	pfnglColor4ubv = NULL;
	pfnglColor4uiv = NULL;
	pfnglColor4usv = NULL;
	pfnglTexCoord1d = NULL;
	pfnglTexCoord1f = NULL;
	pfnglTexCoord1i = NULL;
	pfnglTexCoord1s = NULL;
	pfnglTexCoord2d = NULL;
	pfnglTexCoord2f = NULL;
	pfnglTexCoord2i = NULL;
	pfnglTexCoord2s = NULL;
	pfnglTexCoord3d = NULL;
	pfnglTexCoord3f = NULL;
	pfnglTexCoord3i = NULL;
	pfnglTexCoord3s = NULL;
	pfnglTexCoord4d = NULL;
	pfnglTexCoord4f = NULL;
	pfnglTexCoord4i = NULL;
	pfnglTexCoord4s = NULL;
	pfnglTexCoord1dv = NULL;
	pfnglTexCoord1fv = NULL;
	pfnglTexCoord1iv = NULL;
	pfnglTexCoord1sv = NULL;
	pfnglTexCoord2dv = NULL;
	pfnglTexCoord2fv = NULL;
	pfnglTexCoord2iv = NULL;
	pfnglTexCoord2sv = NULL;
	pfnglTexCoord3dv = NULL;
	pfnglTexCoord3fv = NULL;
	pfnglTexCoord3iv = NULL;
	pfnglTexCoord3sv = NULL;
	pfnglTexCoord4dv = NULL;
	pfnglTexCoord4fv = NULL;
	pfnglTexCoord4iv = NULL;
	pfnglTexCoord4sv = NULL;
	pfnglRasterPos2d = NULL;
	pfnglRasterPos2f = NULL;
	pfnglRasterPos2i = NULL;
	pfnglRasterPos2s = NULL;
	pfnglRasterPos3d = NULL;
	pfnglRasterPos3f = NULL;
	pfnglRasterPos3i = NULL;
	pfnglRasterPos3s = NULL;
	pfnglRasterPos4d = NULL;
	pfnglRasterPos4f = NULL;
	pfnglRasterPos4i = NULL;
	pfnglRasterPos4s = NULL;
	pfnglRasterPos2dv = NULL;
	pfnglRasterPos2fv = NULL;
	pfnglRasterPos2iv = NULL;
	pfnglRasterPos2sv = NULL;
	pfnglRasterPos3dv = NULL;
	pfnglRasterPos3fv = NULL;
	pfnglRasterPos3iv = NULL;
	pfnglRasterPos3sv = NULL;
	pfnglRasterPos4dv = NULL;
	pfnglRasterPos4fv = NULL;
	pfnglRasterPos4iv = NULL;
	pfnglRasterPos4sv = NULL;
	pfnglRectd = NULL;
	pfnglRectf = NULL;
	pfnglRecti = NULL;
	pfnglRects = NULL;
	pfnglRectdv = NULL;
	pfnglRectfv = NULL;
	pfnglRectiv = NULL;
	pfnglRectsv = NULL;
	pfnglVertexPointer = NULL;
	pfnglNormalPointer = NULL;
	pfnglColorPointer = NULL;
	pfnglIndexPointer = NULL;
	pfnglTexCoordPointer = NULL;
	pfnglEdgeFlagPointer = NULL;
	pfnglGetPointerv = NULL;
	pfnglArrayElement = NULL;
	pfnglDrawArrays = NULL;
	pfnglDrawElements = NULL;
	pfnglInterleavedArrays = NULL;
	pfnglShadeModel = NULL;
	pfnglLightf = NULL;
	pfnglLighti = NULL;
	pfnglLightfv = NULL;
	pfnglLightiv = NULL;
	pfnglGetLightfv = NULL;
	pfnglGetLightiv = NULL;
	pfnglLightModelf = NULL;
	pfnglLightModeli = NULL;
	pfnglLightModelfv = NULL;
	pfnglLightModeliv = NULL;
	pfnglMaterialf = NULL;
	pfnglMateriali = NULL;
	pfnglMaterialfv = NULL;
	pfnglMaterialiv = NULL;
	pfnglGetMaterialfv = NULL;
	pfnglGetMaterialiv = NULL;
	pfnglColorMaterial = NULL;
	pfnglPixelZoom = NULL;
	pfnglPixelStoref = NULL;
	pfnglPixelStorei = NULL;
	pfnglPixelTransferf = NULL;
	pfnglPixelTransferi = NULL;
	pfnglPixelMapfv = NULL;
	pfnglPixelMapuiv = NULL;
	pfnglPixelMapusv = NULL;
	pfnglGetPixelMapfv = NULL;
	pfnglGetPixelMapuiv = NULL;
	pfnglGetPixelMapusv = NULL;
	pfnglBitmap = NULL;
	pfnglReadPixels = NULL;
	pfnglDrawPixels = NULL;
	pfnglCopyPixels = NULL;
	pfnglStencilFunc = NULL;
	pfnglStencilMask = NULL;
	pfnglStencilOp = NULL;
	pfnglClearStencil = NULL;
	pfnglTexGend = NULL;
	pfnglTexGenf = NULL;
	pfnglTexGeni = NULL;
	pfnglTexGendv = NULL;
	pfnglTexGenfv = NULL;
	pfnglTexGeniv = NULL;
	pfnglGetTexGendv = NULL;
	pfnglGetTexGenfv = NULL;
	pfnglGetTexGeniv = NULL;
	pfnglTexEnvf = NULL;
	pfnglTexEnvi = NULL;
	pfnglTexEnvfv = NULL;
	pfnglTexEnviv = NULL;
	pfnglGetTexEnvfv = NULL;
	pfnglGetTexEnviv = NULL;
	pfnglTexParameterf = NULL;
	pfnglTexParameteri = NULL;
	pfnglTexParameterfv = NULL;
	pfnglTexParameteriv = NULL;
	pfnglGetTexParameterfv = NULL;
	pfnglGetTexParameteriv = NULL;
	pfnglGetTexLevelParameterfv = NULL;
	pfnglGetTexLevelParameteriv = NULL;
	pfnglTexImage1D = NULL;
	pfnglTexImage2D = NULL;
	pfnglGetTexImage = NULL;
	pfnglGenTextures = NULL;
	pfnglDeleteTextures = NULL;
	pfnglBindTexture = NULL;
	pfnglPrioritizeTextures = NULL;
	pfnglAreTexturesResident = NULL;
	pfnglIsTexture = NULL;
	pfnglTexSubImage1D = NULL;
	pfnglTexSubImage2D = NULL;
	pfnglCopyTexImage1D = NULL;
	pfnglCopyTexImage2D = NULL;
	pfnglCopyTexSubImage1D = NULL;
	pfnglCopyTexSubImage2D = NULL;
	pfnglMap1d = NULL;
	pfnglMap1f = NULL;
	pfnglMap2d = NULL;
	pfnglMap2f = NULL;
	pfnglGetMapdv = NULL;
	pfnglGetMapfv = NULL;
	pfnglGetMapiv = NULL;
	pfnglEvalCoord1d = NULL;
	pfnglEvalCoord1f = NULL;
	pfnglEvalCoord1dv = NULL;
	pfnglEvalCoord1fv = NULL;
	pfnglEvalCoord2d = NULL;
	pfnglEvalCoord2f = NULL;
	pfnglEvalCoord2dv = NULL;
	pfnglEvalCoord2fv = NULL;
	pfnglMapGrid1d = NULL;
	pfnglMapGrid1f = NULL;
	pfnglMapGrid2d = NULL;
	pfnglMapGrid2f = NULL;
	pfnglEvalPoint1 = NULL;
	pfnglEvalPoint2 = NULL;
	pfnglEvalMesh1 = NULL;
	pfnglEvalMesh2 = NULL;
	pfnglFogf = NULL;
	pfnglFogi = NULL;
	pfnglFogfv = NULL;
	pfnglFogiv = NULL;
	pfnglFeedbackBuffer = NULL;
	pfnglPassThrough = NULL;
	pfnglSelectBuffer = NULL;
	pfnglInitNames = NULL;
	pfnglLoadName = NULL;
	pfnglPushName = NULL;
	pfnglPopName = NULL;

	pfnglActiveTextureARB = NULL;
	pfnglClientActiveTextureARB = NULL;
	pfnglMultiTexCoord1dARB = NULL;
	pfnglMultiTexCoord1dvARB = NULL;
	pfnglMultiTexCoord1fARB = NULL;
	pfnglMultiTexCoord1fvARB = NULL;
	pfnglMultiTexCoord1iARB = NULL;
	pfnglMultiTexCoord1ivARB = NULL;
	pfnglMultiTexCoord1sARB = NULL;
	pfnglMultiTexCoord1svARB = NULL;
	pfnglMultiTexCoord2dARB = NULL;
	pfnglMultiTexCoord2dvARB = NULL;
	pfnglMultiTexCoord2fARB = NULL;
	pfnglMultiTexCoord2fvARB = NULL;
	pfnglMultiTexCoord2iARB = NULL;
	pfnglMultiTexCoord2ivARB = NULL;
	pfnglMultiTexCoord2sARB = NULL;
	pfnglMultiTexCoord2svARB = NULL;
	pfnglMultiTexCoord3dARB = NULL;
	pfnglMultiTexCoord3dvARB = NULL;
	pfnglMultiTexCoord3fARB = NULL;
	pfnglMultiTexCoord3fvARB = NULL;
	pfnglMultiTexCoord3iARB = NULL;
	pfnglMultiTexCoord3ivARB = NULL;
	pfnglMultiTexCoord3sARB = NULL;
	pfnglMultiTexCoord3svARB = NULL;
	pfnglMultiTexCoord4dARB = NULL;
	pfnglMultiTexCoord4dvARB = NULL;
	pfnglMultiTexCoord4fARB = NULL;
	pfnglMultiTexCoord4fvARB = NULL;
	pfnglMultiTexCoord4iARB = NULL;
	pfnglMultiTexCoord4ivARB = NULL;
	pfnglMultiTexCoord4sARB = NULL;
	pfnglMultiTexCoord4svARB = NULL;
	pfnglPointParameterfEXT = NULL;
	pfnglPointParameterfvEXT = NULL;
	pfnglLockArraysEXT = NULL;
	pfnglUnlockArraysEXT = NULL;
}

bool GL_Initialize(const char* libname)
{
	if (Sys_GLOpenLibrary(libname) == false)
	{
		printf("No OpenGL libraries could be loaded, the program will now exit.\n");
		return false;
	}

	pfnglClearIndex = (PFNGLCLEARINDEX) Sys_GLGetProc ("glClearIndex");
	pfnglClearColor = (PFNGLCLEARCOLOR) Sys_GLGetProc ("glClearColor");
	pfnglClear = (PFNGLCLEAR) Sys_GLGetProc ("glClear");
	pfnglIndexMask = (PFNGLINDEXMASK) Sys_GLGetProc ("glIndexMask");
	pfnglColorMask = (PFNGLCOLORMASK) Sys_GLGetProc ("glColorMask");
	pfnglAlphaFunc = (PFNGLALPHAFUNC) Sys_GLGetProc ("glAlphaFunc");
	pfnglBlendFunc = (PFNGLBLENDFUNC) Sys_GLGetProc ("glBlendFunc");
	pfnglLogicOp = (PFNGLLOGICOP) Sys_GLGetProc ("glLogicOp");
	pfnglCullFace = (PFNGLCULLFACE) Sys_GLGetProc ("glCullFace");
	pfnglFrontFace = (PFNGLFRONTFACE) Sys_GLGetProc ("glFrontFace");
	pfnglPointSize = (PFNGLPOINTSIZE) Sys_GLGetProc ("glPointSize");
	pfnglLineWidth = (PFNGLLINEWIDTH) Sys_GLGetProc ("glLineWidth");
	pfnglLineStipple = (PFNGLLINESTIPPLE) Sys_GLGetProc ("glLineStipple");
	pfnglPolygonMode = (PFNGLPOLYGONMODE) Sys_GLGetProc ("glPolygonMode");
	pfnglPolygonOffset = (PFNGLPOLYGONOFFSET) Sys_GLGetProc ("glPolygonOffset");
	pfnglPolygonStipple = (PFNGLPOLYGONSTIPPLE) Sys_GLGetProc ("glPolygonStipple");
	pfnglGetPolygonStipple = (PFNGLGETPOLYGONSTIPPLE) Sys_GLGetProc ("glGetPolygonStipple");
	pfnglEdgeFlag = (PFNGLEDGEFLAG) Sys_GLGetProc ("glEdgeFlag");
	pfnglEdgeFlagv = (PFNGLEDGEFLAGV) Sys_GLGetProc ("glEdgeFlagv");
	pfnglScissor = (PFNGLSCISSOR) Sys_GLGetProc ("glScissor");
	pfnglClipPlane = (PFNGLCLIPPLANE) Sys_GLGetProc ("glClipPlane");
	pfnglGetClipPlane = (PFNGLGETCLIPPLANE) Sys_GLGetProc ("glGetClipPlane");
	pfnglDrawBuffer = (PFNGLDRAWBUFFER) Sys_GLGetProc ("glDrawBuffer");
	pfnglReadBuffer = (PFNGLREADBUFFER) Sys_GLGetProc ("glReadBuffer");
	pfnglEnable = (PFNGLENABLE) Sys_GLGetProc ("glEnable");
	pfnglDisable = (PFNGLDISABLE) Sys_GLGetProc ("glDisable");
	pfnglIsEnabled = (PFNGLISENABLED) Sys_GLGetProc ("glIsEnabled");
	pfnglEnableClientState = (PFNGLENABLECLIENTSTATE) Sys_GLGetProc ("glEnableClientState");
	pfnglDisableClientState = (PFNGLDISABLECLIENTSTATE) Sys_GLGetProc ("glDisableClientState");
	pfnglGetBooleanv = (PFNGLGETBOOLEANV) Sys_GLGetProc ("glGetBooleanv");
	pfnglGetDoublev = (PFNGLGETDOUBLEV) Sys_GLGetProc ("glGetDoublev");
	pfnglGetFloatv = (PFNGLGETFLOATV) Sys_GLGetProc ("glGetFloatv");
	pfnglGetIntegerv = (PFNGLGETINTEGERV) Sys_GLGetProc ("glGetIntegerv");
	pfnglPushAttrib = (PFNGLPUSHATTRIB) Sys_GLGetProc ("glPushAttrib");
	pfnglPopAttrib = (PFNGLPOPATTRIB) Sys_GLGetProc ("glPopAttrib");
	pfnglPushClientAttrib = (PFNGLPUSHCLIENTATTRIB) Sys_GLGetProc ("glPushClientAttrib");
	pfnglPopClientAttrib = (PFNGLPOPCLIENTATTRIB) Sys_GLGetProc ("glPopClientAttrib");
	pfnglRenderMode = (PFNGLRENDERMODE) Sys_GLGetProc ("glRenderMode");
	pfnglGetError = (PFNGLGETERROR) Sys_GLGetProc ("glGetError");
	pfnglGetString = (PFNGLGETSTRING) Sys_GLGetProc ("glGetString");
	pfnglFinish = (PFNGLFINISH) Sys_GLGetProc ("glFinish");
	pfnglFlush = (PFNGLFLUSH) Sys_GLGetProc ("glFlush");
	pfnglHint = (PFNGLHINT) Sys_GLGetProc ("glHint");
	pfnglClearDepth = (PFNGLCLEARDEPTH) Sys_GLGetProc ("glClearDepth");
	pfnglDepthFunc = (PFNGLDEPTHFUNC) Sys_GLGetProc ("glDepthFunc");
	pfnglDepthMask = (PFNGLDEPTHMASK) Sys_GLGetProc ("glDepthMask");
	pfnglDepthRange = (PFNGLDEPTHRANGE) Sys_GLGetProc ("glDepthRange");
	pfnglClearAccum = (PFNGLCLEARACCUM) Sys_GLGetProc ("glClearAccum");
	pfnglAccum = (PFNGLACCUM) Sys_GLGetProc ("glAccum");
	pfnglMatrixMode = (PFNGLMATRIXMODE) Sys_GLGetProc ("glMatrixMode");
	pfnglOrtho = (PFNGLORTHO) Sys_GLGetProc ("glOrtho");
	pfnglFrustum = (PFNGLFRUSTUM) Sys_GLGetProc ("glFrustum");
	pfnglViewport = (PFNGLVIEWPORT) Sys_GLGetProc ("glViewport");
	pfnglPushMatrix = (PFNGLPUSHMATRIX) Sys_GLGetProc ("glPushMatrix");
	pfnglPopMatrix = (PFNGLPOPMATRIX) Sys_GLGetProc ("glPopMatrix");
	pfnglLoadIdentity = (PFNGLLOADIDENTITY) Sys_GLGetProc ("glLoadIdentity");
	pfnglLoadMatrixd = (PFNGLLOADMATRIXD) Sys_GLGetProc ("glLoadMatrixd");
	pfnglLoadMatrixf = (PFNGLLOADMATRIXF) Sys_GLGetProc ("glLoadMatrixf");
	pfnglMultMatrixd = (PFNGLMULTMATRIXD) Sys_GLGetProc ("glMultMatrixd");
	pfnglMultMatrixf = (PFNGLMULTMATRIXF) Sys_GLGetProc ("glMultMatrixf");
	pfnglRotated = (PFNGLROTATED) Sys_GLGetProc ("glRotated");
	pfnglRotatef = (PFNGLROTATEF) Sys_GLGetProc ("glRotatef");
	pfnglScaled = (PFNGLSCALED) Sys_GLGetProc ("glScaled");
	pfnglScalef = (PFNGLSCALEF) Sys_GLGetProc ("glScalef");
	pfnglTranslated = (PFNGLTRANSLATED) Sys_GLGetProc ("glTranslated");
	pfnglTranslatef = (PFNGLTRANSLATEF) Sys_GLGetProc ("glTranslatef");
	pfnglIsList = (PFNGLISLIST) Sys_GLGetProc ("glIsList");
	pfnglDeleteLists = (PFNGLDELETELISTS) Sys_GLGetProc ("glDeleteLists");
	pfnglGenLists = (PFNGLGENLISTS) Sys_GLGetProc ("glGenLists");
	pfnglNewList = (PFNGLNEWLIST) Sys_GLGetProc ("glNewList");
	pfnglEndList = (PFNGLENDLIST) Sys_GLGetProc ("glEndList");
	pfnglCallList = (PFNGLCALLLIST) Sys_GLGetProc ("glCallList");
	pfnglCallLists = (PFNGLCALLLISTS) Sys_GLGetProc ("glCallLists");
	pfnglListBase = (PFNGLLISTBASE) Sys_GLGetProc ("glListBase");
	pfnglBegin = (PFNGLBEGIN) Sys_GLGetProc ("glBegin");
	pfnglEnd = (PFNGLEND) Sys_GLGetProc ("glEnd");
	pfnglVertex2d = (PFNGLVERTEX2D) Sys_GLGetProc ("glVertex2d");
	pfnglVertex2f = (PFNGLVERTEX2F) Sys_GLGetProc ("glVertex2f");
	pfnglVertex2i = (PFNGLVERTEX2I) Sys_GLGetProc ("glVertex2i");
	pfnglVertex2s = (PFNGLVERTEX2S) Sys_GLGetProc ("glVertex2s");
	pfnglVertex3d = (PFNGLVERTEX3D) Sys_GLGetProc ("glVertex3d");
	pfnglVertex3f = (PFNGLVERTEX3F) Sys_GLGetProc ("glVertex3f");
	pfnglVertex3i = (PFNGLVERTEX3I) Sys_GLGetProc ("glVertex3i");
	pfnglVertex3s = (PFNGLVERTEX3S) Sys_GLGetProc ("glVertex3s");
	pfnglVertex4d = (PFNGLVERTEX4D) Sys_GLGetProc ("glVertex4d");
	pfnglVertex4f = (PFNGLVERTEX4F) Sys_GLGetProc ("glVertex4f");
	pfnglVertex4i = (PFNGLVERTEX4I) Sys_GLGetProc ("glVertex4i");
	pfnglVertex4s = (PFNGLVERTEX4S) Sys_GLGetProc ("glVertex4s");
	pfnglVertex2dv = (PFNGLVERTEX2DV) Sys_GLGetProc ("glVertex2dv");
	pfnglVertex2fv = (PFNGLVERTEX2FV) Sys_GLGetProc ("glVertex2fv");
	pfnglVertex2iv = (PFNGLVERTEX2IV) Sys_GLGetProc ("glVertex2iv");
	pfnglVertex2sv = (PFNGLVERTEX2SV) Sys_GLGetProc ("glVertex2sv");
	pfnglVertex3dv = (PFNGLVERTEX3DV) Sys_GLGetProc ("glVertex3dv");
	pfnglVertex3fv = (PFNGLVERTEX3FV) Sys_GLGetProc ("glVertex3fv");
	pfnglVertex3iv = (PFNGLVERTEX3IV) Sys_GLGetProc ("glVertex3iv");
	pfnglVertex3sv = (PFNGLVERTEX3SV) Sys_GLGetProc ("glVertex3sv");
	pfnglVertex4dv = (PFNGLVERTEX4DV) Sys_GLGetProc ("glVertex4dv");
	pfnglVertex4fv = (PFNGLVERTEX4FV) Sys_GLGetProc ("glVertex4fv");
	pfnglVertex4iv = (PFNGLVERTEX4IV) Sys_GLGetProc ("glVertex4iv");
	pfnglVertex4sv = (PFNGLVERTEX4SV) Sys_GLGetProc ("glVertex4sv");
	pfnglNormal3b = (PFNGLNORMAL3B) Sys_GLGetProc ("glNormal3b");
	pfnglNormal3d = (PFNGLNORMAL3D) Sys_GLGetProc ("glNormal3d");
	pfnglNormal3f = (PFNGLNORMAL3F) Sys_GLGetProc ("glNormal3f");
	pfnglNormal3i = (PFNGLNORMAL3I) Sys_GLGetProc ("glNormal3i");
	pfnglNormal3s = (PFNGLNORMAL3S) Sys_GLGetProc ("glNormal3s");
	pfnglNormal3bv = (PFNGLNORMAL3BV) Sys_GLGetProc ("glNormal3bv");
	pfnglNormal3dv = (PFNGLNORMAL3DV) Sys_GLGetProc ("glNormal3dv");
	pfnglNormal3fv = (PFNGLNORMAL3FV) Sys_GLGetProc ("glNormal3fv");
	pfnglNormal3iv = (PFNGLNORMAL3IV) Sys_GLGetProc ("glNormal3iv");
	pfnglNormal3sv = (PFNGLNORMAL3SV) Sys_GLGetProc ("glNormal3sv");
	pfnglIndexd = (PFNGLINDEXD) Sys_GLGetProc ("glIndexd");
	pfnglIndexf = (PFNGLINDEXF) Sys_GLGetProc ("glIndexf");
	pfnglIndexi = (PFNGLINDEXI) Sys_GLGetProc ("glIndexi");
	pfnglIndexs = (PFNGLINDEXS) Sys_GLGetProc ("glIndexs");
	pfnglIndexub = (PFNGLINDEXUB) Sys_GLGetProc ("glIndexub");
	pfnglIndexdv = (PFNGLINDEXDV) Sys_GLGetProc ("glIndexdv");
	pfnglIndexfv = (PFNGLINDEXFV) Sys_GLGetProc ("glIndexfv");
	pfnglIndexiv = (PFNGLINDEXIV) Sys_GLGetProc ("glIndexiv");
	pfnglIndexsv = (PFNGLINDEXSV) Sys_GLGetProc ("glIndexsv");
	pfnglIndexubv = (PFNGLINDEXUBV) Sys_GLGetProc ("glIndexubv");
	pfnglColor3b = (PFNGLCOLOR3B) Sys_GLGetProc ("glColor3b");
	pfnglColor3d = (PFNGLCOLOR3D) Sys_GLGetProc ("glColor3d");
	pfnglColor3f = (PFNGLCOLOR3F) Sys_GLGetProc ("glColor3f");
	pfnglColor3i = (PFNGLCOLOR3I) Sys_GLGetProc ("glColor3i");
	pfnglColor3s = (PFNGLCOLOR3S) Sys_GLGetProc ("glColor3s");
	pfnglColor3ub = (PFNGLCOLOR3UB) Sys_GLGetProc ("glColor3ub");
	pfnglColor3ui = (PFNGLCOLOR3UI) Sys_GLGetProc ("glColor3ui");
	pfnglColor3us = (PFNGLCOLOR3US) Sys_GLGetProc ("glColor3us");
	pfnglColor4b = (PFNGLCOLOR4B) Sys_GLGetProc ("glColor4b");
	pfnglColor4d = (PFNGLCOLOR4D) Sys_GLGetProc ("glColor4d");
	pfnglColor4f = (PFNGLCOLOR4F) Sys_GLGetProc ("glColor4f");
	pfnglColor4i = (PFNGLCOLOR4I) Sys_GLGetProc ("glColor4i");
	pfnglColor4s = (PFNGLCOLOR4S) Sys_GLGetProc ("glColor4s");
	pfnglColor4ub = (PFNGLCOLOR4UB) Sys_GLGetProc ("glColor4ub");
	pfnglColor4ui = (PFNGLCOLOR4UI) Sys_GLGetProc ("glColor4ui");
	pfnglColor4us = (PFNGLCOLOR4US) Sys_GLGetProc ("glColor4us");
	pfnglColor3bv = (PFNGLCOLOR3BV) Sys_GLGetProc ("glColor3bv");
	pfnglColor3dv = (PFNGLCOLOR3DV) Sys_GLGetProc ("glColor3dv");
	pfnglColor3fv = (PFNGLCOLOR3FV) Sys_GLGetProc ("glColor3fv");
	pfnglColor3iv = (PFNGLCOLOR3IV) Sys_GLGetProc ("glColor3iv");
	pfnglColor3sv = (PFNGLCOLOR3SV) Sys_GLGetProc ("glColor3sv");
	pfnglColor3ubv = (PFNGLCOLOR3UBV) Sys_GLGetProc ("glColor3ubv");
	pfnglColor3uiv = (PFNGLCOLOR3UIV) Sys_GLGetProc ("glColor3uiv");
	pfnglColor3usv = (PFNGLCOLOR3USV) Sys_GLGetProc ("glColor3usv");
	pfnglColor4bv = (PFNGLCOLOR4BV) Sys_GLGetProc ("glColor4bv");
	pfnglColor4dv = (PFNGLCOLOR4DV) Sys_GLGetProc ("glColor4dv");
	pfnglColor4fv = (PFNGLCOLOR4FV) Sys_GLGetProc ("glColor4fv");
	pfnglColor4iv = (PFNGLCOLOR4IV) Sys_GLGetProc ("glColor4iv");
	pfnglColor4sv = (PFNGLCOLOR4SV) Sys_GLGetProc ("glColor4sv");
	pfnglColor4ubv = (PFNGLCOLOR4UBV) Sys_GLGetProc ("glColor4ubv");
	pfnglColor4uiv = (PFNGLCOLOR4UIV) Sys_GLGetProc ("glColor4uiv");
	pfnglColor4usv = (PFNGLCOLOR4USV) Sys_GLGetProc ("glColor4usv");
	pfnglTexCoord1d = (PFNGLTEXCOORD1D) Sys_GLGetProc ("glTexCoord1d");
	pfnglTexCoord1f = (PFNGLTEXCOORD1F) Sys_GLGetProc ("glTexCoord1f");
	pfnglTexCoord1i = (PFNGLTEXCOORD1I) Sys_GLGetProc ("glTexCoord1i");
	pfnglTexCoord1s = (PFNGLTEXCOORD1S) Sys_GLGetProc ("glTexCoord1s");
	pfnglTexCoord2d = (PFNGLTEXCOORD2D) Sys_GLGetProc ("glTexCoord2d");
	pfnglTexCoord2f = (PFNGLTEXCOORD2F) Sys_GLGetProc ("glTexCoord2f");
	pfnglTexCoord2i = (PFNGLTEXCOORD2I) Sys_GLGetProc ("glTexCoord2i");
	pfnglTexCoord2s = (PFNGLTEXCOORD2S) Sys_GLGetProc ("glTexCoord2s");
	pfnglTexCoord3d = (PFNGLTEXCOORD3D) Sys_GLGetProc ("glTexCoord3d");
	pfnglTexCoord3f = (PFNGLTEXCOORD3F) Sys_GLGetProc ("glTexCoord3f");
	pfnglTexCoord3i = (PFNGLTEXCOORD3I) Sys_GLGetProc ("glTexCoord3i");
	pfnglTexCoord3s = (PFNGLTEXCOORD3S) Sys_GLGetProc ("glTexCoord3s");
	pfnglTexCoord4d = (PFNGLTEXCOORD4D) Sys_GLGetProc ("glTexCoord4d");
	pfnglTexCoord4f = (PFNGLTEXCOORD4F) Sys_GLGetProc ("glTexCoord4f");
	pfnglTexCoord4i = (PFNGLTEXCOORD4I) Sys_GLGetProc ("glTexCoord4i");
	pfnglTexCoord4s = (PFNGLTEXCOORD4S) Sys_GLGetProc ("glTexCoord4s");
	pfnglTexCoord1dv = (PFNGLTEXCOORD1DV) Sys_GLGetProc ("glTexCoord1dv");
	pfnglTexCoord1fv = (PFNGLTEXCOORD1FV) Sys_GLGetProc ("glTexCoord1fv");
	pfnglTexCoord1iv = (PFNGLTEXCOORD1IV) Sys_GLGetProc ("glTexCoord1iv");
	pfnglTexCoord1sv = (PFNGLTEXCOORD1SV) Sys_GLGetProc ("glTexCoord1sv");
	pfnglTexCoord2dv = (PFNGLTEXCOORD2DV) Sys_GLGetProc ("glTexCoord2dv");
	pfnglTexCoord2fv = (PFNGLTEXCOORD2FV) Sys_GLGetProc ("glTexCoord2fv");
	pfnglTexCoord2iv = (PFNGLTEXCOORD2IV) Sys_GLGetProc ("glTexCoord2iv");
	pfnglTexCoord2sv = (PFNGLTEXCOORD2SV) Sys_GLGetProc ("glTexCoord2sv");
	pfnglTexCoord3dv = (PFNGLTEXCOORD3DV) Sys_GLGetProc ("glTexCoord3dv");
	pfnglTexCoord3fv = (PFNGLTEXCOORD3FV) Sys_GLGetProc ("glTexCoord3fv");
	pfnglTexCoord3iv = (PFNGLTEXCOORD3IV) Sys_GLGetProc ("glTexCoord3iv");
	pfnglTexCoord3sv = (PFNGLTEXCOORD3SV) Sys_GLGetProc ("glTexCoord3sv");
	pfnglTexCoord4dv = (PFNGLTEXCOORD4DV) Sys_GLGetProc ("glTexCoord4dv");
	pfnglTexCoord4fv = (PFNGLTEXCOORD4FV) Sys_GLGetProc ("glTexCoord4fv");
	pfnglTexCoord4iv = (PFNGLTEXCOORD4IV) Sys_GLGetProc ("glTexCoord4iv");
	pfnglTexCoord4sv = (PFNGLTEXCOORD4SV) Sys_GLGetProc ("glTexCoord4sv");
	pfnglRasterPos2d = (PFNGLRASTERPOS2D) Sys_GLGetProc ("glRasterPos2d");
	pfnglRasterPos2f = (PFNGLRASTERPOS2F) Sys_GLGetProc ("glRasterPos2f");
	pfnglRasterPos2i = (PFNGLRASTERPOS2I) Sys_GLGetProc ("glRasterPos2i");
	pfnglRasterPos2s = (PFNGLRASTERPOS2S) Sys_GLGetProc ("glRasterPos2s");
	pfnglRasterPos3d = (PFNGLRASTERPOS3D) Sys_GLGetProc ("glRasterPos3d");
	pfnglRasterPos3f = (PFNGLRASTERPOS3F) Sys_GLGetProc ("glRasterPos3f");
	pfnglRasterPos3i = (PFNGLRASTERPOS3I) Sys_GLGetProc ("glRasterPos3i");
	pfnglRasterPos3s = (PFNGLRASTERPOS3S) Sys_GLGetProc ("glRasterPos3s");
	pfnglRasterPos4d = (PFNGLRASTERPOS4D) Sys_GLGetProc ("glRasterPos4d");
	pfnglRasterPos4f = (PFNGLRASTERPOS4F) Sys_GLGetProc ("glRasterPos4f");
	pfnglRasterPos4i = (PFNGLRASTERPOS4I) Sys_GLGetProc ("glRasterPos4i");
	pfnglRasterPos4s = (PFNGLRASTERPOS4S) Sys_GLGetProc ("glRasterPos4s");
	pfnglRasterPos2dv = (PFNGLRASTERPOS2DV) Sys_GLGetProc ("glRasterPos2dv");
	pfnglRasterPos2fv = (PFNGLRASTERPOS2FV) Sys_GLGetProc ("glRasterPos2fv");
	pfnglRasterPos2iv = (PFNGLRASTERPOS2IV) Sys_GLGetProc ("glRasterPos2iv");
	pfnglRasterPos2sv = (PFNGLRASTERPOS2SV) Sys_GLGetProc ("glRasterPos2sv");
	pfnglRasterPos3dv = (PFNGLRASTERPOS3DV) Sys_GLGetProc ("glRasterPos3dv");
	pfnglRasterPos3fv = (PFNGLRASTERPOS3FV) Sys_GLGetProc ("glRasterPos3fv");
	pfnglRasterPos3iv = (PFNGLRASTERPOS3IV) Sys_GLGetProc ("glRasterPos3iv");
	pfnglRasterPos3sv = (PFNGLRASTERPOS3SV) Sys_GLGetProc ("glRasterPos3sv");
	pfnglRasterPos4dv = (PFNGLRASTERPOS4DV) Sys_GLGetProc ("glRasterPos4dv");
	pfnglRasterPos4fv = (PFNGLRASTERPOS4FV) Sys_GLGetProc ("glRasterPos4fv");
	pfnglRasterPos4iv = (PFNGLRASTERPOS4IV) Sys_GLGetProc ("glRasterPos4iv");
	pfnglRasterPos4sv = (PFNGLRASTERPOS4SV) Sys_GLGetProc ("glRasterPos4sv");
	pfnglRectd = (PFNGLRECTD) Sys_GLGetProc ("glRectd");
	pfnglRectf = (PFNGLRECTF) Sys_GLGetProc ("glRectf");
	pfnglRecti = (PFNGLRECTI) Sys_GLGetProc ("glRecti");
	pfnglRects = (PFNGLRECTS) Sys_GLGetProc ("glRects");
	pfnglRectdv = (PFNGLRECTDV) Sys_GLGetProc ("glRectdv");
	pfnglRectfv = (PFNGLRECTFV) Sys_GLGetProc ("glRectfv");
	pfnglRectiv = (PFNGLRECTIV) Sys_GLGetProc ("glRectiv");
	pfnglRectsv = (PFNGLRECTSV) Sys_GLGetProc ("glRectsv");
	pfnglVertexPointer = (PFNGLVERTEXPOINTER) Sys_GLGetProc ("glVertexPointer");
	pfnglNormalPointer = (PFNGLNORMALPOINTER) Sys_GLGetProc ("glNormalPointer");
	pfnglColorPointer = (PFNGLCOLORPOINTER) Sys_GLGetProc ("glColorPointer");
	pfnglIndexPointer = (PFNGLINDEXPOINTER) Sys_GLGetProc ("glIndexPointer");
	pfnglTexCoordPointer = (PFNGLTEXCOORDPOINTER) Sys_GLGetProc ("glTexCoordPointer");
	pfnglEdgeFlagPointer = (PFNGLEDGEFLAGPOINTER) Sys_GLGetProc ("glEdgeFlagPointer");
	pfnglGetPointerv = (PFNGLGETPOINTERV) Sys_GLGetProc ("glGetPointerv");
	pfnglArrayElement = (PFNGLARRAYELEMENT) Sys_GLGetProc ("glArrayElement");
	pfnglDrawArrays = (PFNGLDRAWARRAYS) Sys_GLGetProc ("glDrawArrays");
	pfnglDrawElements = (PFNGLDRAWELEMENTS) Sys_GLGetProc ("glDrawElements");
	pfnglInterleavedArrays = (PFNGLINTERLEAVEDARRAYS) Sys_GLGetProc ("glInterleavedArrays");
	pfnglShadeModel = (PFNGLSHADEMODEL) Sys_GLGetProc ("glShadeModel");
	pfnglLightf = (PFNGLLIGHTF) Sys_GLGetProc ("glLightf");
	pfnglLighti = (PFNGLLIGHTI) Sys_GLGetProc ("glLighti");
	pfnglLightfv = (PFNGLLIGHTFV) Sys_GLGetProc ("glLightfv");
	pfnglLightiv = (PFNGLLIGHTIV) Sys_GLGetProc ("glLightiv");
	pfnglGetLightfv = (PFNGLGETLIGHTFV) Sys_GLGetProc ("glGetLightfv");
	pfnglGetLightiv = (PFNGLGETLIGHTIV) Sys_GLGetProc ("glGetLightiv");
	pfnglLightModelf = (PFNGLLIGHTMODELF) Sys_GLGetProc ("glLightModelf");
	pfnglLightModeli = (PFNGLLIGHTMODELI) Sys_GLGetProc ("glLightModeli");
	pfnglLightModelfv = (PFNGLLIGHTMODELFV) Sys_GLGetProc ("glLightModelfv");
	pfnglLightModeliv = (PFNGLLIGHTMODELIV) Sys_GLGetProc ("glLightModeliv");
	pfnglMaterialf = (PFNGLMATERIALF) Sys_GLGetProc ("glMaterialf");
	pfnglMateriali = (PFNGLMATERIALI) Sys_GLGetProc ("glMateriali");
	pfnglMaterialfv = (PFNGLMATERIALFV) Sys_GLGetProc ("glMaterialfv");
	pfnglMaterialiv = (PFNGLMATERIALIV) Sys_GLGetProc ("glMaterialiv");
	pfnglGetMaterialfv = (PFNGLGETMATERIALFV) Sys_GLGetProc ("glGetMaterialfv");
	pfnglGetMaterialiv = (PFNGLGETMATERIALIV) Sys_GLGetProc ("glGetMaterialiv");
	pfnglColorMaterial = (PFNGLCOLORMATERIAL) Sys_GLGetProc ("glColorMaterial");
	pfnglPixelZoom = (PFNGLPIXELZOOM) Sys_GLGetProc ("glPixelZoom");
	pfnglPixelStoref = (PFNGLPIXELSTOREF) Sys_GLGetProc ("glPixelStoref");
	pfnglPixelStorei = (PFNGLPIXELSTOREI) Sys_GLGetProc ("glPixelStorei");
	pfnglPixelTransferf = (PFNGLPIXELTRANSFERF) Sys_GLGetProc ("glPixelTransferf");
	pfnglPixelTransferi = (PFNGLPIXELTRANSFERI) Sys_GLGetProc ("glPixelTransferi");
	pfnglPixelMapfv = (PFNGLPIXELMAPFV) Sys_GLGetProc ("glPixelMapfv");
	pfnglPixelMapuiv = (PFNGLPIXELMAPUIV) Sys_GLGetProc ("glPixelMapuiv");
	pfnglPixelMapusv = (PFNGLPIXELMAPUSV) Sys_GLGetProc ("glPixelMapusv");
	pfnglGetPixelMapfv = (PFNGLGETPIXELMAPFV) Sys_GLGetProc ("glGetPixelMapfv");
	pfnglGetPixelMapuiv = (PFNGLGETPIXELMAPUIV) Sys_GLGetProc ("glGetPixelMapuiv");
	pfnglGetPixelMapusv = (PFNGLGETPIXELMAPUSV) Sys_GLGetProc ("glGetPixelMapusv");
	pfnglBitmap = (PFNGLBITMAP) Sys_GLGetProc ("glBitmap");
	pfnglReadPixels = (PFNGLREADPIXELS) Sys_GLGetProc ("glReadPixels");
	pfnglDrawPixels = (PFNGLDRAWPIXELS) Sys_GLGetProc ("glDrawPixels");
	pfnglCopyPixels = (PFNGLCOPYPIXELS) Sys_GLGetProc ("glCopyPixels");
	pfnglStencilFunc = (PFNGLSTENCILFUNC) Sys_GLGetProc ("glStencilFunc");
	pfnglStencilMask = (PFNGLSTENCILMASK) Sys_GLGetProc ("glStencilMask");
	pfnglStencilOp = (PFNGLSTENCILOP) Sys_GLGetProc ("glStencilOp");
	pfnglClearStencil = (PFNGLCLEARSTENCIL) Sys_GLGetProc ("glClearStencil");
	pfnglTexGend = (PFNGLTEXGEND) Sys_GLGetProc ("glTexGend");
	pfnglTexGenf = (PFNGLTEXGENF) Sys_GLGetProc ("glTexGenf");
	pfnglTexGeni = (PFNGLTEXGENI) Sys_GLGetProc ("glTexGeni");
	pfnglTexGendv = (PFNGLTEXGENDV) Sys_GLGetProc ("glTexGendv");
	pfnglTexGenfv = (PFNGLTEXGENFV) Sys_GLGetProc ("glTexGenfv");
	pfnglTexGeniv = (PFNGLTEXGENIV) Sys_GLGetProc ("glTexGeniv");
	pfnglGetTexGendv = (PFNGLGETTEXGENDV) Sys_GLGetProc ("glGetTexGendv");
	pfnglGetTexGenfv = (PFNGLGETTEXGENFV) Sys_GLGetProc ("glGetTexGenfv");
	pfnglGetTexGeniv = (PFNGLGETTEXGENIV) Sys_GLGetProc ("glGetTexGeniv");
	pfnglTexEnvf = (PFNGLTEXENVF) Sys_GLGetProc ("glTexEnvf");
	pfnglTexEnvi = (PFNGLTEXENVI) Sys_GLGetProc ("glTexEnvi");
	pfnglTexEnvfv = (PFNGLTEXENVFV) Sys_GLGetProc ("glTexEnvfv");
	pfnglTexEnviv = (PFNGLTEXENVIV) Sys_GLGetProc ("glTexEnviv");
	pfnglGetTexEnvfv = (PFNGLGETTEXENVFV) Sys_GLGetProc ("glGetTexEnvfv");
	pfnglGetTexEnviv = (PFNGLGETTEXENVIV) Sys_GLGetProc ("glGetTexEnviv");
	pfnglTexParameterf = (PFNGLTEXPARAMETERF) Sys_GLGetProc ("glTexParameterf");
	pfnglTexParameteri = (PFNGLTEXPARAMETERI) Sys_GLGetProc ("glTexParameteri");
	pfnglTexParameterfv = (PFNGLTEXPARAMETERFV) Sys_GLGetProc ("glTexParameterfv");
	pfnglTexParameteriv = (PFNGLTEXPARAMETERIV) Sys_GLGetProc ("glTexParameteriv");
	pfnglGetTexParameterfv = (PFNGLGETTEXPARAMETERFV) Sys_GLGetProc ("glGetTexParameterfv");
	pfnglGetTexParameteriv = (PFNGLGETTEXPARAMETERIV) Sys_GLGetProc ("glGetTexParameteriv");
	pfnglGetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFV) Sys_GLGetProc ("glGetTexLevelParameterfv");
	pfnglGetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIV) Sys_GLGetProc ("glGetTexLevelParameteriv");
	pfnglTexImage1D = (PFNGLTEXIMAGE1D) Sys_GLGetProc ("glTexImage1D");
	pfnglTexImage2D = (PFNGLTEXIMAGE2D) Sys_GLGetProc ("glTexImage2D");
	pfnglGetTexImage = (PFNGLGETTEXIMAGE) Sys_GLGetProc ("glGetTexImage");
	pfnglGenTextures = (PFNGLGENTEXTURES) Sys_GLGetProc ("glGenTextures");
	pfnglDeleteTextures = (PFNGLDELETETEXTURES) Sys_GLGetProc ("glDeleteTextures");
	pfnglBindTexture = (PFNGLBINDTEXTURE) Sys_GLGetProc ("glBindTexture");
	pfnglPrioritizeTextures = (PFNGLPRIORITIZETEXTURES) Sys_GLGetProc ("glPrioritizeTextures");
	pfnglAreTexturesResident = (PFNGLARETEXTURESRESIDENT) Sys_GLGetProc ("glAreTexturesResident");
	pfnglIsTexture = (PFNGLISTEXTURE) Sys_GLGetProc ("glIsTexture");
	pfnglTexSubImage1D = (PFNGLTEXSUBIMAGE1D) Sys_GLGetProc ("glTexSubImage1D");
	pfnglTexSubImage2D = (PFNGLTEXSUBIMAGE2D) Sys_GLGetProc ("glTexSubImage2D");
	pfnglCopyTexImage1D = (PFNGLCOPYTEXIMAGE1D) Sys_GLGetProc ("glCopyTexImage1D");
	pfnglCopyTexImage2D = (PFNGLCOPYTEXIMAGE2D) Sys_GLGetProc ("glCopyTexImage2D");
	pfnglCopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1D) Sys_GLGetProc ("glCopyTexSubImage1D");
	pfnglCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2D) Sys_GLGetProc ("glCopyTexSubImage2D");
	pfnglMap1d = (PFNGLMAP1D) Sys_GLGetProc ("glMap1d");
	pfnglMap1f = (PFNGLMAP1F) Sys_GLGetProc ("glMap1f");
	pfnglMap2d = (PFNGLMAP2D) Sys_GLGetProc ("glMap2d");
	pfnglMap2f = (PFNGLMAP2F) Sys_GLGetProc ("glMap2f");
	pfnglGetMapdv = (PFNGLGETMAPDV) Sys_GLGetProc ("glGetMapdv");
	pfnglGetMapfv = (PFNGLGETMAPFV) Sys_GLGetProc ("glGetMapfv");
	pfnglGetMapiv = (PFNGLGETMAPIV) Sys_GLGetProc ("glGetMapiv");
	pfnglEvalCoord1d = (PFNGLEVALCOORD1D) Sys_GLGetProc ("glEvalCoord1d");
	pfnglEvalCoord1f = (PFNGLEVALCOORD1F) Sys_GLGetProc ("glEvalCoord1f");
	pfnglEvalCoord1dv = (PFNGLEVALCOORD1DV) Sys_GLGetProc ("glEvalCoord1dv");
	pfnglEvalCoord1fv = (PFNGLEVALCOORD1FV) Sys_GLGetProc ("glEvalCoord1fv");
	pfnglEvalCoord2d = (PFNGLEVALCOORD2D) Sys_GLGetProc ("glEvalCoord2d");
	pfnglEvalCoord2f = (PFNGLEVALCOORD2F) Sys_GLGetProc ("glEvalCoord2f");
	pfnglEvalCoord2dv = (PFNGLEVALCOORD2DV) Sys_GLGetProc ("glEvalCoord2dv");
	pfnglEvalCoord2fv = (PFNGLEVALCOORD2FV) Sys_GLGetProc ("glEvalCoord2fv");
	pfnglMapGrid1d = (PFNGLMAPGRID1D) Sys_GLGetProc ("glMapGrid1d");
	pfnglMapGrid1f = (PFNGLMAPGRID1F) Sys_GLGetProc ("glMapGrid1f");
	pfnglMapGrid2d = (PFNGLMAPGRID2D) Sys_GLGetProc ("glMapGrid2d");
	pfnglMapGrid2f = (PFNGLMAPGRID2F) Sys_GLGetProc ("glMapGrid2f");
	pfnglEvalPoint1 = (PFNGLEVALPOINT1) Sys_GLGetProc ("glEvalPoint1");
	pfnglEvalPoint2 = (PFNGLEVALPOINT2) Sys_GLGetProc ("glEvalPoint2");
	pfnglEvalMesh1 = (PFNGLEVALMESH1) Sys_GLGetProc ("glEvalMesh1");
	pfnglEvalMesh2 = (PFNGLEVALMESH2) Sys_GLGetProc ("glEvalMesh2");
	pfnglFogf = (PFNGLFOGF) Sys_GLGetProc ("glFogf");
	pfnglFogi = (PFNGLFOGI) Sys_GLGetProc ("glFogi");
	pfnglFogfv = (PFNGLFOGFV) Sys_GLGetProc ("glFogfv");
	pfnglFogiv = (PFNGLFOGIV) Sys_GLGetProc ("glFogiv");
	pfnglFeedbackBuffer = (PFNGLFEEDBACKBUFFER) Sys_GLGetProc ("glFeedbackBuffer");
	pfnglPassThrough = (PFNGLPASSTHROUGH) Sys_GLGetProc ("glPassThrough");
	pfnglSelectBuffer = (PFNGLSELECTBUFFER) Sys_GLGetProc ("glSelectBuffer");
	pfnglInitNames = (PFNGLINITNAMES) Sys_GLGetProc ("glInitNames");
	pfnglLoadName = (PFNGLLOADNAME) Sys_GLGetProc ("glLoadName");
	pfnglPushName = (PFNGLPUSHNAME) Sys_GLGetProc ("glPushName");
	pfnglPopName = (PFNGLPOPNAME) Sys_GLGetProc ("glPopName");

	return true;
}

// =============================================================================
// Extensions support

GLint GL_MultiTextures = 1;
bool  GL_CompiledVertexArrays = false;
bool  GL_ClampToEdge = false;
bool  GL_PointParameters = false;
bool  GL_VertexBufferObject = false;

static bool GL_ExtensionSupported (const char *extension)
{
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	// Extension names should not have spaces.
	where = (GLubyte*) strchr(extension, ' ');
	if (where || *extension == '\0')
		return false;

	extensions = glGetString(GL_EXTENSIONS);

	if (!extensions)
		return false;

	// It takes a bit of care to be fool-proof about parsing the
	// OpenGL extensions string. Don't be fooled by sub-strings, etc.
	for (start = extensions; ;)
	{
		where = (GLubyte*)strstr((const char*)start, extension);
		if (!where)
			break;

		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return true;

		start = terminator;
	}

	return false;
}

// Extensions can only be initialized if there's a current OpenGL context.
bool GL_InitializeExtensions ()
{
	if (GL_ExtensionSupported ("GL_ARB_multitexture"))
	{
		pfnglActiveTextureARB = (PFNGLACTIVETEXTUREARB) Sys_GLGetExtension ("glActiveTextureARB");
		pfnglClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARB) Sys_GLGetExtension ("glClientActiveTextureARB");
		pfnglMultiTexCoord1dARB = (PFNGLMULTITEXCOORD1DARB) Sys_GLGetExtension ("glMultiTexCoord1dARB");
		pfnglMultiTexCoord1dvARB = (PFNGLMULTITEXCOORD1DVARB) Sys_GLGetExtension ("glMultiTexCoord1dvARB");
		pfnglMultiTexCoord1fARB = (PFNGLMULTITEXCOORD1FARB) Sys_GLGetExtension ("glMultiTexCoord1fARB");
		pfnglMultiTexCoord1fvARB = (PFNGLMULTITEXCOORD1FVARB) Sys_GLGetExtension ("glMultiTexCoord1fvARB");
		pfnglMultiTexCoord1iARB = (PFNGLMULTITEXCOORD1IARB) Sys_GLGetExtension ("glMultiTexCoord1iARB");
		pfnglMultiTexCoord1ivARB = (PFNGLMULTITEXCOORD1IVARB) Sys_GLGetExtension ("glMultiTexCoord1ivARB");
		pfnglMultiTexCoord1sARB = (PFNGLMULTITEXCOORD1SARB) Sys_GLGetExtension ("glMultiTexCoord1sARB");
		pfnglMultiTexCoord1svARB = (PFNGLMULTITEXCOORD1SVARB) Sys_GLGetExtension ("glMultiTexCoord1svARB");
		pfnglMultiTexCoord2dARB = (PFNGLMULTITEXCOORD2DARB) Sys_GLGetExtension ("glMultiTexCoord2dARB");
		pfnglMultiTexCoord2dvARB = (PFNGLMULTITEXCOORD2DVARB) Sys_GLGetExtension ("glMultiTexCoord2dvARB");
		pfnglMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARB) Sys_GLGetExtension ("glMultiTexCoord2fARB");
		pfnglMultiTexCoord2fvARB = (PFNGLMULTITEXCOORD2FVARB) Sys_GLGetExtension ("glMultiTexCoord2fvARB");
		pfnglMultiTexCoord2iARB = (PFNGLMULTITEXCOORD2IARB) Sys_GLGetExtension ("glMultiTexCoord2iARB");
		pfnglMultiTexCoord2ivARB = (PFNGLMULTITEXCOORD2IVARB) Sys_GLGetExtension ("glMultiTexCoord2ivARB");
		pfnglMultiTexCoord2sARB = (PFNGLMULTITEXCOORD2SARB) Sys_GLGetExtension ("glMultiTexCoord2sARB");
		pfnglMultiTexCoord2svARB = (PFNGLMULTITEXCOORD2SVARB) Sys_GLGetExtension ("glMultiTexCoord2svARB");
		pfnglMultiTexCoord3dARB = (PFNGLMULTITEXCOORD3DARB) Sys_GLGetExtension ("glMultiTexCoord3dARB");
		pfnglMultiTexCoord3dvARB = (PFNGLMULTITEXCOORD3DVARB) Sys_GLGetExtension ("glMultiTexCoord3dvARB");
		pfnglMultiTexCoord3fARB = (PFNGLMULTITEXCOORD3FARB) Sys_GLGetExtension ("glMultiTexCoord3fARB");
		pfnglMultiTexCoord3fvARB = (PFNGLMULTITEXCOORD3FVARB) Sys_GLGetExtension ("glMultiTexCoord3fvARB");
		pfnglMultiTexCoord3iARB = (PFNGLMULTITEXCOORD3IARB) Sys_GLGetExtension ("glMultiTexCoord3iARB");
		pfnglMultiTexCoord3ivARB = (PFNGLMULTITEXCOORD3IVARB) Sys_GLGetExtension ("glMultiTexCoord3ivARB");
		pfnglMultiTexCoord3sARB = (PFNGLMULTITEXCOORD3SARB) Sys_GLGetExtension ("glMultiTexCoord3sARB");
		pfnglMultiTexCoord3svARB = (PFNGLMULTITEXCOORD3SVARB) Sys_GLGetExtension ("glMultiTexCoord3svARB");
		pfnglMultiTexCoord4dARB = (PFNGLMULTITEXCOORD4DARB) Sys_GLGetExtension ("glMultiTexCoord4dARB");
		pfnglMultiTexCoord4dvARB = (PFNGLMULTITEXCOORD4DVARB) Sys_GLGetExtension ("glMultiTexCoord4dvARB");
		pfnglMultiTexCoord4fARB = (PFNGLMULTITEXCOORD4FARB) Sys_GLGetExtension ("glMultiTexCoord4fARB");
		pfnglMultiTexCoord4fvARB = (PFNGLMULTITEXCOORD4FVARB) Sys_GLGetExtension ("glMultiTexCoord4fvARB");
		pfnglMultiTexCoord4iARB = (PFNGLMULTITEXCOORD4IARB) Sys_GLGetExtension ("glMultiTexCoord4iARB");
		pfnglMultiTexCoord4ivARB = (PFNGLMULTITEXCOORD4IVARB) Sys_GLGetExtension ("glMultiTexCoord4ivARB");
		pfnglMultiTexCoord4sARB = (PFNGLMULTITEXCOORD4SARB) Sys_GLGetExtension ("glMultiTexCoord4sARB");
		pfnglMultiTexCoord4svARB = (PFNGLMULTITEXCOORD4SVARB) Sys_GLGetExtension ("glMultiTexCoord4svARB");
		glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &GL_MultiTextures);
	}

	if (GL_ExtensionSupported ("GL_EXT_point_parameters"))
	{
		pfnglPointParameterfEXT = (PFNGLPOINTPARAMETERFEXT) Sys_GLGetExtension ("glPointParameterfEXT");
		pfnglPointParameterfvEXT = (PFNGLPOINTPARAMETERFVEXT) Sys_GLGetExtension ("glPointParameterfvEXT");
		GL_PointParameters = true;
	}

	if (GL_ExtensionSupported ("GL_EXT_compiled_vertex_array"))
	{
		pfnglLockArraysEXT = (PFNGLLOCKARRAYSEXT) Sys_GLGetExtension ("glLockArraysEXT");
		pfnglUnlockArraysEXT = (PFNGLUNLOCKARRAYSEXT) Sys_GLGetExtension ("glUnlockArraysEXT");
		GL_CompiledVertexArrays = true;
	}

	if (GL_ExtensionSupported ("GL_EXT_texture_edge_clamp"))
	{
		GL_ClampToEdge = true;
	}

	if (GL_ExtensionSupported("GL_ARB_vertex_buffer_object"))
	{
		pfnglBindBufferARB = (PFNGLBINDBUFFERARB)Sys_GLGetExtension("glBindBufferARB");
		pfnglDeleteBuffersARB = (PFNGLDELETEBUFFERSARB)Sys_GLGetExtension("glDeleteBuffersARB");
		pfnglGenBuffersARB = (PFNGLGENBUFFERSARB)Sys_GLGetExtension("glGenBuffersARB");
		pfnglIsBufferARB = (PFNGLISBUFFERARB)Sys_GLGetExtension("glIsBufferARB");
		pfnglBufferDataARB = (PFNGLBUFFERDATAARB)Sys_GLGetExtension("glBufferDataARB");
		pfnglBufferSubDataARB = (PFNGLBUFFERSUBDATAARB)Sys_GLGetExtension("glBufferSubDataARB");
		pfnglGetBufferSubDataARB = (PFNGLGETBUFFERSUBDATAARB)Sys_GLGetExtension("glGetBufferSubDataARB");
		pfnglMapBufferARB = (PFNGLMAPBUFFERARB)Sys_GLGetExtension("glMapBufferARB");
		pfnglUnmapBufferARB = (PFNGLUNMAPBUFFERARB)Sys_GLGetExtension("glUnmapBufferARB");
		pfnglGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARB)Sys_GLGetExtension("glGetBufferParameterivARB");
		pfnglGetBufferPointervARB = (PFNGLGETBUFFERPOINTERVARB)Sys_GLGetExtension("glGetBufferPointervARB");
		GL_VertexBufferObject = true;
	}

	return true;
}

// =============================================================================
// GLU functions
// Temporary, should be replaced with better versions in the code

#include <math.h>

#ifndef M_PI
#define M_PI  3.14159265
#endif

void gluLookAt(GLdouble ex, GLdouble ey, GLdouble ez, GLdouble cx, GLdouble cy, GLdouble cz,
               GLdouble ux, GLdouble uy, GLdouble uz)
{
	GLdouble x[3], y[3], z[3] = { ex-cx, ey-cy, ez-cz };
	GLdouble inv;

	inv = sqrt (z[0]*z[0] + z[1]*z[1] + z[2]*z[2]);
	if (inv)
	{
		inv = 1.0/inv;
		z[0] *= inv;
		z[1] *= inv;
		z[2] *= inv;
	}

	x[0] =  uy*z[2] - uz*z[1];
	x[1] = -ux*z[2] + uz*z[0];
	x[2] =  ux*z[1] - uy*z[0];

	y[0] =  z[1]*x[2] - z[2]*x[1];
	y[1] = -z[0]*x[2] + z[2]*x[0];
	y[2] =  z[0]*x[1] - z[1]*x[0];

	inv = sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
	if (inv)
	{
		x[0] *= inv;
		x[1] *= inv;
		x[2] *= inv;
	}

	inv = sqrt(y[0]*y[0] + y[1]*y[1] + y[2]*y[2]);
	if (inv)
	{
		y[0] *= inv;
		y[1] *= inv;
		y[2] *= inv;
	}

	{
		GLdouble m[16] = { x[0], y[0], z[0], 0, x[1], y[1], z[1], 0, x[2], y[2], z[2], 0, 0, 0, 0, 1 };
		glMultMatrixd(m);
		glTranslated(-ex, -ey, -ez);
	}
}

void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GLdouble y = zNear * tan (fovy * M_PI / 360.0);
	glFrustum (-y*aspect, y*aspect, -y, y, zNear, zFar);
}

/*
 * Transform a point (column vector) by a 4x4 matrix.  I.e.  out = m * in
 * Input:  m - the 4x4 matrix
 *         in - the 4x1 vector
 * Output:  out - the resulting 4x1 vector.
 */
static void transform_point(GLdouble out[4], const GLdouble m[16], const GLdouble in[4])
{
#define M(row,col)  m[col*4+row]
	out[0] = M(0,0) * in[0] + M(0,1) * in[1] + M(0,2) * in[2] + M(0,3) * in[3];
	out[1] = M(1,0) * in[0] + M(1,1) * in[1] + M(1,2) * in[2] + M(1,3) * in[3];
	out[2] = M(2,0) * in[0] + M(2,1) * in[1] + M(2,2) * in[2] + M(2,3) * in[3];
	out[3] = M(3,0) * in[0] + M(3,1) * in[1] + M(3,2) * in[2] + M(3,3) * in[3];
#undef M
}




/*
 * Perform a 4x4 matrix multiplication  (product = a x b).
 * Input:  a, b - matrices to multiply
 * Output:  product - product of a and b
 */
static void matmul(GLdouble *product, const GLdouble *a, const GLdouble *b)
{
	/* This matmul was contributed by Thomas Malik */
	GLdouble temp[16];
	GLint i;

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define T(row,col)  temp[(col<<2)+row]

	/* i-te Zeile */
	for (i = 0; i < 4; i++)
	{
		T(i, 0) = A(i, 0) * B(0, 0) + A(i, 1) * B(1, 0) + A(i, 2) * B(2, 0) + A(i, 3) * B(3, 0);
		T(i, 1) = A(i, 0) * B(0, 1) + A(i, 1) * B(1, 1) + A(i, 2) * B(2, 1) + A(i, 3) * B(3, 1);
		T(i, 2) = A(i, 0) * B(0, 2) + A(i, 1) * B(1, 2) + A(i, 2) * B(2, 2) + A(i, 3) * B(3, 2);
		T(i, 3) = A(i, 0) * B(0, 3) + A(i, 1) * B(1, 3) + A(i, 2) * B(2, 3) + A(i, 3) * B(3, 3);
	}

#undef A
#undef B
#undef T
	memcpy( product, temp, 16*sizeof(GLdouble) );
}



/*
 * Compute inverse of 4x4 transformation matrix.
 * Code contributed by Jacques Leroy jle@star.be
 * Return GL_TRUE for success, GL_FALSE for failure (singular matrix)
 */
static GLboolean invert_matrix(const GLdouble *m, GLdouble *out)
{
/* NB. OpenGL Matrices are COLUMN major. */
#define SWAP_ROWS(a, b) { GLdouble *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]

	GLdouble wtmp[4][8];
	GLdouble m0, m1, m2, m3, s;
	GLdouble *r0, *r1, *r2, *r3;

	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

	r0[0] = MAT(m,0,0), r0[1] = MAT(m,0,1),
		r0[2] = MAT(m,0,2), r0[3] = MAT(m,0,3),
		r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

		r1[0] = MAT(m,1,0), r1[1] = MAT(m,1,1),
		r1[2] = MAT(m,1,2), r1[3] = MAT(m,1,3),
		r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

		r2[0] = MAT(m,2,0), r2[1] = MAT(m,2,1),
		r2[2] = MAT(m,2,2), r2[3] = MAT(m,2,3),
		r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

		r3[0] = MAT(m,3,0), r3[1] = MAT(m,3,1),
		r3[2] = MAT(m,3,2), r3[3] = MAT(m,3,3),
		r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

	/* choose pivot - or die */
	if (fabs(r3[0])>fabs(r2[0])) SWAP_ROWS(r3, r2);
	if (fabs(r2[0])>fabs(r1[0])) SWAP_ROWS(r2, r1);
	if (fabs(r1[0])>fabs(r0[0])) SWAP_ROWS(r1, r0);
	if (0.0 == r0[0])  return GL_FALSE;

	/* eliminate first variable     */
	m1 = r1[0]/r0[0]; m2 = r2[0]/r0[0]; m3 = r3[0]/r0[0];
	s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
	s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
	s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
	s = r0[5];
	if (s != 0.0) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
	s = r0[6];
	if (s != 0.0) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
	s = r0[7];
	if (s != 0.0) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }

	/* choose pivot - or die */
	if (fabs(r3[1])>fabs(r2[1])) SWAP_ROWS(r3, r2);
	if (fabs(r2[1])>fabs(r1[1])) SWAP_ROWS(r2, r1);
	if (0.0 == r1[1])  return GL_FALSE;

	/* eliminate second variable */
	m2 = r2[1]/r1[1]; m3 = r3[1]/r1[1];
	r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
	s = r1[4]; if (0.0 != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
	s = r1[5]; if (0.0 != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
	s = r1[6]; if (0.0 != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
	s = r1[7]; if (0.0 != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }

	/* choose pivot - or die */
	if (fabs(r3[2])>fabs(r2[2])) SWAP_ROWS(r3, r2);
	if (0.0 == r2[2])  return GL_FALSE;

	/* eliminate third variable */
	m3 = r3[2]/r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
		r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
		r3[7] -= m3 * r2[7];

	/* last check */
	if (0.0 == r3[3]) return GL_FALSE;

	s = 1.0/r3[3];              /* now back substitute row 3 */
	r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

	m2 = r2[3];                 /* now back substitute row 2 */
	s  = 1.0/r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
		r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
		r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
		r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

	m1 = r1[2];                 /* now back substitute row 1 */
	s  = 1.0/r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
		r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
		r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

	m0 = r0[1];                 /* now back substitute row 0 */
	s  = 1.0/r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
		r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

	MAT(out,0,0) = r0[4]; MAT(out,0,1) = r0[5],
	MAT(out,0,2) = r0[6]; MAT(out,0,3) = r0[7],
	MAT(out,1,0) = r1[4]; MAT(out,1,1) = r1[5],
	MAT(out,1,2) = r1[6]; MAT(out,1,3) = r1[7],
	MAT(out,2,0) = r2[4]; MAT(out,2,1) = r2[5],
	MAT(out,2,2) = r2[6]; MAT(out,2,3) = r2[7],
	MAT(out,3,0) = r3[4]; MAT(out,3,1) = r3[5],
	MAT(out,3,2) = r3[6]; MAT(out,3,3) = r3[7];

	return GL_TRUE;

#undef MAT
#undef SWAP_ROWS
}

/* projection du point (objx,objy,obz) sur l'ecran (winx,winy,winz) */
GLint gluProject(GLdouble objx,GLdouble objy,GLdouble objz, const GLdouble model[16],const GLdouble proj[16],
                 const GLint viewport[4], GLdouble *winx,GLdouble *winy,GLdouble *winz)
{
	/* matrice de transformation */
	GLdouble in[4],out[4];

	/* initilise la matrice et le vecteur a transformer */
	in[0]=objx; in[1]=objy; in[2]=objz; in[3]=1.0;
	transform_point(out,model,in);
	transform_point(in,proj,out);

	/* d'ou le resultat normalise entre -1 et 1*/
	if (in[3]==0.0)
		return GL_FALSE;

	in[0]/=in[3]; in[1]/=in[3]; in[2]/=in[3];

	/* en coordonnees ecran */
	*winx = viewport[0]+(1+in[0])*viewport[2]/2;
	*winy = viewport[1]+(1+in[1])*viewport[3]/2;
	/* entre 0 et 1 suivant z */
	*winz = (1+in[2])/2;
	return GL_TRUE;
}

/* transformation du point ecran (winx,winy,winz) en point objet */
GLint gluUnProject(GLdouble winx,GLdouble winy,GLdouble winz, const GLdouble model[16],const GLdouble proj[16],
                   const GLint viewport[4], GLdouble *objx,GLdouble *objy,GLdouble *objz)
{
	/* matrice de transformation */
	GLdouble m[16], A[16];
	GLdouble in[4],out[4];

	/* transformation coordonnees normalisees entre -1 et 1 */
	in[0]=(winx-viewport[0])*2/viewport[2] - 1.0;
	in[1]=(winy-viewport[1])*2/viewport[3] - 1.0;
	in[2]=2*winz - 1.0;
	in[3]=1.0;

	/* calcul transformation inverse */
	matmul(A,proj,model);
	invert_matrix(A,m);

	/* d'ou les coordonnees objets */
	transform_point(out,m,in);
	if (out[3]==0.0)
		return GL_FALSE;
	*objx=out[0]/out[3];
	*objy=out[1]/out[3];
	*objz=out[2]/out[3];
	return GL_TRUE;
}

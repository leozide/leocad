#pragma once

#include <QtGlobal>
#include <QWidget>
#include <QtOpenGL>
#include <QGLWidget>
#include <QtGui>
#include <QPrinter>

#if !defined(EGL_VERSION_1_0) && !defined(GL_ES_VERSION_2_0) && !defined(GL_ES_VERSION_3_0)
#undef GL_LINES_ADJACENCY_EXT
#undef GL_LINE_STRIP_ADJACENCY_EXT
#undef GL_TRIANGLES_ADJACENCY_EXT
#undef GL_TRIANGLE_STRIP_ADJACENCY_EXT
#include "lc_glext.h"
#else
#define LC_OPENGLES 1
#endif

// Old defines and declarations.
#define LC_MAXPATH 1024

#define LC_POINTER_TO_INT(p) ((lcint32) (quintptr) (p))

typedef qint8 lcint8;
typedef quint8 lcuint8;
typedef qint16 lcint16;
typedef quint16 lcuint16;
typedef qint32 lcint32;
typedef quint32 lcuint32;
typedef qint64 lcint64;
typedef quint64 lcuint64;
typedef quintptr lcuintptr;

#ifdef Q_OS_WIN
char* strcasestr(const char *s, const char *find);
#else
char* strupr(char* string);
char* strlwr(char* string);
#endif

// Version number.
#define LC_VERSION_MAJOR 17
#define LC_VERSION_MINOR 07
#define LC_VERSION_PATCH 0
#define LC_VERSION_TEXT "17.07"

// Forward declarations.
class Project;
class lcModel;
class lcObject;
class lcPiece;
class lcCamera;
class lcLight;
class lcGroup;
class PieceInfo;
typedef std::map<const PieceInfo*, std::map<int, int>> lcPartsList;
struct lcModelPartsEntry;

class lcVector2;
class lcVector3;
class lcVector4;
class lcMatrix33;
class lcMatrix44;

class lcContext;
class lcMesh;
struct lcMeshSection;
struct lcRenderMesh;
class lcTexture;
class lcScene;

class lcFile;
class lcMemFile;
class lcDiskFile;


#ifndef LC_GLOBAL_H
#define LC_GLOBAL_H

#ifdef __cplusplus

#include <QtGlobal>
#include <QWidget>
#include <QtOpenGL>
#include <QGLWidget>
#include <QtGui>
#include <QPrinter>
#include <array>

#if !defined(EGL_VERSION_1_0) && !defined(GL_ES_VERSION_2_0) && !defined(GL_ES_VERSION_3_0) && !defined(QT_OPENGL_ES)
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

#ifdef Q_OS_WIN
char* strcasestr(const char *s, const char *find);
#else
char* strupr(char* string);
char* strlwr(char* string);
#endif

// Version number.
#define LC_VERSION_MAJOR 18
#define LC_VERSION_MINOR 02
#define LC_VERSION_PATCH 0
#define LC_VERSION_TEXT "18.02"

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

#endif

#endif // LC_GLOBAL_H

#ifndef LC_GLOBAL_H
#define LC_GLOBAL_H

#ifdef __cplusplus

#include <QtGlobal>
#include <QWidget>
#include <QtOpenGL>
#include <QGLWidget>
#include <QtGui>
#include <QPrinter>
#include <map>
#include <vector>
#include <array>
#include <set>
#include <functional>
#include <memory>

#ifndef Q_FALLTHROUGH
#define Q_FALLTHROUGH();
#endif

#ifndef QT_STRINGIFY
#define QT_STRINGIFY2(x) #x
#define QT_STRINGIFY(x) QT_STRINGIFY2(x)
#endif

#define LC_ARRAY_COUNT(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define LC_ARRAY_SIZE_CHECK(a,s) static_assert(LC_ARRAY_COUNT(a) == static_cast<int>(s), QT_STRINGIFY(a) " size mismatch.")

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
#define LC_MAXNAME 1000

#ifdef Q_OS_WIN
char* strcasestr(const char *s, const char *find);
#else
char* strupr(char* string);
char* strlwr(char* string);
#endif

// Version number.
#define LC_VERSION_MAJOR 19
#define LC_VERSION_MINOR 07
#define LC_VERSION_PATCH 1
#define LC_VERSION_TEXT "19.07.1"

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
struct lcMinifig;

class lcVector2;
class lcVector3;
class lcVector4;
class lcMatrix33;
class lcMatrix44;

class lcGLWidget;
class lcContext;
class lcMesh;
struct lcMeshSection;
struct lcRenderMesh;
class lcTexture;
class lcScene;
enum class lcRenderMeshState : int;

class lcFile;
class lcMemFile;
class lcDiskFile;

#endif

#endif // LC_GLOBAL_H

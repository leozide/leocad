#ifndef LC_GLOBAL_H
#define LC_GLOBAL_H

#ifdef __cplusplus

#include <QtGlobal>
#include <QtWidgets>
#include <QtConcurrent>
#include <QtOpenGL>
#include <QtGui>
#include <QWidget>
#include <QOpenGLWidget>
#include <QPrinter>
#include <QPrintDialog>
#include <map>
#include <vector>
#include <array>
#include <set>
#include <functional>
#include <memory>

#if _MSC_VER
#pragma warning(default : 4062) // enumerator 'identifier' in switch of enum 'enumeration' is not handled
#pragma warning(default : 4388) // 'token' : signed/unsigned mismatch
#pragma warning(default : 4389) // 'equality-operator' : signed/unsigned mismatch
#pragma warning(default : 5038) // data member 'A::y' will be initialized after data member 'A::x'
#endif

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
#ifdef Q_OS_MACOS
#define LC_FIXED_FUNCTION 0
#else
#define LC_FIXED_FUNCTION 1
#endif
#else
#define LC_OPENGLES 1
#define LC_FIXED_FUNCTION 0
#endif

// Old defines and declarations.
#define LC_MAXPATH 1024
#define LC_MAXNAME 1000

typedef quint32 lcStep;
#define LC_STEP_MAX 0xffffffff

#ifdef Q_OS_WIN
char* strcasestr(const char *s, const char *find);
#else
char* strupr(char* string);
#endif

// Version number.
#define LC_VERSION_MAJOR 23
#define LC_VERSION_MINOR 03
#define LC_VERSION_PATCH 0
#define LC_VERSION_TEXT "23.03"

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
enum class lcViewpoint;
enum class lcShadingMode;
enum class lcStudStyle;
class lcInstructions;
struct lcInstructionsPageSetup;
struct lcObjectRayTest;
struct lcObjectBoxTest;

class lcVector2;
class lcVector3;
class lcVector4;
class lcMatrix33;
class lcMatrix44;

class lcFindReplaceWidget;
struct lcFindReplaceParams;
class lcCollapsibleWidget;
class lcViewWidget;
class lcView;
class lcContext;
class lcMesh;
struct lcMeshSection;
struct lcRenderMesh;
struct lcObjectSection;
struct lcPieceInfoRayTest;
class lcTexture;
class lcScene;
class lcViewManipulator;
class lcViewSphere;
enum class lcRenderMeshState : int;
enum class lcTrackTool;
enum class lcTrackButton;

class lcFile;
class lcMemFile;
class lcDiskFile;

#endif

#endif // LC_GLOBAL_H

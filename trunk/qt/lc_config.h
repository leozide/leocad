#ifndef _LC_CONFIG_H_
#define _LC_CONFIG_H_

#include <QtGlobal>
#include <QWidget>
#include <QtOpenGL>
#include <QGLWidget>
#include <QtGui>

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

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define LC_LITTLE_ENDIAN
#else
#define LC_BIG_ENDIAN
#endif

#ifdef Q_OS_WIN
#define isnan _isnan
#define strcasecmp stricmp
#define strncasecmp strnicmp
char* strcasestr(const char *s, const char *find);
#else
char* strupr(char* string);
char* strlwr(char* string);
int stricmp(const char* str1, const char* str2);
#endif

#endif // _LC_CONFIG_H_

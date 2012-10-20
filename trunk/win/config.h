//
// LeoCAD configuration
//

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define LC_VERSION_MAJOR 0
#define LC_VERSION_MINOR 79
#define LC_VERSION_PATCH 0
#define LC_VERSION_OSNAME "Windows"
#define LC_VERSION_TEXT "0.79"
#define LC_VERSION_TAG ""
#define LC_INSTALL_PREFIX "C:\\leocad"
#define LC_POINTER_TO_INT(p) ((lcint32)(p))

typedef signed __int8 lcint8;
typedef unsigned __int8 lcuint8;
typedef signed __int16 lcint16;
typedef unsigned __int16 lcuint16;
typedef signed __int32 lcint32;
typedef unsigned __int32 lcuint32;
typedef signed __int64 lcint64;
typedef unsigned __int64 lcuint64;

#define LC_LITTLE_ENDIAN
#define LCUINT16(val) val
#define LCUINT32(val) val
#define LCINT16(val) val
#define LCINT32(val) val
#define LCFLOAT(val) val

#define LC_HAVE_3DSFTK
#define LC_HAVE_JPEGLIB
#define LC_HAVE_PNGLIB

#endif // _CONFIG_H_


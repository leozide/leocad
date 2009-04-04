//
// LeoCAD configuration
//

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define LC_VERSION_MAJOR 0
#define LC_VERSION_MINOR 76
#define LC_VERSION_PATCH 0
#define LC_VERSION_OSNAME "iPhone"
#define LC_VERSION_TEXT "0.76"
#define LC_VERSION_TAG ""
#define LC_INSTALL_PREFIX "."

typedef signed char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;

#define LC_LITTLE_ENDIAN
#define LCUINT16(val) val
#define LCUINT32(val) val
#define LCINT16(val) val
#define LCINT32(val) val
#define LCFLOAT(val) val

//#define LC_HAVE_JPEGLIB
//#define LC_HAVE_ZLIB
//#define LC_HAVE_PNGLIB

#endif // _CONFIG_H_

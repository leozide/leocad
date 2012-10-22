#ifndef _LC_CONFIG_H_
#define _LC_CONFIG_H_

#include "stdafx.h"

#define LC_WINDOWS

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

#define LC_HAVE_3DSFTK
#define LC_HAVE_JPEGLIB
#define LC_HAVE_PNGLIB

#endif // _LC_CONFIG_H_

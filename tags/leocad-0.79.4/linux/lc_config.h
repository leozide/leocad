#ifndef _LC_CONFIG_H_
#define _LC_CONFIG_H_

#include <glib.h>

#define LC_LINUX

#define LC_POINTER_TO_INT GPOINTER_TO_INT

typedef gint8 lcint8;
typedef guint8 lcuint8;
typedef gint16 lcint16;
typedef guint16 lcuint16;
typedef gint32 lcint32;
typedef guint32 lcuint32;
typedef gint64 lcint64;
typedef guint64 lcuint64;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define LC_LITTLE_ENDIAN
#else
#define LC_BIG_ENDIAN
#endif

#endif // _LC_CONFIG_H_


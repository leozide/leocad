#ifndef _LC_GLOBAL_H_
#define _LC_GLOBAL_H_

#include "lc_config.h"
#include "defines.h"

// Version number.
#define LC_VERSION_MAJOR 0
#define LC_VERSION_MINOR 79
#define LC_VERSION_PATCH 4
#define LC_VERSION_TEXT "0.79.4"

// Check for supported platforms.
#if !defined(LC_WINDOWS) && !defined(LC_LINUX)
#error No OS defined.
#endif

// Forward declarations.
class lcVector2;
class lcVector3;
class lcVector4;
class lcMatrix44;

class lcMesh;
class lcTexture;

class lcFile;
class lcMemFile;
class lcDiskFile;

#endif // _LC_GLOBAL_H_

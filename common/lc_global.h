#ifndef _LC_GLOBAL_H_
#define _LC_GLOBAL_H_

#include "config.h"
#include "defines.h"

// Check for supported platforms.
#if !defined(LC_WINDOWS) && !defined(LC_LINUX)
#error No OS defined.
#endif

// Precompiled headers.
#if LC_WINDOWS
#include "stdafx.h"
#endif

// Forward declarations.
class lcVector3;
class lcVector4;

class lcMesh;

class lcFile;
class lcMemFile;
class lcDiskFile;

#endif // _LC_GLOBAL_H_

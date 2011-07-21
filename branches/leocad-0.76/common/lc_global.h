#ifndef _LC_GLOBAL_H_
#define _LC_GLOBAL_H_

#include "config.h"
#include "defines.h"

// Check for supported platforms.
#if !LC_WINDOWS && !LC_LINUX && !LC_MACOSX && !LC_IPHONE
#error No OS defined.
#endif

// Precompiled headers.
#if LC_WINDOWS
#include "stdafx.h"
#endif

#endif // _LC_GLOBAL_H_

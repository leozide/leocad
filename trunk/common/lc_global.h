#ifndef _LC_GLOBAL_H_
#define _LC_GLOBAL_H_

#include "config.h"

// Check for supported platforms.
#if !LC_WINDOWS && !LC_LINUX && !LC_MACOSX
#error You need to define your OS.
#endif

#if LC_WINDOWS
#include "stdafx.h"
#endif

#endif // _LC_GLOBAL_H_

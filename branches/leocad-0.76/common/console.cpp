//
// Debug Console
//

#include "lc_global.h"
#include <stdarg.h>
#include <stdio.h>
#include "console.h"

Console console;

// ============================================================================

Console::Console ()
{
  m_pWindowFunc = NULL;
}

Console::~Console ()
{
}

// ============================================================================

void Console::Print (LC_CONSOLE_LEVEL level, const char* format, ...)
{
  char text[512];
  va_list args;

  va_start (args, format);
  vsprintf (text, format, args);
  va_end (args);

  InternalPrint (level, text);
}

void Console::PrintMisc (const char* format, ...)
{
  char text[512];
  va_list args;

  va_start (args, format);
  vsprintf (text, format, args);
  va_end (args);

  InternalPrint (LC_CONSOLE_MISC, text);
}

void Console::PrintDebug (const char* format, ...)
{
  char text[512];
  va_list args;

  va_start (args, format);
  vsprintf (text, format, args);
  va_end (args);

  InternalPrint (LC_CONSOLE_DEBUG, text);
}

void Console::PrintWarning (const char* format, ...)
{
  char text[512];
  va_list args;

  va_start (args, format);
  vsprintf (text, format, args);
  va_end (args);

  InternalPrint (LC_CONSOLE_WARNING, text);
}

void Console::PrintError (const char* format, ...)
{
  char text[512];
  va_list args;

  va_start (args, format);
  vsprintf (text, format, args);
  va_end (args);

  InternalPrint (LC_CONSOLE_ERROR, text);
}

void Console::InternalPrint (LC_CONSOLE_LEVEL level, const char* text)
{
#ifndef LC_DEBUG
  if (level == LC_CONSOLE_DEBUG)
    return;
#endif

  if (m_pWindowFunc)
    (*m_pWindowFunc) (level, text, m_pWindowFuncData);
}

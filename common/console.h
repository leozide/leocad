#ifndef _CONSOLE_H_
#define _CONSOLE_H_

typedef enum
{
  LC_CONSOLE_ERROR,
  LC_CONSOLE_WARNING,
  LC_CONSOLE_DEBUG,
  LC_CONSOLE_MISC
} LC_CONSOLE_LEVEL;

typedef void (*CONSOLECALLBACK) (LC_CONSOLE_LEVEL level, const char* text, void* user_data);

class Console
{
public:
  Console ();
  virtual ~Console ();

  void Print (LC_CONSOLE_LEVEL level, const char* format, ...);
  void PrintMisc (const char* format, ...);
  void PrintDebug (const char* format, ...);
  void PrintWarning (const char* format, ...);
  void PrintError (const char* format, ...);

  void SetWindowCallback (CONSOLECALLBACK func, void* data)
    { m_pWindowFunc = func; m_pWindowFuncData = data; };

protected:
  void InternalPrint (LC_CONSOLE_LEVEL level, const char* text);

  CONSOLECALLBACK m_pWindowFunc;
  void* m_pWindowFuncData;

  // variables
  bool use_tty;
  bool use_file;
};

extern Console console;

#endif // _CONSOLE_H_

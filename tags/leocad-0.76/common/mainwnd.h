#ifndef _MAINWND_H_
#define _MAINWND_H_

#include "str.h"
#include "basewnd.h"

#define LC_MRU_MAX 4

typedef enum
{
  LC_MAINWND_RECENT1,
  LC_MAINWND_RECENT2,
  LC_MAINWND_RECENT3,
  LC_MAINWND_RECENT4,
  LC_MAINWND_NUM_COMMANDS
} LC_MAINWND_COMMANDS;

class MainWnd : public BaseWnd
{
 public:
  MainWnd ();
  virtual ~MainWnd ();

  void UpdateMRU ();
  void AddToMRU (const char *filename);
  void RemoveFromMRU (int index);
  const char* GetMRU (int index) const
    { return m_strMRU[index]; }

 protected:
  String m_strMRU[LC_MRU_MAX];
};

#endif // _MAINWND_H_

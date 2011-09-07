#ifndef _BASEWND_H_
#define _BASEWND_H_

#include <string.h>

// FIXME: move this to another place
#ifdef WIN32
#include "stdafx.h"
typedef CWnd* BaseWndXID;
typedef struct
{
  CWnd* wnd;
  int index;
  UINT command;
} BaseMenuItem;
#endif

#ifdef LC_LINUX
#include <gtk/gtk.h>
typedef GtkWidget* BaseWndXID;
typedef struct
{
  GtkWidget* widget;
  GtkAccelGroup* accel;
} BaseMenuItem;
#endif

#ifdef LC_MACOSX
typedef void* BaseWndXID;
typedef struct
{
	void* Dummy;
} BaseMenuItem;
#endif

// =============================================================================
// Message Box constants

#define LC_OK           1
#define LC_CANCEL       2
#define LC_ABORT        3
#define LC_RETRY        4
#define LC_IGNORE       5
#define LC_YES          6
#define LC_NO           7
 
#define LC_MB_OK                 0x000
#define LC_MB_OKCANCEL           0x001
//#define LC_MB_ABORTRETRYIGNORE 0x002
#define LC_MB_YESNOCANCEL        0x003
#define LC_MB_YESNO              0x004
//#define LC_MB_RETRYCANCEL      0x005

#define LC_MB_ICONERROR          0x010
#define LC_MB_ICONQUESTION       0x020
#define LC_MB_ICONWARNING        0x030
#define LC_MB_ICONINFORMATION    0x040
 
#define LC_MB_TYPEMASK           0x00F
#define LC_MB_ICONMASK           0x0F0

// =============================================================================

class BaseWnd
{
 public:
  BaseWnd (BaseWnd *parent, int menu_count);
  virtual ~BaseWnd ();

  int MessageBox (const char* text, const char* caption="LeoCAD", int flags=LC_MB_OK|LC_MB_ICONINFORMATION);
  void BeginWait ();
  void EndWait ();
  void SetTitle (const char *title);

  void ShowMenuItem (int id, bool show);
  void EnableMenuItem (int id, bool enable);
  void CheckMenuItem (int id, bool check);
  void SetMenuItemText (int id, const char *text);

  BaseWndXID GetXID () const
    { return m_pXID; }
  void SetXID (BaseWndXID id)
    { m_pXID = id; }

#ifdef LC_LINUX 
  // FIXME: remove
  operator GtkWidget* () const
    { return m_pXID; }
#endif

  BaseMenuItem* GetMenuItem (int id) const
    { return &m_pMenuItems[id]; }
  void SetMenuItem (int id, BaseMenuItem* item)
    { memcpy (&m_pMenuItems[id], item, sizeof (BaseMenuItem)); }

 protected:
  BaseWnd* m_pParent;
  BaseWndXID m_pXID;
  BaseMenuItem* m_pMenuItems;
};

#endif // _BASEWND_H_

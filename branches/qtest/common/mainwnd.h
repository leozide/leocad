#ifndef _MAINWND_H_
#define _MAINWND_H_

#include "basewnd.h"

extern class lcMainWindow* gMainWindow;

class lcMainWindow : public lcBaseWindow
{
public:
	lcMainWindow()
	{
		gMainWindow = this;
	}

	~lcMainWindow()
	{
		gMainWindow = NULL;
	}

	virtual void UpdateAction(int NewAction) = 0;
	virtual void UpdateSnap(lcuint32 Snap) = 0;
	virtual void UpdateUndoRedo(const char* UndoText, const char* RedoText) = 0;
//	void UpdateSnap(unsigned long snap);
};








#include "str.h"

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

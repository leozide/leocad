#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "array.h"

typedef void (*LC_MSG_CALLBACK) (int message, void *data, void *user);

typedef enum
{
  LC_MSG_FOCUS_CHANGED,
  //  LC_MSG_SELECTION_CHANGED,
  LC_MSG_COUNT
} LC_MSG_TYPES;

typedef struct
{
  LC_MSG_CALLBACK func;
  void *user;
} LC_MSG_STRUCT;

class Messenger
{
 public:
  Messenger ();
  ~Messenger ();

  void AddRef ()
    { m_nRef++; };
  void DecRef ()
    { m_nRef--; if (m_nRef == 0) delete this; };

  void Dispatch (int message, void *data);
  void Listen (LC_MSG_CALLBACK func, void *user);

 protected:
  int m_nRef;
  PtrArray<LC_MSG_STRUCT> m_Listeners;
};

#endif // _MESSAGE_H_

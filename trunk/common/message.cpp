//
// LeoCAD messaging system
//

#include "lc_global.h"
#include "message.h"

Messenger::Messenger ()
{
  m_nRef = 0;
}

Messenger::~Messenger ()
{
  for (int i = 0; i < m_Listeners.GetSize (); i++)
    delete m_Listeners[i];
}

void Messenger::Dispatch (int message, void *data)
{
  for (int i = 0; i < m_Listeners.GetSize (); i++)
    m_Listeners[i]->func (message, data, m_Listeners[i]->user);
}

void Messenger::Listen (LC_MSG_CALLBACK func, void *user)
{
  LC_MSG_STRUCT *s = new LC_MSG_STRUCT;

  s->func = func;
  s->user = user;

  m_Listeners.Add (s);
}

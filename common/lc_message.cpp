#include "lc_global.h"
#include "lc_message.h"
#include "lc_array.h"

lcPtrArray<lcListener> gListeners;

lcListener::lcListener()
{
	gListeners.Add(this);
}

lcListener::~lcListener()
{
	gListeners.RemovePointer(this);
}

void lcPostMessage(lcMessageType Message, void* Data)
{
	for (int i = 0; i < gListeners.GetSize(); i++)
		gListeners[i]->ProcessMessage(Message, Data);
}

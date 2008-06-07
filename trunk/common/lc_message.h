#ifndef _LC_MESSAGE_H_
#define _LC_MESSAGE_H_

enum lcMessageType
{
	LC_MSG_FOCUS_OBJECT_CHANGED, // Focused object changed - Data is the new focused object.
	LC_MSG_COLOR_CHANGED,        // Color changed - Data is the new color.
	LC_MSG_MOUSE_CAPTURE_LOST,   // Application lost mouse capture.
	LC_MSG_COUNT
};

class lcListener
{
public:
	lcListener();
	~lcListener();

	virtual void ProcessMessage(lcMessageType Message, void* Data) = 0;
};

void lcPostMessage(lcMessageType Message, void* Data);

#endif // _LC_MESSAGE_H_

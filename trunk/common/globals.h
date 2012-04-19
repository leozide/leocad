#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "typedefs.h"
#include "console.h"

class Messenger;
extern Messenger* messenger;

class MainWnd;
extern MainWnd* main_window;

extern unsigned char FlatColorArray[31][3];
extern unsigned char ColorArray[31][4];

#endif // _GLOBALS_H_

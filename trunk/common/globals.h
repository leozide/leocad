#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "defines.h"
#include "typedefs.h"
#include "console.h"

class Messenger;
extern Messenger* messenger;

class MainWnd;
extern MainWnd* main_window;

/*
extern unsigned char FlatColorArray[31][3];
extern unsigned char ColorArray[31][4];
extern const char* colornames[LC_MAXCOLORS];
extern const char* altcolornames[LC_MAXCOLORS];
extern const char* lg_colors[28];
*/
#endif // _GLOBALS_H_

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "typedefs.h"

// ============================================================================
// Keyboard keys.

#define LC_KEY_BACK       0x08
#define LC_KEY_TAB        0x09

#define LC_KEY_RETURN     0x0D

#define LC_KEY_PAUSE      0x13
#define LC_KEY_CAPITAL    0x14

#define LC_KEY_ESCAPE     0x1B

#define LC_KEY_SPACE      0x20
#define LC_KEY_PRIOR      0x21
#define LC_KEY_NEXT       0x22
#define LC_KEY_END        0x23
#define LC_KEY_HOME       0x24
#define LC_KEY_LEFT       0x25
#define LC_KEY_UP         0x26
#define LC_KEY_RIGHT      0x27
#define LC_KEY_DOWN       0x28
#define LC_KEY_SELECT     0x29
#define LC_KEY_PRINT      0x2A
#define LC_KEY_INSERT     0x2D
#define LC_KEY_DELETE     0x2E

#define LC_KEY_0          0x30
#define LC_KEY_1          0x31
#define LC_KEY_2          0x32
#define LC_KEY_3          0x33
#define LC_KEY_4          0x34
#define LC_KEY_5          0x35
#define LC_KEY_6          0x36
#define LC_KEY_7          0x37
#define LC_KEY_8          0x38
#define LC_KEY_9          0x39

#define LC_KEY_A          0x41
#define LC_KEY_B          0x42
#define LC_KEY_C          0x43
#define LC_KEY_D          0x44
#define LC_KEY_E          0x45
#define LC_KEY_F          0x46
#define LC_KEY_G          0x47
#define LC_KEY_H          0x48
#define LC_KEY_I          0x49
#define LC_KEY_J          0x4A
#define LC_KEY_K          0x4B
#define LC_KEY_L          0x4C
#define LC_KEY_M          0x4D
#define LC_KEY_N          0x4E
#define LC_KEY_O          0x4F
#define LC_KEY_P          0x50
#define LC_KEY_Q          0x51
#define LC_KEY_R          0x52
#define LC_KEY_S          0x53
#define LC_KEY_T          0x54
#define LC_KEY_U          0x55
#define LC_KEY_V          0x56
#define LC_KEY_W          0x57
#define LC_KEY_X          0x58
#define LC_KEY_Y          0x59
#define LC_KEY_Z          0x5A

#define LC_KEY_NUMPAD0    0x60
#define LC_KEY_NUMPAD1    0x61
#define LC_KEY_NUMPAD2    0x62
#define LC_KEY_NUMPAD3    0x63
#define LC_KEY_NUMPAD4    0x64
#define LC_KEY_NUMPAD5    0x65
#define LC_KEY_NUMPAD6    0x66
#define LC_KEY_NUMPAD7    0x67
#define LC_KEY_NUMPAD8    0x68
#define LC_KEY_NUMPAD9    0x69
#define LC_KEY_MULTIPLY   0x6A
#define LC_KEY_ADD        0x6B
//#define LC_KEY_SEPARATOR  0x6C
#define LC_KEY_SUBTRACT   0x6D
#define LC_KEY_DECIMAL    0x6E
#define LC_KEY_DIVIDE     0x6F
#define LC_KEY_F1         0x70
#define LC_KEY_F2         0x71
#define LC_KEY_F3         0x72
#define LC_KEY_F4         0x73
#define LC_KEY_F5         0x74
#define LC_KEY_F6         0x75
#define LC_KEY_F7         0x76
#define LC_KEY_F8         0x77
#define LC_KEY_F9         0x78
#define LC_KEY_F10        0x79
#define LC_KEY_F11        0x7A
#define LC_KEY_F12        0x7B
#define LC_KEY_F13        0x7C
#define LC_KEY_F14        0x7D
#define LC_KEY_F15        0x7E
#define LC_KEY_F16        0x7F
#define LC_KEY_F17        0x80
#define LC_KEY_F18        0x81
#define LC_KEY_F19        0x82
#define LC_KEY_F20        0x83
#define LC_KEY_F21        0x84
#define LC_KEY_F22        0x85
#define LC_KEY_F23        0x86
#define LC_KEY_F24        0x87

#define LC_KEY_NUMLOCK    0x90
#define LC_KEY_SCROLL     0x91

// ============================================================================
// Functions.

#define LC_KEYMOD1_SHIFT    0x01
#define LC_KEYMOD1_CONTROL  0x02
#define LC_KEYMOD2_SHIFT    0x04
#define LC_KEYMOD2_CONTROL  0x08
#define LC_KEYMOD_VIEWONLY  0x10

#define LC_KEYMOD1_MASK     (LC_KEYMOD1_SHIFT | LC_KEYMOD1_CONTROL)
#define LC_KEYMOD2_MASK     (LC_KEYMOD2_SHIFT | LC_KEYMOD2_CONTROL)
#define LC_KEYMOD_MASK      (LC_KEYMOD1_MASK | LC_KEYMOD2_MASK)

#define LC_KEYMOD_1TO2(a)   ((a & ~LC_KEYMOD_MASK) | ((a & LC_KEYMOD1_MASK) << 2))
#define LC_KEYMOD_2TO1(a)   ((a & ~LC_KEYMOD_MASK) | ((a & LC_KEYMOD2_MASK) >> 2))

typedef struct
{
	LC_COMMANDS ID;
	const char* Description;
	unsigned char Flags;
	unsigned char Key1;
	unsigned char Key2;
} LC_KEYBOARD_COMMAND;

extern LC_KEYBOARD_COMMAND KeyboardShortcuts[];
extern const int KeyboardShortcutsCount;

const char* GetKeyName(char Key);
char GetKeyFromName(const char* Name);

void InitKeyboardShortcuts();
void ResetKeyboardShortcuts();
bool SaveKeyboardShortcuts(const char* FileName);
bool LoadKeyboardShortcuts(const char* FileName);

#endif // _KEYBOARD_H_

// Global variables.
//

class Project;
extern Project* project;

#include "defines.h"
#include "typedefs.h"

extern unsigned char FlatColorArray[31][3];
extern unsigned char ColorArray[31][4];
extern const char* colornames[LC_MAXCOLORS];
extern const char* altcolornames[LC_MAXCOLORS];

#define MFW_PIECES 85
extern MFW_PIECEINFO mfwpieceinfo[];

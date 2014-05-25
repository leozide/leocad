#ifndef _LC_GLOBAL_H_
#define _LC_GLOBAL_H_

#include "lc_config.h"

// Version number.
#define LC_VERSION_MAJOR 0
#define LC_VERSION_MINOR 80
#define LC_VERSION_PATCH 4
#define LC_VERSION_TEXT "0.80.4"

// Forward declarations.
class lcObject;
class lcPiece;
class lcCamera;
class lcLight;
class lcGroup;

typedef lcObject Object;
typedef lcPiece Piece;
typedef lcCamera Camera;
typedef lcLight Light;
typedef lcGroup Group;

class lcVector2;
class lcVector3;
class lcVector4;
class lcMatrix44;

class lcContext;
class lcMesh;
struct lcMeshSection;
struct lcRenderMesh;
class lcTexture;

class lcFile;
class lcMemFile;
class lcDiskFile;

#endif // _LC_GLOBAL_H_

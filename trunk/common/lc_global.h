#ifndef _LC_GLOBAL_H_
#define _LC_GLOBAL_H_

#include "lc_config.h"

// Version number.
#define LC_VERSION_MAJOR 0
#define LC_VERSION_MINOR 81
#define LC_VERSION_PATCH 0
#define LC_VERSION_TEXT "0.81.0"

// Forward declarations.
class lcModel;
class lcObject;
class lcPiece;
class lcCamera;
class lcLight;
class lcGroup;
class PieceInfo;
struct lcPartsListEntry;
struct lcModelPartsEntry;

class lcVector2;
class lcVector3;
class lcVector4;
class lcMatrix33;
class lcMatrix44;

class lcContext;
class lcMesh;
struct lcMeshSection;
struct lcRenderMesh;
class lcTexture;
struct lcScene;

class lcFile;
class lcMemFile;
class lcDiskFile;

#endif // _LC_GLOBAL_H_

// Typedefs.
//

#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

class Group;
class Piece;
class PieceInfo;
class Camera;

#include "str.h"
#include "lc_math.h"
#include "lc_commands.h"

enum LC_NOTIFY
{
	LC_COLOR_CHANGED,
	LC_CAPTURE_LOST,
	LC_ACTIVATE,
	LC_PIECE_MODIFIED,
	LC_CAMERA_MODIFIED,
	LC_LIGHT_MODIFIED
};

enum LC_ACTIONS
{
	LC_ACTION_INSERT,
	LC_ACTION_LIGHT,
	LC_ACTION_SPOTLIGHT,
	LC_ACTION_CAMERA,
	LC_ACTION_SELECT, 
	LC_ACTION_MOVE,
	LC_ACTION_ROTATE,
	LC_ACTION_ERASER,
	LC_ACTION_PAINT,
	LC_ACTION_ZOOM,
	LC_ACTION_PAN,
	LC_ACTION_ROTATE_VIEW,
	LC_ACTION_ROLL,
	LC_ACTION_ZOOM_REGION
};

struct LC_PIECE_MODIFY
{
	Piece* piece;
	lcVector3 Position;
	lcVector3 Rotation;
	char name[81];
	int from;
	int to;
	bool hidden;
	int color;
};

struct LC_CAMERA_MODIFY
{
	Camera* camera;
	lcVector3 Eye;
	lcVector3 Target;
	lcVector3 Up;
	char name[81];
	float fovy;
	float znear;
	float zfar;
	bool hidden;
};

#endif

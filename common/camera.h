//
//	camera.h
////////////////////////////////////////////////////

#ifndef _CAMERA_H_
#define _CAMERA_H_

#ifndef GLuint
#include "GL/gl.h"
#endif
#include "boundbox.h"

#define LC_CAMERA_HIDDEN			0x01
#define LC_CAMERA_SELECTED			0x02
#define LC_CAMERA_FOCUSED			0x04
#define LC_CAMERA_TARGET_SELECTED	0x08
#define LC_CAMERA_TARGET_FOCUSED	0x10

typedef enum {	LC_CAMERA_FRONT,LC_CAMERA_BACK, 
				LC_CAMERA_TOP,  LC_CAMERA_UNDER, 
				LC_CAMERA_LEFT, LC_CAMERA_RIGHT,
				LC_CAMERA_MAIN, LC_CAMERA_USER }  LC_CAMERA_TYPES;

typedef enum {	CK_EYE, CK_TARGET, CK_UP } CK_TYPES;

typedef struct CAMERA_KEY {
	unsigned short	time;
	float			param[3];
	unsigned char	type;
	CAMERA_KEY*		next;
} CAMERA_KEY;

class File;
class TiledRender;

class Camera
{
public:
	Camera();
	Camera(unsigned char nType, Camera* pPrev);
	Camera(float ex, float ey, float ez, float tx, float ty, float tz, Camera* pCamera);
	Camera(float eye[3], float target[3], float up[3], Camera* pCamera);
	~Camera();

	Camera* m_pNext;
	void Hide()
		{ m_nState = LC_CAMERA_HIDDEN; }
	void UnHide()
		{ m_nState &= ~LC_CAMERA_HIDDEN; }
	char* GetName()
		{ return m_strName; }
	bool IsSide()
		{ return m_nType < LC_CAMERA_MAIN; }
	bool IsUser()
		{ return m_nType == LC_CAMERA_USER; }
	bool IsVisible()
		{ return (m_nState & LC_CAMERA_HIDDEN) == 0; }
	bool IsSelected()
		{ return (m_nState & (LC_CAMERA_SELECTED|LC_CAMERA_TARGET_SELECTED)) != 0; } 
	bool IsEyeSelected()
		{ return (m_nState & LC_CAMERA_SELECTED) != 0; } 
	bool IsTargetSelected()
		{ return (m_nState & LC_CAMERA_TARGET_SELECTED) != 0; } 
	void Select()
		{ m_nState |= (LC_CAMERA_SELECTED|LC_CAMERA_TARGET_SELECTED); } 
	void UnSelect()
		{ m_nState &= ~(LC_CAMERA_SELECTED|LC_CAMERA_FOCUSED|LC_CAMERA_TARGET_SELECTED|LC_CAMERA_TARGET_FOCUSED); } 
	void UnFocus()
		{ m_nState &= ~(LC_CAMERA_FOCUSED|LC_CAMERA_TARGET_FOCUSED); } 
	bool IsEyeFocused()
		{ return (m_nState & LC_CAMERA_FOCUSED) != 0; } 
	void FocusEye()
		{ m_nState |= (LC_CAMERA_FOCUSED|LC_CAMERA_SELECTED); } 
	bool IsTargetFocused()
		{ return (m_nState & LC_CAMERA_TARGET_FOCUSED) != 0; } 
	void FocusTarget()
		{ m_nState |= (LC_CAMERA_TARGET_FOCUSED|LC_CAMERA_TARGET_SELECTED); } 

	void GetEye(float* eye)
		{ memcpy(eye, m_fEye, sizeof(m_fEye)); };
	void GetTarget(float* target)
		{ memcpy(target, m_fTarget, sizeof(m_fTarget)); };
	void GetUp(float* up)
		{ memcpy(up, m_fUp, sizeof(m_fUp)); };

public:
	void MinIntersectDist(CLICKLINE* Line);
	void ChangeKey(unsigned short nTime, bool bAnimation, bool bAddKey, float param[3], unsigned char nKeyType);
	void UpdatePosition(unsigned short nTime, bool bAnimation);
	void Render(float fLineWidth);
	void LoadProjection(float fAspect);
	void FileLoad(File* file);
	void FileSave(File* file);

	void DoZoom(int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey, float* center);
	void DoRoll(int dx, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z);

	void StartTiledRendering(int tw, int th, int iw, int ih, float fAspect);
	void GetTileInfo(int* row, int* col, int* width, int* height);
	bool EndTile();

	float m_fovy;
	float m_zNear;
	float m_zFar;

protected:
	void CalculatePosition(unsigned short nTime, bool bAnimation, float eye[3], float target[3], float up[3]);
	void RemoveKeys();
	void Initialize();
	
	// For using the mouse
	BoundingBox m_BoundingBox;
	BoundingBox m_TargetBoundingBox;

	// Position
	CAMERA_KEY* m_pAnimationKeys;
	CAMERA_KEY* m_pInstructionKeys;

	// Attributes
	char m_strName[81];
	unsigned char m_nState;
	unsigned char m_nType;
	GLuint m_nList;

	// Temporary position
	float m_fEye[3];
	float m_fTarget[3];
	float m_fUp[3];
	TiledRender* m_pTR;
};

#endif // _CAMERA_H_

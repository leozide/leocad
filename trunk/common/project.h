#ifndef _PROJECT_H_
#define _PROJECT_H_

#include "object.h"
#include "opengl.h"
#include "lc_array.h"
#include "lc_math.h"
#include "lc_commands.h"
#include "str.h"

#define LC_SCENE_FOG			0x004	// Enable fog
#define LC_SCENE_BG				0x010	// Draw bg image
#define LC_SCENE_BG_TILE		0x040	// Tile bg image
#define LC_SCENE_GRADIENT		0x100	// Draw gradient

#define LC_HTML_SINGLEPAGE      0x01
#define LC_HTML_INDEX           0x02
#define LC_HTML_IMAGES          0x04
#define LC_HTML_LISTEND         0x08
#define LC_HTML_LISTSTEP        0x10
#define LC_HTML_HIGHLIGHT       0x20
//#define LC_HTML_HTMLEXT         0x40
//#define LC_HTML_LISTID          0x80

enum LC_MOUSE_TRACK
{
	LC_TRACK_NONE,
	LC_TRACK_LEFT,
	LC_TRACK_RIGHT
};

class PieceInfo;
class View;
class Image;

#include "lc_file.h"

struct LC_FILEENTRY
{
	lcMemFile File;
	char FileName[LC_MAXPATH];
};

struct lcPiecesUsedEntry
{
	PieceInfo* Info;
	int ColorIndex;
	int Count;
};

enum lcObjectProperty
{
	LC_PIECE_PROPERTY_POSITION,
	LC_PIECE_PROPERTY_ROTATION,
	LC_PIECE_PROPERTY_SHOW,
	LC_PIECE_PROPERTY_HIDE,
	LC_PIECE_PROPERTY_COLOR,
	LC_PIECE_PROPERTY_ID,
	LC_CAMERA_PROPERTY_POSITION,
	LC_CAMERA_PROPERTY_TARGET,
	LC_CAMERA_PROPERTY_UPVECTOR,
	LC_CAMERA_PROPERTY_ORTHO,
	LC_CAMERA_PROPERTY_FOV,
	LC_CAMERA_PROPERTY_NEAR,
	LC_CAMERA_PROPERTY_FAR,
	LC_CAMERA_PROPERTY_NAME
};

#include "lc_model.h"

class Project : public lcModel
{
public:
	Project();
	~Project();

	void SetCurrentStep(lcStep Step)
	{
		mCurrentStep = Step;
		CalculateStep();
	}

	float* GetBackgroundColor() // todo: remove
	{
		return mProperties.mBackgroundSolidColor;
	}

	int GetGroupIndex(lcGroup* Group) const
	{
		return mGroups.FindIndex(Group);
	}

	void UpdateInterface();
	void SetPathName(const char* lpszPathName, bool bAddToMRU);
	void SetTitle(const char* lpszTitle);

public:
	void LoadDefaults();
	bool GetPiecesBoundingBox(View* view, float BoundingBox[6]);
	void GetPiecesUsed(lcArray<lcPiecesUsedEntry>& PiecesUsed) const;
	void CreateImages(Image* images, int width, int height, lcStep from, lcStep to, bool hilite);
	void Render(View* view, bool bToMemory);
	lcGroup* AddGroup(lcGroup* Parent);
	void TransformSelectedObjects(lcTransformType Type, const lcVector3& Transform);
	void ModifyObject(lcObject* Object, lcObjectProperty Property, void* Value);
	void ZoomActiveView(int Amount);
	void GetPieceInsertPosition(View* view, lcVector3& Position, lcVector4& Orientation);
	void HandleCommand(LC_COMMANDS id);

protected:
	void CheckPoint(const char* Description);

	void GetPieceInsertPosition(lcPiece* OffsetPiece, lcVector3& Position, lcVector4& Rotation);

	static int InstanceOfName(const String& existingString, const String& candidateString, String& baseNameOut);

	void RenderBackground(View* view);
	void RenderScenePieces(View* view, bool DrawInterface);
	void RenderSceneObjects(View* view);

	void CreateHTMLPieceList(FILE* f, lcStep Step, bool bImages, const char* ext);
	void Export3DStudio();
	void ExportPOVRay(lcFile& File);
	void ZoomExtents(int FirstView, int LastView);

	bool DoSave(const char* FileName);
	bool FileLoad(lcFile* file, bool bUndo, bool bMerge);
	void FileReadLDraw(lcFile* file, const lcMatrix44& CurrentTransform, int* nOk, int DefColor, int* nStep, lcArray<LC_FILEENTRY*>& FileArray);
	void FileReadMPD(lcFile& MPD, lcArray<LC_FILEENTRY*>& FileArray) const;

	char m_strTitle[LC_MAXPATH];
	char m_strPathName[LC_MAXPATH];

public:
	bool OnNewDocument();
	bool OnOpenDocument(const char* FileName);
	bool OpenProject(const char* FileName);
	bool SaveModified();
};

#endif // _PROJECT_H_

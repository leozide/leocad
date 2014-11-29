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

	int GetGroupIndex(lcGroup* Group) const
	{
		return mGroups.FindIndex(Group);
	}

	void UpdateInterface();
	void LoadDefaults();
	void SaveImage();
	void SaveStepImages(const QString& BaseName, int Width, int Height, lcStep Start, lcStep End);
	void HandleCommand(LC_COMMANDS id);

protected:
	void CheckPoint(const char* Description);

	static int InstanceOfName(const String& existingString, const String& candidateString, String& baseNameOut);

	void ExportHTML();
	void CreateHTMLPieceList(QTextStream& Stream, lcStep Step, bool Images, const QString& ImageExtension);

	bool DoSave(const QString& FileName);
	bool FileLoad(lcFile* file, bool bUndo, bool bMerge);
	void FileReadLDraw(lcFile* file, const lcMatrix44& CurrentTransform, int* nOk, int DefColor, int* nStep, lcArray<LC_FILEENTRY*>& FileArray);
	void FileReadMPD(lcFile& MPD, lcArray<LC_FILEENTRY*>& FileArray) const;

	void SetFileName(const QString& FileName);
	QString GetTitle() const;

	QString mFileName;

public:
	bool OnNewDocument();
	bool OnOpenDocument(const QString& FileName);
	bool OpenProject(const QString& FileName);
	bool SaveIfModified();
};

#endif // _PROJECT_H_

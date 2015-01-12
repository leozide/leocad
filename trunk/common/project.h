#ifndef _PROJECT_H_
#define _PROJECT_H_

#include "object.h"
#include "lc_array.h"
#include "lc_application.h"

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

class Project
{
public:
	Project();
	~Project();

	const lcArray<lcModel*>& GetModels() const
	{
		return mModels;
	}

	lcModel* GetActiveModel() const
	{
		return mActiveModel;
	}

	bool IsModified() const;
	QString GetTitle() const;

	QString GetFileName() const
	{
		return mFileName;
	}

	void SetActiveModel(int ModelIndex);
	bool IsModelNameValid(const QString& Name) const;

	void CreateNewModel();
	void ShowModelListDialog();
	bool Load(const QString& FileName);
	bool Save(const QString& FileName);
	void Merge(Project* Other);

	void Export3DStudio();
	void ExportBrickLink();
	void ExportCSV();
	void ExportPOVRay();
	void ExportWavefront();

protected:
	void GetModelParts(lcArray<lcModelPartsEntry>& ModelParts);

	bool mModified;
	QString mFileName;

	lcArray<lcModel*> mModels;
	lcModel* mActiveModel;

	Q_DECLARE_TR_FUNCTIONS(Project);


public:
	void ExportHTML();
	void UpdateInterface();
	void LoadDefaults();
	void SaveImage();

protected:
	void CreateHTMLPieceList(QTextStream& Stream, lcModel* Model, lcStep Step, bool Images, const QString& ImageExtension);
};

inline lcModel* lcGetActiveModel()
{
	return lcGetActiveProject()->GetActiveModel();
}

#endif // _PROJECT_H_

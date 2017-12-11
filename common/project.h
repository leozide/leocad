#pragma once

#include "object.h"
#include "lc_array.h"
#include "lc_application.h"

#define LC_SCENE_BG				0x010	// Draw bg image
#define LC_SCENE_BG_TILE		0x040	// Tile bg image
#define LC_SCENE_GRADIENT		0x100	// Draw gradient

#define LC_HTML_SINGLEPAGE    0x01
#define LC_HTML_INDEX         0x02
#define LC_HTML_IMAGES        0x04
#define LC_HTML_LISTEND       0x08
#define LC_HTML_LISTSTEP      0x10
#define LC_HTML_HIGHLIGHT     0x20
#define LC_HTML_SUBMODELS     0x40
#define LC_HTML_CURRENT_ONLY  0x80

class lcHTMLExportOptions
{
public:
	lcHTMLExportOptions(const Project* Project);
	void SaveDefaults();

	QString PathName;
	bool TransparentImages;
	bool SubModels;
	bool CurrentOnly;
	bool SinglePage;
	bool IndexPage;
	int StepImagesWidth;
	int StepImagesHeight;
	bool HighlightNewParts;
	bool PartsListStep;
	bool PartsListEnd;
	bool PartsListImages;
	int PartImagesColor;
	int PartImagesWidth;
	int PartImagesHeight;
};

enum LC_MOUSE_TRACK
{
	LC_TRACK_NONE,
	LC_TRACK_LEFT,
	LC_TRACK_RIGHT
};

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

	int GetActiveModelIndex() const
	{
		return mModels.FindIndex(mActiveModel);
	}

	lcModel* GetMainModel() const
	{
		return !mModels.IsEmpty() ? mModels[0] : nullptr;
	}

	bool IsModified() const;
	QString GetTitle() const;

	QString GetFileName() const
	{
		return mFileName;
	}

	void SetActiveModel(int ModelIndex);
	void SetActiveModel(const QString& ModelName);

	lcModel* CreateNewModel(bool ShowModel);
	QString GetNewModelName(QWidget* ParentWidget, const QString& DialogTitle, const QString& CurrentName, const QStringList& ExistingModels) const;
	void ShowModelListDialog();

	bool Load(const QString& FileName);
	bool Save(const QString& FileName);
	bool Save(QTextStream& Stream);
	void Merge(Project* Other);
	bool ImportLDD(const QString& FileName);
	bool ImportInventory(const QByteArray& Inventory, const QString& Name, const QString& Description);

	void SaveImage();
	bool ExportModel(const QString& FileName, lcModel* Model);
	void Export3DStudio(const QString& FileName);
	void ExportBrickLink();
	void ExportCOLLADA(const QString& FileName);
	void ExportCSV();
	void ExportHTML(const lcHTMLExportOptions& Options);
	bool ExportPOVRay(const QString& FileName);
	void ExportWavefront(const QString& FileName);

	void UpdatePieceInfo(PieceInfo* Info) const;

protected:
	QString GetExportFileName(const QString& FileName, const QString& DefaultExtension, const QString& DialogTitle, const QString& DialogFilter) const;
	void GetModelParts(lcArray<lcModelPartsEntry>& ModelParts);
	QImage CreatePartsListImage(lcModel* Model, lcStep Step);
	void CreateHTMLPieceList(QTextStream& Stream, lcModel* Model, lcStep Step, bool Images);

	bool mModified;
	QString mFileName;

	lcArray<lcModel*> mModels;
	lcModel* mActiveModel;

	Q_DECLARE_TR_FUNCTIONS(Project);
};

inline lcModel* lcGetActiveModel()
{
	Project* Project = lcGetActiveProject();
	return Project ? Project->GetActiveModel() : nullptr;
}


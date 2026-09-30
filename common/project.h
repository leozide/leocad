#pragma once

#include "lc_application.h"
#include "lc_result.h"

#define LC_HTML_SINGLEPAGE    0x01
#define LC_HTML_INDEX         0x02
#define LC_HTML_LISTEND       0x08
#define LC_HTML_LISTSTEP      0x10
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
	bool PartsListStep;
	bool PartsListEnd;
};

struct lcImageDialogOptions
{
	QString FilePath;
	int Width;
	int Height;
	int Start;
	int End;
};

struct lcSetInventoryItem
{
	QByteArray PartID;
	int Quantity;
	int ColorCode;
};

class Project
{
public:
	Project(bool IsPreview = false);
	~Project();

	Project(const Project&) = delete;
	Project(Project&&) = delete;
	Project& operator=(const Project&) = delete;
	Project& operator=(Project&&) = delete;

	const std::vector<std::unique_ptr<lcModel>>& GetModels() const
	{
		return mModels;
	}

	lcModel* GetModel(const QString& FileName) const;
	PieceInfo* FindPiece(const std::string& Name) const;
	void RegisterPiece(const std::string& Name, PieceInfo* Info);
	void UnregisterPiece(PieceInfo* Info);
	void ClearPieceIndex();
	void TransferPieceIndexTo(Project* Destination);

	lcModel* GetActiveModel() const
	{
		return mActiveModel;
	}

	lcModel* GetMainModel() const
	{
		return !mModels.empty() ? mModels[0].get() : nullptr;
	}

	bool IsPreview() const
	{
		return mIsPreview;
	}

	bool DefersModelMeshRequests() const
	{
		return mDeferModelMeshRequests;
	}

	void SetDeferModelMeshRequests(bool Defer)
	{
		mDeferModelMeshRequests = Defer;
	}

	bool IsModified() const;
	void MarkAsModified();
	QString GetTitle() const;

	QString GetFileName() const
	{
		return mFileName;
	}

	QString GetImageFileName(bool AllowCurrentFolder) const;

	lcInstructions* GetInstructions();

	void SetActiveModel(lcModel* Model, bool UpdateInterface);
	void SetActiveModel(int ModelIndex, bool UpdateInterface);
	void SetActiveModel(const QString& FileName, bool UpdateInterface);

	lcModel* CreateNewModel(bool ShowModel);
	QString GetNewModelName(QWidget* ParentWidget, const QString& DialogTitle, const QString& CurrentName, const QStringList& ExistingModels) const;
	void ShowModelListDialog();

	bool Load(const QString& FileName, bool ShowErrors);
	lcResult<void> Save(const QString& FileName);
	bool Save(QTextStream& Stream);
	lcResult<void> Merge(const std::vector<Project*>& Sources);
	bool ImportLDD(const QString& FileName);
	bool ImportInventory(const std::vector<lcSetInventoryItem>& SetInventory, const QString& Name, const QString& Description);

	lcResult<void> SaveImage(const lcImageDialogOptions& Options);
	std::vector<PieceInfo*> GetRequiredPieces() const;
	std::vector<lcPiece*> GetRequiredSynthPieces() const;
	lcResult<void> EnsureAssetsReady() const;
	lcResult<void> ExportCurrentStep(const QString& FileName);
	lcResult<void> ExportModel(const QString& FileName, lcModel* Model) const;
	lcResult<void> Export3DStudio(const QString& FileName);
	lcResult<void> ExportBrickLink();
	lcResult<void> ExportCOLLADA(const QString& FileName);
	lcResult<void> ExportCSV(const QString& FileName);
	lcResult<void> ExportHTML(const lcHTMLExportOptions& Options);
	lcResult<void> ExportPOVRay(const QString& FileName);
	lcResult<void> ExportWavefront(const QString& FileName);

	void UpdatePieceInfo(PieceInfo* Info) const;

private:
	static QString MakeExportNameFragment(const QString& Name);
	static QString MakeUniqueExportIdentifier(const QString& BaseName, std::set<QString>& UsedNames, const QString& ReservedSuffix);

protected:
	static bool CanShareMergePiece(const PieceInfo* Existing, const PieceInfo* Incoming);
	QString GetExportFileName(const QString& FileName, const QString& DefaultExtension, const QString& DialogTitle, const QString& DialogFilter) const;

	lcResult<std::vector<lcModelPartsEntry>> GetModelParts();
	void SetFileName(const QString& FileName);

	bool mIsPreview;
	// External projects can parse their models before queuing direct meshes.
	bool mDeferModelMeshRequests = false;
	bool mModified;
	QString mFileName;
	QFileSystemWatcher mFileWatcher;

	std::vector<std::unique_ptr<lcModel>> mModels;
	// Maps normalized local filenames to PieceInfo objects visible in this project.
	// This map does not own them; lcPiecesLibrary tracks and deletes them.
	std::map<std::string, PieceInfo*> mPieceIndex;
	lcModel* mActiveModel;
	std::unique_ptr<lcInstructions> mInstructions;

	Q_DECLARE_TR_FUNCTIONS(Project);
};

inline lcModel* lcGetActiveModel()
{
	const Project* const Project = lcGetActiveProject();
	return Project ? Project->GetActiveModel() : nullptr;
}

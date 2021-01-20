#pragma once

#include "lc_array.h"
#include "lc_math.h"

class Project;
class lcPiecesLibrary;
enum class lcViewSphereLocation;

enum class lcShadingMode
{
	Wireframe,
	Flat,
	DefaultLights,
	Full
};

enum class lcColorTheme
{
	Dark,
	System
};

class lcPreferences
{
public:
	void LoadDefaults();
	void SaveDefaults();
	void SetInterfaceColors(lcColorTheme ColorTheme);

	int mMouseSensitivity;
	lcShadingMode mShadingMode;
	bool mBackgroundGradient;
	quint32 mBackgroundSolidColor;
	quint32 mBackgroundGradientColorTop;
	quint32 mBackgroundGradientColorBottom;
	bool mDrawAxes;
	quint32 mAxesColor;
	quint32 mTextColor;
	quint32 mMarqueeBorderColor;
	quint32 mMarqueeFillColor;
	quint32 mOverlayColor;
	quint32 mActiveViewColor;
	quint32 mInactiveViewColor;
	bool mDrawEdgeLines;
	float mLineWidth;
	bool mAllowLOD;
	float mMeshLODDistance;
	bool mFadeSteps;
	quint32 mFadeStepsColor;
	bool mHighlightNewParts;
	quint32 mHighlightNewPartsColor;
	bool mGridEnabled = true;
	bool mDrawGridStuds;
	quint32 mGridStudColor;
	bool mDrawGridLines;
	int mGridLineSpacing;
	quint32 mGridLineColor;
	bool mDrawGridOrigin;
	bool mFixedAxes;
	bool mViewSphereEnabled;
	lcViewSphereLocation mViewSphereLocation;
	int mViewSphereSize;
	quint32 mViewSphereColor;
	quint32 mViewSphereTextColor;
	quint32 mViewSphereHighlightColor;
	bool mAutoLoadMostRecent;
	bool mRestoreTabLayout;
	lcColorTheme mColorTheme;

	int mPreviewViewSphereEnabled;
	int mPreviewViewSphereSize;
	lcViewSphereLocation mPreviewViewSphereLocation;
	int mDrawPreviewAxis;
};

struct lcCommandLineOptions
{
	bool ParseOK;
	bool Exit;
	bool SaveImage;
	bool SaveWavefront;
	bool Save3DS;
	bool SaveCOLLADA;
	bool SaveHTML;
	bool SetCameraAngles;
	bool SetCameraPosition;
	bool Orthographic;
	bool SetFoV;
	bool SetZPlanes;
	bool SetFadeStepsColor;
	bool SetHighlightColor;
	bool FadeSteps;
	bool ImageHighlight;
	int ImageWidth;
	int ImageHeight;
	int AASamples;
	int StudStyle;
	lcStep ImageStart;
	lcStep ImageEnd;
	lcVector3 CameraPosition[3];
	lcVector2 CameraLatLon;
	float FoV;
	lcVector2 ZPlanes;
	lcViewpoint Viewpoint;
	quint32 FadeStepsColor;
	quint32	HighlightColor;
	QString ImageName;
	QString ModelName;
	QString CameraName;
	QString ProjectName;
	QString SaveWavefrontName;
	QString Save3DSName;
	QString SaveCOLLADAName;
	QString SaveHTMLName;
	QList<QPair<QString, bool>> LibraryPaths;
	QString StdOut;
	QString StdErr;
};

enum class lcStartupMode
{
	ShowWindow,
	Success,
	Error
};

class lcApplication : public QApplication
{
	Q_OBJECT

public:
	lcApplication(int& Argc, char** Argv);
	~lcApplication();

	void SetProject(Project* Project);
	lcCommandLineOptions ParseCommandLineOptions();
	lcStartupMode Initialize(const QList<QPair<QString, bool>>& LibraryPaths);
	void Shutdown();
	void ShowPreferencesDialog();
	void SaveTabLayout() const;

	bool LoadPartsLibrary(const QList<QPair<QString, bool>>& LibraryPaths, bool OnlyUsePaths);

	void SetClipboard(const QByteArray& Clipboard);
	void ExportClipboard(const QByteArray& Clipboard);

	Project* mProject = nullptr;
	lcPiecesLibrary* mLibrary = nullptr;
	lcPreferences mPreferences;
	QByteArray mClipboard;

protected:
	bool InitializeRenderer();
	void ShutdownRenderer();
	void UpdateStyle();
	QString GetTabLayoutKey() const;

	QString mDefaultStyle;
};

extern lcApplication* gApplication;

inline lcPiecesLibrary* lcGetPiecesLibrary()
{
	return gApplication->mLibrary;
}

inline Project* lcGetActiveProject()
{
	return gApplication->mProject;
}

inline lcPreferences& lcGetPreferences()
{
	return gApplication->mPreferences;
}

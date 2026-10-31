#include "lc_global.h"
#include <stdio.h>
#include "lc_application.h"
#include "lc_commandline.h"
#include "lc_library.h"
#include "lc_profile.h"
#include "project.h"
#include "lc_mainwindow.h"
#include "lc_qpreferencesdialog.h"
#include "lc_partselectionwidget.h"
#include "lc_shortcuts.h"
#include "lc_view.h"
#include "camera.h"
#include "lc_previewwidget.h"
#include "lc_colors.h"
#include "lc_qutils.h"

#ifdef Q_OS_WIN
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QtPlatformHeaders\QWindowsWindowFunctions>
#endif
#endif

lcApplication* gApplication;

void lcPreferences::LoadDefaults()
{
	mFixedAxes = lcGetProfileInt(LC_PROFILE_FIXED_AXES);
	mMouseSensitivity = lcGetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY);
	mShadingMode = lcGetProfileShadingMode();
	mHighlightRenderMode = lcGetProfileHighlightRenderMode();
	mBackgroundGradient = lcGetProfileInt(LC_PROFILE_BACKGROUND_GRADIENT);
	mBackgroundSolidColor = lcGetProfileUInt(LC_PROFILE_BACKGROUND_COLOR);
	mBackgroundGradientColorTop = lcGetProfileUInt(LC_PROFILE_GRADIENT_COLOR_TOP);
	mBackgroundGradientColorBottom = lcGetProfileUInt(LC_PROFILE_GRADIENT_COLOR_BOTTOM);
	mDrawAxes = lcGetProfileInt(LC_PROFILE_DRAW_AXES);
	mAxisIconLocation = lcGetProfileAxisIconLocation();
	mTextColor = lcGetProfileUInt(LC_PROFILE_TEXT_COLOR);
	mTextHaloColor = lcGetProfileUInt(LC_PROFILE_TEXT_HALO_COLOR);
	mMarqueeBorderColor = lcGetProfileUInt(LC_PROFILE_MARQUEE_BORDER_COLOR);
	mMarqueeFillColor = lcGetProfileUInt(LC_PROFILE_MARQUEE_FILL_COLOR);
	mOverlayColor = lcGetProfileUInt(LC_PROFILE_OVERLAY_COLOR);
	mActiveViewColor = lcGetProfileUInt(LC_PROFILE_ACTIVE_VIEW_COLOR);
	mInactiveViewColor = lcGetProfileUInt(LC_PROFILE_INACTIVE_VIEW_COLOR);
	mDrawEdgeLines = lcGetProfileInt(LC_PROFILE_DRAW_EDGE_LINES);
	mDrawConditionalLines = lcGetProfileInt(LC_PROFILE_DRAW_CONDITIONAL_LINES);
	mLineWidth = lcGetProfileFloat(LC_PROFILE_LINE_WIDTH);
	mAllowLOD = lcGetProfileInt(LC_PROFILE_ALLOW_LOD);
	mMeshLODDistance = lcGetProfileFloat(LC_PROFILE_LOD_DISTANCE);
	mFadeSteps = lcGetProfileInt(LC_PROFILE_FADE_STEPS);
	mFadeStepsColor = lcGetProfileUInt(LC_PROFILE_FADE_STEPS_COLOR);
	mHighlightNewParts = lcGetProfileInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS);
	mHighlightNewPartsColor = lcGetProfileUInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS_COLOR);
	mDrawGridStuds = lcGetProfileInt(LC_PROFILE_GRID_STUDS);
	mGridStudColor = lcGetProfileUInt(LC_PROFILE_GRID_STUD_COLOR);
	mDrawGridLines = lcGetProfileInt(LC_PROFILE_GRID_LINES);
	mGridLineSpacing = lcGetProfileInt(LC_PROFILE_GRID_LINE_SPACING);
	mGridLineColor = lcGetProfileUInt(LC_PROFILE_GRID_LINE_COLOR);
	mDrawGridOrigin = lcGetProfileInt(LC_PROFILE_GRID_ORIGIN);
	mViewSphereEnabled = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_ENABLED);
	mViewSphereLocation = lcGetProfileViewSphereLocation();
	mViewSphereSize = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_SIZE);
	mViewSphereColor = lcGetProfileUInt(LC_PROFILE_VIEW_SPHERE_COLOR);
	mViewSphereTextColor = lcGetProfileUInt(LC_PROFILE_VIEW_SPHERE_TEXT_COLOR);
	mViewSphereHighlightColor = lcGetProfileUInt(LC_PROFILE_VIEW_SPHERE_HIGHLIGHT_COLOR);
	mAutoLoadMostRecent = lcGetProfileInt(LC_PROFILE_AUTOLOAD_MOSTRECENT);
	mRestoreTabLayout = lcGetProfileInt(LC_PROFILE_RESTORE_TAB_LAYOUT);
	mColorTheme = lcGetProfileColorTheme();
	mPreviewViewSphereEnabled = lcGetProfileInt(LC_PROFILE_PREVIEW_VIEW_SPHERE_ENABLED);
	mPreviewViewSphereSize = lcGetProfileInt(LC_PROFILE_PREVIEW_VIEW_SPHERE_SIZE);
	mPreviewViewSphereLocation = lcGetProfilePreviewViewSphereLocation();
	mDrawPreviewAxis = lcGetProfileInt(LC_PROFILE_PREVIEW_DRAW_AXES);
	mStudCylinderColorEnabled = lcGetProfileInt(LC_PROFILE_STUD_CYLINDER_COLOR_ENABLED);
	mStudCylinderColor = lcGetProfileUInt(LC_PROFILE_STUD_CYLINDER_COLOR);
	mPartEdgeColorEnabled = lcGetProfileInt(LC_PROFILE_PART_EDGE_COLOR_ENABLED);
	mPartEdgeColor = lcGetProfileUInt(LC_PROFILE_PART_EDGE_COLOR);
	mBlackEdgeColorEnabled = lcGetProfileInt(LC_PROFILE_BLACK_EDGE_COLOR_ENABLED);
	mBlackEdgeColor = lcGetProfileUInt(LC_PROFILE_BLACK_EDGE_COLOR);
	mDarkEdgeColorEnabled = lcGetProfileInt(LC_PROFILE_DARK_EDGE_COLOR_ENABLED);
	mDarkEdgeColor = lcGetProfileUInt(LC_PROFILE_DARK_EDGE_COLOR);
	mPartEdgeContrast = lcGetProfileFloat(LC_PROFILE_PART_EDGE_CONTRAST);
	mPartColorValueLDIndex = lcGetProfileFloat(LC_PROFILE_PART_COLOR_VALUE_LD_INDEX);
	mAutomateEdgeColor = lcGetProfileInt(LC_PROFILE_AUTOMATE_EDGE_COLOR);
	mObjectSelectedColor = lcGetProfileUInt(LC_PROFILE_OBJECT_SELECTED_COLOR);
	mObjectFocusedColor = lcGetProfileUInt(LC_PROFILE_OBJECT_FOCUSED_COLOR);
	mCameraColor = lcGetProfileUInt(LC_PROFILE_CAMERA_COLOR);
	mLightColor = lcGetProfileUInt(LC_PROFILE_LIGHT_COLOR);
	mControlPointColor = lcGetProfileUInt(LC_PROFILE_CONTROL_POINT_COLOR);
	mControlPointFocusedColor = lcGetProfileUInt(LC_PROFILE_CONTROL_POINT_FOCUSED_COLOR);
}

void lcPreferences::SaveDefaults()
{
	lcSetProfileInt(LC_PROFILE_FIXED_AXES, mFixedAxes);
	lcSetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY, mMouseSensitivity);
	lcSetProfileShadingMode(mShadingMode);
	lcSetProfileHighlightRenderMode(mHighlightRenderMode);
	lcSetProfileInt(LC_PROFILE_DRAW_AXES, mDrawAxes);
	lcSetProfileAxisIconLocation(mAxisIconLocation);
	lcSetProfileUInt(LC_PROFILE_TEXT_COLOR, mTextColor);
	lcSetProfileUInt(LC_PROFILE_TEXT_HALO_COLOR, mTextHaloColor);
	lcSetProfileInt(LC_PROFILE_BACKGROUND_GRADIENT, mBackgroundGradient);
	lcSetProfileUInt(LC_PROFILE_BACKGROUND_COLOR, mBackgroundSolidColor);
	lcSetProfileUInt(LC_PROFILE_GRADIENT_COLOR_TOP, mBackgroundGradientColorTop);
	lcSetProfileUInt(LC_PROFILE_GRADIENT_COLOR_BOTTOM, mBackgroundGradientColorBottom);
	lcSetProfileUInt(LC_PROFILE_MARQUEE_BORDER_COLOR, mMarqueeBorderColor);
	lcSetProfileUInt(LC_PROFILE_MARQUEE_FILL_COLOR, mMarqueeFillColor);
	lcSetProfileUInt(LC_PROFILE_OVERLAY_COLOR, mOverlayColor);
	lcSetProfileUInt(LC_PROFILE_ACTIVE_VIEW_COLOR, mActiveViewColor);
	lcSetProfileUInt(LC_PROFILE_INACTIVE_VIEW_COLOR, mInactiveViewColor);
	lcSetProfileInt(LC_PROFILE_DRAW_EDGE_LINES, mDrawEdgeLines);
	lcSetProfileInt(LC_PROFILE_DRAW_CONDITIONAL_LINES, mDrawConditionalLines);
	lcSetProfileFloat(LC_PROFILE_LINE_WIDTH, mLineWidth);
	lcSetProfileInt(LC_PROFILE_ALLOW_LOD, mAllowLOD);
	lcSetProfileFloat(LC_PROFILE_LOD_DISTANCE, mMeshLODDistance);
	lcSetProfileInt(LC_PROFILE_FADE_STEPS, mFadeSteps);
	lcSetProfileUInt(LC_PROFILE_FADE_STEPS_COLOR, mFadeStepsColor);
	lcSetProfileInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS, mHighlightNewParts);
	lcSetProfileUInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS_COLOR, mHighlightNewPartsColor);
	lcSetProfileInt(LC_PROFILE_GRID_STUDS, mDrawGridStuds);
	lcSetProfileUInt(LC_PROFILE_GRID_STUD_COLOR, mGridStudColor);
	lcSetProfileInt(LC_PROFILE_GRID_LINES, mDrawGridLines);
	lcSetProfileInt(LC_PROFILE_GRID_LINE_SPACING, mGridLineSpacing);
	lcSetProfileUInt(LC_PROFILE_GRID_LINE_COLOR, mGridLineColor);
	lcSetProfileInt(LC_PROFILE_GRID_ORIGIN, mDrawGridOrigin);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_ENABLED, mViewSphereEnabled);
	lcSetProfileViewSphereLocation(mViewSphereLocation);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_SIZE, mViewSphereSize);
	lcSetProfileUInt(LC_PROFILE_VIEW_SPHERE_COLOR, mViewSphereColor);
	lcSetProfileUInt(LC_PROFILE_VIEW_SPHERE_TEXT_COLOR, mViewSphereTextColor);
	lcSetProfileUInt(LC_PROFILE_VIEW_SPHERE_HIGHLIGHT_COLOR, mViewSphereHighlightColor);
	lcSetProfileInt(LC_PROFILE_AUTOLOAD_MOSTRECENT, mAutoLoadMostRecent);
	lcSetProfileInt(LC_PROFILE_RESTORE_TAB_LAYOUT, mRestoreTabLayout);
	lcSetProfileColorTheme(mColorTheme);
	lcSetProfileInt(LC_PROFILE_PREVIEW_VIEW_SPHERE_ENABLED, mPreviewViewSphereEnabled);
	lcSetProfileInt(LC_PROFILE_PREVIEW_VIEW_SPHERE_SIZE, mPreviewViewSphereSize);
	lcSetProfilePreviewViewSphereLocation(mPreviewViewSphereLocation);
	lcSetProfileInt(LC_PROFILE_PREVIEW_DRAW_AXES, mDrawPreviewAxis);
	lcSetProfileInt(LC_PROFILE_STUD_CYLINDER_COLOR_ENABLED, mStudCylinderColorEnabled);
	lcSetProfileUInt(LC_PROFILE_STUD_CYLINDER_COLOR, mStudCylinderColor);
	lcSetProfileInt(LC_PROFILE_PART_EDGE_COLOR_ENABLED, mPartEdgeColorEnabled);
	lcSetProfileUInt(LC_PROFILE_PART_EDGE_COLOR, mPartEdgeColor);
	lcSetProfileInt(LC_PROFILE_BLACK_EDGE_COLOR_ENABLED, mBlackEdgeColorEnabled);
	lcSetProfileUInt(LC_PROFILE_BLACK_EDGE_COLOR, mBlackEdgeColor);
	lcSetProfileInt(LC_PROFILE_DARK_EDGE_COLOR_ENABLED, mDarkEdgeColorEnabled);
	lcSetProfileUInt(LC_PROFILE_DARK_EDGE_COLOR, mDarkEdgeColor);
	lcSetProfileFloat(LC_PROFILE_PART_EDGE_CONTRAST, mPartEdgeContrast);
	lcSetProfileFloat(LC_PROFILE_PART_COLOR_VALUE_LD_INDEX, mPartColorValueLDIndex);
	lcSetProfileInt(LC_PROFILE_AUTOMATE_EDGE_COLOR, mAutomateEdgeColor);
	lcSetProfileUInt(LC_PROFILE_OBJECT_SELECTED_COLOR, mObjectSelectedColor);
	lcSetProfileUInt(LC_PROFILE_OBJECT_FOCUSED_COLOR, mObjectFocusedColor);
	lcSetProfileUInt(LC_PROFILE_CAMERA_COLOR, mCameraColor);
	lcSetProfileUInt(LC_PROFILE_LIGHT_COLOR, mLightColor);
	lcSetProfileUInt(LC_PROFILE_CONTROL_POINT_COLOR, mControlPointColor);
	lcSetProfileUInt(LC_PROFILE_CONTROL_POINT_FOCUSED_COLOR, mControlPointFocusedColor);
}

void lcPreferences::SetInterfaceColors(lcColorTheme ColorTheme)
{
	const bool SystemDark = (ColorTheme == lcColorTheme::System) && (QApplication::palette().color(QPalette::Window).lightness() < 128);

	if (ColorTheme == lcColorTheme::Dark || SystemDark)
	{
		mTextColor = LC_RGBA(214, 220, 227, 255);
		mTextHaloColor = LC_RGBA(17, 19, 24, 160);
		mBackgroundSolidColor = LC_RGB(49, 52, 55);
		mBackgroundGradientColorTop = LC_RGB(0, 0, 191);
		mBackgroundGradientColorBottom = LC_RGB(255, 255, 255);
		mOverlayColor = lcGetProfileUInt(LC_PROFILE_OVERLAY_COLOR);
		mActiveViewColor = LC_RGBA(41, 128, 185, 255);
		mGridStudColor = LC_RGBA(24, 24, 24, 192);
		mGridLineColor = LC_RGBA(24, 24, 24, 255);
		mViewSphereColor = LC_RGBA(27, 31, 36, 255);
		mViewSphereTextColor = LC_RGBA(232, 237, 242, 255);
		mViewSphereHighlightColor = LC_RGBA(41, 128, 185, 255);
	}
	else
	{
		mTextColor = LC_RGBA(24, 33, 43, 255);
		mTextHaloColor = LC_RGBA(248, 250, 252, 204);
		mBackgroundSolidColor = LC_RGB(255, 255, 255);
		mBackgroundGradientColorTop = LC_RGB(54, 72, 95);
		mBackgroundGradientColorBottom = LC_RGB(49, 52, 55);
		mOverlayColor = LC_RGBA(0, 0, 0, 255);
		mActiveViewColor = LC_RGBA(255, 0, 0, 255);
		mGridStudColor = LC_RGBA(64, 64, 64, 192);
		mGridLineColor = LC_RGBA(0, 0, 0, 255);
		mViewSphereColor = LC_RGBA(248, 250, 252, 255);
		mViewSphereTextColor = LC_RGBA(24, 33, 43, 255);
		mViewSphereHighlightColor = LC_RGBA(41, 128, 185, 255);
	}
}

lcApplication::lcApplication(int& Argc, char** Argv)
	: QApplication(Argc, Argv)
{
	setApplicationDisplayName(QLatin1String("LeoCAD"));

	gApplication = this;
	mDefaultStyle = style()->objectName();

	lcProfileInit();

	mPreferences.LoadDefaults();
}

lcApplication::~lcApplication()
{
	gApplication = nullptr;
}

void lcApplication::ResetInterfaceColors(lcPreferences& Preferences, lcColorTheme ColorTheme)
{
	const bool RestoreDarkTheme = ColorTheme == lcColorTheme::System && mCustomStyleApplied;

	if (RestoreDarkTheme)
		RestoreSystemStyle();

	Preferences.SetInterfaceColors(ColorTheme);

	if (RestoreDarkTheme)
		UpdateStyle();
}

void lcApplication::RestoreSystemStyle()
{
	qApp->setStyleSheet(QString());
	QApplication::setStyle(mDefaultStyle);

	// Clear our explicit color overrides so Qt resolves the current desktop palette.
	QPalette SystemPalette;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	SystemPalette.setResolveMask(0);
#else
	SystemPalette.resolve(0);
#endif

	QApplication::setPalette(SystemPalette);

	mCustomStyleApplied = false;
}

void lcApplication::UpdateStyle()
{
	if (mPreferences.mColorTheme == lcColorTheme::Dark)
	{
		if (!QApplication::setStyle("fusion"))
			return;

		QPalette Palette = QApplication::palette();

		Palette.setColor(QPalette::Window, QColor(49, 52, 55));
		Palette.setColor(QPalette::WindowText, QColor(240, 240, 240));
		Palette.setColor(QPalette::Base, QColor(35, 38, 41));
		Palette.setColor(QPalette::AlternateBase, QColor(44, 47, 50));
		Palette.setColor(QPalette::ToolTipBase, QColor(224, 224, 244));
		Palette.setColor(QPalette::ToolTipText, QColor(58, 58, 58));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
		Palette.setColor(QPalette::PlaceholderText, QColor(100, 100, 100));
#endif
		Palette.setColor(QPalette::Text, QColor(224, 224, 224));
		Palette.setColor(QPalette::Button, QColor(45, 48, 51));
		Palette.setColor(QPalette::ButtonText, QColor(224, 224, 244));
		Palette.setColor(QPalette::Light, QColor(65, 65, 65));
		Palette.setColor(QPalette::Midlight, QColor(62, 62, 62));
		Palette.setColor(QPalette::Dark, QColor(35, 35, 35));
		Palette.setColor(QPalette::Mid, QColor(50, 50, 50));
		Palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
//		Palette.setColor(QPalette::Highlight, QColor(46, 108, 219));
		Palette.setColor(QPalette::Highlight, QColor(41, 128, 185));
		Palette.setColor(QPalette::HighlightedText, QColor(232, 232, 232));
		Palette.setColor(QPalette::Link, QColor(41, 128, 185));

		Palette.setColor(QPalette::Disabled, QPalette::Text, QColor(128, 128, 128));
		Palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128));
		Palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));

		QApplication::setPalette(Palette);
		mCustomStyleApplied = true;

		QFile StylesheetFile(QLatin1String(":/stylesheet/stylesheet.qss"));

		if (StylesheetFile.open(QIODevice::ReadOnly))
		{
			QString Stylesheet = QString::fromLatin1(StylesheetFile.readAll());
			qApp->setStyleSheet(Stylesheet);
		}
	}
	else if (mCustomStyleApplied)
	{
		RestoreSystemStyle();
	}
}

void lcApplication::SaveTabLayout() const
{
	if (!gMainWindow || !mProject || mProject->GetFileName().isEmpty())
		return;

	QSettings Settings;
	QByteArray TabLayout = gMainWindow->GetTabLayout();

	Settings.setValue(GetTabLayoutKey(), TabLayout);
}

QString lcApplication::GetTabLayoutKey() const
{
	if (mProject)
	{
		QString FileName = mProject->GetFileName();
		if (!FileName.isEmpty())
		{
			FileName.replace('\\', '?');
			FileName.replace('/', '?');
			return QString("TabLayouts/%1").arg(FileName);
		}
	}

	return QString();
}

void lcApplication::SetProject(Project* Project)
{
	SaveTabLayout();

	if (gMainWindow)
	{
		gMainWindow->RemoveAllModelTabs();

		if (gMainWindow->GetPreviewWidget())
			gMainWindow->GetPreviewWidget()->ClearPreview();
	}

	delete mProject;
	mProject = Project;
	mProject->UpdateFileWatcher();

	if (gMainWindow)
		gMainWindow->ScheduleAssetUpdate();

	Project->SetActiveModel(0, true);
	lcGetPiecesLibrary()->RemoveTemporaryPieceData();

	if (mProject && !mProject->GetFileName().isEmpty() && mPreferences.mRestoreTabLayout)
	{
		QSettings Settings;
		QByteArray TabLayout = Settings.value(GetTabLayoutKey()).toByteArray();

		if (gMainWindow)
			gMainWindow->RestoreTabLayout(TabLayout);
	}
}

void lcApplication::SetClipboard(const QByteArray& Clipboard)
{
	mClipboard = Clipboard;
	gMainWindow->UpdatePaste(!mClipboard.isEmpty());
}

void lcApplication::ExportClipboard(const QByteArray& Clipboard)
{
	QMimeData* MimeData = new QMimeData();

	MimeData->setData("application/vnd.leocad-clipboard", Clipboard);
	QApplication::clipboard()->setMimeData(MimeData);

	SetClipboard(Clipboard);
}

bool lcApplication::LoadPartsLibrary(const QList<QPair<QString, bool>>& LibraryPaths, bool OnlyUsePaths, lcStudStyle StudStyle, bool StudCylinderColorEnabled)
{
	if (mLibrary == nullptr)
		mLibrary = new lcPiecesLibrary(StudStyle, StudCylinderColorEnabled);

	const bool ShowProgress = gMainWindow != nullptr;

	if (!OnlyUsePaths)
	{
		char* EnvPath = getenv("LEOCAD_LIB");

		if (EnvPath && EnvPath[0])
			return mLibrary->Load(EnvPath, ShowProgress, false);

		QString CustomPath = lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);

		if (!CustomPath.isEmpty())
			return mLibrary->Load(CustomPath, ShowProgress, false);
	}

	for (const QPair<QString, bool>& LibraryPathEntry : LibraryPaths)
		if (mLibrary->Load(LibraryPathEntry.first, ShowProgress, LibraryPathEntry.second))
			return true;

	return false;
}

lcStartupMode lcApplication::Initialize(const QList<QPair<QString, bool>>& LibraryPaths)
{
	lcCommandLineOptions Options(QCoreApplication::arguments());
	QTextStream StdErr(stderr, QIODevice::WriteOnly);
	QTextStream StdOut(stdout, QIODevice::WriteOnly);

	if (!Options.StdErr.isEmpty())
	{
		StdErr << Options.StdErr;
		StdErr.flush();
	}

	if (!Options.StdOut.isEmpty())
	{
		StdOut << Options.StdOut;
		StdOut.flush();
	}

	if (!Options.ParseOK)
		return lcStartupMode::Error;

	if (Options.Exit)
		return lcStartupMode::Success;

	const bool SaveAndExit = (Options.SaveImage || Options.SaveWavefront || Options.Save3DS || Options.SaveCOLLADA || Options.SaveCSV || Options.SaveHTML);

#ifdef Q_OS_IOS
	if (Options.SaveImage)
	{
		StdErr << tr("Command line image export is not supported on iOS.\n");
		return lcStartupMode::Error;
	}
#endif

#ifndef Q_OS_IOS
	const bool CreateRenderResources = !SaveAndExit || Options.SaveImage || Options.SaveHTML;
	const lcResult<void> RendererResult = lcContext::InitializeRenderer(CreateRenderResources);

	if (!RendererResult)
	{
		StdErr << RendererResult.error() << '\n';

		if (!SaveAndExit)
			QMessageBox::critical(nullptr, tr("Initialize Renderer"), RendererResult.error());

		return lcStartupMode::Error;
	}

	emit RendererInitialized();
#endif

	bool OutputFailed = false;

	if (!SaveAndExit)
	{
		UpdateStyle();

		gMainWindow = new lcMainWindow();

		lcLoadDefaultKeyboardShortcuts();
		lcLoadDefaultMouseShortcuts();
	}

	mPreferences.mShadingMode = Options.ShadingMode;
	mPreferences.mLineWidth = Options.LineWidth;
	mPreferences.mStudCylinderColorEnabled = Options.StudCylinderColorEnabled;
	mPreferences.mStudCylinderColor = Options.StudCylinderColor;
	mPreferences.mPartEdgeColorEnabled = Options.PartEdgeColorEnabled;
	mPreferences.mPartEdgeColor = Options.PartEdgeColor;
	mPreferences.mBlackEdgeColorEnabled = Options.BlackEdgeColorEnabled;
	mPreferences.mBlackEdgeColor = Options.BlackEdgeColor;
	mPreferences.mDarkEdgeColorEnabled = Options.DarkEdgeColorEnabled;
	mPreferences.mDarkEdgeColor = Options.DarkEdgeColor;
	mPreferences.mPartEdgeContrast = Options.PartEdgeContrast;
	mPreferences.mPartColorValueLDIndex = Options.PartColorValueLDIndex;
	mPreferences.mAutomateEdgeColor = Options.AutomateEdgeColor;

	if (!LoadPartsLibrary(Options.LibraryPaths.isEmpty() ? LibraryPaths : Options.LibraryPaths, !Options.LibraryPaths.isEmpty(), Options.StudStyle, Options.StudCylinderColorEnabled))
	{
		QString Message;

		if (mLibrary->LoadBuiltinPieceLibrary())
			Message = tr("LeoCAD could not find a compatible Pieces Library so only a small number of pieces will be available.\n\nPlease visit https://www.leocad.org for information on how to download and install a library.");
		else
			Message = tr("LeoCAD could not load Pieces Library.\n\nPlease visit https://www.leocad.org for information on how to download and install a library.");

		if (gMainWindow)
			QMessageBox::warning(gMainWindow, tr("Load Pieces Library"), Message);
		else
		{
			StdErr << Message << "\n";
			StdErr.flush();
		}
	}

	if (!SaveAndExit)
		gMainWindow->CreateWidgets();

	Project* NewProject = new Project();
	SetProject(NewProject);

	if (!SaveAndExit && Options.ProjectName.isEmpty() && lcGetProfileInt(LC_PROFILE_AUTOLOAD_MOSTRECENT))
		Options.ProjectName = lcGetProfileString(LC_PROFILE_RECENT_FILE1);

	bool ProjectLoaded = false;

	if (!Options.ProjectName.isEmpty())
	{
		if (gMainWindow)
			gMainWindow->OpenProject(Options.ProjectName);
		else
		{
			Project* LoadedProject = new Project();

			if (LoadedProject->Load(Options.ProjectName, false))
			{
				SetProject(LoadedProject);
				ProjectLoaded = true;
			}
			else
			{
				delete LoadedProject;
			}
		}
	}

	if (ProjectLoaded)
	{
		if (!Options.ModelName.isEmpty())
			mProject->SetActiveModel(Options.ModelName, true);

		std::unique_ptr<lcView> ActiveView;

		if (Options.SaveImage)
		{
			lcModel* Model;

			if (!Options.ModelName.isEmpty())
			{
				Model = mProject->GetModel(Options.ModelName);

				if (!Model)
				{
					StdErr << tr("Error: model '%1' does not exist.\n").arg(Options.ModelName);
					return lcStartupMode::Error;
				}
			}
			else
				Model = mProject->GetMainModel();

			ActiveView = std::unique_ptr<lcView>(new lcView(lcViewType::View, Model));
			ActiveView->SetBackgroundOptions(Options.ImageBackground);

			ActiveView->SetOffscreenContext();
			ActiveView->MakeCurrent();
		}

		if (Options.SaveImage)
			ActiveView->SetSize(Options.ImageWidth, Options.ImageHeight);

		if (ActiveView)
		{
			if (!Options.CameraName.isEmpty())
				ActiveView->SetCamera(Options.CameraName);
			else
			{
				ActiveView->SetCameraProjection(Options.Orthographic ? lcCameraProjection::Orthographic : lcCameraProjection::Perspective);

				if (Options.SetFoV)
					ActiveView->GetCamera()->m_fovy = Options.FoV;

				if (Options.SetZPlanes)
				{
					lcCamera* Camera = ActiveView->GetCamera();

					Camera->m_zNear = Options.ZPlanes[0];
					Camera->m_zFar = Options.ZPlanes[1];
				}

				if (Options.Viewpoint != lcViewpoint::Count)
					ActiveView->SetViewpoint(Options.Viewpoint);
				else if (Options.SetCameraAngles)
					ActiveView->SetCameraAngles(Options.CameraLatLon[0], Options.CameraLatLon[1]);
				else if (Options.SetCameraPosition)
					ActiveView->SetViewpoint(Options.CameraPosition[0], Options.CameraPosition[1], Options.CameraPosition[2]);
			}
		}

		if (Options.SaveImage)
		{
			lcModel* ActiveModel = ActiveView->GetModel();

			if (Options.ImageName.isEmpty())
				Options.ImageName = mProject->GetImageFileName(true);

			if (Options.ImageEnd < Options.ImageStart)
				Options.ImageEnd = Options.ImageStart;
			else if (Options.ImageStart > Options.ImageEnd)
				Options.ImageStart = Options.ImageEnd;

			if ((Options.ImageStart == 0) && (Options.ImageEnd == 0))
				Options.ImageStart = Options.ImageEnd = ActiveModel->GetCurrentStep();
			else if ((Options.ImageStart == 0) && (Options.ImageEnd != 0))
				Options.ImageStart = Options.ImageEnd;
			else if ((Options.ImageStart != 0) && (Options.ImageEnd == 0))
				Options.ImageEnd = Options.ImageStart;

			if (Options.ImageStart > 255)
				Options.ImageStart = 255;

			if (Options.ImageEnd > 255)
				Options.ImageEnd = 255;

			QString Frame;

			if (Options.ImageStart != Options.ImageEnd)
			{
				QString Extension = QFileInfo(Options.ImageName).suffix();
				Frame = Options.ImageName.left(Options.ImageName.length() - Extension.length() - 1) + QLatin1String("%1.") + Extension;
			}
			else
				Frame = Options.ImageName;

			mPreferences.mFadeSteps = Options.FadeSteps;
			if (Options.SetFadeStepsColor)
				mPreferences.mFadeStepsColor = Options.FadeStepsColor;
			mPreferences.mHighlightNewParts = Options.ImageHighlight;
			if (Options.SetHighlightColor)
				mPreferences.mHighlightNewPartsColor = Options.HighlightColor;

			ActiveModel->WaitForAssets();

			const bool UsesCameraPosition = Options.SetCameraPosition && Options.Viewpoint == lcViewpoint::Count && !Options.SetCameraAngles;

			if (Options.CameraName.isEmpty() && !UsesCameraPosition)
			{
				const lcStep FitStep = Options.ImageStart == Options.ImageEnd ? Options.ImageStart : ActiveModel->GetLastStep();

				ActiveView->ZoomExtentsAtStep(FitStep, lcGeometryBoundsMode::AvailableOnly, !Options.SetZPlanes);
			}

			auto ProgressCallback = [&StdOut](const QString& FileName)
			{
				StdOut << tr("Saved '%1'.\n").arg(FileName);
			};

			const lcResult<void> ImagesSaved = ActiveView->SaveStepImages(Frame, Options.ImageStart != Options.ImageEnd, Options.ImageStart, Options.ImageEnd, ProgressCallback);

			if (!ImagesSaved)
			{
				StdErr << ImagesSaved.error() << '\n';

				return lcStartupMode::Error;
			}
		}

		if (Options.SaveWavefront)
		{
			QString FileName;

			if (!Options.SaveWavefrontName.isEmpty())
				FileName = Options.SaveWavefrontName;
			else
				FileName = Options.ProjectName;

			QString Extension = QFileInfo(FileName).suffix().toLower();

			if (Extension.isEmpty())
			{
				FileName += ".obj";
			}
			else if (Extension != "obj")
			{
				FileName = FileName.left(FileName.length() - Extension.length() - 1);
				FileName += ".obj";
			}

			lcResult<void> ExportResult = mProject->ExportWavefront(FileName);

			if (ExportResult)
				StdOut << tr("Saved '%1'.\n").arg(FileName);
			else
			{
				OutputFailed = true;

				if (!ExportResult.error().isEmpty())
					StdErr << ExportResult.error() << '\n';
			}
		}

		if (Options.Save3DS)
		{
			QString FileName;

			if (!Options.Save3DSName.isEmpty())
				FileName = Options.Save3DSName;
			else
				FileName = Options.ProjectName;

			QString Extension = QFileInfo(FileName).suffix().toLower();

			if (Extension.isEmpty())
			{
				FileName += ".3ds";
			}
			else if (Extension != "3ds")
			{
				FileName = FileName.left(FileName.length() - Extension.length() - 1);
				FileName += ".3ds";
			}

			lcResult<void> ExportResult = mProject->Export3DStudio(FileName);

			if (ExportResult)
				StdOut << tr("Saved '%1'.\n").arg(FileName);
			else
			{
				OutputFailed = true;

				if (!ExportResult.error().isEmpty())
					StdErr << ExportResult.error() << '\n';
			}
		}

		if (Options.SaveCOLLADA)
		{
			QString FileName;

			if (!Options.SaveCOLLADAName.isEmpty())
				FileName = Options.SaveCOLLADAName;
			else
				FileName = Options.ProjectName;

			QString Extension = QFileInfo(FileName).suffix().toLower();

			if (Extension.isEmpty())
			{
				FileName += ".dae";
			}
			else if (Extension != "dae")
			{
				FileName = FileName.left(FileName.length() - Extension.length() - 1);
				FileName += ".dae";
			}

			lcResult<void> ExportResult = mProject->ExportCOLLADA(FileName);

			if (ExportResult)
				StdOut << tr("Saved '%1'.\n").arg(FileName);
			else
			{
				OutputFailed = true;

				if (!ExportResult.error().isEmpty())
					StdErr << ExportResult.error() << '\n';
			}
		}

		if (Options.SaveCSV)
		{
			QString FileName;

			if (!Options.SaveCSVName.isEmpty())
				FileName = Options.SaveCSVName;
			else
				FileName = Options.ProjectName;

			QString Extension = QFileInfo(FileName).suffix().toLower();

			if (Extension.isEmpty())
			{
				FileName += ".csv";
			}
			else if (Extension != "csv")
			{
				FileName = FileName.left(FileName.length() - Extension.length() - 1);
				FileName += ".csv";
			}

			lcResult<void> ExportResult = mProject->ExportCSV(FileName);

			if (ExportResult)
				StdOut << tr("Saved '%1'.\n").arg(FileName);
			else
			{
				OutputFailed = true;

				if (!ExportResult.error().isEmpty())
					StdErr << ExportResult.error() << '\n';
			}
		}

		if (Options.SaveHTML)
		{
			lcHTMLExportOptions HTMLOptions(mProject);

			if (!Options.SaveHTMLName.isEmpty())
				HTMLOptions.PathName = Options.SaveHTMLName;

			lcResult<void> ExportResult = mProject->ExportHTML(HTMLOptions);

			if (ExportResult)
				StdOut << tr("Saved '%1'.\n").arg(HTMLOptions.PathName);
			else
			{
				OutputFailed = true;

				if (!ExportResult.error().isEmpty())
					StdErr << ExportResult.error() << '\n';
			}
		}
	}

	if (!SaveAndExit)
	{
		gMainWindow->SetColorIndex(lcGetColorIndex(7));
		gMainWindow->GetPartSelectionWidget()->SetDefaultPart();

#ifdef Q_OS_IOS
		const lcResult<void> RendererResult = lcContext::InitializeRenderer(true);

		if (!RendererResult)
		{
			StdErr << RendererResult.error() << '\n';
			QMessageBox::critical(gMainWindow, tr("Initialize Renderer"), RendererResult.error());
			return lcStartupMode::Error;
		}

		emit RendererInitialized();
#endif

		gMainWindow->show();

#ifdef Q_OS_WIN
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
		QWindowsWindowFunctions::setHasBorderInFullScreen(gMainWindow->windowHandle(), true);
#endif
#endif
	}

	if (OutputFailed)
		return lcStartupMode::Error;

	return SaveAndExit ? lcStartupMode::Success : lcStartupMode::ShowWindow;
}

void lcApplication::Shutdown()
{
	delete gMainWindow;
	gMainWindow = nullptr;

	delete mProject;
	mProject = nullptr;

	delete mLibrary;
	mLibrary = nullptr;

	lcContext::ShutdownRenderer();
}

void lcApplication::ShowPreferencesDialog()
{
	lcPreferencesDialogOptions Options;
	int CurrentAASamples = lcGetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES);
	lcStudStyle CurrentStudStyle = lcGetPiecesLibrary()->GetStudStyle();

	Options.Preferences = mPreferences;

	Options.LibraryPath = lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);
	Options.MinifigSettingsPath = lcGetProfileString(LC_PROFILE_MINIFIG_SETTINGS);
	Options.ColorConfigPath = lcGetProfileString(LC_PROFILE_COLOR_CONFIG);
	Options.POVRayPath = lcGetProfileString(LC_PROFILE_POVRAY_PATH);
	Options.LGEOPath = lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH);
	Options.DefaultAuthor = lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME);
	Options.Language = lcGetProfileString(LC_PROFILE_LANGUAGE);
	Options.CheckForUpdates = lcGetProfileInt(LC_PROFILE_CHECK_UPDATES);

	Options.AASamples = CurrentAASamples;
	Options.StudStyle = CurrentStudStyle;

	Options.Categories = gCategories;
	Options.CategoriesModified = false;
	Options.CategoriesDefault = false;

	Options.KeyboardShortcuts = gKeyboardShortcuts;
	Options.KeyboardShortcutsModified = false;
	Options.KeyboardShortcutsDefault = false;
	Options.MouseShortcuts = gMouseShortcuts;
	Options.MouseShortcutsModified = false;
	Options.MouseShortcutsDefault = false;

	lcPreferencesDialog Dialog(gMainWindow, &Options);
	if (Dialog.exec() != QDialog::Accepted)
		return;

	bool LanguageChanged = Options.Language != lcGetProfileString(LC_PROFILE_LANGUAGE);
	bool LibraryChanged = Options.LibraryPath != lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);
	bool ColorConfigChanged = Options.ColorConfigPath != lcGetProfileString(LC_PROFILE_COLOR_CONFIG);
	bool AAChanged = CurrentAASamples != Options.AASamples;
	bool StudStyleChanged = CurrentStudStyle != Options.StudStyle;
	bool StudCylinderColorEnabledChanged = Options.Preferences.mStudCylinderColorEnabled != mPreferences.mStudCylinderColorEnabled;
	bool EdgeColorSettingsChanged = Options.Preferences.mAutomateEdgeColor != mPreferences.mAutomateEdgeColor;
	EdgeColorSettingsChanged |= Options.Preferences.mPartEdgeContrast != mPreferences.mPartEdgeContrast;
	EdgeColorSettingsChanged |= Options.Preferences.mPartColorValueLDIndex != mPreferences.mPartColorValueLDIndex;
	EdgeColorSettingsChanged |= Options.Preferences.mStudCylinderColorEnabled != mPreferences.mStudCylinderColorEnabled;
	EdgeColorSettingsChanged |= Options.Preferences.mStudCylinderColor != mPreferences.mStudCylinderColor;
	EdgeColorSettingsChanged |= Options.Preferences.mPartEdgeColorEnabled != mPreferences.mPartEdgeColorEnabled;
	EdgeColorSettingsChanged |= Options.Preferences.mPartEdgeColor != mPreferences.mPartEdgeColor;
	EdgeColorSettingsChanged |= Options.Preferences.mBlackEdgeColorEnabled != mPreferences.mBlackEdgeColorEnabled;
	EdgeColorSettingsChanged |= Options.Preferences.mBlackEdgeColor != mPreferences.mBlackEdgeColor;
	EdgeColorSettingsChanged |= Options.Preferences.mDarkEdgeColorEnabled != mPreferences.mDarkEdgeColorEnabled;
	EdgeColorSettingsChanged |= Options.Preferences.mDarkEdgeColor != mPreferences.mDarkEdgeColor;


	mPreferences = Options.Preferences;

	mPreferences.SaveDefaults();
	UpdateStyle();

	lcSetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME, Options.DefaultAuthor);
	lcSetProfileString(LC_PROFILE_PARTS_LIBRARY, Options.LibraryPath);
	lcSetProfileString(LC_PROFILE_MINIFIG_SETTINGS, Options.MinifigSettingsPath);
	lcSetProfileString(LC_PROFILE_POVRAY_PATH, Options.POVRayPath);
	lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, Options.LGEOPath);
	lcSetProfileString(LC_PROFILE_LANGUAGE, Options.Language);
	lcSetProfileInt(LC_PROFILE_CHECK_UPDATES, Options.CheckForUpdates);
	lcSetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES, Options.AASamples);
	lcSetProfileStudStyle(Options.StudStyle);

	if (LanguageChanged || LibraryChanged || AAChanged)
		QMessageBox::information(gMainWindow, tr("Preferences"), tr("Some changes will only take effect the next time you start LeoCAD."));

	if (Options.CategoriesModified)
	{
		if (Options.CategoriesDefault)
			lcResetDefaultCategories();
		else
		{
			gCategories = Options.Categories;
			lcSaveDefaultCategories();
		}

		gMainWindow->UpdateCategories();
	}

	if (Options.KeyboardShortcutsModified)
	{
		if (Options.KeyboardShortcutsDefault)
			lcResetDefaultKeyboardShortcuts();
		else
		{
			gKeyboardShortcuts = Options.KeyboardShortcuts;
			lcSaveDefaultKeyboardShortcuts();
		}

		gMainWindow->UpdateShortcuts();
	}

	if (Options.MouseShortcutsModified)
	{
		if (Options.MouseShortcutsDefault)
			lcResetDefaultMouseShortcuts();
		else
		{
			gMouseShortcuts = Options.MouseShortcuts;
			lcSaveDefaultMouseShortcuts();
		}
	}

	// UpdateStudStyleAndColors() reloads colors too; both paths must see the newly selected config.
	if (ColorConfigChanged)
		lcSetProfileString(LC_PROFILE_COLOR_CONFIG, Options.ColorConfigPath);

	if (StudStyleChanged || StudCylinderColorEnabledChanged)
		lcGetPiecesLibrary()->UpdateStudStyleAndColors(Options.StudStyle, Options.Preferences.mStudCylinderColorEnabled);
	else if (ColorConfigChanged || EdgeColorSettingsChanged)
		lcGetPiecesLibrary()->LoadColors();

	gMainWindow->SetShadingMode(Options.Preferences.mShadingMode);
	lcView::UpdateAllViews();
}

#include "lc_global.h"
#include <stdio.h>
#include "lc_application.h"
#include "lc_library.h"
#include "lc_profile.h"
#include "project.h"
#include "lc_mainwindow.h"
#include "lc_qpreferencesdialog.h"
#include "lc_partselectionwidget.h"
#include "lc_shortcuts.h"
#include "view.h"

lcApplication* gApplication;

void lcPreferences::LoadDefaults()
{
	mFixedAxes = lcGetProfileInt(LC_PROFILE_FIXED_AXES);
	mMouseSensitivity = lcGetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY);
	mShadingMode = static_cast<lcShadingMode>(lcGetProfileInt(LC_PROFILE_SHADING_MODE));
	mDrawAxes = lcGetProfileInt(LC_PROFILE_DRAW_AXES);
	mDrawEdgeLines = lcGetProfileInt(LC_PROFILE_DRAW_EDGE_LINES);
	mLineWidth = lcGetProfileFloat(LC_PROFILE_LINE_WIDTH);
	mAllowLOD = lcGetProfileInt(LC_PROFILE_ALLOW_LOD);
	mFadeSteps = lcGetProfileInt(LC_PROFILE_FADE_STEPS);
	mFadeStepsColor = lcGetProfileInt(LC_PROFILE_FADE_STEPS_COLOR);
	mHighlightNewParts = lcGetProfileInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS);
	mHighlightNewPartsColor = lcGetProfileInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS_COLOR);
	mDrawGridStuds = lcGetProfileInt(LC_PROFILE_GRID_STUDS);
	mGridStudColor = lcGetProfileInt(LC_PROFILE_GRID_STUD_COLOR);
	mDrawGridLines = lcGetProfileInt(LC_PROFILE_GRID_LINES);
	mGridLineSpacing = lcGetProfileInt(LC_PROFILE_GRID_LINE_SPACING);
	mGridLineColor = lcGetProfileInt(LC_PROFILE_GRID_LINE_COLOR);
	mViewSphereEnabled = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_ENABLED);
	mViewSphereLocation = static_cast<lcViewSphereLocation>(lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_LOCATION));
	mViewSphereSize = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_SIZE);
	mViewSphereColor = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_COLOR);
	mViewSphereTextColor = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_TEXT_COLOR);
	mViewSphereHighlightColor = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_HIGHLIGHT_COLOR);
	mAutoLoadMostRecent = lcGetProfileInt(LC_PROFILE_AUTOLOAD_MOSTRECENT);
	mRestoreTabLayout = lcGetProfileInt(LC_PROFILE_RESTORE_TAB_LAYOUT);
}

void lcPreferences::SaveDefaults()
{
	lcSetProfileInt(LC_PROFILE_FIXED_AXES, mFixedAxes);
	lcSetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY, mMouseSensitivity);
	lcSetProfileInt(LC_PROFILE_SHADING_MODE, static_cast<int>(mShadingMode));
	lcSetProfileInt(LC_PROFILE_DRAW_AXES, mDrawAxes);
	lcSetProfileInt(LC_PROFILE_DRAW_EDGE_LINES, mDrawEdgeLines);
	lcSetProfileFloat(LC_PROFILE_LINE_WIDTH, mLineWidth);
	lcSetProfileInt(LC_PROFILE_ALLOW_LOD, mAllowLOD);
	lcSetProfileInt(LC_PROFILE_FADE_STEPS, mFadeSteps);
	lcSetProfileInt(LC_PROFILE_FADE_STEPS_COLOR, mFadeStepsColor);
	lcSetProfileInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS, mHighlightNewParts);
	lcSetProfileInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS_COLOR, mHighlightNewPartsColor);
	lcSetProfileInt(LC_PROFILE_GRID_STUDS, mDrawGridStuds);
	lcSetProfileInt(LC_PROFILE_GRID_STUD_COLOR, mGridStudColor);
	lcSetProfileInt(LC_PROFILE_GRID_LINES, mDrawGridLines);
	lcSetProfileInt(LC_PROFILE_GRID_LINE_SPACING, mGridLineSpacing);
	lcSetProfileInt(LC_PROFILE_GRID_LINE_COLOR, mGridLineColor);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_ENABLED, mViewSphereSize ? 1 : 0);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_LOCATION, (int)mViewSphereLocation);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_SIZE, mViewSphereSize);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_COLOR, mViewSphereColor);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_TEXT_COLOR, mViewSphereTextColor);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_HIGHLIGHT_COLOR, mViewSphereHighlightColor);
	lcSetProfileInt(LC_PROFILE_AUTOLOAD_MOSTRECENT, mAutoLoadMostRecent);
	lcSetProfileInt(LC_PROFILE_RESTORE_TAB_LAYOUT, mRestoreTabLayout);
}

lcApplication::lcApplication(int& Argc, char** Argv)
	: QApplication(Argc, Argv)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	setApplicationDisplayName("LeoCAD");
#endif

	setOrganizationDomain("leocad.org");
	setOrganizationName("LeoCAD Software");
	setApplicationName("LeoCAD");
	setApplicationVersion(LC_VERSION_TEXT);

	gApplication = this;
	mProject = nullptr;
	mLibrary = nullptr;

	mPreferences.LoadDefaults();
}

lcApplication::~lcApplication()
{
	delete mProject;
	delete mLibrary;
	gApplication = nullptr;
}

void lcApplication::SaveTabLayout() const
{
	if (!mProject || mProject->GetFileName().isEmpty())
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

	gMainWindow->RemoveAllModelTabs();

	delete mProject;
	mProject = Project;

	Project->SetActiveModel(0);
	lcGetPiecesLibrary()->RemoveTemporaryPieces();

	if (mProject && !mProject->GetFileName().isEmpty() && mPreferences.mRestoreTabLayout)
	{
		QSettings Settings;
		QByteArray TabLayout = Settings.value(GetTabLayoutKey()).toByteArray();

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

bool lcApplication::LoadPartsLibrary(const QList<QPair<QString, bool>>& LibraryPaths, bool OnlyUsePaths, bool ShowProgress)
{
	if (mLibrary == nullptr)
		mLibrary = new lcPiecesLibrary();

	if (!OnlyUsePaths)
	{
		char* EnvPath = getenv("LEOCAD_LIB");

		if (EnvPath && EnvPath[0])
			return mLibrary->Load(EnvPath, ShowProgress);

		QString CustomPath = lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);

		if (!CustomPath.isEmpty())
			return mLibrary->Load(CustomPath, ShowProgress);
	}

	for (const QPair<QString, bool>& LibraryPathEntry : LibraryPaths)
	{
		if (mLibrary->Load(LibraryPathEntry.first, ShowProgress))
		{
			if (LibraryPathEntry.second)
				mLibrary->SetOfficialPieces();

			return true;
		}
	}

	return false;
}

bool lcApplication::Initialize(QList<QPair<QString, bool>>& LibraryPaths, bool& ShowWindow)
{
	bool OnlyUseLibraryPaths = false;
	bool SaveImage = false;
	bool SaveWavefront = false;
	bool Save3DS = false;
	bool SaveCOLLADA = false;
	bool SaveHTML = false;
	bool SetCameraAngles = false;
	bool Orthographic = false;
	bool ImageHighlight = mPreferences.mHighlightNewParts;
	int ImageWidth = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
	int ImageHeight = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);
	int StudLogo = lcGetProfileInt(LC_PROFILE_STUD_LOGO);
	int ImageStart = 0;
	int ImageEnd = 0;
	float CameraLatitude = 0.0f, CameraLongitude = 0.0f;
	QString ImageName;
	QString ModelName;
	QString CameraName;
	QString ViewpointName;
	QString ProjectName;
	QString SaveWavefrontName;
	QString Save3DSName;
	QString SaveCOLLADAName;
	QString SaveHTMLName;

	QStringList Arguments = arguments();
	const int NumArguments = Arguments.size();

	for (int ArgIdx = 1; ArgIdx < NumArguments; ArgIdx++)
	{
		const QString& Param = Arguments[ArgIdx];

		if (Param.isEmpty())
			continue;

		if (Param[0] != '-')
		{
			ProjectName = Param;
			continue;
		}

		auto ParseString = [&ArgIdx, &Arguments, NumArguments](QString& Value, bool Required)
		{
			if (ArgIdx < NumArguments - 1 && Arguments[ArgIdx + 1][0] != '-')
			{
				ArgIdx++;
				Value = Arguments[ArgIdx];
			}
			else if (Required)
				printf("Not enough parameters for the '%s' argument.\n", Arguments[ArgIdx].toLatin1().constData());
		};

		auto ParseInteger = [&ArgIdx, &Arguments, NumArguments](int& Value)
		{
			if (ArgIdx < NumArguments - 1 && Arguments[ArgIdx + 1][0] != '-')
			{
				bool Ok = false;
				ArgIdx++;
				int NewValue = Arguments[ArgIdx].toInt(&Ok);

				if (Ok)
					Value = NewValue;
				else
					printf("Invalid value specified for the '%s' argument.\n", Arguments[ArgIdx - 1].toLatin1().constData());
			}
			else
				printf("Not enough parameters for the '%s' argument.\n", Arguments[ArgIdx].toLatin1().constData());
		};

		auto ParseFloat = [&ArgIdx, &Arguments, NumArguments](float& Value)
		{
			if (ArgIdx < NumArguments - 1 && Arguments[ArgIdx + 1][0] != '-')
			{
				bool Ok = false;
				ArgIdx++;
				int NewValue = Arguments[ArgIdx].toFloat(&Ok);

				if (Ok)
					Value = NewValue;
				else
					printf("Invalid value specified for the '%s' argument.\n", Arguments[ArgIdx - 1].toLatin1().constData());
			}
			else
				printf("Not enough parameters for the '%s' argument.\n", Arguments[ArgIdx].toLatin1().constData());
		};

		auto ParseVector2 = [&ArgIdx, &Arguments, NumArguments](float& Value1, float& Value2)
		{
			if (ArgIdx < NumArguments - 2 && Arguments[ArgIdx + 1][0] != '-' && Arguments[ArgIdx + 2][0] != '-')
			{
				bool Ok1 = false, Ok2 = false;

				ArgIdx++;
				float NewValue1 = Arguments[ArgIdx].toFloat(&Ok1);
				ArgIdx++;
				float NewValue2 = Arguments[ArgIdx].toFloat(&Ok2);

				if (Ok1 && Ok2)
				{
					Value1 = NewValue1;
					Value2 = NewValue2;
					return true;
				}
				else
					printf("Invalid value specified for the '%s' argument.\n", Arguments[ArgIdx - 2].toLatin1().constData());
			}
			else
				printf("Not enough parameters for the '%s' argument.\n", Arguments[ArgIdx].toLatin1().constData());

			return false;
		};

		if (Param == QLatin1String("-l") || Param == QLatin1String("--libpath"))
		{
			QString LibPath;
			ParseString(LibPath, true);
			if (!LibPath.isEmpty())
			{
				LibraryPaths.clear();
				LibraryPaths += qMakePair<QString, bool>(LibPath, false);
				OnlyUseLibraryPaths = true;
			}
		}
		else if (Param == QLatin1String("-i") || Param == QLatin1String("--image"))
		{
			SaveImage = true;
			ParseString(ImageName, false);
		}
		else if (Param == QLatin1String("-w") || Param == QLatin1String("--width"))
			ParseInteger(ImageWidth);
		else if (Param == QLatin1String("-h") || Param == QLatin1String("--height"))
			ParseInteger(ImageHeight);
		else if (Param == QLatin1String("-f") || Param == QLatin1String("--from"))
			ParseInteger(ImageStart);
		else if (Param == QLatin1String("-t") || Param == QLatin1String("--to"))
			ParseInteger(ImageEnd);
		else if (Param == QLatin1String("-s") || Param == QLatin1String("--submodel"))
			ParseString(ModelName, true);
		else if (Param == QLatin1String("-c") || Param == QLatin1String("--camera"))
			ParseString(CameraName, true);
		else if (Param == QLatin1String("--viewpoint"))
			ParseString(ViewpointName, true);
		else if (Param == QLatin1String("--camera-angles"))
			SetCameraAngles = ParseVector2(CameraLatitude, CameraLongitude);
		else if (Param == QLatin1String("--orthographic"))
			Orthographic = true;
		else if (Param == QLatin1String("--highlight"))
			ImageHighlight = true;
		else if (Param == QLatin1String("--shading"))
		{
			QString ShadingString;
			ParseString(ShadingString, true);

			if (ShadingString == QLatin1String("wireframe"))
				mPreferences.mShadingMode = lcShadingMode::Wireframe;
			else if (ShadingString == QLatin1String("flat"))
				mPreferences.mShadingMode = lcShadingMode::Flat;
			else if (ShadingString == QLatin1String("default"))
				mPreferences.mShadingMode = lcShadingMode::DefaultLights;
			else if (ShadingString == QLatin1String("full"))
				mPreferences.mShadingMode = lcShadingMode::Full;
		}
		else if (Param == QLatin1String("--line-width"))
			ParseFloat(mPreferences.mLineWidth);
		else if (Param == QLatin1String("-sl") || Param == QLatin1String("--stud-logo"))
		{
			ParseInteger(StudLogo);
			if (StudLogo != lcGetProfileInt(LC_PROFILE_STUD_LOGO))
			{
				lcGetPiecesLibrary()->SetStudLogo(StudLogo, false);
			}
		}
		else if (Param == QLatin1String("-obj") || Param == QLatin1String("--export-wavefront"))
		{
			SaveWavefront = true;
			ParseString(SaveWavefrontName, false);
		}
		else if (Param == QLatin1String("-3ds") || Param == QLatin1String("--export-3ds"))
		{
			Save3DS = true;
			ParseString(Save3DSName, false);
		}
		else if (Param == QLatin1String("-dae") || Param == QLatin1String("--export-collada"))
		{
			SaveCOLLADA = true;
			ParseString(SaveCOLLADAName, false);
		}
		else if (Param == QLatin1String("-html") || Param == QLatin1String("--export-html"))
		{
			SaveHTML = true;
			ParseString(SaveHTMLName, false);
		}
		else if (Param == QLatin1String("-v") || Param == QLatin1String("--version"))
		{
#ifdef LC_CONTINUOUS_BUILD
			printf("LeoCAD Continuous Build " QT_STRINGIFY(LC_CONTINUOUS_BUILD) "\n");
#else
			printf("LeoCAD Version " LC_VERSION_TEXT "\n");
#endif
			printf("LeoCAD Version " LC_VERSION_TEXT "\n");
			printf("Compiled " __DATE__ "\n");

			ShowWindow = false;
			return true;
		}
		else if (Param == QLatin1String("-?") || Param == QLatin1String("--help"))
		{
			printf("Usage: leocad [options] [file]\n");
			printf("  [options] can be:\n");
			printf("  -l, --libpath <path>: Set the Parts Library location to path.\n");
			printf("  -i, --image <outfile.ext>: Save a picture in the format specified by ext.\n");
			printf("  -w, --width <width>: Set the picture width.\n");
			printf("  -h, --height <height>: Set the picture height.\n");
			printf("  -f, --from <time>: Set the first step to save pictures.\n");
			printf("  -t, --to <time>: Set the last step to save pictures.\n");
			printf("  -s, --submodel <submodel>: Set the active submodel.\n");
			printf("  -c, --camera <camera>: Set the active camera.\n");
			printf("  -sl --stud-logo <type>: Set the stud logo type 0 - 5, 0 is no logo.\n");
			printf("  --viewpoint <front|back|left|right|top|bottom|home>: Set the viewpoint.\n");
			printf("  --camera-angles <latitude> <longitude>: Set the camera angles in degrees around the model.\n");
			printf("  --orthographic: Make the view orthographic.\n");
			printf("  --highlight: Highlight pieces in the steps they appear.\n");
			printf("  --shading <wireframe|flat|default|full>: Select shading mode for rendering.\n");
			printf("  --line-width <width>: Set the with of the edge lines.\n");
			printf("  -obj, --export-wavefront <outfile.obj>: Export the model to Wavefront OBJ format.\n");
			printf("  -3ds, --export-3ds <outfile.3ds>: Export the model to 3D Studio 3DS format.\n");
			printf("  -dae, --export-collada <outfile.dae>: Export the model to COLLADA DAE format.\n");
			printf("  -html, --export-html <folder>: Create an HTML page for the model.\n");
			printf("  -v, --version: Output version information and exit.\n");
			printf("  -?, --help: Display this help message and exit.\n");
			printf("  \n");

			ShowWindow = false;
			return true;
		}
		else
			printf("Unknown parameter: '%s'\n", Param.toLatin1().constData());
	}

	gMainWindow = new lcMainWindow();
	lcLoadDefaultKeyboardShortcuts();
	lcLoadDefaultMouseShortcuts();

	ShowWindow = !SaveImage && !SaveWavefront && !Save3DS && !SaveCOLLADA && !SaveHTML;

	if (!LoadPartsLibrary(LibraryPaths, OnlyUseLibraryPaths, ShowWindow))
	{
		QString Message;

		if (mLibrary->LoadBuiltinPieces())
			Message = tr("LeoCAD could not find a compatible Parts Library so only a small number of parts will be available.\n\nPlease visit https://www.leocad.org for information on how to download and install a library.");
		else
			Message = tr("LeoCAD could not load Parts Library.\n\nPlease visit https://www.leocad.org for information on how to download and install a library.");

		if (ShowWindow)
			QMessageBox::information(gMainWindow, tr("LeoCAD"), Message);
		else
			fprintf(stderr, "%s", Message.toLatin1().constData());
	}

	gMainWindow->CreateWidgets();

	Project* NewProject = new Project();
	SetProject(NewProject);

	if (ShowWindow && ProjectName.isEmpty() && lcGetProfileInt(LC_PROFILE_AUTOLOAD_MOSTRECENT))
		ProjectName = lcGetProfileString(LC_PROFILE_RECENT_FILE1);

	if (!ProjectName.isEmpty() && gMainWindow->OpenProject(ProjectName))
	{
		if (!ModelName.isEmpty())
			mProject->SetActiveModel(ModelName);

		View* ActiveView = gMainWindow->GetActiveView();

		if (!CameraName.isEmpty())
		{
			ActiveView->SetCamera(CameraName.toLatin1()); // todo: qstring

			if (!ViewpointName.isEmpty())
				printf("Warning: --viewpoint is ignored when --camera is set.\n");

			if (Orthographic)
				printf("Warning: --orthographic is ignored when --camera is set.\n");

			if (SetCameraAngles)
				printf("Warning: --camera-angles is ignored when --camera is set.\n");
		}
		else
		{
			if (!ViewpointName.isEmpty())
			{
				if (ViewpointName == QLatin1String("front"))
					ActiveView->SetViewpoint(LC_VIEWPOINT_FRONT);
				else if (ViewpointName == QLatin1String("back"))
					ActiveView->SetViewpoint(LC_VIEWPOINT_BACK);
				else if (ViewpointName == QLatin1String("top"))
					ActiveView->SetViewpoint(LC_VIEWPOINT_TOP);
				else if (ViewpointName == QLatin1String("bottom"))
					ActiveView->SetViewpoint(LC_VIEWPOINT_BOTTOM);
				else if (ViewpointName == QLatin1String("left"))
					ActiveView->SetViewpoint(LC_VIEWPOINT_LEFT);
				else if (ViewpointName == QLatin1String("right"))
					ActiveView->SetViewpoint(LC_VIEWPOINT_RIGHT);
				else if (ViewpointName == QLatin1String("home"))
					ActiveView->SetViewpoint(LC_VIEWPOINT_HOME);
				else
					printf("Unknown viewpoint: '%s'\n", ViewpointName.toLatin1().constData());

				if (SetCameraAngles)
					printf("Warning: --camera-angles is ignored when --viewpoint is set.\n");
			}
			else if (SetCameraAngles)
				ActiveView->SetCameraAngles(CameraLatitude, CameraLongitude);

			ActiveView->SetProjection(Orthographic);
		}

		if (SaveImage)
		{
			lcModel* ActiveModel;

			if (ModelName.isEmpty())
				ActiveModel = mProject->GetMainModel();
			else
				ActiveModel = mProject->GetActiveModel();

			if (ImageName.isEmpty())
				ImageName = mProject->GetImageFileName(true);

			if (ImageEnd < ImageStart)
				ImageEnd = ImageStart;
			else if (ImageStart > ImageEnd)
				ImageStart = ImageEnd;

			if ((ImageStart == 0) && (ImageEnd == 0))
				ImageStart = ImageEnd = ActiveModel->GetCurrentStep();
			else if ((ImageStart == 0) && (ImageEnd != 0))
				ImageStart = ImageEnd;
			else if ((ImageStart != 0) && (ImageEnd == 0))
				ImageEnd = ImageStart;

			if (ImageStart > 255)
				ImageStart = 255;

			if (ImageEnd > 255)
				ImageEnd = 255;

			QString Frame;

			if (ImageStart != ImageEnd)
			{
				QString Extension = QFileInfo(ImageName).suffix();
				Frame = ImageName.left(ImageName.length() - Extension.length() - 1) + QLatin1String("%1.") + Extension;
			}
			else
				Frame = ImageName;

			mPreferences.mHighlightNewParts = ImageHighlight;

			ActiveModel->SaveStepImages(Frame, ImageStart != ImageEnd, CameraName == nullptr, ImageWidth, ImageHeight, ImageStart, ImageEnd);
		}

		if (SaveWavefront)
		{
			QString FileName;

			if (!SaveWavefrontName.isEmpty())
				FileName = SaveWavefrontName;
			else
				FileName = ProjectName;

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

			mProject->ExportWavefront(FileName);
		}

		if (Save3DS)
		{
			QString FileName;

			if (!Save3DSName.isEmpty())
				FileName = Save3DSName;
			else
				FileName = ProjectName;

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

			mProject->Export3DStudio(FileName);
		}

		if (SaveCOLLADA)
		{
			QString FileName;

			if (!SaveCOLLADAName.isEmpty())
				FileName = SaveCOLLADAName;
			else
				FileName = ProjectName;

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

			mProject->ExportCOLLADA(FileName);
		}

		if (SaveHTML)
		{
			lcHTMLExportOptions Options(mProject);

			if (!SaveHTMLName.isEmpty())
				Options.PathName = SaveHTMLName;

			mProject->ExportHTML(Options);
		}
	}

	if (ShowWindow)
	{
		gMainWindow->SetColorIndex(lcGetColorIndex(4));
		gMainWindow->GetPartSelectionWidget()->SetDefaultPart();
		gMainWindow->UpdateRecentFiles();
		gMainWindow->show();
	}

	return true;
}

void lcApplication::Shutdown()
{
	delete mLibrary;
	mLibrary = nullptr;
}

void lcApplication::ShowPreferencesDialog()
{
	lcPreferencesDialogOptions Options;
	int CurrentAASamples = lcGetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES);
	int CurrentStudLogo = lcGetProfileInt(LC_PROFILE_STUD_LOGO);

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
	Options.StudLogo = CurrentStudLogo;

	Options.Categories = gCategories;
	Options.CategoriesModified = false;
	Options.CategoriesDefault = false;

	Options.KeyboardShortcuts = gKeyboardShortcuts;
	Options.KeyboardShortcutsModified = false;
	Options.KeyboardShortcutsDefault = false;
	Options.MouseShortcuts = gMouseShortcuts;
	Options.MouseShortcutsModified = false;
	Options.MouseShortcutsDefault = false;

	lcQPreferencesDialog Dialog(gMainWindow, &Options);
	if (Dialog.exec() != QDialog::Accepted)
		return;

	bool LanguageChanged = Options.Language != lcGetProfileString(LC_PROFILE_LANGUAGE);
	bool LibraryChanged = Options.LibraryPath != lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);
	bool ColorsChanged = Options.ColorConfigPath != lcGetProfileString(LC_PROFILE_COLOR_CONFIG);
	bool AAChanged = CurrentAASamples != Options.AASamples;
	bool StudLogoChanged = CurrentStudLogo != Options.StudLogo;

	mPreferences = Options.Preferences;

	mPreferences.SaveDefaults();

	lcSetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME, Options.DefaultAuthor);
	lcSetProfileString(LC_PROFILE_PARTS_LIBRARY, Options.LibraryPath);
	lcSetProfileString(LC_PROFILE_COLOR_CONFIG, Options.ColorConfigPath);
	lcSetProfileString(LC_PROFILE_MINIFIG_SETTINGS, Options.MinifigSettingsPath);
	lcSetProfileString(LC_PROFILE_POVRAY_PATH, Options.POVRayPath);
	lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, Options.LGEOPath);
	lcSetProfileString(LC_PROFILE_LANGUAGE, Options.Language);
	lcSetProfileInt(LC_PROFILE_CHECK_UPDATES, Options.CheckForUpdates);
	lcSetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES, Options.AASamples);
	lcSetProfileInt(LC_PROFILE_STUD_LOGO, Options.StudLogo);

	if (LanguageChanged || LibraryChanged || ColorsChanged || AAChanged)
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Some changes will only take effect the next time you start LeoCAD."));

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

	if (StudLogoChanged)
	{
		lcSetProfileInt(LC_PROFILE_STUD_LOGO, Options.StudLogo);
		lcGetPiecesLibrary()->SetStudLogo(Options.StudLogo, true);
	}

	// TODO: printing preferences
	/*
	strcpy(opts.strFooter, m_strFooter);
	strcpy(opts.strHeader, m_strHeader);
	*/

	gMainWindow->SetShadingMode(Options.Preferences.mShadingMode);
	gMainWindow->UpdateAllViews();
}

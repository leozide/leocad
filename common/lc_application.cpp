#include "lc_global.h"
#include <stdio.h>
#include "lc_application.h"
#include "lc_library.h"
#include "lc_profile.h"
#include "project.h"
#include "lc_mainwindow.h"
#include "lc_shortcuts.h"
#include "view.h"

lcApplication* g_App;

void lcPreferences::LoadDefaults()
{
	mFixedAxes = lcGetProfileInt(LC_PROFILE_FIXED_AXES);
	mMouseSensitivity = lcGetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY);
	mShadingMode = (lcShadingMode)lcGetProfileInt(LC_PROFILE_SHADING_MODE);
	mDrawAxes = lcGetProfileInt(LC_PROFILE_DRAW_AXES);
	mDrawEdgeLines = lcGetProfileInt(LC_PROFILE_DRAW_EDGE_LINES);
	mLineWidth = lcGetProfileFloat(LC_PROFILE_LINE_WIDTH);
	mDrawGridStuds = lcGetProfileInt(LC_PROFILE_GRID_STUDS);
	mGridStudColor = lcGetProfileInt(LC_PROFILE_GRID_STUD_COLOR);
	mDrawGridLines = lcGetProfileInt(LC_PROFILE_GRID_LINES);
	mGridLineSpacing = lcGetProfileInt(LC_PROFILE_GRID_LINE_SPACING);
	mGridLineColor = lcGetProfileInt(LC_PROFILE_GRID_LINE_COLOR);
}

void lcPreferences::SaveDefaults()
{
	lcSetProfileInt(LC_PROFILE_FIXED_AXES, mFixedAxes);
	lcSetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY, mMouseSensitivity);
	lcSetProfileInt(LC_PROFILE_SHADING_MODE, mShadingMode);
	lcSetProfileInt(LC_PROFILE_DRAW_AXES, mDrawAxes);
	lcSetProfileInt(LC_PROFILE_DRAW_EDGE_LINES, mDrawEdgeLines);
	lcSetProfileFloat(LC_PROFILE_LINE_WIDTH, mLineWidth);
	lcSetProfileInt(LC_PROFILE_GRID_STUDS, mDrawGridStuds);
	lcSetProfileInt(LC_PROFILE_GRID_STUD_COLOR, mGridStudColor);
	lcSetProfileInt(LC_PROFILE_GRID_LINES, mDrawGridLines);
	lcSetProfileInt(LC_PROFILE_GRID_LINE_SPACING, mGridLineSpacing);
	lcSetProfileInt(LC_PROFILE_GRID_LINE_COLOR, mGridLineColor);
}

lcApplication::lcApplication()
{
	mProject = nullptr;
	mLibrary = nullptr;
	mClipboard = nullptr;

	mPreferences.LoadDefaults();
}

lcApplication::~lcApplication()
{
    delete mProject;
    delete mLibrary;
}

void lcApplication::SetProject(Project* Project)
{
	delete mProject;
	mProject = Project;

	gMainWindow->RemoveAllModelTabs();

	Project->SetActiveModel(0);
	lcGetPiecesLibrary()->RemoveTemporaryPieces();
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

bool lcApplication::LoadPiecesLibrary(const QList<QPair<QString, bool>>& LibraryPaths)
{
	if (mLibrary == nullptr)
		mLibrary = new lcPiecesLibrary();

	char* EnvPath = getenv("LEOCAD_LIB");

	if (EnvPath && EnvPath[0])
		return mLibrary->Load(EnvPath);

	QString CustomPath = lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);

	if (!CustomPath.isEmpty())
		return mLibrary->Load(CustomPath);

	for (const QPair<QString, bool>& LibraryPathEntry : LibraryPaths)
	{
		if (mLibrary->Load(LibraryPathEntry.first))
		{
			if (LibraryPathEntry.second)
				mLibrary->SetOfficialPieces();

			return true;
		}
	}

	return false;
}

void lcApplication::ParseIntegerArgument(int* CurArg, int argc, char* argv[], int* Value) const
{
	if (argc > (*CurArg + 1))
	{
		(*CurArg)++;
		int val;

		if ((sscanf(argv[(*CurArg)], "%d", &val) == 1) && (val > 0))
			*Value = val;
		else
		{
			*Value = 0;
			printf("Invalid value specified for the %s argument.\n", argv[(*CurArg) - 1]);
		}
	}
	else
	{
		*Value = 0;
		printf("Not enough parameters for the %s argument.\n", argv[(*CurArg)]);
	}
}

void lcApplication::ParseStringArgument(int* CurArg, int argc, char* argv[], const char** Value) const
{
	if (argc > (*CurArg + 1))
	{
		(*CurArg)++;
		*Value = argv[(*CurArg)];
	}
	else
	{
		printf("No path specified after the %s argument.\n", argv[(*CurArg)]);
	}
}

bool lcApplication::Initialize(int argc, char* argv[], QList<QPair<QString, bool>>& LibraryPaths, bool& ShowWindow)
{
	// todo: parse command line using Qt to handle multibyte strings.
	bool SaveImage = false;
	bool SaveWavefront = false;
	bool Save3DS = false;
	bool SaveCOLLADA = false;
	bool Orthographic = false;
	bool ImageHighlight = false;
	int ImageWidth = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
	int ImageHeight = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);
	lcStep ImageStart = 0;
	lcStep ImageEnd = 0;
	char* ImageName = nullptr;
	char* ModelName = nullptr;
	char* CameraName = nullptr;
	char* Viewpoint = nullptr;
	char* ProjectName = nullptr;
	char* SaveWavefrontName = nullptr;
	char* Save3DSName = nullptr;
	char* SaveCOLLADAName = nullptr;

	for (int i = 1; i < argc; i++)
	{
		char* Param = argv[i];

		if (Param[0] == '-')
		{
			if ((strcmp(Param, "-l") == 0) || (strcmp(Param, "--libpath") == 0))
			{
				const char* LibPath = nullptr;
				ParseStringArgument(&i, argc, argv, &LibPath);
				if (LibPath && LibPath[0])
				{
					LibraryPaths.clear();
					LibraryPaths += qMakePair<QString, bool>(LibPath, false);
				}
			}
			else if ((strcmp(Param, "-i") == 0) || (strcmp(Param, "--image") == 0))
			{
				SaveImage = true;

				if ((argc > (i+1)) && (argv[i+1][0] != '-'))
				{
					i++;
					ImageName = argv[i];
				}
			}
			else if ((strcmp(Param, "-w") == 0) || (strcmp(Param, "--width") == 0))
			{
				ParseIntegerArgument(&i, argc, argv, &ImageWidth);
			}
			else if ((strcmp(Param, "-h") == 0) || (strcmp(Param, "--height") == 0))
			{
				ParseIntegerArgument(&i, argc, argv, &ImageHeight);
			}
			else if ((strcmp(Param, "-f") == 0) || (strcmp(Param, "--from") == 0))
			{
				int Step;
				ParseIntegerArgument(&i, argc, argv, &Step);
				ImageStart = Step;
			}
			else if ((strcmp(Param, "-t") == 0) || (strcmp(Param, "--to") == 0))
			{
				int Step;
				ParseIntegerArgument(&i, argc, argv, &Step);
				ImageEnd = Step;
			}
			else if ((strcmp(Param, "-m") == 0) || (strcmp(Param, "--model") == 0))
			{
				if ((argc > (i+1)) && (argv[i+1][0] != '-'))
				{
					i++;
					ModelName = argv[i];
				}
			}
			else if ((strcmp(Param, "-c") == 0) || (strcmp(Param, "--camera") == 0))
			{
				if ((argc > (i+1)) && (argv[i+1][0] != '-'))
				{
					i++;
					CameraName = argv[i];
				}
			}
			else if (strcmp(Param, "--viewpoint") == 0)
			{
				if ((argc > (i+1)) && (argv[i+1][0] != '-'))
				{
					i++;
					Viewpoint = argv[i];
				}
			}
			else if (strcmp(Param, "--orthographic") == 0)
				Orthographic = true;
			else if (strcmp(Param, "--highlight") == 0)
				ImageHighlight = true;
			else if ((strcmp(Param, "-obj") == 0) || (strcmp(Param, "--export-wavefront") == 0))
			{
				SaveWavefront = true;

				if ((argc > (i+1)) && (argv[i+1][0] != '-'))
				{
					i++;
					SaveWavefrontName = argv[i];
				}
			}
			else if ((strcmp(Param, "-3ds") == 0) || (strcmp(Param, "--export-3ds") == 0))
			{
				Save3DS = true;

				if ((argc > (i+1)) && (argv[i+1][0] != '-'))
				{
					i++;
					Save3DSName = argv[i];
				}
			}
			else if ((strcmp(Param, "-dae") == 0) || (strcmp(Param, "--export-collada") == 0))
			{
				SaveCOLLADA = true;

				if ((argc > (i+1)) && (argv[i+1][0] != '-'))
				{
					i++;
					SaveCOLLADAName = argv[i];
				}
			}
			else if ((strcmp(Param, "-v") == 0) || (strcmp(Param, "--version") == 0))
			{
				printf("LeoCAD Version " LC_VERSION_TEXT "\n");
				printf("Compiled " __DATE__ "\n");

				ShowWindow = false;
				return true;
			}
			else if ((strcmp(Param, "-?") == 0) || (strcmp(Param, "--help") == 0))
			{
				printf("Usage: leocad [options] [file]\n");
				printf("  [options] can be:\n");
				printf("  -l, --libpath <path>: Loads the Pieces Library from path.\n");
				printf("  -i, --image <outfile.ext>: Saves a picture in the format specified by ext.\n");
				printf("  -w, --width <width>: Sets the picture width.\n");
				printf("  -h, --height <height>: Sets the picture height.\n");
				printf("  -f, --from <time>: Sets the first frame or step to save pictures.\n");
				printf("  -t, --to <time>: Sets the last frame or step to save pictures.\n");
				printf("  -m, --model <model>: Sets the active submodel.\n");
				printf("  -c, --camera <camera>: Sets the active camera.\n");
				printf("  --viewpoint (front|back|left|right|top|bottom|home): Sets the viewpoint.\n");
				printf("  --orthographic: Make the view orthographic.\n");
				printf("  --highlight: Highlight pieces in the steps they appear.\n");
				printf("  -obj, --export-wavefront <outfile.obj>: Exports the model to Wavefront OBJ format.\n");
				printf("  -3ds, --export-3ds <outfile.3ds>: Exports the model to 3D Studio 3DS format.\n");
				printf("  -dae, --export-collada <outfile.dae>: Exports the model to COLLADA DAE format.\n");
				printf("  -v, --version: Output version information and exit.\n");
				printf("  -?, --help: Display this help and exit.\n");
				printf("  \n");

				ShowWindow = false;
				return true;
			}
			else
				printf("Unknown parameter: %s\n", Param);
		}
		else
		{
			ProjectName = Param;
		}
	}

	gMainWindow = new lcMainWindow();
	lcLoadDefaultKeyboardShortcuts();
	lcLoadDefaultMouseShortcuts();

	ShowWindow = !SaveImage && !SaveWavefront && !Save3DS && !SaveCOLLADA;

	if (!LoadPiecesLibrary(LibraryPaths))
	{
		QString Message;

		if (mLibrary->LoadBuiltinPieces())
			Message = tr("LeoCAD could not find a compatible Parts Library so only a small number of parts will be available.\n\nPlease visit http://www.leocad.org for information on how to download and install a library.");
		else
			Message = tr("LeoCAD could not load Parts Library.\n\nPlease visit http://www.leocad.org for information on how to download and install a library.");

		if (ShowWindow)
			QMessageBox::information(gMainWindow, tr("LeoCAD"), Message);
		else
			fprintf(stderr, "%s", Message.toLatin1().constData());
	}

	gMainWindow->CreateWidgets();

	// Create a new project.
	Project* NewProject = new Project();
	SetProject(NewProject);

	// Load project.
	if (ProjectName && gMainWindow->OpenProject(ProjectName))
	{
		if (ModelName)
			lcGetActiveProject()->SetActiveModel(QString::fromUtf8(ModelName));

		if (CameraName)
		{
			gMainWindow->GetActiveView()->SetCamera(CameraName);
			if (Viewpoint || Orthographic)
				printf("Warning: --viewpoint and --orthographic are ignored when --camera is set.\n");
		}
		else
		{
			if (Viewpoint)
			{
				if (strcmp(Viewpoint, "front") == 0)
					gMainWindow->GetActiveView()->SetViewpoint(LC_VIEWPOINT_FRONT);
				else if (strcmp(Viewpoint, "back") == 0)
					gMainWindow->GetActiveView()->SetViewpoint(LC_VIEWPOINT_BACK);
				else if (strcmp(Viewpoint, "top") == 0)
					gMainWindow->GetActiveView()->SetViewpoint(LC_VIEWPOINT_TOP);
				else if (strcmp(Viewpoint, "bottom") == 0)
					gMainWindow->GetActiveView()->SetViewpoint(LC_VIEWPOINT_BOTTOM);
				else if (strcmp(Viewpoint, "left") == 0)
					gMainWindow->GetActiveView()->SetViewpoint(LC_VIEWPOINT_LEFT);
				else if (strcmp(Viewpoint, "right") == 0)
					gMainWindow->GetActiveView()->SetViewpoint(LC_VIEWPOINT_RIGHT);
				else if (strcmp(Viewpoint, "home") == 0)
					gMainWindow->GetActiveView()->SetViewpoint(LC_VIEWPOINT_HOME);
				else
					printf("Unknown viewpoint: %s\n", Viewpoint);
			}
			gMainWindow->GetActiveView()->SetProjection(Orthographic);
		}

		if (SaveImage)
		{
			QString FileName;

			if (ImageName)
				FileName = ImageName;
			else
				FileName = ProjectName;

			QString Extension = QFileInfo(FileName).suffix().toLower();

			if (Extension.isEmpty())
			{
				FileName += lcGetProfileString(LC_PROFILE_IMAGE_EXTENSION);
			}
			else if (Extension != "bmp" && Extension != "jpg" && Extension != "jpeg" && Extension != "png")
			{
				FileName = FileName.left(FileName.length() - Extension.length() - 1);
				FileName += lcGetProfileString(LC_PROFILE_IMAGE_EXTENSION);
			}

			if (ImageEnd < ImageStart)
				ImageEnd = ImageStart;
			else if (ImageStart > ImageEnd)
				ImageStart = ImageEnd;

			if ((ImageStart == 0) && (ImageEnd == 0))
			{
				ImageStart = ImageEnd = mProject->GetActiveModel()->GetCurrentStep();
			}
			else if ((ImageStart == 0) && (ImageEnd != 0))
			{
				ImageStart = ImageEnd;
			}
			else if ((ImageStart != 0) && (ImageEnd == 0))
			{
				ImageEnd = ImageStart;
			}

			if (ImageStart > 255)
				ImageStart = 255;

			if (ImageEnd > 255)
				ImageEnd = 255;

			QString Frame;

			if (ImageStart != ImageEnd)
			{
				QString Extension = QFileInfo(FileName).suffix();
				Frame = FileName.left(FileName.length() - Extension.length() - 1) + QLatin1String("%1.") + Extension;
			}
			else
				Frame = FileName;

			lcGetActiveModel()->SaveStepImages(Frame, ImageStart != ImageEnd, CameraName == nullptr, ImageHighlight, ImageWidth, ImageHeight, ImageStart, ImageEnd);
		}

		if (SaveWavefront)
		{
			QString FileName;

			if (SaveWavefrontName)
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

			if (Save3DSName)
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

			if (SaveCOLLADAName)
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

	Options.Preferences = mPreferences;

	Options.DefaultAuthor = lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME);
	Options.LibraryPath = lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);
	Options.POVRayPath = lcGetProfileString(LC_PROFILE_POVRAY_PATH);
	Options.LGEOPath = lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH);
	Options.CheckForUpdates = lcGetProfileInt(LC_PROFILE_CHECK_UPDATES);

	Options.AASamples = CurrentAASamples;

	Options.Categories = gCategories;
	Options.CategoriesModified = false;
	Options.CategoriesDefault = false;

	Options.KeyboardShortcuts = gKeyboardShortcuts;
	Options.KeyboardShortcutsModified = false;
	Options.KeyboardShortcutsDefault = false;
	Options.MouseShortcuts = gMouseShortcuts;
	Options.MouseShortcutsModified = false;
	Options.MouseShortcutsDefault = false;

	if (!gMainWindow->DoDialog(LC_DIALOG_PREFERENCES, &Options))
		return;

	bool LibraryChanged = Options.LibraryPath != lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);
	bool AAChanged = CurrentAASamples != Options.AASamples;

	mPreferences = Options.Preferences;

	mPreferences.SaveDefaults();

	lcSetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME, Options.DefaultAuthor);
	lcSetProfileString(LC_PROFILE_PARTS_LIBRARY, Options.LibraryPath);
	lcSetProfileString(LC_PROFILE_POVRAY_PATH, Options.POVRayPath);
	lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, Options.LGEOPath);
	lcSetProfileInt(LC_PROFILE_CHECK_UPDATES, Options.CheckForUpdates);
	lcSetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES, Options.AASamples);

	if (LibraryChanged && AAChanged)
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Parts library and Anti-aliasing changes will only take effect the next time you start LeoCAD."));
	else if (LibraryChanged)
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Parts library changes will only take effect the next time you start LeoCAD."));
	else if (AAChanged)
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Anti-aliasing changes will only take effect the next time you start LeoCAD."));

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

	// TODO: printing preferences
	/*
	strcpy(opts.strFooter, m_strFooter);
	strcpy(opts.strHeader, m_strHeader);
	*/

	gMainWindow->UpdateAllViews();
}

#include "lc_global.h"
#include <stdio.h>
#include "lc_application.h"
#include "lc_colors.h"
#include "lc_library.h"
#include "lc_profile.h"
#include "system.h"
#include "opengl.h"
#include "project.h"
#include "image.h"
#include "lc_mainwindow.h"
#include "lc_shortcuts.h"
#include "view.h"

lcApplication* g_App;

void lcPreferences::LoadDefaults()
{
	mFixedAxes = lcGetProfileInt(LC_PROFILE_FIXED_AXES);
	mMouseSensitivity = lcGetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY);
	mLightingMode = (lcLightingMode)lcGetProfileInt(LC_PROFILE_LIGHTING_MODE);
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
	lcSetProfileInt(LC_PROFILE_LIGHTING_MODE, mLightingMode);
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
	mProject = NULL;
	mLibrary = NULL;
	mClipboard = NULL;

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

	const lcArray<View*>& Views = gMainWindow->GetViews();
	for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
		Views[ViewIdx]->SetModel(lcGetActiveModel());

	lcGetPiecesLibrary()->RemoveTemporaryPieces();
	lcGetActiveModel()->UpdateInterface();
	gMainWindow->UpdateAllViews();
}

void lcApplication::SetClipboard(const QByteArray& Clipboard)
{
	mClipboard = Clipboard;
	gMainWindow->UpdatePaste(!mClipboard.isEmpty());
}

bool lcApplication::LoadPiecesLibrary(const char* LibPath, const char* LibraryInstallPath, const char* LDrawPath, const char* LibraryCachePath)
{
	if (mLibrary == NULL)
		mLibrary = new lcPiecesLibrary();

	if (LibPath && LibPath[0])
		return mLibrary->Load(LibPath, LibraryCachePath);

	char* EnvPath = getenv("LEOCAD_LIB");

	if (EnvPath && EnvPath[0])
	{
		return mLibrary->Load(EnvPath, LibraryCachePath);
	}

	char CustomPath[LC_MAXPATH];
	strcpy(CustomPath, lcGetProfileString(LC_PROFILE_PARTS_LIBRARY));

	if (CustomPath[0])
		return mLibrary->Load(CustomPath, LibraryCachePath);

	if (LibraryInstallPath && LibraryInstallPath[0])
	{
		char LibraryPath[LC_MAXPATH];

		strcpy(LibraryPath, LibraryInstallPath);

		int i = strlen(LibraryPath) - 1;
		if ((LibraryPath[i] != '\\') && (LibraryPath[i] != '/'))
			strcat(LibraryPath, "/");

		strcat(LibraryPath, "library.bin");

		if (mLibrary->Load(LibraryPath, LibraryCachePath))
		{
			mLibrary->SetOfficialPieces();
			return true;
		}
	}

	if (LDrawPath && LDrawPath[0])
	{
		char LibraryPath[LC_MAXPATH];

		strcpy(LibraryPath, LDrawPath);

		int i = strlen(LibraryPath) - 1;
		if ((LibraryPath[i] != '\\') && (LibraryPath[i] != '/'))
			strcat(LibraryPath, "/");

		if (mLibrary->Load(LibraryPath, LibraryCachePath))
			return true;
	}

	return false;
}

void lcApplication::ParseIntegerArgument(int* CurArg, int argc, char* argv[], int* Value)
{
	if (argc > (*CurArg + 1))
	{
		(*CurArg)++;
		int val;

		if ((sscanf(argv[(*CurArg)], "%d", &val) == 1) && (val > 0))
			*Value = val;
		else
			printf("Invalid value specified for the %s argument.", argv[(*CurArg) - 1]);
	}
	else
	{
		*Value = 0;
		printf("Not enough parameters for the %s argument.", argv[(*CurArg) - 1]);
	}
}

void lcApplication::ParseStringArgument(int* CurArg, int argc, char* argv[], char** Value)
{
	if (argc > (*CurArg + 1))
	{
		(*CurArg)++;
		*Value = argv[(*CurArg)];
	}
	else
	{
		printf("No path specified after the %s argument.", argv[(*CurArg) - 1]);
	}
}

bool lcApplication::Initialize(int argc, char* argv[], const char* LibraryInstallPath, const char* LDrawPath, const char* LibraryCachePath)
{
	char* LibPath = NULL;

	// Image output options.
	bool SaveImage = false;
	bool SaveWavefront = false;
//	bool ImageHighlight = false;
	int ImageWidth = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
	int ImageHeight = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);
	lcStep ImageStart = 0;
	lcStep ImageEnd = 0;
	char* ImageName = NULL;
	char* ProjectName = NULL;
	char* WavefrontName = NULL;

	// Parse the command line arguments.
	for (int i = 1; i < argc; i++)
	{
		char* Param = argv[i];

		if (Param[0] == '-')
		{
			if ((strcmp(Param, "-l") == 0) || (strcmp(Param, "--libpath") == 0))
			{
				ParseStringArgument(&i, argc, argv, &LibPath);
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
//			else if (strcmp(Param, "--highlight") == 0)
//				ImageHighlight = true;
			else if ((strcmp(Param, "-wf") == 0) || (strcmp(Param, "--wavefront") == 0))
			{
				SaveWavefront = true;

				if ((argc > (i+1)) && (argv[i+1][0] != '-'))
				{
					i++;
					WavefrontName = argv[i];
				}
			}

			else if ((strcmp(Param, "-v") == 0) || (strcmp(Param, "--version") == 0))
			{
				printf("LeoCAD Version " LC_VERSION_TEXT "\n");
				printf("Compiled " __DATE__ "\n");

				return false;
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
//				printf("  --highlight: Highlight pieces in the steps they appear.\n");
				printf("  -wf, --wavefront <outfile.obj>: Exports the model to Wavefront format.\n");
				printf("  \n");

				return false;
			}
			else
				printf("Unknown parameter: %s\n", Param);
		}
		else
		{
			ProjectName = Param;
		}
	}

	if (!LoadPiecesLibrary(LibPath, LibraryInstallPath, LDrawPath, LibraryCachePath))
	{
		if (SaveImage || SaveWavefront)
		{
			fprintf(stderr, "ERROR: Cannot load pieces library.");
			return false;
		}

		if (mLibrary->LoadBuiltinPieces())
			gMainWindow->DoMessageBox("LeoCAD could not find a compatible Parts Library so only a small number of parts will be available.\n\n"
									  "Please visit http://www.leocad.org for information on how to download and install a library.", LC_MB_OK | LC_MB_ICONERROR);
		else
			gMainWindow->DoMessageBox("LeoCAD could not load Parts Library.\n\n"
									  "Please visit http://www.leocad.org for information on how to download and install a library.", LC_MB_OK | LC_MB_ICONERROR);
	}

	// Create a new project.
	Project* NewProject = new Project();
	SetProject(NewProject);

	// Load project.
	if (ProjectName && gMainWindow->OpenProject(ProjectName))
	{
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

			lcGetActiveModel()->SaveStepImages(Frame, ImageWidth, ImageHeight, ImageStart, ImageEnd);
		}

		if (SaveWavefront)
		{
			QString FileName;

			if (WavefrontName)
				FileName = WavefrontName;
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
	}

	if (SaveImage || SaveWavefront)
		return false;

	lcLoadDefaultKeyboardShortcuts();

	return true;
}

void lcApplication::Shutdown()
{
	delete mLibrary;
	mLibrary = NULL;
}

void lcApplication::ShowPreferencesDialog()
{
	lcPreferencesDialogOptions Options;
	int CurrentAASamples = lcGetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES);

	Options.Preferences = mPreferences;

	strcpy(Options.DefaultAuthor, lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME));
	strcpy(Options.ProjectsPath, lcGetProfileString(LC_PROFILE_PROJECTS_PATH));
	strcpy(Options.LibraryPath, lcGetProfileString(LC_PROFILE_PARTS_LIBRARY));
	strcpy(Options.POVRayPath, lcGetProfileString(LC_PROFILE_POVRAY_PATH));
	strcpy(Options.LGEOPath, lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH));
	Options.CheckForUpdates = lcGetProfileInt(LC_PROFILE_CHECK_UPDATES);

	Options.AASamples = CurrentAASamples;

	Options.Categories = gCategories;
	Options.CategoriesModified = false;
	Options.CategoriesDefault = false;

	Options.KeyboardShortcuts = gKeyboardShortcuts;
	Options.ShortcutsModified = false;
	Options.ShortcutsDefault = false;

	if (!gMainWindow->DoDialog(LC_DIALOG_PREFERENCES, &Options))
		return;

	bool LibraryChanged = strcmp(Options.LibraryPath, lcGetProfileString(LC_PROFILE_PARTS_LIBRARY));
	bool AAChanged = CurrentAASamples != Options.AASamples;

	mPreferences = Options.Preferences;

	mPreferences.SaveDefaults();

	lcSetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME, Options.DefaultAuthor);
	lcSetProfileString(LC_PROFILE_PROJECTS_PATH, Options.ProjectsPath);
	lcSetProfileString(LC_PROFILE_PARTS_LIBRARY, Options.LibraryPath);
	lcSetProfileString(LC_PROFILE_POVRAY_PATH, Options.POVRayPath);
	lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, Options.LGEOPath);
	lcSetProfileInt(LC_PROFILE_CHECK_UPDATES, Options.CheckForUpdates);
	lcSetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES, Options.AASamples);

	if (LibraryChanged && AAChanged)
		gMainWindow->DoMessageBox("Parts library and Anti-aliasing changes will only take effect the next time you start LeoCAD.", LC_MB_OK);
	else if (LibraryChanged)
		gMainWindow->DoMessageBox("Parts library changes will only take effect the next time you start LeoCAD.", LC_MB_OK);
	else if (AAChanged)
		gMainWindow->DoMessageBox("Anti-aliasing changes will only take effect the next time you start LeoCAD.", LC_MB_OK);

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

	if (Options.ShortcutsModified)
	{
		if (Options.ShortcutsDefault)
			lcResetDefaultKeyboardShortcuts();
		else
		{
			gKeyboardShortcuts = Options.KeyboardShortcuts;
			lcSaveDefaultKeyboardShortcuts();
		}

		gMainWindow->UpdateShortcuts();
	}

	// TODO: printing preferences
	/*
	strcpy(opts.strFooter, m_strFooter);
	strcpy(opts.strHeader, m_strHeader);
	*/

	gMainWindow->UpdateAllViews();
}

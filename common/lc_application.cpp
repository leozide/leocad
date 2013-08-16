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

lcApplication* g_App;

lcApplication::lcApplication()
{
	mProject = NULL;
	m_Library = NULL;
	mClipboard = NULL;
}

lcApplication::~lcApplication()
{
	delete mClipboard;
}

void lcApplication::SetClipboard(lcFile* Clipboard)
{
	delete mClipboard;
	mClipboard = Clipboard;

	gMainWindow->UpdatePaste(mClipboard != NULL);
}

bool lcApplication::LoadPiecesLibrary(const char* LibPath, const char* LibraryInstallPath, const char* LibraryCachePath)
{
	if (m_Library == NULL)
		m_Library = new lcPiecesLibrary();

	if (LibPath && LibPath[0])
	{
		if (m_Library->Load(LibPath, LibraryCachePath))
			return true;
	}
	else
	{
		char* EnvPath = getenv("LEOCAD_LIB");

		if (EnvPath && EnvPath[0])
		{
			if (m_Library->Load(EnvPath, LibraryCachePath))
				return true;
		}
		else
		{
			char CustomPath[LC_MAXPATH];

			strcpy(CustomPath, lcGetProfileString(LC_PROFILE_PARTS_LIBRARY));

			if (CustomPath[0])
			{
				if (m_Library->Load(CustomPath, LibraryCachePath))
					return true;
			}
			else if (LibraryInstallPath && LibraryInstallPath[0])
			{
				char LibraryPath[LC_MAXPATH];

				strcpy(LibraryPath, LibraryInstallPath);

				int i = strlen(LibraryPath) - 1;
				if ((LibraryPath[i] != '\\') && (LibraryPath[i] != '/'))
					strcat(LibraryPath, "/");

				strcat(LibraryPath, "library.bin");

				if (m_Library->Load(LibraryPath, LibraryCachePath))
				{
					m_Library->mNumOfficialPieces = m_Library->mPieces.GetSize();
					return true;
				}
			}
		}
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

bool lcApplication::Initialize(int argc, char* argv[], const char* LibraryInstallPath, const char* LibraryCachePath)
{
	char* LibPath = NULL;

	// Image output options.
	bool SaveImage = false;
	bool ImageAnimation = false;
	bool ImageInstructions = false;
	bool ImageHighlight = false;
	int ImageWidth = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
	int ImageHeight = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);
	int ImageStart = 0;
	int ImageEnd = 0;
	char* ImageName = NULL;

	// File to open.
	char* ProjectName = NULL;

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
				ParseIntegerArgument(&i, argc, argv, &ImageStart);
			}
			else if ((strcmp(Param, "-t") == 0) || (strcmp(Param, "--to") == 0))
			{
				ParseIntegerArgument(&i, argc, argv, &ImageEnd);
			}
			else if (strcmp(Param, "--animation") == 0)
				ImageAnimation = true;
			else if (strcmp(Param, "--instructions") == 0)
				ImageInstructions = true;
			else if (strcmp(Param, "--highlight") == 0)
				ImageHighlight = true;
			else if ((strcmp(Param, "-v") == 0) || (strcmp(Param, "--version") == 0))
			{
				printf("LeoCAD Version " LC_VERSION_TEXT "\n");
				printf("Compiled "__DATE__"\n");

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
				printf("  --animation: Saves animations frames.\n");
				printf("  --instructions: Saves instructions steps.\n");
				printf("  --highlight: Highlight pieces in the steps they appear.\n");
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

	if (!LoadPiecesLibrary(LibPath, LibraryInstallPath, LibraryCachePath))
	{
		if (SaveImage)
		{
			fprintf(stderr, "ERROR: Cannot load pieces library.");
			return false;
		}

		m_Library->CreateBuiltinPieces();

		gMainWindow->DoMessageBox("LeoCAD could not find a compatible Pieces Library so only a small number of pieces will be available.\n\n"
		                          "Please visit http://www.leocad.org for information on how to download and install a library.", LC_MB_OK | LC_MB_ICONERROR);
	}

	// Create a new project.
	mProject = new Project();

	GL_DisableVertexBufferObject();

	// Load project.
	if (ProjectName && mProject->OpenProject(ProjectName))
	{
		if (!SaveImage)
			return true;

		// Check if there's a file name and it has an extension.
		bool NeedExt = true;
		String FileName;

		if (!ImageName)
		{
			FileName = ProjectName;

			int i = FileName.ReverseFind('.');
			if (i != -1)
				FileName[i] = 0;
		}
		else
		{
			FileName = ImageName;

			int i = FileName.ReverseFind('.');
			String Ext;

			if (i != -1)
			{
				Ext = FileName.Right(FileName.GetLength() - i);
				Ext.MakeLower();
			}

			if (Ext == "bmp")
				NeedExt = false;
			else if ((Ext == "jpg") || (Ext == "jpeg"))
				NeedExt = false;
			else if (Ext == "png")
				NeedExt = false;
		}

		// Setup default options.
		int ImageOptions = lcGetProfileInt(LC_PROFILE_IMAGE_OPTIONS);
		bool ImageTransparent = (ImageOptions & LC_IMAGE_TRANSPARENT) != 0;
		LC_IMAGE_FORMAT ImageFormat = (LC_IMAGE_FORMAT)(ImageOptions & ~(LC_IMAGE_MASK));

		// Append file extension if needed.
		if (NeedExt)
		{
			switch (ImageFormat)
			{
			case LC_IMAGE_BMP:
				FileName += ".bmp";
				break;
			case LC_IMAGE_JPG:
				FileName += ".jpg";
				break;
			default:
			case LC_IMAGE_PNG:
				FileName += ".png";
				break;
			}
		}

		if (ImageInstructions)
			mProject->SetAnimation(false);
		else if (ImageAnimation)
			mProject->SetAnimation(true);

		if (ImageEnd < ImageStart)
			ImageEnd = ImageStart;
		else if (ImageStart > ImageEnd)
			ImageStart = ImageEnd;

		if ((ImageStart == 0) && (ImageEnd == 0))
		{
			ImageStart = ImageEnd = mProject->GetCurrentTime();
		}
		else if ((ImageStart == 0) && (ImageEnd != 0))
		{
			ImageStart = ImageEnd;
		}
		else if ((ImageStart != 0) && (ImageEnd == 0))
		{
			ImageEnd = ImageStart;
		}

		if (mProject->IsAnimation())
		{
			if (ImageStart > mProject->GetTotalFrames())
				ImageStart = mProject->GetTotalFrames();

			if (ImageEnd > mProject->GetTotalFrames())
				ImageEnd = mProject->GetTotalFrames();
		}
		else
		{
			if (ImageStart > 255)
				ImageStart = 255;

			if (ImageEnd > 255)
				ImageEnd = 255;
		}

		Image* images = new Image[ImageEnd - ImageStart + 1];
		mProject->CreateImages(images, ImageWidth, ImageHeight, ImageStart, ImageEnd, ImageHighlight);

		for (int i = 0; i <= ImageEnd - ImageStart; i++)
		{
			char idx[256];
			String Frame;

			if (ImageStart != ImageEnd)
			{
				sprintf(idx, "%02d", i+1);
				int Ext = FileName.ReverseFind('.');

				Frame = FileName.Left(Ext) + idx + FileName.Right(FileName.GetLength() - Ext);
			}
			else
				Frame = FileName;

			images[i].FileSave(Frame, ImageFormat, ImageTransparent);
		}

		delete []images;

		return false;
	}
	else
	{
		if (SaveImage)
			return false;
		else
			mProject->OnNewDocument();
	}

	lcLoadDefaultKeyboardShortcuts();

	return true;
}

void lcApplication::Shutdown()
{
	delete m_Library;
	m_Library = NULL;
}

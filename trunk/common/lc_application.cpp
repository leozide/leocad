#include "lc_global.h"
#include "lc_application.h"

#include <stdio.h>
#include "lc_mesh.h"
#include "library.h"
#include "system.h"
#include "console.h"
#include "opengl.h"
#include "project.h"
#include "image.h"

// ----------------------------------------------------------------------------
// Global functions.

lcApplication* g_App;

PiecesLibrary* lcGetPiecesLibrary()
{
	LC_ASSERT(g_App, "g_App not initialized.");
	return g_App->GetPiecesLibrary();
}

Project* lcGetActiveProject()
{
	LC_ASSERT(g_App, "g_App not initialized.");
	return g_App->GetActiveProject();
}

// ----------------------------------------------------------------------------
// lcApplication class.

lcApplication::lcApplication()
{
	m_PiecePreview = NULL;
	m_ActiveProject = NULL;
	m_Library = NULL;
}

lcApplication::~lcApplication()
{
}

void lcApplication::AddProject(Project* project)
{
	m_Projects.Add(project);

	if (m_ActiveProject == NULL)
		m_ActiveProject = project;
}

bool lcApplication::LoadPiecesLibrary(const char* LibPath, const char* SysLibPath)
{
	// Create an empty library.
	if (m_Library == NULL)
		m_Library = new PiecesLibrary();
	else
		m_Library->Unload();

	// Check if the user specified a library path in the command line.
	if (LibPath != NULL)
		if (m_Library->Load(LibPath))
			return true;

	// Check for the LEOCAD_LIB environment variable.
	char* EnvPath = getenv("LEOCAD_LIB");

	if (EnvPath != NULL)
		if (m_Library->Load(EnvPath))
			return true;

	// Try the last library path.
	const char* ProfilePath = Sys_ProfileLoadString("Settings", "PiecesLibrary", "");
	if (strlen(ProfilePath) != 0)
		if (m_Library->Load(ProfilePath))
			return true;

	// Try the executable install path last.
	if (SysLibPath != NULL)
		if (m_Library->Load(SysLibPath))
			return true;

	// Give the user a chance to select the directory.
	for (;;)
	{
		int Ret = SystemDoMessageBox("LeoCAD was unable to load its Pieces Library, do you want to specify another location?",
		                             LC_MB_YESNO|LC_MB_ICONQUESTION);

		if (Ret == LC_YES)
		{
			LC_DLG_DIRECTORY_BROWSE_OPTS Opts;

			Opts.Title = "Select Pieces Library Directory";

			if (SystemDoDialog(LC_DLG_DIRECTORY_BROWSE, &Opts))
				if (m_Library->Load(Opts.Path))
					return true;
		}
		else
			break;
	}

	SystemDoMessageBox("LeoCAD could not load its Pieces Library and will now exit.\n\n"
	                   "Make sure that you have the library installed in your computer.", LC_MB_OK|LC_MB_ICONERROR);

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
			console.PrintWarning("Invalid value specified for the %s argument.", argv[(*CurArg) - 1]);
	}
	else
	{
		console.PrintWarning("Not enough parameters for the %s argument.", argv[(*CurArg) - 1]);
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
		console.PrintWarning("No path specified after the %s argument.", argv[(*CurArg) - 1]);
	}
}

bool lcApplication::Initialize(int argc, char* argv[], const char* SysLibPath)
{
	// System setup parameters.
	char* LibPath = NULL;
	char* GLPath = NULL;

	// Image output options.
	bool SaveImage = false;
	bool ImageAnimation = false;
	bool ImageInstructions = false;
	bool ImageHighlight = false;
	int ImageWidth = Sys_ProfileLoadInt("Default", "Image Width", 640);
	int ImageHeight = Sys_ProfileLoadInt("Default", "Image Height", 480);
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
			if (strcmp(Param, "--libgl") == 0)
			{
				ParseStringArgument(&i, argc, argv, &GLPath);
			}
			else if ((strcmp(Param, "-l") == 0) || (strcmp(Param, "--libpath") == 0))
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
				printf("LeoCAD version " LC_VERSION_TEXT LC_VERSION_TAG " for "LC_VERSION_OSNAME"\n");
				printf("Copyright (c) 1996-2006, BT Software\n");
				printf("Compiled "__DATE__"\n");

#ifdef LC_HAVE_JPEGLIB
				printf("With JPEG support\n");
#else
				printf("Without JPEG support\n");
#endif

#ifdef LC_HAVE_PNGLIB
				printf("With PNG support\n");
#else
				printf("Without PNG support\n");
#endif

				return false;
			}
			else if ((strcmp(Param, "-?") == 0) || (strcmp(Param, "--help") == 0))
			{
				printf("Usage: leocad [options] [file]\n");
				printf("  [options] can be:\n");
				printf("  --libgl <path>: Searches for OpenGL libraries in path.\n");
				printf("  --libpath <path>: Loads the Pieces library from path.\n");
				printf("  -i, --image <outfile.ext>: Saves a picture in the format specified by ext.\n");
				printf("  -w, --width <width>: Sets the picture width.\n");
				printf("  -h, --height <height>: Sets the picture height.\n");
				printf("  -f, --from <time>: Sets the first frame or step to save pictures.\n");
				printf("  -t, --to <time>: Sets the last frame or step to save pictures.\n");
				printf("  --animation: Saves animations frames.\n");
				printf("  --instructions: Saves instructions steps.\n");
				printf("  --highlight: Highlight pieces in the steps they appear.\n");
				printf("  \n");
			}
			else
				console.PrintWarning("Unknown parameter: %s\n", Param);
		}
		else
		{
			ProjectName = Param;
		}
	}

	// Initialize other systems.
	if (!GL_Initialize(GLPath))
		return false;

	if (!LoadPiecesLibrary(LibPath, SysLibPath))
		return false;

	SystemInit();

	lcCreateDefaultMeshes();

	// Create a new project.
	Project* project = new Project();
	AddProject(project);

	// Load project.
	if (ProjectName && project->OpenProject(ProjectName))
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

			if ((Ext == "bmp") || (Ext == "gif"))
				NeedExt = false;
#ifdef LC_HAVE_JPEGLIB
			else if ((Ext == "jpg") || (Ext == "jpeg"))
				NeedExt = false;
#endif
#ifdef LC_HAVE_PNGLIB
			else if (Ext == "png")
				NeedExt = false;
#endif
		}

		// Setup default options.
		LC_IMAGE_OPTS ImageOptions;
		unsigned long image = Sys_ProfileLoadInt  ("Default", "Image Options", 1|LC_IMAGE_TRANSPARENT);
		ImageOptions.quality = Sys_ProfileLoadInt ("Default", "JPEG Quality", 70);
		ImageOptions.interlaced = (image & LC_IMAGE_PROGRESSIVE) != 0;
		ImageOptions.transparent = (image & LC_IMAGE_TRANSPARENT) != 0;
		ImageOptions.truecolor = (image & LC_IMAGE_HIGHCOLOR) != 0;
		ImageOptions.format = image & ~(LC_IMAGE_MASK);
		ImageOptions.background[0] = (unsigned char)(project->GetBackgroundColor()[0]*255);
		ImageOptions.background[1] = (unsigned char)(project->GetBackgroundColor()[1]*255);
		ImageOptions.background[2] = (unsigned char)(project->GetBackgroundColor()[2]*255);

		// Append file extension if needed.
		if (NeedExt)
		{
			switch (ImageOptions.format)
			{
			default:
			case LC_IMAGE_BMP:
				FileName += ".bmp";
				break;
			case LC_IMAGE_GIF:
				FileName += ".gif";
				break;
			case LC_IMAGE_JPG:
				FileName += ".jpg";
				break;
			case LC_IMAGE_PNG:
				FileName += ".png";
				break;
			}
		}

		if (ImageEnd < ImageStart)
			ImageEnd = ImageStart;
		else if (ImageStart > ImageEnd)
			ImageStart = ImageEnd;

		if ((ImageStart == 0) && (ImageEnd == 0))
		{
			ImageStart = ImageEnd = project->GetCurrentTime();
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

		Image* images = new Image[ImageEnd - ImageStart + 1];
		project->CreateImages(images, ImageWidth, ImageHeight, ImageStart, ImageEnd, ImageHighlight);

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

			images[i].FileSave(Frame, &ImageOptions);
		}

		delete []images;

		return false;
	}
	else
	{
		if (SaveImage)
			return false;
		else
			project->OnNewDocument();
	}

	return true;
}

void lcApplication::Shutdown()
{
	for (int i = 0; i < m_Projects.GetSize(); i++)
	{
		Project* project = m_Projects[i];

		project->HandleNotify(LC_ACTIVATE, 0);
		delete project;
	}

	delete m_Library;
	m_Library = NULL;

	lcDestroyDefaultMeshes();

	GL_Shutdown();
}

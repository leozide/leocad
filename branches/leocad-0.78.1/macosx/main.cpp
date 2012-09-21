#include "glwindow.h"
#include "project.h"
#include "view.h"
#include "globals.h"
#include "opengl.h"
#include "mainwnd.h"
#include "lc_application.h"

#include <Carbon.h>

View* view;

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <OpenGL/glext.h>

Boolean SetUp();
pascal void IdleTimer (EventLoopTimerRef inTimer, void* userData);
EventLoopTimerUPP GetTimerUPP();
void CleanUp();

EventLoopTimerRef gTimer = NULL;

Boolean SetUp()
{
	InitCursor();
  DrawMenuBar();

  InstallEventLoopTimer(GetCurrentEventLoop(), 0, 0.01, GetTimerUPP(), 0, &gTimer);

	return true;
}

pascal void IdleTimer(EventLoopTimerRef inTimer, void* userData)
{
//	view->Redraw();
}

EventLoopTimerUPP GetTimerUPP()
{
  static EventLoopTimerUPP  sTimerUPP = NULL;
  
  if (sTimerUPP == NULL)
    sTimerUPP = NewEventLoopTimerUPP (IdleTimer);
  
  return sTimerUPP;
}

void CleanUp()
{
  RemoveEventLoopTimer(gTimer);
  gTimer = NULL;  
}

// --------------------------------------------------------------------------

pascal OSStatus lcAppEventHandler(EventHandlerCallRef inHandlerCallRef, EventRef Event, void* UserData)
{
	return eventNotHandledErr;
}

DEFINE_ONE_SHOT_HANDLER_GETTER(lcAppEventHandler);

int main(int argc, char* argv[])
{
	UInt32 response;

	// Check the Mac OS version.
	if ((Gestalt (gestaltSystemVersion, (SInt32 *) &response) == noErr) && (response < 0x01020))
	{
		printf("Must have Mac OS v10.2 or later.\n");
		return 1;
	}

	// Initialize the application.
	g_App = new lcApplication();
	main_window = new MainWnd();

	// Get the pieces library path (bundle/Contents/Resources/).
	CFBundleRef Bundle = CFBundleGetMainBundle();
	if (Bundle == NULL)
		return 1;
	CFURLRef BundleURL = CFBundleCopyResourcesDirectoryURL(Bundle);
	if (BundleURL == NULL)
		return 1;

	UInt8 ResourcesPath[LC_MAXPATH];
	CFURLGetFileSystemRepresentation(BundleURL, true, ResourcesPath, LC_MAXPATH);

	if (!g_App->Initialize(argc, argv, (char*)ResourcesPath))
		return 1;

	// Install the application event handler.
	EventTypeSpec AppEventList[] = { { kEventClassCommand, kEventProcessCommand } };
	InstallApplicationEventHandler(GetlcAppEventHandlerUPP(), GetEventTypeCount(AppEventList), AppEventList, NULL, NULL);

	IBNibRef nib = NULL;
	CreateNibReference(CFSTR("main"), &nib);

	WindowRef win = NULL;
	CreateWindowFromNib(nib, CFSTR("MainWindow"), &win);
	ShowWindow(win);

	SetMenuBarFromNib(nib, CFSTR("MainMenu"));

	if (SetUp())
	{
		view = new View(lcGetActiveProject(), NULL);

		HIViewRef userPane; 
		static const HIViewID userPaneID = { 'Moof', 127 };
		HIViewFindByID(HIViewGetRoot(win), userPaneID, &userPane);

		if (!view->Create(userPane))
		{
			delete main_window;
			main_window = NULL;
			delete view;
			g_App->Shutdown();
			delete g_App;
			return 1;
		}

		RunApplicationEventLoop();

		view->DestroyContext();
		DisposeWindow(win);
	}

	CleanUp();

	delete main_window;
	main_window = NULL;
	delete view;
	g_App->Shutdown();
	delete g_App;

	DisposeNibReference(nib);

	return 0;
}

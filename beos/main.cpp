//
// LeoCAD for BeOS
//

#include <stdlib.h>
#include <be/kernel/OS.h>
#include "opengl.h"
#include "project.h"
#include "globals.h"

// Flag to tell whether or not the Be application is active or not
int BeAppActive = 0;
static thread_id AppThread = 0;

static int32 RunThread (void *data)
{
	SDL_RunThread (data);
	return 0;
}

// Initialize the Be Application, if it's not already started
int InitBeApp ()
{
	// Create the BApplication that handles appserver interaction
	if (BeAppActive <= 0)
	{
		// Create the thread and go!
		AppThread = spawn_thread (RunThread, "LeoCAD", B_NORMAL_PRIORITY, NULL);
		if ((AppThread == B_NO_MORE_THREADS) ||
			(AppThread == B_NO_MEMORY))
		{
			printf ("Not enough resources to create thread\n");
			return -1;
		}
		resume_thread (AppThread);

		do
		{
			snooze (10);
		} while ((be_app == NULL) || be_app->IsLaunching ());

		// Mark the application active
		BeAppActive = 0;
	}

	// Increment the application reference count
	BeAppActive++;

	// The app is running, and we're ready to go
	return 0;
}

// Quit the Be Application, if there's nothing left to do
void QuitBeApp ()
{
	// Decrement the application reference count
	BeAppActive--;

	// If the reference count reached zero, clean up the app
	if (BeAppActive == 0)
	{
		if (AppThread != 0)
		{
			if (be_app != NULL)
			{
				// Not tested
				be_app->PostMessage(B_QUIT_REQUESTED);
			}

			status_t the_status;
			wait_for_thread (AppThread, &the_status);
			AppThread = 0;
		}
		// be_app should now be NULL since be_app has quit
	}
}
        
int main (int argc, char** argv)
{
  char* libgl = NULL;
  int i, j, k;

  // Parse and remove system arguments
  for (i = 1; i < argc; i++)
  {
    char* param = argv[i];

    if (param[0] == '-' && param[1] == '-')
    {
      param += 2;

      if ((strcmp (param, "libgl") == 0) && (i != argc))
      {
        libgl = argv[i+1];
        argv[i] = argv[i+1] = NULL;
        i++;
      }
    }
  }
  
  for (i = 1; i < argc; i++)
  {
    for (k = i; k < argc; k++)
      if (argv[k] != NULL)
        break;

    if (k > i)
    {
      k -= i;
      for (j = i + k; j < argc; j++)
        argv[j-k] = argv[j];
      argc -= k;
    }
  }
  
  if (!GL_Initialize (libgl))
    return 1;






  project = new Project();

  char* path;
  path = getenv("LEOCAD_LIB");
  if (path == NULL)
    path = "/boot/home/leocad/";

  if (project->Initialize(argc, argv, path) == false)
  {
    delete project;
    GL_Shutdown ();
    return 1;
  }

  delete project;
  GL_Shutdown ();

  return 0;
}
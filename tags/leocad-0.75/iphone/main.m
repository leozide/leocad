#import <UIKit/UIKit.h>
#include "config.h"
#include "globals.h"
#include "lc_application.h"
#include "library.h"
#include "basewnd.h"
#include "project.h"
#include "preview.h"
#include "mainwnd.h"

PiecePreview* preview;

int main(int argc, char *argv[])
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef url = CFBundleCopyBundleURL(mainBundle);
	UInt8 path[2048];
	CFURLGetFileSystemRepresentation(url, YES, path, 2048);
	CFRelease(url);
	strcat((char*)path, "/");

	g_App = new lcApplication();
	main_window = new MainWnd();

	if (!g_App->Initialize(argc, argv, (char*)path))
		return 1;
	
	preview = new PiecePreview(NULL);

	PieceInfo* Info = lcGetPiecesLibrary()->FindPieceInfo("3005");
	if (!Info)
		Info = lcGetPiecesLibrary()->GetPieceInfo(0);

	if (Info)
	{
		lcGetActiveProject()->SetCurrentPiece(Info);
		preview->SetCurrentPiece(Info);
	}

    int retVal = UIApplicationMain(argc, argv, nil, nil);
    [pool release];
    return retVal;
}

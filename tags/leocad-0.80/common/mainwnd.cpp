#include "lc_global.h"
#include "mainwnd.h"
#include "lc_profile.h"
#include "preview.h"

lcMainWindow* gMainWindow;

lcMainWindow::lcMainWindow()
  {
	mColorIndex = 0;
	mPreviewWidget = NULL;

	for (int FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		strcpy(mRecentFiles[FileIdx], lcGetProfileString((LC_PROFILE_KEY)(LC_PROFILE_RECENT_FILE1 + FileIdx)));

	gMainWindow = this;
}

lcMainWindow::~lcMainWindow()
{
	for (int FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		lcSetProfileString((LC_PROFILE_KEY)(LC_PROFILE_RECENT_FILE1 + FileIdx), mRecentFiles[FileIdx]);

	gMainWindow = NULL;
}

void lcMainWindow::SetColorIndex(int ColorIndex)
  {
	mColorIndex = ColorIndex;

	if (mPreviewWidget)
		mPreviewWidget->Redraw();
}

void lcMainWindow::AddRecentFile(const char* FileName)
{
	int FileIdx;

	for (FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		if (!strcmp(mRecentFiles[FileIdx], FileName))
			break;

	for (FileIdx = lcMin(FileIdx, LC_MAX_RECENT_FILES - 1); FileIdx > 0; FileIdx--)
		strcpy(mRecentFiles[FileIdx], mRecentFiles[FileIdx - 1]);

	strcpy(mRecentFiles[0], FileName);

	UpdateRecentFiles();
}

void lcMainWindow::RemoveRecentFile(int FileIndex)
{
	for (int FileIdx = FileIndex; FileIdx < LC_MAX_RECENT_FILES - 1; FileIdx++)
		strcpy(mRecentFiles[FileIdx], mRecentFiles[FileIdx + 1]);

	mRecentFiles[LC_MAX_RECENT_FILES - 1][0] = 0;

	UpdateRecentFiles();
}


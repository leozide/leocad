#include "lc_global.h"
#include "lc_mainwindow.h"
#include "lc_profile.h"
#include "preview.h"
#include "view.h"

lcMainWindow* gMainWindow;

lcMainWindow::lcMainWindow()
{
	mActiveView = NULL;
	mPreviewWidget = NULL;
	mTransformType = LC_TRANSFORM_RELATIVE_TRANSLATION;

	mAddKeys = false;
	mMoveXYSnapIndex = 4;
	mMoveZSnapIndex = 3;
	mAngleSnapIndex = 5;
	mLockX = false;
	mLockY = false;
	mLockZ = false;

	memset(&mSearchOptions, 0, sizeof(mSearchOptions));

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

void lcMainWindow::AddView(View* View)
{
	mViews.Add(View);

	View->MakeCurrent();

	if (!mActiveView)
	{
		mActiveView = View;
		UpdatePerspective();
	}
}

void lcMainWindow::RemoveView(View* View)
{
	if (View == mActiveView)
		mActiveView = NULL;

	mViews.Remove(View);
}

void lcMainWindow::SetActiveView(View* ActiveView)
{
	if (mActiveView == ActiveView)
		return;

	mActiveView = ActiveView;

	UpdateCameraMenu();
	UpdatePerspective();
}

void lcMainWindow::UpdateAllViews()
{
	for (int ViewIdx = 0; ViewIdx < mViews.GetSize(); ViewIdx++)
		mViews[ViewIdx]->Redraw();
}

void lcMainWindow::SetTool(lcTool Tool)
{
	mTool = Tool;

	UpdateAction(mTool);
	UpdateAllViews();
}

void lcMainWindow::SetColorIndex(int ColorIndex)
{
	mColorIndex = ColorIndex;

	if (mPreviewWidget)
		mPreviewWidget->Redraw();
}

void lcMainWindow::SetMoveXYSnapIndex(int Index)
{
	mMoveXYSnapIndex = Index;
	UpdateSnap();
}

void lcMainWindow::SetMoveZSnapIndex(int Index)
{
	mMoveZSnapIndex = Index;
	UpdateSnap();
}

void lcMainWindow::SetAngleSnapIndex(int Index)
{
	mAngleSnapIndex = Index;
	UpdateSnap();
}

void lcMainWindow::SetLockX(bool LockX)
{
	mLockX = LockX;
	UpdateLockSnap();
}

void lcMainWindow::SetLockY(bool LockY)
{
	mLockY = LockY;
	UpdateLockSnap();
}

void lcMainWindow::SetLockZ(bool LockZ)
{
	mLockZ = LockZ;
	UpdateLockSnap();
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


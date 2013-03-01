#include "lc_global.h"
#include "lc_application.h"
#include "lc_qmainwindow.h"
#include "mainwnd.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationDomain("leocad.org");
	QCoreApplication::setOrganizationName("LeoCAD");
	QCoreApplication::setApplicationName("LeoCAD");
	QCoreApplication::setApplicationVersion(LC_VERSION_TEXT);

	g_App = new lcApplication();
	gMainWindow = new lcMainWindow();

#ifdef Q_OS_WIN
	char libPath[LC_MAXPATH], *ptr;
	strcpy(libPath, argv[0]);
	ptr = strrchr(libPath,'\\');
	if (ptr)
		*(++ptr) = 0;
#else
	const char* libPath = LC_INSTALL_PREFIX"/share/leocad/";
#endif

	QString cachePath = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);

	QDir dir;
	dir.mkdir(cachePath);

	if (!g_App->Initialize(argc, argv, libPath, cachePath.toLocal8Bit().data()))
		return 1;

	QApplication a(argc, argv);
	lcQMainWindow w;
	gMainWindow->mHandle = &w;
	w.show();

	return a.exec();
}

// TODO: move somewhere else

int lcBaseWindow::DoMessageBox(const char* Text, const char* Caption, int Flags)
{
	QWidget* parent = (QWidget*)mHandle;

	QMessageBox::StandardButton	result;
	QMessageBox::StandardButtons buttons;

	switch (Flags & LC_MB_TYPEMASK)
	{
	default:
	case LC_MB_OK:
		buttons = QMessageBox::Ok;
		break;

	case LC_MB_OKCANCEL:
		buttons = QMessageBox::Ok | QMessageBox::Cancel;
		break;

	case LC_MB_YESNOCANCEL:
		buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
		break;

	case LC_MB_YESNO:
		buttons = QMessageBox::Yes | QMessageBox::No;
		break;
	}

	switch (Flags & LC_MB_ICONMASK)
	{
	default:
	case LC_MB_ICONINFORMATION:
		result = QMessageBox::information(parent, Caption, Text, buttons);
		break;

	case LC_MB_ICONQUESTION:
		result = QMessageBox::question(parent, Caption, Text, buttons);
		break;

	case LC_MB_ICONWARNING:
		result = QMessageBox::warning(parent, Caption, Text, buttons);
		break;

	case LC_MB_ICONERROR:
		result = QMessageBox::critical(parent, Caption, Text, buttons);
		break;
	}

	switch (result)
	{
	default:
	case QMessageBox::Ok:
		return LC_OK;

	case QMessageBox::Cancel:
		return LC_CANCEL;

	case QMessageBox::Yes:
		return LC_YES;

	case QMessageBox::No:
		return LC_NO;
	}
}

#include "lc_qpovraydialog.h"
#include "lc_qselectdialog.h"
#include "lc_qarraydialog.h"
#include "lc_qgroupdialog.h"
#include "lc_qeditgroupsdialog.h"
#include "lc_qaboutdialog.h"

bool lcBaseWindow::DoDialog(LC_DIALOG_TYPE Type, void* Data)
{
	QWidget* parent = (QWidget*)mHandle;

	switch (Type)
	{
	case LC_DIALOG_OPEN_PROJECT:
	case LC_DIALOG_SAVE_PROJECT:
	case LC_DIALOG_MERGE_PROJECT:
	case LC_DIALOG_OPEN_CATEGORIES:
	case LC_DIALOG_SAVE_CATEGORIES:
	case LC_DIALOG_EXPORT_BRICKLINK:
	case LC_DIALOG_EXPORT_WAVEFRONT:
		{
			char* FileName = (char*)Data;
			QString result;

			switch (Type)
			{
			case LC_DIALOG_OPEN_PROJECT:
				result = QFileDialog::getOpenFileName(parent, parent->tr("Open Project"), FileName, parent->tr("Supported Files (*.lcd *.ldr *.dat *.mpd);;All Files (*.*)"));
				break;

			case LC_DIALOG_SAVE_PROJECT:
				result = QFileDialog::getSaveFileName(parent, parent->tr("Save Project"), FileName, parent->tr("Supported Files (*.lcd *.ldr *.dat);;All Files (*.*)"));
				break;

			case LC_DIALOG_MERGE_PROJECT:
				result = QFileDialog::getOpenFileName(parent, parent->tr("Merge Project"), FileName, parent->tr("Supported Files (*.lcd *.ldr *.dat *.mpd);;All Files (*.*)"));
				break;

			case LC_DIALOG_OPEN_CATEGORIES:
				result = QFileDialog::getOpenFileName(parent, parent->tr("Open Categories"), FileName, parent->tr("LeoCAD Categories Files (*.lcf);;All Files (*.*)"));
				break;

			case LC_DIALOG_SAVE_CATEGORIES:
				result = QFileDialog::getSaveFileName(parent, parent->tr("Save Categories"), FileName, parent->tr("LeoCAD Categories Files (*.lcf);;All Files (*.*)"));
				break;

			case LC_DIALOG_EXPORT_BRICKLINK:
				result = QFileDialog::getSaveFileName(parent, parent->tr("Export BrickLink"), FileName, parent->tr("XML Files (*.xml);;All Files (*.*)"));
				break;

			case LC_DIALOG_EXPORT_WAVEFRONT:
				result = QFileDialog::getSaveFileName(parent, parent->tr("Export Wavefront"), FileName, parent->tr("Wavefront Files (*.obj);;All Files (*.*)"));
				break;

			default:
				break;
			}

			if (!result.isEmpty())
			{
				strcpy(FileName, result.toLocal8Bit().data());
				return true;
			}
		} break;

	case LC_DIALOG_EXPORT_POVRAY:
		{
			lcQPOVRayDialog dialog(parent, Data);
			return dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_SELECT_BY_NAME:
		{
			lcQSelectDialog dialog(parent, Data);
			return dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_PIECE_ARRAY:
		{
			lcQArrayDialog dialog(parent, Data);
			return dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_PIECE_GROUP:
		{
			lcQGroupDialog dialog(parent, Data);
			return dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_EDIT_GROUPS:
		{
			lcQEditGroupsDialog dialog(parent, Data);
			return dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_ABOUT:
		{
			lcQAboutDialog dialog(parent, Data);
			return dialog.exec() == QDialog::Accepted;
		} break;
	}

	return false;
}

void lcMainWindow::UpdateAction(int NewAction)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateAction(NewAction);
}

void lcMainWindow::UpdatePaste(bool Enabled)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updatePaste(Enabled);
}

void lcMainWindow::UpdateTime(bool Animation, int CurrentTime, int TotalTime)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateTime(Animation, CurrentTime, TotalTime);
}

void lcMainWindow::UpdateAnimation(bool Animation, bool AddKeys)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateAnimation(Animation, AddKeys);
}

void lcMainWindow::UpdateLockSnap(lcuint32 Snap)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateLockSnap(Snap);
}

void lcMainWindow::UpdateSnap(lcuint16 MoveSnap, lcuint16 RotateSnap)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateSnap(MoveSnap, RotateSnap);
}

void lcMainWindow::UpdateUndoRedo(const char* UndoText, const char* RedoText)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateUndoRedo(UndoText, RedoText);
}

void lcMainWindow::UpdateCameraMenu(const PtrArray<Camera>& Cameras, Camera* CurrentCamera)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateCameraMenu(Cameras, CurrentCamera);
}

void lcMainWindow::UpdateCurrentCamera(int CameraIndex)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateCurrentCamera(CameraIndex);
}

void lcMainWindow::UpdateTitle(const char* Title, bool Modified)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateTitle(Title, Modified);
}

void lcMainWindow::UpdateModified(bool Modified)
{
	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateModified(Modified);
}

void lcMainWindow::UpdateRecentFiles()
{
	const char* fileNames[LC_MAX_RECENT_FILES];

	for (int fileIdx = 0; fileIdx < LC_MAX_RECENT_FILES; fileIdx++)
		fileNames[fileIdx] = mRecentFiles[fileIdx];

	lcQMainWindow* window = (lcQMainWindow*)mHandle;

	if (window)
		window->updateRecentFiles(fileNames);
}

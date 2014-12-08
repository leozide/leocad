#include "lc_global.h"
#include "lc_mainwindow.h"
#include "lc_profile.h"
#include "preview.h"
#include "view.h"
#include "project.h"
#include "lc_colors.h"

lcMainWindow* gMainWindow;

lcMainWindow::lcMainWindow()
{
	mActiveView = NULL;
	mPreviewWidget = NULL;
	mTransformType = LC_TRANSFORM_RELATIVE_TRANSLATION;

	mColorIndex = lcGetColorIndex(4);
	mTool = LC_TOOL_SELECT;
	mAddKeys = false;
	mMoveXYSnapIndex = 4;
	mMoveZSnapIndex = 3;
	mAngleSnapIndex = 5;
	mLockX = false;
	mLockY = false;
	mLockZ = false;

	memset(&mSearchOptions, 0, sizeof(mSearchOptions));

	for (int FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		mRecentFiles[FileIdx] = lcGetProfileString((LC_PROFILE_KEY)(LC_PROFILE_RECENT_FILE1 + FileIdx));

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

void lcMainWindow::AddRecentFile(const QString& FileName)
{
	QString SavedName = FileName;
	int FileIdx;

	for (FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		if (mRecentFiles[FileIdx] == FileName)
			break;

	for (FileIdx = lcMin(FileIdx, LC_MAX_RECENT_FILES - 1); FileIdx > 0; FileIdx--)
		mRecentFiles[FileIdx] = mRecentFiles[FileIdx - 1];

	mRecentFiles[0] = SavedName;

	UpdateRecentFiles();
}

void lcMainWindow::RemoveRecentFile(int FileIndex)
{
	for (int FileIdx = FileIndex; FileIdx < LC_MAX_RECENT_FILES - 1; FileIdx++)
		mRecentFiles[FileIdx] = mRecentFiles[FileIdx + 1];

	mRecentFiles[LC_MAX_RECENT_FILES - 1].clear();

	UpdateRecentFiles();
}

void lcMainWindow::NewProject()
{
	if (!SaveProjectIfModified())
		return;

	Project* NewProject = new Project();
	NewProject->NewModel();
	g_App->SetProject(NewProject);
}

bool lcMainWindow::OpenProject(const QString& FileName)
{
	if (!SaveProjectIfModified())
		return false;

	QString LoadFileName = FileName;

	if (LoadFileName.isEmpty())
	{
		LoadFileName = lcGetActiveProject()->GetFileName();

		if (LoadFileName.isEmpty())
			LoadFileName = lcGetProfileString(LC_PROFILE_PROJECTS_PATH);

		LoadFileName = QFileDialog::getOpenFileName(mHandle, tr("Open Project"), LoadFileName, tr("Supported Files (*.lcd *.ldr *.dat *.mpd);;All Files (*.*)"));

		if (LoadFileName.isEmpty())
			return false;
	}

	Project* NewProject = new Project();

	if (NewProject->Load(LoadFileName))
	{
		g_App->SetProject(NewProject);
		AddRecentFile(LoadFileName);

		return true;
	}

	QMessageBox::information(mHandle, tr("LeoCAD"), tr("Error loading '%1'.").arg(LoadFileName));
	delete NewProject;

	return false;
}

bool lcMainWindow::SaveProject(const QString& FileName)
{
	QString SaveFileName;
	Project* Project = lcGetActiveProject();

	if (!FileName.isEmpty())
		SaveFileName = FileName;
	else
	{
		QString SaveFileName = Project->GetFileName();

		if (SaveFileName.isEmpty())
			SaveFileName = QFileInfo(QDir(lcGetProfileString(LC_PROFILE_PROJECTS_PATH)), Project->GetTitle()).absoluteFilePath();

		SaveFileName = QFileDialog::getSaveFileName(mHandle, tr("Save Project"), SaveFileName, tr("Supported Files (*.ldr *.dat);;All Files (*.*)"));

		if (SaveFileName.isEmpty())
			return false;
	}

	if (QFileInfo(SaveFileName).suffix().toLower() == QLatin1String("lcd"))
	{
		QMessageBox::warning(mHandle, tr("Error"), tr("Saving files in LCD format is no longer supported, please use the LDR format instead."));
		return false;
	}

	if (!Project->Save(SaveFileName))
		return false;

	AddRecentFile(SaveFileName);
	UpdateTitle();

	return true;
}

bool lcMainWindow::SaveProjectIfModified()
{
	Project* Project = lcGetActiveProject();
	if (!Project->IsModified())
		return true;

	switch (QMessageBox::question(mHandle, tr("Save Project"), tr("Save changes to '%1'?").arg(Project->GetTitle()), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel))
	{
	default:
	case QMessageBox::Cancel:
		return false;

	case QMessageBox::Yes:
		if (!SaveProject(Project->GetFileName()))
			return false;
		break;

	case QMessageBox::No:
		break;
	}

	return true;
}

void lcMainWindow::HandleCommand(lcCommandId CommandId)
{
	switch (CommandId)
	{
	case LC_FILE_NEW:
		NewProject();
		break;

	case LC_FILE_OPEN:
		OpenProject(QString());
		break;
/*
		case LC_FILE_MERGE:
		{
			QString FileName;

			if (!mFileName.isEmpty())
				FileName = mFileName;
			else
				FileName = lcGetProfileString(LC_PROFILE_PROJECTS_PATH);

			FileName = QFileDialog::getOpenFileName(gMainWindow->mHandle, tr("Merge Project"), FileName, tr("Supported Files (*.lcd *.ldr *.dat *.mpd);;All Files (*.*)"));

			if (!FileName.isEmpty())
			{
				// todo: detect format
				lcDiskFile file;
				if (file.Open(FileName.toLatin1().constData(), "rb")) // todo: qstring
				{
					if (file.GetLength() != 0)
					{
						FileLoad(&file, false, true);
						CheckPoint("Merging");
					}
					file.Close();
				}
			}
		} break;
*/
	case LC_FILE_SAVE:
		SaveProject(lcGetActiveProject()->GetFileName());
		break;

	case LC_FILE_SAVEAS:
		SaveProject(QString());
		break;

	case LC_FILE_SAVE_IMAGE:
		lcGetActiveProject()->SaveImage();
		break;
		/*
	case LC_FILE_EXPORT_3DS:
		lcGetActiveProject()->Export3DStudio();
		break;

	case LC_FILE_EXPORT_HTML:
		lcGetActiveProject()->ExportHTML();
		break;

	case LC_FILE_EXPORT_BRICKLINK:
		lcGetActiveProject()->ExportBrickLink();
		break;

	case LC_FILE_EXPORT_CSV:
		lcGetActiveProject()->ExportCSV();
		break;

	case LC_FILE_EXPORT_POVRAY:
		lcGetActiveProject()->ExportPOVRay();
		break;

	case LC_FILE_EXPORT_WAVEFRONT:
		lcGetActiveProject()->ExportWavefront();
		break;
		*/
	case LC_FILE_PROPERTIES:
		lcGetActiveModel()->ShowPropertiesDialog();
		break;

	case LC_FILE_PRINT_PREVIEW:
		TogglePrintPreview();
		break;

	case LC_FILE_PRINT:
		DoDialog(LC_DIALOG_PRINT, NULL);
		break;

	// TODO: printing
	case LC_FILE_PRINT_BOM:
		break;

	case LC_FILE_RECENT1:
	case LC_FILE_RECENT2:
	case LC_FILE_RECENT3:
	case LC_FILE_RECENT4:
		if (!OpenProject(mRecentFiles[CommandId - LC_FILE_RECENT1]))
			RemoveRecentFile(CommandId - LC_FILE_RECENT1);
		break;

	case LC_FILE_EXIT:
		Close();
		break;

	case LC_EDIT_UNDO:
		lcGetActiveModel()->UndoAction();
		break;

	case LC_EDIT_REDO:
		lcGetActiveModel()->RedoAction();
		break;
/*
	case LC_EDIT_CUT:
	case LC_EDIT_COPY:
		{
			lcMemFile* Clipboard = new lcMemFile();

			int i = 0;
//			lcLight* pLight;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				if (mPieces[PieceIdx]->IsSelected())
					i++;
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
					Piece->FileSave(*Clipboard);
			}

			i = mGroups.GetSize();
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
				mGroups[GroupIdx]->FileSave(Clipboard, mGroups);

			i = 0;
			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				if (mCameras[CameraIdx]->IsSelected())
					i++;
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
			{
				lcCamera* pCamera = mCameras[CameraIdx];

				if (pCamera->IsSelected())
					pCamera->FileSave(*Clipboard);
			}

//			for (i = 0, pLight = m_pLights; pLight; pLight = pLight->m_pNext)
//				if (pLight->IsSelected())
//					i++;
//			Clipboard->Write(&i, sizeof(i));

//			for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
//				if (pLight->IsSelected())
//					pLight->FileSave(Clipboard);

			if (id == LC_EDIT_CUT)
			{
				RemoveSelectedObjects();
				gMainWindow->UpdateFocusObject(GetFocusObject());
				UpdateSelection();
				gMainWindow->UpdateAllViews();
				CheckPoint("Cutting");
			}

			g_App->ExportClipboard(Clipboard);
		} break;

		case LC_EDIT_PASTE:
		{
			lcFile* file = g_App->mClipboard;
			if (file == NULL)
				break;
			file->Seek(0, SEEK_SET);
			ClearSelection(false);

			lcArray<lcPiece*> PastedPieces;
			int NumPieces;
			file->ReadBuffer(&NumPieces, sizeof(NumPieces));

			while (NumPieces--)
			{
				lcPiece* Piece = new lcPiece(NULL);
				Piece->FileLoad(*file);
				PastedPieces.Add(Piece);
			}

			lcArray<lcGroup*> Groups;
			int NumGroups;
			file->ReadBuffer(&NumGroups, sizeof(NumGroups));

			while (NumGroups--)
			{
				lcGroup* Group = new lcGroup();
				Group->FileLoad(file);
				Groups.Add(Group);
			}

			for (int PieceIdx = 0; PieceIdx < PastedPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = PastedPieces[PieceIdx];
				Piece->CreateName(mPieces);
				Piece->SetStepShow(mCurrentStep);
				mPieces.Add(Piece);
				Piece->SetSelected(true);

				int GroupIndex = LC_POINTER_TO_INT(Piece->GetGroup());
				if (GroupIndex != -1)
					Piece->SetGroup(Groups[GroupIndex]);
				else
					Piece->SetGroup(NULL);
			}

			for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = Groups[GroupIdx];
				int GroupIndex = LC_POINTER_TO_INT(Group->mGroup);
				Group->mGroup = (GroupIndex != -1) ? Groups[GroupIndex] : NULL;
			}

			for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = Groups[GroupIdx];
				bool Add = false;

				for (int PieceIdx = 0; PieceIdx < PastedPieces.GetSize(); PieceIdx++)
				{
					lcPiece* Piece = PastedPieces[PieceIdx];

					for (lcGroup* PieceGroup = Piece->GetGroup(); PieceGroup; PieceGroup = PieceGroup->mGroup)
					{
						if (PieceGroup == Group)
						{
							Add = true;
							break;
						}
					}

					if (Add)
						break;
				}

				if (Add)
				{
					int a, max = 0;

					for (int SearchGroupIdx = 0; SearchGroupIdx < mGroups.GetSize(); SearchGroupIdx++)
					{
						lcGroup* SearchGroup = mGroups[SearchGroupIdx];

						if (strncmp("Pasted Group #", SearchGroup ->m_strName, 14) == 0)
							if (sscanf(SearchGroup ->m_strName + 14, "%d", &a) == 1)
								if (a > max)
									max = a;
					}

					sprintf(Group->m_strName, "Pasted Group #%.2d", max+1);
					mGroups.Add(Group);
				}
				else
					delete Group;
			}

			int NumCameras;
			file->ReadBuffer(&NumCameras, sizeof(NumCameras));

			while (NumCameras--)
			{
				lcCamera* pCamera = new lcCamera(false);
				pCamera->FileLoad(*file);
				pCamera->CreateName(mCameras);
				pCamera->SetSelected(true);
				mCameras.Add(pCamera);
			}

			// TODO: lights
			CalculateStep();
			CheckPoint("Pasting");
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();
			gMainWindow->UpdateAllViews();
		} break;
*/
	case LC_EDIT_FIND:
		if (gMainWindow->DoDialog(LC_DIALOG_FIND, &gMainWindow->mSearchOptions))
			lcGetActiveModel()->FindPiece(true, true);
		break;

	case LC_EDIT_FIND_NEXT:
		lcGetActiveModel()->FindPiece(false, true);
		break;

	case LC_EDIT_FIND_PREVIOUS:
		lcGetActiveModel()->FindPiece(false, false);
		break;

	case LC_EDIT_SELECT_ALL:
		lcGetActiveModel()->SelectAllPieces();
		break;

	case LC_EDIT_SELECT_NONE:
		lcGetActiveModel()->ClearSelection(true);
		break;

	case LC_EDIT_SELECT_INVERT:
		lcGetActiveModel()->InvertSelection();
		break;

	case LC_EDIT_SELECT_BY_NAME:
		lcGetActiveModel()->ShowSelectByNameDialog();
		break;

	case LC_VIEW_SPLIT_HORIZONTAL:
		SplitHorizontal();
		break;

	case LC_VIEW_SPLIT_VERTICAL:
		SplitVertical();
		break;

	case LC_VIEW_REMOVE_VIEW:
		RemoveView();
		break;

	case LC_VIEW_RESET_VIEWS:
		ResetViews();
		break;

	case LC_VIEW_FULLSCREEN:
		ToggleFullScreen();
		break;

	case LC_VIEW_PROJECTION_PERSPECTIVE:
		mActiveView->SetProjection(false);
		break;

	case LC_VIEW_PROJECTION_ORTHO:
		mActiveView->SetProjection(true);
		break;
/*
	case LC_VIEW_PROJECTION_FOCUS:
		{
			lcVector3 FocusVector;
			GetSelectionCenter(FocusVector);
			mActiveView->mCamera->SetFocalPoint(FocusVector, mCurrentStep, GetAddKeys());
			UpdateAllViews();
		}
		break;
*/
	case LC_PIECE_INSERT:
		lcGetActiveModel()->AddPiece();
		break;

	case LC_PIECE_DELETE:
		lcGetActiveModel()->DeleteSelectedObjects();
		break;

	case LC_PIECE_MOVE_PLUSX:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(lcMax(GetMoveXYSnap(), 0.01f), 0.0f, 0.0f)), true);
		break;

	case LC_PIECE_MOVE_MINUSX:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(-lcMax(GetMoveXYSnap(), 0.01f), 0.0f, 0.0f)), true);
		break;

	case LC_PIECE_MOVE_PLUSY:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(0.0f, lcMax(GetMoveXYSnap(), 0.01f), 0.0f)), true);
		break;

	case LC_PIECE_MOVE_MINUSY:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(0.0f, -lcMax(GetMoveXYSnap(), 0.01f), 0.0f)), true);
		break;

	case LC_PIECE_MOVE_PLUSZ:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, lcMax(GetMoveZSnap(), 0.01f))), true);
		break;

	case LC_PIECE_MOVE_MINUSZ:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, -lcMax(GetMoveZSnap(), 0.01f))), true);
		break;

	case LC_PIECE_ROTATE_PLUSX:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(lcVector3(lcMax(GetAngleSnap(), 1.0f), 0.0f, 0.0f)), true);
		break;

	case LC_PIECE_ROTATE_MINUSX:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(-lcVector3(lcMax(GetAngleSnap(), 1.0f), 0.0f, 0.0f)), true);
		break;

	case LC_PIECE_ROTATE_PLUSY:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(lcVector3(0.0f, lcMax(GetAngleSnap(), 1.0f), 0.0f)), true);
		break;

	case LC_PIECE_ROTATE_MINUSY:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(lcVector3(0.0f, -lcMax(GetAngleSnap(), 1.0f), 0.0f)), true);
		break;

	case LC_PIECE_ROTATE_PLUSZ:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, lcMax(GetAngleSnap(), 1.0f))), true);
		break;

	case LC_PIECE_ROTATE_MINUSZ:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, -lcMax(GetAngleSnap(), 1.0f))), true);
		break;

	case LC_PIECE_MINIFIG_WIZARD:
		lcGetActiveModel()->ShowMinifigDialog();
		break;

	case LC_PIECE_ARRAY:
		lcGetActiveModel()->ShowArrayDialog();
		break;

	case LC_PIECE_GROUP:
		lcGetActiveModel()->GroupSelection();
		break;

	case LC_PIECE_UNGROUP:
		lcGetActiveModel()->UngroupSelection();
		break;

	case LC_PIECE_GROUP_ADD:
		lcGetActiveModel()->AddSelectedPiecesToGroup();
		break;

	case LC_PIECE_GROUP_REMOVE:
		lcGetActiveModel()->RemoveFocusPieceFromGroup();
		break;

	case LC_PIECE_GROUP_EDIT:
		lcGetActiveModel()->ShowEditGroupsDialog();
		break;

	case LC_PIECE_HIDE_SELECTED:
		lcGetActiveModel()->HideSelectedPieces();
		break;

	case LC_PIECE_HIDE_UNSELECTED:
		lcGetActiveModel()->HideUnselectedPieces();
		break;

	case LC_PIECE_UNHIDE_ALL:
		lcGetActiveModel()->UnhideAllPieces();
		break;

	case LC_PIECE_SHOW_EARLIER:
		lcGetActiveModel()->ShowSelectedPiecesEarlier();
		break;

	case LC_PIECE_SHOW_LATER:
		lcGetActiveModel()->ShowSelectedPiecesLater();
		break;

	case LC_VIEW_PREFERENCES:
		g_App->ShowPreferencesDialog();
		break;

	case LC_VIEW_ZOOM_IN:
		lcGetActiveModel()->Zoom(mActiveView->mCamera, -10.0f);
		break;

	case LC_VIEW_ZOOM_OUT:
		lcGetActiveModel()->Zoom(mActiveView->mCamera, 10.0f);
		break;

	case LC_VIEW_ZOOM_EXTENTS:
		mActiveView->ZoomExtents();
		break;

	case LC_VIEW_LOOK_AT:
		mActiveView->LookAt();
		break;

	case LC_VIEW_TIME_NEXT:
		lcGetActiveModel()->ShowNextStep();
		break;

	case LC_VIEW_TIME_PREVIOUS:
		lcGetActiveModel()->ShowPreviousStep();
		break;

	case LC_VIEW_TIME_FIRST:
		lcGetActiveModel()->ShowFirstStep();
		break;

	case LC_VIEW_TIME_LAST:
		lcGetActiveModel()->ShowLastStep();
		break;

	case LC_VIEW_TIME_INSERT:
		lcGetActiveModel()->InsertStep();
		break;

	case LC_VIEW_TIME_DELETE:
		lcGetActiveModel()->RemoveStep();
		break;

	case LC_VIEW_VIEWPOINT_FRONT:
		mActiveView->SetViewpoint(LC_VIEWPOINT_FRONT);
		break;

	case LC_VIEW_VIEWPOINT_BACK:
		mActiveView->SetViewpoint(LC_VIEWPOINT_BACK);
		break;

	case LC_VIEW_VIEWPOINT_TOP:
		mActiveView->SetViewpoint(LC_VIEWPOINT_TOP);
		break;

	case LC_VIEW_VIEWPOINT_BOTTOM:
		mActiveView->SetViewpoint(LC_VIEWPOINT_BOTTOM);
		break;

	case LC_VIEW_VIEWPOINT_LEFT:
		mActiveView->SetViewpoint(LC_VIEWPOINT_LEFT);
		break;

	case LC_VIEW_VIEWPOINT_RIGHT:
		mActiveView->SetViewpoint(LC_VIEWPOINT_RIGHT);
		break;

	case LC_VIEW_VIEWPOINT_HOME:
		mActiveView->SetViewpoint(LC_VIEWPOINT_HOME);
		break;

	case LC_VIEW_CAMERA_NONE:
		mActiveView->RemoveCamera();
		break;

	case LC_VIEW_CAMERA1:
	case LC_VIEW_CAMERA2:
	case LC_VIEW_CAMERA3:
	case LC_VIEW_CAMERA4:
	case LC_VIEW_CAMERA5:
	case LC_VIEW_CAMERA6:
	case LC_VIEW_CAMERA7:
	case LC_VIEW_CAMERA8:
	case LC_VIEW_CAMERA9:
	case LC_VIEW_CAMERA10:
	case LC_VIEW_CAMERA11:
	case LC_VIEW_CAMERA12:
	case LC_VIEW_CAMERA13:
	case LC_VIEW_CAMERA14:
	case LC_VIEW_CAMERA15:
	case LC_VIEW_CAMERA16:
		mActiveView->SetCameraIndex(CommandId - LC_VIEW_CAMERA1);
		break;
/*
		case LC_VIEW_CAMERA_RESET:
		{
			for (int ViewIdx = 0; ViewIdx < mViews.GetSize(); ViewIdx++)
				mViews[ViewIdx]->SetDefaultCamera();

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				delete mCameras[CameraIdx];
			mCameras.RemoveAll();

			UpdateCameraMenu();
			UpdateFocusObject(GetFocusObject());
			UpdateAllViews();
		} break;
*/
	case LC_HELP_HOMEPAGE:
		g_App->OpenURL("http://www.leocad.org/");
		break;

	case LC_HELP_EMAIL:
		g_App->OpenURL("mailto:leozide@gmail.com?subject=LeoCAD");
		break;

	case LC_HELP_UPDATES:
		DoDialog(LC_DIALOG_CHECK_UPDATES, NULL);
		break;

	case LC_HELP_ABOUT:
		DoDialog(LC_DIALOG_ABOUT, NULL);

	case LC_VIEW_TIME_ADD_KEYS:
		SetAddKeys(!GetAddKeys());
		break;

	case LC_EDIT_SNAP_RELATIVE:
		lcGetPreferences().SetForceGlobalTransforms(!lcGetPreferences().mForceGlobalTransforms);
		break;

	case LC_EDIT_LOCK_X:
		SetLockX(!GetLockX());
		break;

	case LC_EDIT_LOCK_Y:
		SetLockY(!GetLockY());
		break;

	case LC_EDIT_LOCK_Z:
		SetLockZ(!GetLockZ());
		break;

	case LC_EDIT_LOCK_NONE:
		SetLockX(false);
		SetLockY(false);
		SetLockZ(false);
		break;

	case LC_EDIT_SNAP_MOVE_XY0:
	case LC_EDIT_SNAP_MOVE_XY1:
	case LC_EDIT_SNAP_MOVE_XY2:
	case LC_EDIT_SNAP_MOVE_XY3:
	case LC_EDIT_SNAP_MOVE_XY4:
	case LC_EDIT_SNAP_MOVE_XY5:
	case LC_EDIT_SNAP_MOVE_XY6:
	case LC_EDIT_SNAP_MOVE_XY7:
	case LC_EDIT_SNAP_MOVE_XY8:
	case LC_EDIT_SNAP_MOVE_XY9:
		SetMoveXYSnapIndex(CommandId - LC_EDIT_SNAP_MOVE_XY0);
		break;

	case LC_EDIT_SNAP_MOVE_Z0:
	case LC_EDIT_SNAP_MOVE_Z1:
	case LC_EDIT_SNAP_MOVE_Z2:
	case LC_EDIT_SNAP_MOVE_Z3:
	case LC_EDIT_SNAP_MOVE_Z4:
	case LC_EDIT_SNAP_MOVE_Z5:
	case LC_EDIT_SNAP_MOVE_Z6:
	case LC_EDIT_SNAP_MOVE_Z7:
	case LC_EDIT_SNAP_MOVE_Z8:
	case LC_EDIT_SNAP_MOVE_Z9:
		SetMoveZSnapIndex(CommandId - LC_EDIT_SNAP_MOVE_Z0);
		break;

	case LC_EDIT_SNAP_ANGLE0:
	case LC_EDIT_SNAP_ANGLE1:
	case LC_EDIT_SNAP_ANGLE2:
	case LC_EDIT_SNAP_ANGLE3:
	case LC_EDIT_SNAP_ANGLE4:
	case LC_EDIT_SNAP_ANGLE5:
	case LC_EDIT_SNAP_ANGLE6:
	case LC_EDIT_SNAP_ANGLE7:
	case LC_EDIT_SNAP_ANGLE8:
	case LC_EDIT_SNAP_ANGLE9:
		SetAngleSnapIndex(CommandId - LC_EDIT_SNAP_ANGLE0);
		break;

	case LC_EDIT_TRANSFORM:
		lcGetActiveModel()->TransformSelectedObjects(GetTransformType(), GetTransformAmount());
		break;

	case LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION:
	case LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION:
	case LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION:
	case LC_EDIT_TRANSFORM_RELATIVE_ROTATION:
		SetTransformType((lcTransformType)(CommandId - LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION));
		break;

	case LC_EDIT_ACTION_SELECT:
		SetTool(LC_TOOL_SELECT);
		break;

	case LC_EDIT_ACTION_INSERT:
		SetTool(LC_TOOL_INSERT);
		break;

	case LC_EDIT_ACTION_LIGHT:
		SetTool(LC_TOOL_LIGHT);
		break;

	case LC_EDIT_ACTION_SPOTLIGHT:
		SetTool(LC_TOOL_SPOTLIGHT);
		break;

	case LC_EDIT_ACTION_CAMERA:
		SetTool(LC_TOOL_CAMERA);
		break;

	case LC_EDIT_ACTION_MOVE:
		SetTool(LC_TOOL_MOVE);
		break;

	case LC_EDIT_ACTION_ROTATE:
		SetTool(LC_TOOL_ROTATE);
		break;

	case LC_EDIT_ACTION_DELETE:
		SetTool(LC_TOOL_ERASER);
		break;

	case LC_EDIT_ACTION_PAINT:
		SetTool(LC_TOOL_PAINT);
		break;

	case LC_EDIT_ACTION_ZOOM:
		SetTool(LC_TOOL_ZOOM);
		break;

	case LC_EDIT_ACTION_ZOOM_REGION:
		SetTool(LC_TOOL_ZOOM_REGION);
		break;

	case LC_EDIT_ACTION_PAN:
		SetTool(LC_TOOL_PAN);
		break;

	case LC_EDIT_ACTION_ROTATE_VIEW:
		SetTool(LC_TOOL_ROTATE_VIEW);
		break;

	case LC_EDIT_ACTION_ROLL:
		SetTool(LC_TOOL_ROLL);
		break;

	case LC_EDIT_CANCEL:
		mActiveView->CancelTrackingOrClearSelection();
		break;

	case LC_NUM_COMMANDS:
		break;
	}
}

#include "lc_global.h"
#include "lc_qmainwindow.h"
#include "lc_global.h"
#include "lc_library.h"
#include "lc_application.h"
#include "pieceinf.h"
#include "project.h"
#include "preview.h"
#include "lc_colorlistwidget.h"
#include "lc_previewwidget.h"
#include "lc_viewwidget.h"

static int PiecesSortFunc(const PieceInfo* a, const PieceInfo* b, void* SortData)
{
	if (a->IsSubPiece())
	{
		if (b->IsSubPiece())
			return strcmp(a->m_strDescription, b->m_strDescription);
		else
			return 1;
	}
	else
	{
		if (b->IsSubPiece())
			return -1;
		else
			return strcmp(a->m_strDescription, b->m_strDescription);
	}

	return 0;
}

lcQMainWindow::lcQMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	resize(800, 600);
//	centralWidget = new lcViewWidget(this, NULL);
//	centralWidget->mWindow->OnInitialUpdate();
//	setCentralWidget(centralWidget);

	QFrame *previewFrame = new QFrame;
	previewFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	setCentralWidget(previewFrame);

	QGridLayout *previewLayout = new QGridLayout(previewFrame);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	centralWidget = new lcViewWidget(previewFrame, NULL);
	centralWidget->mWindow->OnInitialUpdate();
	previewLayout->addWidget(centralWidget, 0, 0, 1, 1);

	createActions();
	createMenus();
	createToolBars();
	createStatusBar();

	lcPiecesLibrary* Lib = lcGetPiecesLibrary();

	QList<QTreeWidgetItem *> items;
	for (int i = 0; i < Lib->mCategories.GetSize(); i++)
	{
		QTreeWidgetItem* Category = new QTreeWidgetItem((QTreeWidget*)0, QStringList((const char*)Lib->mCategories[i].Name));
		items.append(Category);
		/*
		PtrArray<PieceInfo> SinglePieces, GroupedPieces;

		Lib->GetCategoryEntries(i, true, SinglePieces, GroupedPieces);

		SinglePieces += GroupedPieces;
		SinglePieces.Sort(PiecesSortFunc, NULL);

		for (int j = 0; j < SinglePieces.GetSize(); j++)
		{
			PieceInfo* Info = SinglePieces[j];
			QTreeWidgetItem* Item = new QTreeWidgetItem(Category, QStringList(Info->m_strDescription));

			if (GroupedPieces.FindIndex(Info) != -1)
			{
				PtrArray<PieceInfo> Patterns;
				Lib->GetPatternedPieces(Info, Patterns);

				for (int k = 0; k < Patterns.GetSize(); k++)
				{
					PieceInfo* child = Patterns[k];

					if (!Lib->PieceInCategory(child, Lib->mCategories[i].Keywords))
					continue;

					const char* desc = child->m_strDescription;
					int len = strlen(Info->m_strDescription);

					if (!strncmp(child->m_strDescription, Info->m_strDescription, len))
						desc += len;

					QTreeWidgetItem* Pattern = new QTreeWidgetItem(Item, QStringList(desc));
				}
			}
		}
*/
	}

	pieceList->insertTopLevelItems(0, items);

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	PieceInfo* Info = Library->FindPiece("3005", false);

	if (!Info)
		Info = Library->mPieces[0];

	if (Info)
	{
		lcGetActiveProject()->SetCurrentPiece(Info);
		PiecePreview* Preview = (PiecePreview*)piecePreview->mWindow;
		Preview->OnInitialUpdate();
		Preview->SetCurrentPiece(Info);
	}
}

lcQMainWindow::~lcQMainWindow()
{
}

void lcQMainWindow::createActions()
{
	QAction *action;
	memset(actions, 0, sizeof(actions));

	action = new QAction(QIcon(":/resources/file_new.png"), tr("&New"), this);
	action->setToolTip(tr("New Project"));
	action->setStatusTip(tr("Create a new project"));
	action->setShortcuts(QKeySequence::New);
	actions[LC_FILE_NEW] = action;

	action = new QAction(QIcon(":/resources/file_open.png"), tr("&Open..."), this);
	action->setToolTip(tr("Open Project"));
	action->setStatusTip(tr("Open an existing project"));
	action->setShortcuts(QKeySequence::Open);
	actions[LC_FILE_OPEN] = action;

	action = new QAction(tr("Merge..."), this);
	action->setStatusTip(tr("Merge the contents of another project with the current one"));
	actions[LC_FILE_MERGE] = action;
	
	action = new QAction(QIcon(":/resources/file_save.png"), tr("Save"), this);
	action->setToolTip(tr("Save Project"));
	action->setStatusTip(tr("Save the active project"));
	action->setShortcuts(QKeySequence::Save);
	actions[LC_FILE_SAVE] = action;

	action = new QAction(tr("Save As..."), this);
	action->setStatusTip(tr("Save the active project with a new name"));
	action->setShortcuts(QKeySequence::SaveAs);
	actions[LC_FILE_SAVEAS] = action;

	actions[LC_FILE_SAVE_IMAGE] = new QAction(tr("Save Image..."), this);
	actions[LC_FILE_EXPORT_3DS] = new QAction(tr("3D Studio..."), this);
	actions[LC_FILE_EXPORT_BRICKLINK] = new QAction(tr("BrickLink..."), this);
	actions[LC_FILE_EXPORT_HTML] = new QAction(tr("HTML..."), this);
	actions[LC_FILE_EXPORT_POVRAY] = new QAction(tr("POV-Ray..."), this);
	actions[LC_FILE_EXPORT_WAVEFRONT] = new QAction(tr("Wavefront..."), this);
	actions[LC_FILE_PROPERTIES] = new QAction(tr("Properties..."), this);
	actions[LC_FILE_PIECES_LIBRARY] = new QAction(tr("Pieces Library..."), this);
	actions[LC_FILE_TERRAIN_EDITOR] = new QAction(tr("Terrain Editor..."), this);
	actions[LC_FILE_PRINT] = new QAction(QIcon(":/resources/file_print.png"), tr("Print..."), this);
	actions[LC_FILE_PRINT_PREVIEW] = new QAction(QIcon(":/resources/file_print_preview.png"), tr("Print Preview"), this);
	actions[LC_FILE_PRINT_BOM] = new QAction(tr("Bill Of Materials..."), this);
	actions[LC_FILE_RECENT1] = new QAction(tr("1"), this);
	actions[LC_FILE_RECENT2] = new QAction(tr("2"), this);
	actions[LC_FILE_RECENT3] = new QAction(tr("3"), this);
	actions[LC_FILE_RECENT4] = new QAction(tr("4"), this);
	actions[LC_FILE_EXIT] = new QAction(tr("Exit"), this);

	action = new QAction(QIcon(":/resources/edit_undo.png"), tr("Undo"), this);
	action->setStatusTip(tr("Undo the last action"));
	action->setShortcuts(QKeySequence::Undo);
	actions[LC_EDIT_UNDO] = action;

	action = new QAction(QIcon(":/resources/edit_redo.png"), tr("Redo"), this);
	action->setStatusTip(tr("Redo the previously undone action"));
	action->setShortcuts(QKeySequence::Redo);
	actions[LC_EDIT_REDO] = action;

	action = new QAction(QIcon(":/resources/edit_cut.png"), tr("Cut"), this);
	action->setStatusTip(tr("Cut the selection and put it on the Clipboard"));
	action->setShortcuts(QKeySequence::Cut);
	actions[LC_EDIT_CUT] = action;

	action = new QAction(QIcon(":/resources/edit_copy.png"), tr("Copy"), this);
	action->setStatusTip(tr("Copy the selection and put it on the Clipboard"));
	action->setShortcuts(QKeySequence::Copy);
	actions[LC_EDIT_COPY] = action;

	action = new QAction(QIcon(":/resources/edit_paste.png"), tr("Paste"), this);
	action->setStatusTip(tr("Insert Clipboard contents"));
	action->setShortcuts(QKeySequence::Paste);
	actions[LC_EDIT_PASTE] = action;

	action = new QAction(tr("Select All"), this);
	action->setStatusTip(tr("Select all objects"));
	actions[LC_EDIT_SELECT_ALL] = action;

	action = new QAction(tr("Select None"), this);
	action->setStatusTip(tr("De-select everything"));
	actions[LC_EDIT_SELECT_NONE] = action;

	action = new QAction(tr("Select Invert"), this);
	action->setStatusTip(tr("Invert the current selection set"));
	actions[LC_EDIT_SELECT_INVERT] = action;

	action = new QAction(tr("Select By Name..."), this);
	action->setStatusTip(tr("Select objects by name"));
	actions[LC_EDIT_SELECT_BY_NAME] = action;

	actions[LC_EDIT_LOCK_TOGGLE] = new QAction(QIcon(":/resources/edit_lock.png"), tr("Lock"), this);

	action = new QAction(tr("Lock X"), this);
	action->setCheckable(true);
	actions[LC_EDIT_LOCK_X] = action;

	action = new QAction(tr("Lock Y"), this);
	actions[LC_EDIT_LOCK_Y] = action;
	action->setCheckable(true);

	action = new QAction(tr("Lock Z"), this);
	action->setCheckable(true);
	actions[LC_EDIT_LOCK_Z] = action;

	actions[LC_EDIT_LOCK_NONE]= new QAction(tr("Unlock All"), this);
	QMenu* lockMenu = new QMenu(tr("Lock Menu"));
	lockMenu->addAction(actions[LC_EDIT_LOCK_X]);
	lockMenu->addAction(actions[LC_EDIT_LOCK_Y]);
	lockMenu->addAction(actions[LC_EDIT_LOCK_Z]);
	lockMenu->addAction(actions[LC_EDIT_LOCK_NONE]);
	actions[LC_EDIT_LOCK_TOGGLE]->setMenu(lockMenu);

	actions[LC_EDIT_SNAP_TOGGLE] = new QAction(QIcon(":/resources/edit_snap_move.png"), tr("Snap"), this);

	action = new QAction(tr("Snap X"), this);
	action->setCheckable(true);
	actions[LC_EDIT_SNAP_X] = action;

	action = new QAction(tr("Snap Y"), this);
	action->setCheckable(true);
	actions[LC_EDIT_SNAP_Y] = action;

	action = new QAction(tr("Snap Z"), this);
	action->setCheckable(true);
	actions[LC_EDIT_SNAP_Z] = action;

	actions[LC_EDIT_SNAP_NONE] = new QAction(tr("Snap None"), this);
	actions[LC_EDIT_SNAP_ALL] = new QAction(tr("Snap All"), this);
	QMenu* snapMenu = new QMenu(tr("Snap Menu"));
	snapMenu->addAction(actions[LC_EDIT_SNAP_X]);
	snapMenu->addAction(actions[LC_EDIT_SNAP_Y]);
	snapMenu->addAction(actions[LC_EDIT_SNAP_Z]);
	snapMenu->addAction(actions[LC_EDIT_SNAP_NONE]);
	snapMenu->addAction(actions[LC_EDIT_SNAP_ALL]);
	actions[LC_EDIT_SNAP_TOGGLE]->setMenu(snapMenu);

	action = new QAction(QIcon(":/resources/edit_snap_angle.png"), tr("Snap Angle"), this);
	action->setCheckable(true);
	actions[LC_EDIT_SNAP_ANGLE] = action;

	actions[LC_EDIT_TRANSFORM] = new QAction(QIcon(":/resources/edit_transform.png"), tr("Transform"), this);
	actions[LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION] = new QAction(tr("Absolute Translation"), this);
	actions[LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION] = new QAction(tr("Relative Translation"), this);
	actions[LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION] = new QAction(tr("Absolute Rotation"), this);
	actions[LC_EDIT_TRANSFORM_RELATIVE_ROTATION] = new QAction(tr("Relative Rotation"), this);
	QMenu* transformMenu = new QMenu(tr("Transform"));
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION]);
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION]);
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION]);
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_RELATIVE_ROTATION]);
	actions[LC_EDIT_TRANSFORM]->setMenu(transformMenu);

	action = new QAction(QIcon(":/resources/action_insert.png"), tr("Insert"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_INSERT] = action;

	action = new QAction(QIcon(":/resources/action_light.png"), tr("Light"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_LIGHT] = action;

	action = new QAction(QIcon(":/resources/action_spotlight.png"), tr("Spot Light"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_SPOTLIGHT] = action;

	action = new QAction(QIcon(":/resources/action_camera.png"), tr("Camera"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_CAMERA] = action;

	action = new QAction(QIcon(":/resources/action_select.png"), tr("Select"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_SELECT] = action;

	action = new QAction(QIcon(":/resources/action_move.png"), tr("Move"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_MOVE] = action;

	action = new QAction(QIcon(":/resources/action_rotate.png"), tr("Rotate"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_ROTATE] = action;

	action = new QAction(QIcon(":/resources/action_delete.png"), tr("Delete"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_DELETE] = action;

	action = new QAction(QIcon(":/resources/action_paint.png"), tr("Paint"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_PAINT] = action;

	action = new QAction(QIcon(":/resources/action_zoom.png"), tr("Zoom"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_ZOOM] = action;

	action = new QAction(QIcon(":/resources/action_pan.png"), tr("Pan"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_PAN] = action;

	action = new QAction(QIcon(":/resources/action_rotate_view.png"), tr("Rotate View"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_ROTATE_VIEW] = action;

	action = new QAction(QIcon(":/resources/action_roll.png"), tr("Roll"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_ROLL] = action;

	action = new QAction(QIcon(":/resources/action_zoom_region.png"), tr("Zoom Region"), this);
	action->setCheckable(true);
	actions[LC_EDIT_ACTION_ZOOM_REGION] = action;

	QActionGroup *actionToolGroup = new QActionGroup(this);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_INSERT]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_LIGHT]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_SPOTLIGHT]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_CAMERA]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_SELECT]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_MOVE]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_ROTATE]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_DELETE]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_PAINT]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_ZOOM]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_PAN]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_ROTATE_VIEW]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_ROLL]);
	actionToolGroup->addAction(actions[LC_EDIT_ACTION_ZOOM_REGION]);

	actions[LC_VIEW_PREFERENCES] = new QAction(tr("Preferences..."), this);
	actions[LC_VIEW_ZOOM_IN] = new QAction(tr("Zoom In"), this);
	actions[LC_VIEW_ZOOM_OUT] = new QAction(tr("Zoom Out"), this);
	actions[LC_VIEW_ZOOM_EXTENTS]= new QAction(tr("Zoom Extents"), this);
	actions[LC_VIEW_VIEWPOINT_FRONT] = new QAction(tr("Front"), this);
	actions[LC_VIEW_VIEWPOINT_BACK] = new QAction(tr("Back"), this);
	actions[LC_VIEW_VIEWPOINT_LEFT] = new QAction(tr("Left"), this);
	actions[LC_VIEW_VIEWPOINT_RIGHT] = new QAction(tr("Right"), this);
	actions[LC_VIEW_VIEWPOINT_TOP] = new QAction(tr("Top"), this);
	actions[LC_VIEW_VIEWPOINT_BOTTOM] = new QAction(tr("Bottom"), this);
	actions[LC_VIEW_VIEWPOINT_HOME] = new QAction(tr("Home"), this);

	actions[LC_VIEW_TIME_FIRST] = new QAction(QIcon(":/resources/time_first.png"), tr("First"), this);
	actions[LC_VIEW_TIME_PREVIOUS] = new QAction(QIcon(":/resources/time_previous.png"), tr("Previous"), this);
	actions[LC_VIEW_TIME_NEXT] = new QAction(QIcon(":/resources/time_next.png"), tr("Next"), this);
	actions[LC_VIEW_TIME_LAST] = new QAction(QIcon(":/resources/time_last.png"), tr("Last"), this);
	actions[LC_VIEW_TIME_STOP] = new QAction(tr("Stop"), this);
	actions[LC_VIEW_TIME_PLAY] = new QAction(tr("Play"), this);
	actions[LC_VIEW_TIME_INSERT] = new QAction(tr("Insert"), this);
	actions[LC_VIEW_TIME_DELETE] = new QAction(tr("Delete"), this);

	actions[LC_PIECE_INSERT] = new QAction(tr("Insert"), this);
	actions[LC_PIECE_DELETE] = new QAction(tr("Delete"), this);
	actions[LC_PIECE_MINIFIG_WIZARD] = new QAction(tr("Minifig Wizard"), this);
	actions[LC_PIECE_ARRAY] = new QAction(tr("Array"), this);
	actions[LC_PIECE_COPY_KEYS] = new QAction(tr("Copy Keys"), this);
	actions[LC_PIECE_GROUP] = new QAction(tr("Group"), this);
	actions[LC_PIECE_UNGROUP] = new QAction(tr("Ungroup"), this);
	actions[LC_PIECE_GROUP_ADD] = new QAction(tr("Add To Group"), this);
	actions[LC_PIECE_GROUP_REMOVE] = new QAction(tr("Remove From Group"), this);
	actions[LC_PIECE_GROUP_EDIT] = new QAction(tr("Edit Groups"), this);
	actions[LC_PIECE_HIDE_SELECTED] = new QAction(tr("Hide Selected"), this);
	actions[LC_PIECE_HIDE_UNSELECTED] = new QAction(tr("Hide Unselected"), this);
	actions[LC_PIECE_UNHIDE_ALL] = new QAction(tr("Unhide All"), this);
	actions[LC_PIECE_SHOW_EARLIER] = new QAction(tr("Show Earlier"), this);
	actions[LC_PIECE_SHOW_LATER] = new QAction(tr("Show Later"), this);

	actions[LC_HELP_ABOUT] = new QAction(tr("About"), this);

	for (int Command = 0; Command < LC_NUM_COMMANDS; Command++)
		if (actions[Command])
			connect(actions[Command], SIGNAL(triggered()), this, SLOT(actionTriggered()));
}

void lcQMainWindow::createMenus()
{
	menuFile = menuBar()->addMenu(tr("&File"));
	menuFile->addAction(actions[LC_FILE_NEW]);
	menuFile->addAction(actions[LC_FILE_OPEN]);
	menuFile->addAction(actions[LC_FILE_MERGE]);
	menuFile->addSeparator();
	menuFile->addAction(actions[LC_FILE_SAVE]);
	menuFile->addAction(actions[LC_FILE_SAVEAS]);
	menuFile->addAction(actions[LC_FILE_SAVE_IMAGE]);
	QMenu* exportMenu = menuFile->addMenu(tr("Export"));
	exportMenu->addAction(actions[LC_FILE_EXPORT_3DS]);
	exportMenu->addAction(actions[LC_FILE_EXPORT_BRICKLINK]);
	exportMenu->addAction(actions[LC_FILE_EXPORT_HTML]);
	exportMenu->addAction(actions[LC_FILE_EXPORT_POVRAY]);
	exportMenu->addAction(actions[LC_FILE_EXPORT_WAVEFRONT]);
	menuFile->addSeparator();
	menuFile->addAction(actions[LC_FILE_PROPERTIES]);
	menuFile->addAction(actions[LC_FILE_PIECES_LIBRARY]);
	menuFile->addAction(actions[LC_FILE_TERRAIN_EDITOR]);
	menuFile->addSeparator();
	menuFile->addAction(actions[LC_FILE_PRINT]);
	menuFile->addAction(actions[LC_FILE_PRINT_PREVIEW]);
	menuFile->addAction(actions[LC_FILE_PRINT_BOM]);
	menuFile->addSeparator();
	menuFile->addAction(actions[LC_FILE_RECENT1]);
	menuFile->addAction(actions[LC_FILE_RECENT2]);
	menuFile->addAction(actions[LC_FILE_RECENT3]);
	menuFile->addAction(actions[LC_FILE_RECENT4]);
	actionFileRecentSeparator = menuFile->addSeparator();
	menuFile->addAction(actions[LC_FILE_EXIT]);

	menuEdit = menuBar()->addMenu(tr("&Edit"));
	menuEdit->addAction(actions[LC_EDIT_UNDO]);
	menuEdit->addAction(actions[LC_EDIT_REDO]);
	menuEdit->addSeparator();
	menuEdit->addAction(actions[LC_EDIT_CUT]);
	menuEdit->addAction(actions[LC_EDIT_COPY]);
	menuEdit->addAction(actions[LC_EDIT_PASTE]);
	menuEdit->addSeparator();
	menuEdit->addAction(actions[LC_EDIT_SELECT_ALL]);
	menuEdit->addAction(actions[LC_EDIT_SELECT_NONE]);
	menuEdit->addAction(actions[LC_EDIT_SELECT_INVERT]);
	menuEdit->addAction(actions[LC_EDIT_SELECT_BY_NAME]);

	menuView = menuBar()->addMenu(tr("&View"));
	menuView->addAction(actions[LC_VIEW_PREFERENCES]);
	menuView->addSeparator();
	menuView->addAction(actions[LC_VIEW_ZOOM_IN]);
	menuView->addAction(actions[LC_VIEW_ZOOM_OUT]);
	menuView->addAction(actions[LC_VIEW_ZOOM_EXTENTS]);
	QMenu* menuViewpoints = menuView->addMenu(tr("Viewpoints"));
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_FRONT]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_BACK]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_LEFT]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_RIGHT]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_TOP]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_BOTTOM]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_HOME]);

	menuPiece = menuBar()->addMenu(tr("&Piece"));

	menuHelp = menuBar()->addMenu(tr("&Help"));
}

void lcQMainWindow::createToolBars()
{
	standardToolBar = addToolBar(tr("Standard"));
	standardToolBar->addAction(actions[LC_FILE_NEW]);
	standardToolBar->addAction(actions[LC_FILE_OPEN]);
	standardToolBar->addAction(actions[LC_FILE_SAVE]);
	standardToolBar->addAction(actions[LC_FILE_PRINT]);
	standardToolBar->addAction(actions[LC_FILE_PRINT_PREVIEW]);
	standardToolBar->addSeparator();
	standardToolBar->addAction(actions[LC_EDIT_UNDO]);
	standardToolBar->addAction(actions[LC_EDIT_REDO]);
	standardToolBar->addAction(actions[LC_EDIT_CUT]);
	standardToolBar->addAction(actions[LC_EDIT_COPY]);
	standardToolBar->addAction(actions[LC_EDIT_PASTE]);
	standardToolBar->addSeparator();
	standardToolBar->addAction(actions[LC_EDIT_LOCK_TOGGLE]);
	standardToolBar->addAction(actions[LC_EDIT_SNAP_TOGGLE]);
	standardToolBar->addAction(actions[LC_EDIT_SNAP_ANGLE]);
	standardToolBar->addSeparator();
	standardToolBar->addAction(actions[LC_EDIT_TRANSFORM]);
	transformX = new QLineEdit();
	transformX->setMaximumWidth(75);
	standardToolBar->addWidget(transformX);
	transformY = new QLineEdit();
	transformY->setMaximumWidth(75);
	standardToolBar->addWidget(transformY);
	transformZ = new QLineEdit();
	transformZ->setMaximumWidth(75);
	standardToolBar->addWidget(transformZ);

	toolsToolBar = addToolBar(tr("Tools"));
	insertToolBarBreak(toolsToolBar);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_INSERT]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_LIGHT]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_SPOTLIGHT]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_CAMERA]);
	toolsToolBar->addSeparator();
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_SELECT]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_MOVE]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_ROTATE]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_DELETE]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_PAINT]);
	toolsToolBar->addSeparator();
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_ZOOM]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_PAN]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_ROTATE_VIEW]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_ROLL]);
	toolsToolBar->addAction(actions[LC_EDIT_ACTION_ZOOM_REGION]);

	timeToolBar = addToolBar(tr("Time"));
	timeToolBar->addAction(actions[LC_VIEW_TIME_FIRST]);
	timeToolBar->addAction(actions[LC_VIEW_TIME_PREVIOUS]);
	timeToolBar->addAction(actions[LC_VIEW_TIME_NEXT]);
	timeToolBar->addAction(actions[LC_VIEW_TIME_LAST]);

	piecesToolBar = new QDockWidget(tr("Pieces"), this);
	QWidget *piecesContents = new QWidget();
	QGridLayout *piecesLayout = new QGridLayout(piecesContents);
	piecesLayout->setSpacing(6);
	piecesLayout->setContentsMargins(6, 6, 6, 6);
	QSplitter *piecesSplitter = new QSplitter(Qt::Vertical, piecesContents);

	QFrame *previewFrame = new QFrame;
	previewFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	piecesSplitter->addWidget(previewFrame);

	QGridLayout *previewLayout = new QGridLayout(previewFrame);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	piecePreview = new lcPreviewWidget(previewFrame, centralWidget);
	previewLayout->addWidget(piecePreview, 0, 0, 1, 1);

	pieceList = new QTreeWidget(piecesSplitter);
	pieceList->setHeaderHidden(true);
	piecesSplitter->addWidget(pieceList);
	pieceCombo = new QComboBox(piecesSplitter);
	pieceCombo->setEditable(true);
	piecesSplitter->addWidget(pieceCombo);
	colorList = new lcColorListWidget(piecesSplitter);
	piecesSplitter->addWidget(colorList);

	piecesLayout->addWidget(piecesSplitter, 0, 0, 1, 1);

	piecesToolBar->setWidget(piecesContents);
	addDockWidget(static_cast<Qt::DockWidgetArea>(2), piecesToolBar);
}

void lcQMainWindow::createStatusBar()
{
	statusBar = new QStatusBar(this);
	setStatusBar(statusBar);

	QLabel* statusPositionLabel = new QLabel("XYZ");
	statusBar->addPermanentWidget(statusPositionLabel);

	QLabel* statusSnapLabel = new QLabel("XYZ");
	statusBar->addPermanentWidget(statusSnapLabel);

	QLabel* statusTimeLabel = new QLabel("XYZ");
	statusBar->addPermanentWidget(statusTimeLabel);
}

void lcQMainWindow::actionTriggered()
{
	QObject *action = sender();

	for (int Command = 0; Command < LC_NUM_COMMANDS; Command++)
	{
		if (action == actions[Command])
		{
			lcGetActiveProject()->HandleCommand((LC_COMMANDS)Command);
			break;
		}
	}
}

void lcQMainWindow::UpdateAction(int NewAction)
{
	QAction *action = actions[LC_EDIT_ACTION_FIRST + NewAction];

	if (action)
		action->setChecked(true);
}

void lcQMainWindow::UpdateSnap(lcuint32 Snap)
{
	actions[LC_EDIT_SNAP_ANGLE]->setChecked((Snap & LC_DRAW_SNAP_A) != 0);

	actions[LC_EDIT_SNAP_X]->setChecked((Snap & LC_DRAW_SNAP_X) != 0);
	actions[LC_EDIT_SNAP_Y]->setChecked((Snap & LC_DRAW_SNAP_Y) != 0);
	actions[LC_EDIT_SNAP_Z]->setChecked((Snap & LC_DRAW_SNAP_Z) != 0);

	actions[LC_EDIT_LOCK_X]->setChecked((Snap & LC_DRAW_LOCK_X) != 0);
	actions[LC_EDIT_LOCK_Y]->setChecked((Snap & LC_DRAW_LOCK_Y) != 0);
	actions[LC_EDIT_LOCK_Z]->setChecked((Snap & LC_DRAW_LOCK_Z) != 0);
}

void lcQMainWindow::UpdateUndoRedo(const char* UndoText, const char* RedoText)
{
	QAction *undoAction = actions[LC_EDIT_UNDO];
	QAction *redoAction = actions[LC_EDIT_REDO];

	if (UndoText)
	{
		undoAction->setEnabled(true);
		undoAction->setText(QString("%1 %2").arg(tr("Undo"), UndoText));
	}
	else
	{
		undoAction->setEnabled(false);
		undoAction->setText(tr("Undo"));
	}

	if (RedoText)
	{
		redoAction->setEnabled(true);
		redoAction->setText(QString("%1 %2").arg(tr("Redo"), RedoText));
	}
	else
	{
		redoAction->setEnabled(false);
		redoAction->setText(tr("Redo"));
	}
}

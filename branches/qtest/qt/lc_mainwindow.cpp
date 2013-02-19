#include "lc_global.h"
#include "lc_mainwindow.h"
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

lcMainWindow::lcMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	resize(800, 600);
	centralWidget = new lcViewWidget(this, NULL);
	centralWidget->mWindow->OnInitialUpdate();
	setCentralWidget(centralWidget);

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

lcMainWindow::~lcMainWindow()
{
}

void lcMainWindow::createActions()
{
	actionFileNew = new QAction(QIcon(":/resources/file_new.png"), tr("&New"), this);
	actionFileNew->setToolTip(tr("New Project"));
	actionFileNew->setStatusTip(tr("Create a new project"));
	actionFileNew->setShortcuts(QKeySequence::New);

	actionFileOpen = new QAction(QIcon(":/resources/file_open.png"), tr("&Open..."), this);
	actionFileOpen->setToolTip(tr("Open Project"));
	actionFileOpen->setStatusTip(tr("Open an existing project"));
	actionFileOpen->setShortcuts(QKeySequence::Open);

	actionFileMerge = new QAction(tr("Merge..."), this);
	actionFileMerge->setStatusTip(tr("Merge the contents of another project with the current one"));
	
	actionFileSave = new QAction(QIcon(":/resources/file_save.png"), tr("Save"), this);
	actionFileSave->setToolTip(tr("Save Project"));
	actionFileSave->setStatusTip(tr("Save the active project"));
	actionFileSave->setShortcuts(QKeySequence::Save);

	actionFileSaveAs = new QAction(tr("Save As..."), this);
	actionFileSaveAs->setStatusTip(tr("Save the active project with a new name"));
	actionFileSaveAs->setShortcuts(QKeySequence::SaveAs);

	actionFileSaveImage = new QAction(tr("Save Image..."), this);
	actionFileExport3DS = new QAction(tr("3D Studio..."), this);
	actionFileExportBrickLink = new QAction(tr("BrickLink..."), this);
	actionFileExportHTML = new QAction(tr("HTML..."), this);
	actionFileExportPOVRay = new QAction(tr("POV-Ray..."), this);
	actionFileExportWavefront = new QAction(tr("Wavefront..."), this);
	actionFileProperties = new QAction(tr("Properties..."), this);
	actionFilePiecesLibrary = new QAction(tr("Pieces Library..."), this);
	actionFileTerrainEditor = new QAction(tr("Terrain Editor..."), this);
	actionFilePrint = new QAction(QIcon(":/resources/file_print.png"), tr("Print..."), this);
	actionFilePrintPreview = new QAction(QIcon(":/resources/file_print_preview.png"), tr("Print Preview"), this);
	actionFilePrintBOM = new QAction(tr("Bill Of Materials..."), this);
	actionFileRecent1 = new QAction(tr("1"), this);
	actionFileRecent2 = new QAction(tr("2"), this);
	actionFileRecent3 = new QAction(tr("3"), this);
	actionFileRecent4 = new QAction(tr("4"), this);
	actionFileExit = new QAction(tr("Exit"), this);

	actionEditUndo = new QAction(QIcon(":/resources/edit_undo.png"), tr("Undo"), this);
	actionEditUndo->setStatusTip(tr("Undo the last action"));
	actionEditUndo->setShortcuts(QKeySequence::Undo);

	actionEditRedo = new QAction(QIcon(":/resources/edit_redo.png"), tr("Redo"), this);
	actionEditRedo->setStatusTip(tr("Redo the previously undone action"));
	actionEditRedo->setShortcuts(QKeySequence::Redo);

	actionEditCut = new QAction(QIcon(":/resources/edit_cut.png"), tr("Cut"), this);
	actionEditCut->setStatusTip(tr("Cut the selection and put it on the Clipboard"));
	actionEditCut->setShortcuts(QKeySequence::Cut);

	actionEditCopy = new QAction(QIcon(":/resources/edit_copy.png"), tr("Copy"), this);
	actionEditCopy->setStatusTip(tr("Copy the selection and put it on the Clipboard"));
	actionEditCopy->setShortcuts(QKeySequence::Copy);

	actionEditPaste = new QAction(QIcon(":/resources/edit_paste.png"), tr("Paste"), this);
	actionEditPaste->setStatusTip(tr("Insert Clipboard contents"));
	actionEditPaste->setShortcuts(QKeySequence::Paste);

	actionEditSelectAll = new QAction(tr("Select All"), this);
	actionEditSelectAll->setStatusTip(tr("Select all objects"));

	actionEditSelectNone = new QAction(tr("Select None"), this);
	actionEditSelectNone->setStatusTip(tr("De-select everything"));

	actionEditSelectInvert = new QAction(tr("Select Invert"), this);
	actionEditSelectInvert->setStatusTip(tr("Invert the current selection set"));

	actionEditSelectByName = new QAction(tr("Select By Name..."), this);
	actionEditSelectByName->setStatusTip(tr("Select objects by name"));

	actionToolInsert = new QAction(QIcon(":/resources/action_insert.xpm"), tr("Insert"), this);
	actionToolInsert->setCheckable(true);
	actionToolLight = new QAction(QIcon(":/resources/action_light.xpm"), tr("Light"), this);
	actionToolLight->setCheckable(true);
	actionToolSpotLight = new QAction(QIcon(":/resources/action_spotlight.xpm"), tr("Spot Light"), this);
	actionToolSpotLight->setCheckable(true);
	actionToolCamera = new QAction(QIcon(":/resources/action_camera.xpm"), tr("Camera"), this);
	actionToolCamera->setCheckable(true);
	actionToolSelect = new QAction(QIcon(":/resources/action_select.xpm"), tr("Select"), this);
	actionToolSelect->setCheckable(true);
	actionToolMove = new QAction(QIcon(":/resources/action_move.xpm"), tr("Move"), this);
	actionToolMove->setCheckable(true);
	actionToolRotate = new QAction(QIcon(":/resources/action_rotate.xpm"), tr("Rotate"), this);
	actionToolRotate->setCheckable(true);
	actionToolDelete = new QAction(QIcon(":/resources/action_delete.xpm"), tr("Delete"), this);
	actionToolDelete->setCheckable(true);
	actionToolPaint = new QAction(QIcon(":/resources/action_paint.xpm"), tr("Paint"), this);
	actionToolPaint->setCheckable(true);
	actionToolZoom = new QAction(QIcon(":/resources/action_zoom.xpm"), tr("Zoom"), this);
	actionToolZoom->setCheckable(true);
	actionToolPan = new QAction(QIcon(":/resources/action_pan.xpm"), tr("Pan"), this);
	actionToolPan->setCheckable(true);
	actionToolRotateView = new QAction(QIcon(":/resources/action_rotate_view.xpm"), tr("Rotate View"), this);
	actionToolRotateView->setCheckable(true);
	actionToolRoll = new QAction(QIcon(":/resources/action_roll.xpm"), tr("Roll"), this);
	actionToolRoll->setCheckable(true);
	actionToolZoomRegion = new QAction(QIcon(":/resources/action_zoom_region.xpm"), tr("Zoom Region"), this);
	actionToolZoomRegion->setCheckable(true);

	QActionGroup *actionToolGroup = new QActionGroup(this);
	actionToolGroup->addAction(actionToolInsert);
	actionToolGroup->addAction(actionToolLight);
	actionToolGroup->addAction(actionToolSpotLight);
	actionToolGroup->addAction(actionToolCamera);
	actionToolGroup->addAction(actionToolSelect);
	actionToolGroup->addAction(actionToolMove);
	actionToolGroup->addAction(actionToolRotate);
	actionToolGroup->addAction(actionToolDelete);
	actionToolGroup->addAction(actionToolPaint);
	actionToolGroup->addAction(actionToolZoom);
	actionToolGroup->addAction(actionToolPan);
	actionToolGroup->addAction(actionToolRotateView);
	actionToolGroup->addAction(actionToolRoll);
	actionToolGroup->addAction(actionToolZoomRegion);
	connect(actionToolGroup, SIGNAL(triggered(QAction*)), this, SLOT(toolTriggered(QAction*)));
}

void lcMainWindow::createMenus()
{
	menuFile = menuBar()->addMenu(tr("&File"));
	menuFile->addAction(actionFileNew);
	menuFile->addAction(actionFileOpen);
	menuFile->addAction(actionFileMerge);
	menuFile->addSeparator();
	menuFile->addAction(actionFileSave);
	menuFile->addAction(actionFileSaveAs);
	menuFile->addAction(actionFileSaveImage);
	QMenu* exportMenu = menuFile->addMenu(tr("Export"));
	exportMenu->addAction(actionFileExport3DS);
	exportMenu->addAction(actionFileExportBrickLink);
	exportMenu->addAction(actionFileExportHTML);
	exportMenu->addAction(actionFileExportPOVRay);
	exportMenu->addAction(actionFileExportWavefront);
	menuFile->addSeparator();
	menuFile->addAction(actionFileProperties);
	menuFile->addAction(actionFilePiecesLibrary);
	menuFile->addAction(actionFileTerrainEditor);
	menuFile->addSeparator();
	menuFile->addAction(actionFilePrint);
	menuFile->addAction(actionFilePrintPreview);
	menuFile->addAction(actionFilePrintBOM);
	menuFile->addSeparator();
	menuFile->addAction(actionFileRecent1);
	menuFile->addAction(actionFileRecent2);
	menuFile->addAction(actionFileRecent3);
	menuFile->addAction(actionFileRecent4);
	actionFileRecentSeparator = menuFile->addSeparator();
	menuFile->addAction(actionFileExit);

	menuEdit = menuBar()->addMenu(tr("&Edit"));
	menuEdit->addAction(actionEditUndo);
	menuEdit->addAction(actionEditRedo);
	menuEdit->addSeparator();
	menuEdit->addAction(actionEditCut);
	menuEdit->addAction(actionEditCopy);
	menuEdit->addAction(actionEditPaste);
	menuEdit->addSeparator();
	menuEdit->addAction(actionEditSelectAll);
	menuEdit->addAction(actionEditSelectNone);
	menuEdit->addAction(actionEditSelectInvert);
	menuEdit->addAction(actionEditSelectByName);

	menuView = menuBar()->addMenu(tr("&View"));
	/*
	menuView->addAction(actionViewPreferences);
	menuEdit->addSeparator();
	menuView->addAction(actionViewZoomIn);
	menuView->addAction(actionViewZoomOut);
	menuView->addAction(actionViewZoomExtents);
	QMenu* menuViewpoints = menuView->addMenu(tr("Viewpoints"));
	menuViewpoints->addAction(actionViewpointFront);
	menuViewpoints->addAction(actionViewpointBack);
	menuViewpoints->addAction(actionViewpointLeft);
	menuViewpoints->addAction(actionViewpointRight);
	menuViewpoints->addAction(actionViewpointTop);
	menuViewpoints->addAction(actionViewpointBottom);
	menuViewpoints->addAction(actionViewpointHome);
	*/

	menuPiece = menuBar()->addMenu(tr("&Piece"));

	menuHelp = menuBar()->addMenu(tr("&Help"));

/*
        POPUP "C&ameras"
        BEGIN
            MENUITEM "Dummy",                       ID_CAMERA_FIRST
        END
        POPUP "Ste&p"
        BEGIN
            MENUITEM "Fi&rst",                      ID_VIEW_STEP_FIRST
            MENUITEM "&Previous",                   ID_VIEW_STEP_PREVIOUS
            MENUITEM "Ne&xt",                       ID_VIEW_STEP_NEXT
            MENUITEM "&Last",                       ID_VIEW_STEP_LAST
            MENUITEM "Ch&oose...",                  ID_VIEW_STEP_CHOOSE
            MENUITEM SEPARATOR
            MENUITEM "&Insert",                     ID_VIEW_STEP_INSERT
            MENUITEM "&Delete",                     ID_VIEW_STEP_DELETE
        END
        MENUITEM SEPARATOR
        MENUITEM "Split Horizontally",          ID_VIEW_SPLITHORIZONTALLY
        MENUITEM "Split Vertically",            ID_VIEW_SPLITVERTICALLY
        MENUITEM "Delete View",                 ID_VIEW_DELETEVIEW
        MENUITEM "Reset Views",                 ID_VIEW_RESETVIEWS
        MENUITEM SEPARATOR
        POPUP "Toolbars"
        BEGIN
            MENUITEM "&Standard",                   ID_VIEW_TOOLBAR
            MENUITEM "&Drawing",                    ID_VIEW_TOOLS_BAR
            MENUITEM "&Animation",                  ID_VIEW_ANIMATION_BAR
            MENUITEM "&Pieces",                     ID_VIEW_PIECES_BAR
            MENUITEM "P&roperties",                 ID_VIEW_PROPERTIES_BAR
            MENUITEM "S&tatus Bar",                 ID_VIEW_STATUS_BAR
        END
        MENUITEM "&Full Screen\tCtrl+F",        ID_VIEW_FULLSCREEN
    END
    POPUP "&Piece"
    BEGIN
        MENUITEM "Insert\tIns",                 ID_PIECE_INSERT
        MENUITEM "Delete\tDel",                 ID_PIECE_DELETE
        MENUITEM "Array...",                    ID_PIECE_ARRAY
        MENUITEM "Minifig Wizard...",           ID_PIECE_MINIFIGWIZARD
        MENUITEM "Copy Keys",                   ID_PIECE_COPYKEYS
        MENUITEM SEPARATOR
        MENUITEM "Group...",                    ID_PIECE_GROUP
        MENUITEM "Ungroup",                     ID_PIECE_UNGROUP
        MENUITEM "Remove From Group",           ID_PIECE_DETACH
        MENUITEM "Add To Group",                ID_PIECE_ATTACH
        MENUITEM "Edit Groups...",              ID_PIECE_EDITGROUPS
        MENUITEM SEPARATOR
        MENUITEM "Hide Selected",               ID_PIECE_HIDESELECTED
        MENUITEM "Hide Unselected",             ID_PIECE_HIDEUNSELECTED
        MENUITEM "Unhide All",                  ID_PIECE_UNHIDEALL
    END
    POPUP "&Help"
    BEGIN
        MENUITEM "&Help Topics",                ID_HELP_FINDER
        MENUITEM SEPARATOR
        MENUITEM "LeoCAD Home page",            ID_HELP_LEOCADHOMEPAGE
        MENUITEM "Send E-Mail",                 ID_HELP_SENDEMAIL
        MENUITEM "Check for Updates",           ID_HELP_CHECKFORUPDATES
        MENUITEM SEPARATOR
        MENUITEM "&About LeoCAD...",            ID_APP_ABOUT
    END
END
*/
}

void lcMainWindow::createToolBars()
{
	standardToolBar = addToolBar(tr("Standard"));
	standardToolBar->addAction(actionFileNew);
	standardToolBar->addAction(actionFileOpen);
	standardToolBar->addAction(actionFileSave);
	standardToolBar->addAction(actionFilePrint);
	standardToolBar->addAction(actionFilePrintPreview);
	standardToolBar->addSeparator();
	standardToolBar->addAction(actionEditUndo);
	standardToolBar->addAction(actionEditRedo);
	standardToolBar->addAction(actionEditCut);
	standardToolBar->addAction(actionEditCopy);
	standardToolBar->addAction(actionEditPaste);
//	standardToolBar->addSeparator();

	toolsToolBar = addToolBar(tr("Tools"));
	insertToolBarBreak(toolsToolBar);
	toolsToolBar->addAction(actionToolInsert);
	toolsToolBar->addAction(actionToolLight);
	toolsToolBar->addAction(actionToolSpotLight);
	toolsToolBar->addAction(actionToolCamera);
	toolsToolBar->addSeparator();
	toolsToolBar->addAction(actionToolSelect);
	toolsToolBar->addAction(actionToolMove);
	toolsToolBar->addAction(actionToolRotate);
	toolsToolBar->addAction(actionToolDelete);
	toolsToolBar->addAction(actionToolPaint);
	toolsToolBar->addSeparator();
	toolsToolBar->addAction(actionToolZoom);
	toolsToolBar->addAction(actionToolPan);
	toolsToolBar->addAction(actionToolRotateView);
	toolsToolBar->addAction(actionToolRoll);
	toolsToolBar->addAction(actionToolZoomRegion);

	timeToolBar = addToolBar(tr("Time"));
//	timeToolBar->addAction(actionTime);

	piecesToolBar = new QDockWidget(tr("Pieces"), this);
	QWidget *piecesContents = new QWidget();
	QGridLayout *piecesLayout = new QGridLayout(piecesContents);
	piecesLayout->setSpacing(6);
	piecesLayout->setContentsMargins(11, 11, 11, 11);
	QSplitter *piecesSplitter = new QSplitter(Qt::Vertical, piecesContents);
	piecePreview = new lcPreviewWidget(piecesSplitter, centralWidget);
	piecesSplitter->addWidget(piecePreview);
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

void lcMainWindow::createStatusBar()
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

void lcMainWindow::toolTriggered(QAction *action)
{
	Project* project = lcGetActiveProject();

	if (action == actionToolInsert)
		project->SetAction(LC_ACTION_INSERT);
	else if (action == actionToolLight)
		project->SetAction(LC_ACTION_LIGHT);
	else if (action == actionToolSpotLight)
		project->SetAction(LC_ACTION_SPOTLIGHT);
	else if (action == actionToolCamera)
		project->SetAction(LC_ACTION_CAMERA);
	else if (action == actionToolSelect)
		project->SetAction(LC_ACTION_SELECT);
	else if (action == actionToolMove)
		project->SetAction(LC_ACTION_MOVE);
	else if (action == actionToolRotate)
		project->SetAction(LC_ACTION_ROTATE);
	else if (action == actionToolDelete)
		project->SetAction(LC_ACTION_ERASER);
	else if (action == actionToolPaint)
		project->SetAction(LC_ACTION_PAINT);
	else if (action == actionToolZoom)
		project->SetAction(LC_ACTION_ZOOM);
	else if (action == actionToolPan)
		project->SetAction(LC_ACTION_PAN);
	else if (action == actionToolRotateView)
		project->SetAction(LC_ACTION_ROTATE_VIEW);
	else if (action == actionToolRoll)
		project->SetAction(LC_ACTION_ROLL);
	else if (action == actionToolZoomRegion)
		project->SetAction(LC_ACTION_ZOOM_REGION);
}

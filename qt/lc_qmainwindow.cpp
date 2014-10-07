#include "lc_global.h"
#include <typeinfo>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include "lc_qmainwindow.h"
#include "lc_qutils.h"
#include "lc_qglwidget.h"
#include "lc_library.h"
#include "lc_application.h"
#include "pieceinf.h"
#include "project.h"
#include "preview.h"
#include "piece.h"
#include "camera.h"
#include "view.h"
#include "group.h"
#include "lc_qpartstree.h"
#include "lc_qcolorlist.h"
#include "lc_qpropertiestree.h"
#include "lc_shortcuts.h"
#include "system.h"
#include "lc_mainwindow.h"
#include "lc_profile.h"

lcQMainWindow::lcQMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	memset(actions, 0, sizeof(actions));

	setWindowIcon(QIcon(":/resources/icon64.png"));
	setWindowFilePath(QString());

	createActions();
	createToolBars();
	createMenus();
	createStatusBar();

	QFrame *previewFrame = new QFrame;
	previewFrame->setFrameShape(QFrame::StyledPanel);
	previewFrame->setFrameShadow(QFrame::Sunken);
	setCentralWidget(previewFrame);

	QGridLayout *previewLayout = new QGridLayout(previewFrame);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	QWidget *viewWidget = new lcQGLWidget(previewFrame, piecePreview, new View(lcGetActiveProject()), true);
	previewLayout->addWidget(viewWidget, 0, 0, 1, 1);

	connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardChanged()));
	clipboardChanged();

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	PieceInfo* Info = Library->FindPiece("3005", false);

	GL_EnableVertexBufferObject();

	if (!lcGetActiveProject()->GetPieces().IsEmpty())
	{
		for (int PieceIdx = 0; PieceIdx < Library->mPieces.GetSize(); PieceIdx++)
		{
			lcMesh* Mesh = Library->mPieces[PieceIdx]->mMesh;

			if (Mesh)
				Mesh->UpdateBuffers();
		}
	}

	if (!Info)
		Info = Library->mPieces[0];

	if (Info)
	{
		PiecePreview* Preview = (PiecePreview*)piecePreview->widget;
		gMainWindow->mPreviewWidget = Preview;
		Preview->SetCurrentPiece(Info);
	}

	QSettings settings;
	settings.beginGroup("MainWindow");
	resize(QSize(800, 600));
	move(QPoint(200, 200));
	restoreGeometry(settings.value("Geometry").toByteArray());
	restoreState(settings.value("State").toByteArray());
	settings.endGroup();
}

lcQMainWindow::~lcQMainWindow()
{
}

void lcQMainWindow::createActions()
{
	for (int Command = 0; Command < LC_NUM_COMMANDS; Command++)
	{
		QAction *action = new QAction(tr(gCommands[Command].MenuName), this);
		action->setStatusTip(tr(gCommands[Command].StatusText));
		connect(action, SIGNAL(triggered()), this, SLOT(actionTriggered()));
		addAction(action);
		actions[Command] = action;
	}

	actions[LC_FILE_NEW]->setToolTip(tr("New Project"));
	actions[LC_FILE_OPEN]->setToolTip(tr("Open Project"));
	actions[LC_FILE_SAVE]->setToolTip(tr("Save Project"));

	actions[LC_FILE_NEW]->setIcon(QIcon(":/resources/file_new.png"));
	actions[LC_FILE_OPEN]->setIcon(QIcon(":/resources/file_open.png"));
	actions[LC_FILE_SAVE]->setIcon(QIcon(":/resources/file_save.png"));
	actions[LC_FILE_SAVE_IMAGE]->setIcon(QIcon(":/resources/file_picture.png"));
	actions[LC_FILE_PRINT]->setIcon(QIcon(":/resources/file_print.png"));
	actions[LC_FILE_PRINT_PREVIEW]->setIcon(QIcon(":/resources/file_print_preview.png"));
	actions[LC_EDIT_UNDO]->setIcon(QIcon(":/resources/edit_undo.png"));
	actions[LC_EDIT_REDO]->setIcon(QIcon(":/resources/edit_redo.png"));
	actions[LC_EDIT_CUT]->setIcon(QIcon(":/resources/edit_cut.png"));
	actions[LC_EDIT_COPY]->setIcon(QIcon(":/resources/edit_copy.png"));
	actions[LC_EDIT_PASTE]->setIcon(QIcon(":/resources/edit_paste.png"));
	actions[LC_EDIT_ACTION_INSERT]->setIcon(QIcon(":/resources/action_insert.png"));
	actions[LC_EDIT_ACTION_LIGHT]->setIcon(QIcon(":/resources/action_light.png"));
	actions[LC_EDIT_ACTION_SPOTLIGHT]->setIcon(QIcon(":/resources/action_spotlight.png"));
	actions[LC_EDIT_ACTION_CAMERA]->setIcon(QIcon(":/resources/action_camera.png"));
	actions[LC_EDIT_ACTION_SELECT]->setIcon(QIcon(":/resources/action_select.png"));
	actions[LC_EDIT_ACTION_MOVE]->setIcon(QIcon(":/resources/action_move.png"));
	actions[LC_EDIT_ACTION_ROTATE]->setIcon(QIcon(":/resources/action_rotate.png"));
	actions[LC_EDIT_ACTION_DELETE]->setIcon(QIcon(":/resources/action_delete.png"));
	actions[LC_EDIT_ACTION_PAINT]->setIcon(QIcon(":/resources/action_paint.png"));
	actions[LC_EDIT_ACTION_ZOOM]->setIcon(QIcon(":/resources/action_zoom.png"));
	actions[LC_EDIT_ACTION_PAN]->setIcon(QIcon(":/resources/action_pan.png"));
	actions[LC_EDIT_ACTION_ROTATE_VIEW]->setIcon(QIcon(":/resources/action_rotate_view.png"));
	actions[LC_EDIT_ACTION_ROLL]->setIcon(QIcon(":/resources/action_roll.png"));
	actions[LC_EDIT_ACTION_ZOOM_REGION]->setIcon(QIcon(":/resources/action_zoom_region.png"));
	actions[LC_PIECE_SHOW_EARLIER]->setIcon(QIcon(":/resources/piece_show_earlier.png"));
	actions[LC_PIECE_SHOW_LATER]->setIcon(QIcon(":/resources/piece_show_later.png"));
	actions[LC_VIEW_SPLIT_HORIZONTAL]->setIcon(QIcon(":/resources/view_split_horizontal.png"));
	actions[LC_VIEW_SPLIT_VERTICAL]->setIcon(QIcon(":/resources/view_split_vertical.png"));
	actions[LC_VIEW_ZOOM_IN]->setIcon(QIcon(":/resources/view_zoomin.png"));
	actions[LC_VIEW_ZOOM_OUT]->setIcon(QIcon(":/resources/view_zoomout.png"));
	actions[LC_VIEW_ZOOM_EXTENTS]->setIcon(QIcon(":/resources/view_zoomextents.png"));
	actions[LC_VIEW_TIME_FIRST]->setIcon(QIcon(":/resources/time_first.png"));
	actions[LC_VIEW_TIME_PREVIOUS]->setIcon(QIcon(":/resources/time_previous.png"));
	actions[LC_VIEW_TIME_NEXT]->setIcon(QIcon(":/resources/time_next.png"));
	actions[LC_VIEW_TIME_LAST]->setIcon(QIcon(":/resources/time_last.png"));
	actions[LC_VIEW_TIME_ADD_KEYS]->setIcon(QIcon(":/resources/time_add_keys.png"));
	actions[LC_HELP_HOMEPAGE]->setIcon(QIcon(":/resources/help_homepage.png"));
	actions[LC_HELP_EMAIL]->setIcon(QIcon(":/resources/help_email.png"));

	actions[LC_EDIT_LOCK_X]->setCheckable(true);
	actions[LC_EDIT_LOCK_Y]->setCheckable(true);
	actions[LC_EDIT_LOCK_Z]->setCheckable(true);
	actions[LC_EDIT_SNAP_RELATIVE]->setCheckable(true);
	actions[LC_VIEW_CAMERA_NONE]->setCheckable(true);
	actions[LC_VIEW_TIME_ADD_KEYS]->setCheckable(true);

	QActionGroup *actionSnapXYGroup = new QActionGroup(this);
	for (int actionIdx = LC_EDIT_SNAP_MOVE_XY0; actionIdx <= LC_EDIT_SNAP_MOVE_XY9; actionIdx++)
	{
		actions[actionIdx]->setCheckable(true);
		actionSnapXYGroup->addAction(actions[actionIdx]);
	}

	QActionGroup *actionSnapZGroup = new QActionGroup(this);
	for (int actionIdx = LC_EDIT_SNAP_MOVE_Z0; actionIdx <= LC_EDIT_SNAP_MOVE_Z9; actionIdx++)
	{
		actions[actionIdx]->setCheckable(true);
		actionSnapZGroup->addAction(actions[actionIdx]);
	}

	QActionGroup *actionSnapAngleGroup = new QActionGroup(this);
	for (int actionIdx = LC_EDIT_SNAP_ANGLE0; actionIdx <= LC_EDIT_SNAP_ANGLE9; actionIdx++)
	{
		actions[actionIdx]->setCheckable(true);
		actionSnapAngleGroup->addAction(actions[actionIdx]);
	}

	QActionGroup *actionTransformTypeGroup = new QActionGroup(this);
	for (int actionIdx = LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION; actionIdx <= LC_EDIT_TRANSFORM_RELATIVE_ROTATION; actionIdx++)
	{
		actions[actionIdx]->setCheckable(true);
		actionTransformTypeGroup->addAction(actions[actionIdx]);
	}

	QActionGroup *actionToolGroup = new QActionGroup(this);
	for (int actionIdx = LC_EDIT_ACTION_FIRST; actionIdx <= LC_EDIT_ACTION_LAST; actionIdx++)
	{
		actions[actionIdx]->setCheckable(true);
		actionToolGroup->addAction(actions[actionIdx]);
	}

	QActionGroup *actionCameraGroup = new QActionGroup(this);
	actionCameraGroup->addAction(actions[LC_VIEW_CAMERA_NONE]);
	for (int actionIdx = LC_VIEW_CAMERA_FIRST; actionIdx <= LC_VIEW_CAMERA_LAST; actionIdx++)
	{
		actions[actionIdx]->setCheckable(true);
		actionCameraGroup->addAction(actions[actionIdx]);
	}

	QActionGroup *actionPerspectiveGroup = new QActionGroup(this);
	for (int actionIdx = LC_VIEW_PROJECTION_FIRST; actionIdx <= LC_VIEW_PROJECTION_LAST; actionIdx++)
	{
		actions[actionIdx]->setCheckable(true);
		actionPerspectiveGroup->addAction(actions[actionIdx]);
	}

	updateShortcuts();
}

void lcQMainWindow::createMenus()
{
	QMenu* transformMenu = new QMenu(tr("Transform"));
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION]);
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION]);
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_RELATIVE_ROTATION]);
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION]);
	actions[LC_EDIT_TRANSFORM]->setMenu(transformMenu);

	menuCamera = new QMenu(tr("C&ameras"));
	menuCamera->addAction(actions[LC_VIEW_CAMERA_NONE]);

	for (int actionIdx = LC_VIEW_CAMERA_FIRST; actionIdx <= LC_VIEW_CAMERA_LAST; actionIdx++)
		menuCamera->addAction(actions[actionIdx]);

	menuCamera->addSeparator();
	menuCamera->addAction(actions[LC_VIEW_CAMERA_RESET]);

	menuFile = menuBar()->addMenu(tr("&File"));
	menuFile->addAction(actions[LC_FILE_NEW]);
	menuFile->addAction(actions[LC_FILE_OPEN]);
	menuFile->addAction(actions[LC_FILE_MERGE]);
	menuFile->addSeparator();
	menuFile->addAction(actions[LC_FILE_SAVE]);
	menuFile->addAction(actions[LC_FILE_SAVEAS]);
	menuFile->addAction(actions[LC_FILE_SAVE_IMAGE]);
	QMenu* exportMenu = menuFile->addMenu(tr("&Export"));
	exportMenu->addAction(actions[LC_FILE_EXPORT_3DS]);
	exportMenu->addAction(actions[LC_FILE_EXPORT_BRICKLINK]);
	exportMenu->addAction(actions[LC_FILE_EXPORT_CSV]);
	exportMenu->addAction(actions[LC_FILE_EXPORT_HTML]);
	exportMenu->addAction(actions[LC_FILE_EXPORT_POVRAY]);
	exportMenu->addAction(actions[LC_FILE_EXPORT_WAVEFRONT]);
	menuFile->addSeparator();
	menuFile->addAction(actions[LC_FILE_PROPERTIES]);
//	menuFile->addAction(actions[LC_FILE_TERRAIN_EDITOR]);
	menuFile->addSeparator();
	menuFile->addAction(actions[LC_FILE_PRINT]);
	menuFile->addAction(actions[LC_FILE_PRINT_PREVIEW]);
//	menuFile->addAction(actions[LC_FILE_PRINT_BOM]);
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
	menuEdit->addAction(actions[LC_EDIT_FIND]);
	menuEdit->addAction(actions[LC_EDIT_FIND_NEXT]);
	menuEdit->addAction(actions[LC_EDIT_FIND_PREVIOUS]);
	menuEdit->addSeparator();
	menuEdit->addAction(actions[LC_EDIT_SELECT_ALL]);
	menuEdit->addAction(actions[LC_EDIT_SELECT_NONE]);
	menuEdit->addAction(actions[LC_EDIT_SELECT_INVERT]);
	menuEdit->addAction(actions[LC_EDIT_SELECT_BY_NAME]);

	menuView = menuBar()->addMenu(tr("&View"));
	menuView->addAction(actions[LC_VIEW_PREFERENCES]);
	menuView->addSeparator();
	menuView->addAction(actions[LC_VIEW_ZOOM_EXTENTS]);
	menuView->addAction(actions[LC_VIEW_LOOK_AT]);
	QMenu* menuViewpoints = menuView->addMenu(tr("&Viewpoints"));
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_FRONT]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_BACK]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_LEFT]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_RIGHT]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_TOP]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_BOTTOM]);
	menuViewpoints->addAction(actions[LC_VIEW_VIEWPOINT_HOME]);
	menuView->addMenu(menuCamera);
	QMenu* menuPerspective = menuView->addMenu(tr("Projection"));
	menuPerspective->addAction(actions[LC_VIEW_PROJECTION_PERSPECTIVE]);
	menuPerspective->addAction(actions[LC_VIEW_PROJECTION_ORTHO]);
	QMenu* menuStep = menuView->addMenu(tr("Ste&p"));
	menuStep->addAction(actions[LC_VIEW_TIME_FIRST]);
	menuStep->addAction(actions[LC_VIEW_TIME_PREVIOUS]);
	menuStep->addAction(actions[LC_VIEW_TIME_NEXT]);
	menuStep->addAction(actions[LC_VIEW_TIME_LAST]);
	menuStep->addSeparator();
	menuStep->addAction(actions[LC_VIEW_TIME_INSERT]);
	menuStep->addAction(actions[LC_VIEW_TIME_DELETE]);
	menuView->addSeparator();
	menuView->addAction(actions[LC_VIEW_SPLIT_HORIZONTAL]);
	menuView->addAction(actions[LC_VIEW_SPLIT_VERTICAL]);
	menuView->addAction(actions[LC_VIEW_REMOVE_VIEW]);
	menuView->addAction(actions[LC_VIEW_RESET_VIEWS]);
	menuView->addSeparator();
	QMenu *menuToolBars = menuView->addMenu(tr("T&oolbars"));
	menuToolBars->addAction(partsToolBar->toggleViewAction());
	menuToolBars->addAction(propertiesToolBar->toggleViewAction());
	menuToolBars->addSeparator();
	menuToolBars->addAction(standardToolBar->toggleViewAction());
	menuToolBars->addAction(toolsToolBar->toggleViewAction());
	menuToolBars->addAction(timeToolBar->toggleViewAction());
	menuView->addAction(actions[LC_VIEW_FULLSCREEN]);

	menuPiece = menuBar()->addMenu(tr("&Piece"));
	menuPiece->addAction(actions[LC_PIECE_INSERT]);
	menuPiece->addAction(actions[LC_PIECE_DELETE]);
	menuPiece->addAction(actions[LC_PIECE_ARRAY]);
	menuPiece->addAction(actions[LC_PIECE_MINIFIG_WIZARD]);
	menuPiece->addSeparator();
	menuPiece->addAction(actions[LC_PIECE_GROUP]);
	menuPiece->addAction(actions[LC_PIECE_UNGROUP]);
	menuPiece->addAction(actions[LC_PIECE_GROUP_REMOVE]);
	menuPiece->addAction(actions[LC_PIECE_GROUP_ADD]);
	menuPiece->addAction(actions[LC_PIECE_GROUP_EDIT]);
//	LC_PIECE_SHOW_EARLIER,
//	LC_PIECE_SHOW_LATER,
	menuPiece->addSeparator();
	menuPiece->addAction(actions[LC_PIECE_HIDE_SELECTED]);
	menuPiece->addAction(actions[LC_PIECE_HIDE_UNSELECTED]);
	menuPiece->addAction(actions[LC_PIECE_UNHIDE_ALL]);

	menuHelp = menuBar()->addMenu(tr("&Help"));
	menuHelp->addAction(actions[LC_HELP_HOMEPAGE]);
	menuHelp->addAction(actions[LC_HELP_EMAIL]);
#if !LC_DISABLE_UPDATE_CHECK
	menuHelp->addAction(actions[LC_HELP_UPDATES]);
#endif
	menuHelp->addSeparator();
	menuHelp->addAction(actions[LC_HELP_ABOUT]);
}

void lcQMainWindow::createToolBars()
{
	QMenu* lockMenu = new QMenu(tr("Lock Menu"));
	lockMenu->addAction(actions[LC_EDIT_LOCK_X]);
	lockMenu->addAction(actions[LC_EDIT_LOCK_Y]);
	lockMenu->addAction(actions[LC_EDIT_LOCK_Z]);
	lockMenu->addAction(actions[LC_EDIT_LOCK_NONE]);

	QAction* lockAction = new QAction(tr("Lock Menu"), this);
	lockAction->setStatusTip(tr("Toggle mouse movement on specific axes"));
	lockAction->setIcon(QIcon(":/resources/edit_lock.png"));
	lockAction->setMenu(lockMenu);

	QMenu* snapXYMenu = new QMenu(tr("Snap XY"));
	for (int actionIdx = LC_EDIT_SNAP_MOVE_XY0; actionIdx <= LC_EDIT_SNAP_MOVE_XY9; actionIdx++)
		snapXYMenu->addAction(actions[actionIdx]);

	QMenu* snapZMenu = new QMenu(tr("Snap Z"));
	for (int actionIdx = LC_EDIT_SNAP_MOVE_Z0; actionIdx <= LC_EDIT_SNAP_MOVE_Z9; actionIdx++)
		snapZMenu->addAction(actions[actionIdx]);

	QMenu* snapMenu = new QMenu(tr("Snap Menu"));
	snapMenu->addMenu(snapXYMenu);
	snapMenu->addMenu(snapZMenu);

	QAction* moveAction = new QAction(tr("Snap Move"), this);
	moveAction->setStatusTip(tr("Snap translations to fixed intervals"));
	moveAction->setIcon(QIcon(":/resources/edit_snap_move.png"));
	moveAction->setMenu(snapMenu);

	QMenu* snapAngleMenu = new QMenu(tr("Snap Angle Menu"));
	for (int actionIdx = LC_EDIT_SNAP_ANGLE0; actionIdx <= LC_EDIT_SNAP_ANGLE9; actionIdx++)
		snapAngleMenu->addAction(actions[actionIdx]);

	QAction* angleAction = new QAction(tr("Snap Rotate"), this);
	angleAction->setStatusTip(tr("Snap rotations to fixed intervals"));
	angleAction->setIcon(QIcon(":/resources/edit_snap_angle.png"));
	angleAction->setMenu(snapMenu);

	standardToolBar = addToolBar(tr("Standard"));
	standardToolBar->setObjectName("StandardToolbar");
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
//	standardToolBar->addAction(actions[LC_EDIT_SNAP_RELATIVE]); todo
	standardToolBar->addAction(lockAction);
	standardToolBar->addAction(moveAction);
	standardToolBar->addAction(angleAction);
	standardToolBar->addSeparator();
	standardToolBar->addAction(actions[LC_EDIT_TRANSFORM]);
	((QToolButton*)standardToolBar->widgetForAction(lockAction))->setPopupMode(QToolButton::InstantPopup);
	((QToolButton*)standardToolBar->widgetForAction(moveAction))->setPopupMode(QToolButton::InstantPopup);
	((QToolButton*)standardToolBar->widgetForAction(angleAction))->setPopupMode(QToolButton::InstantPopup);
	((QToolButton*)standardToolBar->widgetForAction(actions[LC_EDIT_TRANSFORM]))->setPopupMode(QToolButton::InstantPopup);

	QHBoxLayout *transformLayout = new QHBoxLayout;
	QWidget *transformWidget = new QWidget();
	transformWidget->setLayout(transformLayout);
	transformX = new QLineEdit();
	transformX->setMaximumWidth(75);
	transformLayout->addWidget(transformX);
	transformY = new QLineEdit();
	transformY->setMaximumWidth(75);
	transformLayout->addWidget(transformY);
	transformZ = new QLineEdit();
	transformZ->setMaximumWidth(75);
	transformLayout->addWidget(transformZ);
	transformLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
	standardToolBar->addWidget(transformWidget);
	connect(transformX, SIGNAL(returnPressed()), actions[LC_EDIT_TRANSFORM], SIGNAL(triggered()));
	connect(transformY, SIGNAL(returnPressed()), actions[LC_EDIT_TRANSFORM], SIGNAL(triggered()));
	connect(transformZ, SIGNAL(returnPressed()), actions[LC_EDIT_TRANSFORM], SIGNAL(triggered()));

	toolsToolBar = addToolBar(tr("Tools"));
	toolsToolBar->setObjectName("ToolsToolbar");
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
	timeToolBar->setObjectName("TimeToolbar");
	timeToolBar->addAction(actions[LC_VIEW_TIME_FIRST]);
	timeToolBar->addAction(actions[LC_VIEW_TIME_PREVIOUS]);
	timeToolBar->addAction(actions[LC_VIEW_TIME_NEXT]);
	timeToolBar->addAction(actions[LC_VIEW_TIME_LAST]);
	timeToolBar->addAction(actions[LC_PIECE_SHOW_EARLIER]);
	timeToolBar->addAction(actions[LC_PIECE_SHOW_LATER]);
	timeToolBar->addAction(actions[LC_VIEW_TIME_ADD_KEYS]);
	// TODO: add missing menu items

	partsToolBar = new QDockWidget(tr("Parts"), this);
	partsToolBar->setObjectName("PartsToolbar");
	partsToolBar->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	QWidget *partsContents = new QWidget();
	QGridLayout *partsLayout = new QGridLayout(partsContents);
	partsLayout->setSpacing(6);
	partsLayout->setContentsMargins(6, 6, 6, 6);
	QSplitter *partsSplitter = new QSplitter(Qt::Vertical, partsContents);

	QFrame *previewFrame = new QFrame(partsSplitter);
	previewFrame->setFrameShape(QFrame::StyledPanel);
	previewFrame->setFrameShadow(QFrame::Sunken);

	QGridLayout *previewLayout = new QGridLayout(previewFrame);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	int AASamples = lcGetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES);
	if (AASamples > 1)
	{
		QGLFormat format;
		format.setSampleBuffers(true);
		format.setSamples(AASamples);
		QGLFormat::setDefaultFormat(format);
	}

	piecePreview = new lcQGLWidget(previewFrame, NULL, new PiecePreview(), false);
	piecePreview->preferredSize = QSize(200, 100);
	previewLayout->addWidget(piecePreview, 0, 0, 1, 1);

	QSizePolicy treePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	treePolicy.setVerticalStretch(1);

	partsTree = new lcQPartsTree(partsSplitter);
	partsTree->setSizePolicy(treePolicy);
	connect(partsTree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(partsTreeItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));

	partSearch = new QLineEdit(partsSplitter);
	connect(partSearch, SIGNAL(returnPressed()), this, SLOT(partSearchReturn()));
	connect(partSearch, SIGNAL(textChanged(QString)), this, SLOT(partSearchChanged(QString)));

	QCompleter *completer = new QCompleter(new lcQPartsListModel(), this);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	partSearch->setCompleter(completer);

	QFrame *colorFrame = new QFrame(partsSplitter);
	colorFrame->setFrameShape(QFrame::StyledPanel);
	colorFrame->setFrameShadow(QFrame::Sunken);

	QGridLayout *colorLayout = new QGridLayout(colorFrame);
	colorLayout->setContentsMargins(0, 0, 0, 0);

	colorList = new lcQColorList(partsSplitter);
	colorLayout->addWidget(colorList);
	connect(colorList, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));

	partsLayout->addWidget(partsSplitter, 0, 0, 1, 1);

	partsToolBar->setWidget(partsContents);
	addDockWidget(Qt::RightDockWidgetArea, partsToolBar);

	propertiesToolBar = new QDockWidget(tr("Properties"), this);
	propertiesToolBar->setObjectName("PropertiesToolbar");
	propertiesToolBar->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

	propertiesWidget = new lcQPropertiesTree(propertiesToolBar);

	propertiesToolBar->setWidget(propertiesWidget);
	addDockWidget(Qt::RightDockWidgetArea, propertiesToolBar);

	tabifyDockWidget(partsToolBar, propertiesToolBar);
	partsToolBar->raise();
}

void lcQMainWindow::createStatusBar()
{
	statusBar = new QStatusBar(this);
	setStatusBar(statusBar);

	statusBarLabel = new QLabel();
	statusBar->addWidget(statusBarLabel);

	statusPositionLabel = new QLabel();
	statusBar->addPermanentWidget(statusPositionLabel);

	statusSnapLabel = new QLabel();
	statusBar->addPermanentWidget(statusSnapLabel);

	statusTimeLabel = new QLabel();
	statusBar->addPermanentWidget(statusTimeLabel);
}

void lcQMainWindow::closeEvent(QCloseEvent *event)
{
	if (lcGetActiveProject()->SaveModified())
	{
		event->accept();

		QSettings settings;
		settings.beginGroup("MainWindow");
		settings.setValue("Geometry", saveGeometry());
		settings.setValue("State", saveState());
		settings.endGroup();
	}
	else
		event->ignore();
}

QMenu *lcQMainWindow::createPopupMenu()
{
	QMenu *menuToolBars = new QMenu(this);

	menuToolBars->addAction(partsToolBar->toggleViewAction());
	menuToolBars->addAction(propertiesToolBar->toggleViewAction());
	menuToolBars->addSeparator();
	menuToolBars->addAction(standardToolBar->toggleViewAction());
	menuToolBars->addAction(toolsToolBar->toggleViewAction());
	menuToolBars->addAction(timeToolBar->toggleViewAction());

	return menuToolBars;
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

void lcQMainWindow::partsTreeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	if (!current)
		return;

	PieceInfo *info = (PieceInfo*)current->data(0, lcQPartsTree::PartInfoRole).value<void*>();

	if (info)
	{
		PiecePreview* preview = (PiecePreview*)piecePreview->widget;
		preview->SetCurrentPiece(info);
	}
}

void lcQMainWindow::colorChanged(int colorIndex)
{
	gMainWindow->SetColorIndex(colorIndex);
}

void lcQMainWindow::partSearchReturn()
{
	partsTree->searchParts(partSearch->text());
}

void lcQMainWindow::partSearchChanged(const QString& text)
{
	const QByteArray textConv = text.toLocal8Bit();
	const char* searchString = textConv.data();
	int length = strlen(searchString);

	if (!length)
		return;

	lcPiecesLibrary *library = lcGetPiecesLibrary();
	PieceInfo* bestMatch = NULL;

	for (int partIndex = 0; partIndex < library->mPieces.GetSize(); partIndex++)
	{
		PieceInfo *info = library->mPieces[partIndex];

		if (strncasecmp(searchString, info->m_strDescription, length) == 0)
		{
			if (!bestMatch || strcasecmp(bestMatch->m_strDescription, info->m_strDescription) > 0)
				bestMatch = info;
		}
		else if (strncasecmp(searchString, info->m_strName, length) == 0)
		{
			if (!bestMatch || strcasecmp(bestMatch->m_strName, info->m_strName) > 0)
				bestMatch = info;
		}
	}

	if (bestMatch)
		partsTree->setCurrentPart(bestMatch);
}

void lcQMainWindow::clipboardChanged()
{
	const QString mimeType("application/vnd.leocad-clipboard");
	const QMimeData *mimeData = QApplication::clipboard()->mimeData();
	lcMemFile *clipboard = NULL;

	if (mimeData->hasFormat(mimeType))
	{
		QByteArray clipboardData = mimeData->data(mimeType);

		clipboard = new lcMemFile();
		clipboard->WriteBuffer(clipboardData.constData(), clipboardData.size());
	}

	g_App->SetClipboard(clipboard);
}

void lcQMainWindow::splitView(Qt::Orientation orientation)
{
	QWidget *focus = focusWidget();

	if (typeid(*focus) != typeid(lcQGLWidget))
		return;

	QWidget *parent = focus->parentWidget();
	QSplitter *splitter;
	QList<int> sizes;

	if (parent == centralWidget())
	{
		splitter = new QSplitter(orientation, parent);
		parent->layout()->addWidget(splitter);
		splitter->addWidget(focus);
		splitter->addWidget(new lcQGLWidget(centralWidget(), piecePreview, new View(lcGetActiveProject()), true));
	}
	else
	{
		QSplitter *parentSplitter = (QSplitter*)parent;	
		sizes = parentSplitter->sizes();
		int focusIndex = parentSplitter->indexOf(focus);

		splitter = new QSplitter(orientation, parent);
		parentSplitter->insertWidget(focusIndex, splitter);
		splitter->addWidget(focus);
		splitter->addWidget(new lcQGLWidget(centralWidget(), piecePreview, new View(lcGetActiveProject()), true));

		parentSplitter->setSizes(sizes);
	}

	sizes.clear();
	sizes.append(10);
	sizes.append(10);
	splitter->setSizes(sizes);
}

void lcQMainWindow::splitHorizontal()
{
	splitView(Qt::Vertical);
}

void lcQMainWindow::splitVertical()
{
	splitView(Qt::Horizontal);
}

void lcQMainWindow::removeView()
{
	QWidget *focus = focusWidget();

	if (typeid(*focus) != typeid(lcQGLWidget))
		return;

	QWidget *parent = focus->parentWidget();

	if (parent == centralWidget())
		return;

	QWidget *parentParentWidget = parent->parentWidget();
	QSplitter *parentSplitter = (QSplitter*)parent;
	int focusIndex = parentSplitter->indexOf(focus);

	if (parentParentWidget == centralWidget())
	{
		QLayout* centralLayout = parentParentWidget->layout();

		centralLayout->addWidget(parentSplitter->widget(!focusIndex));
		centralLayout->removeWidget(parent);

		return;
	}

	QSplitter* parentParentSplitter = (QSplitter*)parentParentWidget;
	QList<int> sizes = parentParentSplitter->sizes();

	int parentIndex = parentParentSplitter->indexOf(parent);
	parentParentSplitter->insertWidget(!parentIndex, focus);

	delete parent;

	parentParentSplitter->setSizes(sizes);
}

void lcQMainWindow::resetViews()
{
	QLayout* centralLayout = centralWidget()->layout();
	delete centralLayout->itemAt(0)->widget();
	centralLayout->addWidget(new lcQGLWidget(centralWidget(), piecePreview, new View(lcGetActiveProject()), true));
}

void lcQMainWindow::print(QPrinter *printer)
{
	Project *project = lcGetActiveProject();
	int docCopies;
	int pageCopies;

	int rows = lcGetProfileInt(LC_PROFILE_PRINT_ROWS);
	int columns = lcGetProfileInt(LC_PROFILE_PRINT_COLUMNS);
	int stepsPerPage = rows * columns;
	int pageCount = (project->GetLastStep() + stepsPerPage - 1) / stepsPerPage;

	if (printer->collateCopies())
	{
		docCopies = 1;
		pageCopies = printer->supportsMultipleCopies() ? 1 : printer->copyCount();
	}
	else
	{
		docCopies = printer->supportsMultipleCopies() ? 1 : printer->copyCount();
		pageCopies = 1;
	}

	int fromPage = printer->fromPage();
	int toPage = printer->toPage();
	bool ascending = true;

	if (fromPage == 0 && toPage == 0)
	{
		fromPage = 1;
		toPage = pageCount;
	}

	fromPage = qMax(1, fromPage);
	toPage = qMin(pageCount, toPage);

	if (toPage < fromPage)
		return;

	if (printer->pageOrder() == QPrinter::LastPageFirst)
	{
		int tmp = fromPage;
		fromPage = toPage;
		toPage = tmp;
		ascending = false;
	}

	piecePreview->makeCurrent();

	QRect pageRect = printer->pageRect();

	int stepWidth = pageRect.width() / columns;
	int stepHeight = pageRect.height() / rows;

	GLint maxTexture;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexture);

	maxTexture = qMin(maxTexture, 2048);

	int tileWidth = qMin(stepWidth, maxTexture);
	int tileHeight = qMin(stepHeight, maxTexture);
	float aspectRatio = (float)stepWidth / (float)stepHeight;

	View view(project);
	view.SetCamera(gMainWindow->GetActiveView()->mCamera, false);
	view.mWidth = tileWidth;
	view.mHeight = tileHeight;
	view.SetContext(piecePreview->widget->mContext);

	GL_BeginRenderToTexture(tileWidth, tileHeight);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	lcStep previousTime = project->GetCurrentStep();

	QPainter painter(printer);
	lcuint8 *buffer = (lcuint8*)malloc(tileWidth * tileHeight * 4);
	// TODO: option to print background

	for (int docCopy = 0; docCopy < docCopies; docCopy++)
	{
		int page = fromPage;

		for (;;)
		{
			for (int pageCopy = 0; pageCopy < pageCopies; pageCopy++)
			{
				if (printer->printerState() == QPrinter::Aborted || printer->printerState() == QPrinter::Error)
					return;

				lcStep currentStep = 1 + ((page - 1) * rows * columns);

				for (int row = 0; row < rows; row++)
				{
					for (int column = 0; column < columns; column++)
					{
						if (currentStep > project->GetLastStep())
							break;

						project->SetCurrentStep(currentStep);

						if (stepWidth > tileWidth || stepHeight > tileHeight)
						{
							lcuint8* imageBuffer = (lcuint8*)malloc(stepWidth * stepHeight * 4);

							lcCamera* camera = view.mCamera;
							camera->StartTiledRendering(tileWidth, tileHeight, stepWidth, stepHeight, aspectRatio);
							do 
							{
								project->Render(&view, true);

								int tileRow, tileColumn, currentTileWidth, currentTileHeight;
								camera->GetTileInfo(&tileRow, &tileColumn, &currentTileWidth, &currentTileHeight);

								glFinish();
								glReadPixels(0, 0, currentTileWidth, currentTileHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

								lcuint32 tileY = 0;
								if (tileRow != 0)
									tileY = tileRow * tileHeight - (tileHeight - stepHeight % tileHeight);

								lcuint32 tileStart = ((tileColumn * tileWidth) + (tileY * stepWidth)) * 4;

								for (int y = 0; y < currentTileHeight; y++)
								{
									lcuint8* src = buffer + (currentTileHeight - y - 1) * currentTileWidth * 4;
									lcuint8* dst = imageBuffer + tileStart + y * stepWidth * 4;

									for (int x = 0; x < currentTileWidth; x++)
									{
										*dst++ = src[2];
										*dst++ = src[1];
										*dst++ = src[0];
										*dst++ = 255;

										src += 4;
									}
								}
							} while (camera->EndTile());

							QImage image = QImage((const lcuint8*)imageBuffer, stepWidth, stepHeight, QImage::Format_ARGB32_Premultiplied);

							QRect rect = painter.viewport();
							int left = rect.x() + (stepWidth * column);
							int bottom = rect.y() + (stepHeight * row);

							painter.drawImage(left, bottom, image);

							free(imageBuffer);
						}
						else
						{
							project->Render(&view, true);

							glFinish();
							glReadPixels(0, 0, tileWidth, tileHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

							for (int y = 0; y < (tileHeight + 1) / 2; y++)
							{
								lcuint8* top = (lcuint8*)buffer + ((tileHeight - y - 1) * tileWidth * 4);
								lcuint8* bottom = (lcuint8*)buffer + y * tileWidth * 4;

								for (int x = 0; x < tileWidth; x++)
								{
									lcuint8 red = top[0];
									lcuint8 green = top[1];
									lcuint8 blue = top[2];
									lcuint8 alpha = 255;//top[3];

									top[0] = bottom[2];
									top[1] = bottom[1];
									top[2] = bottom[0];
									top[3] = 255;//bottom[3];

									bottom[0] = blue;
									bottom[1] = green;
									bottom[2] = red;
									bottom[3] = alpha;

									top += 4;
									bottom +=4;
								}
							}

							QImage image = QImage((const lcuint8*)buffer, tileWidth, tileHeight, QImage::Format_ARGB32);

							QRect rect = painter.viewport();
							int left = rect.x() + (stepWidth * column);
							int bottom = rect.y() + (stepHeight * row);

							painter.drawImage(left, bottom, image);
						}

						// TODO: add print options somewhere but Qt doesn't allow changes to the page setup dialog
//						DWORD dwPrint = theApp.GetProfileInt("Settings","Print", PRINT_NUMBERS|PRINT_BORDER);

						QRect rect = painter.viewport();
						int left = rect.x() + (stepWidth * column);
						int right = rect.x() + (stepWidth * (column + 1));
						int top = rect.y() + (stepHeight * row);
						int bottom = rect.y() + (stepHeight * (row + 1));

//						if (print text)
						{
							QFont font("Helvetica", printer->resolution());
							painter.setFont(font);

							QFontMetrics fontMetrics(font);

							int textTop = top + printer->resolution() / 2 + fontMetrics.ascent();
							int textLeft = left + printer->resolution() / 2;

							painter.drawText(textLeft, textTop, QString::number(currentStep));
						}

//						if (print border)
						{
							QPen blackPen(Qt::black, 2);
							painter.setPen(blackPen);

							if (row == 0)
								painter.drawLine(left, top, right, top);
							if (column == 0)
								painter.drawLine(left, top, left, bottom);
							painter.drawLine(left, bottom, right, bottom);
							painter.drawLine(right, top, right, bottom);
						}

						currentStep++;
					}
				}

				// TODO: print header and footer

				if (pageCopy < pageCopies - 1)
					printer->newPage();
			}

			if (page == toPage)
				break;

			if (ascending)
				page++;
			else
				page--;

			printer->newPage();
		}

		if (docCopy < docCopies - 1)
			printer->newPage();
	}

	free(buffer);

	project->SetCurrentStep(previousTime);

	GL_EndRenderToTexture();
}

void lcQMainWindow::showPrintDialog()
{
	Project *project = lcGetActiveProject();
	int rows = lcGetProfileInt(LC_PROFILE_PRINT_ROWS);
	int columns = lcGetProfileInt(LC_PROFILE_PRINT_COLUMNS);
	int stepsPerPage = rows * columns;
	int pageCount = (project->GetLastStep() + stepsPerPage - 1) / stepsPerPage;

	QPrinter printer(QPrinter::HighResolution);
	printer.setFromTo(1, pageCount + 1);

	QPrintDialog printDialog(&printer, this);

	if (printDialog.exec() == QDialog::Accepted)
		print(&printer);
}

void lcQMainWindow::togglePrintPreview()
{
	// todo: print preview inside main window

	Project *project = lcGetActiveProject();
	int rows = lcGetProfileInt(LC_PROFILE_PRINT_ROWS);
	int columns = lcGetProfileInt(LC_PROFILE_PRINT_COLUMNS);
	int stepsPerPage = rows * columns;
	int pageCount = (project->GetLastStep() + stepsPerPage - 1) / stepsPerPage;

	QPrinter printer(QPrinter::ScreenResolution);
	printer.setFromTo(1, pageCount + 1);

	QPrintPreviewDialog preview(&printer, this);
	connect(&preview, SIGNAL(paintRequested(QPrinter*)), SLOT(print(QPrinter*)));
	preview.exec();
}

void lcQMainWindow::toggleFullScreen()
{
	// todo: hide toolbars and menu
	// todo: create fullscreen toolbar or support esc key to go back
	if (isFullScreen())
		showNormal();
	else
		showFullScreen();
}

void lcQMainWindow::updateFocusObject(lcObject *focus)
{
	propertiesWidget->updateFocusObject(focus);

	lcVector3 position;
	lcGetActiveProject()->GetFocusPosition(position);

	QString label("X: %1 Y: %2 Z: %3");
	label = label.arg(QString::number(position[0], 'f', 2), QString::number(position[1], 'f', 2), QString::number(position[2], 'f', 2));
	statusPositionLabel->setText(label);
}

void lcQMainWindow::updateSelectedObjects(int flags, int selectedCount, lcObject* focus)
{
	actions[LC_EDIT_CUT]->setEnabled(flags & LC_SEL_SELECTED);
	actions[LC_EDIT_COPY]->setEnabled(flags & LC_SEL_SELECTED);
	actions[LC_EDIT_FIND]->setEnabled((flags & LC_SEL_NO_PIECES) == 0);
	actions[LC_EDIT_FIND_NEXT]->setEnabled((flags & LC_SEL_NO_PIECES) == 0);
	actions[LC_EDIT_FIND_PREVIOUS]->setEnabled((flags & LC_SEL_NO_PIECES) == 0);
	actions[LC_EDIT_SELECT_INVERT]->setEnabled((flags & LC_SEL_NO_PIECES) == 0);
	actions[LC_EDIT_SELECT_BY_NAME]->setEnabled((flags & LC_SEL_NO_PIECES) == 0);
	actions[LC_EDIT_SELECT_NONE]->setEnabled(flags & LC_SEL_SELECTED);
	actions[LC_EDIT_SELECT_ALL]->setEnabled(flags & LC_SEL_UNSELECTED);

	actions[LC_PIECE_DELETE]->setEnabled(flags & LC_SEL_SELECTED);
	actions[LC_PIECE_ARRAY]->setEnabled(flags & LC_SEL_PIECE);
	actions[LC_PIECE_HIDE_SELECTED]->setEnabled(flags & LC_SEL_PIECE);
	actions[LC_PIECE_UNHIDE_ALL]->setEnabled(flags & LC_SEL_HIDDEN);
	actions[LC_PIECE_HIDE_UNSELECTED]->setEnabled(flags & LC_SEL_UNSELECTED);
	actions[LC_PIECE_GROUP]->setEnabled(flags & LC_SEL_CAN_GROUP);
	actions[LC_PIECE_UNGROUP]->setEnabled(flags & LC_SEL_GROUPED);
	actions[LC_PIECE_GROUP_ADD]->setEnabled((flags & (LC_SEL_GROUPED | LC_SEL_FOCUS_GROUPED)) == LC_SEL_GROUPED);
	actions[LC_PIECE_GROUP_REMOVE]->setEnabled(flags & LC_SEL_FOCUS_GROUPED);
	actions[LC_PIECE_GROUP_EDIT]->setEnabled((flags & LC_SEL_NO_PIECES) == 0);
	actions[LC_PIECE_SHOW_EARLIER]->setEnabled(flags & LC_SEL_PIECE); // FIXME: disable if current step is 1
	actions[LC_PIECE_SHOW_LATER]->setEnabled(flags & LC_SEL_PIECE);

	QString message;

	if ((selectedCount == 1) && (focus != NULL))
	{
		if (focus->IsPiece())
			message = QString("%1 (ID: %2)").arg(focus->GetName(), ((lcPiece*)focus)->mPieceInfo->m_strName);
		else
			message = focus->GetName();
	}
	else if (selectedCount > 0)
	{
		if (selectedCount == 1)
			message = "1 Object selected";
		else
			message = QString("%1 Objects selected").arg(QString::number(selectedCount));

		if ((focus != NULL) && focus->IsPiece())
		{
			message.append(QString(" - %1 (ID: %2)").arg(focus->GetName(), ((lcPiece*)focus)->mPieceInfo->m_strName));

			const lcGroup* pGroup = ((lcPiece*)focus)->GetGroup();
			if ((pGroup != NULL) && pGroup->m_strName[0])
				message.append(QString(" in group '%1'").arg(pGroup->m_strName));
		}
	}

	statusBarLabel->setText(message);
}

void lcQMainWindow::updateAction(int newAction)
{
	QAction *action = actions[LC_EDIT_ACTION_FIRST + newAction];

	if (action)
		action->setChecked(true);
}

void lcQMainWindow::updatePaste(bool enabled)
{
	QAction *action = actions[LC_EDIT_PASTE];

	if (action)
		action->setEnabled(enabled);
}

void lcQMainWindow::updateCurrentStep()
{
	Project *project = lcGetActiveProject();
	lcStep currentStep = project->GetCurrentStep();
	lcStep lastStep = project->GetLastStep();

	actions[LC_VIEW_TIME_FIRST]->setEnabled(currentStep != 1);
	actions[LC_VIEW_TIME_PREVIOUS]->setEnabled(currentStep > 1);
	actions[LC_VIEW_TIME_NEXT]->setEnabled(currentStep < LC_STEP_MAX);
	actions[LC_VIEW_TIME_LAST]->setEnabled(currentStep != lastStep);

	statusTimeLabel->setText(QString(tr("Step %1")).arg(QString::number(currentStep)));
}

void lcQMainWindow::setAddKeys(bool addKeys)
{
	actions[LC_VIEW_TIME_ADD_KEYS]->setChecked(addKeys);
}

void lcQMainWindow::updateLockSnap()
{
	const lcPreferences& Preferences = lcGetPreferences();

	actions[LC_EDIT_SNAP_RELATIVE]->setChecked(!Preferences.mForceGlobalTransforms);
	actions[LC_EDIT_LOCK_X]->setChecked(gMainWindow->GetLockX());
	actions[LC_EDIT_LOCK_Y]->setChecked(gMainWindow->GetLockY());
	actions[LC_EDIT_LOCK_Z]->setChecked(gMainWindow->GetLockZ());
}

void lcQMainWindow::updateSnap()
{
	actions[LC_EDIT_SNAP_MOVE_XY0 + gMainWindow->GetMoveXYSnapIndex()]->setChecked(true);
	actions[LC_EDIT_SNAP_MOVE_Z0 + gMainWindow->GetMoveZSnapIndex()]->setChecked(true);
	actions[LC_EDIT_SNAP_ANGLE0 + gMainWindow->GetAngleSnapIndex()]->setChecked(true);

	statusSnapLabel->setText(QString(tr(" M: %1 %2 R: %3 ")).arg(gMainWindow->GetMoveXYSnapText(), gMainWindow->GetMoveZSnapText(), QString::number(gMainWindow->GetAngleSnap())));
}

void lcQMainWindow::updateUndoRedo(const QString& UndoText, const QString& RedoText)
{
	QAction* UndoAction = actions[LC_EDIT_UNDO];
	QAction* RedoAction = actions[LC_EDIT_REDO];

	if (!UndoText.isEmpty())
	{
		UndoAction->setEnabled(true);
		UndoAction->setText(QString(tr("&Undo %1")).arg(UndoText));
	}
	else
	{
		UndoAction->setEnabled(false);
		UndoAction->setText(tr("&Undo"));
	}

	if (!RedoText.isEmpty())
	{
		RedoAction->setEnabled(true);
		RedoAction->setText(QString(tr("&Redo %1")).arg(RedoText));
	}
	else
	{
		RedoAction->setEnabled(false);
		RedoAction->setText(tr("&Redo"));
	}
}

void lcQMainWindow::updateTransformType(int newType)
{
	const char* iconNames[] =
	{
		":/resources/edit_transform_absolute_translation.png",
		":/resources/edit_transform_relative_translation.png",
		":/resources/edit_transform_absolute_rotation.png",
		":/resources/edit_transform_relative_rotation.png"
	};

	LC_ASSERT(newType >= 0 && newType <= 3);
	actions[LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION + newType]->setChecked(true);
	actions[LC_EDIT_TRANSFORM]->setIcon(QIcon(iconNames[newType]));
}

void lcQMainWindow::updateCameraMenu()
{
	const lcArray<lcCamera*>& cameras = lcGetActiveProject()->GetCameras();
	lcCamera* currentCamera = gMainWindow->GetActiveView()->mCamera;
	int actionIdx, currentIndex = -1;

	for (actionIdx = LC_VIEW_CAMERA_FIRST; actionIdx <= LC_VIEW_CAMERA_LAST; actionIdx++)
	{
		QAction* action = actions[actionIdx];
		int cameraIdx = actionIdx - LC_VIEW_CAMERA_FIRST;

		if (cameraIdx < cameras.GetSize())
		{
			if (currentCamera == cameras[cameraIdx])
				currentIndex = cameraIdx;

			action->setText(cameras[cameraIdx]->GetName());
			action->setVisible(true);
		}
		else
			action->setVisible(false);
	}

	updateCurrentCamera(currentIndex);
}

void lcQMainWindow::updateCurrentCamera(int cameraIndex)
{
	int actionIndex = LC_VIEW_CAMERA_FIRST + cameraIndex;

	if (actionIndex < LC_VIEW_CAMERA_FIRST || actionIndex > LC_VIEW_CAMERA_LAST)
		actionIndex = LC_VIEW_CAMERA_NONE;

	actions[actionIndex]->setChecked(true);
}

void lcQMainWindow::updatePerspective(View* view)
{
	if (view->mCamera->IsOrtho())
		actions[LC_VIEW_PROJECTION_ORTHO]->setChecked(true);
	else
		actions[LC_VIEW_PROJECTION_PERSPECTIVE]->setChecked(true);
}

void lcQMainWindow::updateCategories()
{
	partsTree->updateCategories();
}

void lcQMainWindow::updateTitle(const char* title, bool modified)
{
	setWindowModified(modified);
	setWindowFilePath(title);
}

void lcQMainWindow::updateModified(bool modified)
{
	setWindowModified(modified);
}

void lcQMainWindow::updateRecentFiles(const char** fileNames)
{
	for (int actionIdx = LC_FILE_RECENT_FIRST; actionIdx <= LC_FILE_RECENT_LAST; actionIdx++)
	{
		int fileIdx = actionIdx - LC_FILE_RECENT_FIRST;
		QAction *action = actions[actionIdx];

		if (fileNames[fileIdx][0])
		{
			action->setText(QString("&%1 %2").arg(QString::number(fileIdx + 1), QDir::toNativeSeparators(fileNames[fileIdx])));
			action->setVisible(true);
		}
		else
			action->setVisible(false);
	}

	actionFileRecentSeparator->setVisible(fileNames[0][0] != 0);
}

void lcQMainWindow::updateShortcuts()
{
	for (int actionIdx = 0; actionIdx < LC_NUM_COMMANDS; actionIdx++)
		actions[actionIdx]->setShortcut(QKeySequence(gKeyboardShortcuts.Shortcuts[actionIdx]));
}

lcVector3 lcQMainWindow::getTransformAmount()
{
	lcVector3 transform;

	transform.x = transformX->text().toFloat();
	transform.y = transformY->text().toFloat();
	transform.z = transformZ->text().toFloat();

	return transform;
}

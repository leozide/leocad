#include "lc_global.h"
#include "lc_mainwindow.h"
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include "lc_qglwidget.h"
#include "lc_qpartstree.h"
#include "lc_qcolorlist.h"
#include "lc_qpropertiestree.h"
#include "lc_qutils.h"
#include "lc_profile.h"
#include "preview.h"
#include "view.h"
#include "project.h"
#include "piece.h"
#include "group.h"
#include "pieceinf.h"
#include "lc_library.h"
#include "lc_colors.h"

lcMainWindow* gMainWindow;

lcMainWindow::lcMainWindow()
{
	memset(mActions, 0, sizeof(mActions));

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
	mRelativeTransform = true;

	memset(&mSearchOptions, 0, sizeof(mSearchOptions));

	for (int FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		mRecentFiles[FileIdx] = lcGetProfileString((LC_PROFILE_KEY)(LC_PROFILE_RECENT_FILE1 + FileIdx));

	gMainWindow = this;
}

lcMainWindow::~lcMainWindow()
{
	delete mPreviewWidget;
	mPreviewWidget = NULL;

	for (int FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		lcSetProfileString((LC_PROFILE_KEY)(LC_PROFILE_RECENT_FILE1 + FileIdx), mRecentFiles[FileIdx]);

	gMainWindow = NULL;
}

void lcMainWindow::CreateWidgets()
{
	setWindowIcon(QIcon(":/resources/icon64.png"));
	setWindowFilePath(QString());

	CreateActions();
	CreateToolBars();
	CreateMenus();
	CreateStatusBar();

	QFrame* CentralFrame = new QFrame;
	CentralFrame->setFrameShape(QFrame::StyledPanel);
	CentralFrame->setFrameShadow(QFrame::Sunken);
	setCentralWidget(CentralFrame);

	QGridLayout* CentralLayout = new QGridLayout(CentralFrame);
	CentralLayout->setContentsMargins(0, 0, 0, 0);

	QWidget* ViewWidget = new lcQGLWidget(CentralFrame, mPiecePreviewWidget, new View(NULL), true);
	CentralLayout->addWidget(ViewWidget, 0, 0, 1, 1);
	ViewWidget->setFocus();

	connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(ClipboardChanged()));
	ClipboardChanged();

	PiecePreview* Preview = (PiecePreview*)mPiecePreviewWidget->widget;
	mPreviewWidget = Preview;

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	PieceInfo* Info = Library->FindPiece("3005", false);

	if (!Info)
		Info = Library->mPieces[0];

	if (Info)
		mPreviewWidget->SetCurrentPiece(Info);

	QSettings Settings;
	Settings.beginGroup("MainWindow");
	resize(QSize(800, 600));
	move(QPoint(200, 200));
	restoreGeometry(Settings.value("Geometry").toByteArray());
	restoreState(Settings.value("State").toByteArray());
	Settings.endGroup();
}

void lcMainWindow::CreateActions()
{
	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
	{
		QAction* Action = new QAction(tr(gCommands[CommandIdx].MenuName), this);
		Action->setStatusTip(tr(gCommands[CommandIdx].StatusText));
		connect(Action, SIGNAL(triggered()), this, SLOT(ActionTriggered()));
		addAction(Action);
		mActions[CommandIdx] = Action;
	}

	mActions[LC_FILE_NEW]->setToolTip(tr("New Project"));
	mActions[LC_FILE_OPEN]->setToolTip(tr("Open Project"));
	mActions[LC_FILE_SAVE]->setToolTip(tr("Save Project"));

	mActions[LC_FILE_NEW]->setIcon(QIcon(":/resources/file_new.png"));
	mActions[LC_FILE_OPEN]->setIcon(QIcon(":/resources/file_open.png"));
	mActions[LC_FILE_SAVE]->setIcon(QIcon(":/resources/file_save.png"));
	mActions[LC_FILE_SAVE_IMAGE]->setIcon(QIcon(":/resources/file_picture.png"));
	mActions[LC_FILE_PRINT]->setIcon(QIcon(":/resources/file_print.png"));
	mActions[LC_FILE_PRINT_PREVIEW]->setIcon(QIcon(":/resources/file_print_preview.png"));
	mActions[LC_EDIT_UNDO]->setIcon(QIcon(":/resources/edit_undo.png"));
	mActions[LC_EDIT_REDO]->setIcon(QIcon(":/resources/edit_redo.png"));
	mActions[LC_EDIT_CUT]->setIcon(QIcon(":/resources/edit_cut.png"));
	mActions[LC_EDIT_COPY]->setIcon(QIcon(":/resources/edit_copy.png"));
	mActions[LC_EDIT_PASTE]->setIcon(QIcon(":/resources/edit_paste.png"));
	mActions[LC_EDIT_ACTION_INSERT]->setIcon(QIcon(":/resources/action_insert.png"));
	mActions[LC_EDIT_ACTION_LIGHT]->setIcon(QIcon(":/resources/action_light.png"));
	mActions[LC_EDIT_ACTION_SPOTLIGHT]->setIcon(QIcon(":/resources/action_spotlight.png"));
	mActions[LC_EDIT_ACTION_CAMERA]->setIcon(QIcon(":/resources/action_camera.png"));
	mActions[LC_EDIT_ACTION_SELECT]->setIcon(QIcon(":/resources/action_select.png"));
	mActions[LC_EDIT_ACTION_MOVE]->setIcon(QIcon(":/resources/action_move.png"));
	mActions[LC_EDIT_ACTION_ROTATE]->setIcon(QIcon(":/resources/action_rotate.png"));
	mActions[LC_EDIT_ACTION_DELETE]->setIcon(QIcon(":/resources/action_delete.png"));
	mActions[LC_EDIT_ACTION_PAINT]->setIcon(QIcon(":/resources/action_paint.png"));
	mActions[LC_EDIT_ACTION_ZOOM]->setIcon(QIcon(":/resources/action_zoom.png"));
	mActions[LC_EDIT_ACTION_PAN]->setIcon(QIcon(":/resources/action_pan.png"));
	mActions[LC_EDIT_ACTION_ROTATE_VIEW]->setIcon(QIcon(":/resources/action_rotate_view.png"));
	mActions[LC_EDIT_ACTION_ROLL]->setIcon(QIcon(":/resources/action_roll.png"));
	mActions[LC_EDIT_ACTION_ZOOM_REGION]->setIcon(QIcon(":/resources/action_zoom_region.png"));
	mActions[LC_EDIT_TRANSFORM_RELATIVE]->setIcon(QIcon(":/resources/edit_transform_relative.png"));
	mActions[LC_PIECE_SHOW_EARLIER]->setIcon(QIcon(":/resources/piece_show_earlier.png"));
	mActions[LC_PIECE_SHOW_LATER]->setIcon(QIcon(":/resources/piece_show_later.png"));
	mActions[LC_VIEW_SPLIT_HORIZONTAL]->setIcon(QIcon(":/resources/view_split_horizontal.png"));
	mActions[LC_VIEW_SPLIT_VERTICAL]->setIcon(QIcon(":/resources/view_split_vertical.png"));
	mActions[LC_VIEW_ZOOM_IN]->setIcon(QIcon(":/resources/view_zoomin.png"));
	mActions[LC_VIEW_ZOOM_OUT]->setIcon(QIcon(":/resources/view_zoomout.png"));
	mActions[LC_VIEW_ZOOM_EXTENTS]->setIcon(QIcon(":/resources/view_zoomextents.png"));
	mActions[LC_VIEW_TIME_FIRST]->setIcon(QIcon(":/resources/time_first.png"));
	mActions[LC_VIEW_TIME_PREVIOUS]->setIcon(QIcon(":/resources/time_previous.png"));
	mActions[LC_VIEW_TIME_NEXT]->setIcon(QIcon(":/resources/time_next.png"));
	mActions[LC_VIEW_TIME_LAST]->setIcon(QIcon(":/resources/time_last.png"));
	mActions[LC_VIEW_TIME_ADD_KEYS]->setIcon(QIcon(":/resources/time_add_keys.png"));
	mActions[LC_HELP_HOMEPAGE]->setIcon(QIcon(":/resources/help_homepage.png"));
	mActions[LC_HELP_EMAIL]->setIcon(QIcon(":/resources/help_email.png"));

	mActions[LC_EDIT_LOCK_X]->setCheckable(true);
	mActions[LC_EDIT_LOCK_Y]->setCheckable(true);
	mActions[LC_EDIT_LOCK_Z]->setCheckable(true);
	mActions[LC_EDIT_TRANSFORM_RELATIVE]->setCheckable(true);
	mActions[LC_VIEW_CAMERA_NONE]->setCheckable(true);
	mActions[LC_VIEW_TIME_ADD_KEYS]->setCheckable(true);

	QActionGroup* ActionSnapXYGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_SNAP_MOVE_XY0; ActionIdx <= LC_EDIT_SNAP_MOVE_XY9; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionSnapXYGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionSnapZGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_SNAP_MOVE_Z0; ActionIdx <= LC_EDIT_SNAP_MOVE_Z9; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionSnapZGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionSnapAngleGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_SNAP_ANGLE0; ActionIdx <= LC_EDIT_SNAP_ANGLE9; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionSnapAngleGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionTransformTypeGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION; ActionIdx <= LC_EDIT_TRANSFORM_RELATIVE_ROTATION; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionTransformTypeGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionToolGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_ACTION_FIRST; ActionIdx <= LC_EDIT_ACTION_LAST; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionToolGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionCameraGroup = new QActionGroup(this);
	ActionCameraGroup->addAction(mActions[LC_VIEW_CAMERA_NONE]);
	for (int ActionIdx = LC_VIEW_CAMERA_FIRST; ActionIdx <= LC_VIEW_CAMERA_LAST; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionCameraGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionPerspectiveGroup = new QActionGroup(this);
	for (int ActionIdx = LC_VIEW_PROJECTION_FIRST; ActionIdx <= LC_VIEW_PROJECTION_LAST; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionPerspectiveGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ModelGroup = new QActionGroup(this);
	for (int ActionIdx = LC_MODEL_FIRST; ActionIdx <= LC_MODEL_LAST; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ModelGroup->addAction(mActions[ActionIdx]);
	}

	UpdateShortcuts();
}

void lcMainWindow::CreateMenus()
{
	QMenu* TransformMenu = new QMenu(tr("Transform"), this);
	TransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION]);
	TransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION]);
	TransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_RELATIVE_ROTATION]);
	TransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION]);
	mActions[LC_EDIT_TRANSFORM]->setMenu(TransformMenu);

	QMenu* CameraMenu = new QMenu(tr("C&ameras"), this);
	CameraMenu->addAction(mActions[LC_VIEW_CAMERA_NONE]);

	for (int actionIdx = LC_VIEW_CAMERA_FIRST; actionIdx <= LC_VIEW_CAMERA_LAST; actionIdx++)
		CameraMenu->addAction(mActions[actionIdx]);

	CameraMenu->addSeparator();
	CameraMenu->addAction(mActions[LC_VIEW_CAMERA_RESET]);

	QMenu* FileMenu = menuBar()->addMenu(tr("&File"));
	FileMenu->addAction(mActions[LC_FILE_NEW]);
	FileMenu->addAction(mActions[LC_FILE_OPEN]);
	FileMenu->addAction(mActions[LC_FILE_MERGE]);
	FileMenu->addSeparator();
	FileMenu->addAction(mActions[LC_FILE_SAVE]);
	FileMenu->addAction(mActions[LC_FILE_SAVEAS]);
	FileMenu->addAction(mActions[LC_FILE_SAVE_IMAGE]);
	QMenu* ExportMenu = FileMenu->addMenu(tr("&Export"));
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_3DS]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_BRICKLINK]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_CSV]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_HTML]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_POVRAY]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_WAVEFRONT]);
	FileMenu->addSeparator();
	FileMenu->addAction(mActions[LC_FILE_PRINT]);
	FileMenu->addAction(mActions[LC_FILE_PRINT_PREVIEW]);
//	FileMenu->addAction(mActions[LC_FILE_PRINT_BOM]);
	FileMenu->addSeparator();
	FileMenu->addAction(mActions[LC_FILE_RECENT1]);
	FileMenu->addAction(mActions[LC_FILE_RECENT2]);
	FileMenu->addAction(mActions[LC_FILE_RECENT3]);
	FileMenu->addAction(mActions[LC_FILE_RECENT4]);
	mActionFileRecentSeparator = FileMenu->addSeparator();
	FileMenu->addAction(mActions[LC_FILE_EXIT]);

	QMenu* EditMenu = menuBar()->addMenu(tr("&Edit"));
	EditMenu->addAction(mActions[LC_EDIT_UNDO]);
	EditMenu->addAction(mActions[LC_EDIT_REDO]);
	EditMenu->addSeparator();
	EditMenu->addAction(mActions[LC_EDIT_CUT]);
	EditMenu->addAction(mActions[LC_EDIT_COPY]);
	EditMenu->addAction(mActions[LC_EDIT_PASTE]);
	EditMenu->addSeparator();
	EditMenu->addAction(mActions[LC_EDIT_FIND]);
	EditMenu->addAction(mActions[LC_EDIT_FIND_NEXT]);
	EditMenu->addAction(mActions[LC_EDIT_FIND_PREVIOUS]);
	EditMenu->addSeparator();
	EditMenu->addAction(mActions[LC_EDIT_SELECT_ALL]);
	EditMenu->addAction(mActions[LC_EDIT_SELECT_NONE]);
	EditMenu->addAction(mActions[LC_EDIT_SELECT_INVERT]);
	EditMenu->addAction(mActions[LC_EDIT_SELECT_BY_NAME]);

	QMenu* ViewMenu = menuBar()->addMenu(tr("&View"));
	ViewMenu->addAction(mActions[LC_VIEW_PREFERENCES]);
	ViewMenu->addSeparator();
	ViewMenu->addAction(mActions[LC_VIEW_ZOOM_EXTENTS]);
	ViewMenu->addAction(mActions[LC_VIEW_LOOK_AT]);
	QMenu* ViewpointsMenu = ViewMenu->addMenu(tr("&Viewpoints"));
	ViewpointsMenu->addAction(mActions[LC_VIEW_VIEWPOINT_FRONT]);
	ViewpointsMenu->addAction(mActions[LC_VIEW_VIEWPOINT_BACK]);
	ViewpointsMenu->addAction(mActions[LC_VIEW_VIEWPOINT_LEFT]);
	ViewpointsMenu->addAction(mActions[LC_VIEW_VIEWPOINT_RIGHT]);
	ViewpointsMenu->addAction(mActions[LC_VIEW_VIEWPOINT_TOP]);
	ViewpointsMenu->addAction(mActions[LC_VIEW_VIEWPOINT_BOTTOM]);
	ViewpointsMenu->addAction(mActions[LC_VIEW_VIEWPOINT_HOME]);
	ViewMenu->addMenu(CameraMenu);
	QMenu* PerspectiveMenu = ViewMenu->addMenu(tr("Projection"));
	PerspectiveMenu->addAction(mActions[LC_VIEW_PROJECTION_PERSPECTIVE]);
	PerspectiveMenu->addAction(mActions[LC_VIEW_PROJECTION_ORTHO]);
	QMenu* StepMenu = ViewMenu->addMenu(tr("Ste&p"));
	StepMenu->addAction(mActions[LC_VIEW_TIME_FIRST]);
	StepMenu->addAction(mActions[LC_VIEW_TIME_PREVIOUS]);
	StepMenu->addAction(mActions[LC_VIEW_TIME_NEXT]);
	StepMenu->addAction(mActions[LC_VIEW_TIME_LAST]);
	StepMenu->addSeparator();
	StepMenu->addAction(mActions[LC_VIEW_TIME_INSERT]);
	StepMenu->addAction(mActions[LC_VIEW_TIME_DELETE]);
	ViewMenu->addSeparator();
	ViewMenu->addAction(mActions[LC_VIEW_SPLIT_HORIZONTAL]);
	ViewMenu->addAction(mActions[LC_VIEW_SPLIT_VERTICAL]);
	ViewMenu->addAction(mActions[LC_VIEW_REMOVE_VIEW]);
	ViewMenu->addAction(mActions[LC_VIEW_RESET_VIEWS]);
	ViewMenu->addSeparator();
	QMenu* ToolBarsMenu = ViewMenu->addMenu(tr("T&oolbars"));
	ToolBarsMenu->addAction(mPartsToolBar->toggleViewAction());
	ToolBarsMenu->addAction(mPropertiesToolBar->toggleViewAction());
	ToolBarsMenu->addSeparator();
	ToolBarsMenu->addAction(mStandardToolBar->toggleViewAction());
	ToolBarsMenu->addAction(mToolsToolBar->toggleViewAction());
	ToolBarsMenu->addAction(mTimeToolBar->toggleViewAction());
	ViewMenu->addAction(mActions[LC_VIEW_FULLSCREEN]);

	QMenu* PieceMenu = menuBar()->addMenu(tr("&Piece"));
	PieceMenu->addAction(mActions[LC_PIECE_INSERT]);
	PieceMenu->addAction(mActions[LC_PIECE_DELETE]);
	PieceMenu->addAction(mActions[LC_PIECE_ARRAY]);
	PieceMenu->addAction(mActions[LC_PIECE_MINIFIG_WIZARD]);
	PieceMenu->addSeparator();
	PieceMenu->addAction(mActions[LC_PIECE_GROUP]);
	PieceMenu->addAction(mActions[LC_PIECE_UNGROUP]);
	PieceMenu->addAction(mActions[LC_PIECE_GROUP_REMOVE]);
	PieceMenu->addAction(mActions[LC_PIECE_GROUP_ADD]);
	PieceMenu->addAction(mActions[LC_PIECE_GROUP_EDIT]);
//	LC_PIECE_SHOW_EARLIER,
//	LC_PIECE_SHOW_LATER,
	PieceMenu->addSeparator();
	PieceMenu->addAction(mActions[LC_PIECE_HIDE_SELECTED]);
	PieceMenu->addAction(mActions[LC_PIECE_HIDE_UNSELECTED]);
	PieceMenu->addAction(mActions[LC_PIECE_UNHIDE_ALL]);

	QMenu* ModelMenu = menuBar()->addMenu(tr("&Model"));
	ModelMenu->addAction(mActions[LC_MODEL_PROPERTIES]);
	ModelMenu->addAction(mActions[LC_MODEL_NEW]);
	ModelMenu->addSeparator();
	for (int ModelIdx = 0; ModelIdx < LC_MODEL_LAST - LC_MODEL_FIRST; ModelIdx++)
		ModelMenu->addAction(mActions[LC_MODEL_FIRST + ModelIdx]);
	ModelMenu->addAction(mActions[LC_MODEL_LIST]);

	QMenu* HelpMenu = menuBar()->addMenu(tr("&Help"));
	HelpMenu->addAction(mActions[LC_HELP_HOMEPAGE]);
	HelpMenu->addAction(mActions[LC_HELP_EMAIL]);
#if !LC_DISABLE_UPDATE_CHECK
	HelpMenu->addAction(mActions[LC_HELP_UPDATES]);
#endif
	HelpMenu->addSeparator();
	HelpMenu->addAction(mActions[LC_HELP_ABOUT]);
}

void lcMainWindow::CreateToolBars()
{
	QMenu* LockMenu = new QMenu(tr("Lock Menu"), this);
	LockMenu->addAction(mActions[LC_EDIT_LOCK_X]);
	LockMenu->addAction(mActions[LC_EDIT_LOCK_Y]);
	LockMenu->addAction(mActions[LC_EDIT_LOCK_Z]);
	LockMenu->addAction(mActions[LC_EDIT_LOCK_NONE]);

	QAction* LockAction = new QAction(tr("Lock Menu"), this);
	LockAction->setStatusTip(tr("Toggle mouse movement on specific axes"));
	LockAction->setIcon(QIcon(":/resources/edit_lock.png"));
	LockAction->setMenu(LockMenu);

	QMenu* SnapXYMenu = new QMenu(tr("Snap XY"), this);
	for (int actionIdx = LC_EDIT_SNAP_MOVE_XY0; actionIdx <= LC_EDIT_SNAP_MOVE_XY9; actionIdx++)
		SnapXYMenu->addAction(mActions[actionIdx]);

	QMenu* SnapZMenu = new QMenu(tr("Snap Z"), this);
	for (int actionIdx = LC_EDIT_SNAP_MOVE_Z0; actionIdx <= LC_EDIT_SNAP_MOVE_Z9; actionIdx++)
		SnapZMenu->addAction(mActions[actionIdx]);

	QMenu* SnapMenu = new QMenu(tr("Snap Menu"), this);
	SnapMenu->addMenu(SnapXYMenu);
	SnapMenu->addMenu(SnapZMenu);

	QAction* MoveAction = new QAction(tr("Snap Move"), this);
	MoveAction->setStatusTip(tr("Snap translations to fixed intervals"));
	MoveAction->setIcon(QIcon(":/resources/edit_snap_move.png"));
	MoveAction->setMenu(SnapMenu);

	QMenu* SnapAngleMenu = new QMenu(tr("Snap Angle Menu"), this);
	for (int actionIdx = LC_EDIT_SNAP_ANGLE0; actionIdx <= LC_EDIT_SNAP_ANGLE9; actionIdx++)
		SnapAngleMenu->addAction(mActions[actionIdx]);

	QAction* AngleAction = new QAction(tr("Snap Rotate"), this);
	AngleAction->setStatusTip(tr("Snap rotations to fixed intervals"));
	AngleAction->setIcon(QIcon(":/resources/edit_snap_angle.png"));
	AngleAction->setMenu(SnapAngleMenu);

	mStandardToolBar = addToolBar(tr("Standard"));
	mStandardToolBar->setObjectName("StandardToolbar");
	mStandardToolBar->addAction(mActions[LC_FILE_NEW]);
	mStandardToolBar->addAction(mActions[LC_FILE_OPEN]);
	mStandardToolBar->addAction(mActions[LC_FILE_SAVE]);
	mStandardToolBar->addAction(mActions[LC_FILE_PRINT]);
	mStandardToolBar->addAction(mActions[LC_FILE_PRINT_PREVIEW]);
	mStandardToolBar->addSeparator();
	mStandardToolBar->addAction(mActions[LC_EDIT_UNDO]);
	mStandardToolBar->addAction(mActions[LC_EDIT_REDO]);
	mStandardToolBar->addAction(mActions[LC_EDIT_CUT]);
	mStandardToolBar->addAction(mActions[LC_EDIT_COPY]);
	mStandardToolBar->addAction(mActions[LC_EDIT_PASTE]);
	mStandardToolBar->addSeparator();
	mStandardToolBar->addAction(mActions[LC_EDIT_TRANSFORM_RELATIVE]);
	mStandardToolBar->addAction(LockAction);
	mStandardToolBar->addAction(MoveAction);
	mStandardToolBar->addAction(AngleAction);
	mStandardToolBar->addSeparator();
	mStandardToolBar->addAction(mActions[LC_EDIT_TRANSFORM]);
	((QToolButton*)mStandardToolBar->widgetForAction(LockAction))->setPopupMode(QToolButton::InstantPopup);
	((QToolButton*)mStandardToolBar->widgetForAction(MoveAction))->setPopupMode(QToolButton::InstantPopup);
	((QToolButton*)mStandardToolBar->widgetForAction(AngleAction))->setPopupMode(QToolButton::InstantPopup);
	((QToolButton*)mStandardToolBar->widgetForAction(mActions[LC_EDIT_TRANSFORM]))->setPopupMode(QToolButton::InstantPopup);

	QHBoxLayout* TransformLayout = new QHBoxLayout;
	QWidget* TransformWidget = new QWidget();
	TransformWidget->setLayout(TransformLayout);
	mTransformXEdit = new QLineEdit();
	mTransformXEdit->setMaximumWidth(75);
	TransformLayout->addWidget(mTransformXEdit);
	mTransformYEdit = new QLineEdit();
	mTransformYEdit->setMaximumWidth(75);
	TransformLayout->addWidget(mTransformYEdit);
	mTransformZEdit = new QLineEdit();
	mTransformZEdit->setMaximumWidth(75);
	TransformLayout->addWidget(mTransformZEdit);
	TransformLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
	mStandardToolBar->addWidget(TransformWidget);
	connect(mTransformXEdit, SIGNAL(returnPressed()), mActions[LC_EDIT_TRANSFORM], SIGNAL(triggered()));
	connect(mTransformYEdit, SIGNAL(returnPressed()), mActions[LC_EDIT_TRANSFORM], SIGNAL(triggered()));
	connect(mTransformZEdit, SIGNAL(returnPressed()), mActions[LC_EDIT_TRANSFORM], SIGNAL(triggered()));

	mToolsToolBar = addToolBar(tr("Tools"));
	mToolsToolBar->setObjectName("ToolsToolbar");
	insertToolBarBreak(mToolsToolBar);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_INSERT]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_LIGHT]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_SPOTLIGHT]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_CAMERA]);
	mToolsToolBar->addSeparator();
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_SELECT]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_MOVE]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_ROTATE]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_DELETE]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_PAINT]);
	mToolsToolBar->addSeparator();
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_ZOOM]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_PAN]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_ROTATE_VIEW]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_ROLL]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_ZOOM_REGION]);

	mTimeToolBar = addToolBar(tr("Time"));
	mTimeToolBar->setObjectName("TimeToolbar");
	mTimeToolBar->addAction(mActions[LC_VIEW_TIME_FIRST]);
	mTimeToolBar->addAction(mActions[LC_VIEW_TIME_PREVIOUS]);
	mTimeToolBar->addAction(mActions[LC_VIEW_TIME_NEXT]);
	mTimeToolBar->addAction(mActions[LC_VIEW_TIME_LAST]);
	mTimeToolBar->addAction(mActions[LC_PIECE_SHOW_EARLIER]);
	mTimeToolBar->addAction(mActions[LC_PIECE_SHOW_LATER]);
	mTimeToolBar->addAction(mActions[LC_VIEW_TIME_ADD_KEYS]);
	// TODO: add missing menu items

	mPartsToolBar = new QDockWidget(tr("Parts"), this);
	mPartsToolBar->setObjectName("PartsToolbar");
	mPartsToolBar->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	QWidget* PartsContents = new QWidget();
	QGridLayout* PartsLayout = new QGridLayout(PartsContents);
	PartsLayout->setSpacing(6);
	PartsLayout->setContentsMargins(6, 6, 6, 6);
	QSplitter* PartsSplitter = new QSplitter(Qt::Vertical, PartsContents);

	QFrame* PreviewFrame = new QFrame(PartsSplitter);
	PreviewFrame->setFrameShape(QFrame::StyledPanel);
	PreviewFrame->setFrameShadow(QFrame::Sunken);

	QGridLayout* PreviewLayout = new QGridLayout(PreviewFrame);
	PreviewLayout->setContentsMargins(0, 0, 0, 0);

	int AASamples = lcGetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES);
	if (AASamples > 1)
	{
		QGLFormat format;
		format.setSampleBuffers(true);
		format.setSamples(AASamples);
		QGLFormat::setDefaultFormat(format);
	}

	mPiecePreviewWidget = new lcQGLWidget(PreviewFrame, NULL, new PiecePreview(), false);
	mPiecePreviewWidget->preferredSize = QSize(200, 100);
	PreviewLayout->addWidget(mPiecePreviewWidget, 0, 0, 1, 1);

	QSizePolicy treePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	treePolicy.setVerticalStretch(1);

	mPartsTree = new lcQPartsTree(PartsSplitter);
	mPartsTree->setSizePolicy(treePolicy);
	connect(mPartsTree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(PartsTreeItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));

	mPartSearchEdit = new QLineEdit(PartsSplitter);
	connect(mPartSearchEdit, SIGNAL(returnPressed()), this, SLOT(PartSearchReturn()));
	connect(mPartSearchEdit, SIGNAL(textChanged(QString)), this, SLOT(PartSearchChanged(QString)));

	QCompleter* Completer = new QCompleter(this);
	Completer->setModel(new lcQPartsListModel(Completer));
	Completer->setCaseSensitivity(Qt::CaseInsensitive);
	mPartSearchEdit->setCompleter(Completer);

	QFrame* ColorFrame = new QFrame(PartsSplitter);
	ColorFrame->setFrameShape(QFrame::StyledPanel);
	ColorFrame->setFrameShadow(QFrame::Sunken);

	QGridLayout* ColorLayout = new QGridLayout(ColorFrame);
	ColorLayout->setContentsMargins(0, 0, 0, 0);

	mColorList = new lcQColorList(PartsSplitter);
	ColorLayout->addWidget(mColorList);
	connect(mColorList, SIGNAL(colorChanged(int)), this, SLOT(ColorChanged(int)));

	PartsLayout->addWidget(PartsSplitter, 0, 0, 1, 1);

	mPartsToolBar->setWidget(PartsContents);
	addDockWidget(Qt::RightDockWidgetArea, mPartsToolBar);

	mPropertiesToolBar = new QDockWidget(tr("Properties"), this);
	mPropertiesToolBar->setObjectName("PropertiesToolbar");
	mPropertiesToolBar->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

	mPropertiesWidget = new lcQPropertiesTree(mPropertiesToolBar);

	mPropertiesToolBar->setWidget(mPropertiesWidget);
	addDockWidget(Qt::RightDockWidgetArea, mPropertiesToolBar);

	tabifyDockWidget(mPartsToolBar, mPropertiesToolBar);
	mPartsToolBar->raise();
}

void lcMainWindow::CreateStatusBar()
{
	QStatusBar* StatusBar = new QStatusBar(this);
	setStatusBar(StatusBar);

	mStatusBarLabel = new QLabel();
	StatusBar->addWidget(mStatusBarLabel);

	mStatusPositionLabel = new QLabel();
	StatusBar->addPermanentWidget(mStatusPositionLabel);

	mStatusSnapLabel = new QLabel();
	StatusBar->addPermanentWidget(mStatusSnapLabel);

	mStatusTimeLabel = new QLabel();
	StatusBar->addPermanentWidget(mStatusTimeLabel);
}

void lcMainWindow::closeEvent(QCloseEvent *event)
{
	if (SaveProjectIfModified())
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

QMenu* lcMainWindow::createPopupMenu()
{
	QMenu* Menu = new QMenu(this);

	Menu->addAction(mPartsToolBar->toggleViewAction());
	Menu->addAction(mPropertiesToolBar->toggleViewAction());
	Menu->addSeparator();
	Menu->addAction(mStandardToolBar->toggleViewAction());
	Menu->addAction(mToolsToolBar->toggleViewAction());
	Menu->addAction(mTimeToolBar->toggleViewAction());

	return Menu;
}

void lcMainWindow::ClipboardChanged()
{
	const QString MimeType = QLatin1String("application/vnd.leocad-clipboard");
	const QMimeData* MimeData = QApplication::clipboard()->mimeData();
	QByteArray ClipboardData;

	if (MimeData->hasFormat(MimeType))
		ClipboardData = MimeData->data(MimeType);

	g_App->SetClipboard(ClipboardData);
}

void lcMainWindow::ActionTriggered()
{
	QObject* Action = sender();

	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
	{
		if (Action == mActions[CommandIdx])
		{
			HandleCommand((lcCommandId)CommandIdx);
			break;
		}
	}
}

void lcMainWindow::PartsTreeItemChanged(QTreeWidgetItem* Current, QTreeWidgetItem* Previous)
{
	if (!Current)
		return;

	PieceInfo* Info = (PieceInfo*)Current->data(0, lcQPartsTree::PieceInfoRole).value<void*>();

	if (Info)
		mPreviewWidget->SetCurrentPiece(Info);
}

void lcMainWindow::ColorChanged(int ColorIndex)
{
	SetColorIndex(ColorIndex);
}

void lcMainWindow::PartSearchReturn()
{
	mPartsTree->searchParts(mPartSearchEdit->text());
}

void lcMainWindow::PartSearchChanged(const QString& Text)
{
	const QByteArray TextConv = Text.toLocal8Bit();
	const char* SearchString = TextConv.data();
	int Length = strlen(SearchString);

	if (!Length)
		return;

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	PieceInfo* BestMatch = NULL;

	for (int PartIdx = 0; PartIdx < Library->mPieces.GetSize(); PartIdx++)
	{
		PieceInfo* Info = Library->mPieces[PartIdx];

		if (strncasecmp(SearchString, Info->m_strDescription, Length) == 0)
		{
			if (!BestMatch || strcasecmp(BestMatch->m_strDescription, Info->m_strDescription) > 0)
				BestMatch = Info;
		}
		else if (strncasecmp(SearchString, Info->m_strName, Length) == 0)
		{
			if (!BestMatch || strcasecmp(BestMatch->m_strName, Info->m_strName) > 0)
				BestMatch = Info;
		}
	}

	if (BestMatch)
		mPartsTree->setCurrentPart(BestMatch);
}

void lcMainWindow::Print(QPrinter* Printer)
{
	lcModel* Model = lcGetActiveModel();
	int DocCopies;
	int PageCopies;

	int Rows = lcGetProfileInt(LC_PROFILE_PRINT_ROWS);
	int Columns = lcGetProfileInt(LC_PROFILE_PRINT_COLUMNS);
	int StepsPerPage = Rows * Columns;
	int PageCount = (Model->GetLastStep() + StepsPerPage - 1) / StepsPerPage;

	if (Printer->collateCopies())
	{
		DocCopies = 1;
		PageCopies = Printer->supportsMultipleCopies() ? 1 : Printer->copyCount();
	}
	else
	{
		DocCopies = Printer->supportsMultipleCopies() ? 1 : Printer->copyCount();
		PageCopies = 1;
	}

	int FromPage = Printer->fromPage();
	int ToPage = Printer->toPage();
	bool Ascending = true;

	if (FromPage == 0 && ToPage == 0)
	{
		FromPage = 1;
		ToPage = PageCount;
	}

	FromPage = qMax(1, FromPage);
	ToPage = qMin(PageCount, ToPage);

	if (ToPage < FromPage)
		return;

	if (Printer->pageOrder() == QPrinter::LastPageFirst)
	{
		int Tmp = FromPage;
		FromPage = ToPage;
		ToPage = Tmp;
		Ascending = false;
	}

	mPreviewWidget->MakeCurrent();
	lcContext* Context = mPreviewWidget->mContext;

	QRect PageRect = Printer->pageRect();

	int StepWidth = PageRect.width() / Columns;
	int StepHeight = PageRect.height() / Rows;

	GLint MaxTexture;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTexture);

	MaxTexture = qMin(MaxTexture, 2048);

	int TileWidth = qMin(StepWidth, MaxTexture);
	int TileHeight = qMin(StepHeight, MaxTexture);
	float AspectRatio = (float)StepWidth / (float)StepHeight;

	View View(Model);
	View.SetCamera(GetActiveView()->mCamera, false);
	View.mWidth = TileWidth;
	View.mHeight = TileHeight;
	View.SetContext(Context);

	Context->BeginRenderToTexture(TileWidth, TileHeight);

	lcStep PreviousTime = Model->GetCurrentStep();

	QPainter Painter(Printer);
	lcuint8* Buffer = (lcuint8*)malloc(TileWidth * TileHeight * 4);
	// TODO: option to print background

	for (int DocCopy = 0; DocCopy < DocCopies; DocCopy++)
	{
		int Page = FromPage;

		for (;;)
		{
			for (int PageCopy = 0; PageCopy < PageCopies; PageCopy++)
			{
				if (Printer->printerState() == QPrinter::Aborted || Printer->printerState() == QPrinter::Error)
				{
					free(Buffer);
					Model->SetCurrentStep(PreviousTime);
					Context->EndRenderToTexture();
					return;
				}

				lcStep CurrentStep = 1 + ((Page - 1) * Rows * Columns);

				for (int Row = 0; Row < Rows; Row++)
				{
					for (int Column = 0; Column < Columns; Column++)
					{
						if (CurrentStep > Model->GetLastStep())
							break;

						Model->SetCurrentStep(CurrentStep);

						if (StepWidth > TileWidth || StepHeight > TileHeight)
						{
							lcuint8* ImageBuffer = (lcuint8*)malloc(StepWidth * StepHeight * 4);

							lcCamera* Camera = View.mCamera;
							Camera->StartTiledRendering(TileWidth, TileHeight, StepWidth, StepHeight, AspectRatio);
							do 
							{
								View.OnDraw();

								int TileRow, TileColumn, CurrentTileWidth, CurrentTileHeight;
								Camera->GetTileInfo(&TileRow, &TileColumn, &CurrentTileWidth, &CurrentTileHeight);

								glFinish();
								glReadPixels(0, 0, CurrentTileWidth, CurrentTileHeight, GL_RGBA, GL_UNSIGNED_BYTE, Buffer);

								lcuint32 TileY = 0;
								if (TileRow != 0)
									TileY = TileRow * TileHeight - (TileHeight - StepHeight % TileHeight);

								lcuint32 tileStart = ((TileColumn * TileWidth) + (TileY * StepWidth)) * 4;

								for (int y = 0; y < CurrentTileHeight; y++)
								{
									lcuint8* src = Buffer + (CurrentTileHeight - y - 1) * CurrentTileWidth * 4;
									lcuint8* dst = ImageBuffer + tileStart + y * StepWidth * 4;

									for (int x = 0; x < CurrentTileWidth; x++)
									{
										*dst++ = src[2];
										*dst++ = src[1];
										*dst++ = src[0];
										*dst++ = 255;

										src += 4;
									}
								}
							} while (Camera->EndTile());

							QImage Image = QImage((const lcuint8*)ImageBuffer, StepWidth, StepHeight, QImage::Format_ARGB32_Premultiplied);

							QRect Rect = Painter.viewport();
							int Left = Rect.x() + (StepWidth * Column);
							int Bottom = Rect.y() + (StepHeight * Row);

							Painter.drawImage(Left, Bottom, Image);

							free(ImageBuffer);
						}
						else
						{
							View.OnDraw();

							glFinish();
							glReadPixels(0, 0, TileWidth, TileHeight, GL_RGBA, GL_UNSIGNED_BYTE, Buffer);

							for (int y = 0; y < (TileHeight + 1) / 2; y++)
							{
								lcuint8* Top = (lcuint8*)Buffer + ((TileHeight - y - 1) * TileWidth * 4);
								lcuint8* Bottom = (lcuint8*)Buffer + y * TileWidth * 4;

								for (int x = 0; x < TileWidth; x++)
								{
									lcuint8 Red = Top[0];
									lcuint8 Green = Top[1];
									lcuint8 Blue = Top[2];
									lcuint8 Alpha = 255;//top[3];

									Top[0] = Bottom[2];
									Top[1] = Bottom[1];
									Top[2] = Bottom[0];
									Top[3] = 255;//Bottom[3];

									Bottom[0] = Blue;
									Bottom[1] = Green;
									Bottom[2] = Red;
									Bottom[3] = Alpha;

									Top += 4;
									Bottom +=4;
								}
							}

							QImage image = QImage((const lcuint8*)Buffer, TileWidth, TileHeight, QImage::Format_ARGB32);

							QRect rect = Painter.viewport();
							int left = rect.x() + (StepWidth * Column);
							int bottom = rect.y() + (StepHeight * Row);

							Painter.drawImage(left, bottom, image);
						}

						// TODO: add print options somewhere but Qt doesn't allow changes to the page setup dialog
//						DWORD dwPrint = theApp.GetProfileInt("Settings","Print", PRINT_NUMBERS|PRINT_BORDER);

						QRect Rect = Painter.viewport();
						int Left = Rect.x() + (StepWidth * Column);
						int Right = Rect.x() + (StepWidth * (Column + 1));
						int Top = Rect.y() + (StepHeight * Row);
						int Bottom = Rect.y() + (StepHeight * (Row + 1));

//						if (print text)
						{
							QFont Font("Helvetica", Printer->resolution());
							Painter.setFont(Font);

							QFontMetrics FontMetrics(Font);

							int TextTop = Top + Printer->resolution() / 2 + FontMetrics.ascent();
							int TextLeft = Left + Printer->resolution() / 2;

							Painter.drawText(TextLeft, TextTop, QString::number(CurrentStep));
						}

//						if (print border)
						{
							QPen BlackPen(Qt::black, 2);
							Painter.setPen(BlackPen);

							if (Row == 0)
								Painter.drawLine(Left, Top, Right, Top);
							if (Column == 0)
								Painter.drawLine(Left, Top, Left, Bottom);
							Painter.drawLine(Left, Bottom, Right, Bottom);
							Painter.drawLine(Right, Top, Right, Bottom);
						}

						CurrentStep++;
					}
				}

				// TODO: print header and footer

				if (PageCopy < PageCopies - 1)
					Printer->newPage();
			}

			if (Page == ToPage)
				break;

			if (Ascending)
				Page++;
			else
				Page--;

			Printer->newPage();
		}

		if (DocCopy < DocCopies - 1)
			Printer->newPage();
	}

	free(Buffer);

	Model->SetCurrentStep(PreviousTime);

	Context->EndRenderToTexture();
}

void lcMainWindow::ShowPrintDialog()
{
	lcModel* Model = lcGetActiveModel();
	int Rows = lcGetProfileInt(LC_PROFILE_PRINT_ROWS);
	int Columns = lcGetProfileInt(LC_PROFILE_PRINT_COLUMNS);
	int StepsPerPage = Rows * Columns;
	int PageCount = (Model->GetLastStep() + StepsPerPage - 1) / StepsPerPage;

	QPrinter Printer(QPrinter::HighResolution);
	Printer.setFromTo(1, PageCount + 1);

	QPrintDialog PrintDialog(&Printer, this);

	if (PrintDialog.exec() == QDialog::Accepted)
		Print(&Printer);
}

// todo: call dialogs directly
#include "lc_qimagedialog.h"
#include "lc_qhtmldialog.h"
#include "lc_qpovraydialog.h"
#include "lc_qpropertiesdialog.h"
#include "lc_qfinddialog.h"
#include "lc_qselectdialog.h"
#include "lc_qminifigdialog.h"
#include "lc_qarraydialog.h"
#include "lc_qgroupdialog.h"
#include "lc_qeditgroupsdialog.h"
#include "lc_qpreferencesdialog.h"
#include "lc_qupdatedialog.h"
#include "lc_qaboutdialog.h"

bool lcMainWindow::DoDialog(LC_DIALOG_TYPE Type, void* Data)
{
	switch (Type)
	{
	case LC_DIALOG_SAVE_IMAGE:
		{
			lcQImageDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_EXPORT_HTML:
		{
			lcQHTMLDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_EXPORT_POVRAY:
		{
			lcQPOVRayDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_PROPERTIES:
		{
			lcQPropertiesDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_FIND:
		{
			lcQFindDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_SELECT_BY_NAME:
		{
			lcQSelectDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_MINIFIG:
		{
			lcQMinifigDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_PIECE_ARRAY:
		{
			lcQArrayDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_PIECE_GROUP:
		{
			lcQGroupDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_EDIT_GROUPS:
		{
			lcQEditGroupsDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_PREFERENCES:
		{
			lcQPreferencesDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_ABOUT:
		{
			lcQAboutDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;

	case LC_DIALOG_CHECK_UPDATES:
		{
			lcQUpdateDialog Dialog(this, Data);
			return Dialog.exec() == QDialog::Accepted;
		} break;
	}

	return false;
}

void lcMainWindow::ResetCameras()
{
	for (int ViewIdx = 0; ViewIdx < mViews.GetSize(); ViewIdx++)
		mViews[ViewIdx]->SetDefaultCamera();

	lcGetActiveModel()->DeleteAllCameras();
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

	QAction* Action = mActions[LC_EDIT_ACTION_FIRST + mTool];

	if (Action)
		Action->setChecked(true);

	UpdateAllViews();
}

void lcMainWindow::SetColorIndex(int ColorIndex)
{
	mColorIndex = ColorIndex;

	if (mPreviewWidget)
		mPreviewWidget->Redraw();

	UpdateColor();
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

void lcMainWindow::SetRelativeTransform(bool RelativeTransform)
{
	mRelativeTransform = RelativeTransform;
	UpdateLockSnap();
	UpdateAllViews();
}

void lcMainWindow::SetTransformType(lcTransformType TransformType)
{
	mTransformType = TransformType;

	const char* IconNames[] =
	{
		":/resources/edit_transform_absolute_translation.png",
		":/resources/edit_transform_relative_translation.png",
		":/resources/edit_transform_absolute_rotation.png",
		":/resources/edit_transform_relative_rotation.png"
	};

	if (TransformType >= 0 && TransformType <= 3)
	{
		mActions[LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION + TransformType]->setChecked(true);
		mActions[LC_EDIT_TRANSFORM]->setIcon(QIcon(IconNames[TransformType]));
	}
}

lcVector3 lcMainWindow::GetTransformAmount()
{
	lcVector3 Transform;

	Transform.x = mTransformXEdit->text().toFloat();
	Transform.y = mTransformYEdit->text().toFloat();
	Transform.z = mTransformZEdit->text().toFloat();

	return Transform;
}

void lcMainWindow::SplitView(Qt::Orientation Orientation)
{
	QWidget* Focus = focusWidget();

	if (Focus->metaObject() != &lcQGLWidget::staticMetaObject)
		return;

	QWidget* Parent = Focus->parentWidget();
	QSplitter* Splitter;
	QList<int> Sizes;

	if (Parent == centralWidget())
	{
		Splitter = new QSplitter(Orientation, Parent);
		Parent->layout()->addWidget(Splitter);
		Splitter->addWidget(Focus);
		Splitter->addWidget(new lcQGLWidget(centralWidget(), mPiecePreviewWidget, new View(lcGetActiveModel()), true));
	}
	else
	{
		QSplitter* ParentSplitter = (QSplitter*)Parent;	
		Sizes = ParentSplitter->sizes();
		int FocusIndex = ParentSplitter->indexOf(Focus);

		Splitter = new QSplitter(Orientation, Parent);
		ParentSplitter->insertWidget(FocusIndex, Splitter);
		Splitter->addWidget(Focus);
		Splitter->addWidget(new lcQGLWidget(centralWidget(), mPiecePreviewWidget, new View(lcGetActiveModel()), true));

		ParentSplitter->setSizes(Sizes);
	}

	Sizes.clear();
	Sizes.append(10);
	Sizes.append(10);
	Splitter->setSizes(Sizes);
}

void lcMainWindow::SplitHorizontal()
{
	SplitView(Qt::Vertical);
}

void lcMainWindow::SplitVertical()
{
	SplitView(Qt::Horizontal);
}

void lcMainWindow::RemoveActiveView()
{
	QWidget* Focus = focusWidget();

	if (Focus->metaObject() != &lcQGLWidget::staticMetaObject)
		return;

	QWidget* Parent = Focus->parentWidget();

	if (Parent == centralWidget())
		return;

	QWidget* ParentParentWidget = Parent->parentWidget();
	QSplitter* ParentSplitter = (QSplitter*)Parent;
	int FocusIndex = ParentSplitter->indexOf(Focus);

	if (ParentParentWidget == centralWidget())
	{
		QLayout* CentralLayout = ParentParentWidget->layout();

		CentralLayout->addWidget(ParentSplitter->widget(!FocusIndex));
		CentralLayout->removeWidget(Parent);

		return;
	}

	QSplitter* ParentParentSplitter = (QSplitter*)ParentParentWidget;
	QList<int> Sizes = ParentParentSplitter->sizes();

	int ParentIndex = ParentParentSplitter->indexOf(Parent);
	ParentParentSplitter->insertWidget(!ParentIndex, Focus);

	delete Parent;

	ParentParentSplitter->setSizes(Sizes);
}

void lcMainWindow::ResetViews()
{
	QLayout* CentralLayout = centralWidget()->layout();
	delete CentralLayout->itemAt(0)->widget();
	CentralLayout->addWidget(new lcQGLWidget(centralWidget(), mPiecePreviewWidget, new View(lcGetActiveModel()), true));
}

void lcMainWindow::TogglePrintPreview()
{
	// todo: print preview inside main window

	lcModel* Model = lcGetActiveModel();
	int Rows = lcGetProfileInt(LC_PROFILE_PRINT_ROWS);
	int Columns = lcGetProfileInt(LC_PROFILE_PRINT_COLUMNS);
	int StepsPerPage = Rows * Columns;
	int PageCount = (Model->GetLastStep() + StepsPerPage - 1) / StepsPerPage;

	QPrinter Printer(QPrinter::ScreenResolution);
	Printer.setFromTo(1, PageCount + 1);

	QPrintPreviewDialog Preview(&Printer, this);
	connect(&Preview, SIGNAL(paintRequested(QPrinter*)), SLOT(Print(QPrinter*)));
	Preview.exec();
}

void lcMainWindow::ToggleFullScreen()
{
	// todo: hide toolbars and menu
	// todo: create fullscreen toolbar or support esc key to go back
	if (isFullScreen())
		showNormal();
	else
		showFullScreen();
}

void lcMainWindow::AddRecentFile(const QString& FileName)
{
	QString SavedName = FileName;
	int FileIdx;

	QFileInfo FileInfo(FileName);

	for (FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		if (QFileInfo(mRecentFiles[FileIdx]) == FileInfo)
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

void lcMainWindow::UpdateFocusObject(lcObject* Focus)
{
	mPropertiesWidget->updateFocusObject(Focus);

	lcVector3 Position;
	lcGetActiveModel()->GetFocusPosition(Position);

	QString Label("X: %1 Y: %2 Z: %3");
	Label = Label.arg(QString::number(Position[0], 'f', 2), QString::number(Position[1], 'f', 2), QString::number(Position[2], 'f', 2));
	mStatusPositionLabel->setText(Label);
}

void lcMainWindow::UpdateSelectedObjects(int Flags, int SelectedCount, lcObject* Focus)
{
	mActions[LC_EDIT_CUT]->setEnabled(Flags & LC_SEL_SELECTED);
	mActions[LC_EDIT_COPY]->setEnabled(Flags & LC_SEL_SELECTED);
	mActions[LC_EDIT_FIND]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
	mActions[LC_EDIT_FIND_NEXT]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
	mActions[LC_EDIT_FIND_PREVIOUS]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
	mActions[LC_EDIT_SELECT_INVERT]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
	mActions[LC_EDIT_SELECT_BY_NAME]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
	mActions[LC_EDIT_SELECT_NONE]->setEnabled(Flags & LC_SEL_SELECTED);
	mActions[LC_EDIT_SELECT_ALL]->setEnabled(Flags & LC_SEL_UNSELECTED);

	mActions[LC_PIECE_DELETE]->setEnabled(Flags & LC_SEL_SELECTED);
	mActions[LC_PIECE_ARRAY]->setEnabled(Flags & LC_SEL_PIECE);
	mActions[LC_PIECE_HIDE_SELECTED]->setEnabled(Flags & LC_SEL_PIECE);
	mActions[LC_PIECE_UNHIDE_ALL]->setEnabled(Flags & LC_SEL_HIDDEN);
	mActions[LC_PIECE_HIDE_UNSELECTED]->setEnabled(Flags & LC_SEL_UNSELECTED);
	mActions[LC_PIECE_GROUP]->setEnabled(Flags & LC_SEL_CAN_GROUP);
	mActions[LC_PIECE_UNGROUP]->setEnabled(Flags & LC_SEL_GROUPED);
	mActions[LC_PIECE_GROUP_ADD]->setEnabled((Flags & (LC_SEL_GROUPED | LC_SEL_FOCUS_GROUPED)) == LC_SEL_GROUPED);
	mActions[LC_PIECE_GROUP_REMOVE]->setEnabled(Flags & LC_SEL_FOCUS_GROUPED);
	mActions[LC_PIECE_GROUP_EDIT]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
	mActions[LC_PIECE_SHOW_EARLIER]->setEnabled(Flags & LC_SEL_PIECE); // FIXME: disable if current step is 1
	mActions[LC_PIECE_SHOW_LATER]->setEnabled(Flags & LC_SEL_PIECE);

	QString Message;

	if ((SelectedCount == 1) && Focus)
	{
		if (Focus->IsPiece())
			Message = QString("%1 (ID: %2)").arg(Focus->GetName(), ((lcPiece*)Focus)->mPieceInfo->m_strName);
		else
			Message = Focus->GetName();
	}
	else if (SelectedCount > 0)
	{
		if (SelectedCount == 1)
			Message = "1 Object selected";
		else
			Message = QString("%1 Objects selected").arg(QString::number(SelectedCount));

		if (Focus && Focus->IsPiece())
		{
			Message.append(QString(" - %1 (ID: %2)").arg(Focus->GetName(), ((lcPiece*)Focus)->mPieceInfo->m_strName));

			const lcGroup* Group = ((lcPiece*)Focus)->GetGroup();
			if (Group && Group->m_strName[0])
				Message.append(QString(" in group '%1'").arg(Group->m_strName));
		}
	}

	mStatusBarLabel->setText(Message);
}

void lcMainWindow::UpdatePaste(bool Enabled)
{
	QAction* Action = mActions[LC_EDIT_PASTE];

	if (Action)
		Action->setEnabled(Enabled);
}

void lcMainWindow::UpdateCurrentStep()
{
	lcModel* Model = lcGetActiveModel();
	lcStep CurrentStep = Model->GetCurrentStep();
	lcStep LastStep = Model->GetLastStep();

	mActions[LC_VIEW_TIME_FIRST]->setEnabled(CurrentStep != 1);
	mActions[LC_VIEW_TIME_PREVIOUS]->setEnabled(CurrentStep > 1);
	mActions[LC_VIEW_TIME_NEXT]->setEnabled(CurrentStep < LC_STEP_MAX);
	mActions[LC_VIEW_TIME_LAST]->setEnabled(CurrentStep != LastStep);

	mStatusTimeLabel->setText(QString(tr("Step %1")).arg(QString::number(CurrentStep)));
}

void lcMainWindow::SetAddKeys(bool AddKeys)
{
	QAction* Action = mActions[LC_VIEW_TIME_ADD_KEYS];

	if (Action)
		Action->setChecked(AddKeys);

	mAddKeys = AddKeys;
}

void lcMainWindow::UpdateLockSnap()
{
	mActions[LC_EDIT_TRANSFORM_RELATIVE]->setChecked(GetRelativeTransform());
	mActions[LC_EDIT_LOCK_X]->setChecked(GetLockX());
	mActions[LC_EDIT_LOCK_Y]->setChecked(GetLockY());
	mActions[LC_EDIT_LOCK_Z]->setChecked(GetLockZ());
}

void lcMainWindow::UpdateSnap()
{
	mActions[LC_EDIT_SNAP_MOVE_XY0 + GetMoveXYSnapIndex()]->setChecked(true);
	mActions[LC_EDIT_SNAP_MOVE_Z0 + GetMoveZSnapIndex()]->setChecked(true);
	mActions[LC_EDIT_SNAP_ANGLE0 + GetAngleSnapIndex()]->setChecked(true);

	mStatusSnapLabel->setText(QString(tr(" M: %1 %2 R: %3 ")).arg(GetMoveXYSnapText(), GetMoveZSnapText(), QString::number(GetAngleSnap())));
}

void lcMainWindow::UpdateColor()
{
	mColorList->setCurrentColor(mColorIndex);
}

void lcMainWindow::UpdateUndoRedo(const QString& UndoText, const QString& RedoText)
{
	QAction* UndoAction = mActions[LC_EDIT_UNDO];
	QAction* RedoAction = mActions[LC_EDIT_REDO];

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

void lcMainWindow::UpdateCameraMenu()
{
	const lcArray<lcCamera*>& Cameras = lcGetActiveModel()->GetCameras();
	lcCamera* CurrentCamera = mActiveView->mCamera;
	int CurrentIndex = -1;

	for (int ActionIdx = LC_VIEW_CAMERA_FIRST; ActionIdx <= LC_VIEW_CAMERA_LAST; ActionIdx++)
	{
		QAction* Action = mActions[ActionIdx];
		int CameraIdx = ActionIdx - LC_VIEW_CAMERA_FIRST;

		if (CameraIdx < Cameras.GetSize())
		{
			if (CurrentCamera == Cameras[CameraIdx])
				CurrentIndex = CameraIdx;

			Action->setText(Cameras[CameraIdx]->GetName());
			Action->setVisible(true);
		}
		else
			Action->setVisible(false);
	}

	UpdateCurrentCamera(CurrentIndex);
}

void lcMainWindow::UpdateCurrentCamera(int CameraIndex)
{
	int ActionIndex = LC_VIEW_CAMERA_FIRST + CameraIndex;

	if (ActionIndex < LC_VIEW_CAMERA_FIRST || ActionIndex > LC_VIEW_CAMERA_LAST)
		ActionIndex = LC_VIEW_CAMERA_NONE;

	mActions[ActionIndex]->setChecked(true);
}

void lcMainWindow::UpdatePerspective()
{
	if (mActiveView->mCamera->IsOrtho())
		mActions[LC_VIEW_PROJECTION_ORTHO]->setChecked(true);
	else
		mActions[LC_VIEW_PROJECTION_PERSPECTIVE]->setChecked(true);
}

void lcMainWindow::UpdateModels()
{
	const lcArray<lcModel*>& Models = lcGetActiveProject()->GetModels();
	lcModel* CurrentModel = lcGetActiveModel();

	for (int ActionIdx = LC_MODEL_FIRST; ActionIdx <= LC_MODEL_LAST; ActionIdx++)
	{
		QAction* Action = mActions[ActionIdx];
		int ModelIdx = ActionIdx - LC_MODEL_FIRST;

		if (ModelIdx < Models.GetSize())
		{
			Action->setChecked(CurrentModel == Models[ModelIdx]);
			Action->setText(QString::fromLatin1("&%1 %2").arg(QString::number(ModelIdx + 1), Models[ModelIdx]->GetProperties().mName));
			Action->setVisible(true);
		}
		else
			Action->setVisible(false);
	}

	mPartsTree->UpdateModels();
}

void lcMainWindow::UpdateCategories()
{
	mPartsTree->updateCategories();
}

void lcMainWindow::UpdateTitle()
{
	setWindowModified(lcGetActiveProject()->IsModified());
	setWindowFilePath(lcGetActiveProject()->GetTitle());
}

void lcMainWindow::UpdateModified(bool Modified)
{
	setWindowModified(Modified);
}

void lcMainWindow::UpdateRecentFiles()
{
	for (int ActionIdx = LC_FILE_RECENT_FIRST; ActionIdx <= LC_FILE_RECENT_LAST; ActionIdx++)
	{
		int FileIdx = ActionIdx - LC_FILE_RECENT_FIRST;
		QAction* Action = mActions[ActionIdx];

		if (!mRecentFiles[FileIdx].isEmpty())
		{
			Action->setText(QString("&%1 %2").arg(QString::number(FileIdx + 1), QDir::toNativeSeparators(mRecentFiles[FileIdx])));
			Action->setVisible(true);
		}
		else
			Action->setVisible(false);
	}

	mActionFileRecentSeparator->setVisible(!mRecentFiles[0].isEmpty());
}

void lcMainWindow::UpdateShortcuts()
{
	for (int ActionIdx = 0; ActionIdx < LC_NUM_COMMANDS; ActionIdx++)
		mActions[ActionIdx]->setShortcut(QKeySequence(gKeyboardShortcuts.Shortcuts[ActionIdx]));
}

void lcMainWindow::NewProject()
{
	if (!SaveProjectIfModified())
		return;

	Project* NewProject = new Project();
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

		LoadFileName = QFileDialog::getOpenFileName(this, tr("Open Project"), LoadFileName, tr("Supported Files (*.lcd *.ldr *.dat *.mpd);;All Files (*.*)"));

		if (LoadFileName.isEmpty())
			return false;
	}

	Project* NewProject = new Project();

	if (NewProject->Load(LoadFileName))
	{
		NewProject->SetActiveModel(0);

		g_App->SetProject(NewProject);
		AddRecentFile(LoadFileName);

		for (int ViewIdx = 0; ViewIdx < mViews.GetSize(); ViewIdx++)
		{
			View* View = mViews[ViewIdx];

			if (!View->mCamera->IsSimple())
				View->SetDefaultCamera();

			View->ZoomExtents();
		}

		UpdateAllViews();

		return true;
	}

	QMessageBox::information(this, tr("LeoCAD"), tr("Error loading '%1'.").arg(LoadFileName));
	delete NewProject;

	return false;
}

void lcMainWindow::MergeProject()
{
	QString LoadFileName = lcGetActiveProject()->GetFileName();

	if (LoadFileName.isEmpty())
		LoadFileName = lcGetProfileString(LC_PROFILE_PROJECTS_PATH);

	LoadFileName = QFileDialog::getOpenFileName(this, tr("Open Project"), LoadFileName, tr("Supported Files (*.lcd *.ldr *.dat *.mpd);;All Files (*.*)"));

	if (LoadFileName.isEmpty())
		return;

	Project* NewProject = new Project();

	if (NewProject->Load(LoadFileName))
	{
		int NumModels = NewProject->GetModels().GetSize();

		lcGetActiveProject()->Merge(NewProject);

		if (NumModels == 1)
			QMessageBox::information(this, tr("LeoCAD"), tr("Merged 1 model."));
		else
			QMessageBox::information(this, tr("LeoCAD"), tr("Merged %1 models.").arg(NumModels));

		UpdateModels();
	}
	else
	{
		QMessageBox::information(this, tr("LeoCAD"), tr("Error loading '%1'.").arg(LoadFileName));
		delete NewProject;
	}
}

bool lcMainWindow::SaveProject(const QString& FileName)
{
	QString SaveFileName;
	Project* Project = lcGetActiveProject();

	if (!FileName.isEmpty())
		SaveFileName = FileName;
	else
	{
		SaveFileName = Project->GetFileName();

		if (SaveFileName.isEmpty())
			SaveFileName = QFileInfo(QDir(lcGetProfileString(LC_PROFILE_PROJECTS_PATH)), Project->GetTitle()).absoluteFilePath();

		QString Filter = (Project->GetModels().GetSize() > 1) ? tr("Supported Files (*.mpd);;All Files (*.*)") : tr("Supported Files (*.ldr *.dat *.mpd);;All Files (*.*)");

		SaveFileName = QFileDialog::getSaveFileName(this, tr("Save Project"), SaveFileName, Filter);

		if (SaveFileName.isEmpty())
			return false;
	}

	if (QFileInfo(SaveFileName).suffix().toLower() == QLatin1String("lcd"))
	{
		QMessageBox::warning(this, tr("Error"), tr("Saving files in LCD format is no longer supported, please use the LDR or MPD formats instead."));
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

	switch (QMessageBox::question(this, tr("Save Project"), tr("Save changes to '%1'?").arg(Project->GetTitle()), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel))
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

	case LC_FILE_MERGE:
		MergeProject();
		break;

	case LC_FILE_SAVE:
		SaveProject(lcGetActiveProject()->GetFileName());
		break;

	case LC_FILE_SAVEAS:
		SaveProject(QString());
		break;

	case LC_FILE_SAVE_IMAGE:
		lcGetActiveProject()->SaveImage();
		break;

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
		lcGetActiveProject()->ExportWavefront(QString());
		break;

	case LC_FILE_PRINT_PREVIEW:
		TogglePrintPreview();
		break;

	case LC_FILE_PRINT:
		ShowPrintDialog();
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
		close();
		break;

	case LC_EDIT_UNDO:
		lcGetActiveModel()->UndoAction();
		break;

	case LC_EDIT_REDO:
		lcGetActiveModel()->RedoAction();
		break;

	case LC_EDIT_CUT:
		lcGetActiveModel()->Cut();
		break;

	case LC_EDIT_COPY:
		lcGetActiveModel()->Copy();
		break;

	case LC_EDIT_PASTE:
		lcGetActiveModel()->Paste();
		break;

	case LC_EDIT_FIND:
		if (DoDialog(LC_DIALOG_FIND, &mSearchOptions))
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
		RemoveActiveView();
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

	case LC_PIECE_INSERT:
		lcGetActiveModel()->AddPiece();
		break;

	case LC_PIECE_DELETE:
		lcGetActiveModel()->DeleteSelectedObjects();
		break;

	case LC_PIECE_MOVE_PLUSX:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(lcMax(GetMoveXYSnap(), 0.01f), 0.0f, 0.0f)), true, true);
		break;

	case LC_PIECE_MOVE_MINUSX:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(-lcMax(GetMoveXYSnap(), 0.01f), 0.0f, 0.0f)), true, true);
		break;

	case LC_PIECE_MOVE_PLUSY:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(0.0f, lcMax(GetMoveXYSnap(), 0.01f), 0.0f)), true, true);
		break;

	case LC_PIECE_MOVE_MINUSY:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(0.0f, -lcMax(GetMoveXYSnap(), 0.01f), 0.0f)), true, true);
		break;

	case LC_PIECE_MOVE_PLUSZ:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, lcMax(GetMoveZSnap(), 0.01f))), true, true);
		break;

	case LC_PIECE_MOVE_MINUSZ:
		lcGetActiveModel()->MoveSelectedObjects(mActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, -lcMax(GetMoveZSnap(), 0.01f))), true, true);
		break;

	case LC_PIECE_ROTATE_PLUSX:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(lcVector3(lcMax(GetAngleSnap(), 1.0f), 0.0f, 0.0f)), true, true);
		break;

	case LC_PIECE_ROTATE_MINUSX:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(-lcVector3(lcMax(GetAngleSnap(), 1.0f), 0.0f, 0.0f)), true, true);
		break;

	case LC_PIECE_ROTATE_PLUSY:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(lcVector3(0.0f, lcMax(GetAngleSnap(), 1.0f), 0.0f)), true, true);
		break;

	case LC_PIECE_ROTATE_MINUSY:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(lcVector3(0.0f, -lcMax(GetAngleSnap(), 1.0f), 0.0f)), true, true);
		break;

	case LC_PIECE_ROTATE_PLUSZ:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, lcMax(GetAngleSnap(), 1.0f))), true, true);
		break;

	case LC_PIECE_ROTATE_MINUSZ:
		lcGetActiveModel()->RotateSelectedPieces(mActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, -lcMax(GetAngleSnap(), 1.0f))), true, true);
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
		lcGetActiveModel()->Zoom(mActiveView->mCamera, 10.0f);
		break;

	case LC_VIEW_ZOOM_OUT:
		lcGetActiveModel()->Zoom(mActiveView->mCamera, -10.0f);
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

	case LC_VIEW_CAMERA_RESET:
		ResetCameras();
		break;

	case LC_MODEL_NEW:
		lcGetActiveProject()->CreateNewModel();
		break;

	case LC_MODEL_PROPERTIES:
		lcGetActiveModel()->ShowPropertiesDialog();
		break;

	case LC_MODEL_LIST:
		lcGetActiveProject()->ShowModelListDialog();
		break;

	case LC_MODEL_01:
	case LC_MODEL_02:
	case LC_MODEL_03:
	case LC_MODEL_04:
	case LC_MODEL_05:
	case LC_MODEL_06:
	case LC_MODEL_07:
	case LC_MODEL_08:
	case LC_MODEL_09:
	case LC_MODEL_10:
	case LC_MODEL_11:
	case LC_MODEL_12:
	case LC_MODEL_13:
	case LC_MODEL_14:
	case LC_MODEL_15:
	case LC_MODEL_16:
		lcGetActiveProject()->SetActiveModel(CommandId - LC_MODEL_01);
		break;

	case LC_HELP_HOMEPAGE:
		QDesktopServices::openUrl(QUrl("http://www.leocad.org/"));
		break;

	case LC_HELP_EMAIL:
		QDesktopServices::openUrl(QUrl("mailto:leozide@gmail.com?subject=LeoCAD"));
		break;

	case LC_HELP_UPDATES:
		DoDialog(LC_DIALOG_CHECK_UPDATES, NULL);
		break;

	case LC_HELP_ABOUT:
		DoDialog(LC_DIALOG_ABOUT, NULL);

	case LC_VIEW_TIME_ADD_KEYS:
		SetAddKeys(!GetAddKeys());
		break;

	case LC_EDIT_TRANSFORM_RELATIVE:
		SetRelativeTransform(!GetRelativeTransform());
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

#include "lc_global.h"
#include "lc_qmainwindow.h"
#include "lc_glwidget.h"
#include "lc_library.h"
#include "lc_application.h"
#include "pieceinf.h"
#include "project.h"
#include "preview.h"
#include "camera.h"
#include "view.h"
#include "lc_qpartstree.h"
#include "lc_colorlistwidget.h"
#include "system.h"

lcQMainWindow::lcQMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	memset(actions, 0, sizeof(actions));

	setWindowFilePath(QString());

	resize(800, 600);
//	centralWidget = new lcViewWidget(this, NULL);
//	centralWidget->mWindow->OnInitialUpdate();
//	setCentralWidget(centralWidget);

	QFrame *previewFrame = new QFrame;
	previewFrame->setFrameShape(QFrame::StyledPanel);
	previewFrame->setFrameShadow(QFrame::Sunken);
	setCentralWidget(previewFrame);

	QGridLayout *previewLayout = new QGridLayout(previewFrame);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	centralWidget = new lcGLWidget(previewFrame, NULL, new View(lcGetActiveProject(), NULL));
	previewLayout->addWidget(centralWidget, 0, 0, 1, 1);

	createActions();
	createMenus();
	createToolBars();
	createStatusBar();

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	PieceInfo* Info = Library->FindPiece("3005", false);

	if (!Info)
		Info = Library->mPieces[0];

	if (Info)
	{
		lcGetActiveProject()->SetCurrentPiece(Info);
		PiecePreview* Preview = (PiecePreview*)piecePreview->mWindow;
		Preview->SetCurrentPiece(Info);
	}
}

lcQMainWindow::~lcQMainWindow()
{
}

void lcQMainWindow::createActions()
{
	const char* actionStrings[LC_NUM_COMMANDS][2] =
	{
		{ "&New",                             "Create a new project" },                                                                 // LC_FILE_NEW
		{ "&Open...",                         "Open an existing project" },                                                             // LC_FILE_OPEN
		{ "&Merge...",                        "Merge the contents of another project with the current one" },                           // LC_FILE_MERGE
		{ "&Save",                            "Save the active project" },                                                              // LC_FILE_SAVE
		{ "Save &As...",                      "Save the active project with a new name" },                                              // LC_FILE_SAVEAS
		{ "Save &Image...",                   "Save a picture of the current view" },                                                   // LC_FILE_SAVE_IMAGE
		{ "3D &Studio...",                    "Export the project in 3D Studio 3DS format" },                                           // LC_FILE_EXPORT_3DS
		{ "&HTML...",                         "Create an HTML page for this project" },                                                 // LC_FILE_EXPORT_HTML
		{ "&BrickLink...",                    "Export a list of pieces used in BrickLink XML format" },                                 // LC_FILE_EXPORT_BRICKLINK
		{ "&CSV...",                          "Export a list of pieces used in comma delimited file format" },                          // LC_FILE_EXPORT_CSV
		{ "&POV-Ray...",                      "Export the project in POV-Ray format" },                                                 // LC_FILE_EXPORT_POVRAY
		{ "&Wavefront...",                    "Export the project in Wavefront OBJ format" },                                           // LC_FILE_EXPORT_WAVEFRONT
		{ "Prope&rties...",                   "Display project properties" },                                                           // LC_FILE_PROPERTIES
		{ "&Terrain Editor...",               "Edit terrain" },                                                                         // LC_FILE_TERRAIN_EDITOR
		{ "&Print...",                        "Print the active project" },                                                             // LC_FILE_PRINT
		{ "Print Pre&view",                   "Display how the project would look if printed" },                                        // LC_FILE_PRINT_PREVIEW
		{ "Print &Bill of Materials...",      "Print a list of pieces used" },                                                          // LC_FILE_PRINT_BOM
		{ "&1",                               "Open this document" },                                                                   // LC_FILE_RECENT1
		{ "&2",                               "Open this document" },                                                                   // LC_FILE_RECENT2
		{ "&3",                               "Open this document" },                                                                   // LC_FILE_RECENT3
		{ "&4",                               "Open this document" },                                                                   // LC_FILE_RECENT4
		{ "E&xit",                            "Quit the application; prompts to save project" },                                        // LC_FILE_EXIT
		{ "Undo",                             "Undo the last action" },                                                                 // LC_EDIT_UNDO
		{ "Redo",                             "Redo the previously undone action" },                                                    // LC_EDIT_REDO
		{ "Cut",                              "Cut the selection and put it on the Clipboard" },                                        // LC_EDIT_CUT
		{ "Copy",                             "Copy the selection and put it on the Clipboard" },                                       // LC_EDIT_COPY
		{ "Paste",                            "Insert Clipboard contents" },                                                            // LC_EDIT_PASTE
		{ "Select All",                       "Select all pieces in the project" },                                                     // LC_EDIT_SELECT_ALL
		{ "Select None",                      "De-select everything" },                                                                 // LC_EDIT_SELECT_NONE
		{ "Select Invert",                    "Invert the current selection set" },                                                     // LC_EDIT_SELECT_INVERT
		{ "Select by Name...",                "Select objects by name" },                                                               // LC_EDIT_SELECT_BY_NAME
		{ "Lock X",                           "Prevents movement and rotation along the X axis" },                                      // LC_EDIT_LOCK_X
		{ "Lock Y",                           "Prevents movement and rotation along the Y axis" },                                      // LC_EDIT_LOCK_Y
		{ "Lock Z",                           "Prevents movement and rotation along the Z axis" },                                      // LC_EDIT_LOCK_Z
		{ "Lock Toggle",                      "Toggle locked axes" },                                                                   // LC_EDIT_LOCK_TOGGLE
		{ "Unlock All",                       "Allows movement and rotation in all directions" },                                       // LC_EDIT_LOCK_NONE
		{ "Snap X",                           "Snap movement along the X axis to fixed intervals" },                                    // LC_EDIT_SNAP_X
		{ "Snap Y",                           "Snap movement along the Y axis to fixed intervals" },                                    // LC_EDIT_SNAP_Y
		{ "Snap Z",                           "Snap movement along the Z axis to fixed intervals" },                                    // LC_EDIT_SNAP_Z
		{ "Snap Toggle",                      "Toggle snap axes" },                                                                     // LC_EDIT_SNAP_TOGGLE
		{ "Snap None",                        "Disable snapping along all axes" },                                                      // LC_EDIT_SNAP_NONE
		{ "Snap All",                         "Snap movement along all axes to fixed intervals" },                                      // LC_EDIT_SNAP_ALL
		{ "Snap Angle Toggle",                "Snap rotations to fixed intervals" },                                                    // LC_EDIT_SNAP_ANGLE
		{ "None",                             "Do not snap movement along the XY plane" },                                              // LC_EDIT_SNAP_MOVE_XY0
		{ "1/20 Stud",                        "Snap movement along the XY plane to 1/20 stud" },                                        // LC_EDIT_SNAP_MOVE_XY1
		{ "1/4 Stud",                         "Snap movement along the XY plane to 1/4 stud" },                                         // LC_EDIT_SNAP_MOVE_XY2
		{ "1 Flat",                           "Snap movement along the XY plane to 1 flat" },                                           // LC_EDIT_SNAP_MOVE_XY3
		{ "1/2 Stud",                         "Snap movement along the XY plane to 1/2 stud" },                                         // LC_EDIT_SNAP_MOVE_XY4
		{ "1 Stud",                           "Snap movement along the XY plane to 1 stud" },                                           // LC_EDIT_SNAP_MOVE_XY5
		{ "2 Stud",                           "Snap movement along the XY plane to 2 stud" },                                           // LC_EDIT_SNAP_MOVE_XY6
		{ "3 Stud",                           "Snap movement along the XY plane to 3 stud" },                                           // LC_EDIT_SNAP_MOVE_XY7
		{ "4 Stud",                           "Snap movement along the XY plane to 4 stud" },                                           // LC_EDIT_SNAP_MOVE_XY8
		{ "8 Stud",                           "Snap movement along the XY plane to 8 stud" },                                           // LC_EDIT_SNAP_MOVE_XY9
		{ "None",                             "Do not snap movement along the Z axis" },                                                // LC_EDIT_SNAP_MOVE_Z0
		{ "1/20 Stud",                        "Snap movement along the Z axis to 1/20 stud" },                                          // LC_EDIT_SNAP_MOVE_Z1
		{ "1/4 Stud",                         "Snap movement along the Z axis to 1/4 stud" },                                           // LC_EDIT_SNAP_MOVE_Z2
		{ "1 Flat",                           "Snap movement along the Z axis to 1 flat" },                                             // LC_EDIT_SNAP_MOVE_Z3
		{ "1/2 Stud",                         "Snap movement along the Z axis to 1/2 stud" },                                           // LC_EDIT_SNAP_MOVE_Z4
		{ "1 Stud",                           "Snap movement along the Z axis to 1 stud" },                                             // LC_EDIT_SNAP_MOVE_Z5
		{ "2 Stud",                           "Snap movement along the Z axis to 2 stud" },                                             // LC_EDIT_SNAP_MOVE_Z6
		{ "3 Stud",                           "Snap movement along the Z axis to 3 stud" },                                             // LC_EDIT_SNAP_MOVE_Z7
		{ "4 Stud",                           "Snap movement along the Z axis to 4 stud" },                                             // LC_EDIT_SNAP_MOVE_Z8
		{ "8 Stud",                           "Snap movement along the Z axis to 8 stud" },                                             // LC_EDIT_SNAP_MOVE_Z9
		{ "None",                             "Do not snap rotations" },                                                                // LC_EDIT_SNAP_ANGLE0
		{ "1 Degree",                         "Snap rotations to 1 degree" },                                                           // LC_EDIT_SNAP_ANGLE1
		{ "5 Degree",                         "Snap rotations to 5 degrees" },                                                          // LC_EDIT_SNAP_ANGLE2
		{ "10 Degree",                        "Snap rotations to 10 degrees" },                                                         // LC_EDIT_SNAP_ANGLE3
		{ "15 Degree",                        "Snap rotations to 15 degrees" },                                                         // LC_EDIT_SNAP_ANGLE4
		{ "30 Degree",                        "Snap rotations to 30 degrees" },                                                         // LC_EDIT_SNAP_ANGLE5
		{ "45 Degree",                        "Snap rotations to 45 degrees" },                                                         // LC_EDIT_SNAP_ANGLE6
		{ "60 Degree",                        "Snap rotations to 60 degrees" },                                                         // LC_EDIT_SNAP_ANGLE7
		{ "90 Degree",                        "Snap rotations to 90 degrees" },                                                         // LC_EDIT_SNAP_ANGLE8
		{ "180 Degree",                       "Snap rotations to 180 degrees" },                                                        // LC_EDIT_SNAP_ANGLE9
		{ "Transform",                        "Apply transform to selected objects" },                                                  // LC_EDIT_TRANSFORM
		{ "Absolute Translation",             "Switch to absolute translation mode when applying transforms" },                         // LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION
		{ "Relative Translation",             "Switch to relative translation mode when applying transforms" },                         // LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION
		{ "Absolute Rotation",                "Switch to absolute rotation mode when applying transforms" },                            // LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION
		{ "Relative Rotation",                "Switch to relative rotation mode when applying transforms" },                            // LC_EDIT_TRANSFORM_RELATIVE_ROTATION
		{ "Insert",                           "Add new pieces to the model" },                                                          // LC_EDIT_ACTION_INSERT
		{ "Light",                            "Add new omni light sources to the model" },                                              // LC_EDIT_ACTION_LIGHT
		{ "Spotlight",                        "Add new spotlights to the model" },                                                      // LC_EDIT_ACTION_SPOTLIGHT
		{ "Camera",                           "Create a new camera" },                                                                  // LC_EDIT_ACTION_CAMERA
		{ "Select",                           "Select objects (hold the CTRL key down or drag the mouse to select multiple objects)" }, // LC_EDIT_ACTION_SELECT
		{ "Move",                             "Move selected objects" },                                                                // LC_EDIT_ACTION_MOVE
		{ "Rotate",                           "Rotate selected pieces" },                                                               // LC_EDIT_ACTION_ROTATE
		{ "Delete",                           "Delete objects" },                                                                       // LC_EDIT_ACTION_DELETE
		{ "Paint",                            "Change piece color" },                                                                   // LC_EDIT_ACTION_PAINT
		{ "Zoom",                             "Zoom in or out" },                                                                       // LC_EDIT_ACTION_ZOOM
		{ "Pan",                              "Pan the current view" },                                                                 // LC_EDIT_ACTION_PAN
		{ "Rotate View",                      "Rotate the current view" },                                                              // LC_EDIT_ACTION_ROTATE_VIEW
		{ "Roll",                             "Roll the current view" },                                                                // LC_EDIT_ACTION_ROLL
		{ "Zoom Region",                      "Zoom into a region of the screen" },                                                     // LC_EDIT_ACTION_ZOOM_REGION
		{ "Preferences...",                   "Change program settings" },                                                              // LC_VIEW_PREFERENCES
		{ "Zoom In",                          "Zoom in" },                                                                              // LC_VIEW_ZOOM_IN
		{ "Zoom Out",                         "Zoom out" },                                                                             // LC_VIEW_ZOOM_OUT
		{ "Zoom Extents",                     "Fit all pieces in current the view (hold the CTRL key down to zoom all views)" },        // LC_VIEW_ZOOM_EXTENTS
		{ "Front",                            "View model from the front" },                                                            // LC_VIEW_VIEWPOINT_FRONT
		{ "Back",                             "View model from the back" },                                                             // LC_VIEW_VIEWPOINT_BACK
		{ "Top",                              "View model from the top" },                                                              // LC_VIEW_VIEWPOINT_TOP
		{ "Bottom",                           "View model from the bottom" },                                                           // LC_VIEW_VIEWPOINT_BOTTOM
		{ "Left",                             "View model from the left" },                                                             // LC_VIEW_VIEWPOINT_LEFT
		{ "Right",                            "View model from the right" },                                                            // LC_VIEW_VIEWPOINT_RIGHT
		{ "Home",                             "View model from the default position" },                                                 // LC_VIEW_VIEWPOINT_HOME
		{ "None",                             "Do not use a camera" },                                                                  // LC_VIEW_CAMERA_NONE
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA1
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA2
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA3
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA4
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA5
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA6
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA7
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA8
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA9
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA10
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA11
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA12
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA13
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA14
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA15
		{ "Camera",                           "Use this camera" },                                                                      // LC_VIEW_CAMERA16
		{ "Reset",                            "Reset views to their default positions" },                                               // LC_VIEW_CAMERA_RESET
		{ "First",                            "Go to the first step of the model" },                                                    // LC_VIEW_TIME_FIRST
		{ "Previous",                         "Go to the previous step" },                                                              // LC_VIEW_TIME_PREVIOUS
		{ "Next",                             "Go to the next step" },                                                                  // LC_VIEW_TIME_NEXT
		{ "Last",                             "Go to the last step of the model" },                                                     // LC_VIEW_TIME_LAST
		{ "Stop",                             "Stop playing animation" },                                                               // LC_VIEW_TIME_STOP
		{ "Play",                             "Play animation" },                                                                       // LC_VIEW_TIME_PLAY
		{ "Insert",                           "Insert new step" },                                                                      // LC_VIEW_TIME_INSERT
		{ "Delete",                           "Delete current step" },                                                                  // LC_VIEW_TIME_DELETE
		{ "Animation",                        "Toggle between animation and instruction mode" },                                        // LC_VIEW_TIME_ANIMATION
		{ "Add Keys",                         "Toggle adding new animation keys" },                                                     // LC_VIEW_TIME_ADD_KEYS
		{ "Insert",                           "Add a new piece to the model" },                                                         // LC_PIECE_INSERT
		{ "Delete",                           "Delete selected objects" },                                                              // LC_PIECE_DELETE
		{ "Minifig Wizard...",                "Add a new minifig to the model" },                                                       // LC_PIECE_MINIFIG_WIZARD
		{ "Array...",                         "Make copies of the selected pieces" },                                                   // LC_PIECE_ARRAY
		{ "Copy Keys",                        "Copy keys between animation and instruction modes" },                                    // LC_PIECE_COPY_KEYS
		{ "Group...",                         "Group selected pieces together" },                                                       // LC_PIECE_GROUP
		{ "Ungroup",                          "Ungroup selected group" },                                                               // LC_PIECE_UNGROUP
		{ "Add to Group",                     "Add focused piece to selected group" },                                                  // LC_PIECE_GROUP_ADD
		{ "Remove from Group",                "Remove focused piece from group" },                                                      // LC_PIECE_GROUP_REMOVE
		{ "Edit Groups...",                   "Edit groups" },                                                                          // LC_PIECE_GROUP_EDIT
		{ "Hide Selected",                    "Hide selected objects" },                                                                // LC_PIECE_HIDE_SELECTED
		{ "Hide Unselected",                  "Hide objects that are not selected" },                                                   // LC_PIECE_HIDE_UNSELECTED
		{ "Unhide All",                       "Show all hidden objects" },                                                              // LC_PIECE_UNHIDE_ALL
		{ "Show Earlier",                     "Show selected pieces one step earlier" },                                                // LC_PIECE_SHOW_EARLIER
		{ "Show Later",                       "Show selected pieces one step later" },                                                  // LC_PIECE_SHOW_LATER
		{ "About...",                         "Display program version number and system information" },                                // LC_HELP_ABOUT
	};

	LC_CASSERT(sizeof(actionStrings)/sizeof(actionStrings[0]) == LC_NUM_COMMANDS);

	for (int Command = 0; Command < LC_NUM_COMMANDS; Command++)
	{
		QAction *action = new QAction(tr(actionStrings[Command][0]), this);
		action->setStatusTip(tr(actionStrings[Command][1]));
		connect(action, SIGNAL(triggered()), this, SLOT(actionTriggered()));
		actions[Command] = action;
	}

	actions[LC_FILE_NEW]->setToolTip(tr("New Project"));
	actions[LC_FILE_OPEN]->setToolTip(tr("Open Project"));
	actions[LC_FILE_SAVE]->setToolTip(tr("Save Project"));

	actions[LC_FILE_NEW]->setIcon(QIcon(":/resources/file_new.png"));
	actions[LC_FILE_OPEN]->setIcon(QIcon(":/resources/file_open.png"));
	actions[LC_FILE_SAVE]->setIcon(QIcon(":/resources/file_save.png"));
	actions[LC_FILE_PRINT]->setIcon(QIcon(":/resources/file_print.png"));
	actions[LC_FILE_PRINT_PREVIEW]->setIcon(QIcon(":/resources/file_print_preview.png"));
	actions[LC_EDIT_UNDO]->setIcon(QIcon(":/resources/edit_undo.png"));
	actions[LC_EDIT_REDO]->setIcon(QIcon(":/resources/edit_redo.png"));
	actions[LC_EDIT_CUT]->setIcon(QIcon(":/resources/edit_cut.png"));
	actions[LC_EDIT_COPY]->setIcon(QIcon(":/resources/edit_copy.png"));
	actions[LC_EDIT_PASTE]->setIcon(QIcon(":/resources/edit_paste.png"));
	actions[LC_EDIT_LOCK_TOGGLE]->setIcon(QIcon(":/resources/edit_lock.png"));
	actions[LC_EDIT_SNAP_TOGGLE]->setIcon(QIcon(":/resources/edit_snap_move.png"));
	actions[LC_EDIT_SNAP_ANGLE]->setIcon(QIcon(":/resources/edit_snap_angle.png"));
	actions[LC_EDIT_TRANSFORM]->setIcon(QIcon(":/resources/edit_transform.png"));
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
	actions[LC_VIEW_TIME_FIRST]->setIcon(QIcon(":/resources/time_first.png"));
	actions[LC_VIEW_TIME_PREVIOUS]->setIcon(QIcon(":/resources/time_previous.png"));
	actions[LC_VIEW_TIME_NEXT]->setIcon(QIcon(":/resources/time_next.png"));
	actions[LC_VIEW_TIME_LAST]->setIcon(QIcon(":/resources/time_last.png"));

	actions[LC_FILE_NEW]->setShortcuts(QKeySequence::New);
	actions[LC_FILE_OPEN]->setShortcuts(QKeySequence::Open);
	actions[LC_FILE_SAVE]->setShortcuts(QKeySequence::Save);
	actions[LC_FILE_SAVEAS]->setShortcuts(QKeySequence::SaveAs);
	actions[LC_EDIT_UNDO]->setShortcuts(QKeySequence::Undo);
	actions[LC_EDIT_REDO]->setShortcuts(QKeySequence::Redo);
	actions[LC_EDIT_CUT]->setShortcuts(QKeySequence::Cut);
	actions[LC_EDIT_COPY]->setShortcuts(QKeySequence::Copy);
	actions[LC_EDIT_PASTE]->setShortcuts(QKeySequence::Paste);

	actions[LC_EDIT_LOCK_X]->setCheckable(true);
	actions[LC_EDIT_LOCK_Y]->setCheckable(true);
	actions[LC_EDIT_LOCK_Z]->setCheckable(true);
	actions[LC_EDIT_SNAP_X]->setCheckable(true);
	actions[LC_EDIT_SNAP_Y]->setCheckable(true);
	actions[LC_EDIT_SNAP_Z]->setCheckable(true);
	actions[LC_EDIT_SNAP_ANGLE]->setCheckable(true);
	actions[LC_VIEW_CAMERA_NONE]->setCheckable(true);

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
}

void lcQMainWindow::createMenus()
{
	QMenu* lockMenu = new QMenu(tr("Lock Menu"));
	lockMenu->addAction(actions[LC_EDIT_LOCK_X]);
	lockMenu->addAction(actions[LC_EDIT_LOCK_Y]);
	lockMenu->addAction(actions[LC_EDIT_LOCK_Z]);
	lockMenu->addAction(actions[LC_EDIT_LOCK_NONE]);
	actions[LC_EDIT_LOCK_TOGGLE]->setMenu(lockMenu);

	QMenu* snapMenu = new QMenu(tr("Snap Menu"));
	snapMenu->addAction(actions[LC_EDIT_SNAP_X]);
	snapMenu->addAction(actions[LC_EDIT_SNAP_Y]);
	snapMenu->addAction(actions[LC_EDIT_SNAP_Z]);
	snapMenu->addAction(actions[LC_EDIT_SNAP_NONE]);
	snapMenu->addAction(actions[LC_EDIT_SNAP_ALL]);
	actions[LC_EDIT_SNAP_TOGGLE]->setMenu(snapMenu);

	QMenu* transformMenu = new QMenu(tr("Transform"));
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION]);
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION]);
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION]);
	transformMenu->addAction(actions[LC_EDIT_TRANSFORM_RELATIVE_ROTATION]);
	actions[LC_EDIT_TRANSFORM]->setMenu(transformMenu);

	menuCamera = new QMenu(tr("Cameras"));
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
	menuView->addMenu(menuCamera);

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
	menuPiece->addSeparator();
	menuPiece->addAction(actions[LC_PIECE_HIDE_SELECTED]);
	menuPiece->addAction(actions[LC_PIECE_HIDE_UNSELECTED]);
	menuPiece->addAction(actions[LC_PIECE_UNHIDE_ALL]);

	menuHelp = menuBar()->addMenu(tr("&Help"));
	menuHelp->addAction(actions[LC_HELP_ABOUT]);
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

	partsToolBar = new QDockWidget(tr("Parts"), this);
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

	piecePreview = new lcGLWidget(previewFrame, centralWidget, new PiecePreview(NULL));
	piecePreview->preferredSize = QSize(200, 100);
	previewLayout->addWidget(piecePreview, 0, 0, 1, 1);

	partsTree = new lcQPartsTree(partsSplitter);
	partsTree->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	connect(partsTree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(partsTreeItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));

	pieceCombo = new QComboBox(partsSplitter);
	pieceCombo->setEditable(true);

	QFrame *colorFrame = new QFrame(partsSplitter);
	colorFrame->setFrameShape(QFrame::StyledPanel);
	colorFrame->setFrameShadow(QFrame::Sunken);

	QGridLayout *colorLayout = new QGridLayout(colorFrame);
	colorLayout->setContentsMargins(0, 0, 0, 0);

	colorList = new lcColorListWidget(partsSplitter);
	colorLayout->addWidget(colorList);

	partsLayout->addWidget(partsSplitter, 0, 0, 1, 1);

	partsToolBar->setWidget(partsContents);
	addDockWidget(static_cast<Qt::DockWidgetArea>(2), partsToolBar);
}

void lcQMainWindow::createStatusBar()
{
	statusBar = new QStatusBar(this);
	setStatusBar(statusBar);

	statusPositionLabel = new QLabel("XYZ");
	statusBar->addPermanentWidget(statusPositionLabel);

	statusSnapLabel = new QLabel();
	statusBar->addPermanentWidget(statusSnapLabel);

	statusTimeLabel = new QLabel();
	statusBar->addPermanentWidget(statusTimeLabel);
}

void lcQMainWindow::closeEvent(QCloseEvent *event)
{
	if (!lcGetActiveProject()->SaveModified())
		event->ignore();
	else
		event->accept();
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
	PieceInfo *info = (PieceInfo*)current->data(0, lcQPartsTree::PartInfoRole).value<void*>();

	if (info)
	{
		lcGetActiveProject()->SetCurrentPiece(info);
		PiecePreview* Preview = (PiecePreview*)piecePreview->mWindow;
		Preview->OnInitialUpdate();
		Preview->SetCurrentPiece(info);
	}
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

void lcQMainWindow::updateTime(bool animation, int currentTime, int totalTime)
{
	actions[LC_VIEW_TIME_FIRST]->setEnabled(currentTime != 1);
	actions[LC_VIEW_TIME_PREVIOUS]->setEnabled(currentTime > 1);
	actions[LC_VIEW_TIME_NEXT]->setEnabled(currentTime < totalTime);
	actions[LC_VIEW_TIME_LAST]->setEnabled(currentTime != totalTime);

	if (animation)
		statusTimeLabel->setText(QString(tr(" %1 / %2 ")).arg(QString::number(currentTime), QString::number(totalTime)));
	else
		statusTimeLabel->setText(QString(tr(" Step %1 ")).arg(QString::number(currentTime)));
}

void lcQMainWindow::updateAnimation(bool animation, bool addKeys)
{
	/*
	gtk_widget_set_sensitive (anim_toolbar.play, bAnimation);
	gtk_widget_set_sensitive (anim_toolbar.stop, FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(anim_toolbar.anim), bAnimation);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(anim_toolbar.keys), bAddKeys);
	gpointer item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_copykeys");
	gtk_label_set_text (GTK_LABEL (GTK_BIN (item)->child), bAnimation ? "Copy Keys from Instructions" : "Copy Keys from Animation");
	*/
}

void lcQMainWindow::updateLockSnap(lcuint32 snap)
{
	actions[LC_EDIT_SNAP_ANGLE]->setChecked((snap & LC_DRAW_SNAP_A) != 0);

	actions[LC_EDIT_SNAP_X]->setChecked((snap & LC_DRAW_SNAP_X) != 0);
	actions[LC_EDIT_SNAP_Y]->setChecked((snap & LC_DRAW_SNAP_Y) != 0);
	actions[LC_EDIT_SNAP_Z]->setChecked((snap & LC_DRAW_SNAP_Z) != 0);

	actions[LC_EDIT_LOCK_X]->setChecked((snap & LC_DRAW_LOCK_X) != 0);
	actions[LC_EDIT_LOCK_Y]->setChecked((snap & LC_DRAW_LOCK_Y) != 0);
	actions[LC_EDIT_LOCK_Z]->setChecked((snap & LC_DRAW_LOCK_Z) != 0);
}

void lcQMainWindow::updateSnap(lcuint16 moveSnap, lcuint16 rotateSnap)
{
	char xy[32], z[32];

	lcGetActiveProject()->GetSnapDistanceText(xy, z);

	statusSnapLabel->setText(QString(tr(" M: %1 %2 R: %3 ")).arg(xy, z, QString::number(rotateSnap)));
}

void lcQMainWindow::updateUndoRedo(const char* undoText, const char* redoText)
{
	QAction *undoAction = actions[LC_EDIT_UNDO];
	QAction *redoAction = actions[LC_EDIT_REDO];

	if (undoText)
	{
		undoAction->setEnabled(true);
		undoAction->setText(QString(tr("Undo %1")).arg(undoText));
	}
	else
	{
		undoAction->setEnabled(false);
		undoAction->setText(tr("Undo"));
	}

	if (redoText)
	{
		redoAction->setEnabled(true);
		redoAction->setText(QString(tr("Redo %1")).arg(redoText));
	}
	else
	{
		redoAction->setEnabled(false);
		redoAction->setText(tr("Redo"));
	}
}

void lcQMainWindow::updateCameraMenu(const PtrArray<Camera>& cameras, Camera* currentCamera)
{
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

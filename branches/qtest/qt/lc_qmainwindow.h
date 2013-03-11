#ifndef _LC_QMAINWINDOW_H_
#define _LC_QMAINWINDOW_H_

#include <QMainWindow>
#include "typedefs.h"
#include "array.h"

class QComboBox;
class lcGLWidget;
class lcQPartsTree;
class lcColorListWidget;

class lcQMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit lcQMainWindow(QWidget *parent = 0);
	~lcQMainWindow();

	void updateAction(int newAction);
	void updatePaste(bool enabled);
	void updateTime(bool animation, int currentTime, int totalTime);
	void updateAnimation(bool animation, bool addKeys);
	void updateLockSnap(lcuint32 snap);
	void updateSnap(lcuint16 moveSnap, lcuint16 rotateSnap);
	void updateUndoRedo(const char* undoText, const char* redoText);
	void updateCameraMenu(const PtrArray<Camera>& cameras, Camera* currentCamera);
	void updateCurrentCamera(int cameraIndex);
	void updateTitle(const char* title, bool modified);
	void updateModified(bool modified);
	void updateRecentFiles(const char** fileNames);

private slots:
	void actionTriggered();
	void partsTreeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();

	lcGLWidget *centralWidget;
	QAction *actions[LC_NUM_COMMANDS];

	QAction *actionFileRecentSeparator;
	QMenu *menuCamera;

	QMenu *menuFile;
	QMenu *menuEdit;
	QMenu *menuView;
	QMenu *menuPiece;
	QMenu *menuHelp;

	QToolBar *standardToolBar;
	QToolBar *toolsToolBar;
	QToolBar *timeToolBar;
	QDockWidget *partsToolBar;

	lcGLWidget *piecePreview;
	lcQPartsTree *partsTree;
	QComboBox *pieceCombo;
	lcColorListWidget *colorList;
	QLineEdit *transformX;
	QLineEdit *transformY;
	QLineEdit *transformZ;

	QStatusBar *statusBar;
	QLabel* statusPositionLabel;
	QLabel* statusSnapLabel;
	QLabel* statusTimeLabel;
};

#endif // _LC_QMAINWINDOW_H_

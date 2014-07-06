#ifndef _LC_QMAINWINDOW_H_
#define _LC_QMAINWINDOW_H_

#include <QMainWindow>
#include <QPrinter>
#include "lc_array.h"
#include "lc_commands.h"

class QComboBox;
class lcQGLWidget;
class lcQPartsTree;
class lcQColorList;
class lcQPropertiesTree;
class View;

class lcQMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit lcQMainWindow(QWidget *parent = 0);
	~lcQMainWindow();

	void showPrintDialog();

	void splitHorizontal();
	void splitVertical();
	void removeView();
	void resetViews();
	void togglePrintPreview();
	void toggleFullScreen();

	void updateFocusObject(Object *focus);
	void updateSelectedObjects(int flags, int selectedCount, Object* focus);
	void updateAction(int newAction);
	void updatePaste(bool enabled);
	void updateCurrentStep();
	void setAddKeys(bool addKeys);
	void updateLockSnap(lcuint32 snap);
	void updateSnap();
	void updateUndoRedo(const char* undoText, const char* redoText);
	void updateTransformType(int newType);
	void updateCameraMenu();
	void updateCurrentCamera(int cameraIndex);
	void updatePerspective(View* view);
	void updateCategories();
	void updateTitle(const char* title, bool modified);
	void updateModified(bool modified);
	void updateRecentFiles(const char** fileNames);
	void updateShortcuts();

	lcVector3 getTransformAmount();

	QAction *actions[LC_NUM_COMMANDS];

private slots:
	void print(QPrinter *printer);
	void actionTriggered();
	void partsTreeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void colorChanged(int colorIndex);
	void partSearchReturn();
	void partSearchChanged(const QString& text);
	void clipboardChanged();

private:
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();
	void splitView(Qt::Orientation orientation);

	void closeEvent(QCloseEvent *event);
	QMenu *createPopupMenu();

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
	QDockWidget *propertiesToolBar;

	lcQGLWidget *piecePreview;
	lcQPartsTree *partsTree;
	QLineEdit *partSearch;
	lcQColorList *colorList;
	lcQPropertiesTree *propertiesWidget;
	QLineEdit *transformX;
	QLineEdit *transformY;
	QLineEdit *transformZ;

	QStatusBar *statusBar;
	QLabel *statusBarLabel;
	QLabel *statusPositionLabel;
	QLabel *statusSnapLabel;
	QLabel *statusTimeLabel;
};

#endif // _LC_QMAINWINDOW_H_

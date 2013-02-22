#ifndef _LC_QMAINWINDOW_H_
#define _LC_QMAINWINDOW_H_

#include <QMainWindow>
#include "typedefs.h"
#include "mainwnd.h"

class QComboBox;
class QTreeWidget;
class lcViewWidget;
class lcPreviewWidget;
class lcColorListWidget;

class lcQMainWindow : public QMainWindow, public lcMainWindow
{
	Q_OBJECT

public:
	explicit lcQMainWindow(QWidget *parent = 0);
	~lcQMainWindow();

	void UpdateAction(int NewAction);

private slots:
	void actionTriggered();

private:
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();

	lcViewWidget *centralWidget;
	QAction *actions[LC_NUM_COMMANDS];

	QAction *actionFileRecentSeparator;
	QMenu *menuFile;
	QMenu *menuEdit;
	QMenu *menuView;
	QMenu *menuPiece;
	QMenu *menuHelp;

	QToolBar *standardToolBar;
	QToolBar *toolsToolBar;
	QToolBar *timeToolBar;
	QDockWidget *piecesToolBar;

	lcPreviewWidget *piecePreview;
	QTreeWidget *pieceList;
	QComboBox *pieceCombo;
	lcColorListWidget *colorList;
	QLineEdit *transformX;
	QLineEdit *transformY;
	QLineEdit *transformZ;

	QStatusBar *statusBar;
};

#endif // _LC_QMAINWINDOW_H_

#ifndef _LC_MAINWINDOW_H_
#define _LC_MAINWINDOW_H_

#include <QMainWindow>
#include "typedefs.h"

class QComboBox;
class QTreeWidget;
class lcViewWidget;
class lcPreviewWidget;
class lcColorListWidget;

class lcMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit lcMainWindow(QWidget *parent = 0);
	~lcMainWindow();

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

#endif // _LC_MAINWINDOW_H_

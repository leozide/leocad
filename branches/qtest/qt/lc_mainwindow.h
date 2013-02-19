#ifndef _LC_MAINWINDOW_H_
#define _LC_MAINWINDOW_H_

#include <QMainWindow>

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
	void toolTriggered(QAction *action);

private:
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();

	lcViewWidget *centralWidget;

	QAction *actionFileNew;
	QAction *actionFileOpen;
	QAction *actionFileMerge;
	QAction *actionFileSave;
	QAction *actionFileSaveAs;
	QAction *actionFileSaveImage;
	QAction *actionFileExport3DS;
	QAction *actionFileExportBrickLink;
	QAction *actionFileExportHTML;
	QAction *actionFileExportPOVRay;
	QAction *actionFileExportWavefront;
	QAction *actionFileProperties;
	QAction *actionFilePiecesLibrary;
	QAction *actionFileTerrainEditor;
	QAction *actionFilePrint;
	QAction *actionFilePrintPreview;
	QAction *actionFilePrintBOM;
	QAction *actionFileRecent1;
	QAction *actionFileRecent2;
	QAction *actionFileRecent3;
	QAction *actionFileRecent4;
	QAction *actionFileRecentSeparator;
	QAction *actionFileExit;
	QAction *actionEditUndo;
	QAction *actionEditRedo;
	QAction *actionEditCut;
	QAction *actionEditCopy;
	QAction *actionEditPaste;
	QAction *actionEditSelectAll;
	QAction *actionEditSelectNone;
	QAction *actionEditSelectInvert;
	QAction *actionEditSelectByName;
	QAction *actionToolInsert;
	QAction *actionToolLight;
	QAction *actionToolSpotLight;
	QAction *actionToolCamera;
	QAction *actionToolSelect;
	QAction *actionToolMove;
	QAction *actionToolRotate;
	QAction *actionToolDelete;
	QAction *actionToolPaint;
	QAction *actionToolZoom;
	QAction *actionToolPan;
	QAction *actionToolRotateView;
	QAction *actionToolRoll;
	QAction *actionToolZoomRegion;

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

	QStatusBar *statusBar;
};

#endif // _LC_MAINWINDOW_H_

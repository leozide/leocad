#pragma once

#include "lc_instructions.h"

class lcInstructionsPageWidget : public QGraphicsView
{
	Q_OBJECT

public:
	lcInstructionsPageWidget(QWidget* Parent);

	void SetCurrentPage(const lcInstructionsPage* Page);
};

class lcInstructionsPageListWidget : public QDockWidget
{
	Q_OBJECT

public:
	lcInstructionsPageListWidget(QWidget* Parent);

//protected:
	QListWidget* mThumbnailsWidget = nullptr;
};

class lcInstructionsDialog : public QMainWindow
{
	Q_OBJECT

public:
	lcInstructionsDialog(QWidget* Parent, Project* Project);

protected slots:
	void UpdatePageSettings();
	void CurrentThumbnailChanged(int Index);

protected:
	Project* mProject = nullptr;

	int mCurrentPageNumber;
	lcInstructions mInstructions;

	lcInstructionsPageWidget* mPageWidget = nullptr;
	lcInstructionsPageListWidget* mPageListWidget = nullptr;

	QToolBar* mPageSettingsToolBar = nullptr;
	QAction* mVerticalPageAction = nullptr;
	QAction* mHorizontalPageAction = nullptr;
	QSpinBox* mRowsSpinBox = nullptr;
	QSpinBox* mColumnsSpinBox = nullptr;
};

#pragma once

struct lcInstructionsPageLayout;

class lcInstructionsPageWidget : public QGraphicsView
{
	Q_OBJECT

public:
	lcInstructionsPageWidget(QWidget* Parent, Project* Project);

	void SetCurrentPage(lcInstructionsPageLayout* PageLayout);
};

class lcInstructionsDialog : public QMainWindow
{
	Q_OBJECT

public:
	lcInstructionsDialog(QWidget* Parent, Project* Project);

protected slots:
	void CurrentThumbnailChanged(int Index);

protected:
	Project* mProject;

	int mCurrentPageNumber;
	std::vector<lcInstructionsPageLayout> mPageLayouts;

	QListWidget* mThumbnailsWidget;
	lcInstructionsPageWidget* mPageWidget;
};

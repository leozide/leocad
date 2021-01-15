#pragma once

#include "lc_instructions.h"

class lcInstructionsPropertiesWidget;

class lcInstructionsStepImageItem : public QGraphicsPixmapItem
{
public:
	lcInstructionsStepImageItem(const QPixmap& Pixmap, QGraphicsItem* Parent, lcModel* Model, lcStep Step);

	lcModel* GetModel() const
	{
		return mModel;
	}

	lcStep GetStep() const
	{
		return mStep;
	}

protected:
	lcModel* mModel = nullptr;
	lcStep mStep = 1;
};

class lcInstructionsStepNumberItem : public QGraphicsSimpleTextItem
{
public:
	lcInstructionsStepNumberItem(const QString& Text, QGraphicsItem* Parent, lcModel* Model, lcStep Step);

	lcModel* GetModel() const
	{
		return mModel;
	}

	lcStep GetStep() const
	{
		return mStep;
	}

protected:
	lcModel* mModel = nullptr;
	lcStep mStep = 1;
};

class lcInstructionsPageWidget : public QGraphicsView
{
	Q_OBJECT

public:
	lcInstructionsPageWidget(QWidget* Parent, lcInstructions* Instructions, lcInstructionsPropertiesWidget* PropertiesWidget);

	void SetCurrentPage(const lcInstructionsPage* Page);

protected slots:
	void SelectionChanged();

protected:
	lcInstructions* mInstructions;
	lcInstructionsPropertiesWidget* mPropertiesWidget;
};

class lcInstructionsPageListWidget : public QDockWidget
{
	Q_OBJECT

public:
	lcInstructionsPageListWidget(QWidget* Parent, lcInstructions* Instructions);

protected slots:
	void ShowPageSetupDialog();

public:
//protected:
//	QComboBox* mSizeComboBox = nullptr;
//	QLineEdit* mWidthEdit = nullptr;
//	QLineEdit* mHeightEdit = nullptr;
//
//	QRadioButton* mPortraitButton = nullptr;
//	QRadioButton* mLandscapeButton = nullptr;
//
//	QLineEdit* mLeftMarginEdit = nullptr;
//	QLineEdit* mRightMarginEdit = nullptr;
//	QLineEdit* mTopMarginEdit = nullptr;
//	QLineEdit* mBottomMarginEdit = nullptr;
//
//	QComboBox* mUnitsComboBox = nullptr;

	QListWidget* mThumbnailsWidget = nullptr;

protected:
	lcInstructions* mInstructions;
};

class lcInstructionsPropertiesWidget : public QDockWidget
{
	Q_OBJECT

public:
	lcInstructionsPropertiesWidget(QWidget* Parent, lcInstructions* Instructions);

	void SelectionChanged(QGraphicsItem* FocusItem);

protected slots:
	void ColorButtonClicked();

protected:
	void StepImageItemFocusIn(lcInstructionsStepImageItem* ImageItem);
	void StepNumberItemFocusIn(lcInstructionsStepNumberItem* NumberItem);

	lcCollapsibleWidget* mWidget = nullptr;
	lcInstructions* mInstructions = nullptr;
	QGraphicsItem* mFocusItem = nullptr;
};

class lcInstructionsDialog : public QMainWindow
{
	Q_OBJECT

public:
	lcInstructionsDialog(QWidget* Parent, Project* Project);

protected slots:
	void UpdatePageSettings();
	void CurrentThumbnailChanged(int Index);
	void PageInvalid(int PageIndex);

protected:
	Project* mProject = nullptr;

	int mCurrentPageNumber;
	lcInstructions* mInstructions;

	lcInstructionsPageWidget* mPageWidget = nullptr;
	lcInstructionsPageListWidget* mPageListWidget = nullptr;
	lcInstructionsPropertiesWidget* mPropertiesWidget = nullptr;

	QToolBar* mPageSettingsToolBar = nullptr;
	QAction* mVerticalPageAction = nullptr;
	QAction* mHorizontalPageAction = nullptr;
	QSpinBox* mRowsSpinBox = nullptr;
	QSpinBox* mColumnsSpinBox = nullptr;
};

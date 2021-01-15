#pragma once

#include "lc_instructions.h"

class lcInstructionsPropertiesWidget;

class lcInstructionsStepImageItem : public QGraphicsPixmapItem
{
public:
	lcInstructionsStepImageItem(const QPixmap& Pixmap, QGraphicsItem* Parent, lcModel* Model, lcStep Step, lcInstructionsPropertiesWidget* PropertiesWidget);

	lcModel* GetModel() const
	{
		return mModel;
	}

	lcStep GetStep() const
	{
		return mStep;
	}

protected:
	void focusInEvent(QFocusEvent* FocusEvent) override;
	void focusOutEvent(QFocusEvent* FocusEvent) override;

	lcModel* mModel = nullptr;
	lcStep mStep = 1;
	lcInstructionsPropertiesWidget* mPropertiesWidget = nullptr;
};

class lcInstructionsStepNumberItem : public QGraphicsSimpleTextItem
{
public:
	lcInstructionsStepNumberItem(const QString& Text, QGraphicsItem* Parent, lcModel* Model, lcStep Step, lcInstructionsPropertiesWidget* PropertiesWidget);

	lcModel* GetModel() const
	{
		return mModel;
	}

	lcStep GetStep() const
	{
		return mStep;
	}

protected:
	void focusInEvent(QFocusEvent* FocusEvent) override;
	void focusOutEvent(QFocusEvent* FocusEvent) override;

	lcModel* mModel = nullptr;
	lcStep mStep = 1;
	lcInstructionsPropertiesWidget* mPropertiesWidget = nullptr;
};

class lcInstructionsPageWidget : public QGraphicsView
{
	Q_OBJECT

public:
	lcInstructionsPageWidget(QWidget* Parent, lcInstructions* Instructions);

	void SetCurrentPage(const lcInstructionsPage* Page, lcInstructionsPropertiesWidget* PropertiesWidget);

protected:
	lcInstructions* mInstructions;
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

	void StepImageItemFocusIn(lcInstructionsStepImageItem* ImageItem);
	void StepNumberItemFocusIn(lcInstructionsStepNumberItem* NumberItem);
	void ItemFocusOut(QGraphicsItem* Item);

protected slots:
	void ColorButtonClicked();

protected:
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

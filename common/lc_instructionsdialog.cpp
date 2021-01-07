#include "lc_global.h"
#include "lc_instructionsdialog.h"
#include "lc_collapsiblewidget.h"
#include "project.h"
#include "lc_model.h"
#include "lc_qutils.h"

lcInstructionsPageWidget::lcInstructionsPageWidget(QWidget* Parent)
	: QGraphicsView(Parent)
{
}

void lcInstructionsPageWidget::SetCurrentPage(const lcInstructionsPage* Page)
{
	QGraphicsScene* Scene = new QGraphicsScene(this);
	setScene(Scene);
	Scene->setSceneRect(0, 0, 1000, 1000);
//	Scene->setBackgroundBrush(Qt::black);

	if (Page)
	{
		for (const lcInstructionsStep& Step : Page->Steps)
		{
			QImage StepImage = Step.Model->GetStepImage(false, 500, 500, Step.Step);
			QGraphicsPixmapItem* StepImageItem = Scene->addPixmap(QPixmap::fromImage(StepImage));
			StepImageItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
			StepImageItem->setPos(1000.0 * Step.Rect.x(), 1000.0 * Step.Rect.y());

			QGraphicsSimpleTextItem* StepNumberItem = Scene->addSimpleText(QString::number(Step.Step), QFont("Helvetica", 96));
			StepNumberItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
			StepNumberItem->setPos(1000.0 * Step.Rect.x(), 1000.0 * Step.Rect.y());

			QImage PartsImage = Step.Model->GetPartsListImage(300, Step.Step);
			QGraphicsPixmapItem* PartsImageItem = Scene->addPixmap(QPixmap::fromImage(PartsImage));
			PartsImageItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
			PartsImageItem->setPos(StepNumberItem->pos() + QPointF(StepNumberItem->sceneBoundingRect().width(), 0));
		}
	}
}

lcInstructionsPageListWidget::lcInstructionsPageListWidget(QWidget* Parent)
	: QDockWidget(Parent)
{
	QWidget* CentralWidget = new QWidget(this);
	setWidget(CentralWidget);
	setWindowTitle(tr("Pages"));

	QVBoxLayout* Layout = new QVBoxLayout(CentralWidget);
	Layout->setContentsMargins(0, 0, 0, 0);

	lcCollapsibleWidget* SetupWidget = new lcCollapsibleWidget(tr("Page Setup"), CentralWidget);
	Layout->addWidget(SetupWidget);

	QVBoxLayout* SetupLayout = new QVBoxLayout();
	SetupWidget->SetChildLayout(SetupLayout);

	lcCollapsibleWidget* SizeWidget = new lcCollapsibleWidget(tr("Size"));
	SetupLayout->addWidget(SizeWidget);

	QGridLayout* SizeLayout = new QGridLayout();
	SizeWidget->SetChildLayout(SizeLayout);

	mSizeComboBox = new QComboBox();
	SizeLayout->addWidget(mSizeComboBox, 0, 0, 1, -1);

	mWidthEdit = new lcSmallLineEdit();
	SizeLayout->addWidget(new QLabel(tr("Width:")), 1, 0);
	SizeLayout->addWidget(mWidthEdit, 1, 1);

	mHeightEdit = new lcSmallLineEdit();
	SizeLayout->addWidget(new QLabel(tr("Height:")), 1, 2);
	SizeLayout->addWidget(mHeightEdit, 1, 3);

	lcCollapsibleWidget* OrientationWidget = new lcCollapsibleWidget(tr("Orientation"));
	SetupLayout->addWidget(OrientationWidget);

	QVBoxLayout* OrientationLayout = new QVBoxLayout();
	OrientationWidget->SetChildLayout(OrientationLayout);

	mPortraitButton = new QRadioButton(tr("Portrait"));
	OrientationLayout->addWidget(mPortraitButton);
	mLandscapeButton = new QRadioButton(tr("Landscape"));
	OrientationLayout->addWidget(mLandscapeButton);

	lcCollapsibleWidget* MarginsWidget = new lcCollapsibleWidget(tr("Margins"));
	SetupLayout->addWidget(MarginsWidget);

	QGridLayout* MarginsLayout = new QGridLayout();
	MarginsWidget->SetChildLayout(MarginsLayout);

	mLeftMarginEdit = new lcSmallLineEdit();
	MarginsLayout->addWidget(new QLabel(tr("Left:")), 0, 0);
	MarginsLayout->addWidget(mLeftMarginEdit, 0, 1);

	mRightMarginEdit = new lcSmallLineEdit();
	MarginsLayout->addWidget(new QLabel(tr("Right:")), 0, 2);
	MarginsLayout->addWidget(mRightMarginEdit, 0, 3);

	mTopMarginEdit = new lcSmallLineEdit();
	MarginsLayout->addWidget(new QLabel(tr("Top:")), 1, 0);
	MarginsLayout->addWidget(mTopMarginEdit, 1, 1);

	mBottomMarginEdit = new lcSmallLineEdit();
	MarginsLayout->addWidget(new QLabel(tr("Bottom:")), 1, 2);
	MarginsLayout->addWidget(mBottomMarginEdit, 1, 3);

	lcCollapsibleWidget* UnitsWidget = new lcCollapsibleWidget(tr("Units"));
	SetupLayout->addWidget(UnitsWidget);

	QVBoxLayout* UnitsLayout = new QVBoxLayout();
	UnitsWidget->SetChildLayout(UnitsLayout);

	mUnitsComboBox = new QComboBox();
	mUnitsComboBox->addItems(QStringList() << tr("Pixels") << tr("Centimeters") << tr("Inches"));
	UnitsLayout->addWidget(mUnitsComboBox);

	SetupWidget->Collapse();

	mThumbnailsWidget = new QListWidget(CentralWidget);
	Layout->addWidget(mThumbnailsWidget);
}

lcInstructionsDialog::lcInstructionsDialog(QWidget* Parent, Project* Project)
	: QMainWindow(Parent), mProject(Project)
{
	setWindowTitle(tr("Instructions"));

	mPageWidget = new lcInstructionsPageWidget(this);
	setCentralWidget(mPageWidget);

	mPageListWidget = new lcInstructionsPageListWidget(this);
	mPageListWidget->setObjectName("PageList");
	addDockWidget(Qt::LeftDockWidgetArea, mPageListWidget);

	mInstructions = mProject->GetInstructions();

	mPageSettingsToolBar = addToolBar(tr("Page Settings"));
	mPageSettingsToolBar->setObjectName("PageSettings");
	mPageSettingsToolBar->setFloatable(false);
	mPageSettingsToolBar->setMovable(false);

	mVerticalPageAction = mPageSettingsToolBar->addAction("Vertical");
	mVerticalPageAction->setCheckable(true);
	mHorizontalPageAction = mPageSettingsToolBar->addAction("Horizontal");
	mHorizontalPageAction->setCheckable(true);

	mRowsSpinBox = new QSpinBox(mPageSettingsToolBar);
	mPageSettingsToolBar->addWidget(mRowsSpinBox);

	mColumnsSpinBox = new QSpinBox(mPageSettingsToolBar);
	mPageSettingsToolBar->addWidget(mColumnsSpinBox);

	QActionGroup* PageDirectionGroup = new QActionGroup(mPageSettingsToolBar);
	PageDirectionGroup->addAction(mVerticalPageAction);
	PageDirectionGroup->addAction(mHorizontalPageAction);

	for (size_t PageNumber = 0; PageNumber < mInstructions.mPages.size(); PageNumber++)
		mPageListWidget->mThumbnailsWidget->addItem(QString("Page %1").arg(PageNumber + 1));

	connect(mPageListWidget->mThumbnailsWidget, SIGNAL(currentRowChanged(int)), this, SLOT(CurrentThumbnailChanged(int)));
	mPageListWidget->mThumbnailsWidget->setCurrentRow(0);

	connect(mVerticalPageAction, SIGNAL(toggled(bool)), this, SLOT(UpdatePageSettings()));
	connect(mHorizontalPageAction, SIGNAL(toggled(bool)), this, SLOT(UpdatePageSettings()));
	connect(mRowsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(UpdatePageSettings()));
	connect(mColumnsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(UpdatePageSettings()));
}

void lcInstructionsDialog::UpdatePageSettings()
{
	lcInstructionsPageSettings PageSettings;

	PageSettings.Rows = mRowsSpinBox->value();
	PageSettings.Columns = mColumnsSpinBox->value();

	if (mHorizontalPageAction->isChecked())
		PageSettings.Direction = lcInstructionsDirection::Horizontal;
	else
		PageSettings.Direction = lcInstructionsDirection::Vertical;

	mInstructions.SetDefaultPageSettings(PageSettings);
//	lcInstructionsPage* Page = &mInstructions.mPages[mThumbnailsWidget->currentIndex().row()];

	mPageListWidget->mThumbnailsWidget->clear();
	for (size_t PageNumber = 0; PageNumber < mInstructions.mPages.size(); PageNumber++)
		mPageListWidget->mThumbnailsWidget->addItem(QString("Page %1").arg(PageNumber + 1));

//	mThumbnailsWidget->setCurrentRow(0);

//	mPageWidget->SetCurrentPage(Page);
}

void lcInstructionsDialog::CurrentThumbnailChanged(int Index)
{
	if (Index < 0 || Index >= static_cast<int>(mInstructions.mPages.size()))
	{
		mPageWidget->SetCurrentPage(nullptr);
		return;
	}

	const lcInstructionsPage* Page = &mInstructions.mPages[Index];
//	const lcInstructionsPageSettings& PageSettings = Page->Settings;
	const lcInstructionsPageSettings& PageSettings = mInstructions.mPageSettings;

	mPageWidget->SetCurrentPage(Page);

	if (PageSettings.Direction == lcInstructionsDirection::Horizontal)
	{
		mHorizontalPageAction->blockSignals(true);
		mHorizontalPageAction->setChecked(true);
		mHorizontalPageAction->blockSignals(false);
	}
	else
	{
		mVerticalPageAction->blockSignals(true);
		mVerticalPageAction->setChecked(true);
		mVerticalPageAction->blockSignals(false);
	}

	mRowsSpinBox->blockSignals(true);
	mRowsSpinBox->setValue(PageSettings.Rows);
	mRowsSpinBox->blockSignals(false);

	mColumnsSpinBox->blockSignals(true);
	mColumnsSpinBox->setValue(PageSettings.Columns);
	mColumnsSpinBox->blockSignals(false);
}

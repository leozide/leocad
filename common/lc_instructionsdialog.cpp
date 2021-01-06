#include "lc_global.h"
#include "lc_instructionsdialog.h"
#include "project.h"
#include "lc_model.h"

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

lcInstructionsDialog::lcInstructionsDialog(QWidget* Parent, Project* Project)
	: QMainWindow(Parent), mProject(Project)
{
	QWidget* CentralWidget = new QWidget(this);
	setCentralWidget(CentralWidget);
	setWindowTitle(tr("Instructions"));

	QVBoxLayout* Layout = new QVBoxLayout(CentralWidget);
	Layout->setContentsMargins(0, 0, 0, 0);

	QSplitter* Splitter = new QSplitter(CentralWidget);
	Splitter->setOrientation(Qt::Horizontal);
	Layout->addWidget(Splitter);

	mThumbnailsWidget = new QListWidget(Splitter);
	Splitter->addWidget(mThumbnailsWidget);

	mPageWidget = new lcInstructionsPageWidget(Splitter);
	Splitter->addWidget(mPageWidget);

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
		mThumbnailsWidget->addItem(QString("Page %1").arg(PageNumber + 1));

	connect(mThumbnailsWidget, SIGNAL(currentRowChanged(int)), this, SLOT(CurrentThumbnailChanged(int)));
	mThumbnailsWidget->setCurrentRow(0);

	connect(mVerticalPageAction, SIGNAL(toggled()), this, SLOT(UpdatePageSettings()));
	connect(mHorizontalPageAction, SIGNAL(toggled()), this, SLOT(UpdatePageSettings()));
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

	mThumbnailsWidget->clear();
	for (size_t PageNumber = 0; PageNumber < mInstructions.mPages.size(); PageNumber++)
		mThumbnailsWidget->addItem(QString("Page %1").arg(PageNumber + 1));

//	mThumbnailsWidget->setCurrentRow(0);

//	mPageWidget->SetCurrentPage(Page);
}

void lcInstructionsDialog::CurrentThumbnailChanged(int Index)
{
	if (Index < 0 || Index >= mInstructions.mPages.size())
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

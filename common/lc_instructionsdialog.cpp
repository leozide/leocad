#include "lc_global.h"
#include "lc_instructionsdialog.h"
#include "project.h"
#include "lc_model.h"

lcInstructionsPageWidget::lcInstructionsPageWidget(QWidget* Parent, Project* Project)
	: QGraphicsView(Parent)
{
}

void lcInstructionsPageWidget::SetCurrentPage(lcInstructionsPageLayout* PageLayout)
{
	QGraphicsScene* Scene = new QGraphicsScene(this);
	setScene(Scene);
	Scene->setSceneRect(0, 0, 1000, 1000);
//	Scene->setBackgroundBrush(Qt::black);

	QImage StepImage = PageLayout->Model->GetStepImage(false, 500, 500, PageLayout->Step);
	QGraphicsPixmapItem* StepImageItem = Scene->addPixmap(QPixmap::fromImage(StepImage));
	StepImageItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);

	QGraphicsSimpleTextItem* StepNumberItem = Scene->addSimpleText(QString::number(PageLayout->Step), QFont("Helvetica", 96));
	StepNumberItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);

	QImage PartsImage = PageLayout->Model->GetPartsListImage(300, PageLayout->Step);
	QGraphicsPixmapItem* PartsImageItem = Scene->addPixmap(QPixmap::fromImage(PartsImage));
	PartsImageItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
	PartsImageItem->setPos(StepNumberItem->pos() + QPointF(StepNumberItem->sceneBoundingRect().width(), 0));
}

lcInstructionsDialog::lcInstructionsDialog(QWidget* Parent, Project* Project)
	: QMainWindow(Parent), mProject(Project)
{
	QWidget* CentralWidget = new QWidget(this);
	setCentralWidget(CentralWidget);
	setWindowTitle(tr("Instructions"));

	QVBoxLayout* Layout = new QVBoxLayout(CentralWidget);

	QSplitter* Splitter = new QSplitter(CentralWidget);
	Splitter->setOrientation(Qt::Horizontal);
	Layout->addWidget(Splitter);

	mThumbnailsWidget = new QListWidget(Splitter);
	Splitter->addWidget(mThumbnailsWidget);

	mPageWidget = new lcInstructionsPageWidget(Splitter, mProject);
	Splitter->addWidget(mPageWidget);

	mPageLayouts = mProject->GetPageLayouts();

	for (size_t PageNumber = 0; PageNumber < mPageLayouts.size(); PageNumber++)
		mThumbnailsWidget->addItem(QString("Page %1").arg(PageNumber + 1));

	connect(mThumbnailsWidget, SIGNAL(currentRowChanged(int)), this, SLOT(CurrentThumbnailChanged(int)));
	mThumbnailsWidget->setCurrentRow(0);
}

void lcInstructionsDialog::CurrentThumbnailChanged(int Index)
{
	mPageWidget->SetCurrentPage(&mPageLayouts[Index]);
}

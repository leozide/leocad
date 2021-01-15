#include "lc_global.h"
#include "lc_instructionsdialog.h"
#include "lc_pagesetupdialog.h"
#include "project.h"
#include "lc_model.h"
#include "lc_view.h"
#include "lc_collapsiblewidget.h"

lcInstructionsStepImageItem::lcInstructionsStepImageItem(const QPixmap& Pixmap, QGraphicsItem* Parent, lcModel* Model, lcStep Step, lcInstructionsPropertiesWidget* PropertiesWidget)
	: QGraphicsPixmapItem(Pixmap, Parent), mModel(Model), mStep(Step), mPropertiesWidget(PropertiesWidget)
{
}

void lcInstructionsStepImageItem::focusInEvent(QFocusEvent* FocusEvent)
{
	mPropertiesWidget->StepImageItemFocusIn(this);

	QGraphicsPixmapItem::focusInEvent(FocusEvent);
}

void lcInstructionsStepImageItem::focusOutEvent(QFocusEvent* FocusEvent)
{
	mPropertiesWidget->ItemFocusOut(this);

	QGraphicsPixmapItem::focusOutEvent(FocusEvent);
}

lcInstructionsStepNumberItem::lcInstructionsStepNumberItem(const QString& Text, QGraphicsItem* Parent, lcModel* Model, lcStep Step, lcInstructionsPropertiesWidget* PropertiesWidget)
	: QGraphicsSimpleTextItem(Text, Parent), mModel(Model), mStep(Step), mPropertiesWidget(PropertiesWidget)
{
}

void lcInstructionsStepNumberItem::focusInEvent(QFocusEvent* FocusEvent)
{
	mPropertiesWidget->StepNumberItemFocusIn(this);

	QGraphicsSimpleTextItem::focusInEvent(FocusEvent);
}

void lcInstructionsStepNumberItem::focusOutEvent(QFocusEvent* FocusEvent)
{
	mPropertiesWidget->ItemFocusOut(this);

	QGraphicsSimpleTextItem::focusOutEvent(FocusEvent);
}

lcInstructionsPageWidget::lcInstructionsPageWidget(QWidget* Parent, lcInstructions* Instructions)
	: QGraphicsView(Parent), mInstructions(Instructions)
{
}

void lcInstructionsPageWidget::SetCurrentPage(const lcInstructionsPage* Page, lcInstructionsPropertiesWidget* PropertiesWidget)
{
	QGraphicsScene* Scene = new QGraphicsScene();
	setScene(Scene);

	if (!Page)
		return;

	const lcInstructionsPageSetup& PageSetup = mInstructions->mPageSetup;
//	Scene->setSceneRect(0, 0, mInstructions->mPageSetup.Width, mInstructions->mPageSetup.Height);

	QGraphicsRectItem* PageItem = Scene->addRect(QRectF(0.0f, 0.0f, PageSetup.Width, PageSetup.Height), QPen(Qt::black), QBrush(Qt::white));
	PageItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

	QRectF MarginsRect(PageSetup.MarginLeft, PageSetup.MarginTop, PageSetup.Width - PageSetup.MarginLeft - PageSetup.MarginRight, PageSetup.Height - PageSetup.MarginTop - PageSetup.MarginBottom);

	for (const lcInstructionsStep& Step : Page->Steps)
	{
		const float StepWidth = MarginsRect.width() * Step.Rect.width();
		const float StepHeight = MarginsRect.height() * Step.Rect.height();
		const lcInstructionsStepProperties StepProperties = mInstructions->GetStepProperties(Step.Model, Step.Step);

		lcView View(lcViewType::View, Step.Model);

		View.SetOffscreenContext();
		View.MakeCurrent();
		View.SetBackgroundColorOverride(StepProperties.BackgroundColor);
		View.SetSize(StepWidth, StepHeight);

		std::vector<QImage> Images = View.GetStepImages(Step.Step, Step.Step);

		if (Images.empty())
			continue;

		QImage& StepImage = Images.front();

		lcInstructionsStepImageItem* StepImageItem = new lcInstructionsStepImageItem(QPixmap::fromImage(StepImage), PageItem, Step.Model, Step.Step, PropertiesWidget);
		StepImageItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
		StepImageItem->setPos(MarginsRect.left() + MarginsRect.width() * Step.Rect.x(), MarginsRect.top() + MarginsRect.height() * Step.Rect.y());

		lcInstructionsStepNumberItem* StepNumberItem = new lcInstructionsStepNumberItem(QString::number(Step.Step), StepImageItem, Step.Model, Step.Step, PropertiesWidget);
		QFont StepNumberFont;
		if (!StepNumberFont.fromString(StepProperties.StepNumberFont))
			StepNumberFont = QFont("Helvetica", 72);
		StepNumberItem->setFont(StepNumberFont);
		StepNumberItem->setBrush(QBrush(QColor::fromRgb(LC_RGBA_RED(StepProperties.StepNumberColor), LC_RGBA_GREEN(StepProperties.StepNumberColor), LC_RGBA_BLUE(StepProperties.StepNumberColor))));
		StepNumberItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);

		QImage PartsImage = Step.Model->GetPartsListImage(300, Step.Step);

		QGraphicsPixmapItem* PartsImageItem = new QGraphicsPixmapItem(QPixmap::fromImage(PartsImage), StepImageItem);
		PartsImageItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
		PartsImageItem->setPos(StepNumberItem->boundingRect().topRight());
	}
}

lcInstructionsPageListWidget::lcInstructionsPageListWidget(QWidget* Parent, lcInstructions* Instructions)
	: QDockWidget(Parent), mInstructions(Instructions)
{
	QWidget* CentralWidget = new QWidget(this);
	setWidget(CentralWidget);
	setWindowTitle(tr("Pages"));

	QVBoxLayout* Layout = new QVBoxLayout(CentralWidget);
	Layout->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout* ButtonsLayout = new QHBoxLayout();
	ButtonsLayout->setContentsMargins(0, 0, 0, 0);
	Layout->addLayout(ButtonsLayout);

	QToolButton* PageSetupButton = new QToolButton();
	PageSetupButton->setText("Page Setup");
	ButtonsLayout->addWidget(PageSetupButton);

	connect(PageSetupButton, SIGNAL(clicked()), this, SLOT(ShowPageSetupDialog()));

	ButtonsLayout->addStretch(1);

	/*
	lcCollapsibleWidget* SetupWidget = new lcCollapsibleWidget(tr("Page Setup"), CentralWidget);
	Layout->addWidget(SetupWidget);

//	QVBoxLayout* SetupLayout = new QVBoxLayout();
//	SetupWidget->SetChildLayout(SetupLayout);

	QGridLayout* SetupLayout = new QGridLayout();
	SetupWidget->SetChildLayout(SetupLayout);

//	lcCollapsibleWidget* SizeWidget = new lcCollapsibleWidget(tr("Size"));
	QGroupBox* SizeWidget = new QGroupBox(tr("Size"));

	SetupLayout->addWidget(SizeWidget);

	QGridLayout* SizeLayout = new QGridLayout();
//	SizeWidget->SetChildLayout(SizeLayout);
	SizeWidget->setLayout(SizeLayout);

	mWidthEdit = new lcSmallLineEdit();
	SizeLayout->addWidget(new QLabel(tr("Width:")), 1, 0);
	SizeLayout->addWidget(mWidthEdit, 1, 1);

	mHeightEdit = new lcSmallLineEdit();
	SizeLayout->addWidget(new QLabel(tr("Height:")), 1, 2);
	SizeLayout->addWidget(mHeightEdit, 1, 3);

	mUnitsComboBox = new QComboBox();
	mUnitsComboBox->addItems(QStringList() << tr("Pixels") << tr("Centimeters") << tr("Inches"));
	SizeLayout->addWidget(new QLabel(tr("Units:")), 4, 0);
	SizeLayout->addWidget(mUnitsComboBox, 4, 1, 1, -1);

	mSizeComboBox = new QComboBox();
	SizeLayout->addWidget(new QLabel(tr("Preset:")), 5, 0);
	SizeLayout->addWidget(mSizeComboBox, 5, 1, 1, -1);


//	lcCollapsibleWidget* OrientationWidget = new lcCollapsibleWidget(tr("Orientation"));
//	SetupLayout->addWidget(OrientationWidget);
//
//	QVBoxLayout* OrientationLayout = new QVBoxLayout();
//	OrientationWidget->SetChildLayout(OrientationLayout);
//
//	mPortraitButton = new QRadioButton(tr("Portrait"));
//	OrientationLayout->addWidget(mPortraitButton);
//	mLandscapeButton = new QRadioButton(tr("Landscape"));
//	OrientationLayout->addWidget(mLandscapeButton);

	QGroupBox* MarginsWidget = new QGroupBox(tr("Margins"));
//	lcCollapsibleWidget* MarginsWidget = new lcCollapsibleWidget(tr("Margins"));
	SetupLayout->addWidget(MarginsWidget);

	QGridLayout* MarginsLayout = new QGridLayout();
//	MarginsWidget->SetChildLayout(MarginsLayout);
	MarginsWidget->setLayout(MarginsLayout);

	mLeftMarginEdit = new lcSmallLineEdit();
	MarginsLayout->addWidget(new QLabel(tr("Left:")), 2, 0);
	MarginsLayout->addWidget(mLeftMarginEdit, 2, 1);

	mRightMarginEdit = new lcSmallLineEdit();
	MarginsLayout->addWidget(new QLabel(tr("Right:")), 2, 2);
	MarginsLayout->addWidget(mRightMarginEdit, 2, 3);

	mTopMarginEdit = new lcSmallLineEdit();
	MarginsLayout->addWidget(new QLabel(tr("Top:")), 3, 0);
	MarginsLayout->addWidget(mTopMarginEdit, 3, 1);

	mBottomMarginEdit = new lcSmallLineEdit();
	MarginsLayout->addWidget(new QLabel(tr("Bottom:")), 3, 2);
	MarginsLayout->addWidget(mBottomMarginEdit, 3, 3);

//	lcCollapsibleWidget* UnitsWidget = new lcCollapsibleWidget(tr("Units"));
//	SetupLayout->addWidget(UnitsWidget);
//
//	QVBoxLayout* UnitsLayout = new QVBoxLayout();
//	UnitsWidget->SetChildLayout(UnitsLayout);

//	SetupWidget->Collapse();
*/
	mThumbnailsWidget = new QListWidget(CentralWidget);
	Layout->addWidget(mThumbnailsWidget);
}

void lcInstructionsPageListWidget::ShowPageSetupDialog()
{
	lcPageSetupDialog Dialog(this, &mInstructions->mPageSetup);

	if (Dialog.exec() != QDialog::Accepted)
		return;
}

lcInstructionsPropertiesWidget::lcInstructionsPropertiesWidget(QWidget* Parent, lcInstructions* Instructions)
	: QDockWidget(Parent), mInstructions(Instructions)
{
	QWidget* CentralWidget = new QWidget(this);
	setWidget(CentralWidget);
	setWindowTitle(tr("Properties"));
	
	QGridLayout* Layout = new QGridLayout(CentralWidget);
	Layout->setContentsMargins(0, 0, 0, 0);

	QComboBox* ScopeComboBox = new QComboBox(CentralWidget);
	ScopeComboBox->addItems(QStringList() << tr("Default") << tr("Current Model") << tr("Current Step Only") << tr("Current Step Forward"));

	Layout->addWidget(new QLabel(tr("Scope:")), 0, 0);
	Layout->addWidget(ScopeComboBox, 0, 1);

	QComboBox* PresetComboBox = new QComboBox(CentralWidget);

	Layout->addWidget(new QLabel(tr("Preset:")), 1, 0);
	Layout->addWidget(PresetComboBox, 1, 1);

	Layout->setRowStretch(3, 1);
}

void lcInstructionsPropertiesWidget::ColorButtonClicked()
{
//	const lcInstructionsStepProperties StepProperties = mInstructions->GetStepProperties(Step.Model, Step.Step);

	QColor Color = QColorDialog::getColor();

	if (!Color.isValid())
		return;

	mInstructions->SetDefaultStepBackgroundColor(LC_RGBA(Color.red(), Color.green(), Color.blue(), Color.alpha()));

//	mPageListWidget->mThumbnailsWidget->setCurrentRow(0);
}

void lcInstructionsPropertiesWidget::StepImageItemFocusIn(lcInstructionsStepImageItem* ImageItem)
{
	if (mFocusItem == ImageItem)
		return;

	mFocusItem = ImageItem;

	delete mWidget;
	mWidget = new lcCollapsibleWidget(tr("Step Properties")); // todo: disable collapse

	QGridLayout* WidgetLayout = qobject_cast<QGridLayout*>(widget()->layout());
	WidgetLayout->addWidget(mWidget, 2, 0, 1, -1);

	QGridLayout* Layout = new QGridLayout();
	mWidget->SetChildLayout(Layout);

	QToolButton* ColorButton = new QToolButton();
	ColorButton->setText("background");

	Layout->addWidget(ColorButton);

	connect(ColorButton, &QToolButton::clicked, this, &lcInstructionsPropertiesWidget::ColorButtonClicked);
}

void lcInstructionsPropertiesWidget::StepNumberItemFocusIn(lcInstructionsStepNumberItem* NumberItem)
{
	if (mFocusItem == NumberItem)
		return;

	mFocusItem = NumberItem;

	delete mWidget;
	mWidget = new lcCollapsibleWidget(tr("Step Number Properties")); // todo: disable collapse

	QGridLayout* WidgetLayout = qobject_cast<QGridLayout*>(widget()->layout());
	WidgetLayout->addWidget(mWidget, 2, 0, 1, -1);

	QGridLayout* Layout = new QGridLayout();
	mWidget->SetChildLayout(Layout);

	lcInstructionsStepProperties StepProperties = mInstructions->GetStepProperties(NumberItem->GetModel(), NumberItem->GetStep());

	QFont Font;
	Font.fromString(StepProperties.StepNumberFont);

	QLabel* FontLabel = new QLabel(tr("Font:"));
	Layout->addWidget(FontLabel, 0, 0);

	QToolButton* FontButton = new QToolButton();
	FontButton->setText(Font.family());
	Layout->addWidget(FontButton, 0, 1);

	connect(FontButton, &QToolButton::clicked, []()
	{
		bool Ok;
		QFontDialog::getFont(&Ok);
	});// this, & lcInstructionsPropertiesWidget::ColorButtonClicked);

	QLabel* ColorLabel = new QLabel(tr("Color:"));
	Layout->addWidget(ColorLabel, 1, 0);

//	QToolButton* ColorButton = new QToolButton();
//	ColorButton->setText("Font");
//
//	Layout->addWidget(ColorButton);
//
//	connect(ColorButton, &QToolButton::clicked, this, &lcInstructionsPropertiesWidget::ColorButtonClicked);
}

void lcInstructionsPropertiesWidget::ItemFocusOut(QGraphicsItem* Item)
{
	if (mFocusItem != Item)
		return;

	mFocusItem = nullptr;

	delete mWidget;
	mWidget = nullptr;
}

lcInstructionsDialog::lcInstructionsDialog(QWidget* Parent, Project* Project)
	: QMainWindow(Parent), mProject(Project)
{
	setWindowTitle(tr("Instructions"));

	mInstructions = mProject->GetInstructions();

	mPageWidget = new lcInstructionsPageWidget(this, mInstructions);
	setCentralWidget(mPageWidget);

	mPageListWidget = new lcInstructionsPageListWidget(this, mInstructions);
	mPageListWidget->setObjectName("InstructionsPageList");
	addDockWidget(Qt::LeftDockWidgetArea, mPageListWidget);

	mPropertiesWidget = new lcInstructionsPropertiesWidget(this, mInstructions);
	mPropertiesWidget->setObjectName("InstructionsProperties");
	addDockWidget(Qt::RightDockWidgetArea, mPropertiesWidget);

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

	for (size_t PageNumber = 0; PageNumber < mInstructions->mPages.size(); PageNumber++)
		mPageListWidget->mThumbnailsWidget->addItem(QString("Page %1").arg(PageNumber + 1));

	connect(mPageListWidget->mThumbnailsWidget, SIGNAL(currentRowChanged(int)), this, SLOT(CurrentThumbnailChanged(int)));
	mPageListWidget->mThumbnailsWidget->setCurrentRow(0);

	connect(mInstructions, &lcInstructions::PageInvalid, this, &lcInstructionsDialog::PageInvalid);
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

	mInstructions->SetDefaultPageSettings(PageSettings);
//	lcInstructionsPage* Page = &mInstructions.mPages[mThumbnailsWidget->currentIndex().row()];

	mPageListWidget->mThumbnailsWidget->clear();
	for (size_t PageNumber = 0; PageNumber < mInstructions->mPages.size(); PageNumber++)
		mPageListWidget->mThumbnailsWidget->addItem(QString("Page %1").arg(PageNumber + 1));

//	mThumbnailsWidget->setCurrentRow(0);

//	mPageWidget->SetCurrentPage(Page);
}

void lcInstructionsDialog::CurrentThumbnailChanged(int Index)
{
	if (Index < 0 || Index >= static_cast<int>(mInstructions->mPages.size()))
	{
		mPageWidget->SetCurrentPage(nullptr, mPropertiesWidget);
		return;
	}

	const lcInstructionsPage* Page = &mInstructions->mPages[Index];
//	const lcInstructionsPageSettings& PageSettings = Page->Settings;
	const lcInstructionsPageSettings& PageSettings = mInstructions->mPageSettings;

	mPageWidget->SetCurrentPage(Page, mPropertiesWidget);

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

void lcInstructionsDialog::PageInvalid(int PageIndex)
{
	if (mPageListWidget->mThumbnailsWidget->currentRow() == PageIndex)
	{
		const lcInstructionsPage* Page = &mInstructions->mPages[PageIndex];
		mPageWidget->SetCurrentPage(Page, mPropertiesWidget);
	}
}

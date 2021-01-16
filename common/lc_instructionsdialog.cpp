#include "lc_global.h"
#include "lc_instructionsdialog.h"
#include "lc_pagesetupdialog.h"
#include "project.h"
#include "lc_model.h"
#include "lc_view.h"
#include "lc_collapsiblewidget.h"

lcInstructionsStepImageItem::lcInstructionsStepImageItem(QGraphicsItem* Parent, lcInstructions* Instructions, lcModel* Model, lcStep Step)
	: QGraphicsPixmapItem(Parent), mInstructions(Instructions), mModel(Model), mStep(Step)
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
}

void lcInstructionsStepImageItem::Update()
{
	lcView View(lcViewType::View, mModel);

	View.SetOffscreenContext();
	View.MakeCurrent();
	View.SetBackgroundColorOverride(lcRGBAFromQColor(mInstructions->GetColorProperty(lcInstructionsPropertyType::StepBackgroundColor, mModel, mStep)));
	View.SetSize(mWidth, mHeight);

	std::vector<QImage> Images = View.GetStepImages(mStep, mStep);

	if (!Images.empty())
		setPixmap(QPixmap::fromImage(Images.front()));
}

lcInstructionsStepNumberItem::lcInstructionsStepNumberItem(QGraphicsItem* Parent, lcInstructions* Instructions, lcModel* Model, lcStep Step)
	: QGraphicsSimpleTextItem(Parent), mInstructions(Instructions), mModel(Model), mStep(Step)
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
}

void lcInstructionsStepNumberItem::Update()
{
	QFont StepNumberFont = mInstructions->GetFontProperty(lcInstructionsPropertyType::StepNumberFont, mModel, mStep);

	setFont(StepNumberFont);
	setBrush(QBrush(mInstructions->GetColorProperty(lcInstructionsPropertyType::StepNumberColor, mModel, mStep)));
	setText(QString::number(mStep));
}

lcInstructionsPartsListItem::lcInstructionsPartsListItem(QGraphicsItem* Parent, lcInstructions* Instructions, lcModel* Model, lcStep Step)
	: QGraphicsPixmapItem(Parent), mInstructions(Instructions), mModel(Model), mStep(Step)
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
}

void lcInstructionsPartsListItem::Update()
{
	QColor BackgroundColor = mInstructions->GetColorProperty(lcInstructionsPropertyType::PLIBackgroundColor, mModel, mStep);
	QFont Font = mInstructions->GetFontProperty(lcInstructionsPropertyType::PLIFont, mModel, mStep);
	QColor TextColor = mInstructions->GetColorProperty(lcInstructionsPropertyType::PLITextColor, mModel, mStep);

	QImage PartsImage = mModel->GetPartsListImage(300, mStep, lcRGBAFromQColor(BackgroundColor), Font, TextColor);
	setPixmap(QPixmap::fromImage(PartsImage));
}

lcInstructionsPageWidget::lcInstructionsPageWidget(QWidget* Parent, lcInstructions* Instructions, lcInstructionsPropertiesWidget* PropertiesWidget)
	: QGraphicsView(Parent), mInstructions(Instructions), mPropertiesWidget(PropertiesWidget)
{
	QGraphicsScene* Scene = new QGraphicsScene();
	setScene(Scene);

	connect(mInstructions, &lcInstructions::StepSettingsChanged, this, &lcInstructionsPageWidget::StepSettingsChanged);
	connect(Scene, &QGraphicsScene::selectionChanged, this, &lcInstructionsPageWidget::SelectionChanged);
}

void lcInstructionsPageWidget::StepSettingsChanged(lcModel* Model, lcStep Step)
{
	QGraphicsScene* Scene = scene();

	QList<QGraphicsItem*> Items = Scene->items();

	for (QGraphicsItem* Item : Items)
	{
		lcInstructionsStepImageItem* ImageItem = dynamic_cast<lcInstructionsStepImageItem*>(Item);

		if (ImageItem)
		{
			if (!Model || (ImageItem->GetModel() == Model && ImageItem->GetStep() == Step))
				ImageItem->Update();

			continue;
		}

		lcInstructionsStepNumberItem* NumberItem = dynamic_cast<lcInstructionsStepNumberItem*>(Item);

		if (NumberItem)
		{
			if (!Model || (NumberItem->GetModel() == Model && NumberItem->GetStep() == Step))
				NumberItem->Update();

			continue;
		}

		lcInstructionsPartsListItem* PartsItem = dynamic_cast<lcInstructionsPartsListItem*>(Item);

		if (PartsItem)
		{
			if (!Model || (PartsItem->GetModel() == Model && PartsItem->GetStep() == Step))
				PartsItem->Update();

			continue;
		}
	}
}

void lcInstructionsPageWidget::SelectionChanged()
{
	QGraphicsScene* Scene = qobject_cast<QGraphicsScene*>(sender());
	QGraphicsItem* Focus = nullptr;

	if (Scene)
	{
		QList<QGraphicsItem*> SelectedItems = Scene->selectedItems();

		if (!SelectedItems.isEmpty())
			Focus = SelectedItems.first();
	}

	mPropertiesWidget->SelectionChanged(Focus);
}

void lcInstructionsPageWidget::SetCurrentPage(const lcInstructionsPage* Page)
{
	QGraphicsScene* Scene = scene();

	Scene->clear();

	if (!Page)
		return;

	const lcInstructionsPageSetup& PageSetup = mInstructions->mPageSetup;
//	Scene->setSceneRect(0, 0, mInstructions->mPageSetup.Width, mInstructions->mPageSetup.Height);

	QGraphicsRectItem* PageItem = Scene->addRect(QRectF(0.0f, 0.0f, PageSetup.Width, PageSetup.Height), QPen(Qt::black), QBrush(Qt::white));
	PageItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

	QRectF MarginsRect(PageSetup.MarginLeft, PageSetup.MarginTop, PageSetup.Width - PageSetup.MarginLeft - PageSetup.MarginRight, PageSetup.Height - PageSetup.MarginTop - PageSetup.MarginBottom);

	for (const lcInstructionsStep& Step : Page->Steps)
	{
		lcInstructionsStepImageItem* StepImageItem = new lcInstructionsStepImageItem(PageItem, mInstructions, Step.Model, Step.Step);
		StepImageItem->setPos(MarginsRect.left() + MarginsRect.width() * Step.Rect.x(), MarginsRect.top() + MarginsRect.height() * Step.Rect.y());
		StepImageItem->SetImageSize(MarginsRect.width() * Step.Rect.width(), MarginsRect.height() * Step.Rect.height());
		StepImageItem->Update();

		lcInstructionsStepNumberItem* StepNumberItem = new lcInstructionsStepNumberItem(StepImageItem, mInstructions, Step.Model, Step.Step);
		StepNumberItem->Update();

		lcInstructionsPartsListItem* PartsImageItem = new lcInstructionsPartsListItem(StepImageItem, mInstructions, Step.Model, Step.Step);
		PartsImageItem->setPos(StepNumberItem->boundingRect().topRight());
		PartsImageItem->Update();
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

void lcInstructionsPropertiesWidget::AddColorProperty(lcInstructionsPropertyType Type)
{
	QString Label;

	switch (Type)
	{
	case lcInstructionsPropertyType::StepNumberColor:
	case lcInstructionsPropertyType::PLITextColor:
		Label = tr("Text Color:");
		break;

	case lcInstructionsPropertyType::StepBackgroundColor:
	case lcInstructionsPropertyType::PLIBackgroundColor:
		Label = tr("Background Color:");
		break;

	case lcInstructionsPropertyType::PLIBorderColor:
		Label = tr("Border Color:");
		break;

	case lcInstructionsPropertyType::StepNumberFont:
	case lcInstructionsPropertyType::PLIFont:
//	case lcInstructionsPropertyType::PLIBorderWidth:
//	case lcInstructionsPropertyType::PLIBorderRound:
	case lcInstructionsPropertyType::Count:
		break;
	}

	const int Row = mPropertiesLayout->rowCount();

	mPropertiesLayout->addWidget(new QLabel(Label), Row, 0);

	QToolButton* ColorButton = new QToolButton();
	mPropertiesLayout->addWidget(ColorButton, Row, 1);

	auto UpdateButton = [this, Type, ColorButton]()
	{
		QPixmap Pixmap(12, 12);
		QColor Color = mInstructions->GetColorProperty(Type, mModel, mStep);
		Pixmap.fill(Color);
		ColorButton->setIcon(Pixmap);
	};
	
	UpdateButton();

	connect(ColorButton, &QToolButton::clicked, [this, Type, UpdateButton]()
	{
		QString Title;

		switch (Type)
		{
			case lcInstructionsPropertyType::StepNumberColor:
				Title = tr("Select Step Number Color");
				break;

			case lcInstructionsPropertyType::StepBackgroundColor:
				Title = tr("Select Step Background Color");
				break;

			case lcInstructionsPropertyType::PLIBackgroundColor:
				Title = tr("Select Parts List Background Color");
				break;

			case lcInstructionsPropertyType::PLIBorderColor:
				Title = tr("Select Parts List Border Color");
				break;

			case lcInstructionsPropertyType::PLITextColor:
				Title = tr("Select Parts List Text Color");
				break;

			case lcInstructionsPropertyType::StepNumberFont:
			case lcInstructionsPropertyType::PLIFont:
//			case lcInstructionsPropertyType::StepPLIBorderWidth:
//			case lcInstructionsPropertyType::StepPLIBorderRound:
			case lcInstructionsPropertyType::Count:
				break;
		}

		QColor Color = mInstructions->GetColorProperty(Type, mModel, mStep);
		Color = QColorDialog::getColor(Color, this, Title);

		if (Color.isValid())
		{
			mInstructions->SetDefaultColor(Type, Color);
			UpdateButton();
		}
	});
}

void lcInstructionsPropertiesWidget::AddFontProperty(lcInstructionsPropertyType Type)
{
	const QString Label = tr("Font:");
	const int Row = mPropertiesLayout->rowCount();

	mPropertiesLayout->addWidget(new QLabel(Label), Row, 0);

	QToolButton* FontButton = new QToolButton();
	mPropertiesLayout->addWidget(FontButton, Row, 1);

	auto UpdateButton = [this, Type, FontButton]()
	{
		QFont Font = mInstructions->GetFontProperty(Type, mModel, mStep);
		QString FontName = QString("%1 %2").arg(Font.family(), QString::number(Font.pointSize()));
		FontButton->setText(FontName);
	};

	UpdateButton();

	connect(FontButton, &QToolButton::clicked, [this, Type, UpdateButton]()
	{
		QString Title;

		switch (Type)
		{
			case lcInstructionsPropertyType::StepNumberFont:
				Title = tr("Select Step Number Font");
				break;

			case lcInstructionsPropertyType::PLIFont:
				Title = tr("Select Parts List Font");
				break;

			case lcInstructionsPropertyType::StepNumberColor:
			case lcInstructionsPropertyType::StepBackgroundColor:
			case lcInstructionsPropertyType::PLIBackgroundColor:
			case lcInstructionsPropertyType::PLITextColor:
			case lcInstructionsPropertyType::PLIBorderColor:
			case lcInstructionsPropertyType::Count:
				break;
		}

		bool Ok = false;

		QFont Font = mInstructions->GetFontProperty(Type, mModel, mStep);
		Font = QFontDialog::getFont(&Ok, Font, this, tr("Select Step Number Font"));

		if (Ok)
		{
			UpdateButton();
			mInstructions->SetDefaultFont(Type, Font);
		}
	});
}

void lcInstructionsPropertiesWidget::SelectionChanged(QGraphicsItem* FocusItem)
{
	if (mFocusItem == FocusItem)
		return;

	delete mWidget;
	mWidget = nullptr;
	mFocusItem = FocusItem;
	mModel = nullptr;
	mStep = 1;

	if (!FocusItem)
		return;

	auto CreatePropertyWidget = [this](const QString& Title)
	{
		mWidget = new lcCollapsibleWidget(Title); // todo: disable collapse

		QGridLayout* WidgetLayout = qobject_cast<QGridLayout*>(widget()->layout());
		WidgetLayout->addWidget(mWidget, 2, 0, 1, -1);

		mPropertiesLayout = new QGridLayout();
		mWidget->SetChildLayout(mPropertiesLayout);
	};

	lcInstructionsStepImageItem* ImageItem = dynamic_cast<lcInstructionsStepImageItem*>(FocusItem);

	if (ImageItem)
	{
		CreatePropertyWidget(tr("Step Properties"));

		mModel = ImageItem->GetModel();
		mStep = ImageItem->GetStep();

		AddColorProperty(lcInstructionsPropertyType::StepBackgroundColor);

		return;
	}

	lcInstructionsStepNumberItem* NumberItem = dynamic_cast<lcInstructionsStepNumberItem*>(FocusItem);

	if (NumberItem)
	{
		CreatePropertyWidget(tr("Step Number Properties"));

		mModel = NumberItem->GetModel();
		mStep = NumberItem->GetStep();

		AddFontProperty(lcInstructionsPropertyType::StepNumberFont);
		AddColorProperty(lcInstructionsPropertyType::StepNumberColor);

		return;
	}

	lcInstructionsPartsListItem* PartsItem = dynamic_cast<lcInstructionsPartsListItem*>(FocusItem);

	if (PartsItem)
	{
		CreatePropertyWidget(tr("Parts List Properties"));

		mModel = PartsItem->GetModel();
		mStep = PartsItem->GetStep();

		AddColorProperty(lcInstructionsPropertyType::PLIBackgroundColor);
		AddFontProperty(lcInstructionsPropertyType::PLIFont);
		AddColorProperty(lcInstructionsPropertyType::PLITextColor);
		AddColorProperty(lcInstructionsPropertyType::PLIBorderColor);
//		PLIBorderWidth,
//		PLIBorderRound,

		return;
	}
}

lcInstructionsDialog::lcInstructionsDialog(QWidget* Parent, Project* Project)
	: QMainWindow(Parent), mProject(Project)
{
	setWindowTitle(tr("Instructions"));

	mInstructions = mProject->GetInstructions();

	mPropertiesWidget = new lcInstructionsPropertiesWidget(this, mInstructions);
	mPropertiesWidget->setObjectName("InstructionsProperties");
	addDockWidget(Qt::RightDockWidgetArea, mPropertiesWidget);

	mPageWidget = new lcInstructionsPageWidget(this, mInstructions, mPropertiesWidget);
	setCentralWidget(mPageWidget);

	mPageListWidget = new lcInstructionsPageListWidget(this, mInstructions);
	mPageListWidget->setObjectName("InstructionsPageList");
	addDockWidget(Qt::LeftDockWidgetArea, mPageListWidget);

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
		mPageWidget->SetCurrentPage(nullptr);
		return;
	}

	const lcInstructionsPage* Page = &mInstructions->mPages[Index];
//	const lcInstructionsPageSettings& PageSettings = Page->Settings;
	const lcInstructionsPageSettings& PageSettings = mInstructions->mPageSettings;

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

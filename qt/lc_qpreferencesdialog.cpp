#include "lc_global.h"
#include "lc_qpreferencesdialog.h"
#include "ui_lc_qpreferencesdialog.h"
#include "lc_qutils.h"
#include "lc_qcategorydialog.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_qutils.h"
#include "lc_glextensions.h"
#include "pieceinf.h"
#include "lc_edgecolordialog.h"

static const char* gLanguageLocales[] =
{
	"", "cs_CZ", "de_DE", "en_US", "fr_FR", "pt_PT", "es_ES"
};

lcQPreferencesDialog::lcQPreferencesDialog(QWidget* Parent, lcPreferencesDialogOptions* Options)
	: QDialog(Parent), mOptions(Options), ui(new Ui::lcQPreferencesDialog)
{
	ui->setupUi(this);

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
	ui->povrayLabel->hide();
	ui->povrayExecutable->hide();
	ui->povrayExecutableBrowse->hide();
	delete ui->povrayLabel;
	delete ui->povrayLayout;
#endif

	connect(ui->BackgroundSolidColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->BackgroundGradient1ColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->BackgroundGradient2ColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->AxesColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->TextColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->MarqueeBorderColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->MarqueeFillColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->OverlayColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->FadeStepsColor, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->HighlightNewPartsColor, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->gridStudColor, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->gridLineColor, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->ViewSphereColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->ViewSphereTextColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->ViewSphereHighlightColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->categoriesTree, SIGNAL(itemSelectionChanged()), this, SLOT(updateParts()));
	ui->shortcutEdit->installEventFilter(this);
	connect(ui->commandList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(commandChanged(QTreeWidgetItem*)));
	connect(ui->mouseTree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(MouseTreeItemChanged(QTreeWidgetItem*)));
	connect(ui->HighContrastButton, SIGNAL(clicked()), this, SLOT(AutomateEdgeColor()));
	connect(ui->AutomateEdgeColorButton, SIGNAL(clicked()), this, SLOT(AutomateEdgeColor()));

	ui->partsLibrary->setText(mOptions->LibraryPath);
	ui->ColorConfigEdit->setText(mOptions->ColorConfigPath);
	ui->MinifigSettingsEdit->setText(mOptions->MinifigSettingsPath);
	ui->povrayExecutable->setText(mOptions->POVRayPath);
	ui->lgeoPath->setText(mOptions->LGEOPath);
	ui->authorName->setText(mOptions->DefaultAuthor);
	ui->mouseSensitivity->setValue(mOptions->Preferences.mMouseSensitivity);
	const bool ColorThemeBlocked = ui->ColorTheme->blockSignals(true);
	ui->ColorTheme->setCurrentIndex(static_cast<int>(mOptions->Preferences.mColorTheme));
	ui->ColorTheme->blockSignals(ColorThemeBlocked);
	for (unsigned int LanguageIdx = 0; LanguageIdx < LC_ARRAY_COUNT(gLanguageLocales); LanguageIdx++)
	{
		if (mOptions->Language == gLanguageLocales[LanguageIdx])
		{
			ui->Language->setCurrentIndex(LanguageIdx);
			break;
		}
	}
	ui->checkForUpdates->setCurrentIndex(mOptions->CheckForUpdates);
	ui->fixedDirectionKeys->setChecked(mOptions->Preferences.mFixedAxes);
	ui->autoLoadMostRecent->setChecked(mOptions->Preferences.mAutoLoadMostRecent);
	ui->RestoreTabLayout->setChecked(mOptions->Preferences.mRestoreTabLayout);
	ui->AutomateEdgeColor->setChecked(mOptions->Preferences.mAutomateEdgeColor);

	ui->antiAliasing->setChecked(mOptions->AASamples != 1);
	if (mOptions->AASamples == 8)
		ui->antiAliasingSamples->setCurrentIndex(2);
	else if (mOptions->AASamples == 4)
		ui->antiAliasingSamples->setCurrentIndex(1);
	else
		ui->antiAliasingSamples->setCurrentIndex(0);
	ui->edgeLines->setChecked(mOptions->Preferences.mDrawEdgeLines);

#ifndef LC_OPENGLES
	if (QSurfaceFormat::defaultFormat().samples() > 1)
	{
		glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, mLineWidthRange);
		glGetFloatv(GL_SMOOTH_LINE_WIDTH_GRANULARITY, &mLineWidthGranularity);
	}
	else
#endif
	{
		glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, mLineWidthRange);
		mLineWidthGranularity = 1.0f;
	}

	ui->LineWidthSlider->setRange(0, (mLineWidthRange[1] - mLineWidthRange[0]) / mLineWidthGranularity);
	ui->LineWidthSlider->setValue((mOptions->Preferences.mLineWidth - mLineWidthRange[0]) / mLineWidthGranularity);

	ui->MeshLOD->setChecked(mOptions->Preferences.mAllowLOD);

	ui->MeshLODSlider->setRange(0, 1500.0f / mMeshLODMultiplier);
	ui->MeshLODSlider->setValue(mOptions->Preferences.mMeshLODDistance / mMeshLODMultiplier);

	ui->FadeSteps->setChecked(mOptions->Preferences.mFadeSteps);
	ui->HighlightNewParts->setChecked(mOptions->Preferences.mHighlightNewParts);
	ui->gridStuds->setChecked(mOptions->Preferences.mDrawGridStuds);
	ui->gridLines->setChecked(mOptions->Preferences.mDrawGridLines);
	ui->gridLineSpacing->setText(QString::number(mOptions->Preferences.mGridLineSpacing));
	ui->GridOriginCheckBox->setChecked(mOptions->Preferences.mDrawGridOrigin);
	ui->AxisIconCheckBox->setChecked(mOptions->Preferences.mDrawAxes);

	if (!mOptions->Preferences.mBackgroundGradient)
		ui->BackgroundSolidRadio->setChecked(true);
	else
		ui->BackgroundGradientRadio->setChecked(true);

	ui->ViewSphereLocationCombo->setCurrentIndex((int)mOptions->Preferences.mViewSphereLocation);

	if (mOptions->Preferences.mViewSphereEnabled)
	{
		switch (mOptions->Preferences.mViewSphereSize)
		{
		case 200:
			ui->ViewSphereSizeCombo->setCurrentIndex(3);
			break;
		case 100:
			ui->ViewSphereSizeCombo->setCurrentIndex(2);
			break;
		case 50:
			ui->ViewSphereSizeCombo->setCurrentIndex(1);
			break;
		default:
			ui->ViewSphereSizeCombo->setCurrentIndex(0);
			break;
		}
	}
	else
		ui->ViewSphereSizeCombo->setCurrentIndex(0);

	ui->PreviewAxisIconCheckBox->setChecked(mOptions->Preferences.mDrawPreviewAxis);

	ui->PreviewViewSphereLocationCombo->setCurrentIndex((int)mOptions->Preferences.mPreviewViewSphereLocation);

	if (mOptions->Preferences.mPreviewViewSphereEnabled)
	{
		switch (mOptions->Preferences.mPreviewViewSphereSize)
		{
		case 100:
			ui->PreviewViewSphereSizeCombo->setCurrentIndex(3);
			break;
		case 75:
			ui->PreviewViewSphereSizeCombo->setCurrentIndex(2);
			break;
		case 50:
			ui->PreviewViewSphereSizeCombo->setCurrentIndex(1);
			break;
		default:
			ui->PreviewViewSphereSizeCombo->setCurrentIndex(0);
			break;
		}
	}
	else
		ui->PreviewViewSphereSizeCombo->setCurrentIndex(0);

	if (!lcGetPiecesLibrary()->SupportsStudStyle())
		ui->studStyleCombo->setEnabled(false);

	ui->studStyleCombo->setCurrentIndex(static_cast<int>(mOptions->StudStyle));

	if (!gSupportsShaderObjects)
		ui->ShadingMode->removeItem(static_cast<int>(lcShadingMode::DefaultLights));
	ui->ShadingMode->setCurrentIndex(static_cast<int>(mOptions->Preferences.mShadingMode));

	auto SetButtonPixmap = [](quint32 Color, QToolButton* Button)
	{
		QPixmap Pixmap(12, 12);

		Pixmap.fill(QColor(LC_RGBA_RED(Color), LC_RGBA_GREEN(Color), LC_RGBA_BLUE(Color)));
		Button->setIcon(Pixmap);
	};

	SetButtonPixmap(mOptions->Preferences.mBackgroundSolidColor, ui->BackgroundSolidColorButton);
	SetButtonPixmap(mOptions->Preferences.mBackgroundGradientColorTop, ui->BackgroundGradient1ColorButton);
	SetButtonPixmap(mOptions->Preferences.mBackgroundGradientColorBottom, ui->BackgroundGradient2ColorButton);
	SetButtonPixmap(mOptions->Preferences.mAxesColor, ui->AxesColorButton);
	SetButtonPixmap(mOptions->Preferences.mTextColor, ui->TextColorButton);
	SetButtonPixmap(mOptions->Preferences.mMarqueeBorderColor, ui->MarqueeBorderColorButton);
	SetButtonPixmap(mOptions->Preferences.mMarqueeFillColor, ui->MarqueeFillColorButton);
	SetButtonPixmap(mOptions->Preferences.mOverlayColor, ui->OverlayColorButton);
	SetButtonPixmap(mOptions->Preferences.mActiveViewColor, ui->ActiveViewColorButton);
	SetButtonPixmap(mOptions->Preferences.mInactiveViewColor, ui->InactiveViewColorButton);
	SetButtonPixmap(mOptions->Preferences.mFadeStepsColor, ui->FadeStepsColor);
	SetButtonPixmap(mOptions->Preferences.mHighlightNewPartsColor, ui->HighlightNewPartsColor);
	SetButtonPixmap(mOptions->Preferences.mGridStudColor, ui->gridStudColor);
	SetButtonPixmap(mOptions->Preferences.mGridLineColor, ui->gridLineColor);
	SetButtonPixmap(mOptions->Preferences.mViewSphereColor, ui->ViewSphereColorButton);
	SetButtonPixmap(mOptions->Preferences.mViewSphereTextColor, ui->ViewSphereTextColorButton);
	SetButtonPixmap(mOptions->Preferences.mViewSphereHighlightColor, ui->ViewSphereHighlightColorButton);

	on_antiAliasing_toggled();
	on_AutomateEdgeColor_toggled();
	on_edgeLines_toggled();
	on_LineWidthSlider_valueChanged();
	on_MeshLODSlider_valueChanged();
	on_FadeSteps_toggled();
	on_HighlightNewParts_toggled();
	on_gridStuds_toggled();
	on_gridLines_toggled();
	on_ViewSphereSizeCombo_currentIndexChanged(ui->ViewSphereSizeCombo->currentIndex());
	on_PreviewViewSphereSizeCombo_currentIndexChanged(ui->PreviewViewSphereSizeCombo->currentIndex());

	updateCategories();
	ui->categoriesTree->setCurrentItem(ui->categoriesTree->topLevelItem(0));

	updateCommandList();
	new lcQTreeWidgetColumnStretcher(ui->commandList, 0);
	commandChanged(nullptr);

	UpdateMouseTree();
	ui->mouseTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui->mouseTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui->mouseTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	MouseTreeItemChanged(nullptr);
}

lcQPreferencesDialog::~lcQPreferencesDialog()
{
	delete ui;
}

void lcQPreferencesDialog::accept()
{
	int gridLineSpacing = ui->gridLineSpacing->text().toInt();
	if (gridLineSpacing < 1)
	{
		QMessageBox::information(this, "LeoCAD", tr("Grid spacing must be greater than 0."));
		return;
	}

	mOptions->LibraryPath = ui->partsLibrary->text();
	mOptions->MinifigSettingsPath = ui->MinifigSettingsEdit->text();
	mOptions->ColorConfigPath = ui->ColorConfigEdit->text();
	mOptions->POVRayPath = ui->povrayExecutable->text();
	mOptions->LGEOPath = ui->lgeoPath->text();
	mOptions->DefaultAuthor = ui->authorName->text();
	mOptions->Preferences.mMouseSensitivity = ui->mouseSensitivity->value();
	mOptions->Preferences.mColorTheme = static_cast<lcColorTheme>(ui->ColorTheme->currentIndex());
	mOptions->Preferences.mAutomateEdgeColor = ui->AutomateEdgeColor->isChecked();

	int Language = ui->Language->currentIndex();
	if (Language < 0 || Language > static_cast<int>(LC_ARRAY_COUNT(gLanguageLocales)))
		Language = 0;
	mOptions->Language = gLanguageLocales[Language];

	mOptions->CheckForUpdates = ui->checkForUpdates->currentIndex();
	mOptions->Preferences.mFixedAxes = ui->fixedDirectionKeys->isChecked();
	mOptions->Preferences.mAutoLoadMostRecent = ui->autoLoadMostRecent->isChecked();
	mOptions->Preferences.mRestoreTabLayout = ui->RestoreTabLayout->isChecked();

	if (!ui->antiAliasing->isChecked())
		mOptions->AASamples = 1;
	else if (ui->antiAliasingSamples->currentIndex() == 2)
		mOptions->AASamples = 8;
	else if (ui->antiAliasingSamples->currentIndex() == 1)
		mOptions->AASamples = 4;
	else
		mOptions->AASamples = 2;

	mOptions->Preferences.mDrawEdgeLines = ui->edgeLines->isChecked();
	mOptions->Preferences.mLineWidth = mLineWidthRange[0] + static_cast<float>(ui->LineWidthSlider->value()) * mLineWidthGranularity;
	mOptions->Preferences.mAllowLOD = ui->MeshLOD->isChecked();
	mOptions->Preferences.mMeshLODDistance = ui->MeshLODSlider->value() * mMeshLODMultiplier;
	mOptions->Preferences.mFadeSteps = ui->FadeSteps->isChecked();
	mOptions->Preferences.mHighlightNewParts = ui->HighlightNewParts->isChecked();

	mOptions->Preferences.mDrawGridStuds = ui->gridStuds->isChecked();
	mOptions->Preferences.mDrawGridLines = ui->gridLines->isChecked();
	mOptions->Preferences.mGridLineSpacing = gridLineSpacing;
	mOptions->Preferences.mDrawGridOrigin = ui->GridOriginCheckBox->isChecked();

	mOptions->Preferences.mBackgroundGradient = ui->BackgroundGradientRadio->isChecked();
	mOptions->Preferences.mDrawAxes = ui->AxisIconCheckBox->isChecked();
	mOptions->Preferences.mViewSphereEnabled = ui->ViewSphereSizeCombo->currentIndex() > 0;
	mOptions->Preferences.mViewSphereLocation = (lcViewSphereLocation)ui->ViewSphereLocationCombo->currentIndex();

	switch (ui->ViewSphereSizeCombo->currentIndex())
	{
	case 3:
		mOptions->Preferences.mViewSphereSize = 200;
		break;
	case 2:
		mOptions->Preferences.mViewSphereSize = 100;
		break;
	case 1:
		mOptions->Preferences.mViewSphereSize = 50;
		break;
	}

	mOptions->Preferences.mShadingMode = (lcShadingMode)ui->ShadingMode->currentIndex();

	mOptions->StudStyle = static_cast<lcStudStyle>(ui->studStyleCombo->currentIndex());

	mOptions->Preferences.mDrawPreviewAxis = ui->PreviewAxisIconCheckBox->isChecked();
	mOptions->Preferences.mPreviewViewSphereEnabled = ui->PreviewViewSphereSizeCombo->currentIndex() > 0;
	mOptions->Preferences.mPreviewViewSphereLocation = (lcViewSphereLocation)ui->PreviewViewSphereLocationCombo->currentIndex();

	switch (ui->PreviewViewSphereSizeCombo->currentIndex())
	{
	case 3:
		mOptions->Preferences.mPreviewViewSphereSize = 100;
		break;
	case 2:
		mOptions->Preferences.mPreviewViewSphereSize = 75;
		break;
	case 1:
		mOptions->Preferences.mPreviewViewSphereSize = 50;
		break;
	}

	QDialog::accept();
}

void lcQPreferencesDialog::on_partsLibraryBrowse_clicked()
{
	QString result = QFileDialog::getExistingDirectory(this, tr("Select Parts Library Folder"), ui->partsLibrary->text());

	if (!result.isEmpty())
		ui->partsLibrary->setText(QDir::toNativeSeparators(result));
}

void lcQPreferencesDialog::on_partsArchiveBrowse_clicked()
{
	QString result = QFileDialog::getOpenFileName(this, tr("Select Parts Library Archive"), ui->partsLibrary->text(), tr("Supported Archives (*.zip *.bin);;All Files (*.*)"));

	if (!result.isEmpty())
		ui->partsLibrary->setText(QDir::toNativeSeparators(result));
}

void lcQPreferencesDialog::on_ColorConfigBrowseButton_clicked()
{
	QString Result = QFileDialog::getOpenFileName(this, tr("Select Color Configuration File"), ui->ColorConfigEdit->text(), tr("Settings Files (*.ldr);;All Files (*.*)"));

	if (!Result.isEmpty())
		ui->ColorConfigEdit->setText(QDir::toNativeSeparators(Result));
}

void lcQPreferencesDialog::on_MinifigSettingsBrowseButton_clicked()
{
	QString Result = QFileDialog::getOpenFileName(this, tr("Select Minifig Settings File"), ui->MinifigSettingsEdit->text(), tr("Settings Files (*.ini);;All Files (*.*)"));

	if (!Result.isEmpty())
		ui->MinifigSettingsEdit->setText(QDir::toNativeSeparators(Result));
}

void lcQPreferencesDialog::on_povrayExecutableBrowse_clicked()
{
#ifdef Q_OS_WIN
	QString filter(tr("Executable Files (*.exe);;All Files (*.*)"));
#else
	QString filter(tr("All Files (*.*)"));
#endif

	QString result = QFileDialog::getOpenFileName(this, tr("Select POV-Ray Executable"), ui->povrayExecutable->text(), filter);

	if (!result.isEmpty())
		ui->povrayExecutable->setText(QDir::toNativeSeparators(result));
}

void lcQPreferencesDialog::on_lgeoPathBrowse_clicked()
{
	QString result = QFileDialog::getExistingDirectory(this, tr("Open LGEO Folder"), ui->lgeoPath->text());

	if (!result.isEmpty())
		ui->lgeoPath->setText(QDir::toNativeSeparators(result));
}

void lcQPreferencesDialog::on_ColorTheme_currentIndexChanged(int Index)
{
	Q_UNUSED(Index);

	if (QMessageBox::question(this, tr("Reset Colors"), tr("Would you like to also reset the interface colors to match the color theme?")) == QMessageBox::Yes)
		mOptions->Preferences.SetInterfaceColors(static_cast<lcColorTheme>(ui->ColorTheme->currentIndex()));
}

void lcQPreferencesDialog::ColorButtonClicked()
{
	QObject* Button = sender();
	QString Title;
	quint32* Color = nullptr;
	QColorDialog::ColorDialogOptions DialogOptions;

	if (Button == ui->BackgroundSolidColorButton)
	{
		Color = &mOptions->Preferences.mBackgroundSolidColor;
		Title = tr("Select Background Color");
	}
	else if (Button == ui->BackgroundGradient1ColorButton)
	{
		Color = &mOptions->Preferences.mBackgroundGradientColorTop;
		Title = tr("Select Gradient Top Color");
	}
	else if (Button == ui->BackgroundGradient2ColorButton)
	{
		Color = &mOptions->Preferences.mBackgroundGradientColorBottom;
		Title = tr("Select Gradient Bottom Color");
	}
	else if (Button == ui->AxesColorButton)
	{
		Color = &mOptions->Preferences.mAxesColor;
		Title = tr("Select Axes Color");
	}
	else if (Button == ui->TextColorButton)
	{
		Color = &mOptions->Preferences.mTextColor;
		Title = tr("Select Text Color");
	}
	else if (Button == ui->MarqueeBorderColorButton)
	{
		Color = &mOptions->Preferences.mMarqueeBorderColor;
		Title = tr("Select Marquee Border Color");
	}
	else if (Button == ui->MarqueeFillColorButton)
	{
		Color = &mOptions->Preferences.mMarqueeFillColor;
		Title = tr("Select Marquee Fill Color");
		DialogOptions = QColorDialog::ShowAlphaChannel;
	}
	else if (Button == ui->OverlayColorButton)
	{
		Color = &mOptions->Preferences.mOverlayColor;
		Title = tr("Select Overlay Color");
	}
	else if (Button == ui->ActiveViewColorButton)
	{
		Color = &mOptions->Preferences.mActiveViewColor;
		Title = tr("Select Active View Color");
	}
	else if (Button == ui->InactiveViewColorButton)
	{
		Color = &mOptions->Preferences.mInactiveViewColor;
		Title = tr("Select Inactive View Color");
	}
	else if (Button == ui->FadeStepsColor)
	{
		Color = &mOptions->Preferences.mFadeStepsColor;
		Title = tr("Select Fade Color");
		DialogOptions = QColorDialog::ShowAlphaChannel;
	}
	else if (Button == ui->HighlightNewPartsColor)
	{
		Color = &mOptions->Preferences.mHighlightNewPartsColor;
		Title = tr("Select Highlight Color");
		DialogOptions = QColorDialog::ShowAlphaChannel;
	}
	else if (Button == ui->gridStudColor)
	{
		Color = &mOptions->Preferences.mGridStudColor;
		Title = tr("Select Grid Stud Color");
		DialogOptions = QColorDialog::ShowAlphaChannel;
	}
	else if (Button == ui->gridLineColor)
	{
		Color = &mOptions->Preferences.mGridLineColor;
		Title = tr("Select Grid Line Color");
	}
	else if (Button == ui->ViewSphereColorButton)
	{
		Color = &mOptions->Preferences.mViewSphereColor;
		Title = tr("Select View Sphere Color");
	}
	else if (Button == ui->ViewSphereTextColorButton)
	{
		Color = &mOptions->Preferences.mViewSphereTextColor;
		Title = tr("Select View Sphere Text Color");
	}
	else if (Button == ui->ViewSphereHighlightColorButton)
	{
		Color = &mOptions->Preferences.mViewSphereHighlightColor;
		Title = tr("Select View Sphere Highlight Color");
	}
	else
		return;

	QColor oldColor = QColor(LC_RGBA_RED(*Color), LC_RGBA_GREEN(*Color), LC_RGBA_BLUE(*Color), LC_RGBA_ALPHA(*Color));
	QColor newColor = QColorDialog::getColor(oldColor, this, Title, DialogOptions);

	if (newColor == oldColor || !newColor.isValid())
		return;

	*Color = LC_RGBA(newColor.red(), newColor.green(), newColor.blue(), newColor.alpha());

	QPixmap pix(12, 12);

	newColor.setAlpha(255);
	pix.fill(newColor);
	((QToolButton*)Button)->setIcon(pix);
}

void lcQPreferencesDialog::on_antiAliasing_toggled()
{
	ui->antiAliasingSamples->setEnabled(ui->antiAliasing->isChecked());
}

void lcQPreferencesDialog::on_edgeLines_toggled()
{
	ui->LineWidthSlider->setEnabled(ui->edgeLines->isChecked());
	ui->LineWidthLabel->setEnabled(ui->edgeLines->isChecked());
}

void lcQPreferencesDialog::on_LineWidthSlider_valueChanged()
{
	float Value = mLineWidthRange[0] + static_cast<float>(ui->LineWidthSlider->value()) * mLineWidthGranularity;
	ui->LineWidthLabel->setText(QString::number(Value));
}

void lcQPreferencesDialog::on_MeshLODSlider_valueChanged()
{
	float Value = ui->MeshLODSlider->value() * mMeshLODMultiplier;
	ui->MeshLODLabel->setText(QString::number(static_cast<int>(Value)));
}

void lcQPreferencesDialog::on_FadeSteps_toggled()
{
	ui->FadeStepsColor->setEnabled(ui->FadeSteps->isChecked());
}

void lcQPreferencesDialog::on_HighlightNewParts_toggled()
{
	ui->HighlightNewPartsColor->setEnabled(ui->HighlightNewParts->isChecked());
}

void lcQPreferencesDialog::on_gridStuds_toggled()
{
	ui->gridStudColor->setEnabled(ui->gridStuds->isChecked());
}

void lcQPreferencesDialog::on_gridLines_toggled()
{
	ui->gridLineColor->setEnabled(ui->gridLines->isChecked());
	ui->gridLineSpacing->setEnabled(ui->gridLines->isChecked());
}

void lcQPreferencesDialog::on_PreviewViewSphereSizeCombo_currentIndexChanged(int Index)
{
	ui->PreviewViewSphereLocationCombo->setEnabled(Index != 0);
}

void lcQPreferencesDialog::on_ViewSphereSizeCombo_currentIndexChanged(int Index)
{
	ui->ViewSphereLocationCombo->setEnabled(Index != 0);
}

void lcQPreferencesDialog::on_AutomateEdgeColor_toggled()
{
	ui->AutomateEdgeColorButton->setEnabled(ui->AutomateEdgeColor->isChecked());
}

void lcQPreferencesDialog::on_studStyleCombo_currentIndexChanged(int index)
{
	ui->HighContrastButton->setEnabled(index > static_cast<int>(lcStudStyle::FlattenedLogo));
}

void lcQPreferencesDialog::AutomateEdgeColor()
{
	lcAutomateEdgeColorDialog Dialog(this, sender() == ui->HighContrastButton);
	if (Dialog.exec() == QDialog::Accepted)
	{
		mOptions->Preferences.mStudColor = Dialog.mStudColor;
		mOptions->Preferences.mStudEdgeColor = Dialog.mStudEdgeColor;
		mOptions->Preferences.mBlackEdgeColor = Dialog.mBlackEdgeColor;
		mOptions->Preferences.mDarkEdgeColor = Dialog.mDarkEdgeColor;
		mOptions->Preferences.mPartEdgeContrast = Dialog.mPartEdgeContrast;
		mOptions->Preferences.mPartColorValueLDIndex = Dialog.mPartColorValueLDIndex;
	}
}

void lcQPreferencesDialog::updateCategories()
{
	QTreeWidgetItem* CategoryItem;
	QTreeWidget* CategoriesTree = ui->categoriesTree;

	CategoriesTree->clear();

	for (int CategoryIndex = 0; CategoryIndex < static_cast<int>(mOptions->Categories.size()); CategoryIndex++)
	{
		CategoryItem = new QTreeWidgetItem(CategoriesTree, QStringList(mOptions->Categories[CategoryIndex].Name));
		CategoryItem->setData(0, CategoryRole, QVariant(CategoryIndex));
	}

	CategoryItem = new QTreeWidgetItem(CategoriesTree, QStringList(tr("Unassigned")));
	CategoryItem->setData(0, CategoryRole, QVariant(-1));
}

void lcQPreferencesDialog::updateParts()
{
	lcPiecesLibrary *Library = lcGetPiecesLibrary();
	QTreeWidget *tree = ui->partsTree;

	tree->clear();

	QList<QTreeWidgetItem*> selectedItems = ui->categoriesTree->selectedItems();

	if (selectedItems.empty())
		return;

	QTreeWidgetItem *categoryItem = selectedItems.first();
	int categoryIndex = categoryItem->data(0, CategoryRole).toInt();

	if (categoryIndex != -1)
	{
		lcArray<PieceInfo*> singleParts, groupedParts;

		Library->GetCategoryEntries(mOptions->Categories[categoryIndex].Keywords.constData(), false, singleParts, groupedParts);

		for (int partIndex = 0; partIndex < singleParts.GetSize(); partIndex++)
		{
			PieceInfo *info = singleParts[partIndex];

			QStringList rowList(info->m_strDescription);
			rowList.append(info->mFileName);

			new QTreeWidgetItem(tree, rowList);
		}
	}
	else
	{
		for (const auto& PartIt : Library->mPieces)
		{
			PieceInfo* Info = PartIt.second;

			for (categoryIndex = 0; categoryIndex < static_cast<int>(mOptions->Categories.size()); categoryIndex++)
			{
				if (Library->PieceInCategory(Info, mOptions->Categories[categoryIndex].Keywords.constData()))
					break;
			}

			if (categoryIndex == static_cast<int>(mOptions->Categories.size()))
			{
				QStringList rowList(Info->m_strDescription);
				rowList.append(Info->mFileName);

				new QTreeWidgetItem(tree, rowList);
			}
		}
	}

	tree->resizeColumnToContents(0);
	tree->resizeColumnToContents(1);
}

void lcQPreferencesDialog::on_newCategory_clicked()
{
	lcLibraryCategory category;

	lcQCategoryDialog dialog(this, &category);
	if (dialog.exec() != QDialog::Accepted)
		return;

	mOptions->CategoriesModified = true;
	mOptions->CategoriesDefault = false;
	mOptions->Categories.emplace_back(std::move(category));

	updateCategories();
	ui->categoriesTree->setCurrentItem(ui->categoriesTree->topLevelItem(static_cast<int>(mOptions->Categories.size()) - 1));
}

void lcQPreferencesDialog::on_editCategory_clicked()
{
	QList<QTreeWidgetItem*> selectedItems = ui->categoriesTree->selectedItems();

	if (selectedItems.empty())
		return;

	QTreeWidgetItem *categoryItem = selectedItems.first();
	int categoryIndex = categoryItem->data(0, CategoryRole).toInt();

	if (categoryIndex == -1)
		return;

	lcQCategoryDialog dialog(this, &mOptions->Categories[categoryIndex]);
	if (dialog.exec() != QDialog::Accepted)
		return;

	mOptions->CategoriesModified = true;
	mOptions->CategoriesDefault = false;

	updateCategories();
	ui->categoriesTree->setCurrentItem(ui->categoriesTree->topLevelItem(categoryIndex));
}

void lcQPreferencesDialog::on_deleteCategory_clicked()
{
	QList<QTreeWidgetItem*> selectedItems = ui->categoriesTree->selectedItems();

	if (selectedItems.empty())
		return;

	QTreeWidgetItem *categoryItem = selectedItems.first();
	int categoryIndex = categoryItem->data(0, CategoryRole).toInt();

	if (categoryIndex == -1)
		return;

	QString question = tr("Are you sure you want to delete the category '%1'?").arg(mOptions->Categories[categoryIndex].Name);
	if (QMessageBox::question(this, "LeoCAD", question, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	mOptions->CategoriesModified = true;
	mOptions->CategoriesDefault = false;
	mOptions->Categories.erase(mOptions->Categories.begin() + categoryIndex);

	updateCategories();
}

void lcQPreferencesDialog::on_importCategories_clicked()
{
	QString FileName = QFileDialog::getOpenFileName(this, tr("Import Categories"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	std::vector<lcLibraryCategory> Categories;
	if (!lcLoadCategories(FileName, Categories))
	{
		QMessageBox::warning(this, "LeoCAD", tr("Error loading categories file."));
		return;
	}

	mOptions->Categories = Categories;
	mOptions->CategoriesModified = true;
	mOptions->CategoriesDefault = false;
}

void lcQPreferencesDialog::on_exportCategories_clicked()
{
	QString FileName = QFileDialog::getSaveFileName(this, tr("Export Categories"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	if (!lcSaveCategories(FileName, mOptions->Categories))
	{
		QMessageBox::warning(this, "LeoCAD", tr("Error saving categories file."));
		return;
	}
}

void lcQPreferencesDialog::on_resetCategories_clicked()
{
	if (QMessageBox::question(this, "LeoCAD", tr("Are you sure you want to load the default categories?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	lcResetCategories(mOptions->Categories);

	mOptions->CategoriesModified = true;
	mOptions->CategoriesDefault = true;

	updateCategories();
}

bool lcQPreferencesDialog::eventFilter(QObject *object, QEvent *event)
{
	Q_UNUSED(object);

	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		int nextKey = keyEvent->key();
		if (nextKey == Qt::Key_Control || nextKey == Qt::Key_Shift || nextKey == Qt::Key_Meta || nextKey == Qt::Key_Alt)
			return true;

		Qt::KeyboardModifiers state = keyEvent->modifiers();
		QString text = QKeySequence(nextKey).toString();
		if ((state & Qt::ShiftModifier) && (text.isEmpty() || !text.at(0).isPrint() || text.at(0).isLetter() || text.at(0).isSpace()))
			nextKey |= Qt::SHIFT;
		if (state & Qt::ControlModifier)
			nextKey |= Qt::CTRL;
		if (state & Qt::MetaModifier)
			nextKey |= Qt::META;
		if (state & Qt::AltModifier)
			nextKey |= Qt::ALT;

		QKeySequence ks(nextKey);
		ui->shortcutEdit->setText(ks.toString(QKeySequence::NativeText));
		keyEvent->accept();

		return true;
	}

	if (event->type() == QEvent::Shortcut || event->type() == QEvent::KeyRelease || event->type() == QEvent::ShortcutOverride)
	{
		event->accept();
		return true;
	}

	return QDialog::eventFilter(object, event);
}

void lcQPreferencesDialog::updateCommandList()
{
	ui->commandList->clear();
	QMap<QString, QTreeWidgetItem*> sections;

	for (unsigned int actionIdx = 0; actionIdx < LC_NUM_COMMANDS; actionIdx++)
	{
		if (!gCommands[actionIdx].ID[0])
			continue;

		const QString identifier = tr(gCommands[actionIdx].ID);

		int pos = identifier.indexOf(QLatin1Char('.'));
		int subPos = identifier.indexOf(QLatin1Char('.'), pos + 1);
		if (subPos == -1)
			subPos = pos;

		const QString parentSection = identifier.left(pos);

		if (subPos != pos)
		{
			if (!sections.contains(parentSection))
			{
				QTreeWidgetItem *categoryItem = new QTreeWidgetItem(ui->commandList, QStringList(parentSection));
				QFont f = categoryItem->font(0);
				f.setBold(true);
				categoryItem->setFont(0, f);
				sections.insert(parentSection, categoryItem);
				ui->commandList->expandItem(categoryItem);
			}
		}

		const QString section = identifier.left(subPos);
		const QString subId = identifier.mid(subPos + 1);

		if (!sections.contains(section))
		{
			QTreeWidgetItem *parent = sections[parentSection];
			QTreeWidgetItem *categoryItem;
			QString subSection;

			if (pos != subPos)
				subSection = identifier.mid(pos + 1, subPos - pos - 1);
			else
				subSection = section;

			if (parent)
				categoryItem = new QTreeWidgetItem(parent, QStringList(subSection));
			else
				categoryItem = new QTreeWidgetItem(ui->commandList, QStringList(subSection));

			QFont f = categoryItem->font(0);
			f.setBold(true);
			categoryItem->setFont(0, f);
			sections.insert(section, categoryItem);
			ui->commandList->expandItem(categoryItem);
		}

		QTreeWidgetItem *item = new QTreeWidgetItem;
		QKeySequence sequence(mOptions->KeyboardShortcuts.mShortcuts[actionIdx]);
		item->setText(0, qApp->translate("Menu", gCommands[actionIdx].MenuName).remove('&').remove(QLatin1String("...")));
		item->setText(1, sequence.toString(QKeySequence::NativeText));
		item->setData(0, Qt::UserRole, QVariant::fromValue(actionIdx));

		if (mOptions->KeyboardShortcuts.mShortcuts[actionIdx] != gCommands[actionIdx].DefaultShortcut)
			setShortcutModified(item, true);

		sections[section]->addChild(item);
	}
}

void lcQPreferencesDialog::setShortcutModified(QTreeWidgetItem *treeItem, bool modified)
{
	QFont font = treeItem->font(0);
	font.setItalic(modified);
	treeItem->setFont(0, font);
	font.setBold(modified);
	treeItem->setFont(1, font);
}

void lcQPreferencesDialog::commandChanged(QTreeWidgetItem *current)
{
	if (!current || !current->data(0, Qt::UserRole).isValid())
	{
		ui->shortcutEdit->setText(QString());
		ui->shortcutGroup->setEnabled(false);
		return;
	}

	ui->shortcutGroup->setEnabled(true);

	int shortcutIndex = qvariant_cast<int>(current->data(0, Qt::UserRole));
	QKeySequence key(mOptions->KeyboardShortcuts.mShortcuts[shortcutIndex]);
	ui->shortcutEdit->setText(key.toString(QKeySequence::NativeText));
}

void lcQPreferencesDialog::on_KeyboardFilterEdit_textEdited(const QString& Text)
{
	if (Text.isEmpty())
	{
		std::function<void(QTreeWidgetItem*)> ShowItems = [&ShowItems](QTreeWidgetItem* ParentItem)
		{
			for (int ChildIdx = 0; ChildIdx < ParentItem->childCount(); ChildIdx++)
				ShowItems(ParentItem->child(ChildIdx));

			ParentItem->setHidden(false);
		};

		ShowItems(ui->commandList->invisibleRootItem());
	}
	else
	{
		std::function<bool(QTreeWidgetItem*,bool)> ShowItems = [&ShowItems, &Text](QTreeWidgetItem* ParentItem, bool ForceVisible)
		{
			ForceVisible |= (bool)ParentItem->text(0).contains(Text, Qt::CaseInsensitive) | (bool)ParentItem->text(1).contains(Text, Qt::CaseInsensitive);
			bool Visible = ForceVisible;

			for (int ChildIdx = 0; ChildIdx < ParentItem->childCount(); ChildIdx++)
				Visible |= ShowItems(ParentItem->child(ChildIdx), ForceVisible);

			ParentItem->setHidden(!Visible);

			return Visible;
		};

		ShowItems(ui->commandList->invisibleRootItem(), false);
	}
}

void lcQPreferencesDialog::on_shortcutAssign_clicked()
{
	QTreeWidgetItem* CurrentItem = ui->commandList->currentItem();

	if (!CurrentItem || !CurrentItem->data(0, Qt::UserRole).isValid())
		return;

	uint ShortcutIndex = CurrentItem->data(0, Qt::UserRole).toUInt();
	QString (&Shortcuts)[LC_NUM_COMMANDS] = mOptions->KeyboardShortcuts.mShortcuts;

	if (ShortcutIndex >= LC_ARRAY_COUNT(Shortcuts))
		return;

	QString NewShortcut = ui->shortcutEdit->text();

	if (!NewShortcut.isEmpty())
	{
		for (uint ExistingIndex = 0; ExistingIndex < LC_ARRAY_COUNT(Shortcuts); ExistingIndex++)
		{
			if (NewShortcut == Shortcuts[ExistingIndex] && ExistingIndex != ShortcutIndex)
			{
				QString ActionText = qApp->translate("Menu", gCommands[ExistingIndex].MenuName).remove('&').remove(QLatin1String("..."));
				QString QuestionText = tr("The shortcut '%1' is already assigned to '%2'. Do you want to replace it?").arg(NewShortcut, ActionText);

				if (QMessageBox::question(this, tr("Override Shortcut"), QuestionText, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
					return;

				mOptions->KeyboardShortcuts.mShortcuts[ExistingIndex].clear();

				std::function<QTreeWidgetItem* (QTreeWidgetItem*)> FindItem = [&FindItem, ExistingIndex](QTreeWidgetItem* ParentItem) -> QTreeWidgetItem*
				{
					for (int ChildIdx = 0; ChildIdx < ParentItem->childCount(); ChildIdx++)
					{
						QTreeWidgetItem* ChildItem = ParentItem->child(ChildIdx);
						uint ChildIndex = ChildItem->data(0, Qt::UserRole).toUInt();

						if (ChildIndex == ExistingIndex)
							return ChildItem;

						QTreeWidgetItem* ExistingItem = FindItem(ChildItem);

						if (ExistingItem)
							return ExistingItem;
					}

					return nullptr;
				};

				QTreeWidgetItem* ExistingItem = FindItem(ui->commandList->invisibleRootItem());

				if (ExistingItem)
				{
					ExistingItem->setText(1, QString());
					setShortcutModified(ExistingItem, gCommands[ShortcutIndex].DefaultShortcut[0] != 0);
				}
			}
		}
	}

	mOptions->KeyboardShortcuts.mShortcuts[ShortcutIndex] = NewShortcut;
	CurrentItem->setText(1, NewShortcut);

	setShortcutModified(CurrentItem, mOptions->KeyboardShortcuts.mShortcuts[ShortcutIndex] != gCommands[ShortcutIndex].DefaultShortcut);

	mOptions->KeyboardShortcutsModified = true;
	mOptions->KeyboardShortcutsDefault = false;
}

void lcQPreferencesDialog::on_shortcutRemove_clicked()
{
	ui->shortcutEdit->setText(QString());

	on_shortcutAssign_clicked();
}

void lcQPreferencesDialog::on_shortcutsImport_clicked()
{
	QString FileName = QFileDialog::getOpenFileName(this, tr("Import shortcuts"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	lcKeyboardShortcuts Shortcuts;
	if (!Shortcuts.Load(FileName))
	{
		QMessageBox::warning(this, "LeoCAD", tr("Error loading keyboard shortcuts file."));
		return;
	}

	mOptions->KeyboardShortcuts = Shortcuts;

	mOptions->KeyboardShortcutsModified = true;
	mOptions->KeyboardShortcutsDefault = false;
}

void lcQPreferencesDialog::on_shortcutsExport_clicked()
{
	QString FileName = QFileDialog::getSaveFileName(this, tr("Export shortcuts"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	if (!mOptions->KeyboardShortcuts.Save(FileName))
	{
		QMessageBox::warning(this, "LeoCAD", tr("Error saving keyboard shortcuts file."));
		return;
	}
}

void lcQPreferencesDialog::on_shortcutsReset_clicked()
{
	if (QMessageBox::question(this, "LeoCAD", tr("Are you sure you want to load the default keyboard shortcuts?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	mOptions->KeyboardShortcuts.Reset();
	updateCommandList();

	mOptions->KeyboardShortcutsModified = true;
	mOptions->KeyboardShortcutsDefault = true;
}

void lcQPreferencesDialog::UpdateMouseTree()
{
	ui->mouseTree->clear();

	for (int ToolIdx = 0; ToolIdx < static_cast<int>(lcTool::Count); ToolIdx++)
		UpdateMouseTreeItem(ToolIdx);
}

void lcQPreferencesDialog::UpdateMouseTreeItem(int ItemIndex)
{
	auto GetShortcutText = [](Qt::MouseButton Button, Qt::KeyboardModifiers Modifiers)
	{
		QString Shortcut = QKeySequence(Modifiers).toString(QKeySequence::NativeText);

		switch (Button)
		{
		case Qt::LeftButton:
			Shortcut += tr("Left Button");
			break;

		case Qt::MiddleButton:
			Shortcut += tr("Middle Button");
			break;

		case Qt::RightButton:
			Shortcut += tr("Right Button");
			break;

		default:
			Shortcut.clear();
		}
		return Shortcut;
	};

	QString Shortcut1 = GetShortcutText(mOptions->MouseShortcuts.mShortcuts[ItemIndex].Button1, mOptions->MouseShortcuts.mShortcuts[ItemIndex].Modifiers1);
	QString Shortcut2 = GetShortcutText(mOptions->MouseShortcuts.mShortcuts[ItemIndex].Button2, mOptions->MouseShortcuts.mShortcuts[ItemIndex].Modifiers2);

	QTreeWidgetItem* Item = ui->mouseTree->topLevelItem(ItemIndex);

	if (Item)
	{
		Item->setText(1, Shortcut1);
		Item->setText(2, Shortcut2);
	}
	else
		new QTreeWidgetItem(ui->mouseTree, QStringList() << tr(gToolNames[ItemIndex]) << Shortcut1 << Shortcut2);
}

void lcQPreferencesDialog::on_mouseAssign_clicked()
{
	QTreeWidgetItem* Current = ui->mouseTree->currentItem();

	if (!Current)
		return;

	int ButtonIndex = ui->mouseButton->currentIndex();
	Qt::MouseButton Button = Qt::NoButton;
	Qt::KeyboardModifiers Modifiers = Qt::NoModifier;

	if (ButtonIndex)
	{
		switch (ButtonIndex)
		{
		case 1:
			Button = Qt::LeftButton;
			break;

		case 2:
			Button = Qt::MiddleButton;
			break;

		case 3:
			Button = Qt::RightButton;
			break;
		}

		if (ui->mouseControl->isChecked())
			Modifiers |= Qt::ControlModifier;

		if (ui->mouseShift->isChecked())
			Modifiers |= Qt::ShiftModifier;

		if (ui->mouseAlt->isChecked())
			Modifiers |= Qt::AltModifier;

		for (int ToolIdx = 0; ToolIdx < static_cast<int>(lcTool::Count); ToolIdx++)
		{
			if (ToolIdx == ButtonIndex)
				continue;

			if (mOptions->MouseShortcuts.mShortcuts[ToolIdx].Button2 == Button && mOptions->MouseShortcuts.mShortcuts[ToolIdx].Modifiers2 == Modifiers)
			{
				if (QMessageBox::question(this, tr("Override Shortcut"), tr("This shortcut is already assigned to '%1', do you want to replace it?").arg(tr(gToolNames[ToolIdx])), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
					return;

				mOptions->MouseShortcuts.mShortcuts[ToolIdx].Button2 = Qt::NoButton;
				mOptions->MouseShortcuts.mShortcuts[ToolIdx].Modifiers2 = Qt::NoModifier;
			}

			if (mOptions->MouseShortcuts.mShortcuts[ToolIdx].Button1 == Button && mOptions->MouseShortcuts.mShortcuts[ToolIdx].Modifiers1 == Modifiers)
			{
				if (QMessageBox::question(this, tr("Override Shortcut"), tr("This shortcut is already assigned to '%1', do you want to replace it?").arg(tr(gToolNames[ToolIdx])), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
					return;

				mOptions->MouseShortcuts.mShortcuts[ToolIdx].Button1 = mOptions->MouseShortcuts.mShortcuts[ToolIdx].Button2;
				mOptions->MouseShortcuts.mShortcuts[ToolIdx].Modifiers1 = mOptions->MouseShortcuts.mShortcuts[ToolIdx].Modifiers2;
				mOptions->MouseShortcuts.mShortcuts[ToolIdx].Button2 = Qt::NoButton;
				mOptions->MouseShortcuts.mShortcuts[ToolIdx].Modifiers2 = Qt::NoModifier;

				UpdateMouseTreeItem(ToolIdx);
			}
		}
	}

	int ItemIndex = ui->mouseTree->indexOfTopLevelItem(Current);
	mOptions->MouseShortcuts.mShortcuts[ItemIndex].Button2 = mOptions->MouseShortcuts.mShortcuts[ItemIndex].Button1;
	mOptions->MouseShortcuts.mShortcuts[ItemIndex].Modifiers2 = mOptions->MouseShortcuts.mShortcuts[ItemIndex].Modifiers1;
	mOptions->MouseShortcuts.mShortcuts[ItemIndex].Button1 = Button;
	mOptions->MouseShortcuts.mShortcuts[ItemIndex].Modifiers1 = Modifiers;

	mOptions->MouseShortcutsModified = true;
	mOptions->MouseShortcutsDefault = false;

	UpdateMouseTreeItem(ItemIndex);
}

void lcQPreferencesDialog::on_mouseRemove_clicked()
{
	QTreeWidgetItem* Current = ui->mouseTree->currentItem();

	if (!Current)
		return;

	int ItemIndex = ui->mouseTree->indexOfTopLevelItem(Current);
	mOptions->MouseShortcuts.mShortcuts[ItemIndex].Button1 = mOptions->MouseShortcuts.mShortcuts[ItemIndex].Button2;
	mOptions->MouseShortcuts.mShortcuts[ItemIndex].Modifiers1 = mOptions->MouseShortcuts.mShortcuts[ItemIndex].Modifiers2;
	mOptions->MouseShortcuts.mShortcuts[ItemIndex].Button2 = Qt::NoButton;
	mOptions->MouseShortcuts.mShortcuts[ItemIndex].Modifiers2 = Qt::NoModifier;

	mOptions->MouseShortcutsModified = true;
	mOptions->MouseShortcutsDefault = false;

	UpdateMouseTreeItem(ItemIndex);
	MouseTreeItemChanged(Current);
}

void lcQPreferencesDialog::on_MouseImportButton_clicked()
{
	QString FileName = QFileDialog::getOpenFileName(this, tr("Import Shortcuts"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	lcMouseShortcuts Shortcuts;
	if (!Shortcuts.Load(FileName))
	{
		QMessageBox::warning(this, "LeoCAD", tr("Error loading mouse shortcuts file."));
		return;
	}

	mOptions->MouseShortcuts = Shortcuts;
	UpdateMouseTree();

	mOptions->MouseShortcutsModified = true;
	mOptions->MouseShortcutsDefault = false;
}

void lcQPreferencesDialog::on_MouseExportButton_clicked()
{
	QString FileName = QFileDialog::getSaveFileName(this, tr("Export Shortcuts"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	if (!mOptions->MouseShortcuts.Save(FileName))
		QMessageBox::warning(this, "LeoCAD", tr("Error saving mouse shortcuts file."));
}

void lcQPreferencesDialog::on_mouseReset_clicked()
{
	if (QMessageBox::question(this, "LeoCAD", tr("Are you sure you want to load the default mouse shortcuts?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	mOptions->MouseShortcuts.Reset();
	UpdateMouseTree();

	mOptions->MouseShortcutsModified = true;
	mOptions->MouseShortcutsDefault = true;
}

void lcQPreferencesDialog::MouseTreeItemChanged(QTreeWidgetItem* Current)
{
	if (!Current)
	{
		ui->MouseShortcutGroup->setEnabled(false);
		return;
	}

	ui->MouseShortcutGroup->setEnabled(true);

	int ToolIndex = ui->mouseTree->indexOfTopLevelItem(Current);

	Qt::MouseButton Button = mOptions->MouseShortcuts.mShortcuts[ToolIndex].Button1;

	switch (Button)
	{
	case Qt::LeftButton:
		ui->mouseButton->setCurrentIndex(1);
		break;

	case Qt::MiddleButton:
		ui->mouseButton->setCurrentIndex(2);
		break;

	case Qt::RightButton:
		ui->mouseButton->setCurrentIndex(3);
		break;

	default:
		ui->mouseButton->setCurrentIndex(0);
		break;
	}

	Qt::KeyboardModifiers Modifiers = mOptions->MouseShortcuts.mShortcuts[ToolIndex].Modifiers1;
	ui->mouseControl->setChecked((Modifiers & Qt::ControlModifier) != 0);
	ui->mouseShift->setChecked((Modifiers & Qt::ShiftModifier) != 0);
	ui->mouseAlt->setChecked((Modifiers & Qt::AltModifier) != 0);
}

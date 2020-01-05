#include "lc_global.h"
#include "lc_qpreferencesdialog.h"
#include "ui_lc_qpreferencesdialog.h"
#include "lc_qutils.h"
#include "lc_qcategorydialog.h"
#include "lc_basewindow.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_qutils.h"
#include "lc_glextensions.h"
#include "pieceinf.h"

static const char* gLanguageLocales[] =
{
	"", "de_DE", "en_US", "fr_FR", "pt_PT"
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

	ui->lineWidth->setValidator(new QDoubleValidator(ui->lineWidth));
	connect(ui->gridStudColor, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->gridLineColor, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->ViewSphereColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->ViewSphereTextColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->ViewSphereHighlightColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));
	connect(ui->categoriesTree, SIGNAL(itemSelectionChanged()), this, SLOT(updateParts()));
	ui->shortcutEdit->installEventFilter(this);
	connect(ui->commandList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(commandChanged(QTreeWidgetItem*)));
	connect(ui->mouseTree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(MouseTreeItemChanged(QTreeWidgetItem*)));

	ui->partsLibrary->setText(mOptions->LibraryPath);
	ui->ColorConfigEdit->setText(mOptions->ColorConfigPath);
	ui->MinifigSettingsEdit->setText(mOptions->MinifigSettingsPath);
	ui->povrayExecutable->setText(mOptions->POVRayPath);
	ui->lgeoPath->setText(mOptions->LGEOPath);
	ui->authorName->setText(mOptions->DefaultAuthor);
	ui->mouseSensitivity->setValue(mOptions->Preferences.mMouseSensitivity);
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

	ui->antiAliasing->setChecked(mOptions->AASamples != 1);
	if (mOptions->AASamples == 8)
		ui->antiAliasingSamples->setCurrentIndex(2);
	else if (mOptions->AASamples == 4)
		ui->antiAliasingSamples->setCurrentIndex(1);
	else
		ui->antiAliasingSamples->setCurrentIndex(0);
	ui->edgeLines->setChecked(mOptions->Preferences.mDrawEdgeLines);
	ui->lineWidth->setText(lcFormatValueLocalized(mOptions->Preferences.mLineWidth));
	ui->MeshLOD->setChecked(mOptions->Preferences.mAllowLOD);
	ui->FadeSteps->setChecked(mOptions->Preferences.mFadeSteps);
	ui->gridStuds->setChecked(mOptions->Preferences.mDrawGridStuds);
	ui->gridLines->setChecked(mOptions->Preferences.mDrawGridLines);
	ui->gridLineSpacing->setText(QString::number(mOptions->Preferences.mGridLineSpacing));
	ui->axisIcon->setChecked(mOptions->Preferences.mDrawAxes);

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

	ui->studLogo->setChecked(mOptions->StudLogo);
	if (ui->studLogo->isChecked())
		ui->studLogoCombo->setCurrentIndex(mOptions->StudLogo - 1);
	else
		ui->studLogoCombo->setCurrentIndex(mOptions->StudLogo);

	if (!gSupportsShaderObjects)
		ui->ShadingMode->removeItem(LC_SHADING_DEFAULT_LIGHTS);
	ui->ShadingMode->setCurrentIndex(mOptions->Preferences.mShadingMode);

	QPixmap pix(12, 12);

	pix.fill(QColor(LC_RGBA_RED(mOptions->Preferences.mGridStudColor), LC_RGBA_GREEN(mOptions->Preferences.mGridStudColor), LC_RGBA_BLUE(mOptions->Preferences.mGridStudColor)));
	ui->gridStudColor->setIcon(pix);

	pix.fill(QColor(LC_RGBA_RED(mOptions->Preferences.mGridLineColor), LC_RGBA_GREEN(mOptions->Preferences.mGridLineColor), LC_RGBA_BLUE(mOptions->Preferences.mGridLineColor)));
	ui->gridLineColor->setIcon(pix);

	pix.fill(QColor(LC_RGBA_RED(mOptions->Preferences.mViewSphereColor), LC_RGBA_GREEN(mOptions->Preferences.mViewSphereColor), LC_RGBA_BLUE(mOptions->Preferences.mViewSphereColor)));
	ui->ViewSphereColorButton->setIcon(pix);

	pix.fill(QColor(LC_RGBA_RED(mOptions->Preferences.mViewSphereTextColor), LC_RGBA_GREEN(mOptions->Preferences.mViewSphereTextColor), LC_RGBA_BLUE(mOptions->Preferences.mViewSphereTextColor)));
	ui->ViewSphereTextColorButton->setIcon(pix);

	pix.fill(QColor(LC_RGBA_RED(mOptions->Preferences.mViewSphereHighlightColor), LC_RGBA_GREEN(mOptions->Preferences.mViewSphereHighlightColor), LC_RGBA_BLUE(mOptions->Preferences.mViewSphereHighlightColor)));
	ui->ViewSphereHighlightColorButton->setIcon(pix);

	on_studLogo_toggled();
	on_antiAliasing_toggled();
	on_edgeLines_toggled();
	on_gridStuds_toggled();
	on_gridLines_toggled();
	on_ViewSphereSizeCombo_currentIndexChanged(ui->ViewSphereSizeCombo->currentIndex());

	updateCategories();
	ui->categoriesTree->setCurrentItem(ui->categoriesTree->topLevelItem(0));

	updateCommandList();
	new lcQTreeWidgetColumnStretcher(ui->commandList, 0);
	commandChanged(nullptr);

	UpdateMouseTree();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	ui->mouseTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui->mouseTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui->mouseTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
#else
	ui->mouseTree->header()->setResizeMode(0, QHeaderView::Stretch);
	ui->mouseTree->header()->setResizeMode(1, QHeaderView::ResizeToContents);
	ui->mouseTree->header()->setResizeMode(2, QHeaderView::ResizeToContents);
#endif
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
	mOptions->Preferences.mLineWidth = lcParseValueLocalized(ui->lineWidth->text());
	mOptions->Preferences.mAllowLOD = ui->MeshLOD->isChecked();
	mOptions->Preferences.mFadeSteps = ui->FadeSteps->isChecked();

	mOptions->Preferences.mDrawGridStuds = ui->gridStuds->isChecked();
	mOptions->Preferences.mDrawGridLines = ui->gridLines->isChecked();
	mOptions->Preferences.mGridLineSpacing = gridLineSpacing;

	mOptions->Preferences.mDrawAxes = ui->axisIcon->isChecked();
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
	default:
		mOptions->Preferences.mViewSphereEnabled = 0;
		break;
	}

	mOptions->Preferences.mShadingMode = (lcShadingMode)ui->ShadingMode->currentIndex();

	if (ui->studLogoCombo->isEnabled())
		mOptions->StudLogo = ui->studLogoCombo->currentIndex() + 1;
	else
		mOptions->StudLogo = 0;

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

void lcQPreferencesDialog::ColorButtonClicked()
{
	QObject *button = sender();
	QString title;
	quint32 *color = nullptr;
	QColorDialog::ColorDialogOptions dialogOptions;

	if (button == ui->gridStudColor)
	{
		color = &mOptions->Preferences.mGridStudColor;
		title = tr("Select Grid Stud Color");
		dialogOptions = QColorDialog::ShowAlphaChannel;
	}
	else if (button == ui->gridLineColor)
	{
		color = &mOptions->Preferences.mGridLineColor;
		title = tr("Select Grid Line Color");
		dialogOptions = 0;
	}
	else if (button == ui->ViewSphereColorButton)
	{
		color = &mOptions->Preferences.mViewSphereColor;
		title = tr("Select View Sphere Color");
		dialogOptions = 0;
	}
	else if (button == ui->ViewSphereTextColorButton)
	{
		color = &mOptions->Preferences.mViewSphereTextColor;
		title = tr("Select View Sphere Text Color");
		dialogOptions = 0;
	}
	else if (button == ui->ViewSphereHighlightColorButton)
	{
		color = &mOptions->Preferences.mViewSphereHighlightColor;
		title = tr("Select View Sphere Highlight Color");
		dialogOptions = 0;
	}
	else
		return;

	QColor oldColor = QColor(LC_RGBA_RED(*color), LC_RGBA_GREEN(*color), LC_RGBA_BLUE(*color), LC_RGBA_ALPHA(*color));
	QColor newColor = QColorDialog::getColor(oldColor, this, title, dialogOptions);

	if (newColor == oldColor || !newColor.isValid())
		return;

	*color = LC_RGBA(newColor.red(), newColor.green(), newColor.blue(), newColor.alpha());

	QPixmap pix(12, 12);

	pix.fill(newColor);
	((QToolButton*)button)->setIcon(pix);
}

void lcQPreferencesDialog::on_studLogo_toggled()
{
   ui->studLogoCombo->setEnabled(ui->studLogo->isChecked());
}

void lcQPreferencesDialog::on_antiAliasing_toggled()
{
	ui->antiAliasingSamples->setEnabled(ui->antiAliasing->isChecked());
}

void lcQPreferencesDialog::on_edgeLines_toggled()
{
	ui->lineWidth->setEnabled(ui->edgeLines->isChecked());
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

void lcQPreferencesDialog::on_ViewSphereSizeCombo_currentIndexChanged(int Index)
{
	bool Enabled = Index != 0;

	ui->ViewSphereLocationCombo->setEnabled(Enabled);
	ui->ViewSphereColorButton->setEnabled(Enabled);
	ui->ViewSphereTextColorButton->setEnabled(Enabled);
	ui->ViewSphereHighlightColorButton->setEnabled(Enabled);
}

void lcQPreferencesDialog::updateCategories()
{
	QTreeWidgetItem *categoryItem;
	QTreeWidget *tree = ui->categoriesTree;

	tree->clear();

	for (int categoryIndex = 0; categoryIndex < mOptions->Categories.GetSize(); categoryIndex++)
	{
		categoryItem = new QTreeWidgetItem(tree, QStringList(mOptions->Categories[categoryIndex].Name));
		categoryItem->setData(0, CategoryRole, QVariant(categoryIndex));
	}

	categoryItem = new QTreeWidgetItem(tree, QStringList(tr("Unassigned")));
	categoryItem->setData(0, CategoryRole, QVariant(-1));
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

			for (categoryIndex = 0; categoryIndex < mOptions->Categories.GetSize(); categoryIndex++)
			{
				if (Library->PieceInCategory(Info, mOptions->Categories[categoryIndex].Keywords.constData()))
					break;
			}

			if (categoryIndex == mOptions->Categories.GetSize())
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
	mOptions->Categories.Add(category);

	updateCategories();
	ui->categoriesTree->setCurrentItem(ui->categoriesTree->topLevelItem(mOptions->Categories.GetSize() - 1));
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
	mOptions->Categories.RemoveIndex(categoryIndex);

	updateCategories();
}

void lcQPreferencesDialog::on_importCategories_clicked()
{
	QString FileName = QFileDialog::getOpenFileName(this, tr("Import Categories"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	lcArray<lcLibraryCategory> Categories;
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

	for (int actionIdx = 0; actionIdx < LC_NUM_COMMANDS; actionIdx++)
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
		item->setText(0, subId);
		item->setText(1, sequence.toString(QKeySequence::NativeText));
		item->setData(0, Qt::UserRole, qVariantFromValue(actionIdx));

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
	QTreeWidgetItem *current = ui->commandList->currentItem();

	if (!current || !current->data(0, Qt::UserRole).isValid())
		return;

	int shortcutIndex = qvariant_cast<int>(current->data(0, Qt::UserRole));
	mOptions->KeyboardShortcuts.mShortcuts[shortcutIndex] = ui->shortcutEdit->text();

	current->setText(1, ui->shortcutEdit->text());

	setShortcutModified(current, mOptions->KeyboardShortcuts.mShortcuts[shortcutIndex] != gCommands[shortcutIndex].DefaultShortcut);

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

	for (int ToolIdx = 0; ToolIdx < LC_NUM_TOOLS; ToolIdx++)
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

#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
		case Qt::MiddleButton:
			Shortcut += tr("Middle Button");
			break;
#endif

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

#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
		case 2:
			Button = Qt::MiddleButton;
			break;
#endif

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

		for (int ToolIdx = 0; ToolIdx < LC_NUM_TOOLS; ToolIdx++)
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

#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
	case Qt::MiddleButton:
		ui->mouseButton->setCurrentIndex(2);
		break;
#endif

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

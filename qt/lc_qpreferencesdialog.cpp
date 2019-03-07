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

lcQPreferencesDialog::lcQPreferencesDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQPreferencesDialog)
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

	options = (lcPreferencesDialogOptions*)data;

	ui->authorName->setText(options->DefaultAuthor);
	ui->partsLibrary->setText(options->LibraryPath);
	ui->povrayExecutable->setText(options->POVRayPath);
	ui->lgeoPath->setText(options->LGEOPath);
	ui->mouseSensitivity->setValue(options->Preferences.mMouseSensitivity);
	ui->checkForUpdates->setCurrentIndex(options->CheckForUpdates);
	ui->fixedDirectionKeys->setChecked(options->Preferences.mFixedAxes);
	ui->autoLoadMostRecent->setChecked(options->Preferences.autoLoadMostRecent);

	ui->antiAliasing->setChecked(options->AASamples != 1);
	if (options->AASamples == 8)
		ui->antiAliasingSamples->setCurrentIndex(2);
	else if (options->AASamples == 4)
		ui->antiAliasingSamples->setCurrentIndex(1);
	else
		ui->antiAliasingSamples->setCurrentIndex(0);
	ui->edgeLines->setChecked(options->Preferences.mDrawEdgeLines);
	ui->lineWidth->setText(lcFormatValueLocalized(options->Preferences.mLineWidth));
	ui->gridStuds->setChecked(options->Preferences.mDrawGridStuds);
	ui->gridLines->setChecked(options->Preferences.mDrawGridLines);
	ui->gridLineSpacing->setText(QString::number(options->Preferences.mGridLineSpacing));
	ui->axisIcon->setChecked(options->Preferences.mDrawAxes);

	ui->ViewSphereLocationCombo->setCurrentIndex((int)options->Preferences.mViewSphereLocation);

	switch (options->Preferences.mViewSphereSize)
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

	if (!gSupportsShaderObjects)
		ui->ShadingMode->removeItem(LC_SHADING_DEFAULT_LIGHTS);
	ui->ShadingMode->setCurrentIndex(options->Preferences.mShadingMode);

	QPixmap pix(12, 12);

	pix.fill(QColor(LC_RGBA_RED(options->Preferences.mGridStudColor), LC_RGBA_GREEN(options->Preferences.mGridStudColor), LC_RGBA_BLUE(options->Preferences.mGridStudColor)));
	ui->gridStudColor->setIcon(pix);

	pix.fill(QColor(LC_RGBA_RED(options->Preferences.mGridLineColor), LC_RGBA_GREEN(options->Preferences.mGridLineColor), LC_RGBA_BLUE(options->Preferences.mGridLineColor)));
	ui->gridLineColor->setIcon(pix);

	pix.fill(QColor(LC_RGBA_RED(options->Preferences.mViewSphereColor), LC_RGBA_GREEN(options->Preferences.mViewSphereColor), LC_RGBA_BLUE(options->Preferences.mViewSphereColor)));
	ui->ViewSphereColorButton->setIcon(pix);

	pix.fill(QColor(LC_RGBA_RED(options->Preferences.mViewSphereTextColor), LC_RGBA_GREEN(options->Preferences.mViewSphereTextColor), LC_RGBA_BLUE(options->Preferences.mViewSphereTextColor)));
	ui->ViewSphereTextColorButton->setIcon(pix);

	pix.fill(QColor(LC_RGBA_RED(options->Preferences.mViewSphereHighlightColor), LC_RGBA_GREEN(options->Preferences.mViewSphereHighlightColor), LC_RGBA_BLUE(options->Preferences.mViewSphereHighlightColor)));
	ui->ViewSphereHighlightColorButton->setIcon(pix);

	on_antiAliasing_toggled();
	on_edgeLines_toggled();
	on_gridStuds_toggled();
	on_gridLines_toggled();
	on_ViewSphereSizeCombo_currentIndexChanged((int)options->Preferences.mViewSphereLocation);

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

	options->DefaultAuthor = ui->authorName->text();
	options->LibraryPath = ui->partsLibrary->text();
	options->POVRayPath = ui->povrayExecutable->text();
	options->LGEOPath = ui->lgeoPath->text();
	options->Preferences.mMouseSensitivity = ui->mouseSensitivity->value();
	options->CheckForUpdates = ui->checkForUpdates->currentIndex();
	options->Preferences.mFixedAxes = ui->fixedDirectionKeys->isChecked();
	options->Preferences.autoLoadMostRecent = ui->autoLoadMostRecent->isChecked();

	if (!ui->antiAliasing->isChecked())
		options->AASamples = 1;
	else if (ui->antiAliasingSamples->currentIndex() == 2)
		options->AASamples = 8;
	else if (ui->antiAliasingSamples->currentIndex() == 1)
		options->AASamples = 4;
	else
		options->AASamples = 2;

	options->Preferences.mDrawEdgeLines = ui->edgeLines->isChecked();
	options->Preferences.mLineWidth = lcParseValueLocalized(ui->lineWidth->text());

	options->Preferences.mDrawGridStuds = ui->gridStuds->isChecked();
	options->Preferences.mDrawGridLines = ui->gridLines->isChecked();
	options->Preferences.mGridLineSpacing = gridLineSpacing;

	options->Preferences.mDrawAxes = ui->axisIcon->isChecked();
	options->Preferences.mViewSphereLocation = (lcViewSphereLocation)ui->ViewSphereLocationCombo->currentIndex();

	switch (ui->ViewSphereSizeCombo->currentIndex())
	{
	case 3:
		options->Preferences.mViewSphereSize = 200;
		break;
	case 2:
		options->Preferences.mViewSphereSize = 100;
		break;
	case 1:
		options->Preferences.mViewSphereSize = 50;
		break;
	default:
		options->Preferences.mViewSphereSize = 0;
		break;
	}

	options->Preferences.mShadingMode = (lcShadingMode)ui->ShadingMode->currentIndex();

	QDialog::accept();
}

void lcQPreferencesDialog::on_partsLibraryBrowse_clicked()
{
	QString result = QFileDialog::getExistingDirectory(this, tr("Select Parts Library Folder..."), ui->partsLibrary->text());

	if (!result.isEmpty())
		ui->partsLibrary->setText(QDir::toNativeSeparators(result));
}

void lcQPreferencesDialog::on_partsArchiveBrowse_clicked()
{
	QString result = QFileDialog::getOpenFileName(this, tr("Select Parts Library Archive..."), ui->partsLibrary->text(), tr("Supported Archives (*.zip *.bin);;All Files (*.*)"));

	if (!result.isEmpty())
		ui->partsLibrary->setText(QDir::toNativeSeparators(result));
}

void lcQPreferencesDialog::on_povrayExecutableBrowse_clicked()
{
#ifdef Q_OS_WIN
	QString filter(tr("Executable Files (*.exe);;All Files (*.*)"));
#else
	QString filter(tr("All Files (*.*)"));
#endif

	QString result = QFileDialog::getOpenFileName(this, tr("Open POV-Ray Executable"), ui->povrayExecutable->text(), filter);

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
		color = &options->Preferences.mGridStudColor;
		title = tr("Select Grid Stud Color");
		dialogOptions = QColorDialog::ShowAlphaChannel;
	}
	else if (button == ui->gridLineColor)
	{
		color = &options->Preferences.mGridLineColor;
		title = tr("Select Grid Line Color");
		dialogOptions = 0;
	}
	else if (button == ui->ViewSphereColorButton)
	{
		color = &options->Preferences.mViewSphereColor;
		title = tr("Select View Sphere Color");
		dialogOptions = 0;
	}
	else if (button == ui->ViewSphereTextColorButton)
	{
		color = &options->Preferences.mViewSphereTextColor;
		title = tr("Select View Sphere Text Color");
		dialogOptions = 0;
	}
	else if (button == ui->ViewSphereHighlightColorButton)
	{
		color = &options->Preferences.mViewSphereHighlightColor;
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

	for (int categoryIndex = 0; categoryIndex < options->Categories.GetSize(); categoryIndex++)
	{
		categoryItem = new QTreeWidgetItem(tree, QStringList(options->Categories[categoryIndex].Name));
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

		Library->GetCategoryEntries(options->Categories[categoryIndex].Keywords.constData(), false, singleParts, groupedParts);

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

			for (categoryIndex = 0; categoryIndex < options->Categories.GetSize(); categoryIndex++)
			{
				if (Library->PieceInCategory(Info, options->Categories[categoryIndex].Keywords.constData()))
					break;
			}

			if (categoryIndex == options->Categories.GetSize())
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

	options->CategoriesModified = true;
	options->CategoriesDefault = false;
	options->Categories.Add(category);

	updateCategories();
	ui->categoriesTree->setCurrentItem(ui->categoriesTree->topLevelItem(options->Categories.GetSize() - 1));
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

	lcQCategoryDialog dialog(this, &options->Categories[categoryIndex]);
	if (dialog.exec() != QDialog::Accepted)
		return;

	options->CategoriesModified = true;
	options->CategoriesDefault = false;

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

	QString question = tr("Are you sure you want to delete the category '%1'?").arg(options->Categories[categoryIndex].Name);
	if (QMessageBox::question(this, "LeoCAD", question, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	options->CategoriesModified = true;
	options->CategoriesDefault = false;
	options->Categories.RemoveIndex(categoryIndex);

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

	options->Categories = Categories;
	options->CategoriesModified = true;
	options->CategoriesDefault = false;
}

void lcQPreferencesDialog::on_exportCategories_clicked()
{
	QString FileName = QFileDialog::getSaveFileName(this, tr("Export Categories"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	if (!lcSaveCategories(FileName, options->Categories))
	{
		QMessageBox::warning(this, "LeoCAD", tr("Error saving categories file."));
		return;
	}
}

void lcQPreferencesDialog::on_resetCategories_clicked()
{
	if (QMessageBox::question(this, "LeoCAD", tr("Are you sure you want to load the default categories?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	lcResetCategories(options->Categories);

	options->CategoriesModified = true;
	options->CategoriesDefault = true;

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
		QKeySequence sequence(options->KeyboardShortcuts.mShortcuts[actionIdx]);
		item->setText(0, subId);
		item->setText(1, sequence.toString(QKeySequence::NativeText));
		item->setData(0, Qt::UserRole, qVariantFromValue(actionIdx));

		if (options->KeyboardShortcuts.mShortcuts[actionIdx] != gCommands[actionIdx].DefaultShortcut)
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
	QKeySequence key(options->KeyboardShortcuts.mShortcuts[shortcutIndex]);
	ui->shortcutEdit->setText(key.toString(QKeySequence::NativeText));
}

void lcQPreferencesDialog::on_shortcutAssign_clicked()
{
    QTreeWidgetItem *current = ui->commandList->currentItem();

	if (!current || !current->data(0, Qt::UserRole).isValid())
		return;

	int shortcutIndex = qvariant_cast<int>(current->data(0, Qt::UserRole));
	options->KeyboardShortcuts.mShortcuts[shortcutIndex] = ui->shortcutEdit->text();

	current->setText(1, ui->shortcutEdit->text());

	setShortcutModified(current, options->KeyboardShortcuts.mShortcuts[shortcutIndex] != gCommands[shortcutIndex].DefaultShortcut);

	options->KeyboardShortcutsModified = true;
	options->KeyboardShortcutsDefault = false;
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

	options->KeyboardShortcuts = Shortcuts;

	options->KeyboardShortcutsModified = true;
	options->KeyboardShortcutsDefault = false;
}

void lcQPreferencesDialog::on_shortcutsExport_clicked()
{
	QString FileName = QFileDialog::getSaveFileName(this, tr("Export shortcuts"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	if (!options->KeyboardShortcuts.Save(FileName))
	{
		QMessageBox::warning(this, "LeoCAD", tr("Error saving keyboard shortcuts file."));
		return;
	}
}

void lcQPreferencesDialog::on_shortcutsReset_clicked()
{
	if (QMessageBox::question(this, "LeoCAD", tr("Are you sure you want to load the default keyboard shortcuts?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	options->KeyboardShortcuts.Reset();
	updateCommandList();

	options->KeyboardShortcutsModified = true;
	options->KeyboardShortcutsDefault = true;
}

void lcQPreferencesDialog::UpdateMouseTree()
{
	ui->mouseTree->clear();

	for (int ToolIdx = 0; ToolIdx < LC_NUM_TOOLS; ToolIdx++)
		UpdateMouseTreeItem(ToolIdx);
}

void lcQPreferencesDialog::UpdateMouseTreeItem(int ItemIndex)
{
	auto GetShortcutText = [this](Qt::MouseButton Button, Qt::KeyboardModifiers Modifiers)
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

	QString Shortcut1 = GetShortcutText(options->MouseShortcuts.mShortcuts[ItemIndex].Button1, options->MouseShortcuts.mShortcuts[ItemIndex].Modifiers1);
	QString Shortcut2 = GetShortcutText(options->MouseShortcuts.mShortcuts[ItemIndex].Button2, options->MouseShortcuts.mShortcuts[ItemIndex].Modifiers2);

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

			if (options->MouseShortcuts.mShortcuts[ToolIdx].Button2 == Button && options->MouseShortcuts.mShortcuts[ToolIdx].Modifiers2 == Modifiers)
			{
				if (QMessageBox::question(this, tr("Override Shortcut"), tr("This shortcut is already assigned to '%1', do you want to replace it?").arg(tr(gToolNames[ToolIdx])), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
					return;

				options->MouseShortcuts.mShortcuts[ToolIdx].Button2 = Qt::NoButton;
				options->MouseShortcuts.mShortcuts[ToolIdx].Modifiers2 = Qt::NoModifier;
			}

			if (options->MouseShortcuts.mShortcuts[ToolIdx].Button1 == Button && options->MouseShortcuts.mShortcuts[ToolIdx].Modifiers1 == Modifiers)
			{
				if (QMessageBox::question(this, tr("Override Shortcut"), tr("This shortcut is already assigned to '%1', do you want to replace it?").arg(tr(gToolNames[ToolIdx])), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
					return;

				options->MouseShortcuts.mShortcuts[ToolIdx].Button1 = options->MouseShortcuts.mShortcuts[ToolIdx].Button2;
				options->MouseShortcuts.mShortcuts[ToolIdx].Modifiers1 = options->MouseShortcuts.mShortcuts[ToolIdx].Modifiers2;
				options->MouseShortcuts.mShortcuts[ToolIdx].Button2 = Qt::NoButton;
				options->MouseShortcuts.mShortcuts[ToolIdx].Modifiers2 = Qt::NoModifier;
			}
		}
	}

	int ItemIndex = ui->mouseTree->indexOfTopLevelItem(Current);
	options->MouseShortcuts.mShortcuts[ItemIndex].Button2 = options->MouseShortcuts.mShortcuts[ItemIndex].Button1;
	options->MouseShortcuts.mShortcuts[ItemIndex].Modifiers2 = options->MouseShortcuts.mShortcuts[ItemIndex].Modifiers1;
	options->MouseShortcuts.mShortcuts[ItemIndex].Button1 = Button;
	options->MouseShortcuts.mShortcuts[ItemIndex].Modifiers1 = Modifiers;

	options->MouseShortcutsModified = true;
	options->MouseShortcutsDefault = false;

	UpdateMouseTreeItem(ItemIndex);
}

void lcQPreferencesDialog::on_mouseRemove_clicked()
{
	QTreeWidgetItem* Current = ui->mouseTree->currentItem();

	if (!Current)
		return;

	int ItemIndex = ui->mouseTree->indexOfTopLevelItem(Current);
	options->MouseShortcuts.mShortcuts[ItemIndex].Button1 = options->MouseShortcuts.mShortcuts[ItemIndex].Button2;
	options->MouseShortcuts.mShortcuts[ItemIndex].Modifiers1 = options->MouseShortcuts.mShortcuts[ItemIndex].Modifiers2;
	options->MouseShortcuts.mShortcuts[ItemIndex].Button2 = Qt::NoButton;
	options->MouseShortcuts.mShortcuts[ItemIndex].Modifiers2 = Qt::NoModifier;

	options->MouseShortcutsModified = true;
	options->MouseShortcutsDefault = false;

	UpdateMouseTreeItem(ItemIndex);
	MouseTreeItemChanged(Current);
}

void lcQPreferencesDialog::on_mouseReset_clicked()
{
	if (QMessageBox::question(this, "LeoCAD", tr("Are you sure you want to load the default mouse shortcuts?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	options->MouseShortcuts.Reset();
	UpdateMouseTree();

	options->MouseShortcutsModified = true;
	options->MouseShortcutsDefault = true;
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

	Qt::MouseButton Button = options->MouseShortcuts.mShortcuts[ToolIndex].Button1;

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

	Qt::KeyboardModifiers Modifiers = options->MouseShortcuts.mShortcuts[ToolIndex].Modifiers1;
	ui->mouseControl->setChecked((Modifiers & Qt::ControlModifier) != 0);
	ui->mouseShift->setChecked((Modifiers & Qt::ShiftModifier) != 0);
	ui->mouseAlt->setChecked((Modifiers & Qt::AltModifier) != 0);
}

#include "lc_global.h"
#include "lc_qpreferencesdialog.h"
#include "ui_lc_qpreferencesdialog.h"
#include "lc_qutils.h"
#include "lc_qcategorydialog.h"
#include "lc_basewindow.h"
#include "lc_library.h"
#include "lc_application.h"
#include "pieceinf.h"

lcQPreferencesDialog::lcQPreferencesDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQPreferencesDialog)
{
    ui->setupUi(this);

	ui->lineWidth->setValidator(new QDoubleValidator());
	connect(ui->categoriesTree, SIGNAL(itemSelectionChanged()), this, SLOT(updateParts()));
	ui->shortcutEdit->installEventFilter(this);
	connect(ui->commandList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(commandChanged(QTreeWidgetItem*)));

	options = (lcPreferencesDialogOptions*)data;

	ui->authorName->setText(options->DefaultAuthor);
	ui->projectsFolder->setText(options->ProjectsPath);
	ui->partsLibrary->setText(options->LibraryPath);
	ui->povrayExecutable->setText(options->POVRayPath);
	ui->lgeoPath->setText(options->LGEOPath);
	ui->mouseSensitivity->setValue(options->MouseSensitivity);
	ui->checkForUpdates->setChecked(options->CheckForUpdates != 0);
	ui->centimeterUnits->setChecked((options->Snap & LC_DRAW_CM_UNITS) != 0);
	ui->noRelativeSnap->setChecked((options->Snap & LC_DRAW_GLOBAL_SNAP) != 0);
	ui->fixedDirectionKeys->setChecked((options->Snap & LC_DRAW_MOVEAXIS) != 0);

	ui->antiAliasing->setChecked(options->AASamples != 1);
	if (options->AASamples == 8)
		ui->antiAliasingSamples->setCurrentIndex(2);
	else if (options->AASamples == 4)
		ui->antiAliasingSamples->setCurrentIndex(1);
	else
		ui->antiAliasingSamples->setCurrentIndex(0);
	ui->edgeLines->setChecked((options->Detail & LC_DET_BRICKEDGES) != 0);
	ui->lineWidth->setText(QString::number(options->LineWidth));
	ui->baseGrid->setChecked((options->Snap & LC_DRAW_GRID) != 0);
	ui->gridUnits->setText(QString::number(options->GridSize));
	ui->axisIcon->setChecked((options->Snap & LC_DRAW_AXIS) != 0);
	ui->enableLighting->setChecked((options->Detail & LC_DET_LIGHTING) != 0);
	ui->fastRendering->setChecked((options->Detail & LC_DET_FAST) != 0);

	on_antiAliasing_toggled();
	on_edgeLines_toggled();
	on_baseGrid_toggled();

	updateCategories();
	ui->categoriesTree->setCurrentItem(ui->categoriesTree->topLevelItem(0));

	updateCommandList();

	new lcQTreeWidgetColumnStretcher(ui->commandList, 0);
	commandChanged(NULL);
}

lcQPreferencesDialog::~lcQPreferencesDialog()
{
    delete ui;
}

void lcQPreferencesDialog::accept()
{
	options->Detail &= ~(LC_DET_BRICKEDGES | LC_DET_LIGHTING | LC_DET_FAST);
	options->Snap &= ~(LC_DRAW_CM_UNITS | LC_DRAW_GLOBAL_SNAP | LC_DRAW_MOVEAXIS | LC_DRAW_GRID | LC_DRAW_AXIS);

	strcpy(options->DefaultAuthor, ui->authorName->text().toLocal8Bit().data());
	strcpy(options->ProjectsPath, ui->projectsFolder->text().toLocal8Bit().data());
	strcpy(options->LibraryPath, ui->partsLibrary->text().toLocal8Bit().data());
	strcpy(options->POVRayPath, ui->povrayExecutable->text().toLocal8Bit().data());
	strcpy(options->LGEOPath, ui->lgeoPath->text().toLocal8Bit().data());
	options->MouseSensitivity = ui->mouseSensitivity->value();
	options->CheckForUpdates = ui->checkForUpdates->isChecked() ? 1 : 0;

	if (ui->centimeterUnits->isChecked())
		options->Snap |= LC_DRAW_CM_UNITS;

	if (ui->noRelativeSnap->isChecked())
		options->Snap |= LC_DRAW_GLOBAL_SNAP;

	if (ui->fixedDirectionKeys->isChecked())
		options->Snap |= LC_DRAW_MOVEAXIS;

	if (!ui->antiAliasing->isChecked())
		options->AASamples = 1;
	else if (ui->antiAliasingSamples->currentIndex() == 2)
		options->AASamples = 8;
	else if (ui->antiAliasingSamples->currentIndex() == 1)
		options->AASamples = 4;
	else
		options->AASamples = 2;

	if (ui->edgeLines->isChecked())
	{
		options->Detail |= LC_DET_BRICKEDGES;
		options->LineWidth = ui->lineWidth->text().toFloat();
	}

	if (ui->baseGrid->isChecked())
	{
		options->Snap |= LC_DRAW_GRID;
		options->GridSize = ui->gridUnits->text().toInt();
	}

	if (ui->axisIcon->isChecked())
		options->Snap |= LC_DRAW_AXIS;

	if (ui->enableLighting->isChecked())
		options->Detail |= LC_DET_LIGHTING;

	if (ui->fastRendering->isChecked())
		options->Detail |= LC_DET_FAST;

	QDialog::accept();
}

void lcQPreferencesDialog::on_projectsFolderBrowse_clicked()
{
	QString result = QFileDialog::getExistingDirectory(this, tr("Open Projects Folder"), ui->projectsFolder->text());

	if (!result.isEmpty())
		ui->projectsFolder->setText(QDir::toNativeSeparators(result));
}

void lcQPreferencesDialog::on_partsLibraryBrowse_clicked()
{
	QString result = QFileDialog::getExistingDirectory(this, tr("Open Parts Library Folder"), ui->partsLibrary->text());

	if (!result.isEmpty())
		ui->partsLibrary->setText(QDir::toNativeSeparators(result));
}

void lcQPreferencesDialog::on_povrayBrowse_clicked()
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

void lcQPreferencesDialog::on_lgeoBrowse_clicked()
{
	QString result = QFileDialog::getExistingDirectory(this, tr("Open LGEO Folder"), ui->lgeoPathBrowse->text());

	if (!result.isEmpty())
		ui->lgeoPathBrowse->setText(QDir::toNativeSeparators(result));
}

void lcQPreferencesDialog::on_antiAliasing_toggled()
{
	ui->antiAliasingSamples->setEnabled(ui->antiAliasing->isChecked());
}

void lcQPreferencesDialog::on_edgeLines_toggled()
{
	ui->lineWidth->setEnabled(ui->edgeLines->isChecked());
}

void lcQPreferencesDialog::on_baseGrid_toggled()
{
	ui->gridUnits->setEnabled(ui->baseGrid->isChecked());
}

void lcQPreferencesDialog::updateCategories()
{
	QTreeWidgetItem *categoryItem;
	QTreeWidget *tree = ui->categoriesTree;

	tree->clear();

	for (int categoryIndex = 0; categoryIndex < options->Categories.GetSize(); categoryIndex++)
	{
		categoryItem = new QTreeWidgetItem(tree, QStringList((const char*)options->Categories[categoryIndex].Name));
		categoryItem->setData(0, CategoryRole, QVariant(categoryIndex));
	}

	categoryItem = new QTreeWidgetItem(tree, QStringList(tr("Unassigned")));
	categoryItem->setData(0, CategoryRole, QVariant(-1));
}

void lcQPreferencesDialog::updateParts()
{
	lcPiecesLibrary *library = lcGetPiecesLibrary();
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

		library->SearchPieces(options->Categories[categoryIndex].Keywords, false, singleParts, groupedParts);

		for (int partIndex = 0; partIndex < singleParts.GetSize(); partIndex++)
		{
			PieceInfo *info = singleParts[partIndex];

			QStringList rowList(info->m_strDescription);
			rowList.append(info->m_strName);

			new QTreeWidgetItem(tree, rowList);
		}
	}
	else
	{
		for (int partIndex = 0; partIndex < library->mPieces.GetSize(); partIndex++)
		{
			PieceInfo *info = library->mPieces[partIndex];

			for (categoryIndex = 0; categoryIndex < options->Categories.GetSize(); categoryIndex++)
			{
				if (library->PieceInCategory(info, options->Categories[categoryIndex].Keywords))
					break;
			}

			if (categoryIndex == options->Categories.GetSize())
			{
				QStringList rowList(info->m_strDescription);
				rowList.append(info->m_strName);

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

	QString question = tr("Are you sure you want to delete the category '%1'?").arg((const char*)options->Categories[categoryIndex].Name);
	if (QMessageBox::question(this, "LeoCAD", question, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	options->CategoriesModified = true;
	options->CategoriesDefault = false;
	options->Categories.RemoveIndex(categoryIndex);

	updateCategories();
}

void lcQPreferencesDialog::on_importCategories_clicked()
{
	QString result = QFileDialog::getOpenFileName(this, tr("Import Categories"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (result.isEmpty())
		return;

	char fileName[LC_MAXPATH];
	strcpy(fileName, result.toLocal8Bit().data());

	lcArray<lcLibraryCategory> categories;
	if (!lcLoadCategories(fileName, categories))
	{
		QMessageBox::warning(this, "LeoCAD", tr("Error loading categories file."));
		return;
	}

	options->Categories = categories;
	options->CategoriesModified = true;
	options->CategoriesDefault = false;
}

void lcQPreferencesDialog::on_exportCategories_clicked()
{
	QString result = QFileDialog::getSaveFileName(this, tr("Export Categories"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (result.isEmpty())
		return;

	char fileName[LC_MAXPATH];
	strcpy(fileName, result.toLocal8Bit().data());

	if (!lcSaveCategories(fileName, options->Categories))
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
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		int nextKey = keyEvent->key();
		if (nextKey == Qt::Key_Control || nextKey == Qt::Key_Shift || nextKey == Qt::Key_Meta || nextKey == Qt::Key_Alt)
			return true;

		Qt::KeyboardModifiers state = keyEvent->modifiers();
		if (state & Qt::ShiftModifier)
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

	if (event->type() == QEvent::Shortcut || event->type() == QEvent::KeyRelease)
		return true;

	if (event->type() == QEvent::ShortcutOverride)
	{
		event->accept();
		return true;
	}

	return false;
}

void lcQPreferencesDialog::updateCommandList()
{
	ui->commandList->clear();
	QMap<QString, QTreeWidgetItem*> sections;

	for (int actionIdx = 0; actionIdx < LC_NUM_COMMANDS; actionIdx++)
	{
		const QString identifier = gCommands[actionIdx].ID;

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
		QKeySequence sequence(options->KeyboardShortcuts.Shortcuts[actionIdx]);
		item->setText(0, subId);
		item->setText(1, sequence.toString(QKeySequence::NativeText));
		item->setData(0, Qt::UserRole, qVariantFromValue(actionIdx));

		if (strcmp(options->KeyboardShortcuts.Shortcuts[actionIdx], gCommands[actionIdx].DefaultShortcut))
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
	QKeySequence key(options->KeyboardShortcuts.Shortcuts[shortcutIndex]);
	ui->shortcutEdit->setText(key.toString(QKeySequence::NativeText));
}

void lcQPreferencesDialog::on_shortcutAssign_clicked()
{
    QTreeWidgetItem *current = ui->commandList->currentItem();

	if (!current || !current->data(0, Qt::UserRole).isValid())
		return;

	int shortcutIndex = qvariant_cast<int>(current->data(0, Qt::UserRole));
	strcpy(options->KeyboardShortcuts.Shortcuts[shortcutIndex], ui->shortcutEdit->text().toLocal8Bit().data());

	current->setText(1, ui->shortcutEdit->text());

	setShortcutModified(current, strcmp(options->KeyboardShortcuts.Shortcuts[shortcutIndex], gCommands[shortcutIndex].DefaultShortcut) != 0);

	options->ShortcutsModified = true;
	options->ShortcutsDefault = false;
}

void lcQPreferencesDialog::on_shortcutRemove_clicked()
{
	ui->shortcutEdit->setText(QString());

	on_shortcutAssign_clicked();
}

void lcQPreferencesDialog::on_shortcutsImport_clicked()
{
	QString result = QFileDialog::getOpenFileName(this, tr("Import shortcuts"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (result.isEmpty())
		return;

	char fileName[LC_MAXPATH];
	strcpy(fileName, result.toLocal8Bit().data());

	lcKeyboardShortcuts shortcuts;
	if (!lcLoadKeyboardShortcuts(fileName, shortcuts))
	{
		QMessageBox::warning(this, "LeoCAD", tr("Error loading keyboard shortcuts file."));
		return;
	}

	options->KeyboardShortcuts = shortcuts;

	options->ShortcutsModified = true;
	options->ShortcutsDefault = false;
}

void lcQPreferencesDialog::on_shortcutsExport_clicked()
{
	QString result = QFileDialog::getSaveFileName(this, tr("Export shortcuts"), "", tr("Text Files (*.txt);;All Files (*.*)"));

	if (result.isEmpty())
		return;

	char fileName[LC_MAXPATH];
	strcpy(fileName, result.toLocal8Bit().data());

	if (!lcSaveKeyboardShortcuts(fileName, options->KeyboardShortcuts))
	{
		QMessageBox::warning(this, "LeoCAD", tr("Error saving keyboard shortcuts file."));
		return;
	}
}

void lcQPreferencesDialog::on_shortcutsReset_clicked()
{
	if (QMessageBox::question(this, "LeoCAD", tr("Are you sure you want to load the default keyboard shortcuts?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	lcResetKeyboardShortcuts(options->KeyboardShortcuts);
	updateCommandList();

	options->ShortcutsModified = true;
	options->ShortcutsDefault = true;
}

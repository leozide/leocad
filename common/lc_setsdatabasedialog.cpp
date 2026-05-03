#include "lc_global.h"
#include "lc_setsdatabasedialog.h"
#include "ui_lc_setsdatabasedialog.h"
#include "lc_http.h"
#include "project.h"

lcSetsDatabaseDialog::lcSetsDatabaseDialog(QWidget* Parent)
	: QDialog(Parent),
    ui(new Ui::lcSetsDatabaseDialog)
{
	ui->setupUi(this);
	ui->SearchEdit->installEventFilter(this);

	mHttpManager = new lcHttpManager(this);

	connect(ui->SetsTree, &QTreeWidget::itemDoubleClicked, this, &lcSetsDatabaseDialog::accept);
	connect(this, &QDialog::finished, this, &lcSetsDatabaseDialog::Finished);
	connect(mHttpManager, &lcHttpManager::DownloadFinished, this, &lcSetsDatabaseDialog::DownloadFinished);
	connect(ui->SearchButton, &QPushButton::clicked, this, &lcSetsDatabaseDialog::SearchButtonClicked);

	mKeyListReply = mHttpManager->DownloadFile(QLatin1String("https://www.leocad.org/rebrickable.json"));
}

lcSetsDatabaseDialog::~lcSetsDatabaseDialog()
{
	delete ui;
}

QString lcSetsDatabaseDialog::GetSetName() const
{
	QTreeWidgetItem* Current = ui->SetsTree->currentItem();
	return Current ? Current->text(0) : QString();
}

QString lcSetsDatabaseDialog::GetSetDescription() const
{
	QTreeWidgetItem* Current = ui->SetsTree->currentItem();
	return Current ? Current->text(1) : QString();
}

std::vector<lcSetInventoryItem> lcSetsDatabaseDialog::GetSetInventory() const
{
	std::vector<lcSetInventoryItem> Inventory;

	if (mInventory.isEmpty())
		return Inventory;

	QJsonDocument Document = QJsonDocument::fromJson(mInventory);
	QJsonObject Root = Document.object();
	const QJsonArray Parts = Root["results"].toArray();

	for (const QJsonValue& Part : Parts)
	{
		QJsonObject PartObject = Part.toObject();
		QByteArray PartID = PartObject["part"].toObject()["part_num"].toString().toLatin1();
		QJsonArray PartIDArray = PartObject["part"].toObject()["external_ids"].toObject()["LDraw"].toArray();
		
		if (!PartIDArray.isEmpty())
			PartID = PartIDArray.first().toString().toLatin1();

		int Quantity = PartObject["quantity"].toInt();
		int ColorCode = 16;
		
		QJsonArray ColorArray = PartObject["color"].toObject()["external_ids"].toObject()["LDraw"].toObject()["ext_ids"].toArray();
		
		if (!ColorArray.isEmpty())
			ColorCode = ColorArray.first().toInt();

		Inventory.push_back({ PartID, Quantity, ColorCode });
	}

	return Inventory;
}

bool lcSetsDatabaseDialog::eventFilter(QObject* Object, QEvent* Event)
{
	if (Event->type() == QEvent::KeyPress)
	{
		QKeyEvent* KeyEvent = static_cast<QKeyEvent*>(Event);

		int Key = KeyEvent->key();
		if (Key == Qt::Key_Return || Key == Qt::Key_Enter)
		{
			KeyEvent->accept();
			SearchButtonClicked();
			return true;
		}
	}

	return QDialog::eventFilter(Object, Event);
}

void lcSetsDatabaseDialog::accept()
{
	QTreeWidgetItem* Current = ui->SetsTree->currentItem();
	if (!Current)
	{
		QMessageBox::information(this, "LeoCAD", tr("Please select a set from the list."));
		return;
	}

	QString SetNum = Current->text(0);

	QProgressDialog ProgressDialog(this);
	ProgressDialog.setWindowTitle(tr("Downloading"));
	ProgressDialog.setLabelText(tr("Downloading set inventory"));
	ProgressDialog.setMaximum(0);
	ProgressDialog.setMinimum(0);
	ProgressDialog.setValue(0);
	ProgressDialog.show();

	int KeyIndex = QTime::currentTime().msec() % mKeys.size();
	QString DownloadUrl = QString("https://rebrickable.com/api/v3/lego/sets/%1/parts/?key=%2&page_size=1000").arg(SetNum, mKeys[KeyIndex]);

	mInventoryReply = mHttpManager->DownloadFile(DownloadUrl);

	while (mInventoryReply)
	{
		QApplication::processEvents();

		if (ProgressDialog.wasCanceled())
		{
			mInventoryReply->abort();
			mInventoryReply->deleteLater();
			mInventoryReply = nullptr;
			return;
		}
	}

	QDialog::accept();
}

void lcSetsDatabaseDialog::Finished(int Result)
{
	Q_UNUSED(Result);

	if (mKeyListReply)
	{
		mKeyListReply->abort();
		mKeyListReply->deleteLater();
	}
}

void lcSetsDatabaseDialog::SearchButtonClicked()
{
	QString Keyword = ui->SearchEdit->text();

	if (Keyword.isEmpty())
	{
		QMessageBox::information(this, "LeoCAD", tr("Keyword cannot be empty."));
		return;
	}

	QProgressDialog ProgressDialog(this);
	ProgressDialog.setWindowTitle(tr("Searching"));
	ProgressDialog.setLabelText(tr("Connecting to server"));
	ProgressDialog.setMaximum(0);
	ProgressDialog.setMinimum(0);
	ProgressDialog.setValue(0);
	ProgressDialog.show();

	while (mKeyListReply)
	{
		QApplication::processEvents();

		if (ProgressDialog.wasCanceled())
			return;
	}

	if (mKeys.isEmpty())
		return;

	int KeyIndex = QTime::currentTime().msec() % mKeys.size();
	QString SearchUrl = QString("https://rebrickable.com/api/v3/lego/sets/?search=%1&key=%2").arg(Keyword, mKeys[KeyIndex]);

	mSearchReply = mHttpManager->DownloadFile(SearchUrl);

	while (mSearchReply)
	{
		QApplication::processEvents();

		if (ProgressDialog.wasCanceled())
		{
			mSearchReply->abort();
			mSearchReply->deleteLater();
			mSearchReply = nullptr;
			break;
		}
	}
}

void lcSetsDatabaseDialog::DownloadFinished(lcHttpReply* Reply)
{
	if (Reply == mKeyListReply)
	{
		if (!Reply->error())
		{
			QJsonDocument Document = QJsonDocument::fromJson(Reply->readAll());
			QJsonObject Root = Document.object();

			int Version = Root["Version"].toInt();

			if (Version == 1)
			{
				const QJsonArray Keys = Root["Keys"].toArray();

				for (const QJsonValue& Key : Keys)
					mKeys.append(Key.toString());
			}
		}

		if (mKeys.isEmpty())
		{
			QMessageBox::information(this, "LeoCAD", tr("Error connecting to server."));
			close();
		}

		mKeyListReply = nullptr;
	}
	else if (Reply == mSearchReply)
	{
		QTreeWidget* SetsTree = ui->SetsTree;
		SetsTree->clear();

		if (!Reply->error())
		{
			QJsonDocument Document = QJsonDocument::fromJson(Reply->readAll());
			QJsonObject Root = Document.object();
			const QJsonArray Sets = Root["results"].toArray();

			for (const QJsonValue& Set : Sets)
			{
				QJsonObject SetObject = Set.toObject();
				QStringList SetInfo;

				SetInfo << SetObject["set_num"].toString();
				SetInfo << SetObject["name"].toString();
				SetInfo << QString::number(SetObject["year"].toInt());
				SetInfo << QString::number(SetObject["num_parts"].toInt());

				new QTreeWidgetItem(ui->SetsTree, SetInfo);
			}

			if (!Sets.isEmpty())
			{
				SetsTree->resizeColumnToContents(0);
				SetsTree->resizeColumnToContents(1);
				SetsTree->resizeColumnToContents(2);
				SetsTree->resizeColumnToContents(3);
				SetsTree->setCurrentItem(SetsTree->topLevelItem(0));
			}
		}

		mSearchReply = nullptr;
	}
	else if (Reply == mInventoryReply)
	{
		if (!Reply->error())
			mInventory = Reply->readAll();
		else
			QMessageBox::information(this, "LeoCAD", tr("Error downloading set inventory."));

		mInventoryReply = nullptr;
	}

	Reply->deleteLater();
}

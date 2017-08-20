#pragma once

#include <QDialog>
#include <QNetworkAccessManager>

namespace Ui {
class lcSetsDatabaseDialog;
}

class lcSetsDatabaseDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcSetsDatabaseDialog(QWidget* Parent);
	~lcSetsDatabaseDialog();

	QString GetSetName() const;
	QString GetSetDescription() const;

	QByteArray GetSetInventory() const
	{
		return mInventory;
	}

public slots:
	void DownloadFinished(QNetworkReply* Reply);
	void on_SearchButton_clicked();
	void accept();
	void Finished(int Result);

private:
	QNetworkAccessManager mNetworkManager;
	QNetworkReply* mKeyListReply;
	QNetworkReply* mSearchReply;
	QNetworkReply* mInventoryReply;
	QStringList mKeys;
	QByteArray mInventory;

	Ui::lcSetsDatabaseDialog* ui;
};

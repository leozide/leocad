#pragma once

#include <QDialog>

class lcHttpReply;
class lcHttpManager;

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

	bool eventFilter(QObject* Object, QEvent* Event) override;

public slots:
	void DownloadFinished(lcHttpReply* Reply);
	void on_SearchButton_clicked();
	void accept() override;
	void Finished(int Result);

protected:
	lcHttpManager* mHttpManager;

	lcHttpReply* mKeyListReply;
	lcHttpReply* mSearchReply;
	lcHttpReply* mInventoryReply;
	QStringList mKeys;
	QByteArray mInventory;

	Ui::lcSetsDatabaseDialog* ui;
};

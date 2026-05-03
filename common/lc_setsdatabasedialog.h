#pragma once

class lcHttpReply;
class lcHttpManager;
struct lcSetInventoryItem;

namespace Ui {
class lcSetsDatabaseDialog;
}

class lcSetsDatabaseDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcSetsDatabaseDialog(QWidget* Parent);
	virtual ~lcSetsDatabaseDialog();

	QString GetSetName() const;
	QString GetSetDescription() const;
	std::vector<lcSetInventoryItem> GetSetInventory() const;

	bool eventFilter(QObject* Object, QEvent* Event) override;

public slots:
	void DownloadFinished(lcHttpReply* Reply);
	void SearchButtonClicked();
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

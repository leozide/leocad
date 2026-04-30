#pragma once

class lcHttpReply;
class lcHttpManager;

namespace Ui {
class lcUpdateDialog;
}

void lcDoInitialUpdateCheck();

class lcUpdateDialog : public QDialog
{
	Q_OBJECT

public:
	lcUpdateDialog(QWidget* Parent, bool InitialUpdate);
	virtual ~lcUpdateDialog();

	void ParseUpdate(const char* Update);

public slots:
	void DownloadFinished(lcHttpReply* Reply);
	void accept() override;
	void finished(int result);

private:
	Ui::lcUpdateDialog* ui;

	lcHttpManager* mHttpManager;
	QByteArray mVersionData;
	bool mInitialUpdate;
};

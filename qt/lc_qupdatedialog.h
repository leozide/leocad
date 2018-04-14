#pragma once

#include <QDialog>

class lcHttpReply;
class lcHttpManager;

namespace Ui {
class lcQUpdateDialog;
}

void lcDoInitialUpdateCheck();

class lcQUpdateDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit lcQUpdateDialog(QWidget* Parent, bool InitialUpdate);
	~lcQUpdateDialog();

	void parseUpdate(const char *update);

public slots:
	void DownloadFinished(lcHttpReply* Reply);
	void accept();
	void reject();
	void finished(int result);

private:
	Ui::lcQUpdateDialog *ui;

	lcHttpManager* mHttpManager;
	QByteArray versionData;
	bool mInitialUpdate;
};


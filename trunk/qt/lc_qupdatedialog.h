#ifndef LC_QUPDATEDIALOG_H
#define LC_QUPDATEDIALOG_H

#include <QDialog>
#include <QNetworkReply>

namespace Ui {
class lcQUpdateDialog;
}

class lcQUpdateDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit lcQUpdateDialog(QWidget *parent, void *data);
	~lcQUpdateDialog();

	void parseUpdate(const char *update);

public slots:
	void replyFinished(QNetworkReply *reply);
	void cancel();
	
private:
	Ui::lcQUpdateDialog *ui;

	QNetworkReply *updateReply;
	QNetworkAccessManager *manager;
};

#endif // LC_QUPDATEDIALOG_H

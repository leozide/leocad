#pragma once

#include <QDialog>
struct lcHTMLExportOptions;

namespace Ui {
class lcQHTMLDialog;
}

class lcQHTMLDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcQHTMLDialog(QWidget* Parent, lcHTMLExportOptions* Options);
	~lcQHTMLDialog();

public slots:
	void accept();
	void on_outputFolderBrowse_clicked();

private:
	lcHTMLExportOptions* mOptions;
	Ui::lcQHTMLDialog *ui;
};


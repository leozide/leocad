#pragma once

#include <QDialog>
class lcHTMLExportOptions;

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
	void accept() override;

private slots:
	void OutputFolderBrowseClicked();

private:
	lcHTMLExportOptions* mOptions;
	Ui::lcQHTMLDialog *ui;
};


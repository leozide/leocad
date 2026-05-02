#pragma once

class lcHTMLExportOptions;

namespace Ui {
class lcHTMLDialog;
}

class lcHTMLDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcHTMLDialog(QWidget* Parent, lcHTMLExportOptions* Options);
	virtual ~lcHTMLDialog();

public slots:
	void accept() override;

private slots:
	void OutputFolderBrowseClicked();

private:
	lcHTMLExportOptions* mOptions;
	Ui::lcHTMLDialog* ui;
};


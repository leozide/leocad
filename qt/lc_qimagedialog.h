#pragma once

namespace Ui {
class lcImageDialog;
}

struct lcImageDialogOptions;

class lcImageDialog : public QDialog
{
	Q_OBJECT

public:
	lcImageDialog(QWidget* Parent, lcImageDialogOptions* Options);
	virtual ~lcImageDialog();
	
public slots:
	void accept() override;

private slots:
	void FileNameBrowseClicked();

private:
	lcImageDialogOptions* mOptions;
	Ui::lcImageDialog* ui;
};


#pragma once

namespace Ui
{
class lcPageSetupDialog;
}

class lcPageSetupDialog : public QDialog
{
	Q_OBJECT

public:
	lcPageSetupDialog(QWidget* Parent, lcInstructionsPageSetup* PageSetup);
	~lcPageSetupDialog();

public slots:
	void accept() override;

private:
	Ui::lcPageSetupDialog *ui;
	lcInstructionsPageSetup* mPageSetup;
};

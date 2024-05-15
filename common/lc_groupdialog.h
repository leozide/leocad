#pragma once

#include <QDialog>

namespace Ui {
class lcGroupDialog;
}

class lcGroupDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcGroupDialog(QWidget* Parent, const QString& Name);
	~lcGroupDialog();

	QString mName;

public slots:
	void accept() override;

private:
	Ui::lcGroupDialog *ui;
};

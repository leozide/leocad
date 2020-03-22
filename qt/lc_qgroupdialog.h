#pragma once

#include <QDialog>

namespace Ui {
class lcQGroupDialog;
}

class lcQGroupDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcQGroupDialog(QWidget* Parent, const QString& Name);
	~lcQGroupDialog();

	QString mName;

public slots:
	void accept() override;

private:
	Ui::lcQGroupDialog *ui;
};

#pragma once

#include <QDialog>

namespace Ui {
class lcQGroupDialog;
}

class lcQGroupDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcQGroupDialog(QWidget *parent, const QString& Name);
	~lcQGroupDialog();

	QString mName;

public slots:
	void accept();

private:
	Ui::lcQGroupDialog *ui;
};


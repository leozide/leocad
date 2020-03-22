#pragma once

#include <QDialog>
#include "lc_math.h"

namespace Ui {
class lcQArrayDialog;
}

class lcQArrayDialog : public QDialog
{
	Q_OBJECT

public:
	lcQArrayDialog(QWidget* Parent);
	~lcQArrayDialog();

	int mCounts[3];
	lcVector3 mOffsets[3];
	lcVector3 mRotations[3];

public slots:
	void accept() override;

private:
	Ui::lcQArrayDialog *ui;
};


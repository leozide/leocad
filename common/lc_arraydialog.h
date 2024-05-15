#pragma once

#include "lc_math.h"

namespace Ui {
class lcArrayDialog;
}

class lcArrayDialog : public QDialog
{
	Q_OBJECT

public:
	lcArrayDialog(QWidget* Parent);
	~lcArrayDialog();

	int mCounts[3];
	lcVector3 mOffsets[3];
	lcVector3 mRotations[3];

public slots:
	void accept() override;

private:
	Ui::lcArrayDialog *ui;
};


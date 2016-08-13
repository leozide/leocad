#ifndef _LC_QARRAYDIALOG_H_
#define _LC_QARRAYDIALOG_H_

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
	void accept();

private:
	Ui::lcQArrayDialog *ui;
};

#endif // _LC_QARRAYDIALOG_H_

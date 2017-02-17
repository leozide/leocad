#ifndef _LC_SELECTBYCOLORDIALOG_H_
#define _LC_SELECTBYCOLORDIALOG_H_

#include "lc_array.h"
class lcQColorPicker;

class lcSelectByColorDialog : public QDialog
{
public:
	Q_OBJECT

public:
	lcSelectByColorDialog(QWidget* Parent, int ColorIndex);
	~lcSelectByColorDialog();

	int mColorIndex;

public slots:
	void accept();

protected:
	lcQColorPicker* mColorPicker;
};

#endif

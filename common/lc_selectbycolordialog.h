#pragma once

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
	void accept() override;

protected:
	lcQColorPicker* mColorPicker;
};


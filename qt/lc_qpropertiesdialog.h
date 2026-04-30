#pragma once

#include "lc_model.h"

struct lcPropertiesDialogOptions
{
	lcModelProperties Properties;
	lcPartsList PartsList;
	lcBoundingBox BoundingBox;
};

namespace Ui
{
class lcPropertiesDialog;
}

class lcPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	lcPropertiesDialog(QWidget* Parent, lcPropertiesDialogOptions* Options);
	~lcPropertiesDialog();

	lcPropertiesDialogOptions* mOptions;

public slots:
	void accept() override;

private:
	Ui::lcPropertiesDialog* ui;
};


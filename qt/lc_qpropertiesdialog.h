#pragma once

#include "lc_model.h"

struct lcPropertiesDialogOptions
{
	lcModelProperties Properties;

	lcPartsList PartsList;
};

namespace Ui
{
class lcQPropertiesDialog;
}

class lcQPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	lcQPropertiesDialog(QWidget* Parent, lcPropertiesDialogOptions* Options);
	~lcQPropertiesDialog();

	lcPropertiesDialogOptions* mOptions;

public slots:
	void accept() override;

private:
	Ui::lcQPropertiesDialog* ui;
};


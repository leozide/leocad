#pragma once

#include "minifig.h"
class lcQColorPicker;

namespace Ui {
class lcQMinifigDialog;
}

class lcQMinifigDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcQMinifigDialog(QWidget* Parent);
	~lcQMinifigDialog();

	MinifigWizard* mMinifigWidget;

protected slots:
	void on_TemplateComboBox_currentIndexChanged(const QString& TemplateName);
	void on_TemplateSaveButton_clicked();
	void on_TemplateDeleteButton_clicked();
	void on_TemplateImportButton_clicked();
	void on_TemplateExportButton_clicked();
	void TypeChanged(int Index);
	void ColorChanged(int Index);
	void AngleChanged(double Value);

protected:
	void UpdateTemplateCombo();

	Ui::lcQMinifigDialog* ui;

	std::array<QComboBox*, LC_MFW_NUMITEMS> mComboBoxes;
	std::array<lcQColorPicker*, LC_MFW_NUMITEMS> mColorPickers;
	std::array<QDoubleSpinBox*, LC_MFW_NUMITEMS> mSpinBoxes;
};

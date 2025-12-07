#pragma once

#include "minifig.h"

class lcColorPicker;
class lcDoubleSpinBox;

namespace Ui
{
class lcMinifigDialog;
}

class lcMinifigDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcMinifigDialog(QWidget* Parent);
	virtual ~lcMinifigDialog();

	MinifigWizard* mMinifigWizard;

protected slots:
	void TemplateComboBoxCurrentTextChanged(const QString& TemplateName);
	void TemplateSaveButtonClicked();
	void TemplateDeleteButtonClicked();
	void TemplateImportButtonClicked();
	void TemplateExportButtonClicked();
	void PieceButtonClicked();
	void ColorChanged(int Index);
	void AngleChanged(double Value);

protected:
	void UpdateTemplateCombo();
	void SetCurrentTemplateModified();

	Ui::lcMinifigDialog* ui;

	lcView* mView;
	std::array<QToolButton*, LC_MFW_NUMITEMS> mPieceButtons;
	std::array<lcColorPicker*, LC_MFW_NUMITEMS> mColorPickers;
	std::array<lcDoubleSpinBox*, LC_MFW_NUMITEMS> mSpinBoxes;

	QString mCurrentTemplateName;
	bool mCurrentTemplateModified = false;
};

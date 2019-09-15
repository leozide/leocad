#pragma once

#include <QDialog>
struct lcMinifig;
class MinifigWizard;
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

public slots:
	void on_TemplateComboBox_currentIndexChanged(const QString& TemplateName);
	void on_TemplateSaveButton_clicked();
	void on_TemplateDeleteButton_clicked();
	void on_TemplateImportButton_clicked();
	void on_TemplateExportButton_clicked();
	void typeChanged(int index);
	void colorChanged(int index);
	void angleChanged(double value);

protected:
	Ui::lcQMinifigDialog *ui;

	void UpdateTemplateCombo();
	QComboBox *getTypeComboBox(int type);
	int getTypeIndex(QObject *widget);
	lcQColorPicker* getColorPicker(int index);
	int getColorIndex(QObject *widget);
	QDoubleSpinBox* getAngleEdit(int index);
	int getAngleIndex(QObject *widget);
};


#ifndef LC_QMINIFIGDIALOG_H
#define LC_QMINIFIGDIALOG_H

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
	explicit lcQMinifigDialog(QWidget *parent, void *data);
	~lcQMinifigDialog();

	lcMinifig *options;
	MinifigWizard *wizard;

public slots:
	void accept();
	void typeChanged(int index);
	void colorChanged(int index);
	void angleChanged(double value);

private:
	Ui::lcQMinifigDialog *ui;

	QComboBox *getTypeComboBox(int type);
	int getTypeIndex(QObject *widget);
	lcQColorPicker* getColorPicker(int index);
	int getColorIndex(QObject *widget);
	int getAngleIndex(QObject *widget);
};

#endif // LC_QMINIFIGDIALOG_H

#ifndef LC_QMINIFIGDIALOG_H
#define LC_QMINIFIGDIALOG_H

#include <QDialog>
struct MinifigWizard;

namespace Ui {
class lcQMinifigDialog;
}

class lcQMinifigDialog : public QDialog
{
    Q_OBJECT
    
public:
	explicit lcQMinifigDialog(QWidget *parent, void *data);
    ~lcQMinifigDialog();
    
	MinifigWizard *options;

public slots:
	void accept();
	void typeChanged(int index);
	void angleChanged(double value);

private:
    Ui::lcQMinifigDialog *ui;

	QComboBox *getTypeComboBox(int type);
	int getTypeIndex(QObject *widget);
	int getAngleIndex(QObject *widget);
};

#endif // LC_QMINIFIGDIALOG_H

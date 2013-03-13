#ifndef LC_QMINIFIGDIALOG_H
#define LC_QMINIFIGDIALOG_H

#include <QDialog>

namespace Ui {
class lcQMinifigDialog;
}

class lcQMinifigDialog : public QDialog
{
    Q_OBJECT
    
public:
	explicit lcQMinifigDialog(QWidget *parent, void *data);
    ~lcQMinifigDialog();
    
private:
    Ui::lcQMinifigDialog *ui;
};

#endif // LC_QMINIFIGDIALOG_H

#ifndef LC_QPREFERENCESDIALOG_H
#define LC_QPREFERENCESDIALOG_H

#include <QDialog>

namespace Ui {
class lcQPreferencesDialog;
}

class lcQPreferencesDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit lcQPreferencesDialog(QWidget *parent = 0);
    ~lcQPreferencesDialog();
    
private:
    Ui::lcQPreferencesDialog *ui;
};

#endif // LC_QPREFERENCESDIALOG_H

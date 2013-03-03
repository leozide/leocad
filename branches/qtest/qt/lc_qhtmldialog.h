#ifndef LC_QHTMLDIALOG_H
#define LC_QHTMLDIALOG_H

#include <QDialog>

namespace Ui {
class lcQHTMLDialog;
}

class lcQHTMLDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit lcQHTMLDialog(QWidget *parent = 0);
    ~lcQHTMLDialog();
    
private:
    Ui::lcQHTMLDialog *ui;
};

#endif // LC_QHTMLDIALOG_H

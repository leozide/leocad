#ifndef LC_MAINWINDOW_H
#define LC_MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class lcMainWindow;
}

class lcMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit lcMainWindow(QWidget *parent = 0);
	~lcMainWindow();

private:
	Ui::lcMainWindow *ui;
};

#endif // LC_MAINWINDOW_H

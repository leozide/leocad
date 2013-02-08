#include <QtGui/QApplication>
#include "lc_mainwindow.h"

int qt_main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	lcMainWindow w;
	w.show();

	return a.exec();
}

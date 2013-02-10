#include "lc_global.h"
#include "lc_application.h"
#include <QApplication>
#include "lc_mainwindow.h"

int main(int argc, char *argv[])
{
	g_App = new lcApplication();
//    main_window = new MainWnd();

	if (!g_App->Initialize(argc, argv, "", ""))
	  return 1;

	QApplication a(argc, argv);
	lcMainWindow w;
	w.show();

	return a.exec();
}

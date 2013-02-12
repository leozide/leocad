#include "lc_global.h"
#include "lc_application.h"
#include <QApplication>
#include "lc_mainwindow.h"

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationDomain("leocad.org");
	QCoreApplication::setOrganizationName("LeoCAD");
	QCoreApplication::setApplicationName("LeoCAD");
	QCoreApplication::setApplicationVersion(LC_VERSION_TEXT);

	g_App = new lcApplication();
//    main_window = new MainWnd();

	if (!g_App->Initialize(argc, argv, "", ""))
		return 1;

	QApplication a(argc, argv);
	lcMainWindow w;
	w.show();

	return a.exec();
}

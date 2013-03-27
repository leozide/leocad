#include "lc_global.h"
#include "lc_application.h"

void lcApplication::OpenURL(const char* URL)
{
	QDesktopServices::openUrl(QUrl(URL));
}

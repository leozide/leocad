#include "lc_global.h"
#include "lc_application.h"
#include "lc_file.h"

void lcApplication::OpenURL(const char* URL)
{
	QDesktopServices::openUrl(QUrl(URL));
}

void lcApplication::ExportClipboard(lcMemFile* Clipboard)
{
	QByteArray clipboardData = QByteArray::fromRawData((const char*)Clipboard->mBuffer, Clipboard->GetLength());
	QMimeData *mimeData = new QMimeData();

	mimeData->setData("application/vnd.leocad-clipboard", clipboardData);
	QApplication::clipboard()->setMimeData(mimeData);

	SetClipboard(Clipboard);
}

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

void lcApplication::GetFileList(const char* Path, ObjArray<String>& FileList)
{
	QDir dir(Path);
	dir.setFilter(QDir::Files | QDir::Hidden | QDir::Readable);

	FileList.RemoveAll();
	QStringList files = dir.entryList();

	for (int fileIdx = 0; fileIdx < files.size(); fileIdx++)
	{
		QString absolutePath = dir.absoluteFilePath(files[fileIdx]);

		FileList.Add(absolutePath.toLocal8Bit().data());
	}
}

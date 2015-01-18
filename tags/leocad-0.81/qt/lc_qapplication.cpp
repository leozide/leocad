#include "lc_global.h"
#include "lc_application.h"
#include "lc_file.h"

void lcApplication::OpenURL(const char* URL)
{
	QDesktopServices::openUrl(QUrl(URL));
}

void lcApplication::RunProcess(const char* ExecutablePath, const lcArray<String>& Arguments)
{
	QStringList argumentList;

	for (int argIdx = 0; argIdx < Arguments.GetSize(); argIdx++)
		argumentList << (const char*)Arguments[argIdx];

	QProcess::execute(ExecutablePath, argumentList);
}

void lcApplication::ExportClipboard(const QByteArray& Clipboard)
{
	QMimeData *mimeData = new QMimeData();

	mimeData->setData("application/vnd.leocad-clipboard", Clipboard);
	QApplication::clipboard()->setMimeData(mimeData);

	SetClipboard(Clipboard);
}

void lcApplication::GetFileList(const char* Path, lcArray<String>& FileList)
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

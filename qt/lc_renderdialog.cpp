#include "lc_global.h"
#include "lc_renderdialog.h"
#include "ui_lc_renderdialog.h"
#include "project.h"
#include "lc_application.h"
#include "lc_profile.h"

lcRenderDialog::lcRenderDialog(QWidget* Parent)
	: QDialog(Parent),
    ui(new Ui::lcRenderDialog)
{
	mProcess = nullptr;
#ifdef Q_OS_WIN
	mMapFile = NULL;
	mBuffer = nullptr;
#endif

	ui->setupUi(this);

	ui->WidthEdit->setText(QString::number(lcGetProfileInt(LC_PROFILE_POVRAY_WIDTH)));
	ui->WidthEdit->setValidator(new QIntValidator(16, INT_MAX));
	ui->HeightEdit->setText(QString::number(lcGetProfileInt(LC_PROFILE_POVRAY_HEIGHT)));
	ui->HeightEdit->setValidator(new QIntValidator(16, INT_MAX));

	connect(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(Update()));
	mUpdateTimer.start(100);
}

lcRenderDialog::~lcRenderDialog()
{
	CloseSharedMemory();

	delete ui;
}

QString lcRenderDialog::GetPOVFileName() const
{
	return QDir(QDir::tempPath()).absoluteFilePath("leocad-render.pov");
}

/*
void lcRenderDialog::reject()
{
// todo: cancel
}
*/

void lcRenderDialog::on_RenderButton_clicked()
{
	if (mProcess)
	{
// todo: cancel
return;
	}

	QString FileName = GetPOVFileName();

	if (!lcGetActiveProject()->ExportPOVRay(FileName))
		return;

	QStringList Arguments;

	Arguments.append(QString::fromLatin1("+I%1").arg(FileName));
//	Arguments.append("+OC:\\Users\\leo\\Projects\\leocad\\test.png");
	Arguments.append(QString::fromLatin1("+W%1").arg(ui->WidthEdit->text()));
	Arguments.append(QString::fromLatin1("+H%1").arg(ui->HeightEdit->text()));

	Arguments.append("+Q11");
	Arguments.append("+R3");
	Arguments.append("+A0.1");
	Arguments.append("+J0.5");

	/*
	if (!LGEOPath.isEmpty())
	{
		Arguments.append(QString::fromLatin1("+L%1lg/").arg(LGEOPath));
		Arguments.append(QString::fromLatin1("+L%1ar/").arg(LGEOPath));
	}
	Arguments.append(QString::fromLatin1("/EXIT"));
	*/

	QString POVRayPath;

#ifdef Q_OS_WIN
	POVRayPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String("/povray/povconsole32-sse2.exe"));
#endif

#ifdef Q_OS_LINUX
	POVRayPath = lcGetProfileString(LC_PROFILE_POVRAY_PATH);
	Arguments.append("+FC");
	Arguments.append("-O-");
	Arguments.append("-D");
#endif

#ifdef Q_OS_MACOS
	POVRayPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String("/povray/povconsole"));
#endif

	mProcess = new QProcess(this);
	mProcess->start(POVRayPath, Arguments);

	if (!mProcess->waitForStarted())
	{
		QMessageBox::warning(this, tr("Error"), tr("Error starting POV-Ray."));

		delete mProcess;
		mProcess = nullptr;
		QFile::remove(FileName);
	}
	else
		ui->RenderButton->setText(tr("Cancel"));
}

void lcRenderDialog::Update()
{
	if (mProcess)
	{
		if (mProcess->state() == QProcess::NotRunning)
		{
//			QString Output = mProcess->readAllStandardError();
//			QMessageBox::information(this, "LeoCAD", Output);

#ifdef Q_OS_LINUX
			QByteArray Output = mProcess->readAllStandardOutput();
			QImage Image = QImage::fromData(Output);
			ui->label->setPixmap(QPixmap::fromImage(Image.scaled(768, 432, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
#endif
			delete mProcess;
			mProcess = nullptr;
			QFile::remove(GetPOVFileName());
			ui->RenderButton->setText(tr("Render"));
		}
	}

#ifdef Q_OS_WIN
	if (mMapFile == NULL)
	{
		mMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "leocad-povray");

		if (!mMapFile)
			return;

		mBuffer = MapViewOfFile(mMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		if (!mBuffer)
		{
			CloseHandle(mMapFile);
			mMapFile = NULL;
			return;
		}

		uint32_t* Header = (uint32_t*)mBuffer;
		if (Header[0] != 1)
		{
			CloseSharedMemory();
			return;
		}
	}

	if (!mBuffer)
		return;

	struct lcSharedMemoryHeader
	{
		uint32_t Version;
		uint32_t Width;
		uint32_t Height;
		uint32_t PixelsWritten;
		uint32_t PixelsRead;
	};

	lcSharedMemoryHeader* Header = (lcSharedMemoryHeader*)mBuffer;

	int Width = Header->Width;
	int Height = Header->Height;
	int PixelsRead = Header->PixelsWritten;

//		if (width != expected) ...

	QImage Image(Width, Height, QImage::Format_ARGB32);
	uint8_t* Pixels = (uint8_t*)(Header + 1);

	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			Image.setPixelColor(x, y, QColor::fromRgb(Pixels[0], Pixels[1], Pixels[2], Pixels[3]));
			Pixels += 4;
		}
	}

	Header->PixelsRead = PixelsRead;

	ui->label->setPixmap(QPixmap::fromImage(Image));
#endif
}

void lcRenderDialog::CloseSharedMemory()
{
#ifdef Q_OS_WIN
	if (mBuffer)
	{
		UnmapViewOfFile(mBuffer);
		mBuffer = NULL;
	}

	if (mMapFile)
	{
		CloseHandle(mMapFile);
		mMapFile = NULL;
	}
#endif
}

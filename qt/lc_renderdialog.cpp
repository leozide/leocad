#include "lc_global.h"
#include "lc_renderdialog.h"
#include "ui_lc_renderdialog.h"
#include "project.h"
#include "lc_application.h"
#include "lc_profile.h"

#define LC_POVRAY_PREVIEW_WIDTH 768
#define LC_POVRAY_PREVIEW_HEIGHT 432

lcRenderDialog::lcRenderDialog(QWidget* Parent)
	: QDialog(Parent),
    ui(new Ui::lcRenderDialog)
{
#ifndef QT_NO_PROCESS
	mProcess = nullptr;
#endif
	mSharedMemory.setNativeKey("leocad-povray");

	ui->setupUi(this);

	ui->WidthEdit->setText(QString::number(lcGetProfileInt(LC_PROFILE_POVRAY_WIDTH)));
	ui->WidthEdit->setValidator(new QIntValidator(16, INT_MAX));
	ui->HeightEdit->setText(QString::number(lcGetProfileInt(LC_PROFILE_POVRAY_HEIGHT)));
	ui->HeightEdit->setValidator(new QIntValidator(16, INT_MAX));

	QImage Image(LC_POVRAY_PREVIEW_WIDTH, LC_POVRAY_PREVIEW_HEIGHT, QImage::Format_RGB32);
	Image.fill(QColor(255, 255, 255));
	ui->label->setPixmap(QPixmap::fromImage(Image));

	connect(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(Update()));
	mUpdateTimer.start(100);
}

lcRenderDialog::~lcRenderDialog()
{
	mSharedMemory.detach();

	delete ui;
}

QString lcRenderDialog::GetPOVFileName() const
{
	return QDir(QDir::tempPath()).absoluteFilePath("leocad-render.pov");
}

void lcRenderDialog::CloseProcess()
{
#ifndef QT_NO_PROCESS
	delete mProcess;
	mProcess = nullptr;
#endif
	
	QFile::remove(GetPOVFileName());

	ui->RenderButton->setText(tr("Render"));
}

bool lcRenderDialog::PromptCancel()
{
#ifndef QT_NO_PROCESS
	if (mProcess)
	{
		if (QMessageBox::question(this, tr("Cancel Render"), tr("Are you sure you want to cancel the current render?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
		{
			if (mProcess)
			{
				mProcess->kill();
				CloseProcess();
			}
		}
		else
			return false;
	}
#endif
	
	return true;
}

void lcRenderDialog::reject()
{
	if (PromptCancel())
		QDialog::reject();
}

void lcRenderDialog::on_RenderButton_clicked()
{
#ifndef QT_NO_PROCESS
	if (!PromptCancel())
		return;

	QString FileName = GetPOVFileName();

	if (!lcGetActiveProject()->ExportPOVRay(FileName))
		return;

	QStringList Arguments;

	Arguments.append(QString::fromLatin1("+I%1").arg(FileName));
	Arguments.append(QString::fromLatin1("+W%1").arg(ui->WidthEdit->text()));
	Arguments.append(QString::fromLatin1("+H%1").arg(ui->HeightEdit->text()));
	Arguments.append("-O-");

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
	*/

	QString POVRayPath;

#ifdef Q_OS_WIN
	POVRayPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String("/povray/povconsole32-sse2.exe"));
#endif

#ifdef Q_OS_LINUX
	POVRayPath = lcGetProfileString(LC_PROFILE_POVRAY_PATH);
	Arguments.append("+FC");
	Arguments.append("-D");
#endif

#ifdef Q_OS_MACOS
	POVRayPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String("/povray/povconsole"));
#endif

	mProcess = new QProcess(this);
	mProcess->start(POVRayPath, Arguments);

	if (mProcess->waitForStarted())
		ui->RenderButton->setText(tr("Cancel"));
	else
	{
		QMessageBox::warning(this, tr("Error"), tr("Error starting POV-Ray."));
		CloseProcess();
	}
#endif
}

void lcRenderDialog::Update()
{
#ifndef QT_NO_PROCESS
	if (mProcess)
	{
		if (mProcess->state() == QProcess::NotRunning)
		{
//			QString Output = mProcess->readAllStandardError();
//			QMessageBox::information(this, "LeoCAD", Output);

#ifdef Q_OS_LINUX
			QByteArray Output = mProcess->readAllStandardOutput();
			QImage Image = QImage::fromData(Output);
			ui->label->setPixmap(QPixmap::fromImage(Image.scaled(LC_POVRAY_PREVIEW_WIDTH, LC_POVRAY_PREVIEW_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
#endif
			CloseProcess();
		}
	}
#endif

#ifdef Q_OS_WIN
	if (!mSharedMemory.isAttached() && !mSharedMemory.attach())
		return;

	void* Buffer = mSharedMemory.data();

	if (!Buffer)
	{
		mSharedMemory.detach();
		return;
	}

	struct lcSharedMemoryHeader
	{
		quint32 Version;
		quint32 Width;
		quint32 Height;
		quint32 PixelsWritten;
		quint32 PixelsRead;
	};

	lcSharedMemoryHeader* Header = (lcSharedMemoryHeader*)Buffer;

	int Width = Header->Width;
	int Height = Header->Height;
	int PixelsRead = Header->PixelsWritten;

//		if (width != expected) ...

	QImage Image(Width, Height, QImage::Format_ARGB32);
	quint8* Pixels = (quint8*)(Header + 1);

	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			Image.setPixelColor(x, y, QColor::fromRgb(Pixels[0], Pixels[1], Pixels[2], Pixels[3]));
			Pixels += 4;
		}
	}

	Header->PixelsRead = PixelsRead;

	ui->label->setPixmap(QPixmap::fromImage(Image.scaled(LC_POVRAY_PREVIEW_WIDTH, LC_POVRAY_PREVIEW_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
#endif
}

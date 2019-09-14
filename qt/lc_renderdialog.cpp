#include "lc_global.h"
#include "lc_renderdialog.h"
#include "ui_lc_renderdialog.h"
#include "project.h"
#include "lc_application.h"
#include "lc_profile.h"

#define LC_POVRAY_PREVIEW_WIDTH 768
#define LC_POVRAY_PREVIEW_HEIGHT 432
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
#define LC_POVRAY_MEMORY_MAPPED_FILE 1
#endif

void lcRenderPreviewWidget::resizeEvent(QResizeEvent* Event)
{
	mScaledImage = QImage();

	QWidget::resizeEvent(Event);
}

void lcRenderPreviewWidget::paintEvent(QPaintEvent* PaintEvent)
{
	Q_UNUSED(PaintEvent);

	QPainter Painter(this);

	if (!mImage.isNull())
	{
		QSize Size = size();

		if (mScaledImage.isNull())
			mScaledImage = mImage.scaled(Size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

		Painter.drawImage((Size.width() - mScaledImage.width()) / 2, (Size.height() - mScaledImage.height()) / 2, mScaledImage);
	}
	else
		Painter.fillRect(rect(), Qt::white);
}

lcRenderDialog::lcRenderDialog(QWidget* Parent)
	: QDialog(Parent),
	ui(new Ui::lcRenderDialog)
{
#ifndef QT_NO_PROCESS
	mProcess = nullptr;
#endif
	mOutputBuffer = nullptr;

	ui->setupUi(this);

	ui->WidthEdit->setText(QString::number(lcGetProfileInt(LC_PROFILE_POVRAY_WIDTH)));
	ui->WidthEdit->setValidator(new QIntValidator(16, INT_MAX));
	ui->HeightEdit->setText(QString::number(lcGetProfileInt(LC_PROFILE_POVRAY_HEIGHT)));
	ui->HeightEdit->setValidator(new QIntValidator(16, INT_MAX));
	ui->OutputEdit->setText(lcGetActiveProject()->GetImageFileName(false));

	QImage Image(LC_POVRAY_PREVIEW_WIDTH, LC_POVRAY_PREVIEW_HEIGHT, QImage::Format_RGB32);
	Image.fill(QColor(255, 255, 255));
	ui->preview->SetImage(Image);

	connect(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(Update()));
	mUpdateTimer.start(500);
}

lcRenderDialog::~lcRenderDialog()
{
	delete ui;
}

QString lcRenderDialog::GetOutputFileName() const
{
	return QDir(QDir::tempPath()).absoluteFilePath("leocad-render.out");
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

#if LC_POVRAY_MEMORY_MAPPED_FILE
	mOutputFile.unmap((uchar*)mOutputBuffer);
	mOutputBuffer = nullptr;
	mOutputFile.close();

	QFile::remove(GetOutputFileName());
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
	if (mProcess)
	{
		PromptCancel();
		return;
	}

	QString FileName = GetPOVFileName();

	if (!lcGetActiveProject()->ExportPOVRay(FileName))
		return;

	QStringList Arguments;

	Arguments.append(QString::fromLatin1("+I\"%1\"").arg(FileName));
	Arguments.append(QString::fromLatin1("+W%1").arg(ui->WidthEdit->text()));
	Arguments.append(QString::fromLatin1("+H%1").arg(ui->HeightEdit->text()));
	Arguments.append("-O-");

#if LC_POVRAY_MEMORY_MAPPED_FILE
	Arguments.append(QString::fromLatin1("+SM\"%1\"").arg(GetOutputFileName()));
#endif

	int Quality = ui->QualityComboBox->currentIndex();

	switch (Quality)
	{
	case 0:
		Arguments.append("+Q11");
		Arguments.append("+R3");
		Arguments.append("+A0.1");
		Arguments.append("+J0.5");
		break;

	case 1:
		Arguments.append("+Q5");
		Arguments.append("+A0.1");
		break;

	case 2:
		break;
	}

	/*
	if (!LGEOPath.isEmpty())
	{
		Arguments.append(QString::fromLatin1("+L%1lg/").arg(LGEOPath));
		Arguments.append(QString::fromLatin1("+L%1ar/").arg(LGEOPath));
	}
	*/

	QString POVRayPath;

#ifdef Q_OS_WIN
	POVRayPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String("/povconsole32-sse2.exe"));
#endif

#ifdef Q_OS_LINUX
	POVRayPath = lcGetProfileString(LC_PROFILE_POVRAY_PATH);
	Arguments.append("+FN");
	Arguments.append("-D");
#endif

#ifdef Q_OS_MACOS
	POVRayPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String("/povray"));
#endif

	mProcess = new QProcess(this);
#ifdef Q_OS_LINUX
	connect(mProcess, SIGNAL(readyReadStandardError()), this, SLOT(ReadStdErr()));
#endif
	mProcess->start(POVRayPath, Arguments);

	mImage = QImage(ui->WidthEdit->text().toInt(), ui->HeightEdit->text().toInt(), QImage::Format_ARGB32);
	mImage.fill(QColor(255, 255, 255));
	ui->preview->SetImage(mImage);

	if (mProcess->waitForStarted())
	{
		ui->RenderButton->setText(tr("Cancel"));
		ui->RenderProgress->setValue(ui->RenderProgress->minimum());
		mStdErrList.clear();
	}
	else
	{
		QMessageBox::warning(this, tr("Error"), tr("Error starting POV-Ray."));
		CloseProcess();
	}
#endif
}

void lcRenderDialog::ReadStdErr()
{
	QString StdErr = QString(mProcess->readAllStandardError());
	mStdErrList.append(StdErr);
#ifdef Q_OS_LINUX
	QRegExp RegexPovRayProgress("Rendered (\\d+) of (\\d+) pixels.*");
	RegexPovRayProgress.setCaseSensitivity(Qt::CaseInsensitive);
	if (RegexPovRayProgress.indexIn(StdErr) == 0)
	{
		ui->RenderProgress->setMaximum(RegexPovRayProgress.cap(2).toInt());
		ui->RenderProgress->setValue(RegexPovRayProgress.cap(1).toInt());
	}
#endif
}

void lcRenderDialog::Update()
{
#ifndef QT_NO_PROCESS
	if (!mProcess)
		return;

	if (mProcess->state() == QProcess::NotRunning)
	{
#ifdef Q_OS_LINUX
		QByteArray Output = mProcess->readAllStandardOutput();
		mImage = QImage::fromData(Output);
#endif

		ShowResult();
		CloseProcess();
	}
#endif

#if LC_POVRAY_MEMORY_MAPPED_FILE
	if (!mOutputBuffer)
	{
		mOutputFile.setFileName(GetOutputFileName());

		if (!mOutputFile.open(QFile::ReadWrite))
			return;

		mOutputBuffer = mOutputFile.map(0, mOutputFile.size());

		if (!mOutputBuffer)
		{
			mOutputFile.close();
			return;
		}
	}

	struct lcSharedMemoryHeader
	{
		quint32 Version;
		quint32 Width;
		quint32 Height;
		quint32 PixelsWritten;
		quint32 PixelsRead;
	};

	lcSharedMemoryHeader* Header = (lcSharedMemoryHeader*)mOutputBuffer;

	if (Header->PixelsWritten == Header->PixelsRead)
		return;

	int Width = Header->Width;
	int Height = Header->Height;
	int PixelsWritten = Header->PixelsWritten;

	quint8* Pixels = (quint8*)(Header + 1);

	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			mImage.setPixel(x, y, qRgba(Pixels[0], Pixels[1], Pixels[2], Pixels[3]));
			Pixels += 4;
		}
	}

	Header->PixelsRead = PixelsWritten;

	ui->RenderProgress->setMaximum(mImage.width() * mImage.height());
	ui->RenderProgress->setValue(int(Header->PixelsRead));

	ui->preview->SetImage(mImage);
#endif
}

void lcRenderDialog::ShowResult()
{
	ReadStdErr();
	ui->RenderProgress->setValue(ui->RenderProgress->maximum());

	bool Error = (mProcess->exitStatus() != QProcess::NormalExit || mProcess->exitCode() != 0);
	if (Error)
	{
		WriteStdLog(Error);
		QMessageBox MessageBox;
		MessageBox.setWindowTitle(tr("Error"));
		MessageBox.setIcon(QMessageBox::Information);
		MessageBox.setText(tr("An error occurred while rendering. Check details or try again."));
		MessageBox.setDetailedText(mStdErrList.join(QString()));
		MessageBox.exec();
		return;
	}

	ui->preview->SetImage(mImage);

	QString FileName = ui->OutputEdit->text();

	if (!FileName.isEmpty())
	{
		QImageWriter Writer(FileName);

		bool Result = Writer.write(mImage);

		if (!Result)
			QMessageBox::information(this, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));
	}

	WriteStdLog();
}

void lcRenderDialog::WriteStdLog(bool Error)
{
	QString FileName = lcGetActiveProject()->GetFileName();
	if (FileName.isEmpty())
		return;

	QFile LogFile(QDir::toNativeSeparators(QFileInfo(FileName).absolutePath() + "/" +
				  (Error ? "stderr-povrayrender" : "stdout-povrayrender")));

	if (LogFile.open(QFile::WriteOnly | QIODevice::Truncate | QFile::Text))
	{
		QTextStream Out(&LogFile);
		for (const QString& Line : mStdErrList)
			Out << Line;
		LogFile.close();
	}
	else
	{
		QMessageBox::information(this, tr("Error"), tr("Error writing to %1 file '%2':\n%3")
			.arg(Error ? "stderr" : "stdout").arg(LogFile.fileName(), LogFile.errorString()));
	}
}

void lcRenderDialog::on_OutputBrowseButton_clicked()
{
	QString Result = QFileDialog::getSaveFileName(this, tr("Select Output File"), ui->OutputEdit->text(), tr("Supported Image Files (*.bmp *.png *.jpg);;BMP Files (*.bmp);;PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*.*)"));

	if (!Result.isEmpty())
		ui->OutputEdit->setText(QDir::toNativeSeparators(Result));

}

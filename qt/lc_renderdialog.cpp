#include "lc_global.h"
#include "lc_renderdialog.h"
#include "ui_lc_renderdialog.h"
#include "project.h"
#include "lc_application.h"
#include "lc_profile.h"
#include "lc_blenderpreferences.h"
#include "lc_mainwindow.h"
#include "lc_model.h"

#ifndef LC_DISABLE_RENDER_DIALOG

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
		Painter.fillRect(rect(), Qt::transparent);
}

lcRenderDialog::lcRenderDialog(QWidget* Parent, lcRenderDialogMode RenderDialogMode)
	: QDialog(Parent), mDialogMode(RenderDialogMode), ui(new Ui::lcRenderDialog)
{
	ui->setupUi(this);

	mWidth = lcGetProfileInt(LC_PROFILE_RENDER_WIDTH);
	mHeight = lcGetProfileInt(LC_PROFILE_RENDER_HEIGHT);
	mQuality = static_cast<lcPOVRayRenderQuality>(lcGetProfileInt(LC_PROFILE_RENDER_QUALITY));
	mScale = 1.0f;

	ui->OutputEdit->setText(lcGetActiveProject()->GetImageFileName(false));

	mRenderButton = new QPushButton(tr("Render"), ui->buttonBox);
    ui->buttonBox->addButton(mRenderButton, QDialogButtonBox::ActionRole);

	mSettingsButton = new QPushButton(tr("Settings..."), ui->buttonBox);
	ui->buttonBox->addButton(mSettingsButton, QDialogButtonBox::ResetRole);

	mLogButton = new QPushButton(tr("View Log..."), ui->buttonBox);
    ui->buttonBox->addButton(mLogButton, QDialogButtonBox::ResetRole);
	mLogButton->setEnabled(false);

	connect(mRenderButton, &QPushButton::clicked, this, &lcRenderDialog::RenderButtonClicked);
	connect(mSettingsButton, &QPushButton::clicked, this, &lcRenderDialog::SettingsButtonClicked);
	connect(mLogButton, &QPushButton::clicked, this, &lcRenderDialog::LogButtonClicked);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &lcRenderDialog::reject);

	if (mDialogMode == lcRenderDialogMode::RenderPOVRay)
	{
		setWindowTitle(tr("POV-Ray Image Render"));

		mRenderButton->setToolTip(tr("Render Model"));
	}
	else
	{
		setWindowTitle(mDialogMode == lcRenderDialogMode::OpenInBlender ? tr("Blender LDraw Import") : tr("Blender Image Render"));

		bool BlenderConfigured = !lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty();

		QStringList const& DataPathList = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
		if (!QDir(QString("%1/Blender/addons/%2").arg(DataPathList.first()).arg(LC_BLENDER_ADDON_RENDER_FOLDER)).isReadable())
		{
			BlenderConfigured = false;
			lcSetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE, QString());
		}

		mRenderButton->setToolTip(BlenderConfigured ? tr("Render Model") : tr("Blender not configured. Use Settings to configure."));
		mRenderButton->setEnabled(BlenderConfigured);

		mSettingsButton->setToolTip(tr("Blender render settings"));

		if (mDialogMode == lcRenderDialogMode::OpenInBlender)
		{
			mSettingsButton->setToolTip(tr("Blender import settings"));

			mRenderButton->setText(tr("Open in Blender"));

			if (BlenderConfigured)
				mRenderButton->setToolTip(tr("Import and open LDraw model in Blender"));

			ui->outputLabel->hide();
			ui->OutputEdit->hide();
			ui->RenderProgress->hide();
			ui->OutputBrowseButton->hide();
			mLogButton->hide();

			adjustSize();
			setMinimumWidth(ui->preview->geometry().width());
			ui->preview->hide();
		}
		else
		{
			QImage Image(QPixmap(":/resources/file_render_blender_logo_1280x720.png").toImage());
			Image = Image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
			ui->preview->SetImage(Image);
		}
	}

	connect(&mUpdateTimer, &QTimer::timeout, this, &lcRenderDialog::Update);

	mUpdateTimer.start(500);
}

lcRenderDialog::~lcRenderDialog()
{
	delete ui;
}

QString lcRenderDialog::GetStdOutFileName() const
{
	QString LogFile = mDialogMode == lcRenderDialogMode::RenderPOVRay ? QLatin1String("leocad-povray-render.out") : QLatin1String("leocad-blender-render.out");

	return QDir(QDir::tempPath()).absoluteFilePath(LogFile);
}

QString lcRenderDialog::GetStdErrFileName() const
{
	QString LogFile = mDialogMode == lcRenderDialogMode::RenderPOVRay ? QLatin1String("leocad-povray-render.err") : QLatin1String("leocad-blender-render.err");

	return QDir(QDir::tempPath()).absoluteFilePath(LogFile);
}

QString lcRenderDialog::GetPOVFileName() const
{
	return QDir(QDir::tempPath()).absoluteFilePath("leocad-render.pov");
}

void lcRenderDialog::CloseProcess()
{
	delete mProcess;
	mProcess = nullptr;

	if (mDialogMode == lcRenderDialogMode::RenderPOVRay)
	{
#if LC_POVRAY_MEMORY_MAPPED_FILE
		mOutputFile.unmap((uchar*)mOutputBuffer);
		mOutputBuffer = nullptr;
		mOutputFile.close();

		QFile::remove(GetStdOutFileName());
#endif

		QFile::remove(GetPOVFileName());
	}

	if (mDialogMode != lcRenderDialogMode::OpenInBlender)
		mRenderButton->setText(tr("Render"));
}

bool lcRenderDialog::PromptCancel()
{
	if (!mProcess)
		return true;
	
	if (QMessageBox::question(this, tr("Cancel Render"), tr("Are you sure you want to cancel the current render?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return false;

	if (mDialogMode == lcRenderDialogMode::RenderPOVRay)
		gMainWindow->mActions[LC_FILE_RENDER_POVRAY]->setEnabled(true);
	else if (mDialogMode == lcRenderDialogMode::RenderBlender)
		gMainWindow->mActions[LC_FILE_RENDER_BLENDER]->setEnabled(true);

	if (mProcess)
	{
#ifdef Q_OS_WIN
		lcTerminateChildProcess(this, mProcess->processId(), QCoreApplication::applicationPid());
#endif
		mProcess->kill();
	}

	CloseProcess();

	if (mStdOutList.size())
	{
		WriteStdOut();
		mLogButton->setEnabled(true);
	}

	return true;
}

void lcRenderDialog::RenderButtonClicked()
{
	if (mProcess)
	{
		PromptCancel();
		return;
	}

	mLogButton->setEnabled(false);

	mPreviewWidth  = ui->preview->width();
	mPreviewHeight = ui->preview->height();

	mRenderTime.start();

	if (mDialogMode == lcRenderDialogMode::RenderPOVRay)
		RenderPOVRay();
	else
		RenderBlender();
}

void lcRenderDialog::reject()
{
	if (PromptCancel())
		QDialog::reject();
}

void lcRenderDialog::RenderPOVRay()
{
	gMainWindow->mActions[LC_FILE_RENDER_POVRAY]->setEnabled(false);

	QString FileName = GetPOVFileName();

	ui->preview->SetImage(QImage());
	ui->RenderProgress->setValue(ui->RenderProgress->minimum());
	ui->RenderProgress->setFormat(tr("Exporting Model"));

	QFuture<std::pair<bool, QString>> exportThread = QtConcurrent::run([FileName]()
	{
		return lcGetActiveProject()->ExportPOVRay(FileName);
	});

	QApplication::setOverrideCursor(Qt::WaitCursor);

	while (!exportThread.isFinished())
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	QGuiApplication::restoreOverrideCursor();

	ui->RenderProgress->setFormat("%p%");

	auto [Success, ErrorMessage] = exportThread.result();

	if (!Success)
	{
		if (!ErrorMessage.isEmpty())
			QMessageBox::information(this, tr("Render Error"), ErrorMessage);

		return;
	}

	QStringList Arguments;

	Arguments.append(QString::fromLatin1("+I\"%1\"").arg(FileName));
	Arguments.append(QString::fromLatin1("+W%1").arg(QString::number(mWidth)));
	Arguments.append(QString::fromLatin1("+H%1").arg(QString::number(mHeight)));
	Arguments.append("-O-");

#if LC_POVRAY_MEMORY_MAPPED_FILE
	Arguments.append(QString::fromLatin1("+SM\"%1\"").arg(GetStdOutFileName()));
#endif

	switch (mQuality)
	{
	case lcPOVRayRenderQuality::High:
		Arguments.append("+Q11");
		Arguments.append("+R3");
		Arguments.append("+A0.1");
		Arguments.append("+J0.5");
		break;

	case lcPOVRayRenderQuality::Medium:
		Arguments.append("+Q5");
		Arguments.append("+A0.1");
		break;

	case lcPOVRayRenderQuality::Low:
		break;
	}

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

	const QString POVRayDir = QFileInfo(POVRayPath).absolutePath();
	const QString IncludePath = QDir::cleanPath(POVRayDir + "/include");

	if (QFileInfo(IncludePath).exists())
		Arguments.append(QString("+L\"%1\"").arg(IncludePath));

	const QString IniPath = QDir::cleanPath(POVRayDir + "/ini");

	if (QFileInfo(IniPath).exists())
		Arguments.append(QString("+L\"%1\"").arg(IniPath));

	if (lcGetActiveProject()->GetModels()[0]->GetPOVRayOptions().UseLGEO)
	{
		const QString LGEOPath = lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH);

		if (QFileInfo(LGEOPath).exists())
		{
			const QString LgPath = QDir::cleanPath(LGEOPath + "/lg");
			if (QFileInfo(LgPath).exists())
				Arguments.append(QString("+L\"%1\"").arg(LgPath));
			const QString ArPath = QDir::cleanPath(LGEOPath + "/ar");
			if (QFileInfo(ArPath).exists())
				Arguments.append(QString("+L\"%1\"").arg(ArPath));
			const QString StlPath = QDir::cleanPath(LGEOPath + "/stl");
			if (QFileInfo(StlPath).exists())
				Arguments.append(QString("+L\"%1\"").arg(StlPath));
		}
	}

	lcRenderProcess* Process = new lcRenderProcess(this);
#ifdef Q_OS_LINUX
	connect(Process, SIGNAL(readyReadStandardError()), this, SLOT(ReadStdErr()));
#endif
	QStringList POVEnv = QProcess::systemEnvironment();
	POVEnv.prepend("POV_IGNORE_SYSCONF_MSG=1");
	Process->setEnvironment(POVEnv);
	Process->setStandardErrorFile(GetStdErrFileName());
	Process->start(POVRayPath, Arguments);

	mImage = QImage(mWidth, mHeight, QImage::Format_ARGB32);
	mImage.fill(Qt::transparent);
	ui->preview->SetImage(mImage);

	if (Process->waitForStarted())
	{
		mProcess = Process;
		mRenderButton->setText(tr("Cancel"));
		ui->RenderProgress->setValue(ui->RenderProgress->minimum());
		mStdErrList.clear();
	}
	else
	{
		delete Process;
		gMainWindow->mActions[LC_FILE_RENDER_POVRAY]->setEnabled(true);
		QMessageBox::information(this, tr("Render Error"), tr("Error starting POV-Ray."));
		CloseProcess();
	}
}

void lcRenderDialog::RenderBlender()
{
	const QString BlenderLDrawConfigFile = lcGetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH);
	const QString BlenderImportModule = lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE);

	if (!QFileInfo(BlenderLDrawConfigFile).isReadable() && !BlenderImportModule.isEmpty())
		lcBlenderPreferences::SaveSettings();

	ui->RenderProgress->setFormat(tr("Saving Blender Model"));
	QApplication::processEvents();

	mBlendProgValue = 0;
	mBlendProgMax = 0;

	const QStringList DataPathList = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
	mDataPath = DataPathList.first();
	const QString DefaultBlendFile = QString("%1/blender/config/%2").arg(mDataPath).arg(LC_BLENDER_ADDON_BLEND_FILE);

	lcModel* Model = lcGetActiveProject()->GetActiveModel();
	const QString ModelFileName = QFileInfo(QDir(lcGetProfileString(LC_PROFILE_PROJECTS_PATH)), QString("%1_Step_%2.ldr").arg(QFileInfo(Model->GetProperties().mFileName).baseName()).arg(Model->GetCurrentStep())).absoluteFilePath();

	lcGetActiveProject()->ExportCurrentStep(ModelFileName);

	ui->RenderProgress->setFormat("%p%");
	QApplication::processEvents();

	if (!QFileInfo(ModelFileName).isReadable())
		return;

	bool SearchCustomDir = true;

	QString Message;
	QStringList Arguments;
	QString PythonExpression = QString("\"import bpy; bpy.ops.render_scene.lpub3d_render_ldraw("
										"'EXEC_DEFAULT', "
										"resolution_width=%1, resolution_height=%2, "
										"render_percentage=%3, model_file=r'%4', "
										"image_file=r'%5'")
										.arg(mWidth).arg(mHeight)
										.arg(mScale * 100)
										.arg(QDir::toNativeSeparators(ModelFileName).replace("\\","\\\\"))
										.arg(QDir::toNativeSeparators(ui->OutputEdit->text()).replace("\\","\\\\"));
	if (BlenderImportModule == QLatin1String("MM"))
		PythonExpression.append(", use_ldraw_import_mm=True");
	if (SearchCustomDir)
		PythonExpression.append(", search_additional_paths=True");

	if (mDialogMode == lcRenderDialogMode::OpenInBlender)
	{
		PythonExpression.append(", import_only=True");

		Arguments << QLatin1String("--window-geometry");
		Arguments << QLatin1String("200 100 1440 900");
	}
	else
	{
		Arguments << QLatin1String("--background");
		gMainWindow->mActions[LC_FILE_RENDER_BLENDER]->setEnabled(false);
	}

	PythonExpression.append(", cli_render=True)\"");

	if (QFileInfo(DefaultBlendFile).exists())
		Arguments << QDir::toNativeSeparators(DefaultBlendFile);
	Arguments << QString("--python-expr");
	Arguments << PythonExpression;

	QString ScriptName, ScriptCommand, ShellProgram;

#ifdef Q_OS_WIN
	ScriptName =  QLatin1String("render_ldraw_model.bat");
#else
	ScriptName =  QLatin1String("render_ldraw_model.sh");
#endif
	ScriptCommand = QString("\"%1\" %2").arg(lcGetProfileString(LC_PROFILE_BLENDER_PATH)).arg(Arguments.join(" "));

	if (mDialogMode == lcRenderDialogMode::OpenInBlender)
		ScriptCommand.append(QString(" > %1").arg(QDir::toNativeSeparators(GetStdOutFileName())));

	const QLatin1String LineEnding("\r\n");

	QFile ScriptFile(QString("%1/%2").arg(QDir::tempPath()).arg(ScriptName));
	if (ScriptFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream Stream(&ScriptFile);
#ifdef Q_OS_WIN
		Stream << QLatin1String("@ECHO OFF &SETLOCAL") << LineEnding;
#else
		Stream << QLatin1String("#!/bin/bash") << LineEnding;
#endif
		Stream << ScriptCommand << LineEnding;
		ScriptFile.close();
	}
	else
	{
		QMessageBox::information(this, tr("Render Error"), tr("Cannot write Blender render script file '%1':\n%2.").arg(ScriptFile.fileName(), ScriptFile.errorString()));

		gMainWindow->mActions[LC_FILE_RENDER_BLENDER]->setEnabled(true);
		return;
	}

	QThread::sleep(2);

#ifdef Q_OS_WIN
	ShellProgram = QLatin1String(LC_WINDOWS_SHELL);
#else
	ShellProgram = QLatin1String(LC_UNIX_SHELL);
#endif

	mProcess = new lcRenderProcess(this);

	connect(mProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(ReadStdOut()));

	const QString LDrawLibPath = QFileInfo(lcGetProfileString(LC_PROFILE_PARTS_LIBRARY)).absolutePath();
	QStringList SystemEnvironment = QProcess::systemEnvironment();
	SystemEnvironment.prepend("LDRAW_DIRECTORY=" + LDrawLibPath);

	mProcess->setEnvironment(SystemEnvironment);

	mProcess->setWorkingDirectory(QDir::toNativeSeparators(QString("%1/blender").arg(mDataPath)));

	mProcess->setStandardErrorFile(GetStdErrFileName());

	if (mDialogMode == lcRenderDialogMode::OpenInBlender)
	{
		QFileInfo Info(GetStdOutFileName());
		if (Info.exists())
			QFile::remove(Info.absoluteFilePath());
#ifdef Q_OS_WIN
		mProcess->startDetached(ShellProgram, QStringList() << "/C" << ScriptFile.fileName());
#else
		mProcess->startDetached(ShellProgram, QStringList() << ScriptFile.fileName());
#endif
		if (mProcess)
		{
			mProcess->kill();
			CloseProcess();
			if (mStdOutList.size())
				WriteStdOut();
			if (Info.exists())
			{
				QFile Log(Info.absoluteFilePath());
				QTime Wait = QTime::currentTime().addSecs(3);
				while (!Log.size() || QTime::currentTime() < Wait)
					QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
				if (Log.size()) {
					if (Log.open(QFile::ReadOnly | QFile::Text))
					{
						QByteArray Ba = Log.readAll();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
						const bool Error = QString(Ba).contains(QRegularExpression("(?:\\w)*ERROR: ", QRegularExpression::CaseInsensitiveOption));
						const bool Warning = QString(Ba).contains(QRegularExpression("(?:\\w)*WARNING: ", QRegularExpression::CaseInsensitiveOption));
#else
						const bool Error = QString(Ba).contains(QRegExp("(?:\\w)*ERROR: ", Qt::CaseInsensitive));
						const bool Warning = QString(Ba).contains(QRegExp("(?:\\w)*WARNING: ", Qt::CaseInsensitive));
#endif
						if (Error || Warning)
						{
							QMessageBox::Icon Icon = QMessageBox::Information;
							const QString Items = Error ? tr("errors%1").arg(Warning ? tr(" and warnings") : "") : Warning ? tr("warnings") : "";
							const QString Title = tr("Open in Blender output");
							const QString Body = tr("Open in Blender encountered %1. See Show Details...").arg(Items);
							lcBlenderPreferences::ShowMessage(this, Body, Title, QString(), QString(Ba), 0, Icon);
						}
					}
				}
			}
			close();
			return;
		}
	}
	else
	{
		ui->RenderProgress->setRange(mBlendProgValue, mBlendProgMax);
		ui->RenderProgress->setValue(1);
#ifdef Q_OS_WIN
		mProcess->start(ShellProgram, QStringList() << "/C" << ScriptFile.fileName());
#else
		mProcess->start(ShellProgram, QStringList() << ScriptFile.fileName());
#endif
	}

	if (mProcess->waitForStarted())
	{
		mRenderButton->setText(tr("Cancel"));
		ui->RenderProgress->setValue(ui->RenderProgress->minimum());
		QApplication::processEvents();
	}
	else
	{
		gMainWindow->mActions[LC_FILE_RENDER_BLENDER]->setEnabled(true);
		Message = tr("Error starting Blender render process");
		QMessageBox::information(this, tr("Render Error"), Message);
		CloseProcess();
	}
}

void lcRenderDialog::ReadStdOut()
{
	if (mDialogMode == lcRenderDialogMode::RenderPOVRay)
		return;

	QString StdOut = QString(mProcess->readAllStandardOutput());
	mStdOutList.append(StdOut);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QRegularExpression RxRenderProgress;
	RxRenderProgress.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
#else
	QRegExp RxRenderProgress;
	RxRenderProgress.setCaseSensitivity(Qt::CaseInsensitive);
#endif
	int BlenderVersionNum = QString(lcGetProfileString(LC_PROFILE_BLENDER_VERSION).mid(1, 1)).toInt();
	bool BlenderVersion3OrGreater = BlenderVersionNum > 2;

	if (BlenderVersion3OrGreater)
		RxRenderProgress.setPattern("Sample (\\d+)\\/(\\d+)");
	else
		RxRenderProgress.setPattern("(\\d+)\\/(\\d+) Tiles");

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QRegularExpressionMatch Match = RxRenderProgress.match(StdOut);
	if (Match.hasMatch())
	{
		mBlendProgValue = Match.captured(1).toInt();
		mBlendProgMax   = Match.captured(2).toInt();
		ui->RenderProgress->setMaximum(mBlendProgMax);
		ui->RenderProgress->setValue(mBlendProgValue);
	}
#else
	if (StdOut.contains(RxRenderProgress))
	{
		mBlendProgValue = RxRenderProgress.cap(1).toInt();
		mBlendProgMax   = RxRenderProgress.cap(2).toInt();
		ui->RenderProgress->setMaximum(mBlendProgMax);
		ui->RenderProgress->setValue(mBlendProgValue);
	}
#endif
}

QString lcRenderDialog::ReadStdErr(bool& HasError) const
{
	HasError = mDialogMode == lcRenderDialogMode::RenderBlender ? false : true;

	QFile File;
	QStringList ReturnLines;

	File.setFileName(GetStdErrFileName());

	if (!File.open(QFile::ReadOnly | QFile::Text))
	{
		const QString message = tr("Failed to open log file: %1:\n%2").arg(File.fileName()).arg(File.errorString());
		return message;
	}

	QTextStream In(&File);
	while (!In.atEnd())
	{
		QString Line = In.readLine(0);
		ReturnLines << Line.trimmed() + "<br>";

		if (mDialogMode == lcRenderDialogMode::RenderPOVRay)
		{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
			if (Line.contains(QRegularExpression("^POV-Ray finished$", QRegularExpression::CaseInsensitiveOption)))
				HasError = false;
#else
			if (Line.contains(QRegExp("^POV-Ray finished$", Qt::CaseSensitive)))
				HasError = false;
#endif
		}
		else if (mDialogMode == lcRenderDialogMode::RenderBlender)
		{
			if (!HasError && !Line.isEmpty())
				HasError = true;
		}
	}

	return ReturnLines.join(" ");
}

void lcRenderDialog::WriteStdOut()
{
	QFile File(GetStdOutFileName());

	if (File.open(QFile::WriteOnly | QIODevice::Truncate | QFile::Text))
	{
		QTextStream Out(&File);

		for (const QString& Line : mStdOutList)
			Out << Line;

		File.close();

		mLogButton->setEnabled(true);
	}
}

void lcRenderDialog::Update()
{
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

	if (mDialogMode == lcRenderDialogMode::RenderPOVRay)
	{
#if LC_POVRAY_MEMORY_MAPPED_FILE
		if (!mOutputBuffer)
		{
			mOutputFile.setFileName(GetStdOutFileName());

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

		if (!Header->PixelsRead)
			mImage = QImage(Width, Height, QImage::Format_ARGB32);

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

		if (PixelsWritten == Width * Height)
			ui->RenderProgress->setValue(ui->RenderProgress->maximum());

		ui->preview->SetImage(mImage.scaled(mPreviewWidth, mPreviewHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
#endif
	}
}

void lcRenderDialog::ShowResult()
{
	bool Error;
	const QString StdErrLog = ReadStdErr(Error);

	if (mProcess->exitStatus() != QProcess::NormalExit || mProcess->exitCode() != 0 || Error)
	{
		ui->RenderProgress->setRange(0, 1);
		ui->RenderProgress->setValue(0);

		const QString Body  = tr("An error occurred while rendering.");
		lcBlenderPreferences::ShowMessage(this, Body, tr("Render Error"), QString(), StdErrLog, 0, QMessageBox::Information);
		return;
	}
	else
	{
		ui->RenderProgress->setValue(ui->RenderProgress->maximum());
	}

	bool Success = false;
	QString FileName = ui->OutputEdit->text();

	if (mDialogMode == lcRenderDialogMode::RenderPOVRay)
	{
		gMainWindow->mActions[LC_FILE_RENDER_POVRAY]->setEnabled(true);

		ui->preview->SetImage(mImage.scaled(mPreviewWidth, mPreviewHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));

		if (!FileName.isEmpty())
		{
			QImageWriter Writer(FileName);

			Success = Writer.write(mImage);

			if (!Success)
				QMessageBox::information(this, tr("Render Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));
		}

	}
	else if (mDialogMode == lcRenderDialogMode::RenderBlender)
	{
		gMainWindow->mActions[LC_FILE_RENDER_BLENDER]->setEnabled(true);

		Success = QFileInfo(FileName).exists();
		if (Success)
		{
			setMinimumSize(100, 100);
			QImageReader Reader(FileName);
			mImage = Reader.read();
			mImage = mImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

			mPreviewWidth  = mImage.width();
			mPreviewHeight = mImage.height();

			ui->preview->SetImage(mImage.scaled(mPreviewWidth, mPreviewHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
	}

	if (!Success)
		QMessageBox::information(this, tr("Render Error"), tr("Render failed."));

	WriteStdOut();
}

void lcRenderDialog::on_OutputBrowseButton_clicked()
{
	const QString Result = QFileDialog::getSaveFileName(this, tr("Select Output File"), ui->OutputEdit->text(), tr("Supported Image Files (*.bmp *.png *.jpg);;BMP Files (*.bmp);;PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*.*)"));

	if (!Result.isEmpty())
		ui->OutputEdit->setText(QDir::toNativeSeparators(Result));
}

void lcRenderDialog::SettingsButtonClicked()
{
	if (mDialogMode == lcRenderDialogMode::RenderPOVRay)
	{
		QDialog Dialog(this);

		Dialog.setWindowTitle(tr("POV-Ray Settings"));

		QFormLayout* Layout = new QFormLayout(&Dialog);

		QLineEdit* WidthEdit = new QLineEdit(&Dialog);
		Layout->addRow(tr("Width:"), WidthEdit);

		WidthEdit->setText(QString::number(mWidth));
		WidthEdit->setValidator(new QIntValidator(16, INT_MAX, this));

		QLineEdit* HeightEdit = new QLineEdit(&Dialog);
		Layout->addRow(tr("Height:"), HeightEdit);

		HeightEdit->setText(QString::number(mHeight));
		HeightEdit->setValidator(new QIntValidator(16, INT_MAX, this));

		QComboBox* QualityComboBox = new QComboBox(&Dialog);
		Layout->addRow(tr("Quality:"), QualityComboBox);

		QualityComboBox->addItems({ tr("Low"), tr("Medium"), tr("High") });
		QualityComboBox->setCurrentIndex(static_cast<int>(mQuality));

		QDialogButtonBox* ButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &Dialog);
		Layout->addWidget(ButtonBox);

		QObject::connect(ButtonBox, &QDialogButtonBox::rejected, &Dialog, &QDialog::reject);
		QObject::connect(ButtonBox, &QDialogButtonBox::accepted, &Dialog, &QDialog::accept);

		if (Dialog.exec() != QDialog::Accepted)
			return;

		mWidth = std::clamp(WidthEdit->text().toInt(), 16, INT_MAX);
		mHeight = std::clamp(HeightEdit->text().toInt(), 16, INT_MAX);
		mQuality = static_cast<lcPOVRayRenderQuality>(QualityComboBox->currentIndex());

		lcSetProfileInt(LC_PROFILE_RENDER_WIDTH, mWidth);
		lcSetProfileInt(LC_PROFILE_RENDER_HEIGHT, mHeight);
		lcSetProfileInt(LC_PROFILE_RENDER_QUALITY, static_cast<int>(mQuality));
	}
	else
	{
		lcBlenderPreferencesDialog::GetBlenderPreferences(mWidth, mHeight, mScale, this);

		if (lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty())
			mRenderButton->setToolTip(tr("Blender not configured. Use Settings to configure."));
		else
			mRenderButton->setEnabled(true);
	}
}

void lcRenderDialog::LogButtonClicked()
{
	QFileInfo FileInfo;
	QString Message ;

	if (mDialogMode == lcRenderDialogMode::RenderPOVRay)
	{
		FileInfo.setFile(GetStdErrFileName());
		Message = tr("POV-Ray standard error file not found: %1.").arg(FileInfo.absoluteFilePath());
	}
	else
	{
		FileInfo.setFile(GetStdOutFileName());
		Message = tr("Blender standard output file not found: %1.").arg(FileInfo.absoluteFilePath());
	}

	if (!FileInfo.exists())
	{
		QMessageBox::information(this, tr("Render Log"), Message);
		return;
	}

	QFile File(FileInfo.absoluteFilePath());

	if (!File.open(QFile::ReadOnly | QFile::Text))
	{
		QMessageBox::information(this, tr("Render Log"), tr("Error opening log file '%1':\n%2.").arg(FileInfo.absoluteFilePath(), File.errorString()));
		return;
	}

	QString Text = File.readAll();
	File.close();

	QDialog Dialog(this);

	Dialog.setWindowTitle(tr("Render Log"));
	Dialog.resize(800, 600);

	QVBoxLayout* Layout = new QVBoxLayout(&Dialog);

	QTextBrowser* TextBrowser = new QTextBrowser(&Dialog);
	Layout->addWidget(TextBrowser);

	TextBrowser->setText(Text);

	QDialogButtonBox* ButtonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &Dialog);
	Layout->addWidget(ButtonBox);

	QObject::connect(ButtonBox, &QDialogButtonBox::rejected, &Dialog, &QDialog::reject);
	QObject::connect(ButtonBox, &QDialogButtonBox::accepted, &Dialog, &QDialog::accept);

	Dialog.exec();
}

lcRenderProcess::~lcRenderProcess()
{
	if (state() == QProcess::Running || state() == QProcess::Starting)
	{
		terminate();
		waitForFinished();
	}
}

#endif // LC_DISABLE_RENDER_DIALOG

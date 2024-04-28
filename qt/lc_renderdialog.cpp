#include "lc_global.h"
#include "lc_renderdialog.h"
#include "ui_lc_renderdialog.h"
#include "project.h"
#include "lc_application.h"
#include "lc_profile.h"
#include "lc_blenderpreferences.h"
#include "lc_mainwindow.h"
#include "lc_model.h"

#define LC_POVRAY_PREVIEW_WIDTH 768
#define LC_POVRAY_PREVIEW_HEIGHT 432
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
#define LC_POVRAY_MEMORY_MAPPED_FILE 1
#endif

static inline QString ElapsedTime(const qint64& Duration)
{
	qint64 Elapsed = Duration;
	int Milliseconds = int(Elapsed % 1000);
	Elapsed /= 1000;
	int Seconds = int(Elapsed % 60);
	Elapsed /= 60;
	int Minutes = int(Elapsed % 60);
	Elapsed /= 60;
	int Hours = int(Elapsed % 24);

	return QObject::tr("Elapsed time: %1%2%3")
					   .arg(Hours > 0 ? QString("%1 %2 ").arg(Hours).arg(Hours > 1 ? QObject::tr("hours") : QObject::tr("hour")) : QString())
					   .arg(Minutes > 0 ? QString("%1 %2 ").arg(Minutes).arg(Minutes > 1 ? QObject::tr("minutes") : QObject::tr("minute")) : QString())
					   .arg(QString("%1.%2 %3").arg(Seconds).arg(Milliseconds,3,10,QLatin1Char('0')).arg(Seconds > 1 ? QObject::tr("seconds") : QObject::tr("second")));
}

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

lcRenderDialog::lcRenderDialog(QWidget* Parent, int Command)
	: QDialog(Parent),
	mCommand(Command),
	ui(new Ui::lcRenderDialog)
{
#ifndef QT_NO_PROCESS
	mProcess = nullptr;
#endif
	mOutputBuffer = nullptr;

	ui->setupUi(this);

	mWidth = lcGetProfileInt(LC_PROFILE_RENDER_WIDTH);
	mHeight = lcGetProfileInt(LC_PROFILE_RENDER_HEIGHT);
	mScale = 1.0f;

	ui->WidthEdit->setText(QString::number(mWidth));
	ui->WidthEdit->setValidator(new QIntValidator(16, INT_MAX));
	ui->HeightEdit->setText(QString::number(mHeight));
	ui->HeightEdit->setValidator(new QIntValidator(16, INT_MAX));
	ui->OutputEdit->setText(lcGetActiveProject()->GetImageFileName(false));

	lcModel* Model = lcGetActiveProject()->GetActiveModel();
	mLabelMessage = tr("Render image:");
	if (Model)
		mLabelMessage = tr("Render <b>STEP %1</b> image:").arg(Model->GetCurrentStep());

	ui->renderLabel->setText(mLabelMessage);
	ui->RenderOutputButton->setEnabled(false);

	if (mCommand == POVRAY_RENDER)
	{
		setWindowTitle(tr("POV-Ray Image Render"));

		ui->RenderButton->setToolTip(tr("Render LDraw model"));

		QImage Image(LC_POVRAY_PREVIEW_WIDTH, LC_POVRAY_PREVIEW_HEIGHT, QImage::Format_RGB32);
		Image.fill(QColor(255, 255, 255));
		ui->preview->SetImage(Image);

		ui->RenderSettingsButton->hide();
	}
	else
	{
		setWindowTitle(tr("Blender %1").arg(mCommand == OPEN_IN_BLENDER ? tr("LDraw Import") : tr("Image Render")));

		bool BlenderConfigured = !lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty();

		QStringList const& DataPathList = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
		if (!QDir(QString("%1/Blender/addons/%2").arg(DataPathList.first()).arg(LC_BLENDER_ADDON_FOLDER_STR)).isReadable())
		{
			BlenderConfigured = false;
			lcSetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE, QString());
		}

		if (BlenderConfigured)
			mImportModule = lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE) == QLatin1String("TN") ? tr("LDraw Import TN") : tr("LDraw Import MM");

		ui->RenderButton->setToolTip(BlenderConfigured ? tr("Render LDraw Model") : tr("Blender not configured. Use Settings... to configure."));

		ui->RenderButton->setEnabled(BlenderConfigured);

		ui->RenderSettingsButton->setToolTip(tr("Blender render settings"));

		ui->qualityLabel->hide();
		ui->QualityComboBox->hide();

		if (mCommand == OPEN_IN_BLENDER)
		{
			mLabelMessage = tr("Open%1 in Blender using %2:") .arg(Model ? tr(" <b>STEP %1</b>").arg(Model->GetCurrentStep()) : "");

			ui->RenderSettingsButton->setToolTip(tr("Blender import settings"));

			ui->RenderButton->setText(tr("Open in Blender"));
			ui->RenderButton->setFixedWidth(ui->RenderButton->sizeHint().width() + 20);

			if (BlenderConfigured)
				ui->RenderButton->setToolTip(tr("Import and open LDraw model in Blender"));

			ui->renderLabel->setText(mLabelMessage.arg(mImportModule));
			ui->renderLabel->setAlignment(Qt::AlignTrailing | Qt::AlignVCenter);

			ui->outputLabel->hide();
			ui->OutputEdit->hide();
			ui->RenderProgress->hide();
			ui->OutputBrowseButton->hide();
			ui->RenderOutputButton->hide();

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

		connect(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(UpdateElapsedTime()));
	}

	connect(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(Update()));

	mUpdateTimer.start(500);

	setSizeGripEnabled(true);
}

lcRenderDialog::~lcRenderDialog()
{
	delete ui;
}

QString lcRenderDialog::GetStdOutFileName() const
{
	QString LogFile = mCommand == POVRAY_RENDER ? QLatin1String("leocad-povray-render.out") : QLatin1String("leocad-blender-render.out");

	return QDir(QDir::tempPath()).absoluteFilePath(LogFile);
}

QString lcRenderDialog::GetStdErrFileName() const
{
	QString LogFile = mCommand == POVRAY_RENDER ? QLatin1String("leocad-povray-render.err") : QLatin1String("leocad-blender-render.err");

	return QDir(QDir::tempPath()).absoluteFilePath(LogFile);
}

QString lcRenderDialog::GetPOVFileName() const
{
	return QDir(QDir::tempPath()).absoluteFilePath("leocad-render.pov");
}

void lcRenderDialog::UpdateElapsedTime() const
{
	if (mProcess && mCommand == BLENDER_RENDER)
	{
		const QString RenderType = lcGetProfileString(LC_PROFILE_BLENDER_VERSION).startsWith("v3") ? QLatin1String("Samples") : QLatin1String("Tiles");
		ui->renderLabel->setText(tr("%1: %2/%3, %4") .arg(RenderType) .arg(mBlendProgValue) .arg(mBlendProgMax) .arg(ElapsedTime(mRenderTime.elapsed())));
	}
}

void lcRenderDialog::CloseProcess()
{
#ifndef QT_NO_PROCESS
	delete mProcess;
	mProcess = nullptr;
#endif

	if (mCommand == POVRAY_RENDER)
	{
#if LC_POVRAY_MEMORY_MAPPED_FILE
		mOutputFile.unmap((uchar*)mOutputBuffer);
		mOutputBuffer = nullptr;
		mOutputFile.close();

		QFile::remove(GetStdOutFileName());
#endif

		QFile::remove(GetPOVFileName());
	}

	if (mCommand != OPEN_IN_BLENDER)
		ui->RenderButton->setText(tr("Render"));
}

bool lcRenderDialog::PromptCancel()
{
#ifndef QT_NO_PROCESS
	if (mProcess)
	{
		if (QMessageBox::question(this, tr("Cancel Render"), tr("Are you sure you want to cancel the current render?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
		{
			if (mCommand == POVRAY_RENDER)
				gMainWindow->mActions[LC_FILE_RENDER_POVRAY]->setEnabled(true);
			else if (mCommand == BLENDER_RENDER)
				gMainWindow->mActions[LC_FILE_RENDER_BLENDER]->setEnabled(true);
#ifdef Q_OS_WIN
			lcTerminateChildProcess(this, mProcess->processId(), QCoreApplication::applicationPid());
#endif
			mProcess->kill();
			CloseProcess();
			if (mStdOutList.size())
			{
				WriteStdOut();
				ui->RenderOutputButton->setEnabled(true);
			}
			if (mCommand == BLENDER_RENDER)
				ui->renderLabel->setText(tr("Tiles: %1/%2, Render Cancelled.") .arg(mBlendProgValue) .arg(mBlendProgMax));
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

void lcRenderDialog::on_RenderSettingsButton_clicked()
{
	if (mCommand == POVRAY_RENDER)
		return;

	lcBlenderPreferencesDialog::GetBlenderPreferences(mWidth, mHeight, mScale, this);

	if (lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE).isEmpty())
		ui->RenderButton->setToolTip(tr("Blender not configured. Use Settings... to configure."));
	else
	{
		if (mCommand == OPEN_IN_BLENDER)
		{
			mImportModule = lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE) == QLatin1String("TN") ? tr("LDraw Import TN") : tr("LDraw Import MM");
			ui->renderLabel->setText(mLabelMessage.arg(mImportModule));
			ui->renderLabel->setAlignment(Qt::AlignTrailing | Qt::AlignVCenter);
		}

		ui->RenderButton->setEnabled(true);
	}
}

void lcRenderDialog::on_RenderButton_clicked()
{
#ifndef QT_NO_PROCESS
	if (mProcess)
	{
		PromptCancel();
		return;
	}

	ui->RenderOutputButton->setEnabled(false);

	mPreviewWidth  = ui->preview->width();
	mPreviewHeight = ui->preview->height();

	mRenderTime.start();

	if (mCommand == POVRAY_RENDER)
	{
		gMainWindow->mActions[LC_FILE_RENDER_POVRAY]->setEnabled(false);

		QString FileName = GetPOVFileName();

		QImage Image(mPreviewWidth, mPreviewHeight, QImage::Format_RGB32);
		Image.fill(QColor(255, 255, 255));
		ui->preview->SetImage(Image);

		if (!lcGetActiveProject()->ExportPOVRay(FileName))
			return;

		QStringList Arguments;

		Arguments.append(QString::fromLatin1("+I\"%1\"").arg(FileName));
		Arguments.append(QString::fromLatin1("+W%1").arg(ui->WidthEdit->text()));
		Arguments.append(QString::fromLatin1("+H%1").arg(ui->HeightEdit->text()));
		Arguments.append("-O-");

#if LC_POVRAY_MEMORY_MAPPED_FILE
		Arguments.append(QString::fromLatin1("+SM\"%1\"").arg(GetStdOutFileName()));
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
		if (lcGetActiveProject()->GetModels()[0]->GetPOVRayOptions().UseLGEO) {
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

		mProcess = new lcRenderProcess(this);
#ifdef Q_OS_LINUX
		connect(mProcess, SIGNAL(readyReadStandardError()), this, SLOT(ReadStdErr()));
#endif
		QStringList POVEnv = QProcess::systemEnvironment();
		POVEnv.prepend("POV_IGNORE_SYSCONF_MSG=1");
		mProcess->setEnvironment(POVEnv);
		mProcess->setStandardErrorFile(GetStdErrFileName());
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
			gMainWindow->mActions[LC_FILE_RENDER_POVRAY]->setEnabled(true);
			QMessageBox::warning(this, tr("Error"), tr("Error starting POV-Ray."));
			CloseProcess();
		}
	}
	else
	{
		const QString BlenderLDrawConfigFile = lcGetProfileString(LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH);
		const QString BlenderImportModule = lcGetProfileString(LC_PROFILE_BLENDER_IMPORT_MODULE);
		if (!QFileInfo(BlenderLDrawConfigFile).isReadable() && !BlenderImportModule.isEmpty())
			lcBlenderPreferences::SaveSettings();

		 const QString Option = mCommand == OPEN_IN_BLENDER ? tr("import") : tr("render");

		ui->renderLabel->setText(tr("Saving Blender %1 model...").arg(Option));

		QApplication::processEvents();

		mBlendProgValue = 0;
		mBlendProgMax   = 0;

		const QStringList DataPathList = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
		mDataPath = DataPathList.first();
		 const QString DefaultBlendFile = QString("%1/blender/config/%2").arg(mDataPath).arg(LC_BLENDER_ADDON_BLEND_FILE);

		lcModel* Model = lcGetActiveProject()->GetActiveModel();
		const QString ModelFileName = QFileInfo(QDir(lcGetProfileString(LC_PROFILE_PROJECTS_PATH)), QString("%1_Step_%2.ldr").arg(QFileInfo(Model->GetProperties().mFileName).baseName()).arg(Model->GetCurrentStep())).absoluteFilePath();

		lcGetActiveProject()->ExportCurrentStep(ModelFileName);

		if (!QFileInfo(ModelFileName).isReadable())
			return;

		bool SearchCustomDir = true;

		QString Message;
		QStringList Arguments;
		QString PythonExpression = QString("\"import bpy; bpy.ops.render_scene.lpub3d_render_ldraw("
										   "'EXEC_DEFAULT', "
										   "resolution_width=%1, resolution_height=%2, "
										   "render_percentage=%3, model_file=r'%4', "
										   "image_file=r'%5', preferences_file=r'%6'")
										   .arg(mWidth).arg(mHeight)
										   .arg(mScale * 100)
										   .arg(QDir::toNativeSeparators(ModelFileName).replace("\\","\\\\"))
										   .arg(QDir::toNativeSeparators(ui->OutputEdit->text()).replace("\\","\\\\"))
										   .arg(QDir::toNativeSeparators(BlenderLDrawConfigFile).replace("\\","\\\\"));
		if (BlenderImportModule == QLatin1String("MM"))
			PythonExpression.append(", use_ldraw_import_mm=True");
		if (SearchCustomDir)
			PythonExpression.append(", search_additional_paths=True");
		if (mCommand == OPEN_IN_BLENDER)
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
		ScriptCommand = QString("%1 %2").arg(lcGetProfileString(LC_PROFILE_BLENDER_PATH)).arg(Arguments.join(" "));

		if (mCommand == OPEN_IN_BLENDER)
			ScriptCommand.append(QString(" > %1").arg(QDir::toNativeSeparators(GetStdOutFileName())));

		const QLatin1String LineEnding("\r\n");

		QFile Script(QString("%1/%2").arg(QDir::tempPath()).arg(ScriptName));
		if(Script.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream Stream(&Script);
#ifdef Q_OS_WIN
			Stream << QLatin1String("@ECHO OFF &SETLOCAL") << LineEnding;
#else
			Stream << QLatin1String("#!/bin/bash") << LineEnding;
#endif
			Stream << ScriptCommand << LineEnding;
			Script.close();
		}
		else
		{
			QMessageBox::warning(this, tr("Error"), tr("Cannot write Blender render script file [%1] %2.").arg(Script.fileName()).arg(Script.errorString()));

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

		if (mCommand == OPEN_IN_BLENDER)
		{
			QFileInfo Info(GetStdOutFileName());
			if (Info.exists())
				QFile::remove(Info.absoluteFilePath());
#ifdef Q_OS_WIN
			mProcess->startDetached(ShellProgram, QStringList() << "/C" << Script.fileName());
#else
			mProcess->startDetached(ShellProgram, QStringList() << Script.fileName());
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
								QMessageBox::Icon Icon = QMessageBox::Warning;
								const QString Items = Error ? tr("errors%1").arg(Warning ? tr(" and warnings") : "") : Warning ? tr("warnings") : "";
								const QString Title = tr("Open in Blender output");
								const QString Body = tr("Open in Blender encountered %1. See Show Details...").arg(Items);
								lcBlenderPreferences::ShowMessage(Body, Title, QString(), QString(Ba), 0, Icon);
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
#ifdef Q_OS_WIN
			mProcess->start(ShellProgram, QStringList() << "/C" << Script.fileName());
#else
			mProcess->start(ShellProgram, QStringList() << Script.fileName());
#endif
		}

		if (mProcess->waitForStarted())
		{
			ui->RenderButton->setText(tr("Cancel"));
			ui->RenderProgress->setValue(ui->RenderProgress->minimum());
			ui->renderLabel->setText(tr("Loading LDraw model... %1").arg(ElapsedTime(mRenderTime.elapsed())));
			QApplication::processEvents();
		}
		else
		{
			gMainWindow->mActions[LC_FILE_RENDER_BLENDER]->setEnabled(true);
			Message = tr("Error starting Blender render process");
			QMessageBox::warning(this, tr("Error"), Message);
			CloseProcess();
		}
	}  // BLENDER_RENDER
#endif
}

void lcRenderDialog::ReadStdOut()
{
	if (mCommand == POVRAY_RENDER)
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
	bool BlenderVersion3 = lcGetProfileString(LC_PROFILE_BLENDER_VERSION).startsWith("v3");

	if (BlenderVersion3)
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
	HasError = mCommand == BLENDER_RENDER ? false : true;

	QFile File;
	QStringList returnLines;

	File.setFileName(GetStdErrFileName());

	if (! File.open(QFile::ReadOnly | QFile::Text))
	{
		const QString message = tr("Failed to open log file: %1:\n%2").arg(File.fileName()).arg(File.errorString());
		return message;
	}

	QTextStream In(&File);
	while (! In.atEnd())
	{
		QString Line = In.readLine(0);
		returnLines << Line.trimmed() + "<br>";
		if (mCommand == POVRAY_RENDER)
		{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
			if (Line.contains(QRegularExpression("^POV-Ray finished$", QRegularExpression::CaseInsensitiveOption)))
				HasError = false;
#else
			if (Line.contains(QRegExp("^POV-Ray finished$", Qt::CaseSensitive)))
				HasError = false;
#endif
		}
		else if (mCommand == BLENDER_RENDER)
		{
			if (!HasError && !Line.isEmpty())
				HasError = true;
		}
	}

	return returnLines.join(" ");
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

		ui->RenderOutputButton->setEnabled(true);
	}
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

	if (mCommand == POVRAY_RENDER)
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
#ifndef QT_NO_PROCESS
	bool Error;

	const QString StdErrLog = ReadStdErr(Error);

	const QString RenderLabel = mCommand == BLENDER_RENDER ? tr("Blender Render") : tr("POV-Ray Render");

	if (mProcess->exitStatus() != QProcess::NormalExit || mProcess->exitCode() != 0 || Error)
	{
		ui->renderLabel->setText(tr("Image generation failed."));
		ui->RenderProgress->setRange(0,1);
		ui->RenderProgress->setValue(0);

		const QString Title = mCommand == BLENDER_RENDER ? tr("Blender Render") : tr("POV-Ray Render");
		const QString Body  = tr ("An error occurred while rendering. See Show Details...");
		lcBlenderPreferences::ShowMessage(Body, Title, QString(), StdErrLog, 0, QMessageBox::Warning);
		return;
	}
	else
	{
		ui->RenderProgress->setValue(ui->RenderProgress->maximum());
	}
#endif

	bool Success = false;

	QString FileName = ui->OutputEdit->text();

	if (mCommand == POVRAY_RENDER)
	{
		gMainWindow->mActions[LC_FILE_RENDER_POVRAY]->setEnabled(true);

		ui->preview->SetImage(mImage.scaled(mPreviewWidth, mPreviewHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));

		if (!FileName.isEmpty())
		{
			QImageWriter Writer(FileName);

			Success = Writer.write(mImage);

			if (!Success)
				QMessageBox::warning(this, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));
		}

	}
	else if (mCommand == BLENDER_RENDER)
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
	{
		ui->renderLabel->setStyleSheet("QLabel { color : red; }");
		ui->renderLabel->setText(tr("Image render failed."));
		const QString Message = QString("%1 %2. %3").arg(RenderLabel).arg(tr("failed (unknown reason)")).arg(ElapsedTime(mRenderTime.elapsed()));
		QMessageBox::warning(this, tr("Error"), Message);
	}

	WriteStdOut();
}

void lcRenderDialog::on_OutputBrowseButton_clicked()
{
	const QString Result = QFileDialog::getSaveFileName(this, tr("Select Output File"), ui->OutputEdit->text(), tr("Supported Image Files (*.bmp *.png *.jpg);;BMP Files (*.bmp);;PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*.*)"));

	if (!Result.isEmpty())
		ui->OutputEdit->setText(QDir::toNativeSeparators(Result));
}

void lcRenderDialog::on_RenderOutputButton_clicked()
{
	QFileInfo FileInfo(GetStdErrFileName());
	QString Message = tr("POV-Ray standard error file not found: %1.").arg(FileInfo.absoluteFilePath());
	if (mCommand == BLENDER_RENDER)
	{
		FileInfo.setFile(GetStdOutFileName());
		Message = tr("Blender standard output file not found: %1.").arg(FileInfo.absoluteFilePath());
	}
	if (!FileInfo.exists())
	{
		QMessageBox::warning(this, tr("Error"), Message);
		return;
	}

	QDesktopServices::openUrl(QUrl("file:///"+FileInfo.absoluteFilePath(), QUrl::TolerantMode));
}

lcRenderProcess::~lcRenderProcess()
{
	if(state() == QProcess::Running || state() == QProcess::Starting)
	{
		terminate();
		waitForFinished();
	}
}

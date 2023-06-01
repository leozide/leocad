#pragma once

#include <QDialog>

namespace Ui {
class lcRenderDialog;
}

class QProcess;
class RenderProcess : public QProcess
{
	Q_OBJECT

public:
	explicit RenderProcess(QObject* parent = nullptr)
		: QProcess(parent)
	{
	}
	~RenderProcess();
};

class lcRenderPreviewWidget : public QWidget
{
	Q_OBJECT

public:
	lcRenderPreviewWidget(QWidget* Parent)
		: QWidget(Parent)
	{
	}

	void SetImage(QImage Image)
	{
		mImage = Image;
		mScaledImage = QImage();
		update();
	}

protected:
	void resizeEvent(QResizeEvent* Event) override;
	void paintEvent(QPaintEvent* PaintEvent) override;

	QImage mImage;
	QImage mScaledImage;
};

class lcRenderDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcRenderDialog(QWidget* Parent, int Command);
	~lcRenderDialog();

public slots:
	void reject() override;
	void on_RenderButton_clicked();
	void on_OutputBrowseButton_clicked();
	void on_RenderSettingsButton_clicked();
	void on_RenderOutputButton_clicked();
	void Update();

protected slots:
	void ReadStdOut();
	void WriteStdOut();
	void UpdateElapsedTime() const;

protected:
	QString GetOutputFileName(int StdErr = 0) const;
	QString GetPOVFileName() const;
	QString ReadStdErr(bool& Error) const;
	void CloseProcess();
	bool PromptCancel();
	void ShowResult();
#ifdef Q_OS_WIN
	int TerminateChildProcess(const qint64 Pid, const qint64 Ppid);
#endif
#ifndef QT_NO_PROCESS
	RenderProcess* mProcess;
#endif
	enum CommandType
	{
		POVRAY_RENDER,
		BLENDER_RENDER,
		OPEN_IN_BLENDER
	};

	QTimer mUpdateTimer;
	QElapsedTimer mRenderTime;
	QFile mOutputFile;
	void* mOutputBuffer;
	QImage mImage;
	QStringList mStdErrList;
	QStringList mStdOutList;

	int mWidth;
	int mHeight;
	int mPreviewWidth;
	int mPreviewHeight;
	int mCommand;
	int mBlendProgValue;
	int mBlendProgMax;
	double mScale;
	QString mImportModule;
	QString mDataPath;

	Ui::lcRenderDialog* ui;
};

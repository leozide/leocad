#pragma once

#include <QDialog>

namespace Ui {
class lcRenderDialog;
}

class lcRenderProcess : public QProcess
{
	Q_OBJECT

public:
	explicit lcRenderProcess(QObject* parent = nullptr)
		: QProcess(parent)
	{
	}
	~lcRenderProcess();
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
	QString GetStdOutFileName() const;
	QString GetStdErrFileName() const;
	QString GetPOVFileName() const;
	QString ReadStdErr(bool& Error) const;
	void CloseProcess();
	bool PromptCancel();
	void ShowResult();
#ifndef QT_NO_PROCESS
	lcRenderProcess* mProcess;
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
	QString mLabelMessage;
	QString mDataPath;

	Ui::lcRenderDialog* ui;
};

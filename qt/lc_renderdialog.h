#pragma once

#ifndef LC_DISABLE_RENDER_DIALOG

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

enum class lcRenderDialogMode
{
	RenderPOVRay,
	RenderBlender,
	OpenInBlender
};

enum class lcPOVRayRenderQuality
{
	Low,
	Medium,
	High
};

class lcRenderDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcRenderDialog(QWidget* Parent, lcRenderDialogMode Mode);
	virtual ~lcRenderDialog();

public slots:
	void reject() override;
	void on_OutputBrowseButton_clicked();
	void Update();

protected slots:
	void RenderButtonClicked();
	void SettingsButtonClicked();
	void LogButtonClicked();
	void ReadStdOut();
	void WriteStdOut();

protected:
	QString GetStdOutFileName() const;
	QString GetStdErrFileName() const;
	QString GetPOVFileName() const;
	QString ReadStdErr(bool& Error) const;
	void RenderPOVRay();
	void RenderBlender();
	void CloseProcess();
	bool PromptCancel();
	void ShowResult();

	QTimer mUpdateTimer;
	QElapsedTimer mRenderTime;
	QFile mOutputFile;
	void* mOutputBuffer = nullptr;
	QImage mImage;
	QStringList mStdErrList;
	QStringList mStdOutList;
	QPushButton* mRenderButton = nullptr;
	QPushButton* mSettingsButton = nullptr;
	QPushButton* mLogButton = nullptr;
	lcRenderProcess* mProcess = nullptr;

	int mWidth = 1280;
	int mHeight = 720;
	lcPOVRayRenderQuality mQuality = lcPOVRayRenderQuality::High;
	int mPreviewWidth;
	int mPreviewHeight;
	lcRenderDialogMode mDialogMode;
	int mBlendProgValue;
	int mBlendProgMax;
	double mScale;
	QString mDataPath;

	Ui::lcRenderDialog* ui;
};

#endif // LC_DISABLE_RENDER_DIALOG

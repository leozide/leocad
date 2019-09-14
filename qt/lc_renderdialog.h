#pragma once

#include <QDialog>

namespace Ui {
class lcRenderDialog;
}

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
	virtual void resizeEvent(QResizeEvent* Event) override;
	virtual void paintEvent(QPaintEvent* PaintEvent) override;

	QImage mImage;
	QImage mScaledImage;
};

class lcRenderDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcRenderDialog(QWidget* Parent);
	~lcRenderDialog();

public slots:
	void reject();
	void on_RenderButton_clicked();
	void on_OutputBrowseButton_clicked();
	void Update();

protected slots:
	void ReadStdErr();
	void WriteStdLog(bool = false);

protected:
	QString GetOutputFileName() const;
	QString GetPOVFileName() const;
	void CloseProcess();
	bool PromptCancel();
	void ShowResult();

#ifndef QT_NO_PROCESS
	QProcess* mProcess;
#endif
	QTimer mUpdateTimer;
	QFile mOutputFile;
	void* mOutputBuffer;
	QImage mImage;
	QStringList mStdErrList;

	Ui::lcRenderDialog* ui;
};

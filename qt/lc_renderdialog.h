#pragma once

#include <QDialog>

namespace Ui {
class lcRenderDialog;
}

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
	QStringList stdErrList;

	Ui::lcRenderDialog* ui;
};

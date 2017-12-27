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

protected:
	QString GetPOVFileName() const;
	void CloseProcess();
	bool PromptCancel();

#ifndef QT_NO_PROCESS
	QProcess* mProcess;
#endif
	QTimer mUpdateTimer;
	QSharedMemory mSharedMemory;
	QImage mImage;

	Ui::lcRenderDialog* ui;
};

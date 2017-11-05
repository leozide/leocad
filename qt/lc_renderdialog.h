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
	void Update();

protected:
	QString GetPOVFileName() const;
	void CloseProcess();
	bool PromptCancel();

	QProcess* mProcess;
	QTimer mUpdateTimer;
	QSharedMemory mSharedMemory;

	Ui::lcRenderDialog* ui;
};

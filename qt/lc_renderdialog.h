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
	void on_RenderButton_clicked();
	void Update();

protected:
	QTimer mUpdateTimer;
	QProcess* mProcess;
	Ui::lcRenderDialog* ui;

	QString GetPOVFileName() const;
	void CloseSharedMemory();
#ifdef Q_OS_WIN
	HANDLE mMapFile;
	void* mBuffer;
#endif
};

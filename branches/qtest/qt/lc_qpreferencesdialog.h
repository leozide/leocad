#ifndef LC_QPREFERENCESDIALOG_H
#define LC_QPREFERENCESDIALOG_H

#include <QDialog>

namespace Ui {
class lcQPreferencesDialog;
}

class lcQPreferencesDialog : public QDialog
{
	Q_OBJECT
    
public:
	explicit lcQPreferencesDialog(QWidget *parent, void *data);
	~lcQPreferencesDialog();

	enum
	{
		CategoryRole = Qt::UserRole
	};

private:
	Ui::lcQPreferencesDialog *ui;

	void updateCategories();

public slots:
	void updateParts();
};

#endif // LC_QPREFERENCESDIALOG_H

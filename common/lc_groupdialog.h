#pragma once

namespace Ui {
class lcGroupDialog;
}

class lcGroupDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcGroupDialog(QWidget* Parent, const QString& Name, const std::vector<std::unique_ptr<lcGroup>>& Groups);
	virtual ~lcGroupDialog();

	QString mName;

public slots:
	void accept() override;

private:
	const std::vector<std::unique_ptr<lcGroup>>& mGroups;
	Ui::lcGroupDialog* ui;
};

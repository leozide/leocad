#pragma once

class lcElidableToolButton;
class lcView;
enum class lcFindReplaceAction;

class lcFindReplaceWidget : public QWidget
{
	Q_OBJECT

public:
	lcFindReplaceWidget(QWidget* Parent, lcView* View, bool Replace);

protected slots:
	void FocusChanged(QWidget* Previous, QWidget* Focused);
	void FindColorIndexChanged(int ColorIndex);
	void FindTextEdited(const QString& Text);
	void FindActivated(int Index);
	void ReplaceColorIndexChanged(int ColorIndex);
	void ReplaceButtonClicked();

protected:
	QAction* CreateAction(QAction* Source, lcFindReplaceAction Operation);

	lcView* mView;
	QComboBox* mFindPartComboBox = nullptr;
	lcElidableToolButton* mReplacePartButton = nullptr;
};

#pragma once

class lcFindReplaceWidget : public QWidget
{
	Q_OBJECT

public:
	lcFindReplaceWidget(QWidget* Parent, lcModel* Model, bool Replace);

protected slots:
	void FindColorIndexChanged(int ColorIndex);
	void FindTextEdited(const QString& Text);
	void FindActivated(int Index);
	void ReplaceColorIndexChanged(int ColorIndex);
	void ReplaceActivated(int Index);

protected:
	QComboBox* mFindPartComboBox = nullptr;
	QComboBox* mReplacePartComboBox = nullptr;
};

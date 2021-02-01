#pragma once

struct lcFindReplaceParams
{
	PieceInfo* FindInfo = nullptr;
	QString FindString;
	int FindColorIndex = 0;
	PieceInfo* ReplacePieceInfo = nullptr;
	int ReplaceColorIndex = 0;
};

class lcFindReplaceWidget : public QWidget
{
	Q_OBJECT

public:
	lcFindReplaceWidget(QWidget* Parent, lcModel* Model, bool Replace);

	lcFindReplaceParams* GetFindReplaceParams()
	{
		return &mFindReplaceParams;
	}

protected slots:
	void FindColorIndexChanged(int ColorIndex);
	void FindTextEdited(const QString& Text);
	void FindActivated(int Index);
	void ReplaceColorIndexChanged(int ColorIndex);
	void ReplaceActivated(int Index);

protected:
	QComboBox* mFindPartComboBox = nullptr;
	QComboBox* mReplacePartComboBox = nullptr;

	lcFindReplaceParams mFindReplaceParams;
};

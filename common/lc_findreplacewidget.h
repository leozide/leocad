#pragma once

struct lcFindReplaceParams
{
	PieceInfo* FindInfo = nullptr;
	QString FindString;
	int FindColorIndex = 0;

	bool ReplaceInfo = false;
	bool ReplaceColor = false;
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

protected:
	QComboBox* mFindPartComboBox = nullptr;

	lcFindReplaceParams mFindReplaceParams;
};

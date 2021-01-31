#pragma once

struct lcFindReplaceParams
{
	PieceInfo* FindInfo = nullptr;
	QString FindString;

	bool MatchColor = false;
	bool ReplaceInfo = false;
	bool ReplaceColor = false;
	int ColorIndex = 0;
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
	void FindTextEdited(const QString& Text);
	void FindIndexChanged(int Index);

protected:
	QComboBox* mFindPartComboBox = nullptr;

	lcFindReplaceParams mFindReplaceParams;
};

#pragma once

class lcCollapsibleWidget : public QWidget
{
	Q_OBJECT

public:
	lcCollapsibleWidget(const QString& RootTitle, QWidget* Parent = nullptr);

	void Collapse();
	void SetChildLayout(QLayout* Layout);

protected slots:
	void TitleClicked();

protected:
	void UpdateIcon();

	QToolButton* mTitleButton = nullptr;
	QWidget* mChildWidget = nullptr;
	bool mExpanded = true;

	static QImage mExpandedIcon;
	static QImage mCollapsedIcon;
};

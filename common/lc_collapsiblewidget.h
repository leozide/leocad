#pragma once

class lcCollapsibleWidgetButton : public QToolButton
{
	Q_OBJECT

public:
	lcCollapsibleWidgetButton(const QString& Title, QWidget* Parent = nullptr);

	void Collapse();

	bool IsExpanded() const
	{
		return mExpanded;
	}

signals:
	void StateChanged(bool Expanded);

protected slots:
	void Clicked();

protected:
	void UpdateIcon();

	bool mExpanded = true;

	static QImage mExpandedIcon;
	static QImage mCollapsedIcon;
};

class lcCollapsibleWidget : public QWidget
{
	Q_OBJECT

public:
	lcCollapsibleWidget(const QString& RootTitle, QWidget* Parent = nullptr);

	void Collapse();
	void SetChildLayout(QLayout* Layout);

protected slots:
	void ButtonStateChanged(bool Expanded);

protected:
	lcCollapsibleWidgetButton* mTitleButton = nullptr;
	QWidget* mChildWidget = nullptr;
};

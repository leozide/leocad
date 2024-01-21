#pragma once

class lcKeyFrameWidget : public QCheckBox
{
	Q_OBJECT

public:
	lcKeyFrameWidget(QWidget* Parent);

	QSize sizeHint() const override
	{
		return QSize(15, 15);
	}

	void paintEvent(QPaintEvent* PaintEvent) override;
};

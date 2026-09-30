#pragma once

class lcSpinnerWidget : public QWidget
{
public:
	lcSpinnerWidget(QWidget* Parent = nullptr);

protected:
	void paintEvent(QPaintEvent* Event) override;
	void showEvent(QShowEvent* Event) override;
	void hideEvent(QHideEvent* Event) override;

private:
	static constexpr qreal OrbitRadius = 7.0; // Logical pixels from the center.
	static constexpr qreal DiscRadius = 3; // Logical pixels.

	QTimer mTimer;
	QElapsedTimer mElapsed;
};

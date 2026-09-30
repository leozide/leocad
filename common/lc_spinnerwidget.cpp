#include "lc_global.h"
#include "lc_spinnerwidget.h"

lcSpinnerWidget::lcSpinnerWidget(QWidget* Parent)
	: QWidget(Parent)
{
	const int Size = qCeil(2.0 * (OrbitRadius + qMax(DiscRadius, 1.5) + 1.5));
	setFixedSize(Size, Size);
	setAccessibleName(tr("Loading"));
	mTimer.setInterval(16);
	connect(&mTimer, &QTimer::timeout, this, [this]() { update(); });
}

void lcSpinnerWidget::showEvent(QShowEvent* Event)
{
	QWidget::showEvent(Event);
	mElapsed.start();
	mTimer.start();
}

void lcSpinnerWidget::hideEvent(QHideEvent* Event)
{
	mTimer.stop();
	QWidget::hideEvent(Event);
}

void lcSpinnerWidget::paintEvent(QPaintEvent* Event)
{
	Q_UNUSED(Event);

	QPainter Painter(this);
	Painter.setRenderHint(QPainter::Antialiasing);
	Painter.translate(width() / 2.0, height() / 2.0);
	constexpr qreal SpinSpeed = 0.75; // Revolutions per second.
	Painter.rotate(mElapsed.isValid() ? mElapsed.elapsed() * 0.36 * SpinSpeed : 0.0);

	const QRectF Orbit(-OrbitRadius, -OrbitRadius, 2.0 * OrbitRadius, 2.0 * OrbitRadius);
	constexpr qreal TrailLength = 270.0; // Degrees behind the disc (0 to 360).
	constexpr qreal FadeDecay = 5.0; // Positive values; larger values fade faster.
	constexpr int Segments = 120;
	const qreal SegmentAngle = qBound(0.0, TrailLength, 360.0) / Segments;
	const qreal TailOpacity = qExp(-FadeDecay);

	// Paint the oldest end first, keeping the trail white and fading its opacity.
	for (int Segment = Segments - 1; Segment >= 0 && SegmentAngle > 0.0; Segment--)
	{
		const qreal Age = static_cast<qreal>(Segment) / (Segments - 1);
		const qreal Opacity = (qExp(-FadeDecay * Age) - TailOpacity) / (1.0 - TailOpacity);
		QColor Color;
		Color.setRgbF(1.0, 1.0, 1.0, Opacity);
		Painter.setPen(QPen(Color, 3.0, Qt::SolidLine, Qt::RoundCap));
		Painter.drawArc(Orbit, qRound(Segment * SegmentAngle * 16.0), qRound(SegmentAngle * 16.0));
	}

	Painter.setPen(Qt::NoPen);
	Painter.setBrush(Qt::white);
//	Painter.drawEllipse(QPointF(OrbitRadius, 0.0), DiscRadius, DiscRadius);
}

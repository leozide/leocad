#include "lc_global.h"
#include "lc_keyframewidget.h"

lcKeyFrameWidget::lcKeyFrameWidget(QWidget* Parent)
	: QAbstractButton(Parent)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setCheckable(true);
}

void lcKeyFrameWidget::paintEvent(QPaintEvent* PaintEvent)
{
	Q_UNUSED(PaintEvent);

	QPainter Painter(this);

	QRect Rect = rect();
	Painter.fillRect(Rect, palette().brush(QPalette::Window));

	Painter.setPen(hasFocus() ? palette().color(QPalette::Highlight) : palette().color(QPalette::Shadow));
	Painter.setBrush(palette().color(QPalette::Text));

	Rect = isChecked() ? QRect(1, 1, 13, 13) : QRect(4, 4, 7, 7);
	QPoint Center = Rect.center();

	QPoint Points[4] =
	{
		QPoint(Rect.left(), Center.y()),
		QPoint(Center.x(), Rect.bottom()),
		QPoint(Rect.right(), Center.y()),
		QPoint(Center.x(), Rect.top())
	};

	Painter.drawPolygon(Points, 4);
}

#include "lc_global.h"
#include "lc_qcolorpicker.h"
#include "lc_qcolorlist.h"
#include "lc_colors.h"

lcQColorPickerPopup::lcQColorPickerPopup(QWidget *parent, int colorIndex)
	: QFrame(parent, Qt::Popup)
{
	setFrameStyle(QFrame::StyledPanel);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	QGridLayout *layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(layout);

	colorList = new lcQColorList(this);
	connect(colorList, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(colorList, SIGNAL(colorSelected(int)), this, SLOT(colorSelected(int)));
	layout->addWidget(colorList);

	colorList->blockSignals(true);
	colorList->setCurrentColor(colorIndex);
	colorList->blockSignals(false);

	eventLoop = NULL;
}

lcQColorPickerPopup::~lcQColorPickerPopup()
{
	if (eventLoop)
		eventLoop->exit();
}

void lcQColorPickerPopup::exec()
{
	show();

	QEventLoop e;
	eventLoop = &e;
	(void) e.exec();
	eventLoop = NULL;
}

void lcQColorPickerPopup::mouseReleaseEvent(QMouseEvent *e)
{
	if (!rect().contains(e->pos()))
		hide();
}

void lcQColorPickerPopup::hideEvent(QHideEvent *e)
{
	if (eventLoop)
		eventLoop->exit();

	emit hid();
	QFrame::hideEvent(e);
}

void lcQColorPickerPopup::colorChanged(int colorIndex)
{
	emit changed(colorIndex);
}

void lcQColorPickerPopup::colorSelected(int colorIndex)
{
	emit selected(colorIndex);
	hide();
}

void lcQColorPickerPopup::showEvent(QShowEvent *)
{
	colorList->setFocus();
}

lcQColorPicker::lcQColorPicker(QWidget *parent)
	: QPushButton(parent)
{
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	setAutoDefault(false);
	setCheckable(true);

	initialColorIndex = 0;
	currentColorIndex = 0;
	updateIcon();

	connect(this, SIGNAL(toggled(bool)), SLOT(buttonPressed(bool)));
}

lcQColorPicker::~lcQColorPicker()
{
}

void lcQColorPicker::setCurrentColor(int colorIndex)
{
	selected(colorIndex);
}

int lcQColorPicker::currentColor() const
{
	return currentColorIndex;
}

void lcQColorPicker::buttonPressed(bool toggled)
{
	if (!toggled)
		return;

	lcQColorPickerPopup *popup = new lcQColorPickerPopup(this, currentColorIndex);
	connect(popup, SIGNAL(changed(int)), SLOT(changed(int)));
	connect(popup, SIGNAL(selected(int)), SLOT(selected(int)));
	connect(popup, SIGNAL(hid()), SLOT(popupClosed()));
	popup->setMinimumSize(300, 200);

	const QRect desktop = QApplication::desktop()->geometry();

	QPoint pos = mapToGlobal(rect().bottomLeft());
	if (pos.x() < desktop.left())
		pos.setX(desktop.left());
	if (pos.y() < desktop.top())
		pos.setY(desktop.top());

	if ((pos.x() + popup->width()) > desktop.width())
		pos.setX(desktop.width() - popup->width());
	if ((pos.y() + popup->height()) > desktop.bottom())
		pos.setY(desktop.bottom() - popup->height());
	popup->move(pos);

	clearFocus();
	update();

	popup->setFocus();
	popup->show();
}

void lcQColorPicker::updateIcon()
{
	int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
	QPixmap pix(iconSize, iconSize);
	pix.fill(palette().button().color());

	QPainter p(&pix);

	lcColor* color = &gColorList[currentColorIndex];
	QRgb rgb = qRgb(color->Value[0] * 255, color->Value[1] * 255, color->Value[2] * 255);

	p.fillRect(0, 0, pix.width(), pix.height(), rgb);
	setIcon(QIcon(pix));
}

void lcQColorPicker::popupClosed()
{
	if (initialColorIndex != currentColorIndex)
		changed(initialColorIndex);

	setChecked(false);
	setFocus();
}

void lcQColorPicker::changed(int colorIndex)
{
	if (colorIndex == currentColorIndex)
		return;

	currentColorIndex = colorIndex;
	updateIcon();

	repaint();

	emit colorChanged(currentColorIndex);
}

void lcQColorPicker::selected(int colorIndex)
{
	initialColorIndex = colorIndex;
	changed(colorIndex);
}

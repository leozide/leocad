#include "lc_global.h"
#include "lc_qcolorpicker.h"
#include "lc_qcolorlist.h"
#include "lc_colors.h"

lcQColorPickerPopup::lcQColorPickerPopup(QWidget* Parent, int ColorIndex, bool AllowNoColor)
	: QFrame(Parent, Qt::Popup)
{
	setFrameStyle(QFrame::StyledPanel);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	QGridLayout *layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(layout);

	colorList = new lcQColorList(this, AllowNoColor);
	connect(colorList, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(colorList, SIGNAL(colorSelected(int)), this, SLOT(colorSelected(int)));
	layout->addWidget(colorList);

	colorList->blockSignals(true);
	colorList->setCurrentColor(ColorIndex);
	colorList->blockSignals(false);

	eventLoop = nullptr;
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
	eventLoop = nullptr;
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

lcQColorPicker::lcQColorPicker(QWidget* Parent, bool AllowNoColor)
	: QPushButton(Parent), mAllowNoColor(AllowNoColor)
{
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	setAutoDefault(false);
	setCheckable(true);

	UpdateIcon();

	connect(this, SIGNAL(toggled(bool)), SLOT(buttonPressed(bool)));
}

lcQColorPicker::~lcQColorPicker()
{
}

void lcQColorPicker::setCurrentColor(int colorIndex)
{
	selected(colorIndex);
}

void lcQColorPicker::setCurrentColorCode(int colorCode)
{
	setCurrentColor(lcGetColorIndex(colorCode));
}

int lcQColorPicker::currentColor() const
{
	return mCurrentColorIndex;
}

int lcQColorPicker::currentColorCode() const
{
	return gColorList[mCurrentColorIndex].Code;
}

void lcQColorPicker::buttonPressed(bool toggled)
{
	if (!toggled)
		return;

	lcQColorPickerPopup *popup = new lcQColorPickerPopup(this, mCurrentColorIndex, mAllowNoColor);
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

void lcQColorPicker::UpdateIcon()
{
	const int IconSize = 14;//style()->pixelMetric(QStyle::PM_SmallIconSize);
	QPixmap Pixmap(IconSize, IconSize);

	QPainter Painter(&Pixmap);

	Painter.setPen(Qt::darkGray);

	const lcColor* Color = &gColorList[mCurrentColorIndex];

	if (Color->Code != LC_COLOR_NOCOLOR)
	{
		Painter.setBrush(QColor::fromRgbF(Color->Value[0], Color->Value[1], Color->Value[2]));
		Painter.drawRect(0, 0, Pixmap.width() - 1, Pixmap.height() - 1);
	}
	else
		lcDrawNoColorRect(Painter, QRect(0, 0, Pixmap.width() - 1, Pixmap.height() - 1));

	Painter.end();

	setIcon(QIcon(Pixmap));
}

void lcQColorPicker::popupClosed()
{
	if (mInitialColorIndex != mCurrentColorIndex)
		changed(mInitialColorIndex);

	setChecked(false);
	setFocus();
}

void lcQColorPicker::changed(int colorIndex)
{
	if (colorIndex == mCurrentColorIndex)
		return;

	mCurrentColorIndex = colorIndex;
	UpdateIcon();

	repaint();

	emit colorChanged(mCurrentColorIndex);
}

void lcQColorPicker::selected(int colorIndex)
{
	mInitialColorIndex = colorIndex;
	changed(colorIndex);
}

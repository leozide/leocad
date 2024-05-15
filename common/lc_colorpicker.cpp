#include "lc_global.h"
#include "lc_colorpicker.h"
#include "lc_colorlist.h"
#include "lc_colors.h"

lcColorPickerPopup::lcColorPickerPopup(QWidget* Parent, int ColorIndex, bool AllowNoColor)
	: QFrame(Parent, Qt::Popup)
{
	setFrameStyle(QFrame::StyledPanel);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	QGridLayout *layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(layout);

	mColorList = new lcColorList(this, AllowNoColor);
	connect(mColorList, &lcColorList::ColorChanged, this, &lcColorPickerPopup::ColorChanged);
	connect(mColorList, &lcColorList::ColorSelected, this, &lcColorPickerPopup::ColorSelected);
	layout->addWidget(mColorList);

	mColorList->blockSignals(true);
	mColorList->SetCurrentColor(ColorIndex);
	mColorList->blockSignals(false);

	mEventLoop = nullptr;
}

lcColorPickerPopup::~lcColorPickerPopup()
{
	if (mEventLoop)
		mEventLoop->exit();
}

void lcColorPickerPopup::exec()
{
	show();

	QEventLoop EventLoop;
	mEventLoop = &EventLoop;
	(void) EventLoop.exec();
	mEventLoop = nullptr;
}

void lcColorPickerPopup::mouseReleaseEvent(QMouseEvent* MouseEvent)
{
	if (!rect().contains(MouseEvent->pos()))
		hide();
}

void lcColorPickerPopup::hideEvent(QHideEvent* HideEvent)
{
	if (mEventLoop)
		mEventLoop->exit();

	emit Hid();
	QFrame::hideEvent(HideEvent);
}

void lcColorPickerPopup::ColorChanged(int ColorIndex)
{
	emit Changed(ColorIndex);
}

void lcColorPickerPopup::ColorSelected(int ColorIndex)
{
	emit Selected(ColorIndex);
	hide();
}

void lcColorPickerPopup::showEvent(QShowEvent* ShowEvent)
{
	Q_UNUSED(ShowEvent);

	mColorList->setFocus();
}

lcColorPicker::lcColorPicker(QWidget* Parent, bool AllowNoColor)
	: QPushButton(Parent), mAllowNoColor(AllowNoColor)
{
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	setAutoDefault(false);
	setCheckable(true);

	UpdateIcon();

	connect(this, &QPushButton::toggled, this, &lcColorPicker::ButtonPressed);
}

lcColorPicker::~lcColorPicker()
{
}

void lcColorPicker::SetCurrentColor(int ColorIndex)
{
	Selected(ColorIndex);
}

void lcColorPicker::SetCurrentColorCode(int ColorCode)
{
	SetCurrentColor(lcGetColorIndex(ColorCode));
}

int lcColorPicker::GetCurrentColor() const
{
	return mCurrentColorIndex;
}

int lcColorPicker::GetCurrentColorCode() const
{
	return gColorList[mCurrentColorIndex].Code;
}

void lcColorPicker::ButtonPressed(bool Toggled)
{
	if (!Toggled)
		return;

	lcColorPickerPopup* Popup = new lcColorPickerPopup(this, mCurrentColorIndex, mAllowNoColor);
	connect(Popup, &lcColorPickerPopup::Changed, this, &lcColorPicker::Changed);
	connect(Popup, &lcColorPickerPopup::Selected, this, &lcColorPicker::Selected);
	connect(Popup, &lcColorPickerPopup::Hid, this, &lcColorPicker::PopupClosed);
	Popup->setMinimumSize(300, 200);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QScreen* Screen = screen();
	const QRect Desktop = Screen ? Screen->geometry() : QRect();
#else
	const QRect Desktop = QApplication::desktop()->geometry();
#endif

	QPoint Pos = mapToGlobal(rect().bottomLeft());
	if (Pos.x() < Desktop.left())
		Pos.setX(Desktop.left());
	if (Pos.y() < Desktop.top())
		Pos.setY(Desktop.top());

	if ((Pos.x() + Popup->width()) > Desktop.width())
		Pos.setX(Desktop.width() - Popup->width());
	if ((Pos.y() + Popup->height()) > Desktop.bottom())
		Pos.setY(Desktop.bottom() - Popup->height());
	Popup->move(Pos);

	clearFocus();
	update();

	Popup->setFocus();
	Popup->show();
}

void lcColorPicker::UpdateIcon()
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

void lcColorPicker::PopupClosed()
{
	if (mInitialColorIndex != mCurrentColorIndex)
		Changed(mInitialColorIndex);

	setChecked(false);
	setFocus();
}

void lcColorPicker::Changed(int ColorIndex)
{
	if (ColorIndex == mCurrentColorIndex)
		return;

	mCurrentColorIndex = ColorIndex;
	UpdateIcon();

	update();

	emit ColorChanged(mCurrentColorIndex);
}

void lcColorPicker::Selected(int ColorIndex)
{
	mInitialColorIndex = ColorIndex;
	Changed(ColorIndex);
}

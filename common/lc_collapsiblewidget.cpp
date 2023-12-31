#include "lc_global.h"
#include "lc_collapsiblewidget.h"

QImage lcCollapsibleWidgetButton::mExpandedIcon;
QImage lcCollapsibleWidgetButton::mCollapsedIcon;

lcCollapsibleWidgetButton::lcCollapsibleWidgetButton(const QString& Title, QWidget* Parent)
	: QToolButton(Parent)
{
	setText(Title);
	setAutoRaise(true);
	setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

	UpdateIcon();

	connect(this, &QToolButton::clicked, this, &lcCollapsibleWidgetButton::Clicked);
}

void lcCollapsibleWidgetButton::Collapse()
{
	if (mExpanded)
		Clicked();
}

void lcCollapsibleWidgetButton::UpdateIcon()
{
	if (mExpanded)
	{
		if (mExpandedIcon.isNull())
		{
			QImage Image(16, 16, QImage::Format::Format_ARGB32);
			Image.fill(QColor(0, 0, 0, 0));
			uint Color = palette().color(QPalette::Text).rgba();

			for (int y = 0; y < 8; y++)
				for (int x = y; x < 8 - y; x++)
					Image.setPixel(x + 4, y + 6, Color);

			mExpandedIcon = Image;
		}

		setIcon(QPixmap::fromImage(mExpandedIcon));
	}
	else
	{
		if (mCollapsedIcon.isNull())
		{
			QImage Image(16, 16, QImage::Format::Format_ARGB32);
			Image.fill(QColor(0, 0, 0, 0));
			uint Color = palette().color(QPalette::Text).rgba();

			for (int y = 0; y < 8; y++)
				for (int x = y; x < 8 - y; x++)
					Image.setPixel(y + 6, x + 4, Color);

			mCollapsedIcon = Image;
		}

		setIcon(QPixmap::fromImage(mCollapsedIcon));
	}
}

void lcCollapsibleWidgetButton::Clicked()
{
	mExpanded = !mExpanded;
	UpdateIcon();

	emit StateChanged(mExpanded);
}

lcCollapsibleWidget::lcCollapsibleWidget(const QString& Title, QWidget* Parent)
	: QWidget(Parent)
{
	QVBoxLayout* Layout = new QVBoxLayout(this);
//	Layout->setSpacing(0);
	Layout->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout* TitleLayout = new QHBoxLayout();
	TitleLayout->setContentsMargins(0, 0, 0, 0);
	Layout->addLayout(TitleLayout);

	mTitleButton = new lcCollapsibleWidgetButton(Title, this);
	TitleLayout->addWidget(mTitleButton);

	connect(mTitleButton, &lcCollapsibleWidgetButton::StateChanged, this, &lcCollapsibleWidget::ButtonStateChanged);

	mChildWidget = new QWidget(this);
	Layout->addWidget(mChildWidget);
}

void lcCollapsibleWidget::ButtonStateChanged(bool Expanded)
{
	mChildWidget->setVisible(Expanded);
}

void lcCollapsibleWidget::Collapse()
{
	mTitleButton->Collapse();
}

void lcCollapsibleWidget::SetChildLayout(QLayout* Layout)
{
	Layout->setContentsMargins(12, 0, 0, 0);
	mChildWidget->setLayout(Layout);
}

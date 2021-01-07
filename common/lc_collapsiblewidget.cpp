#include "lc_global.h"
#include "lc_collapsiblewidget.h"

QImage lcCollapsibleWidget::mExpandedIcon;
QImage lcCollapsibleWidget::mCollapsedIcon;

lcCollapsibleWidget::lcCollapsibleWidget(const QString& Title, QWidget* Parent)
	: QWidget(Parent)
{
	QVBoxLayout* Layout = new QVBoxLayout(this);
//	Layout->setSpacing(0);
	Layout->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout* TitleLayout = new QHBoxLayout();
	TitleLayout->setContentsMargins(0, 0, 0, 0);
	Layout->addLayout(TitleLayout);

	mTitleButton = new QToolButton(this);
	mTitleButton->setText(Title);
	mTitleButton->setAutoRaise(true);
	mTitleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	mTitleButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
	TitleLayout->addWidget(mTitleButton);

	connect(mTitleButton, SIGNAL(clicked()), this, SLOT(TitleClicked()));

	mChildWidget = new QWidget(this);
	Layout->addWidget(mChildWidget);

	UpdateIcon();
}

void lcCollapsibleWidget::TitleClicked()
{
	mExpanded = !mExpanded;
	mChildWidget->setVisible(mExpanded);
	UpdateIcon();
}

void lcCollapsibleWidget::Collapse()
{
	if (mExpanded)
		TitleClicked();
}

void lcCollapsibleWidget::SetChildLayout(QLayout* Layout)
{
	Layout->setContentsMargins(12, 0, 0, 0);
	mChildWidget->setLayout(Layout);
}

void lcCollapsibleWidget::UpdateIcon()
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

		mTitleButton->setIcon(QPixmap::fromImage(mExpandedIcon));
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

		mTitleButton->setIcon(QPixmap::fromImage(mCollapsedIcon));
	}
}

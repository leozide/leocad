#include "lc_global.h"
#include "lc_partselectionpopup.h"
#include "lc_partselectionwidget.h"
#include "pieceinf.h"

lcPartSelectionPopup::lcPartSelectionPopup(PieceInfo* InitialPart, QWidget* Parent)
	: QWidget(Parent), mInitialPart(InitialPart)
{
	QVBoxLayout* Layout = new QVBoxLayout(this);

	mPartSelectionWidget = new lcPartSelectionWidget(this);
	Layout->addWidget(mPartSelectionWidget);
	mPartSelectionWidget->SetOrientation(Qt::Horizontal);

	mPartSelectionWidget->SetDragEnabled(false);

	connect(mPartSelectionWidget, &lcPartSelectionWidget::PartPicked, this, &lcPartSelectionPopup::Accept);
	connect(mPartSelectionWidget, &lcPartSelectionWidget::LayoutChanged, this, &lcPartSelectionPopup::ResizeToContents, Qt::QueuedConnection);

	QDialogButtonBox* ButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	Layout->addWidget(ButtonBox);

	QObject::connect(ButtonBox, &QDialogButtonBox::accepted, this, &lcPartSelectionPopup::Accept);
	QObject::connect(ButtonBox, &QDialogButtonBox::rejected, this, &lcPartSelectionPopup::Reject);
}

QSize lcPartSelectionPopup::GetPartsViewSizeHint(const lcPartSelectionListView* PartsView, const QSize& PopupSize) const
{
	const int WidthOverhead = PopupSize.width() - PartsView->sizeHint().width();
	const int ViewWidth = qMax(1, qMin(PopupSize.width(), maximumWidth()) - WidthOverhead);
	return PartsView->GetPreferredSize(ViewWidth);
}

QSize lcPartSelectionPopup::sizeHint() const
{
	QSize Size = QWidget::sizeHint();
	const lcPartSelectionListView* PartsView = mPartSelectionWidget->findChild<lcPartSelectionListView*>();
	if (!PartsView || PartsView->GetListModel()->GetIconSize() == lcPartSelectionListView::NoIconSize || PartsView->GetListModel()->IsListMode())
		return Size;

	const int RowHeight = PartsView->GetCellSize().height();
	if (RowHeight <= 0)
		return Size;

	const int Overhead = Size.height() - PartsView->sizeHint().height() + 2 * PartsView->frameWidth();
	const int ViewHeight = GetPartsViewSizeHint(PartsView, Size).height();
	const int PreferredHeight = Overhead + ViewHeight - 2 * PartsView->frameWidth();
	const int AvailableHeight = qMin(PreferredHeight, maximumHeight()) - Overhead;
	const int Rows = qMax(1, AvailableHeight / RowHeight);
	Size.setHeight(Overhead + Rows * RowHeight);

	return Size;
}

void lcPartSelectionPopup::showEvent(QShowEvent* ShowEvent)
{
	QWidget::showEvent(ShowEvent);

	mPartSelectionWidget->SetOrientation(Qt::Horizontal);
	mPartSelectionWidget->SetCurrentPart(mInitialPart);

	mPartSelectionWidget->FocusPartFilterWidget();
}

void lcPartSelectionPopup::SetHeightLimit(int Height)
{
	mHeightLimit = Height;
	UpdateHeightLimit();
}

void lcPartSelectionPopup::UpdateHeightLimit()
{
	int Height = mHeightLimit;
	const lcPartSelectionListView* PartsView = mPartSelectionWidget->findChild<lcPartSelectionListView*>();
	if (PartsView && PartsView->GetListModel()->GetIconSize() && !PartsView->GetListModel()->IsListMode())
	{
		const int RowHeight = PartsView->GetCellSize().height();
		if (RowHeight > 0)
		{
			const QSize PopupSize = QWidget::sizeHint();
			const int ViewHeight = GetPartsViewSizeHint(PartsView, PopupSize).height();
			const int Overhead = PopupSize.height() - PartsView->sizeHint().height() + 2 * PartsView->frameWidth();
			const int Rows = qMin(2, (ViewHeight - 2 * PartsView->frameWidth()) / RowHeight);
			Height = qMax(Height, Overhead + Rows * RowHeight);
		}
	}

	setMaximumHeight(Height);
	QMenu* Menu = qobject_cast<QMenu*>(parent());
	if (Menu)
	{
		QStyleOption Option;
		Option.initFrom(Menu);
		QStyle* Style = Menu->style();
		const int Margin = Style->pixelMetric(QStyle::PM_MenuPanelWidth, &Option, Menu) + Style->pixelMetric(QStyle::PM_MenuVMargin, &Option, Menu);
		const QSize MenuSize = Style->sizeFromContents(QStyle::CT_Menu, &Option, QSize(0, Height + 2 * Margin), Menu);
		Menu->setMaximumHeight(qMin(QWIDGETSIZE_MAX, MenuSize.height()));
	}
}

void lcPartSelectionPopup::Accept()
{
	mPickedPiece = mPartSelectionWidget->GetCurrentPart();
	mAccepted = true;

	Close();
}

void lcPartSelectionPopup::Reject()
{
	Close();
}

void lcPartSelectionPopup::ResizeToContents()
{
	QMenu* Menu = qobject_cast<QMenu*>(parent());
	if (!Menu || !Menu->isVisible())
		return;

	updateGeometry();
	UpdateHeightLimit();

	// Refresh QMenu's cached widget action geometry as well as its own size.
	for (QAction* Action : Menu->actions())
	{
		QWidgetAction* WidgetAction = qobject_cast<QWidgetAction*>(Action);
		if (WidgetAction && WidgetAction->defaultWidget() == this)
		{
			QActionEvent Event(QEvent::ActionChanged, Action);
			QCoreApplication::sendEvent(Menu, &Event);
			break;
		}
	}

	for (QScreen* Screen : QGuiApplication::screens())
	{
		const QRect AvailableGeometry = Screen->availableGeometry();
		if (!Screen->geometry().contains(Menu->pos()))
			continue;

		const int X = qBound(AvailableGeometry.left(), Menu->x(), qMax(AvailableGeometry.left(), AvailableGeometry.right() - Menu->width() + 1));
		const int Y = qBound(AvailableGeometry.top(), Menu->y(), qMax(AvailableGeometry.top(), AvailableGeometry.bottom() - Menu->height() + 1));
		Menu->move(X, Y);
		break;
	}
}

void lcPartSelectionPopup::Close()
{
	QMenu* Menu = qobject_cast<QMenu*>(parent());

	if (Menu)
		Menu->close();
}

std::optional<PieceInfo*> lcShowPartSelectionPopup(PieceInfo* InitialPart, const std::vector<std::pair<PieceInfo*, std::string>>& CustomParts, int ColorIndex, QWidget* Parent, QPoint Position)
{
	std::unique_ptr<QMenu> Menu(new QMenu(Parent));
	QWidgetAction* Action = new QWidgetAction(Menu.get());
	lcPartSelectionPopup* Popup = new lcPartSelectionPopup(InitialPart, Menu.get());
	lcPartSelectionWidget* PartSelectionWidget = Popup->GetPartSelectionWidget();

	PartSelectionWidget->SetIsPopup(true);

	if (CustomParts.empty())
	{
		PartSelectionWidget->SetCategory(lcPartCategoryType::AllParts, 0);
		PartSelectionWidget->SetColorIndex(ColorIndex);
	}
	else
		PartSelectionWidget->SetCustomParts(CustomParts, ColorIndex);

	QScreen* PopupScreen = QGuiApplication::primaryScreen();
	for (QScreen* Screen : QGuiApplication::screens())
	{
		if (Screen->geometry().contains(Position))
		{
			PopupScreen = Screen;
			break;
		}
	}

	if (PopupScreen)
	{
		const QSize ScreenSize = PopupScreen->availableGeometry().size();
		const QSize MaximumSize(ScreenSize.width() * 3 / 5, ScreenSize.height() * 3 / 5);
		Popup->setMaximumWidth(MaximumSize.width());
		Menu->setMaximumWidth(MaximumSize.width());
		Popup->SetHeightLimit(MaximumSize.height());
	}

	Action->setDefaultWidget(Popup);
	Menu->addAction(Action);

	Menu->exec(Position);

	return Popup->GetPickedPart();
}

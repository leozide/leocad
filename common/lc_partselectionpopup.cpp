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

	mPartSelectionWidget->SetDragEnabled(false);

	connect(mPartSelectionWidget, &lcPartSelectionWidget::PartPicked, this, &lcPartSelectionPopup::Accept);

	QDialogButtonBox* ButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	Layout->addWidget(ButtonBox);

	QObject::connect(ButtonBox, &QDialogButtonBox::accepted, this, &lcPartSelectionPopup::Accept);
	QObject::connect(ButtonBox, &QDialogButtonBox::rejected, this, &lcPartSelectionPopup::Reject);
}

void lcPartSelectionPopup::showEvent(QShowEvent* ShowEvent)
{
	QWidget::showEvent(ShowEvent);

	mPartSelectionWidget->SetOrientation(Qt::Horizontal);
	mPartSelectionWidget->SetCurrentPart(mInitialPart);

	mPartSelectionWidget->FocusPartFilterWidget();
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

	if (CustomParts.empty())
	{
		PartSelectionWidget->SetCategory(lcPartCategoryType::AllParts, 0);
		PartSelectionWidget->SetColorIndex(ColorIndex);
	}
	else
		PartSelectionWidget->SetCustomParts(CustomParts, ColorIndex);

	Action->setDefaultWidget(Popup);
	Menu->addAction(Action);

	Menu->exec(Position);

	return Popup->GetPickedPart();
}

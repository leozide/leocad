#include "lc_global.h"
#include "lc_findreplacewidget.h"
#include "lc_colorpicker.h"
#include "lc_mainwindow.h"
#include "lc_partselectionpopup.h"
#include "lc_piecedata.h"
#include "piece.h"
#include "lc_model.h"
#include "lc_view.h"
#include "lc_qutils.h"

lcFindReplaceWidget::lcFindReplaceWidget(QWidget* Parent, lcView* View, bool Replace)
	: QWidget(Parent), mView(View)
{
	connect(qApp, &QApplication::focusChanged, this, &lcFindReplaceWidget::FocusChanged);

	setAutoFillBackground(true);
	QPalette Palette = palette();
	Palette.setColor(QPalette::Window, QApplication::palette().color(QPalette::Button));
	setPalette(Palette);

	QGridLayout* Layout = new QGridLayout(this);
	Layout->setContentsMargins(5, 5, 5, 5);

	Layout->addWidget(new QLabel(tr("Find:")), 0, 0);

	lcColorPicker* FindColorPicker = new lcColorPicker(this, true);
	FindColorPicker->setToolTip(tr("Search Color"));
	Layout->addWidget(FindColorPicker, 0, 1);

	mFindPartComboBox = new QComboBox(this);
	mFindPartComboBox->setToolTip(tr("Search Piece"));
	mFindPartComboBox->setEditable(true);
	mFindPartComboBox->setInsertPolicy(QComboBox::NoInsert);
	Layout->addWidget(mFindPartComboBox, 0, 2);

	QToolButton* FindNextButton = new QToolButton(this);
	FindNextButton->setAutoRaise(true);
	FindNextButton->setDefaultAction(CreateAction(gMainWindow->mActions[LC_EDIT_FIND_NEXT], lcFindReplaceAction::FindNext));
	Layout->addWidget(FindNextButton, 0, 3);

	QToolButton* FindAllButton = new QToolButton(this);
	FindAllButton->setAutoRaise(true);
	FindAllButton->setDefaultAction(CreateAction(gMainWindow->mActions[LC_EDIT_FIND_ALL], lcFindReplaceAction::FindAll));
	Layout->addWidget(FindAllButton, 0, 4);

	connect(FindColorPicker, &lcColorPicker::ColorChanged, this, &lcFindReplaceWidget::FindColorIndexChanged);
	connect(mFindPartComboBox->lineEdit(), &QLineEdit::returnPressed, FindNextButton->defaultAction(), &QAction::trigger);
	connect(mFindPartComboBox->lineEdit(), &QLineEdit::textEdited, this, &lcFindReplaceWidget::FindTextEdited);
	connect(mFindPartComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &lcFindReplaceWidget::FindActivated);

	lcColorPicker* ReplaceColorPicker = nullptr;

	if (Replace)
	{
		Layout->addWidget(new QLabel(tr("Replace:")), 1, 0);

		ReplaceColorPicker = new lcColorPicker(this, true);
		ReplaceColorPicker->setToolTip(tr("Replacement Color"));
		Layout->addWidget(ReplaceColorPicker, 1, 1);

		QPixmap Pixmap(1, 1);
		Pixmap.fill(QColor::fromRgba64(0, 0, 0, 0));

		mReplacePartButton = new lcElidableToolButton(this);
		mReplacePartButton->setToolTip(tr("Replacement Piece"));

		QSizePolicy PieceButtonSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		PieceButtonSizePolicy.setHorizontalStretch(2);
		PieceButtonSizePolicy.setVerticalStretch(0);
		mReplacePartButton->setSizePolicy(PieceButtonSizePolicy);

		mReplacePartButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mReplacePartButton->setIcon(Pixmap);

		Layout->addWidget(mReplacePartButton, 1, 2);

		QToolButton* ReplaceNextButton = new QToolButton(this);
		ReplaceNextButton->setAutoRaise(true);
		ReplaceNextButton->setDefaultAction(CreateAction(gMainWindow->mActions[LC_EDIT_REPLACE_NEXT], lcFindReplaceAction::ReplaceNext));
		Layout->addWidget(ReplaceNextButton, 1, 3);

		QToolButton* ReplaceAllButton = new QToolButton(this);
		ReplaceAllButton->setAutoRaise(true);
		ReplaceAllButton->setDefaultAction(CreateAction(gMainWindow->mActions[LC_EDIT_REPLACE_ALL], lcFindReplaceAction::ReplaceAll));
		Layout->addWidget(ReplaceAllButton, 1, 4);

		connect(ReplaceColorPicker, &lcColorPicker::ColorChanged, this, &lcFindReplaceWidget::ReplaceColorIndexChanged);
		connect(mReplacePartButton, &lcElidableToolButton::clicked, this, &lcFindReplaceWidget::ReplaceButtonClicked);

		ReplaceColorPicker->SetCurrentColor(lcGetColorIndex(LC_COLOR_NOCOLOR));
		mReplacePartButton->setText(tr("None"));
	}

	QToolButton* CloseButton = new QToolButton(this);
	CloseButton->setAutoRaise(true);
	CloseButton->setIcon(QIcon(":/stylesheet/close.svg"));
	CloseButton->setToolTip(tr("Close"));
	Layout->addWidget(CloseButton, 0, 5);

	connect(CloseButton, &QToolButton::clicked, this, &QObject::deleteLater);

	lcModel* Model = mView->GetActiveModel();
	lcPartsList PartsList;
	Model->GetPartsList(gDefaultColor, false, true, PartsList);

	for (const auto& PartIt : PartsList)
		mFindPartComboBox->addItem(QString::fromLatin1(PartIt.first->m_strDescription), QVariant::fromValue((void*)PartIt.first));
	mFindPartComboBox->model()->sort(0);

	lcPiece* Focus = dynamic_cast<lcPiece*>(Model->GetFocusObject());

	if (Focus)
	{
		lcFindReplaceParams& Params = mView->GetFindReplaceParams();

		Params.FindPieceData = Focus->mPieceData.Get();
		Params.FindColorIndex = Focus->GetColorIndex();

		FindColorPicker->SetCurrentColor(Params.FindColorIndex);
		mFindPartComboBox->setCurrentIndex(mFindPartComboBox->findData(QVariant::fromValue((void*)Params.FindPieceData)));

	}
	else
	{
		FindColorPicker->SetCurrentColor(lcGetColorIndex(LC_COLOR_NOCOLOR));
		mFindPartComboBox->setEditText(QString());
	}

	mFindPartComboBox->setFocus();
	adjustSize();
	show();
}

QAction* lcFindReplaceWidget::CreateAction(QAction* Source, lcFindReplaceAction Operation)
{
	QAction* Action = new QAction(Source->icon(), Source->text(), this);
	Action->setToolTip(Source->toolTip());
	Action->setStatusTip(Source->statusTip());
	Action->setWhatsThis(Source->whatsThis());

	connect(Action, &QAction::triggered, this, [this, Operation]()
	{
		mView->SetFocus(true);

		if (lcModel* Model = mView->GetActiveModel())
			Model->FindReplacePiece(mView->GetFindReplaceParams(), Operation);
	});

	return Action;
}

void lcFindReplaceWidget::FocusChanged(QWidget* Previous, QWidget* Focused)
{
	Q_UNUSED(Previous);

	if (Focused && (Focused == this || isAncestorOf(Focused)) && !mView->IsLastFocused())
		mView->SetFocus(true);
}

void lcFindReplaceWidget::FindColorIndexChanged(int ColorIndex)
{
	lcFindReplaceParams& Params = mView->GetFindReplaceParams();

	Params.FindColorIndex = ColorIndex;
}

void lcFindReplaceWidget::FindTextEdited(const QString& Text)
{
	lcFindReplaceParams& Params = mView->GetFindReplaceParams();

	Params.FindString = Text;
	Params.FindPieceData = nullptr;
}

void lcFindReplaceWidget::FindActivated(int Index)
{
	lcFindReplaceParams& Params = mView->GetFindReplaceParams();

	Params.FindString.clear();
	Params.FindPieceData = (lcPieceData*)mFindPartComboBox->itemData(Index).value<void*>();
}

void lcFindReplaceWidget::ReplaceColorIndexChanged(int ColorIndex)
{
	lcFindReplaceParams& Params = mView->GetFindReplaceParams();

	Params.ReplaceColorIndex = ColorIndex;
}

void lcFindReplaceWidget::ReplaceButtonClicked()
{
	lcFindReplaceParams& Params = mView->GetFindReplaceParams();

	std::optional<lcPieceDataRef> Result = lcShowPartSelectionPopup(Params.ReplacePieceData.Get(), std::vector<std::pair<lcPieceData*, std::string>>(),
                gDefaultColor, mReplacePartButton, mReplacePartButton->mapToGlobal(mReplacePartButton->rect().bottomLeft()));

	if (!Result.has_value())
		return;

	Params.ReplacePieceData = Result.value();
	mReplacePartButton->setText(Result.value()->m_strDescription);
}

#include "lc_global.h"
#include "lc_findreplacewidget.h"
#include "lc_colorpicker.h"
#include "lc_mainwindow.h"
#include "lc_partselectionpopup.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_model.h"
#include "lc_view.h"
#include "lc_qutils.h"

lcFindReplaceWidget::lcFindReplaceWidget(QWidget* Parent, lcModel* Model, bool Replace)
	: QWidget(Parent)
{
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
	mFindPartComboBox->setToolTip(tr("Search Part"));
	mFindPartComboBox->setEditable(true);
	mFindPartComboBox->setInsertPolicy(QComboBox::NoInsert);
	Layout->addWidget(mFindPartComboBox, 0, 2);

	QToolButton* FindNextButton = new QToolButton(this);
	FindNextButton->setAutoRaise(true);
	FindNextButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_FIND_NEXT]);
	Layout->addWidget(FindNextButton, 0, 3);

	QToolButton* FindAllButton = new QToolButton(this);
	FindAllButton->setAutoRaise(true);
	FindAllButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_FIND_ALL]);
	Layout->addWidget(FindAllButton, 0, 4);

	connect(FindColorPicker, &lcColorPicker::ColorChanged, this, &lcFindReplaceWidget::FindColorIndexChanged);
	connect(mFindPartComboBox->lineEdit(), &QLineEdit::returnPressed, gMainWindow->mActions[LC_EDIT_FIND_NEXT], &QAction::trigger);
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
		mReplacePartButton->setToolTip(tr("Replacement Part"));

		QSizePolicy PieceButtonSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		PieceButtonSizePolicy.setHorizontalStretch(2);
		PieceButtonSizePolicy.setVerticalStretch(0);
		mReplacePartButton->setSizePolicy(PieceButtonSizePolicy);

		mReplacePartButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mReplacePartButton->setIcon(Pixmap);

		Layout->addWidget(mReplacePartButton, 1, 2);

		QToolButton* ReplaceNextButton = new QToolButton(this);
		ReplaceNextButton->setAutoRaise(true);
		ReplaceNextButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_REPLACE_NEXT]);
		Layout->addWidget(ReplaceNextButton, 1, 3);

		QToolButton* ReplaceAllButton = new QToolButton(this);
		ReplaceAllButton->setAutoRaise(true);
		ReplaceAllButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_REPLACE_ALL]);
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

	lcPartsList PartsList;
	Model->GetPartsList(gDefaultColor, false, true, PartsList);

	for (const auto& PartIt : PartsList)
		mFindPartComboBox->addItem(QString::fromLatin1(PartIt.first->m_strDescription), QVariant::fromValue((void*)PartIt.first));
	mFindPartComboBox->model()->sort(0);

	lcPiece* Focus = dynamic_cast<lcPiece*>(Model->GetFocusObject());

	if (Focus)
	{
		lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

		Params.FindInfo = Focus->mPieceInfo;
		Params.FindColorIndex = Focus->GetColorIndex();

		FindColorPicker->SetCurrentColor(Params.FindColorIndex);
		mFindPartComboBox->setCurrentIndex(mFindPartComboBox->findData(QVariant::fromValue((void*)Params.FindInfo)));

	}
	else
	{
		FindColorPicker->SetCurrentColor(lcGetColorIndex(LC_COLOR_NOCOLOR));
		mFindPartComboBox->setEditText(QString());
	}

	mFindPartComboBox->setFocus();
	adjustSize();
	move(1, 1);
	show();
}

void lcFindReplaceWidget::FindColorIndexChanged(int ColorIndex)
{
	lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	Params.FindColorIndex = ColorIndex;
}

void lcFindReplaceWidget::FindTextEdited(const QString& Text)
{
	lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	Params.FindString = Text;
	Params.FindInfo = nullptr;
}

void lcFindReplaceWidget::FindActivated(int Index)
{
	lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	Params.FindString.clear();
	Params.FindInfo = (PieceInfo*)mFindPartComboBox->itemData(Index).value<void*>();
}

void lcFindReplaceWidget::ReplaceColorIndexChanged(int ColorIndex)
{
	lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	Params.ReplaceColorIndex = ColorIndex;
}

void lcFindReplaceWidget::ReplaceButtonClicked()
{
	lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	std::optional<PieceInfo*> Result = lcShowPartSelectionPopup(Params.ReplacePieceInfo, std::vector<std::pair<PieceInfo*, std::string>>(),
                gDefaultColor, mReplacePartButton, mReplacePartButton->mapToGlobal(mReplacePartButton->rect().bottomLeft()));

	if (!Result.has_value())
		return;

	Params.ReplacePieceInfo = Result.value();
	mReplacePartButton->setText(Result.value()->m_strDescription);
}

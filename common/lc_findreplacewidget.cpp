#include "lc_global.h"
#include "lc_findreplacewidget.h"
#include "lc_qcolorpicker.h"
#include "lc_library.h"
#include "lc_mainwindow.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_model.h"

lcFindReplaceWidget::lcFindReplaceWidget(QWidget* Parent, lcModel* Model, bool Replace)
	: QWidget(Parent)
{
	setAutoFillBackground(true);

	QGridLayout* Layout = new QGridLayout(this);
	Layout->setContentsMargins(5, 5, 5, 5);

	QCheckBox* FindColorCheckBox = new QCheckBox(tr("Find Color"), this);
	Layout->addWidget(FindColorCheckBox, 0, 1);

	lcQColorPicker* FindColorPicker = new lcQColorPicker(this);
	Layout->addWidget(FindColorPicker, 0, 2);

	QCheckBox* FindPartCheckBox = new QCheckBox(tr("Find Part"), this);
	Layout->addWidget(FindPartCheckBox, 0, 3);

	QComboBox* FindPartComboBox = new QComboBox(this);
	Layout->addWidget(FindPartComboBox, 0, 4);

	QToolButton* FindNextButton = new QToolButton(this);
	FindNextButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_FIND_NEXT]);
	Layout->addWidget(FindNextButton, 0, 5);

	connect(FindColorCheckBox, &QCheckBox::toggled, [](bool Checked)
	{
		gMainWindow->mSearchOptions.MatchColor = Checked;
	});

	connect(FindColorPicker, &lcQColorPicker::colorChanged, [](int ColorIndex)
	{
		gMainWindow->mSearchOptions.ColorIndex = ColorIndex;
	});

	connect(FindPartCheckBox, &QCheckBox::toggled, [](bool Checked)
	{
		gMainWindow->mSearchOptions.MatchInfo = Checked;
	});

	connect(FindPartComboBox, qOverload<int>(&QComboBox::currentIndexChanged), [FindPartComboBox](int Index)
	{
		gMainWindow->mSearchOptions.Info = (PieceInfo*)FindPartComboBox->itemData(Index).value<void*>();
	});

	QCheckBox* ReplaceColorCheckBox = nullptr;
	lcQColorPicker* ReplaceColorPicker = nullptr;
	QCheckBox* ReplacePartCheckBox = nullptr;
	QComboBox* ReplacePartComboBox = nullptr;

	if (Replace)
	{
		ReplaceColorCheckBox = new QCheckBox(tr("Replace Color"), this);
		Layout->addWidget(ReplaceColorCheckBox, 1, 1);

		ReplaceColorPicker = new lcQColorPicker(this);
		Layout->addWidget(ReplaceColorPicker, 1, 2);

		ReplacePartCheckBox = new QCheckBox(tr("Replace Part"), this);
		Layout->addWidget(ReplacePartCheckBox, 1, 3);

		ReplacePartComboBox = new QComboBox(this);
		Layout->addWidget(ReplacePartComboBox, 1, 4);

		QToolButton* ReplaceNextButton = new QToolButton(this);
		ReplaceNextButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_REPLACE_NEXT]);
		Layout->addWidget(ReplaceNextButton, 1, 5);

		connect(ReplaceColorCheckBox, &QCheckBox::toggled, [](bool Checked)
		{
			gMainWindow->mSearchOptions.ReplaceColor = Checked;
		});

		connect(ReplaceColorPicker, &lcQColorPicker::colorChanged, [](int ColorIndex)
		{
			gMainWindow->mSearchOptions.ReplaceColorIndex = ColorIndex;
		});

		connect(ReplacePartCheckBox, &QCheckBox::toggled, [](bool Checked)
		{
			gMainWindow->mSearchOptions.ReplaceInfo = Checked;
		});

		connect(ReplacePartComboBox, qOverload<int>(&QComboBox::currentIndexChanged), [ReplacePartComboBox](int Index)
		{
			gMainWindow->mSearchOptions.ReplacePieceInfo = (PieceInfo*)ReplacePartComboBox->itemData(Index).value<void*>();
		});

		ReplacePartComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		ReplacePartComboBox->setMinimumContentsLength(1);

		lcPiecesLibrary* Library = lcGetPiecesLibrary();
		std::vector<PieceInfo*> SortedPieces;
		SortedPieces.reserve(Library->mPieces.size());

		for (const auto& PartIt : Library->mPieces)
			SortedPieces.push_back(PartIt.second);

		auto PieceCompare = [](PieceInfo* Info1, PieceInfo* Info2)
		{
			return strcmp(Info1->m_strDescription, Info2->m_strDescription) < 0;
		};

		std::sort(SortedPieces.begin(), SortedPieces.end(), PieceCompare);

		for (PieceInfo* Info : SortedPieces)
			ReplacePartComboBox->addItem(Info->m_strDescription, QVariant::fromValue((void*)Info));
	}

	lcPartsList PartsList;
	Model->GetPartsList(gDefaultColor, false, true, PartsList);

	for (const auto& PartIt : PartsList)
		FindPartComboBox->addItem(PartIt.first->m_strDescription, QVariant::fromValue((void*)PartIt.first));
	FindPartComboBox->model()->sort(0);

	lcPiece* Focus = dynamic_cast<lcPiece*>(Model->GetFocusObject());

	lcSearchOptions& SearchOptions = gMainWindow->mSearchOptions;

	SearchOptions.SearchValid = true;

	if (!Replace)
	{
		SearchOptions.ReplaceColor = false;
		SearchOptions.ReplaceInfo = false;
	}

	if (Focus)
	{
		SearchOptions.Info = Focus->mPieceInfo;
		SearchOptions.ColorIndex = Focus->GetColorIndex();

		FindColorCheckBox->setChecked(true);
		FindColorPicker->setCurrentColor(SearchOptions.ColorIndex);
		FindPartCheckBox->setChecked(true);
		FindPartComboBox->setCurrentIndex(FindPartComboBox->findData(QVariant::fromValue((void*)SearchOptions.Info)));

		if (Replace)
		{
			ReplaceColorCheckBox->setChecked(true);
			ReplaceColorPicker->setCurrentColor(SearchOptions.ColorIndex);
			ReplacePartCheckBox->setChecked(true);
			ReplacePartComboBox->setCurrentIndex(ReplacePartComboBox->findData(QVariant::fromValue((void*)SearchOptions.Info)));
		}
	}

	adjustSize();
	move(1, 1);
	show();
}

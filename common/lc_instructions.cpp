#include "lc_global.h"
#include "lc_instructions.h"
#include "project.h"
#include "lc_model.h"
#include "piece.h"
#include "pieceinf.h"

const float lcInstructions::mDisplayDPI = 72.0f;

lcInstructions::lcInstructions(Project* Project)
	: mProject(Project)
{
	mPageSetup.Width = 8.5f * mDisplayDPI;
	mPageSetup.Height = 11.0f * mDisplayDPI;
	mPageSetup.MarginLeft = 0.5 * mDisplayDPI;
	mPageSetup.MarginRight = 0.5 * mDisplayDPI;
	mPageSetup.MarginTop = 0.5 * mDisplayDPI;
	mPageSetup.MarginBottom = 0.5 * mDisplayDPI;

	mPageSettings.Rows = 1;
	mPageSettings.Columns = 1;
	mPageSettings.Direction = lcInstructionsDirection::Horizontal;

	CreatePages();
}

void lcInstructions::SetDefaultPageSettings(const lcInstructionsPageSettings& PageSettings)
{
	mPageSettings = PageSettings;

	CreatePages();
}

void lcInstructions::CreatePages()
{
	mPages.clear();

	if (mProject)
	{
		std::vector<const lcModel*> AddedModels;

		lcModel* Model = mProject->GetMainModel();

		if (Model)
			AddDefaultPages(Model, AddedModels);
	}
}

void lcInstructions::AddDefaultPages(lcModel* Model, std::vector<const lcModel*>& AddedModels)
{
	if (std::find(AddedModels.begin(), AddedModels.end(), Model) != AddedModels.end())
		return;

	AddedModels.push_back(Model);

	const lcStep LastStep = Model->GetLastStep();
	lcInstructionsPage Page;
	int Row = 0, Column = 0;

	for (lcStep Step = 1; Step <= LastStep; Step++)
	{
		std::set<lcModel*> StepSubModels;

		for (lcPiece* Piece : Model->GetPieces())
		{
			if (!Piece->IsHidden() && Piece->GetStepShow() == Step && Piece->mPieceInfo->IsModel())
			{
				lcModel* SubModel = Piece->mPieceInfo->GetModel();

				if (std::find(AddedModels.begin(), AddedModels.end(), SubModel) != AddedModels.end())
					StepSubModels.insert(SubModel);
			}
		}

		if (!StepSubModels.empty())
		{
			if (!Page.Steps.empty())
			{
				mPages.emplace_back(std::move(Page));
				Row = 0;
				Column = 0;
			}

			for (lcModel* SubModel : StepSubModels)
				AddDefaultPages(SubModel, AddedModels);
		}

		lcInstructionsStep InstructionsStep;

		InstructionsStep.Model = Model;
		InstructionsStep.Step = Step;

		const double Width = 1.0 / (double)mPageSettings.Columns;
		const double Height = 1.0 / (double)mPageSettings.Rows;
		InstructionsStep.Rect = QRectF(Column * Width, Row * Height, Width, Height);

		Page.Steps.push_back(std::move(InstructionsStep));

		if (mPageSettings.Direction == lcInstructionsDirection::Horizontal)
		{
			Column++;

			if (Column == mPageSettings.Columns)
			{
				Row++;
				Column = 0;
			}

			if (Row == mPageSettings.Rows)
			{
				mPages.emplace_back(std::move(Page));
				Row = 0;
				Column = 0;
			}
		}
		else
		{
			Row++;

			if (Row == mPageSettings.Rows)
			{
				Row = 0;
				Column++;
			}

			if (Column == mPageSettings.Columns)
			{
				mPages.emplace_back(std::move(Page));
				Row = 0;
				Column = 0;
			}
		}
	}
}

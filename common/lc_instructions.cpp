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

	mStepProperties[static_cast<int>(lcInstructionsPropertyType::ShowStepNumber)].Value = true;
	mStepProperties[static_cast<int>(lcInstructionsPropertyType::ShowStepPLI)].Value = true;
	mStepProperties[static_cast<int>(lcInstructionsPropertyType::StepNumberFont)].Value = QFont("Arial", 72).toString();
	mStepProperties[static_cast<int>(lcInstructionsPropertyType::StepNumberColor)].Value = LC_RGBA(0, 0, 0, 255);
	mStepProperties[static_cast<int>(lcInstructionsPropertyType::StepBackgroundColor)].Value = LC_RGBA(255, 255, 255, 0);
	mStepProperties[static_cast<int>(lcInstructionsPropertyType::PLIBackgroundColor)].Value = LC_RGBA(255, 255, 255, 255);
	mStepProperties[static_cast<int>(lcInstructionsPropertyType::PLIFont)].Value = QFont("Arial", 16, QFont::Bold).toString();
	mStepProperties[static_cast<int>(lcInstructionsPropertyType::PLITextColor)].Value = LC_RGBA(0, 0, 0, 255);
	mStepProperties[static_cast<int>(lcInstructionsPropertyType::PLIBorderColor)].Value = LC_RGBA(0, 0, 0, 255);
//	mStepProperties[static_cast<int>(lcInstructionsPropertyType::PLIBorderWidth)].Value = 2.0f;
//	mStepProperties[static_cast<int>(lcInstructionsPropertyType::PLIBorderRound)].Value = true;

	static_assert(static_cast<int>(lcInstructionsPropertyType::Count) == 9, "Missing default property");

	CreatePages();
}

QString lcInstructions::GetPropertyLabel(lcInstructionsPropertyType Type)
{
	switch (Type)
	{
		case lcInstructionsPropertyType::ShowStepNumber:
			return tr("Show Step Number");
		case lcInstructionsPropertyType::ShowStepPLI:
			return tr("Show Parts List");
		case lcInstructionsPropertyType::StepNumberFont:
			return tr("Font:");
		case lcInstructionsPropertyType::StepNumberColor:
			return tr("Text Color:");
		case lcInstructionsPropertyType::StepBackgroundColor:
			return tr("Background Color:");
		case lcInstructionsPropertyType::PLIBackgroundColor:
			return tr("Background Color:");
		case lcInstructionsPropertyType::PLIFont:
			return tr("Font:");
		case lcInstructionsPropertyType::PLITextColor:
			return tr("Text Color:");
		case lcInstructionsPropertyType::PLIBorderColor:
			return tr("Border Color:");
		case lcInstructionsPropertyType::Count:
			break;
	}

	return QString();
}

void lcInstructions::SetDefaultPageSettings(const lcInstructionsPageSettings& PageSettings)
{
	mPageSettings = PageSettings;

	CreatePages();
}

bool lcInstructions::GetBoolProperty(lcInstructionsPropertyType Type, lcModel* Model, lcStep Step) const
{
	QVariant Value = GetProperty(Type, Model, Step);
	return Value.toBool();
}

QColor lcInstructions::GetColorProperty(lcInstructionsPropertyType Type, lcModel* Model, lcStep Step) const
{
	QVariant Value = GetProperty(Type, Model, Step);
	return lcRGBAFromQColor(Value.toUInt());
}

QFont lcInstructions::GetFontProperty(lcInstructionsPropertyType Type, lcModel* Model, lcStep Step) const
{
	QVariant Value = GetProperty(Type, Model, Step);
	QFont Font;
	Font.fromString(Value.toString());
	return Font;
}

QVariant lcInstructions::GetProperty(lcInstructionsPropertyType Type, lcModel* Model, lcStep Step) const
{
	QVariant Value = mStepProperties[static_cast<int>(Type)].Value;

	std::map<lcModel*, lcInstructionsModel>::const_iterator ModelIt = mModels.find(Model);

	if (ModelIt == mModels.end())
		return Value;

	const lcInstructionsModel& InstructionModel = ModelIt->second;

	for (lcStep StepIndex = 0; StepIndex <= Step; StepIndex++)
	{
		const lcInstructionsProperties& Properties = InstructionModel.StepProperties[StepIndex];
		const lcInstructionsProperty& Property = Properties[static_cast<int>(Type)];

		if (Property.Mode == lcInstructionsPropertyMode::NotSet || (Property.Mode == lcInstructionsPropertyMode::StepOnly && StepIndex != Step))
			continue;

		Value = Property.Value;
	}

	return Value;
}

void lcInstructions::SetDefaultBool(lcInstructionsPropertyType Type, bool Enabled)
{
	SetDefaultProperty(Type, Enabled);
}

void lcInstructions::SetDefaultColor(lcInstructionsPropertyType Type, const QColor& Color)
{
	SetDefaultProperty(Type, lcRGBAFromQColor(Color));
}

void lcInstructions::SetDefaultFont(lcInstructionsPropertyType Type, const QFont& Font)
{
	SetDefaultProperty(Type, Font.toString());
}

void lcInstructions::SetDefaultProperty(lcInstructionsPropertyType Type, const QVariant& Value)
{
	lcInstructionsProperty& Property = mStepProperties[static_cast<int>(Type)];

	if (Property.Value == Value)
		return;

	Property.Value = Value;

	emit StepSettingsChanged(nullptr, 0);
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

	lcInstructionsModel InstructionModel;
	InstructionModel.StepProperties.resize(LastStep + 1);
	mModels.emplace(Model, std::move(InstructionModel));

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

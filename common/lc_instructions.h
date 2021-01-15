#pragma once

#include "lc_math.h"

struct lcInstructionsPageSetup
{
	float Width;
	float Height;
	float MarginLeft;
	float MarginRight;
	float MarginTop;
	float MarginBottom;
};

enum class lcInstructionsDirection
{
	Horizontal,
	Vertical
};

struct lcInstructionsPageSettings
{
	int Rows;
	int Columns;
	lcInstructionsDirection Direction;
};

enum class lcInstructionsPropertyMode
{
	NotSet,
	Set,
	StepOnly
};

struct lcInstructionsStepProperties
{
	lcInstructionsPropertyMode BackgroundColorMode = lcInstructionsPropertyMode::NotSet;
	quint32 BackgroundColor = LC_RGBA(255, 255, 255, 0);
};

struct lcInstructionsStep
{
	QRectF Rect;
	lcModel* Model;
	lcStep Step;

	lcInstructionsStepProperties Properties;
};

struct lcInstructionsPage
{
//	lcInstructionsPageSettings Settings;
	std::vector<lcInstructionsStep> Steps;
};

struct lcInstructionsModel
{
	std::vector<lcInstructionsStepProperties> StepProperties;
};

class lcInstructions : public QObject
{
	Q_OBJECT

public:
	lcInstructions(Project* Project = nullptr);

	lcInstructionsStepProperties GetStepProperties(lcModel* Model, lcStep Step) const;
	void SetDefaultPageSettings(const lcInstructionsPageSettings& PageSettings);
	void SetDefaultStepBackgroundColor(quint32 Color);

	std::vector<lcInstructionsPage> mPages;
	lcInstructionsPageSettings mPageSettings;
	lcInstructionsPageSetup mPageSetup;
	lcInstructionsStepProperties mStepProperties;

	std::map<lcModel*, lcInstructionsModel> mModels;

signals:
	void PageInvalid(int PageIndex);

protected:
	void CreatePages();
	void AddDefaultPages(lcModel* Model, std::vector<const lcModel*>& AddedModels);

	Project* mProject = nullptr;

	static const float mDisplayDPI;
};

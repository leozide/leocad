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
	Default,
	Model,
	StepForward,
	StepOnly
};

enum class lcInstructionsPropertyType
{
	StepNumberFont,
	StepNumberColor,
	StepBackgroundColor,
	Count
};

struct lcInstructionsProperty
{
	lcInstructionsPropertyMode Mode = lcInstructionsPropertyMode::NotSet;
	QVariant Value;
};

using lcInstructionsProperties = std::array<lcInstructionsProperty, static_cast<int>(lcInstructionsPropertyType::Count)>;

struct lcInstructionsStep
{
	QRectF Rect;
	lcModel* Model;
	lcStep Step;

	lcInstructionsProperties Properties;
};

struct lcInstructionsPage
{
//	lcInstructionsPageSettings Settings;
	std::vector<lcInstructionsStep> Steps;
};

struct lcInstructionsModel
{
	std::vector<lcInstructionsProperties> StepProperties;
};

class lcInstructions : public QObject
{
	Q_OBJECT

public:
	lcInstructions(Project* Project = nullptr);

	void SetDefaultPageSettings(const lcInstructionsPageSettings& PageSettings);

	QColor GetColorProperty(lcInstructionsPropertyType Type, lcModel* Model, lcStep Step) const;
	QFont GetFontProperty(lcInstructionsPropertyType Type, lcModel* Model, lcStep Step) const;

	void SetDefaultColor(lcInstructionsPropertyType Type, const QColor& Color);
	void SetDefaultFont(lcInstructionsPropertyType Type, const QFont& Font);

	std::vector<lcInstructionsPage> mPages;
	lcInstructionsPageSettings mPageSettings;
	lcInstructionsPageSetup mPageSetup;
	lcInstructionsProperties mStepProperties;

	std::map<lcModel*, lcInstructionsModel> mModels;

signals:
	void StepSettingsChanged(lcModel* Model, lcStep Step);

protected:
	void CreatePages();
	void AddDefaultPages(lcModel* Model, std::vector<const lcModel*>& AddedModels);

	QVariant GetProperty(lcInstructionsPropertyType Type, lcModel* Model, lcStep Step) const;
	void SetDefaultProperty(lcInstructionsPropertyType Type, const QVariant& Value);

	Project* mProject = nullptr;

	static const float mDisplayDPI;
};

#pragma once

#include "lc_array.h"
#include "lc_objectproperty.h"

class lcCollapsibleWidgetButton;

class lcPropertiesWidget : public QWidget
{
	Q_OBJECT;

public:
	lcPropertiesWidget(QWidget* Parent);

	void Update(const lcArray<lcObject*>& Selection, lcObject* Focus);

protected slots:
	void CategoryStateChanged(bool Expanded);
	void BoolChanged();
	void FloatChanged();
	void IntegerChanged();
	void StepNumberChanged();
	void StringChanged();
	void StringListChanged(int Index);
	void ColorButtonClicked();
	void PieceColorButtonClicked();
	void PieceColorChanged(int ColorIndex);
	void PieceIdButtonClicked();
	void PieceIdChanged(PieceInfo* Info);

protected:
	enum class CategoryIndex
	{
		Piece,
		Camera,
		CameraTransform,
		Light,
		ObjectTransform,
		Count
	};

	enum class PropertyType
	{
		Bool,
		Float,
		Integer,
		StepNumber,
		String,
		StringList,
		Color,
		PieceColor,
		PieceId
	};

	enum class LayoutMode
	{
		Empty,
		Piece,
		Camera,
		Light,
		Multiple,
		Count
	};

	struct CategoryWidgets
	{
		lcCollapsibleWidgetButton* Button = nullptr;
		std::vector<lcObjectPropertyId> Properties;
		std::vector<int> SpacingRows;
	};

	struct PropertyWidgets
	{
		QLabel* Label = nullptr;
		QWidget* Widget = nullptr;
	};

	lcObjectPropertyId GetWidgetPropertyId(QWidget* Widget) const;

	void AddCategory(CategoryIndex Index, const QString& Title);
	void AddSpacing();
	void AddLabel(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip);
	void AddBoolProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip);
	void AddFloatProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, float Min, float Max);
	void AddIntegerProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, int Min, int Max);
	void AddStepNumberProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip);
	void AddStringProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip);
	void AddStringListProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, const QStringList& Strings);
	void AddColorProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip);
	void AddPieceColorProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip);
	void AddPieceIdProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip);

	void UpdateBool(lcObjectPropertyId PropertyId, bool Value);
	void UpdateFloat(lcObjectPropertyId PropertyId, float Value);
	void UpdateInteger(lcObjectPropertyId PropertyId, int Value);
	void UpdateStepNumber(lcObjectPropertyId PropertyId, lcStep Step, lcStep Min, lcStep Max);
	void UpdateString(lcObjectPropertyId PropertyId, const QString& Text);
	void UpdateStringList(lcObjectPropertyId PropertyId, int ListIndex);
	void UpdateColor(lcObjectPropertyId PropertyId, QColor Color);
	void UpdatePieceColor(lcObjectPropertyId PropertyId, int ColorIndex);
	void UpdatePieceId(lcObjectPropertyId PropertyId, const QString& Name);

	void SetEmpty();
	void SetPiece(const lcArray<lcObject*>& Selection, lcObject* Focus);
	void SetCamera(lcObject* Focus);
	void SetLight(lcObject* Focus);

	void CreateWidgets();
	void SetLayoutMode(LayoutMode Mode);
	void SetPropertyVisible(lcObjectPropertyId PropertyId, bool Visible);
	void SetCategoryVisible(CategoryIndex Index, bool Visible);
	void SetCategoryWidgetsVisible(CategoryWidgets& Category, bool Visible);

	lcObject* mFocusObject = nullptr;

	std::array<PropertyWidgets, static_cast<int>(lcObjectPropertyId::Count)> mPropertyWidgets = {};
	std::array<CategoryWidgets, static_cast<int>(CategoryIndex::Count)> mCategoryWidgets = {};

	CategoryWidgets* mCurrentCategory = nullptr;
	QGridLayout* mLayout = nullptr;
	int mLayoutRow = 0;

	LayoutMode mLayoutMode = LayoutMode::Count;
};

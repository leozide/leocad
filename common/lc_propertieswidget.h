#pragma once

#include "lc_objectproperty.h"

class lcCollapsibleWidgetButton;
class lcKeyFrameWidget;

class lcPropertiesWidget : public QWidget
{
	Q_OBJECT;

public:
	lcPropertiesWidget(QWidget* Parent);

	void Update(const std::vector<lcObject*>& Selection, lcObject* Focus);

protected slots:
	void CategoryStateChanged(bool Expanded);
	void KeyFrameChanged();
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
		LightBlender,
		LightPOVRay,
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
		CategoryIndex Category;
	};

	struct PropertyWidgets
	{
		QLabel* Label = nullptr;
		QWidget* Editor = nullptr;
		lcKeyFrameWidget* KeyFrame = nullptr;
		CategoryIndex Category;
		bool Visible = true;
	};

	lcObjectPropertyId GetEditorWidgetPropertyId(QWidget* Widget) const;
	lcObjectPropertyId GetKeyFrameWidgetPropertyId(QWidget* Widget) const;

	void AddCategory(CategoryIndex Index, const QString& Title);
	void AddSpacing();
	void AddLabel(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip);
	void AddKeyFrameWidget(lcObjectPropertyId PropertyId);
	void AddBoolProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, bool SupportsKeyFrames);
	void AddFloatProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, bool SupportsKeyFrames, float Min, float Max);
	void AddIntegerProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, bool SupportsKeyFrames, int Min, int Max);
	void AddStepNumberProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, bool SupportsKeyFrames);
	void AddStringProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, bool SupportsKeyFrames);
	void AddStringListProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, bool SupportsKeyFrames, const QStringList& Strings);
	void AddColorProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, bool SupportsKeyFrames);
	void AddPieceColorProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, bool SupportsKeyFrames);
	void AddPieceIdProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, bool SupportsKeyFrames);

	std::pair<QVariant, bool> GetUpdateValue(lcObjectPropertyId PropertyId);

	void UpdateKeyFrameWidget(lcObjectPropertyId PropertyId);
	void UpdateBool(lcObjectPropertyId PropertyId);
	void UpdateFloat(lcObjectPropertyId PropertyId, float Value);
	void UpdateInteger(lcObjectPropertyId PropertyId);
	void UpdateStepNumber(lcObjectPropertyId PropertyId, lcStep Step, lcStep Min, lcStep Max);
	void UpdateString(lcObjectPropertyId PropertyId);
	void UpdateStringList(lcObjectPropertyId PropertyId);
	void UpdateColor(lcObjectPropertyId PropertyId);
	void UpdatePieceColor(lcObjectPropertyId PropertyId);
	void UpdatePieceId(lcObjectPropertyId PropertyId);

	void SetEmpty();
	void SetPiece(const std::vector<lcObject*>& Selection, lcObject* Focus);
	void SetCamera(const std::vector<lcObject*>& Selection, lcObject* Focus);
	void SetLight(const std::vector<lcObject*>& Selection, lcObject* Focus);

	void CreateWidgets();
	void SetLayoutMode(LayoutMode Mode);
	void SetPropertyVisible(lcObjectPropertyId PropertyId, bool Visible);
	void SetPropertyWidgetsVisible(lcObjectPropertyId PropertyId, bool Visible);
	void SetCategoryVisible(CategoryIndex Index, bool Visible);
	void SetCategoryWidgetsVisible(CategoryWidgets& Category, bool Visible);

	std::vector<lcObject*> mSelection;
	lcObject* mFocusObject = nullptr;

	std::array<PropertyWidgets, static_cast<int>(lcObjectPropertyId::Count)> mPropertyWidgets = {};
	std::array<CategoryWidgets, static_cast<int>(CategoryIndex::Count)> mCategoryWidgets = {};

	CategoryWidgets* mCurrentCategory = nullptr;
	QGridLayout* mLayout = nullptr;
	int mLayoutRow = 0;

	LayoutMode mLayoutMode = LayoutMode::Count;
};

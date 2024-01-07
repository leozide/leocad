#pragma once

#include "lc_array.h"

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

	enum class PropertyIndex
	{
		PieceId,
		PieceColor,
		PieceStepShow,
		PieceStepHide,
		CameraName,
		CameraType,
		CameraFOV,
		CameraNear,
		CameraFar,
		CameraPositionX,
		CameraPositionY,
		CameraPositionZ,
		CameraTargetX,
		CameraTargetY,
		CameraTargetZ,
		CameraUpX,
		CameraUpY,
		CameraUpZ,
		LightName,
		LightType,
		LightColor,
		LightPower,
		LightCastShadow,
		LightAttenuationDistance,
		LightAttenuationPower,
		LightPointSize,
		LightSpotSize,
		LightDirectionalSize,
		LightAreaSize,
		LightAreaSizeX,
		LightAreaSizeY,
		LightSpotConeAngle,
		LightSpotPenumbraAngle,
		LightSpotTightness,
		LightAreaShape,
		LightAreaGridX,
		LightAreaGridY,
		ObjectPositionX,
		ObjectPositionY,
		ObjectPositionZ,
		ObjectRotationX,
		ObjectRotationY,
		ObjectRotationZ,
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
		std::vector<QWidget*> Widgets;
		std::vector<int> SpacingRows;
	};

	struct PropertyWidgets
	{
		QLabel* Label = nullptr;
		QWidget* Widget = nullptr;
	};

	PropertyIndex GetWidgetIndex(QWidget* Widget) const;

	void AddCategory(CategoryIndex Index, const QString& Title);
	void AddSpacing();
	void AddLabel(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddBoolProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddFloatProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip, float Min, float Max);
	void AddIntegerProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip, int Min, int Max);
	void AddStepNumberProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddStringProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddStringListProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip, const QStringList& Strings);
	void AddColorProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddPieceColorProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddPieceIdProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);

	void UpdateBool(PropertyIndex Index, bool Value);
	void UpdateFloat(PropertyIndex Index, float Value);
	void UpdateInteger(PropertyIndex Index, int Value);
	void UpdateStepNumber(PropertyIndex Index, lcStep Step, lcStep Min, lcStep Max);
	void UpdateString(PropertyIndex Index, const QString& Text);
	void UpdateStringList(PropertyIndex Index, int ListIndex);
	void UpdateColor(PropertyIndex Index, QColor Color);
	void UpdatePieceColor(PropertyIndex Index, int ColorIndex);
	void UpdatePieceId(PropertyIndex Index, const QString& Name);

	void SetEmpty();
	void SetPiece(const lcArray<lcObject*>& Selection, lcObject* Focus);
	void SetCamera(lcObject* Focus);
	void SetLight(lcObject* Focus);

	void CreateWidgets();
	void SetLayoutMode(LayoutMode Mode);
	void SetPropertyVisible(PropertyIndex Index, bool Visible);
	void SetCategoryVisible(CategoryIndex Index, bool Visible);
	void SetCategoryWidgetsVisible(CategoryWidgets& Category, bool Visible);

	lcObject* mFocusObject = nullptr;

	std::array<PropertyWidgets, static_cast<int>(PropertyIndex::Count)> mPropertyWidgets = {};
	std::array<CategoryWidgets, static_cast<int>(CategoryIndex::Count)> mCategoryWidgets = {};

	CategoryWidgets* mCurrentCategory = nullptr;
	QGridLayout* mLayout = nullptr;
	int mLayoutRow = 0;

	LayoutMode mLayoutMode = LayoutMode::Count;
};

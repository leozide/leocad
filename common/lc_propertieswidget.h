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
	void FloatChanged();
	void StepNumberChanged();
	void PieceColorButtonClicked();
	void PieceColorChanged(int ColorIndex);
	void PieceIdButtonClicked();
	void PieceIdChanged(PieceInfo* Info);

protected:
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
		LightSizeX,
		LightSizeY,
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
		Multiple
	};

	PropertyIndex GetWidgetIndex(QWidget* Widget) const;

	void AddPropertyCategory(const QString& Title);
	void AddSpacing();
	void AddLabel(const QString& Text, const QString& ToolTip);
	void AddBoolProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddFloatProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip, float Min, float Max);
	void AddIntegerProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip, int Min, int Max);
	void AddStepNumberProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddStringProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddStringListProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip, const QStringList& Strings);
	void AddColorProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddPieceColorProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);
	void AddPieceIdProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip);

	void UpdateFloat(PropertyIndex Index, float Value);
	void UpdateStepNumber(PropertyIndex Index, lcStep Step, lcStep Min, lcStep Max);
	void UpdatePieceColor(PropertyIndex Index, int ColorIndex);
	void UpdatePieceId(PropertyIndex Index, const QString& Name);

	void SetEmpty();
	void SetPiece(const lcArray<lcObject*>& Selection, lcObject* Focus);
	void SetCamera(lcObject* Focus);
	void SetLight(lcObject* Focus);
	void SetMultiple();

	void ClearLayout();
	void AddTransformCategory();
	void SetPieceLayout();
	void SetCameraLayout();
	void SetLightLayout(lcLightType LightType, lcLightAreaShape LightAreaShape);

	struct CategoryWidgets
	{
		lcCollapsibleWidgetButton* Button;
		std::vector<QWidget*> Widgets;
		std::vector<int> SpacingRows;
	};

	lcObject* mFocusObject = nullptr;

	std::array<QWidget*, static_cast<int>(PropertyIndex::Count)> mPropertyWidgets = {};
	std::vector<std::unique_ptr<CategoryWidgets>> mCategories;

	CategoryWidgets* mCurrentCategory = nullptr;
	QGridLayout* mLayout = nullptr;
	int mLayoutRow = 0;

	LayoutMode mLayoutMode = LayoutMode::Empty;
	lcLightType mLayoutLightType;
	lcLightAreaShape mLayoutLightAreaShape;
};

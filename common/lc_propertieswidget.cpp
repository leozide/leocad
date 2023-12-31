#include "lc_global.h"
#include "lc_propertieswidget.h"
#include "object.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "pieceinf.h"
#include "lc_mainwindow.h"
#include "lc_collapsiblewidget.h"
#include "lc_colorpicker.h"
#include "lc_qutils.h"

lcPropertiesWidget::lcPropertiesWidget(QWidget* Parent)
	: QWidget(Parent)
{
	mLayoutLightType = lcLightType::Count;
	mLayoutLightAreaShape = lcLightAreaShape::Count;
}

lcPropertiesWidget::PropertyIndex lcPropertiesWidget::GetWidgetIndex(QWidget* Widget) const
{
	if (!Widget)
		return PropertyIndex::Count;

	for (size_t Index = 0; Index < mPropertyWidgets.size(); Index++)
		if (mPropertyWidgets[Index] == Widget)
			return static_cast<PropertyIndex>(Index);

	return PropertyIndex::Count;
}

void lcPropertiesWidget::CategoryStateChanged(bool Expanded)
{
	QObject* Button = sender();

	for (const std::unique_ptr<CategoryWidgets>& Category : mCategories)
	{
		if (Category->Button == Button)
		{
			for (QWidget* Widget : Category->Widgets)
				Widget->setVisible(Expanded);

			for (int Row : Category->SpacingRows)
				mLayout->setRowMinimumHeight(Row, Expanded ? 5 : 0);

			break;
		}
	}
}

void lcPropertiesWidget::AddPropertyCategory(const QString& Title)
{
	mCurrentCategory = new CategoryWidgets();
	mCategories.emplace_back(std::unique_ptr<CategoryWidgets>(mCurrentCategory));

	lcCollapsibleWidgetButton* CategoryButton = new lcCollapsibleWidgetButton(Title);

	mLayout->addWidget(CategoryButton, mLayoutRow, 0, 1, -1);
	mCurrentCategory->Button = CategoryButton;

	connect(CategoryButton, &lcCollapsibleWidgetButton::StateChanged, this, &lcPropertiesWidget::CategoryStateChanged);

	mLayoutRow++;
}

void lcPropertiesWidget::AddSpacing()
{
	mLayout->setRowMinimumHeight(mLayoutRow, 5);
	mCurrentCategory->SpacingRows.push_back(mLayoutRow);
	mLayoutRow++;
}

void lcPropertiesWidget::AddLabel(const QString& Text, const QString& ToolTip)
{
	QLabel* Label = new QLabel(Text, this);
	Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	Label->setToolTip(ToolTip);

	mLayout->addWidget(Label, mLayoutRow, 1);
	mCurrentCategory->Widgets.push_back(Label);
}

void lcPropertiesWidget::AddBoolProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip)
{
	AddLabel(Text, ToolTip);

	QCheckBox* Widget = new QCheckBox(this);
	Widget->setToolTip(ToolTip);
	//	int value = Item->data(0, PropertyValueRole).toInt();

//	updateColorEditor(editor, value);

//	connect(editor, SIGNAL(clicked()), this, SLOT(slotColorButtonClicked()));

	mLayout->addWidget(Widget, mLayoutRow, 2);
	mCurrentCategory->Widgets.push_back(Widget);

	mPropertyWidgets[static_cast<int>(Index)] = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::FloatChanged()
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(sender());
	PropertyIndex Index = GetWidgetIndex(Widget);

	if (Index == PropertyIndex::Count)
		return;

	lcModel* Model = gMainWindow->GetActiveModel();
	lcPiece* Piece = dynamic_cast<lcPiece*>(mFocusObject);
	lcLight* Light = dynamic_cast<lcLight*>(mFocusObject);
	float Value = lcParseValueLocalized(Widget->text());

	// todo: mouse drag

	if (Index == PropertyIndex::ObjectPositionX || Index == PropertyIndex::ObjectPositionY || Index == PropertyIndex::ObjectPositionZ)
	{
		lcVector3 Center;
		lcMatrix33 RelativeRotation;
		Model->GetMoveRotateTransform(Center, RelativeRotation);
		lcVector3 Position = Center;

		if (Index == PropertyIndex::ObjectPositionX)
			Position[0] = Value;
		else if (Index == PropertyIndex::ObjectPositionY)
			Position[1] = Value;
		else if (Index == PropertyIndex::ObjectPositionZ)
			Position[2] = Value;

		lcVector3 Distance = Position - Center;

		Model->MoveSelectedObjects(Distance, Distance, false, true, true, true);
	}
	else if (Index == PropertyIndex::ObjectRotationX || Index == PropertyIndex::ObjectRotationY || Index == PropertyIndex::ObjectRotationZ)
	{
		lcVector3 InitialRotation(0.0f, 0.0f, 0.0f);

		if (Piece)
			InitialRotation = lcMatrix44ToEulerAngles(Piece->mModelWorld) * LC_RTOD;
		else if (Light)
			InitialRotation = lcMatrix44ToEulerAngles(Light->GetWorldMatrix()) * LC_RTOD;

		lcVector3 Rotation = InitialRotation;

		if (Index == PropertyIndex::ObjectRotationX)
			Rotation[0] = Value;
		else if (Index == PropertyIndex::ObjectRotationY)
			Rotation[1] = Value;
		else if (Index == PropertyIndex::ObjectRotationZ)
			Rotation[2] = Value;

		Model->RotateSelectedObjects(Rotation - InitialRotation, true, false, true, true);
	}
}

void lcPropertiesWidget::UpdateFloat(PropertyIndex Index, float Value)
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(mPropertyWidgets[static_cast<int>(Index)]);
		
	if (Widget)
	{
		QSignalBlocker Blocker(Widget);

		Widget->setText(lcFormatValueLocalized(Value));
	}
}

void lcPropertiesWidget::AddFloatProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip, float Min, float Max)
{
	AddLabel(Text, ToolTip);

	QLineEdit* Widget = new QLineEdit(this);
	Widget->setToolTip(ToolTip);

	Widget->setValidator(new QDoubleValidator(Min, Max, 1, Widget));

	connect(Widget, &QLineEdit::editingFinished, this, &lcPropertiesWidget::FloatChanged);

	mLayout->addWidget(Widget, mLayoutRow, 2);
	mCurrentCategory->Widgets.push_back(Widget);

	mPropertyWidgets[static_cast<int>(Index)] = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::AddIntegerProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip, int Min, int Max)
{
	AddLabel(Text, ToolTip);

	QSpinBox* Widget = new QSpinBox(this);
	Widget->setRange(Min, Max);
	Widget->setToolTip(ToolTip);

	mLayout->addWidget(Widget, mLayoutRow, 2);
	mCurrentCategory->Widgets.push_back(Widget);

	mPropertyWidgets[static_cast<int>(Index)] = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::StepNumberChanged()
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(sender());
	PropertyIndex Index = GetWidgetIndex(Widget);

	if (Index == PropertyIndex::Count)
		return;

	lcModel* Model = gMainWindow->GetActiveModel();

	if (!Model)
		return;

	bool Ok = true;
	QString Text = Widget->text();
	lcStep Step = Text.isEmpty() && Index == PropertyIndex::PieceStepHide ? LC_STEP_MAX : Text.toUInt(&Ok);

	if (!Ok)
		return;

	if (Index == PropertyIndex::PieceStepShow)
	{
		Model->SetSelectedPiecesStepShow(Step);
	}
	else if (Index == PropertyIndex::PieceStepHide)
	{
		Model->SetSelectedPiecesStepHide(Step);
	}
}

void lcPropertiesWidget::UpdateStepNumber(PropertyIndex Index, lcStep Step, lcStep Min, lcStep Max)
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(mPropertyWidgets[static_cast<int>(Index)]);

	if (Widget)
	{
		QSignalBlocker Blocker(Widget);

		Widget->setValidator(new lcStepValidator(Min, Max, Index == PropertyIndex::PieceStepHide, Widget));
		Widget->setText(Step == LC_STEP_MAX ? QString() : QString::number(Step));
	}
}

void lcPropertiesWidget::AddStepNumberProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip)
{
	AddLabel(Text, ToolTip);

	QLineEdit* Widget = new QLineEdit(this);
	Widget->setToolTip(ToolTip);

	connect(Widget, &QLineEdit::editingFinished, this, &lcPropertiesWidget::StepNumberChanged);

	mLayout->addWidget(Widget, mLayoutRow, 2);
	mCurrentCategory->Widgets.push_back(Widget);

	mPropertyWidgets[static_cast<int>(Index)] = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::AddStringProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip)
{
	AddLabel(Text, ToolTip);

	QLineEdit* Widget = new QLineEdit(this);
	Widget->setToolTip(ToolTip);

	mLayout->addWidget(Widget, mLayoutRow, 2);
	mCurrentCategory->Widgets.push_back(Widget);

	mPropertyWidgets[static_cast<int>(Index)] = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::AddStringListProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip, const QStringList& Strings)
{
	AddLabel(Text, ToolTip);

	QComboBox* Widget = new QComboBox(this);
	Widget->setToolTip(ToolTip);
	Widget->addItems(Strings);
	//	int value = Item->data(0, PropertyValueRole).toInt();

	//	updateColorEditor(editor, value);

	//	connect(editor, SIGNAL(clicked()), this, SLOT(slotColorButtonClicked()));

	mLayout->addWidget(Widget, mLayoutRow, 2);
	mCurrentCategory->Widgets.push_back(Widget);

	mPropertyWidgets[static_cast<int>(Index)] = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::AddColorProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip)
{
	AddLabel(Text, ToolTip);

	QPushButton* Widget = new QPushButton(this);
	Widget->setToolTip(ToolTip);
	//	int value = Item->data(0, PropertyValueRole).toInt();

//	updateColorEditor(editor, value);

//	connect(editor, SIGNAL(clicked()), this, SLOT(slotColorButtonClicked()));

	mLayout->addWidget(Widget, mLayoutRow, 2);
	mCurrentCategory->Widgets.push_back(Widget);

	mPropertyWidgets[static_cast<int>(Index)] = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::PieceColorChanged(int ColorIndex)
{
	if (!mFocusObject || !mFocusObject->IsPiece())
		return;

	lcModel* Model = gMainWindow->GetActiveModel();
	Model->SetSelectedPiecesColorIndex(ColorIndex);
}

void lcPropertiesWidget::PieceColorButtonClicked()
{
	QToolButton* ColorButton = qobject_cast<QToolButton*>(sender());

	if (!ColorButton || !mFocusObject || !mFocusObject->IsPiece())
		return;

	int ColorIndex = reinterpret_cast<lcPiece*>(mFocusObject)->GetColorIndex();

	lcColorPickerPopup* Popup = new lcColorPickerPopup(ColorButton, ColorIndex);
	connect(Popup, &lcColorPickerPopup::Selected, this, &lcPropertiesWidget::PieceColorChanged);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QScreen* Screen = Button->screen();
	const QRect ScreenRect = Screen ? Screen->geometry() : QRect();
#elif (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	QScreen* Screen = QGuiApplication::screenAt(ColorButton->mapToGlobal(ColorButton->rect().bottomLeft()));
	const QRect ScreenRect = Screen ? Screen->geometry() : QApplication::desktop()->geometry();
#else
	const QRect ScreenRect = QApplication::desktop()->geometry();
#endif

	QPoint Pos = ColorButton->mapToGlobal(ColorButton->rect().bottomLeft());

	if (Pos.x() < ScreenRect.left())
		Pos.setX(ScreenRect.left());
	if (Pos.y() < ScreenRect.top())
		Pos.setY(ScreenRect.top());

	Popup->adjustSize();

	if (Pos.x() + Popup->width() > ScreenRect.right())
		Pos.setX(ScreenRect.right() - Popup->width());
	if (Pos.y() + Popup->height() > ScreenRect.bottom())
		Pos.setY(ScreenRect.bottom() - Popup->height());

	Popup->move(Pos);
	Popup->setFocus();
	Popup->show();
}

void lcPropertiesWidget::UpdatePieceColor(PropertyIndex Index, int ColorIndex)
{
	QToolButton* ColorButton = qobject_cast<QToolButton*>(mPropertyWidgets[static_cast<int>(Index)]);

	if (!ColorButton)
		return;
		
	QPixmap Pixmap(14, 14);
	Pixmap.fill(QColor::fromRgbF(gColorList[ColorIndex].Value[0], gColorList[ColorIndex].Value[1], gColorList[ColorIndex].Value[2]));

	ColorButton->setIcon(Pixmap);
	ColorButton->setText(QString("  ") + gColorList[ColorIndex].Name);
}

void lcPropertiesWidget::AddPieceColorProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip)
{
	AddLabel(Text, ToolTip);

	QToolButton* Widget = new QToolButton(this);
	Widget->setToolTip(ToolTip);
	Widget->setAutoRaise(true);
	Widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	Widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	connect(Widget, &QToolButton::clicked, this, &lcPropertiesWidget::PieceColorButtonClicked);

	mLayout->addWidget(Widget, mLayoutRow, 2);
	mCurrentCategory->Widgets.push_back(Widget);

	mPropertyWidgets[static_cast<int>(Index)] = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::UpdatePieceId(PropertyIndex Index, const QString& Name)
{
	lcElidableToolButton* PieceIdButton = qobject_cast<lcElidableToolButton*>(mPropertyWidgets[static_cast<int>(Index)]);

	if (!PieceIdButton)
		return;

	PieceIdButton->setText(Name);
}

void lcPropertiesWidget::PieceIdButtonClicked()
{
	QToolButton* PieceIdButton = qobject_cast<QToolButton*>(sender());
	lcPiece* Piece = dynamic_cast<lcPiece*>(mFocusObject);

	if (!PieceIdButton || !Piece)
		return;

	QMenu* Menu = new QMenu();

	QWidgetAction* Action = new QWidgetAction(Menu);
	lcPieceIdPickerPopup* Popup = new lcPieceIdPickerPopup(gMainWindow->GetActiveModel(), Piece->mPieceInfo, Menu);
	Action->setDefaultWidget(Popup);
	Menu->addAction(Action);

	connect(Popup, &lcPieceIdPickerPopup::PieceIdSelected, this, &lcPropertiesWidget::PieceIdChanged);

	Menu->exec(PieceIdButton->mapToGlobal(PieceIdButton->rect().bottomLeft()));

	delete Menu;
}

void lcPropertiesWidget::PieceIdChanged(PieceInfo* Info)
{
	lcModel* Model = gMainWindow->GetActiveModel();

	if (!Model || !Info)
		return;

	Model->SetSelectedPiecesPieceInfo(Info);

//	int ColorIndex = gDefaultColor;
//	lcObject* Focus = gMainWindow->GetActiveModel()->GetFocusObject();
//	if (Focus && Focus->IsPiece())
//		ColorIndex = ((lcPiece*)Focus)->GetColorIndex();
//	quint32 ColorCode = lcGetColorCode(ColorIndex);
//	gMainWindow->PreviewPiece(Info->mFileName, ColorCode, false);
}

void lcPropertiesWidget::AddPieceIdProperty(PropertyIndex Index, const QString& Text, const QString& ToolTip)
{
	AddLabel(Text, ToolTip);

	lcElidableToolButton* Widget = new lcElidableToolButton(this);
	Widget->setToolTip(ToolTip);
	Widget->setAutoRaise(true);
	Widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	Widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QPixmap Pixmap(1, 1);
	Pixmap.fill(QColor::fromRgba64(0, 0, 0, 0));
	Widget->setIcon(Pixmap);

	connect(Widget, &QToolButton::clicked, this, &lcPropertiesWidget::PieceIdButtonClicked);

	mLayout->addWidget(Widget, mLayoutRow, 2);
	mCurrentCategory->Widgets.push_back(Widget);

	mPropertyWidgets[static_cast<int>(Index)] = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::ClearLayout()
{
	for (const std::unique_ptr<CategoryWidgets>& Category : mCategories)
	{
		delete Category->Button;
		qDeleteAll(Category->Widgets);
	}

	mCategories.clear();
	mPropertyWidgets.fill(nullptr);

	delete mLayout;
	mLayout = new QGridLayout(this);
	mLayout->setVerticalSpacing(1);
	mLayoutRow = 0;
	mCurrentCategory = nullptr;
}

void lcPropertiesWidget::AddTransformCategory()
{
	AddPropertyCategory(tr("Transform"));

	AddFloatProperty(PropertyIndex::ObjectPositionX, tr("Position X"), tr("Position of the object"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(PropertyIndex::ObjectPositionY, tr("Y"), tr("Position of the object"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(PropertyIndex::ObjectPositionZ, tr("Z"), tr("Position of the object"), -FLT_MAX, FLT_MAX);
	AddSpacing();
	AddFloatProperty(PropertyIndex::ObjectRotationX, tr("Rotation X"), tr("Rotation of the object in degrees"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(PropertyIndex::ObjectRotationY, tr("Y"), tr("Rotation of the object in degrees"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(PropertyIndex::ObjectRotationZ, tr("Z"), tr("Rotation of the object in degrees"), -FLT_MAX, FLT_MAX);

	mLayout->setRowStretch(mLayout->rowCount(), 1);
}

void lcPropertiesWidget::SetEmpty()
{
	ClearLayout();

	mFocusObject = nullptr;
	mLayoutMode = LayoutMode::Empty;
}

void lcPropertiesWidget::SetPieceLayout()
{
	if (mLayoutMode == LayoutMode::Piece)
		return;

	ClearLayout();

	AddPropertyCategory(tr("Piece"));

	AddPieceIdProperty(PropertyIndex::PieceId, tr("Part"), tr("Part Id"));
	AddPieceColorProperty(PropertyIndex::PieceColor, tr("Color"), tr("Piece color"));

	AddSpacing();

	AddStepNumberProperty(PropertyIndex::PieceStepShow, tr("Show"), tr("Step when piece is added to the model"));
	AddStepNumberProperty(PropertyIndex::PieceStepHide, tr("Hide"), tr("Step when piece is hidden"));

	AddTransformCategory();

	mLayoutMode = LayoutMode::Piece;
}

void lcPropertiesWidget::SetPiece(const lcArray<lcObject*>& Selection, lcObject* Focus)
{
	SetPieceLayout();

	lcModel* Model = gMainWindow->GetActiveModel();
	lcPiece* Piece = dynamic_cast<lcPiece*>(Focus);
	mFocusObject = Piece;

	lcVector3 Position;
	lcMatrix33 RelativeRotation;

	Model->GetMoveRotateTransform(Position, RelativeRotation);

	UpdateFloat(PropertyIndex::ObjectPositionX, Position[0]);
	UpdateFloat(PropertyIndex::ObjectPositionY, Position[1]);
	UpdateFloat(PropertyIndex::ObjectPositionZ, Position[2]);

	lcVector3 Rotation;

	if (Piece)
		Rotation = lcMatrix44ToEulerAngles(Piece->mModelWorld) * LC_RTOD;
	else
		Rotation = lcVector3(0.0f, 0.0f, 0.0f);

	UpdateFloat(PropertyIndex::ObjectRotationX, Rotation[0]);
	UpdateFloat(PropertyIndex::ObjectRotationY, Rotation[1]);
	UpdateFloat(PropertyIndex::ObjectRotationZ, Rotation[2]);

	lcStep StepShow = 1;
	lcStep StepHide = LC_STEP_MAX;
	PieceInfo* Info = nullptr;
	int ColorIndex = gDefaultColor;

	if (Piece)
	{
		StepShow = Piece->GetStepShow();
		StepHide = Piece->GetStepHide();
		ColorIndex = Piece->GetColorIndex();
		Info = Piece->mPieceInfo;
//		quint32 ColorCode = lcGetColorCode(ColorIndex);
//		gMainWindow->PreviewPiece(Info->mFileName, ColorCode, false);
	}
	else
	{
		bool FirstPiece = true;

		for (int ObjectIdx = 0; ObjectIdx < Selection.GetSize(); ObjectIdx++)
		{
			lcObject* Object = Selection[ObjectIdx];

			if (!Object->IsPiece())
				continue;

			lcPiece* SelectedPiece = (lcPiece*)Object;

			if (FirstPiece)
			{
				StepShow = SelectedPiece->GetStepShow();
				StepHide = SelectedPiece->GetStepHide();
				ColorIndex = SelectedPiece->GetColorIndex();
				Info = SelectedPiece->mPieceInfo;

				FirstPiece = false;
			}
			else
			{
				if (SelectedPiece->GetStepShow() != StepShow)
					StepShow = 0;

				if (SelectedPiece->GetStepHide() != StepHide)
					StepHide = 0;

				if (SelectedPiece->GetColorIndex() != ColorIndex)
					ColorIndex = gDefaultColor;

				if (SelectedPiece->mPieceInfo != Info)
					Info = nullptr;
			}
		}
	}

	UpdatePieceId(PropertyIndex::PieceId, Info ? Info->m_strDescription : QString());
	UpdatePieceColor(PropertyIndex::PieceColor, ColorIndex);
	UpdateStepNumber(PropertyIndex::PieceStepShow, StepShow ? StepShow : LC_STEP_MAX, 1, StepHide - 1);
	UpdateStepNumber(PropertyIndex::PieceStepHide, StepHide ? StepHide : LC_STEP_MAX, StepShow + 1, LC_STEP_MAX);
}

void lcPropertiesWidget::SetCameraLayout()
{
	if (mLayoutMode == LayoutMode::Camera)
		return;

	ClearLayout();

	AddPropertyCategory(tr("Camera"));
	
	AddStringProperty(PropertyIndex::CameraName, tr("Name"), tr("Camera name"));
	AddStringListProperty(PropertyIndex::CameraType, tr("Type"), tr("Camera type"), { tr("Perspective"), tr("Orthographic") });
	AddSpacing();
	AddFloatProperty(PropertyIndex::CameraFOV, tr("FOV"), tr("Field of view in degrees"), 0.1f, 179.9f);
	AddFloatProperty(PropertyIndex::CameraNear, tr("Near"), tr("Near clipping distance"), 0.001f, FLT_MAX);
	AddFloatProperty(PropertyIndex::CameraFar, tr("Far"), tr("Far clipping distance"), 0.001f, FLT_MAX);

	AddPropertyCategory(tr("Transform"));

	AddFloatProperty(PropertyIndex::CameraPositionX, tr("Position X"), tr("Camera position"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(PropertyIndex::CameraPositionY, tr("Y"), tr("Camera position"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(PropertyIndex::CameraPositionZ, tr("Z"), tr("Camera position"), -FLT_MAX, FLT_MAX);
	AddSpacing();
	AddFloatProperty(PropertyIndex::CameraTargetX, tr("Target X"), tr("Camera target position"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(PropertyIndex::CameraTargetY, tr("Y"), tr("Camera target position"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(PropertyIndex::CameraTargetZ, tr("Z"), tr("Camera target position"), -FLT_MAX, FLT_MAX);
	AddSpacing();
	AddFloatProperty(PropertyIndex::CameraUpX, tr("Up X"), tr("Camera up direction"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(PropertyIndex::CameraUpY, tr("Y"), tr("Camera up direction"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(PropertyIndex::CameraUpZ, tr("Z"), tr("Camera up direction"), -FLT_MAX, FLT_MAX);

//	AddTransformCategory(Layout);

	mLayoutMode = LayoutMode::Camera;
}

void lcPropertiesWidget::SetCamera(lcObject* Focus)
{
	SetCameraLayout();
}

void lcPropertiesWidget::SetLightLayout(lcLightType LightType, lcLightAreaShape LightAreaShape)
{
	if (mLayoutMode == LayoutMode::Light && mLayoutLightType == LightType)
		return;

	ClearLayout();

	AddPropertyCategory(tr("Light"));

	AddStringProperty(PropertyIndex::LightName, tr("Name"), tr("Light name"));
	AddStringListProperty(PropertyIndex::LightType, tr("Type"), tr("Light type"), lcLight::GetLightTypeStrings());
	AddColorProperty(PropertyIndex::LightColor, tr("Color"), tr("Light color"));
	AddFloatProperty(PropertyIndex::LightPower, tr("Power"), tr("Power of the light (Watts in Blender, multiplicative factor in POV-Ray)"), 0.0f, FLT_MAX);
	AddBoolProperty(PropertyIndex::LightCastShadow, tr("Cast Shadow"), tr("Cast a shadow from this light"));
	AddFloatProperty(PropertyIndex::LightAttenuationDistance, tr("Fade Distance"), tr("The distance at which the full light intensity arrives (POV-Ray only)"), 0.0f, FLT_MAX);
	AddFloatProperty(PropertyIndex::LightAttenuationPower, tr("Fade Power"), tr("Light falloff rate (POV-Ray only)"), 0.0f, FLT_MAX);
//	AddSpacing(LightLayout, LightRow);

	switch (LightType)
	{
	case lcLightType::Point:
		AddFloatProperty(PropertyIndex::LightSizeX, tr("Radius"), tr("Shadow soft size (Blender only)"), 0.0f, FLT_MAX);
		break;

	case lcLightType::Spot:
		AddFloatProperty(PropertyIndex::LightSizeX, tr("Radius"), tr("Shadow soft size (Blender only)"), 0.0f, FLT_MAX);
		AddFloatProperty(PropertyIndex::LightSpotConeAngle, tr("Spot Cone Angle"), tr("Angle in degrees of the spot light's beam"), 0.0f, 179.9f);
		AddFloatProperty(PropertyIndex::LightSpotPenumbraAngle, tr("Spot Penumbra Angle"), tr("Angle in degrees over which the intensity of the spot light falls off to zero"), 0.0f, 179.9f);
		AddFloatProperty(PropertyIndex::LightSpotTightness, tr("Spot Tightness"), tr("Additional exponential spot light edge softening (POV-Ray only)"), 0.0f, FLT_MAX);
		break;

	case lcLightType::Directional:
		AddFloatProperty(PropertyIndex::LightSizeX, tr("Angle"), tr("Angular diameter of the light (Blender only)"), 0.0f, 180.0f);
		break;

	case lcLightType::Area:
		AddStringListProperty(PropertyIndex::LightAreaShape, tr("Area Shape"), tr("The shape of the area light"), lcLight::GetAreaShapeStrings());

		switch (LightAreaShape)
		{
		case lcLightAreaShape::Rectangle:
		case lcLightAreaShape::Ellipse:
			AddFloatProperty(PropertyIndex::LightSizeX, tr("Size X"), tr("The width of the area light"), 0.0f, FLT_MAX);
			AddFloatProperty(PropertyIndex::LightSizeY, tr("Y"), tr("The height of the area light"), 0.0f, FLT_MAX);
			break;

		case lcLightAreaShape::Square:
		case lcLightAreaShape::Disk:
			AddFloatProperty(PropertyIndex::LightSizeX, tr("Size"), tr("The size of the area light"), 0.0f, FLT_MAX);
			break;

		case lcLightAreaShape::Count:
			break;
		}

		AddIntegerProperty(PropertyIndex::LightAreaGridX, tr("Grid X"), tr("Number of point sources along the X axis (POV-Ray only)"), 1, INT_MAX);
		AddIntegerProperty(PropertyIndex::LightAreaGridY, tr("Y"), tr("Number of point sources along the Y axis (POV-Ray only)"), 1, INT_MAX);
		break;

	case lcLightType::Count:
		break;
	}

	AddTransformCategory();

	mLayoutLightType = LightType;
	mLayoutMode = LayoutMode::Light;
}

void lcPropertiesWidget::SetLight(lcObject* Focus)
{
	lcLight* Light = (Focus && Focus->IsLight()) ? (lcLight*)Focus : nullptr;

	if (Light)
		SetLightLayout(Light->GetLightType(), Light->GetAreaShape());
	else
		SetLightLayout(lcLightType::Count, lcLightAreaShape::Count);
}

void lcPropertiesWidget::SetMultiple()
{
}

void lcPropertiesWidget::Update(const lcArray<lcObject*>& Selection, lcObject* Focus)
{
	LayoutMode Mode = LayoutMode::Empty;

	if (Focus)
	{
		switch (Focus->GetType())
		{
		case lcObjectType::Piece:
			Mode = LayoutMode::Piece;
			break;

		case lcObjectType::Camera:
			Mode = LayoutMode::Camera;
			break;

		case lcObjectType::Light:
			Mode = LayoutMode::Light;
			break;
		}
	}
	else
	{
		for (int ObjectIdx = 0; ObjectIdx < Selection.GetSize(); ObjectIdx++)
		{
			switch (Selection[ObjectIdx]->GetType())
			{
			case lcObjectType::Piece:
				if (Mode == LayoutMode::Empty)
					Mode = LayoutMode::Piece;
				else if (Mode != LayoutMode::Piece)
				{
					Mode = LayoutMode::Multiple;
					ObjectIdx = Selection.GetSize();
				}
				break;

			case lcObjectType::Camera:
				if (Mode != LayoutMode::Empty)
				{
					Mode = LayoutMode::Multiple;
					ObjectIdx = Selection.GetSize();
				}
				else
				{
					Mode = LayoutMode::Camera;
					Focus = Selection[ObjectIdx];
				}
				break;

			case lcObjectType::Light:
				if (Mode != LayoutMode::Empty)
				{
					Mode = LayoutMode::Multiple;
					ObjectIdx = Selection.GetSize();
				}
				else
				{
					Mode = LayoutMode::Light;
					Focus = Selection[ObjectIdx];
				}
				break;
			}
		}
	}

	switch (Mode)
	{
	case LayoutMode::Empty:
		SetEmpty();
		break;

	case LayoutMode::Piece:
		SetPiece(Selection, Focus);
		break;

	case LayoutMode::Camera:
		SetCamera(Focus);
		break;

	case LayoutMode::Light:
		SetLight(Focus);
		break;

	case LayoutMode::Multiple:
		SetMultiple();
		break;
	}
}

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
	CreateWidgets();
	SetLayoutMode(LayoutMode::Empty);
}

lcObjectPropertyId lcPropertiesWidget::GetWidgetPropertyId(QWidget* Widget) const
{
	if (!Widget)
		return lcObjectPropertyId::Count;

	for (size_t Index = 0; Index < mPropertyWidgets.size(); Index++)
		if (mPropertyWidgets[Index].Widget == Widget)
			return static_cast<lcObjectPropertyId>(Index);

	return lcObjectPropertyId::Count;
}

void lcPropertiesWidget::CategoryStateChanged(bool Expanded)
{
	QObject* Button = sender();

	for (CategoryWidgets& Category : mCategoryWidgets)
	{
		if (Category.Button == Button)
		{
			SetCategoryWidgetsVisible(Category, Expanded);
			break;
		}
	}
}

void lcPropertiesWidget::AddCategory(CategoryIndex Index, const QString& Title)
{
	mCurrentCategory = &mCategoryWidgets[static_cast<int>(Index)];

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

void lcPropertiesWidget::AddLabel(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip)
{
	QLabel* Label = new QLabel(Text, this);
	Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	Label->setToolTip(ToolTip);

	mLayout->addWidget(Label, mLayoutRow, 1);

	mPropertyWidgets[static_cast<int>(PropertyId)].Label = Label;
}

void lcPropertiesWidget::BoolChanged()
{
	QCheckBox* Widget = qobject_cast<QCheckBox*>(sender());
	lcObjectPropertyId PropertyId = GetWidgetPropertyId(Widget);

	if (PropertyId == lcObjectPropertyId::Count)
		return;

	lcModel* Model = gMainWindow->GetActiveModel();

	if (!Model)
		return;

	lcLight* Light = dynamic_cast<lcLight*>(mFocusObject);
	bool Value = Widget->isChecked();

	if (Light)
	{
		if (PropertyId == lcObjectPropertyId::LightCastShadow)
		{
			Model->SetLightCastShadow(Light, Value);
		}
	}
}

void lcPropertiesWidget::UpdateBool(lcObjectPropertyId PropertyId, bool Value)
{
	QCheckBox* Widget = qobject_cast<QCheckBox*>(mPropertyWidgets[static_cast<int>(PropertyId)].Widget);

	if (Widget)
	{
		QSignalBlocker Blocker(Widget);

		Widget->setChecked(Value);
	}
}

void lcPropertiesWidget::AddBoolProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip)
{
	AddLabel(PropertyId, Text, ToolTip);

	QCheckBox* Widget = new QCheckBox(this);
	Widget->setToolTip(ToolTip);

	connect(Widget, &QCheckBox::toggled, this, &lcPropertiesWidget::BoolChanged);

	mLayout->addWidget(Widget, mLayoutRow, 2);

	mCurrentCategory->Properties.push_back(PropertyId);
	mPropertyWidgets[static_cast<int>(PropertyId)].Widget = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::FloatChanged()
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(sender());
	lcObjectPropertyId PropertyId = GetWidgetPropertyId(Widget);

	if (PropertyId == lcObjectPropertyId::Count)
		return;

	lcModel* Model = gMainWindow->GetActiveModel();

	if (!Model)
		return;

	lcPiece* Piece = dynamic_cast<lcPiece*>(mFocusObject);
	lcCamera* Camera = dynamic_cast<lcCamera*>(mFocusObject);
	lcLight* Light = dynamic_cast<lcLight*>(mFocusObject);
	float Value = lcParseValueLocalized(Widget->text());

	// todo: mouse drag

	if (Piece || Light)
	{
		if (PropertyId == lcObjectPropertyId::ObjectPositionX || PropertyId == lcObjectPropertyId::ObjectPositionY || PropertyId == lcObjectPropertyId::ObjectPositionZ)
		{
			lcVector3 Center;
			lcMatrix33 RelativeRotation;
			Model->GetMoveRotateTransform(Center, RelativeRotation);
			lcVector3 Position = Center;

			if (PropertyId == lcObjectPropertyId::ObjectPositionX)
				Position[0] = Value;
			else if (PropertyId == lcObjectPropertyId::ObjectPositionY)
				Position[1] = Value;
			else if (PropertyId == lcObjectPropertyId::ObjectPositionZ)
				Position[2] = Value;

			lcVector3 Distance = Position - Center;

			Model->MoveSelectedObjects(Distance, Distance, false, true, true, true);
		}
		else if (PropertyId == lcObjectPropertyId::ObjectRotationX || PropertyId == lcObjectPropertyId::ObjectRotationY || PropertyId == lcObjectPropertyId::ObjectRotationZ)
		{
			lcVector3 InitialRotation(0.0f, 0.0f, 0.0f);

			if (Piece)
				InitialRotation = lcMatrix44ToEulerAngles(Piece->mModelWorld) * LC_RTOD;
			else if (Light)
				InitialRotation = lcMatrix44ToEulerAngles(Light->GetWorldMatrix()) * LC_RTOD;

			lcVector3 Rotation = InitialRotation;

			if (PropertyId == lcObjectPropertyId::ObjectRotationX)
				Rotation[0] = Value;
			else if (PropertyId == lcObjectPropertyId::ObjectRotationY)
				Rotation[1] = Value;
			else if (PropertyId == lcObjectPropertyId::ObjectRotationZ)
				Rotation[2] = Value;

			Model->RotateSelectedObjects(Rotation - InitialRotation, true, false, true, true);
		}
	}

	if (Camera)
	{
		if (PropertyId == lcObjectPropertyId::CameraPositionX || PropertyId == lcObjectPropertyId::CameraPositionY || PropertyId == lcObjectPropertyId::CameraPositionZ)
		{
			lcVector3 Center = Camera->mPosition;
			lcVector3 Position = Center;

			if (PropertyId == lcObjectPropertyId::CameraPositionX)
				Position[0] = Value;
			else if (PropertyId == lcObjectPropertyId::CameraPositionY)
				Position[1] = Value;
			else if (PropertyId == lcObjectPropertyId::CameraPositionZ)
				Position[2] = Value;

			lcVector3 Distance = Position - Center;

			Model->MoveSelectedObjects(Distance, Distance, false, false, true, true);
		}
		else if (PropertyId == lcObjectPropertyId::CameraTargetX || PropertyId == lcObjectPropertyId::CameraTargetY || PropertyId == lcObjectPropertyId::CameraTargetZ)
		{
			lcVector3 Center = Camera->mTargetPosition;
			lcVector3 Position = Center;

			if (PropertyId == lcObjectPropertyId::CameraTargetX)
				Position[0] = Value;
			else if (PropertyId == lcObjectPropertyId::CameraTargetY)
				Position[1] = Value;
			else if (PropertyId == lcObjectPropertyId::CameraTargetZ)
				Position[2] = Value;

			lcVector3 Distance = Position - Center;

			Model->MoveSelectedObjects(Distance, Distance, false, false, true, true);
		}
		else if (PropertyId == lcObjectPropertyId::CameraUpX || PropertyId == lcObjectPropertyId::CameraUpY || PropertyId == lcObjectPropertyId::CameraUpZ)
		{
			lcVector3 Center = Camera->mUpVector;
			lcVector3 Position = Center;

			if (PropertyId == lcObjectPropertyId::CameraUpX)
				Position[0] = Value;
			else if (PropertyId == lcObjectPropertyId::CameraUpY)
				Position[1] = Value;
			else if (PropertyId == lcObjectPropertyId::CameraUpZ)
				Position[2] = Value;

			lcVector3 Distance = Position - Center;

			Model->MoveSelectedObjects(Distance, Distance, false, false, true, true);
		}
		else if (PropertyId == lcObjectPropertyId::CameraFOV)
		{
			Model->SetCameraFOV(Camera, Value);
		}
		else if (PropertyId == lcObjectPropertyId::CameraNear)
		{
			Model->SetCameraZNear(Camera, Value);
		}
		else if (PropertyId == lcObjectPropertyId::CameraFar)
		{
			Model->SetCameraZFar(Camera, Value);
		}
	}

	if (Light)
	{
		if (PropertyId == lcObjectPropertyId::LightPower)
		{
			Model->SetLightPower(Light, Value);
		}
		else if (PropertyId == lcObjectPropertyId::LightAttenuationDistance)
		{
			Model->SetLightAttenuationDistance(Light, Value);
		}
		else if (PropertyId == lcObjectPropertyId::LightAttenuationPower)
		{
			Model->SetLightAttenuationPower(Light, Value);
		}
		else if (PropertyId == lcObjectPropertyId::LightPointSize || PropertyId == lcObjectPropertyId::LightSpotSize || PropertyId == lcObjectPropertyId::LightDirectionalSize || PropertyId == lcObjectPropertyId::LightAreaSizeX)
		{
			lcVector2 LightSize = Light->GetSize();
			LightSize[0] = Value;

			Model->SetLightSize(Light, LightSize);
		}
		else if (PropertyId == lcObjectPropertyId::LightAreaSizeY)
		{
			lcVector2 LightSize = Light->GetSize();
			LightSize[1] = Value;

			Model->SetLightSize(Light, LightSize);
		}
		else if (PropertyId == lcObjectPropertyId::LightSpotConeAngle)
		{
			Model->SetSpotLightConeAngle(Light, Value);
		}
		else if (PropertyId == lcObjectPropertyId::LightSpotPenumbraAngle)
		{
			Model->SetSpotLightPenumbraAngle(Light, Value);
		}
		else if (PropertyId == lcObjectPropertyId::LightSpotTightness)
		{
			Model->SetSpotLightTightness(Light, Value);
		}
	}
}

void lcPropertiesWidget::UpdateFloat(lcObjectPropertyId PropertyId, float Value)
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(mPropertyWidgets[static_cast<int>(PropertyId)].Widget);
		
	if (Widget)
	{
		QSignalBlocker Blocker(Widget);

		Widget->setText(lcFormatValueLocalized(Value));
	}
}

void lcPropertiesWidget::AddFloatProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, float Min, float Max)
{
	AddLabel(PropertyId, Text, ToolTip);

	QLineEdit* Widget = new QLineEdit(this);
	Widget->setToolTip(ToolTip);

	Widget->setValidator(new QDoubleValidator(Min, Max, 1, Widget));

	connect(Widget, &QLineEdit::editingFinished, this, &lcPropertiesWidget::FloatChanged);

	mLayout->addWidget(Widget, mLayoutRow, 2);

	mCurrentCategory->Properties.push_back(PropertyId);
	mPropertyWidgets[static_cast<int>(PropertyId)].Widget = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::IntegerChanged()
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(sender());
	lcObjectPropertyId PropertyId = GetWidgetPropertyId(Widget);

	if (PropertyId == lcObjectPropertyId::Count)
		return;

	lcModel* Model = gMainWindow->GetActiveModel();

	if (!Model)
		return;

	lcLight* Light = dynamic_cast<lcLight*>(mFocusObject);
	int Value = Widget->text().toInt();

	// todo: mouse drag

	if (Light)
	{
		if (PropertyId == lcObjectPropertyId::LightAreaGridX)
		{
			lcVector2i AreaGrid = Light->GetAreaGrid();
			AreaGrid.x = Value;

			Model->SetLightAreaGrid(Light, AreaGrid);
		}
		else if (PropertyId == lcObjectPropertyId::LightAreaGridY)
		{
			lcVector2i AreaGrid = Light->GetAreaGrid();
			AreaGrid.y = Value;

			Model->SetLightAreaGrid(Light, AreaGrid);
		}
	}
}

void lcPropertiesWidget::UpdateInteger(lcObjectPropertyId PropertyId, int Value)
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(mPropertyWidgets[static_cast<int>(PropertyId)].Widget);

	if (Widget)
	{
		QSignalBlocker Blocker(Widget);

		Widget->setText(lcFormatValueLocalized(Value));
	}
}

void lcPropertiesWidget::AddIntegerProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, int Min, int Max)
{
	AddLabel(PropertyId, Text, ToolTip);

	QLineEdit* Widget = new QLineEdit(this);
	Widget->setToolTip(ToolTip);

	Widget->setValidator(new QIntValidator(Min, Max, Widget));

	connect(Widget, &QLineEdit::editingFinished, this, &lcPropertiesWidget::IntegerChanged);

	mLayout->addWidget(Widget, mLayoutRow, 2);

	mCurrentCategory->Properties.push_back(PropertyId);
	mPropertyWidgets[static_cast<int>(PropertyId)].Widget = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::StepNumberChanged()
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(sender());
	lcObjectPropertyId PropertyId = GetWidgetPropertyId(Widget);

	if (PropertyId == lcObjectPropertyId::Count)
		return;

	lcModel* Model = gMainWindow->GetActiveModel();

	if (!Model)
		return;

	bool Ok = true;
	QString Text = Widget->text();
	lcStep Step = Text.isEmpty() && PropertyId == lcObjectPropertyId::PieceStepHide ? LC_STEP_MAX : Text.toUInt(&Ok);

	if (!Ok)
		return;

	if (PropertyId == lcObjectPropertyId::PieceStepShow)
	{
		Model->SetSelectedPiecesStepShow(Step);
	}
	else if (PropertyId == lcObjectPropertyId::PieceStepHide)
	{
		Model->SetSelectedPiecesStepHide(Step);
	}
}

void lcPropertiesWidget::UpdateStepNumber(lcObjectPropertyId PropertyId, lcStep Step, lcStep Min, lcStep Max)
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(mPropertyWidgets[static_cast<int>(PropertyId)].Widget);

	if (Widget)
	{
		QSignalBlocker Blocker(Widget);

		Widget->setValidator(new lcStepValidator(Min, Max, PropertyId == lcObjectPropertyId::PieceStepHide, Widget));
		Widget->setText(Step == LC_STEP_MAX ? QString() : QString::number(Step));
	}
}

void lcPropertiesWidget::AddStepNumberProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip)
{
	AddLabel(PropertyId, Text, ToolTip);

	QLineEdit* Widget = new QLineEdit(this);
	Widget->setToolTip(ToolTip);

	connect(Widget, &QLineEdit::editingFinished, this, &lcPropertiesWidget::StepNumberChanged);

	mLayout->addWidget(Widget, mLayoutRow, 2);

	mCurrentCategory->Properties.push_back(PropertyId);
	mPropertyWidgets[static_cast<int>(PropertyId)].Widget = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::StringChanged()
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(sender());
	lcObjectPropertyId PropertyId = GetWidgetPropertyId(Widget);

	if (PropertyId == lcObjectPropertyId::Count)
		return;

	lcModel* Model = gMainWindow->GetActiveModel();

	if (!Model)
		return;

	lcCamera* Camera = dynamic_cast<lcCamera*>(mFocusObject);
	lcLight* Light = dynamic_cast<lcLight*>(mFocusObject);
	QString Text = Widget->text();

	if (Camera)
	{
		if (PropertyId == lcObjectPropertyId::CameraName)
		{
			Model->SetCameraName(Camera, Text);
		}
	}
	else if (Light)
	{
		if (PropertyId == lcObjectPropertyId::LightName)
		{
			Model->SetLightName(Light, Text);
		}
	}
}

void lcPropertiesWidget::UpdateString(lcObjectPropertyId PropertyId, const QString& Text)
{
	QLineEdit* Widget = qobject_cast<QLineEdit*>(mPropertyWidgets[static_cast<int>(PropertyId)].Widget);

	if (Widget)
	{
		QSignalBlocker Blocker(Widget);

		Widget->setText(Text);
	}
}

void lcPropertiesWidget::AddStringProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip)
{
	AddLabel(PropertyId, Text, ToolTip);

	QLineEdit* Widget = new QLineEdit(this);
	Widget->setToolTip(ToolTip);

	connect(Widget, &QLineEdit::editingFinished, this, &lcPropertiesWidget::StringChanged);

	mLayout->addWidget(Widget, mLayoutRow, 2);

	mCurrentCategory->Properties.push_back(PropertyId);
	mPropertyWidgets[static_cast<int>(PropertyId)].Widget = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::StringListChanged(int Value)
{
	QComboBox* Widget = qobject_cast<QComboBox*>(sender());
	lcObjectPropertyId PropertyId = GetWidgetPropertyId(Widget);

	if (PropertyId == lcObjectPropertyId::Count)
		return;

	lcModel* Model = gMainWindow->GetActiveModel();

	if (!Model)
		return;

	lcCamera* Camera = dynamic_cast<lcCamera*>(mFocusObject);
	lcLight* Light = dynamic_cast<lcLight*>(mFocusObject);

	if (Camera)
	{
		if (PropertyId == lcObjectPropertyId::CameraType)
		{
			Model->SetCameraOrthographic(Camera, Value == 1);
		}
	}
	else if (Light)
	{
		if (PropertyId == lcObjectPropertyId::LightType)
		{
			Model->SetLightType(Light, static_cast<lcLightType>(Value));
		}
		else if (PropertyId == lcObjectPropertyId::LightAreaShape)
		{
			Model->SetLightAreaShape(Light, static_cast<lcLightAreaShape>(Value));
		}
	}
}

void lcPropertiesWidget::UpdateStringList(lcObjectPropertyId PropertyId, int ListIndex)
{
	QComboBox* Widget = qobject_cast<QComboBox*>(mPropertyWidgets[static_cast<int>(PropertyId)].Widget);

	if (Widget)
	{
		QSignalBlocker Blocker(Widget);

		Widget->setCurrentIndex(ListIndex);
	}
}

void lcPropertiesWidget::AddStringListProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip, const QStringList& Strings)
{
	AddLabel(PropertyId, Text, ToolTip);

	QComboBox* Widget = new QComboBox(this);
	Widget->setToolTip(ToolTip);
	Widget->addItems(Strings);

	connect(Widget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &lcPropertiesWidget::StringListChanged);

	mLayout->addWidget(Widget, mLayoutRow, 2);

	mCurrentCategory->Properties.push_back(PropertyId);
	mPropertyWidgets[static_cast<int>(PropertyId)].Widget = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::ColorButtonClicked()
{
	lcLight* Light = dynamic_cast<lcLight*>(mFocusObject);
	QToolButton* ColorButton = qobject_cast<QToolButton*>(sender());
	lcModel* Model = gMainWindow->GetActiveModel();

	if (!ColorButton || !Light || !Model)
		return;

	QColor Color = QColorDialog::getColor(lcQColorFromVector3(Light->GetColor()), this, tr("Select Light Color"));

	if (!Color.isValid())
		return;

	Model->SetLightColor(Light, lcVector3FromQColor(Color));
}

void lcPropertiesWidget::UpdateColor(lcObjectPropertyId PropertyId, QColor Color)
{
	QToolButton* ColorButton = qobject_cast<QToolButton*>(mPropertyWidgets[static_cast<int>(PropertyId)].Widget);

	if (!ColorButton)
		return;

	QPixmap Pixmap(14, 14);
	Pixmap.fill(Color);

	ColorButton->setIcon(Pixmap);
	ColorButton->setText(QString("  ") + Color.name());
}

void lcPropertiesWidget::AddColorProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip)
{
	AddLabel(PropertyId, Text, ToolTip);

	QToolButton* Widget = new QToolButton(this);
	Widget->setToolTip(ToolTip);
	Widget->setAutoRaise(true);
	Widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	Widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	connect(Widget, &QToolButton::clicked, this, &lcPropertiesWidget::ColorButtonClicked);

	mLayout->addWidget(Widget, mLayoutRow, 2);

	mCurrentCategory->Properties.push_back(PropertyId);
	mPropertyWidgets[static_cast<int>(PropertyId)].Widget = Widget;

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

void lcPropertiesWidget::UpdatePieceColor(lcObjectPropertyId PropertyId, int ColorIndex)
{
	QToolButton* ColorButton = qobject_cast<QToolButton*>(mPropertyWidgets[static_cast<int>(PropertyId)].Widget);

	if (!ColorButton)
		return;
		
	QPixmap Pixmap(14, 14);
	Pixmap.fill(QColor::fromRgbF(gColorList[ColorIndex].Value[0], gColorList[ColorIndex].Value[1], gColorList[ColorIndex].Value[2]));

	ColorButton->setIcon(Pixmap);
	ColorButton->setText(QString("  ") + gColorList[ColorIndex].Name);
}

void lcPropertiesWidget::AddPieceColorProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip)
{
	AddLabel(PropertyId, Text, ToolTip);

	QToolButton* Widget = new QToolButton(this);
	Widget->setToolTip(ToolTip);
	Widget->setAutoRaise(true);
	Widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	Widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	connect(Widget, &QToolButton::clicked, this, &lcPropertiesWidget::PieceColorButtonClicked);

	mLayout->addWidget(Widget, mLayoutRow, 2);

	mCurrentCategory->Properties.push_back(PropertyId);
	mPropertyWidgets[static_cast<int>(PropertyId)].Widget = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::UpdatePieceId(lcObjectPropertyId PropertyId, const QString& Name)
{
	lcElidableToolButton* PieceIdButton = qobject_cast<lcElidableToolButton*>(mPropertyWidgets[static_cast<int>(PropertyId)].Widget);

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
}

void lcPropertiesWidget::AddPieceIdProperty(lcObjectPropertyId PropertyId, const QString& Text, const QString& ToolTip)
{
	AddLabel(PropertyId, Text, ToolTip);

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

	mCurrentCategory->Properties.push_back(PropertyId);
	mPropertyWidgets[static_cast<int>(PropertyId)].Widget = Widget;

	mLayoutRow++;
}

void lcPropertiesWidget::CreateWidgets()
{
	mLayout = new QGridLayout(this);
	mLayout->setVerticalSpacing(1);

	AddCategory(CategoryIndex::Piece, tr("Piece"));

	AddPieceIdProperty(lcObjectPropertyId::PieceId, tr("Part"), tr("Part Id"));
	AddPieceColorProperty(lcObjectPropertyId::PieceColor, tr("Color"), tr("Piece color"));

	AddSpacing();

	AddStepNumberProperty(lcObjectPropertyId::PieceStepShow, tr("Show"), tr("Step when piece is added to the model"));
	AddStepNumberProperty(lcObjectPropertyId::PieceStepHide, tr("Hide"), tr("Step when piece is hidden"));

	AddCategory(CategoryIndex::Camera, tr("Camera"));

	AddStringProperty(lcObjectPropertyId::CameraName, tr("Name"), tr("Camera name"));
	AddStringListProperty(lcObjectPropertyId::CameraType, tr("Type"), tr("Camera type"), { tr("Perspective"), tr("Orthographic") });

	AddSpacing();

	AddFloatProperty(lcObjectPropertyId::CameraFOV, tr("FOV"), tr("Field of view in degrees"), 0.1f, 179.9f);
	AddFloatProperty(lcObjectPropertyId::CameraNear, tr("Near"), tr("Near clipping distance"), 0.001f, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::CameraFar, tr("Far"), tr("Far clipping distance"), 0.001f, FLT_MAX);

	AddCategory(CategoryIndex::CameraTransform, tr("Transform"));

	AddFloatProperty(lcObjectPropertyId::CameraPositionX, tr("Position X"), tr("Camera position"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::CameraPositionY, tr("Y"), tr("Camera position"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::CameraPositionZ, tr("Z"), tr("Camera position"), -FLT_MAX, FLT_MAX);

	AddSpacing();

	AddFloatProperty(lcObjectPropertyId::CameraTargetX, tr("Target X"), tr("Camera target position"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::CameraTargetY, tr("Y"), tr("Camera target position"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::CameraTargetZ, tr("Z"), tr("Camera target position"), -FLT_MAX, FLT_MAX);

	AddSpacing();

	AddFloatProperty(lcObjectPropertyId::CameraUpX, tr("Up X"), tr("Camera up direction"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::CameraUpY, tr("Y"), tr("Camera up direction"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::CameraUpZ, tr("Z"), tr("Camera up direction"), -FLT_MAX, FLT_MAX);

	AddCategory(CategoryIndex::Light, tr("Light"));

	AddStringProperty(lcObjectPropertyId::LightName, tr("Name"), tr("Light name"));
	AddStringListProperty(lcObjectPropertyId::LightType, tr("Type"), tr("Light type"), lcLight::GetLightTypeStrings());

	AddSpacing();

	AddColorProperty(lcObjectPropertyId::LightColor, tr("Color"), tr("Light color"));
	AddFloatProperty(lcObjectPropertyId::LightPower, tr("Power"), tr("Power of the light (Watts in Blender, multiplicative factor in POV-Ray)"), 0.0f, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::LightAttenuationDistance, tr("Fade Distance"), tr("The distance at which the full light intensity arrives (POV-Ray only)"), 0.0f, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::LightAttenuationPower, tr("Fade Power"), tr("Light falloff rate (POV-Ray only)"), 0.0f, FLT_MAX);
	AddBoolProperty(lcObjectPropertyId::LightCastShadow, tr("Cast Shadow"), tr("Cast a shadow from this light"));

	AddSpacing();

	AddFloatProperty(lcObjectPropertyId::LightPointSize, tr("Radius"), tr("Shadow soft size (Blender only)"), 0.0f, FLT_MAX);

	AddFloatProperty(lcObjectPropertyId::LightSpotSize, tr("Radius"), tr("Shadow soft size (Blender only)"), 0.0f, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::LightSpotConeAngle, tr("Spot Cone Angle"), tr("Angle in degrees of the spot light's beam"), 0.0f, 179.9f);
	AddFloatProperty(lcObjectPropertyId::LightSpotPenumbraAngle, tr("Spot Penumbra Angle"), tr("Angle in degrees over which the intensity of the spot light falls off to zero"), 0.0f, 179.9f);
	AddFloatProperty(lcObjectPropertyId::LightSpotTightness, tr("Spot Tightness"), tr("Additional exponential spot light edge softening (POV-Ray only)"), 0.0f, FLT_MAX);

	AddFloatProperty(lcObjectPropertyId::LightDirectionalSize, tr("Angle"), tr("Angular diameter of the light (Blender only)"), 0.0f, 180.0f);

	AddStringListProperty(lcObjectPropertyId::LightAreaShape, tr("Area Shape"), tr("The shape of the area light"), lcLight::GetAreaShapeStrings());
	AddFloatProperty(lcObjectPropertyId::LightAreaSizeX, tr("Size X"), tr("The width of the area light"), 0.0f, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::LightAreaSizeY, tr("Y"), tr("The height of the area light"), 0.0f, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::LightAreaSize, tr("Size"), tr("The size of the area light"), 0.0f, FLT_MAX);
	AddIntegerProperty(lcObjectPropertyId::LightAreaGridX, tr("Grid X"), tr("Number of point sources along the X axis (POV-Ray only)"), 1, INT_MAX);
	AddIntegerProperty(lcObjectPropertyId::LightAreaGridY, tr("Y"), tr("Number of point sources along the Y axis (POV-Ray only)"), 1, INT_MAX);

	AddCategory(CategoryIndex::ObjectTransform, tr("Transform"));

	AddFloatProperty(lcObjectPropertyId::ObjectPositionX, tr("Position X"), tr("Position of the object"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::ObjectPositionY, tr("Y"), tr("Position of the object"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::ObjectPositionZ, tr("Z"), tr("Position of the object"), -FLT_MAX, FLT_MAX);

	AddSpacing();

	AddFloatProperty(lcObjectPropertyId::ObjectRotationX, tr("Rotation X"), tr("Rotation of the object in degrees"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::ObjectRotationY, tr("Y"), tr("Rotation of the object in degrees"), -FLT_MAX, FLT_MAX);
	AddFloatProperty(lcObjectPropertyId::ObjectRotationZ, tr("Z"), tr("Rotation of the object in degrees"), -FLT_MAX, FLT_MAX);

	mLayout->setRowStretch(mLayout->rowCount(), 1);
}

void lcPropertiesWidget::SetLayoutMode(LayoutMode Mode)
{
	if (mLayoutMode == Mode)
		return;

	mLayoutMode = Mode;
	const bool IsPiece = (mLayoutMode == LayoutMode::Piece);
	const bool IsCamera = (mLayoutMode == LayoutMode::Camera);
	const bool IsLight = (mLayoutMode == LayoutMode::Light);

	SetCategoryVisible(CategoryIndex::Piece, IsPiece);
	SetCategoryVisible(CategoryIndex::Camera, IsCamera);
	SetCategoryVisible(CategoryIndex::CameraTransform, IsCamera);
	SetCategoryVisible(CategoryIndex::Light, IsLight);
	SetCategoryVisible(CategoryIndex::ObjectTransform, IsPiece || IsLight);
}

void lcPropertiesWidget::SetCategoryWidgetsVisible(CategoryWidgets& Category, bool Visible)
{
	for (lcObjectPropertyId PropertyId : Category.Properties)
		SetPropertyVisible(PropertyId, Visible);

	for (int Row : Category.SpacingRows)
		mLayout->setRowMinimumHeight(Row, Visible ? 5 : 0);
}

void lcPropertiesWidget::SetPropertyVisible(lcObjectPropertyId PropertyId, bool Visible)
{
	PropertyWidgets& Property = mPropertyWidgets[static_cast<int>(PropertyId)];

	if (Property.Label)
		Property.Label->setVisible(Visible);

	if (Property.Widget)
		Property.Widget->setVisible(Visible);
}

void lcPropertiesWidget::SetCategoryVisible(CategoryIndex Index, bool Visible)
{
	CategoryWidgets& Category = mCategoryWidgets[static_cast<int>(Index)];

	Category.Button->setVisible(Visible);

	SetCategoryWidgetsVisible(Category, Visible && Category.Button->IsExpanded());
}

void lcPropertiesWidget::SetEmpty()
{
	SetLayoutMode(LayoutMode::Empty);

	mFocusObject = nullptr;
}

void lcPropertiesWidget::SetPiece(const lcArray<lcObject*>& Selection, lcObject* Focus)
{
	SetLayoutMode(LayoutMode::Piece);

	lcPiece* Piece = dynamic_cast<lcPiece*>(Focus);
	mFocusObject = Piece;

	lcVector3 Position;
	lcMatrix33 RelativeRotation;
	lcModel* Model = gMainWindow->GetActiveModel();

	if (Model)
		Model->GetMoveRotateTransform(Position, RelativeRotation);

	UpdateFloat(lcObjectPropertyId::ObjectPositionX, Position[0]);
	UpdateFloat(lcObjectPropertyId::ObjectPositionY, Position[1]);
	UpdateFloat(lcObjectPropertyId::ObjectPositionZ, Position[2]);

	lcVector3 Rotation;

	if (Piece)
		Rotation = lcMatrix44ToEulerAngles(Piece->mModelWorld) * LC_RTOD;
	else
		Rotation = lcVector3(0.0f, 0.0f, 0.0f);

	UpdateFloat(lcObjectPropertyId::ObjectRotationX, Rotation[0]);
	UpdateFloat(lcObjectPropertyId::ObjectRotationY, Rotation[1]);
	UpdateFloat(lcObjectPropertyId::ObjectRotationZ, Rotation[2]);

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

	UpdatePieceId(lcObjectPropertyId::PieceId, Info ? Info->m_strDescription : QString());
	UpdatePieceColor(lcObjectPropertyId::PieceColor, ColorIndex);
	UpdateStepNumber(lcObjectPropertyId::PieceStepShow, StepShow ? StepShow : LC_STEP_MAX, 1, StepHide - 1);
	UpdateStepNumber(lcObjectPropertyId::PieceStepHide, StepHide ? StepHide : LC_STEP_MAX, StepShow + 1, LC_STEP_MAX);
}

void lcPropertiesWidget::SetCamera(lcObject* Focus)
{
	SetLayoutMode(LayoutMode::Camera);

	lcCamera* Camera = dynamic_cast<lcCamera*>(Focus);
	mFocusObject = Camera;

	lcVector3 Position(0.0f, 0.0f, 0.0f);
	lcVector3 Target(0.0f, 0.0f, 0.0f);
	lcVector3 UpVector(0.0f, 0.0f, 0.0f);
	bool Ortho = false;
	float FoV = 60.0f;
	float ZNear = 1.0f;
	float ZFar = 100.0f;
	QString Name;

	if (Camera)
	{
		Position = Camera->mPosition;
		Target = Camera->mTargetPosition;
		UpVector = Camera->mUpVector;

		Ortho = Camera->IsOrtho();
		FoV = Camera->m_fovy;
		ZNear = Camera->m_zNear;
		ZFar = Camera->m_zFar;
		Name = Camera->GetName();
	}

	UpdateString(lcObjectPropertyId::CameraName, Name);
	UpdateStringList(lcObjectPropertyId::CameraType, Ortho ? 1 : 0);

	UpdateFloat(lcObjectPropertyId::CameraFOV, FoV);
	UpdateFloat(lcObjectPropertyId::CameraNear, ZNear);
	UpdateFloat(lcObjectPropertyId::CameraFar, ZFar);

	UpdateFloat(lcObjectPropertyId::CameraPositionX, Position[0]);
	UpdateFloat(lcObjectPropertyId::CameraPositionY, Position[1]);
	UpdateFloat(lcObjectPropertyId::CameraPositionZ, Position[2]);

	UpdateFloat(lcObjectPropertyId::CameraTargetX, Target[0]);
	UpdateFloat(lcObjectPropertyId::CameraTargetY, Target[1]);
	UpdateFloat(lcObjectPropertyId::CameraTargetZ, Target[2]);

	UpdateFloat(lcObjectPropertyId::CameraUpX, UpVector[0]);
	UpdateFloat(lcObjectPropertyId::CameraUpY, UpVector[1]);
	UpdateFloat(lcObjectPropertyId::CameraUpZ, UpVector[2]);
}

void lcPropertiesWidget::SetLight(lcObject* Focus)
{
	SetLayoutMode(LayoutMode::Light);

	lcLight* Light = dynamic_cast<lcLight*>(Focus);
	mFocusObject = Light;

	QString Name;
	lcLightType LightType = lcLightType::Point;
	QColor Color(Qt::white);
	lcLightAreaShape LightAreaShape = lcLightAreaShape::Rectangle;
	lcVector2 LightSize(0.0f, 0.0f);
	lcVector2i AreaGrid(2, 2);
	float Power = 0.0f;
	float AttenuationDistance = 0.0f;
	float AttenuationPower = 0.0f;
	bool CastShadow = true;
	lcVector3 Position(0.0f, 0.0f, 0.0f);
	lcVector3 Rotation = lcVector3(0.0f, 0.0f, 0.0f);
	float SpotConeAngle = 0.0f, SpotPenumbraAngle = 0.0f, SpotTightness = 0.0f;

	if (Light)
	{
		Name = Light->GetName();
		LightType = Light->GetLightType();
		Color = lcQColorFromVector3(Light->GetColor());

		CastShadow = Light->GetCastShadow();
		Position = Light->GetPosition();
		Rotation = lcMatrix44ToEulerAngles(Light->GetWorldMatrix()) * LC_RTOD;
		Power = Light->GetPower();
		AttenuationDistance = Light->GetAttenuationDistance();
		AttenuationPower = Light->GetAttenuationPower();
		SpotConeAngle = Light->GetSpotConeAngle();
		SpotPenumbraAngle = Light->GetSpotPenumbraAngle();
		SpotTightness = Light->GetSpotTightness();

		LightAreaShape = Light->GetAreaShape();
		LightSize = Light->GetSize();
		AreaGrid = Light->GetAreaGrid();
	}

	UpdateString(lcObjectPropertyId::LightName, Name);
	UpdateStringList(lcObjectPropertyId::LightType, static_cast<int>(LightType));
	UpdateColor(lcObjectPropertyId::LightColor, Color);

	UpdateFloat(lcObjectPropertyId::LightPower, Power);
	UpdateBool(lcObjectPropertyId::LightCastShadow, CastShadow);

	UpdateFloat(lcObjectPropertyId::LightAttenuationDistance, AttenuationDistance);
	UpdateFloat(lcObjectPropertyId::LightAttenuationPower, AttenuationPower);

	const bool IsPointLight = Light && Light->IsPointLight();
	SetPropertyVisible(lcObjectPropertyId::LightPointSize, IsPointLight);

	if (IsPointLight)
		UpdateFloat(lcObjectPropertyId::LightPointSize, LightSize.x);

	const bool IsSpotLight = Light && Light->IsSpotLight();
	SetPropertyVisible(lcObjectPropertyId::LightSpotSize, IsSpotLight);
	SetPropertyVisible(lcObjectPropertyId::LightSpotConeAngle, IsSpotLight);
	SetPropertyVisible(lcObjectPropertyId::LightSpotPenumbraAngle, IsSpotLight);
	SetPropertyVisible(lcObjectPropertyId::LightSpotTightness, IsSpotLight);

	if (IsSpotLight)
	{
		UpdateFloat(lcObjectPropertyId::LightSpotSize, LightSize.x);
		UpdateFloat(lcObjectPropertyId::LightSpotConeAngle, SpotConeAngle);
		UpdateFloat(lcObjectPropertyId::LightSpotPenumbraAngle, SpotPenumbraAngle);
		UpdateFloat(lcObjectPropertyId::LightSpotTightness, SpotTightness);
	}

	const bool IsDirectionalLight = Light && Light->IsDirectionalLight();
	SetPropertyVisible(lcObjectPropertyId::LightDirectionalSize, IsDirectionalLight);

	if (IsDirectionalLight)
		UpdateFloat(lcObjectPropertyId::LightDirectionalSize, LightSize.x);

	const bool IsAreaLight = Light && Light->IsAreaLight();
	SetPropertyVisible(lcObjectPropertyId::LightAreaShape, IsAreaLight);

	const bool IsSquare = IsAreaLight && (LightAreaShape == lcLightAreaShape::Square || LightAreaShape == lcLightAreaShape::Disk);
	SetPropertyVisible(lcObjectPropertyId::LightAreaSize, IsSquare);
	SetPropertyVisible(lcObjectPropertyId::LightAreaSizeX, !IsSquare);
	SetPropertyVisible(lcObjectPropertyId::LightAreaSizeY, !IsSquare);

	SetPropertyVisible(lcObjectPropertyId::LightAreaGridX, IsAreaLight);
	SetPropertyVisible(lcObjectPropertyId::LightAreaGridY, IsAreaLight);

	if (IsAreaLight)
	{
		UpdateStringList(lcObjectPropertyId::LightAreaShape, static_cast<int>(LightAreaShape));
		UpdateFloat(lcObjectPropertyId::LightAreaSize, LightSize.x);
		UpdateFloat(lcObjectPropertyId::LightAreaSizeX, LightSize.x);
		UpdateFloat(lcObjectPropertyId::LightAreaSizeY, LightSize.y);
		UpdateInteger(lcObjectPropertyId::LightAreaGridX, AreaGrid.x);
		UpdateInteger(lcObjectPropertyId::LightAreaGridY, AreaGrid.y);
	}

	UpdateFloat(lcObjectPropertyId::ObjectPositionX, Position[0]);
	UpdateFloat(lcObjectPropertyId::ObjectPositionY, Position[1]);
	UpdateFloat(lcObjectPropertyId::ObjectPositionZ, Position[2]);

	UpdateFloat(lcObjectPropertyId::ObjectRotationX, Rotation[0]);
	UpdateFloat(lcObjectPropertyId::ObjectRotationY, Rotation[1]);
	UpdateFloat(lcObjectPropertyId::ObjectRotationZ, Rotation[2]);
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
	case LayoutMode::Multiple:
	case LayoutMode::Count:
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
	}
}

#include "lc_global.h"
#include "lc_qpropertiestree.h"
#include "lc_qcolorpicker.h"
#include "lc_application.h"
#include "project.h"
#include "lc_model.h"
#include "object.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "pieceinf.h"
#include "lc_library.h"
#include "lc_qutils.h"

// Draw an icon indicating opened/closing branches
static QIcon drawIndicatorIcon(const QPalette &palette, QStyle *style)
{
	QPixmap pix(14, 14);
	pix.fill(Qt::transparent);
	QStyleOption branchOption;
	branchOption.rect = QRect(2, 2, 9, 9); // ### hardcoded in qcommonstyle.cpp
	branchOption.palette = palette;
	branchOption.state = QStyle::State_Children;

	QPainter p;
	// Draw closed state
	p.begin(&pix);
	style->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, &p);
	p.end();
	QIcon rc = pix;
	rc.addPixmap(pix, QIcon::Selected, QIcon::Off);
	// Draw opened state
	branchOption.state |= QStyle::State_Open;
	pix.fill(Qt::transparent);
	p.begin(&pix);
	style->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, &p);
	p.end();

	rc.addPixmap(pix, QIcon::Normal, QIcon::On);
	rc.addPixmap(pix, QIcon::Selected, QIcon::On);
	return rc;
}

static QIcon drawCheckBox(bool value)
{
	QStyleOptionButton opt;
	opt.state |= value ? QStyle::State_On : QStyle::State_Off;
	opt.state |= QStyle::State_Enabled;
	const QStyle *style = QApplication::style();
	// Figure out size of an indicator and make sure it is not scaled down in a list view item
	// by making the pixmap as big as a list view icon and centering the indicator in it.
	// (if it is smaller, it can't be helped)
	const int indicatorWidth = style->pixelMetric(QStyle::PM_IndicatorWidth, &opt);
	const int indicatorHeight = style->pixelMetric(QStyle::PM_IndicatorHeight, &opt);
	const int listViewIconSize = indicatorWidth;
	const int pixmapWidth = indicatorWidth;
	const int pixmapHeight = qMax(indicatorHeight, listViewIconSize);

	opt.rect = QRect(0, 0, indicatorWidth, indicatorHeight);
	QPixmap pixmap = QPixmap(pixmapWidth, pixmapHeight);
	pixmap.fill(Qt::transparent);
	{
		// Center?
		const int xoff = (pixmapWidth  > indicatorWidth)  ? (pixmapWidth  - indicatorWidth)  / 2 : 0;
		const int yoff = (pixmapHeight > indicatorHeight) ? (pixmapHeight - indicatorHeight) / 2 : 0;
		QPainter painter(&pixmap);
		painter.translate(xoff, yoff);
		style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &opt, &painter);
	}
	return QIcon(pixmap);
}

int lcQPropertiesTreeDelegate::indentation(const QModelIndex &index) const
{
	if (!m_treeWidget)
		return 0;

	QTreeWidgetItem *item = m_treeWidget->indexToItem(index);
	int indent = 0;
	while (item->parent())
	{
		item = item->parent();
		++indent;
	}

	if (m_treeWidget->rootIsDecorated())
		++indent;

	return indent * m_treeWidget->indentation();
}

void lcQPropertiesTreeDelegate::slotEditorDestroyed(QObject *object)
{
	if (m_editedWidget == object)
	{
		m_editedWidget = NULL;
		m_editedItem = NULL;
	}
}

QWidget *lcQPropertiesTreeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &style, const QModelIndex &index) const
{
	Q_UNUSED(style);

	if (index.column() == 1 && m_treeWidget)
	{
		QTreeWidgetItem *item = m_treeWidget->indexToItem(index);

		if (item && (item->flags() & Qt::ItemIsEnabled))
		{
			QWidget *editor = m_treeWidget->createEditor(parent, item);
			if (editor)
			{
				editor->setAutoFillBackground(true);
				editor->installEventFilter(const_cast<lcQPropertiesTreeDelegate *>(this));

				m_editedItem = item;
				m_editedWidget = editor;

				connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));

				return editor;
			}
		}
	}

	return NULL;
}

void lcQPropertiesTreeDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(index)
	editor->setGeometry(option.rect.adjusted(0, 0, 0, -1));
}

void lcQPropertiesTreeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	bool hasValue = true;
	if (m_treeWidget)
		hasValue = m_treeWidget->indexToItem(index)->data(0, lcQPropertiesTree::PropertyTypeRole).toInt() != lcQPropertiesTree::PropertyGroup;

	QStyleOptionViewItemV3 opt = option;

	opt.state &= ~QStyle::State_HasFocus;

	if (index.column() == 1)
	{
		QTreeWidgetItem *item = m_treeWidget->indexToItem(index);
		if (m_editedItem && m_editedItem == item)
			m_disablePainting = true;
	}

	QItemDelegate::paint(painter, opt, index);
	if (option.type)
		m_disablePainting = false;

	opt.palette.setCurrentColorGroup(QPalette::Active);
	QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));
	painter->save();
	painter->setPen(QPen(color));
 
	if (!m_treeWidget || (!m_treeWidget->lastColumn(index.column()) && hasValue))
	{
		int right = (option.direction == Qt::LeftToRight) ? option.rect.right() : option.rect.left();
		painter->drawLine(right, option.rect.y(), right, option.rect.bottom());
	}

	painter->restore();
}

void lcQPropertiesTreeDelegate::drawDecoration(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QPixmap &pixmap) const
{
	if (m_disablePainting)
		return;

	QItemDelegate::drawDecoration(painter, option, rect, pixmap);
}

void lcQPropertiesTreeDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const
{
	if (m_disablePainting)
		return;

	QItemDelegate::drawDisplay(painter, option, rect, text);
}

QSize lcQPropertiesTreeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return QItemDelegate::sizeHint(option, index) + QSize(3, 4);
}

bool lcQPropertiesTreeDelegate::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::FocusOut)
	{
		QFocusEvent *fe = static_cast<QFocusEvent *>(event);
		if (fe->reason() == Qt::ActiveWindowFocusReason)
			return false;
	}
	return QItemDelegate::eventFilter(object, event);
}

lcQPropertiesTree::lcQPropertiesTree(QWidget *parent) :
	QTreeWidget(parent)
{
	setIconSize(QSize(18, 18));
	setColumnCount(2);
	QStringList labels;
	labels.append(tr("Property"));
	labels.append(tr("Value"));
	setHeaderLabels(labels);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	header()->setSectionsMovable(false);
	header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
	header()->setMovable(false);
	header()->setResizeMode(QHeaderView::ResizeToContents);
#endif
	header()->setVisible(false);
	setAlternatingRowColors(true);
	setRootIsDecorated(false);
	setEditTriggers(QAbstractItemView::EditKeyPressed);

	m_expandIcon = drawIndicatorIcon(palette(), style());
	m_checkedIcon = drawCheckBox(true);
	m_uncheckedIcon = drawCheckBox(false);

	m_delegate = new lcQPropertiesTreeDelegate(parent);
	m_delegate->setTreeWidget(this);
	setItemDelegate(m_delegate);

	SetEmpty();

	connect(header(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(resizeColumnToContents(int)));
}

QSize lcQPropertiesTree::sizeHint() const
{
	return QSize(200, -1);
}

void lcQPropertiesTree::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionViewItemV3 opt = option;

	QTreeWidgetItem *item = itemFromIndex(index);

	if (item->data(0, lcQPropertiesTree::PropertyTypeRole).toInt() == lcQPropertiesTree::PropertyGroup)
	{
		const QColor c = option.palette.color(QPalette::Dark);
		painter->fillRect(option.rect, c);
		opt.palette.setColor(QPalette::AlternateBase, c);
	}

	QTreeWidget::drawRow(painter, opt, index);
	QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));
	painter->save();
	painter->setPen(QPen(color));
	painter->drawLine(opt.rect.x(), opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
	painter->restore();
}

void lcQPropertiesTree::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
	case Qt::Key_Return:
	case Qt::Key_Enter:
	case Qt::Key_Space: // Trigger Edit
		if (!m_delegate->editedItem())
		{
			if (const QTreeWidgetItem *item = currentItem())
			{
				if (item->columnCount() >= 2 && ((item->flags() & (Qt::ItemIsEditable | Qt::ItemIsEnabled)) == (Qt::ItemIsEditable | Qt::ItemIsEnabled)))
				{
					event->accept();
					// If the current position is at column 0, move to 1.
					QModelIndex index = currentIndex();
					if (index.column() == 0)
					{
						index = index.sibling(index.row(), 1);
						setCurrentIndex(index);
					}
					edit(index);
					return;
				}
			}
		}
		break;

	default:
		break;
	}

	QTreeWidget::keyPressEvent(event);
}

void lcQPropertiesTree::mousePressEvent(QMouseEvent *event)
{
	QTreeWidget::mousePressEvent(event);
	QTreeWidgetItem *item = itemAt(event->pos());

	if (item)
	{
		if ((item != m_delegate->editedItem()) && (event->button() == Qt::LeftButton) && (header()->logicalIndexAt(event->pos().x()) == 1) &&
		    ((item->flags() & (Qt::ItemIsEditable | Qt::ItemIsEnabled)) == (Qt::ItemIsEditable | Qt::ItemIsEnabled)))
			editItem(item, 1);
	}
}

void lcQPropertiesTree::Update(const lcArray<lcObject*>& Selection, lcObject* Focus)
{
	lcPropertyWidgetMode Mode = LC_PROPERTY_WIDGET_EMPTY;

	if (Focus)
	{
		switch (Focus->GetType())
		{
		case LC_OBJECT_PIECE:
			Mode = LC_PROPERTY_WIDGET_PIECE;
			break;

		case LC_OBJECT_CAMERA:
			Mode = LC_PROPERTY_WIDGET_CAMERA;
			break;

		case LC_OBJECT_LIGHT:
			Mode = LC_PROPERTY_WIDGET_LIGHT;
			break;
		}
	}
	else
	{
		for (int ObjectIdx = 0; ObjectIdx < Selection.GetSize(); ObjectIdx++)
		{
			switch (Selection[ObjectIdx]->GetType())
			{
			case LC_OBJECT_PIECE:
				if (Mode == LC_PROPERTY_WIDGET_EMPTY)
					Mode = LC_PROPERTY_WIDGET_PIECE;
				else if (Mode != LC_PROPERTY_WIDGET_PIECE)
				{
					Mode = LC_PROPERTY_WIDGET_MULTIPLE;
					ObjectIdx = Selection.GetSize();
				}
				break;

			case LC_OBJECT_CAMERA:
				if (Mode != LC_PROPERTY_WIDGET_EMPTY)
				{
					Mode = LC_PROPERTY_WIDGET_MULTIPLE;
					ObjectIdx = Selection.GetSize();
				}
				else
				{
					Mode = LC_PROPERTY_WIDGET_CAMERA;
					Focus = Selection[ObjectIdx];
				}
				break;

			case LC_OBJECT_LIGHT:
				if (Mode != LC_PROPERTY_WIDGET_EMPTY)
				{
					Mode = LC_PROPERTY_WIDGET_MULTIPLE;
					ObjectIdx = Selection.GetSize();
				}
				else
				{
					Mode = LC_PROPERTY_WIDGET_LIGHT;
					Focus = Selection[ObjectIdx];
				}
				break;
			}
		}
	}

	switch (Mode)
	{
	case LC_PROPERTY_WIDGET_EMPTY:
		SetEmpty();
		break;

	case LC_PROPERTY_WIDGET_PIECE:
		SetPiece(Selection, Focus);
		break;

	case LC_PROPERTY_WIDGET_CAMERA:
		SetCamera(Focus);
		break;

	case LC_PROPERTY_WIDGET_LIGHT:
		SetLight(Focus);
		break;

	case LC_PROPERTY_WIDGET_MULTIPLE:
		SetMultiple();
		break;
	}
}

QWidget *lcQPropertiesTree::createEditor(QWidget *parent, QTreeWidgetItem *item) const
{
	PropertyType propertyType = (PropertyType)item->data(0, lcQPropertiesTree::PropertyTypeRole).toInt();

	switch (propertyType)
	{
	case PropertyGroup:
		return NULL;

	case PropertyBool:
		{
			QCheckBox *editor = new QCheckBox(parent);
			bool value = item->data(0, PropertyValueRole).toBool();

			editor->setChecked(value);

			connect(editor, SIGNAL(toggled(bool)), this, SLOT(slotToggled(bool)));

			return editor;
		}

	case PropertyFloat:
		{
			QLineEdit *editor = new QLineEdit(parent);
			float value = item->data(0, PropertyValueRole).toFloat();

			editor->setValidator(new QDoubleValidator());
			editor->setText(lcFormatValue(value));

			connect(editor, SIGNAL(returnPressed()), this, SLOT(slotReturnPressed()));

			return editor;
		}

	case PropertyInt:
		{
			QLineEdit *editor = new QLineEdit(parent);
			lcuint32 value = item->data(0, PropertyValueRole).toUInt();

			editor->setValidator(new QIntValidator());
			editor->setText(QString::number(value));

			connect(editor, SIGNAL(returnPressed()), this, SLOT(slotReturnPressed()));

			return editor;
		}

	case PropertyString:
		{
			QLineEdit *editor = new QLineEdit(parent);
			const char *value = (const char*)item->data(0, PropertyValueRole).value<void*>();

			editor->setText(value);

			connect(editor, SIGNAL(returnPressed()), this, SLOT(slotReturnPressed()));

			return editor;
		}

	case PropertyColor:
		{
			QPushButton *editor = new QPushButton(parent);
			int value = item->data(0, PropertyValueRole).toInt();

			updateColorEditor(editor, value);

			connect(editor, SIGNAL(clicked()), this, SLOT(slotColorButtonClicked()));

			return editor;
		}

	case PropertyPart:
		{
			QComboBox *editor = new QComboBox(parent);

			editor->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
			editor->setMinimumContentsLength(1);

			lcPiecesLibrary* library = lcGetPiecesLibrary();
			for (int partIdx = 0; partIdx < library->mPieces.GetSize(); partIdx++)
				editor->addItem(library->mPieces[partIdx]->m_strDescription, qVariantFromValue((void*)library->mPieces[partIdx]));
			editor->model()->sort(0);

			PieceInfo *info = (PieceInfo*)item->data(0, PropertyValueRole).value<void*>();
			editor->setCurrentIndex(editor->findData(qVariantFromValue((void*)info)));

			connect(editor, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSetValue(int)));

			return editor;
		}
	}

	return NULL;
}

void lcQPropertiesTree::updateColorEditor(QPushButton *editor, int value) const
{
	QImage img(12, 12, QImage::Format_ARGB32);
	img.fill(0);

	lcColor* color = &gColorList[value];
	QRgb rgb = qRgb(color->Value[0] * 255, color->Value[1] * 255, color->Value[2] * 255);
	QBrush b(rgb);
	QPainter painter(&img);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(0, 0, img.width(), img.height(), b);
	painter.end();

	editor->setStyleSheet("Text-align:left");
	editor->setIcon(QPixmap::fromImage(img));
	editor->setText(color->Name);
}

void lcQPropertiesTree::slotToggled(bool Value)
{
	QTreeWidgetItem* Item = m_delegate->editedItem();
	lcModel* Model = lcGetActiveModel();

	if (mWidgetMode == LC_PROPERTY_WIDGET_CAMERA)
	{
		lcObject* Focus = Model->GetFocusObject();

		if (Focus && Focus->IsCamera())
		{
			lcCamera* Camera = (lcCamera*)Focus;

			if (Item == cameraOrtho)
			{
				Model->SetCameraOrthographic(Camera, Value);
			}
		}
	}
}

void lcQPropertiesTree::slotReturnPressed()
{
	QLineEdit* Editor = (QLineEdit*)sender();
	QTreeWidgetItem* Item = m_delegate->editedItem();
	lcModel* Model = lcGetActiveModel();

	if (mWidgetMode == LC_PROPERTY_WIDGET_PIECE)
	{
		lcPiece* Piece = (mFocus && mFocus->IsPiece()) ? (lcPiece*)mFocus : NULL;

		if (Item == partPositionX || Item == partPositionY || Item == partPositionZ)
		{
			lcVector3 Center;
			lcMatrix33 RelativeRotation;
			Model->GetMoveRotateTransform(Center, RelativeRotation);
			lcVector3 Position = Center;
			float Value = Editor->text().toFloat();

			if (Item == partPositionX)
				Position[0] = Value;
			else if (Item == partPositionY)
				Position[1] = Value;
			else if (Item == partPositionZ)
				Position[2] = Value;

			lcVector3 Distance = Position - Center;

			Model->MoveSelectedObjects(Distance, Distance, true, false, true, true);
		}
		else if (Item == partRotationX || Item == partRotationY || Item == partRotationZ)
		{
			lcVector3 InitialRotation;
			if (Piece)
				InitialRotation = lcMatrix44ToEulerAngles(Piece->mModelWorld) * LC_RTOD;
			else
				InitialRotation = lcVector3(0.0f, 0.0f, 0.0f);
			lcVector3 Rotation = InitialRotation;

			float Value = Editor->text().toFloat();

			if (Item == partRotationX)
				Rotation[0] = Value;
			else if (Item == partRotationY)
				Rotation[1] = Value;
			else if (Item == partRotationZ)
				Rotation[2] = Value;

			Model->RotateSelectedPieces(Rotation - InitialRotation, true, false, true, true);
		}
		else if (Item == partShow)
		{
			lcStep Step = Editor->text().toUInt();

			Model->SetSelectedPiecesStepShow(Step);
		}
		else if (Item == partHide)
		{
			lcStep Step = Editor->text().toUInt();

			Model->SetSelectedPiecesStepHide(Step);
		}
	}
	else if (mWidgetMode == LC_PROPERTY_WIDGET_CAMERA)
	{
		lcCamera* Camera = (mFocus && mFocus->IsCamera()) ? (lcCamera*)mFocus : NULL;

		if (Camera)
		{
			if (Item == cameraPositionX || Item == cameraPositionY || Item == cameraPositionZ)
			{
				lcVector3 Center = Camera->mPosition;
				lcVector3 Position = Center;
				float Value = Editor->text().toFloat();

				if (Item == cameraPositionX)
					Position[0] = Value;
				else if (Item == cameraPositionY)
					Position[1] = Value;
				else if (Item == cameraPositionZ)
					Position[2] = Value;

				lcVector3 Distance = Position - Center;

				Model->MoveSelectedObjects(Distance, Distance, true, false, true, true);
			}
			else if (Item == cameraTargetX || Item == cameraTargetY || Item == cameraTargetZ)
			{
				lcVector3 Center = Camera->mTargetPosition;
				lcVector3 Position = Center;
				float Value = Editor->text().toFloat();

				if (Item == cameraTargetX)
					Position[0] = Value;
				else if (Item == cameraTargetY)
					Position[1] = Value;
				else if (Item == cameraTargetZ)
					Position[2] = Value;

				lcVector3 Distance = Position - Center;

				Model->MoveSelectedObjects(Distance, Distance, true, false, true, true);
			}
			else if (Item == cameraUpX || Item == cameraUpY || Item == cameraUpZ)
			{
				lcVector3 Center = Camera->mUpVector;
				lcVector3 Position = Center;
				float Value = Editor->text().toFloat();

				if (Item == cameraUpX)
					Position[0] = Value;
				else if (Item == cameraUpY)
					Position[1] = Value;
				else if (Item == cameraUpZ)
					Position[2] = Value;

				lcVector3 Distance = Position - Center;

				Model->MoveSelectedObjects(Distance, Distance, true, false, true, true);
			}
			else if (Item == cameraFOV)
			{
				float Value = Editor->text().toFloat();

				Model->SetCameraFOV(Camera, Value);
			}
			else if (Item == cameraNear)
			{
				float Value = Editor->text().toFloat();

				Model->SetCameraZNear(Camera, Value);
			}
			else if (Item == cameraFar)
			{
				float Value = Editor->text().toFloat();

				Model->SetCameraZFar(Camera, Value);
			}
			else if (Item == cameraName)
			{
				QString Value = Editor->text();

				Model->SetCameraName(Camera, Value.toLocal8Bit().data());
			}
		}
	}
}

void lcQPropertiesTree::slotSetValue(int Value)
{
	QTreeWidgetItem* Item = m_delegate->editedItem();
	lcModel* Model = lcGetActiveModel();

	if (mWidgetMode == LC_PROPERTY_WIDGET_PIECE)
	{
		if (Item == partColor)
		{
			Model->SetSelectedPiecesColorIndex(Value);

			QPushButton *editor = (QPushButton*)m_delegate->editor();
			updateColorEditor(editor, Value);
		}
		else if (Item == partID)
		{
			QComboBox *editor = (QComboBox*)sender();

			Model->SetSelectedPiecesPieceInfo((PieceInfo*)editor->itemData(Value).value<void*>());
		}
	}
}

void lcQPropertiesTree::slotColorButtonClicked()
{
	int ColorIndex = gDefaultColor;
	lcObject* Focus = lcGetActiveModel()->GetFocusObject();
	if (Focus && Focus->IsPiece())
		ColorIndex = ((lcPiece*)Focus)->mColorIndex;

	QWidget *parent = (QWidget*)sender();
	lcQColorPickerPopup *popup = new lcQColorPickerPopup(parent, ColorIndex);
	connect(popup, SIGNAL(selected(int)), SLOT(slotSetValue(int)));
	popup->setMinimumSize(300, 200);

	const QRect desktop = QApplication::desktop()->geometry();

	QPoint pos = parent->mapToGlobal(parent->rect().bottomLeft());
	if (pos.x() < desktop.left())
		pos.setX(desktop.left());
	if (pos.y() < desktop.top())
		pos.setY(desktop.top());

	if ((pos.x() + popup->width()) > desktop.width())
		pos.setX(desktop.width() - popup->width());
	if ((pos.y() + popup->height()) > desktop.bottom())
		pos.setY(desktop.bottom() - popup->height());
	popup->move(pos);

	popup->setFocus();
	popup->show();
}

QTreeWidgetItem *lcQPropertiesTree::addProperty(QTreeWidgetItem *parent, const QString& label, PropertyType propertyType)
{
	QTreeWidgetItem *newItem;

	if (parent)
		newItem = new QTreeWidgetItem(parent, QStringList(label));
	else
		newItem = new QTreeWidgetItem(this, QStringList(label));

	newItem->setData(0, PropertyTypeRole, QVariant(propertyType));
	newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
	setItemExpanded(newItem, true);

	if (propertyType == PropertyGroup)
	{
		newItem->setFirstColumnSpanned(true);
		newItem->setIcon(0, m_expandIcon);
	}

	return newItem;
}

void lcQPropertiesTree::SetEmpty()
{
	clear();

	partPosition = NULL;
	partPositionX = NULL;
	partPositionY = NULL;
	partPositionZ = NULL;
	partRotation = NULL;
	partRotationX = NULL;
	partRotationY = NULL;
	partRotationZ = NULL;
	partVisibility = NULL;
	partShow = NULL;
	partHide = NULL;
	partAppearance = NULL;
	partColor = NULL;
	partID = NULL;

	cameraPosition = NULL;
	cameraPositionX = NULL;
	cameraPositionY = NULL;
	cameraPositionZ = NULL;
	cameraTarget = NULL;
	cameraTargetX = NULL;
	cameraTargetY = NULL;
	cameraTargetZ = NULL;
	cameraUp = NULL;
	cameraUpX = NULL;
	cameraUpY = NULL;
	cameraUpZ = NULL;
	cameraSettings = NULL;
	cameraOrtho = NULL;
	cameraFOV = NULL;
	cameraNear = NULL;
	cameraFar = NULL;
	cameraName = NULL;

	mWidgetMode = LC_PROPERTY_WIDGET_EMPTY;
	mFocus = NULL;
}

void lcQPropertiesTree::SetPiece(const lcArray<lcObject*>& Selection, lcObject* Focus)
{
	if (mWidgetMode != LC_PROPERTY_WIDGET_PIECE)
	{
		SetEmpty();

		partPosition = addProperty(NULL, tr("Position"), PropertyGroup);
		partPositionX = addProperty(partPosition, tr("X"), PropertyFloat);
		partPositionY = addProperty(partPosition, tr("Y"), PropertyFloat);
		partPositionZ = addProperty(partPosition, tr("Z"), PropertyFloat);

		partRotation = addProperty(NULL, tr("Rotation"), PropertyGroup);
		partRotationX = addProperty(partRotation, tr("X"), PropertyFloat);
		partRotationY = addProperty(partRotation, tr("Y"), PropertyFloat);
		partRotationZ = addProperty(partRotation, tr("Z"), PropertyFloat);

		partVisibility = addProperty(NULL, tr("Visibility"), PropertyGroup);
		partShow = addProperty(partVisibility, tr("Show"), PropertyInt);
		partHide = addProperty(partVisibility, tr("Hide"), PropertyInt);

		partAppearance = addProperty(NULL, tr("Appearance"), PropertyGroup);
		partColor = addProperty(partAppearance, tr("Color"), PropertyColor);
		partID = addProperty(partAppearance, tr("Part"), PropertyPart);

		mWidgetMode = LC_PROPERTY_WIDGET_PIECE;
	}

	lcModel* Model = lcGetActiveModel();
	lcPiece* Piece = (Focus && Focus->IsPiece()) ? (lcPiece*)Focus : NULL;
	mFocus = Piece;

	lcVector3 Position;
	lcMatrix33 RelativeRotation;
	Model->GetMoveRotateTransform(Position, RelativeRotation);
	partPositionX->setText(1, lcFormatValue(Position[0]));
	partPositionX->setData(0, PropertyValueRole, Position[0]);
	partPositionY->setText(1, lcFormatValue(Position[1]));
	partPositionY->setData(0, PropertyValueRole, Position[1]);
	partPositionZ->setText(1, lcFormatValue(Position[2]));
	partPositionZ->setData(0, PropertyValueRole, Position[2]);

	lcVector3 Rotation;
	if (Piece)
		Rotation = lcMatrix44ToEulerAngles(Piece->mModelWorld) * LC_RTOD;
	else
		Rotation = lcVector3(0.0f, 0.0f, 0.0f);
	partRotationX->setText(1, lcFormatValue(Rotation[0]));
	partRotationX->setData(0, PropertyValueRole, Rotation[0]);
	partRotationY->setText(1, lcFormatValue(Rotation[1]));
	partRotationY->setData(0, PropertyValueRole, Rotation[1]);
	partRotationZ->setText(1, lcFormatValue(Rotation[2]));
	partRotationZ->setData(0, PropertyValueRole, Rotation[2]);

	lcStep Show = 0;
	lcStep Hide = 0;
	int ColorIndex = gDefaultColor;
	PieceInfo* Info = NULL;

	if (Piece)
	{
		Show = Piece->GetStepShow();
		Hide = Piece->GetStepHide();
		ColorIndex = Piece->mColorIndex;
		Info = Piece->mPieceInfo;
	}
	else
	{
		bool FirstPiece = true;

		for (int ObjectIdx = 0; ObjectIdx < Selection.GetSize(); ObjectIdx++)
		{
			lcObject* Object = Selection[ObjectIdx];

			if (Object->GetType() != LC_OBJECT_PIECE)
				continue;

			lcPiece* SelectedPiece = (lcPiece*)Object;

			if (FirstPiece)
			{
				Show = SelectedPiece->GetStepShow();
				Hide = SelectedPiece->GetStepHide();
				ColorIndex = SelectedPiece->mColorIndex;
				Info = SelectedPiece->mPieceInfo;

				FirstPiece = false;
			}
			else
			{
				if (SelectedPiece->GetStepShow() != Show)
					Show = 0;

				if (SelectedPiece->GetStepHide() != Hide)
					Hide = 0;

				if (SelectedPiece->mColorIndex != ColorIndex)
					ColorIndex = gDefaultColor;

				if (SelectedPiece->mPieceInfo != Info)
					Info = NULL;
			}
		}
	}

	partShow->setText(1, QString::number(Show));
	partShow->setData(0, PropertyValueRole, Show);
	partHide->setText(1, QString::number(Hide));
	partHide->setData(0, PropertyValueRole, Hide);

	QImage img(16, 16, QImage::Format_ARGB32);
	img.fill(0);

	lcColor* color = &gColorList[ColorIndex];
	QRgb rgb = qRgb(color->Value[0] * 255, color->Value[1] * 255, color->Value[2] * 255);
	QBrush b(rgb);
	QPainter painter(&img);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(0, 0, img.width(), img.height(), b);
	painter.end();

	partColor->setIcon(1, QIcon(QPixmap::fromImage(img)));
	partColor->setText(1, color->Name);
	partColor->setData(0, PropertyValueRole, ColorIndex);

	partID->setText(1, Info ? Info->m_strDescription : QString());
	partID->setData(0, PropertyValueRole, qVariantFromValue((void*)Info));
}

void lcQPropertiesTree::SetCamera(lcObject* Focus)
{
	Q_UNUSED(Focus);

	if (mWidgetMode != LC_PROPERTY_WIDGET_CAMERA)
	{
		SetEmpty();

		cameraPosition = addProperty(NULL, tr("Position"), PropertyGroup);
		cameraPositionX = addProperty(cameraPosition, tr("X"), PropertyFloat);
		cameraPositionY = addProperty(cameraPosition, tr("Y"), PropertyFloat);
		cameraPositionZ = addProperty(cameraPosition, tr("Z"), PropertyFloat);

		cameraTarget = addProperty(NULL, tr("Target"), PropertyGroup);
		cameraTargetX = addProperty(cameraTarget, tr("X"), PropertyFloat);
		cameraTargetY = addProperty(cameraTarget, tr("Y"), PropertyFloat);
		cameraTargetZ = addProperty(cameraTarget, tr("Z"), PropertyFloat);

		cameraUp = addProperty(NULL, tr("Up"), PropertyGroup);
		cameraUpX = addProperty(cameraUp, tr("X"), PropertyFloat);
		cameraUpY = addProperty(cameraUp, tr("Y"), PropertyFloat);
		cameraUpZ = addProperty(cameraUp, tr("Z"), PropertyFloat);

		cameraSettings = addProperty(NULL, tr("Up"), PropertyGroup);
		cameraOrtho = addProperty(cameraSettings, tr("Orthographic"), PropertyBool);
		cameraFOV = addProperty(cameraSettings, tr("FOV"), PropertyFloat);
		cameraNear = addProperty(cameraSettings, tr("Near"), PropertyFloat);
		cameraFar = addProperty(cameraSettings, tr("Far"), PropertyFloat);
		cameraName = addProperty(cameraSettings, tr("Name"), PropertyString);

		mWidgetMode = LC_PROPERTY_WIDGET_CAMERA;
	}

	lcCamera* Camera = (Focus && Focus->IsCamera()) ? (lcCamera*)Focus : NULL;
	mFocus = Camera;

	lcVector3 Position(0.0f, 0.0f, 0.0f);
	lcVector3 Target(0.0f, 0.0f, 0.0f);
	lcVector3 UpVector(0.0f, 0.0f, 0.0f);
	bool Ortho = false;
	float FoV = 60.0f;
	float ZNear = 1.0f;
	float ZFar = 100.0f;
	const char* Name = "";

	if (Focus)
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

	cameraPositionX->setText(1, lcFormatValue(Position[0]));
	cameraPositionX->setData(0, PropertyValueRole, Position[0]);
	cameraPositionY->setText(1, lcFormatValue(Position[1]));
	cameraPositionY->setData(0, PropertyValueRole, Position[1]);
	cameraPositionZ->setText(1, lcFormatValue(Position[2]));
	cameraPositionZ->setData(0, PropertyValueRole, Position[2]);

	cameraTargetX->setText(1, lcFormatValue(Target[0]));
	cameraTargetX->setData(0, PropertyValueRole, Target[0]);
	cameraTargetY->setText(1, lcFormatValue(Target[1]));
	cameraTargetY->setData(0, PropertyValueRole, Target[1]);
	cameraTargetZ->setText(1, lcFormatValue(Target[2]));
	cameraTargetZ->setData(0, PropertyValueRole, Target[2]);

	cameraUpX->setText(1, lcFormatValue(UpVector[0]));
	cameraUpX->setData(0, PropertyValueRole, UpVector[0]);
	cameraUpY->setText(1, lcFormatValue(UpVector[1]));
	cameraUpY->setData(0, PropertyValueRole, UpVector[1]);
	cameraUpZ->setText(1, lcFormatValue(UpVector[2]));
	cameraUpZ->setData(0, PropertyValueRole, UpVector[2]);

	cameraOrtho->setText(1, Ortho ? "True" : "False");
	cameraOrtho->setData(0, PropertyValueRole, Ortho);
	cameraFOV->setText(1, lcFormatValue(FoV));
	cameraFOV->setData(0, PropertyValueRole, FoV);
	cameraNear->setText(1, lcFormatValue(ZNear));
	cameraNear->setData(0, PropertyValueRole, ZNear);
	cameraFar->setText(1, lcFormatValue(ZFar));
	cameraFar->setData(0, PropertyValueRole, ZFar);

	cameraName->setText(1, Name);
	cameraName->setData(0, PropertyValueRole, qVariantFromValue((void*)Name));
}

void lcQPropertiesTree::SetLight(lcObject* Focus)
{
	Q_UNUSED(Focus);

	SetEmpty();
	mFocus = NULL;

	// todo: light properties
}

void lcQPropertiesTree::SetMultiple()
{
	if (mWidgetMode != LC_PROPERTY_WIDGET_MULTIPLE)
	{
		SetEmpty();

		addProperty(NULL, tr("Multiple Objects Selected"), PropertyGroup);

		mWidgetMode = LC_PROPERTY_WIDGET_MULTIPLE;
	}

	mFocus = NULL;
}

bool lcQPropertiesTree::lastColumn(int column) const
{
	return header()->visualIndex(column) == columnCount() - 1;
}

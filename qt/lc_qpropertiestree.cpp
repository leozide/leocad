#include "lc_global.h"
#include "lc_qpropertiestree.h"
#include "lc_qcolorpicker.h"
#include "lc_application.h"
#include "project.h"
#include "object.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "pieceinf.h"
#include "lc_library.h"

// Draw an icon indicating opened/closing branches
static QIcon drawIndicatorIcon(const QPalette &palette, QStyle *style)
{
	QPixmap pix(14, 14);
	pix.fill(Qt::transparent);
	QStyleOption branchOption;
	QRect r(QPoint(0, 0), pix.size());
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

class lcQPropertiesTreeDelegate : public QItemDelegate
{
	Q_OBJECT
public:
	lcQPropertiesTreeDelegate(QObject *parent = 0)
		: QItemDelegate(parent), m_treeWidget(0), m_editedItem(0), m_editedWidget(0), m_disablePainting(false)
	{}

	void setTreeWidget(lcQPropertiesTree *treeWidget)
	{
		m_treeWidget = treeWidget;
	}

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setModelData(QWidget *, QAbstractItemModel *, const QModelIndex &) const {}
	void setEditorData(QWidget *, const QModelIndex &) const {}
	bool eventFilter(QObject *object, QEvent *event);

	QTreeWidgetItem *editedItem() const
	{
		return m_editedItem;
	}

	QWidget *editor() const
	{
		return m_editedWidget;
	}

protected:
	void drawDecoration(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QPixmap &pixmap) const;
	void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const;

private slots:
	void slotEditorDestroyed(QObject *object);

private:
	int indentation(const QModelIndex &index) const;

	lcQPropertiesTree *m_treeWidget;
	mutable QTreeWidgetItem *m_editedItem;
	mutable QWidget *m_editedWidget;
	mutable bool m_disablePainting;
};

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

	focusObject = NULL;
	setEmpty();

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

void lcQPropertiesTree::updateFocusObject(Object *newFocusObject)
{
	if (!newFocusObject)
	{
		setEmpty();
		return;
	}

	switch (newFocusObject->GetType())
	{
	case LC_OBJECT_PIECE:
		setPart(newFocusObject);
		break;

	case LC_OBJECT_CAMERA:
		setCamera(newFocusObject);
		break;

	case LC_OBJECT_CAMERA_TARGET:
		setCamera(((CameraTarget*)newFocusObject)->GetParent());
		break;

	case LC_OBJECT_LIGHT:
		setLight(newFocusObject);
		break;

	case LC_OBJECT_LIGHT_TARGET:
		setLight(((LightTarget*)newFocusObject)->GetParent());
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

	case PropertyFloat:
		{
			QLineEdit *editor = new QLineEdit(parent);
			float value = item->data(0, PropertyValueRole).toFloat();

			editor->setValidator(new QDoubleValidator());
			editor->setText(QString::number(value));

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

void lcQPropertiesTree::slotReturnPressed()
{
	if (!focusObject)
		return;

	QLineEdit *editor = (QLineEdit*)sender();
	QTreeWidgetItem *item = m_delegate->editedItem();
	Project *project = lcGetActiveProject();

	if (focusObject->GetType() == LC_OBJECT_PIECE)
	{
		Piece *part = (Piece*)focusObject;

		if (item == partPositionX || item == partPositionY || item == partPositionZ)
		{
			lcVector3 position = part->mPosition;
			project->ConvertToUserUnits(position);
			float value = editor->text().toFloat();

			if (item == partPositionX)
				position[0] = value;
			else if (item == partPositionY)
				position[1] = value;
			else if (item == partPositionZ)
				position[2] = value;

			project->ConvertFromUserUnits(position);
			project->ModifyObject(focusObject, LC_PART_POSITION, &position);
		}
		else if (item == partRotationX || item == partRotationY || item == partRotationZ)
		{
			lcVector3 rotation = lcMatrix44ToEulerAngles(part->mModelWorld) * LC_RTOD;
			float value = editor->text().toFloat();

			if (item == partRotationX)
				rotation[0] = value;
			else if (item == partRotationY)
				rotation[1] = value;
			else if (item == partRotationZ)
				rotation[2] = value;


			lcVector4 axisAngle = lcMatrix44ToAxisAngle(lcMatrix44FromEulerAngles(rotation * LC_DTOR));
			axisAngle[3] *= LC_RTOD;
			project->ModifyObject(focusObject, LC_PART_ROTATION, &axisAngle);
		}
		else if (item == partShow)
		{
			lcuint32 value = editor->text().toUInt();

			project->ModifyObject(focusObject, LC_PART_SHOW, &value);
		}
		else if (item == partHide)
		{
			lcuint32 value = editor->text().toUInt();

			project->ModifyObject(focusObject, LC_PART_HIDE, &value);
		}
	}
	else if (focusObject->GetType() == LC_OBJECT_CAMERA)
	{
		Camera *camera = (Camera*)focusObject;

		if (item == cameraPositionX || item == cameraPositionY || item == cameraPositionZ)
		{
			lcVector3 position = camera->mPosition;
			project->ConvertToUserUnits(position);
			float value = editor->text().toFloat();

			if (item == cameraPositionX)
				position[0] = value;
			else if (item == cameraPositionY)
				position[1] = value;
			else if (item == cameraPositionZ)
				position[2] = value;

			project->ConvertFromUserUnits(position);
			project->ModifyObject(focusObject, LC_CAMERA_POSITION, &position);
		}
		else if (item == cameraTargetX || item == cameraTargetY || item == cameraTargetZ)
		{
			lcVector3 target = camera->mTargetPosition;
			project->ConvertToUserUnits(target);
			float value = editor->text().toFloat();

			if (item == cameraTargetX)
				target[0] = value;
			else if (item == cameraTargetY)
				target[1] = value;
			else if (item == cameraTargetZ)
				target[2] = value;

			project->ConvertFromUserUnits(target);
			project->ModifyObject(focusObject, LC_CAMERA_TARGET, &target);
		}
		else if (item == cameraUpX || item == cameraUpY || item == cameraUpZ)
		{
			lcVector3 up = camera->mUpVector;
			float value = editor->text().toFloat();

			if (item == cameraUpX)
				up[0] = value;
			else if (item == cameraUpY)
				up[1] = value;
			else if (item == cameraUpZ)
				up[2] = value;

			project->ModifyObject(focusObject, LC_CAMERA_UP, &up);
		}
		else if (item == cameraFOV)
		{
			float value = editor->text().toFloat();

			project->ModifyObject(focusObject, LC_CAMERA_FOV, &value);
		}
		else if (item == cameraNear)
		{
			float value = editor->text().toFloat();

			project->ModifyObject(focusObject, LC_CAMERA_NEAR, &value);
		}
		else if (item == cameraFar)
		{
			float value = editor->text().toFloat();

			project->ModifyObject(focusObject, LC_CAMERA_FAR, &value);
		}
		else if (item == cameraName)
		{
			QString value = editor->text();

			project->ModifyObject(focusObject, LC_CAMERA_NAME, value.toLocal8Bit().data());
		}
	}
}

void lcQPropertiesTree::slotSetValue(int value)
{
	if (!focusObject)
		return;

	QTreeWidgetItem *item = m_delegate->editedItem();
	Project *project = lcGetActiveProject();

	if (focusObject->GetType() == LC_OBJECT_PIECE)
	{
		if (item == partColor)
		{
			project->ModifyObject(focusObject, LC_PART_COLOR, &value);

			QPushButton *editor = (QPushButton*)m_delegate->editor();
			updateColorEditor(editor, value);
		}
		else if (item == partID)
		{
			QComboBox *editor = (QComboBox*)sender();

			project->ModifyObject(focusObject, LC_PART_ID, editor->itemData(value).value<void*>());
		}
	}
}

void lcQPropertiesTree::slotColorButtonClicked()
{
	QWidget *parent = (QWidget*)sender();
	Piece *part = (Piece*)focusObject;

	lcQColorPickerPopup *popup = new lcQColorPickerPopup(parent, part->mColorIndex);
	connect(popup, SIGNAL(selected(int)), SLOT(slotSetValue(int)));

	const QRect desktop = QApplication::desktop()->geometry();

	QPoint pos = parent->mapToGlobal(parent->rect().bottomLeft());
	if (pos.x() < desktop.left())
		pos.setX(desktop.left());
	if (pos.y() < desktop.top())
		pos.setY(desktop.top());

	if ((pos.x() + popup->sizeHint().width()) > desktop.width())
		pos.setX(desktop.width() - popup->sizeHint().width());
	if ((pos.y() + popup->sizeHint().height()) > desktop.bottom())
		pos.setY(desktop.bottom() - popup->sizeHint().height());
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

void lcQPropertiesTree::setEmpty()
{
	clear();

	focusObject = NULL;

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
	cameraFOV = NULL;
	cameraNear = NULL;
	cameraFar = NULL;
	cameraName = NULL;
}

void lcQPropertiesTree::setPart(Object *newFocusObject)
{
	if (!focusObject || focusObject->GetType() != LC_OBJECT_PIECE)
	{
		setEmpty();

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
	}

	focusObject = newFocusObject;
	Piece *part = (Piece*)focusObject;

	lcVector3 position = part->mPosition;
	lcGetActiveProject()->ConvertToUserUnits(position);

	partPositionX->setText(1, QString::number(position[0]));
	partPositionX->setData(0, PropertyValueRole, position[0]);
	partPositionY->setText(1, QString::number(position[1]));
	partPositionY->setData(0, PropertyValueRole, position[1]);
	partPositionZ->setText(1, QString::number(position[2]));
	partPositionZ->setData(0, PropertyValueRole, position[2]);

	lcVector3 rotation = lcMatrix44ToEulerAngles(part->mModelWorld) * LC_RTOD;

	partRotationX->setText(1, QString::number(rotation[0]));
	partRotationX->setData(0, PropertyValueRole, rotation[0]);
	partRotationY->setText(1, QString::number(rotation[1]));
	partRotationY->setData(0, PropertyValueRole, rotation[1]);
	partRotationZ->setText(1, QString::number(rotation[2]));
	partRotationZ->setData(0, PropertyValueRole, rotation[2]);

	lcuint32 show, hide;
	if (lcGetActiveProject()->IsAnimation())
	{
		show = part->GetFrameShow();
		hide = part->GetFrameHide();
	}
	else
	{
		show = part->GetStepShow();
		hide = part->GetStepHide();
	}

	partShow->setText(1, QString::number(show));
	partShow->setData(0, PropertyValueRole, show);
	partHide->setText(1, QString::number(hide));
	partHide->setData(0, PropertyValueRole, hide);

	QImage img(16, 16, QImage::Format_ARGB32);
	img.fill(0);

	lcColor* color = &gColorList[part->mColorIndex];
	QRgb rgb = qRgb(color->Value[0] * 255, color->Value[1] * 255, color->Value[2] * 255);
	QBrush b(rgb);
	QPainter painter(&img);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(0, 0, img.width(), img.height(), b);
	painter.end();

	partColor->setIcon(1, QIcon(QPixmap::fromImage(img)));
	partColor->setText(1, color->Name);
	partColor->setData(0, PropertyValueRole, part->mColorIndex);

	partID->setText(1, part->mPieceInfo->m_strDescription);
	partID->setData(0, PropertyValueRole, qVariantFromValue((void*)part->mPieceInfo));
}

void lcQPropertiesTree::setCamera(Object *newFocusObject)
{
	if (!focusObject || focusObject->GetType() != LC_OBJECT_CAMERA)
	{
		setEmpty();

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
		cameraFOV = addProperty(cameraSettings, tr("FOV"), PropertyFloat);
		cameraNear = addProperty(cameraSettings, tr("Near"), PropertyFloat);
		cameraFar = addProperty(cameraSettings, tr("Far"), PropertyFloat);
		cameraName = addProperty(cameraSettings, tr("Name"), PropertyString);
	}

	focusObject = newFocusObject;
	Camera *camera = (Camera*)focusObject;

	lcVector3 position = camera->mPosition;
	lcGetActiveProject()->ConvertToUserUnits(position);

	cameraPositionX->setText(1, QString::number(position[0]));
	cameraPositionX->setData(0, PropertyValueRole, position[0]);
	cameraPositionY->setText(1, QString::number(position[1]));
	cameraPositionY->setData(0, PropertyValueRole, position[1]);
	cameraPositionZ->setText(1, QString::number(position[2]));
	cameraPositionZ->setData(0, PropertyValueRole, position[2]);

	lcVector3 target = camera->mTargetPosition;
	lcGetActiveProject()->ConvertToUserUnits(target);

	cameraTargetX->setText(1, QString::number(target[0]));
	cameraTargetX->setData(0, PropertyValueRole, target[0]);
	cameraTargetY->setText(1, QString::number(target[1]));
	cameraTargetY->setData(0, PropertyValueRole, target[1]);
	cameraTargetZ->setText(1, QString::number(target[2]));
	cameraTargetZ->setData(0, PropertyValueRole, target[2]);

	lcVector3 up = camera->mUpVector;
	lcGetActiveProject()->ConvertToUserUnits(up);

	cameraUpX->setText(1, QString::number(up[0]));
	cameraUpX->setData(0, PropertyValueRole, up[0]);
	cameraUpY->setText(1, QString::number(up[1]));
	cameraUpY->setData(0, PropertyValueRole, up[1]);
	cameraUpZ->setText(1, QString::number(up[2]));
	cameraUpZ->setData(0, PropertyValueRole, up[2]);

	cameraFOV->setText(1, QString::number(camera->m_fovy));
	cameraFOV->setData(0, PropertyValueRole, camera->m_fovy);
	cameraNear->setText(1, QString::number(camera->m_zNear));
	cameraNear->setData(0, PropertyValueRole, camera->m_zNear);
	cameraFar->setText(1, QString::number(camera->m_zFar));
	cameraFar->setData(0, PropertyValueRole, camera->m_zFar);

	cameraName->setText(1, camera->GetName());
	cameraName->setData(0, PropertyValueRole, qVariantFromValue((void*)camera->GetName()));
}

void lcQPropertiesTree::setLight(Object *newFocusObject)
{
	// todo: light properties
}

bool lcQPropertiesTree::lastColumn(int column) const
{
	return header()->visualIndex(column) == columnCount() - 1;
}

#include "lc_qpropertiestree.moc"

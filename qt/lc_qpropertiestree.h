#pragma once

#include "lc_array.h"

struct lcPartProperties;
class lcQPropertiesTreeDelegate;

enum lcPropertyWidgetMode
{
	LC_PROPERTY_WIDGET_EMPTY,
	LC_PROPERTY_WIDGET_PIECE,
	LC_PROPERTY_WIDGET_CAMERA,
	LC_PROPERTY_WIDGET_LIGHT,
	LC_PROPERTY_WIDGET_MULTIPLE
};

class lcQPropertiesTree : public QTreeWidget
{
	Q_OBJECT

public:
	lcQPropertiesTree(QWidget *parent = 0);

	QSize sizeHint() const override;

	QTreeWidgetItem *indexToItem(const QModelIndex &index) const
	{
		return itemFromIndex(index);
	}

	void Update(const lcArray<lcObject*>& Selection, lcObject* Focus);

	QWidget *createEditor(QWidget *parent, QTreeWidgetItem *item) const;
	bool lastColumn(int column) const;

	enum
	{
		PropertyTypeRole = Qt::UserRole,
		PropertyValueRole,
		PropertyRangeRole
	};

	enum PropertyType
	{
		PropertyGroup,
		PropertyBool,
		PropertyFloat,
		PropertyStep,
		PropertyString,
		PropertyStringList,
		PropertyLightFormat,
		PropertyColor,
		PropertyPieceColor,
		PropertyPart
	};

protected slots:
	void slotToggled(bool value);
	void slotReturnPressed();
	void slotSetValue(int value);
	void slotColorButtonClicked();
	void LightColorButtonClicked();

protected:
	void keyPressEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void updateColorEditor(QPushButton *editor, int value) const;
	void UpdateLightColorEditor(QPushButton* Editor, QColor Color) const;

	QTreeWidgetItem *addProperty(QTreeWidgetItem *parent, const QString& label, PropertyType propertyType);

	void SetEmpty();
	void SetPiece(const lcArray<lcObject*>& Selection, lcObject* Focus);
	void SetCamera(lcObject* Focus);
	void SetLight(lcObject* Focus);
	void SetMultiple();

	lcLightType mLightType;
	bool mPOVRayLight;

	lcPropertyWidgetMode mWidgetMode;
	lcObject* mFocus;

	lcQPropertiesTreeDelegate* mDelegate;
	QIcon m_expandIcon;
	QIcon m_checkedIcon;
	QIcon m_uncheckedIcon;

	QTreeWidgetItem* mPieceAttributesItem;
	QTreeWidgetItem* partVisibility;
	QTreeWidgetItem* partShow;
	QTreeWidgetItem* partHide;
	QTreeWidgetItem* partAppearance;
	QTreeWidgetItem* mPieceColorItem;
	QTreeWidgetItem* mPieceIdItem;

	QTreeWidgetItem* cameraPosition;
	QTreeWidgetItem* cameraPositionX;
	QTreeWidgetItem* cameraPositionY;
	QTreeWidgetItem* cameraPositionZ;
	QTreeWidgetItem* cameraTarget;
	QTreeWidgetItem* cameraTargetX;
	QTreeWidgetItem* cameraTargetY;
	QTreeWidgetItem* cameraTargetZ;
	QTreeWidgetItem* cameraUp;
	QTreeWidgetItem* cameraUpX;
	QTreeWidgetItem* cameraUpY;
	QTreeWidgetItem* cameraUpZ;
	QTreeWidgetItem* mCameraAttributesItem;
	QTreeWidgetItem* mCameraProjectionItem;
	QTreeWidgetItem* cameraFOV;
	QTreeWidgetItem* cameraNear;
	QTreeWidgetItem* cameraFar;
	QTreeWidgetItem* mCameraNameItem;

	QTreeWidgetItem* lightConfiguration;
	QTreeWidgetItem* mLightColorItem;
	QTreeWidgetItem* mLightPowerItem;
	QTreeWidgetItem* mLightAttributesItem;
	QTreeWidgetItem* lightDiffuse;
	QTreeWidgetItem* lightSpecular;
	QTreeWidgetItem* lightCutoff;
	QTreeWidgetItem* lightEnableCutoff;
	QTreeWidgetItem* lightExponent;
	QTreeWidgetItem* mLightTypeItem;
	QTreeWidgetItem* mLightSpotConeAngleItem;
	QTreeWidgetItem* mLightSpotPenumbraAngleItem;
	QTreeWidgetItem* mLightSpotTightnessItem;
	QTreeWidgetItem* mLightAreaShapeItem;
	QTreeWidgetItem* mLightSizeXItem;
	QTreeWidgetItem* mLightSizeYItem;
	QTreeWidgetItem* mLightNameItem;
	QTreeWidgetItem* lightFormat;
	QTreeWidgetItem* mLightCastShadowItem;
	QTreeWidgetItem* lightAreaGridRows;
	QTreeWidgetItem* lightAreaGridColumns;

	QTreeWidgetItem* mPositionItem;
	QTreeWidgetItem* mPositionXItem;
	QTreeWidgetItem* mPositionYItem;
	QTreeWidgetItem* mPositionZItem;
	QTreeWidgetItem* mRotationItem;
	QTreeWidgetItem* mRotationXItem;
	QTreeWidgetItem* mRotationYItem;
	QTreeWidgetItem* mRotationZItem;
};

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

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setModelData(QWidget *, QAbstractItemModel *, const QModelIndex &) const override {}
	void setEditorData(QWidget *, const QModelIndex &) const override {}
	bool eventFilter(QObject *object, QEvent *event) override;

	QTreeWidgetItem *editedItem() const
	{
		return m_editedItem;
	}

	QWidget *editor() const
	{
		return m_editedWidget;
	}

protected:
	void drawDecoration(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QPixmap &pixmap) const override;
	void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const override;

private slots:
	void slotEditorDestroyed(QObject *object);

private:
	int indentation(const QModelIndex &index) const;

	lcQPropertiesTree *m_treeWidget;
	mutable QTreeWidgetItem *m_editedItem;
	mutable QWidget *m_editedWidget;
	mutable bool m_disablePainting;
};

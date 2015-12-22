#ifndef _LC_QPROPERTIESWIDGET_H_
#define _LC_QPROPERTIESWIDGET_H_

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

	QSize sizeHint() const;

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
		PropertyValueRole
	};

	enum PropertyType
	{
		PropertyGroup,
		PropertyBool,
		PropertyFloat,
		PropertyInt,
		PropertyString,
		PropertyColor,
		PropertyPart
	};

protected slots:
	void slotToggled(bool value);
	void slotReturnPressed();
	void slotSetValue(int value);
	void slotColorButtonClicked();

protected:
	void keyPressEvent(QKeyEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void updateColorEditor(QPushButton *editor, int value) const;

	QTreeWidgetItem *addProperty(QTreeWidgetItem *parent, const QString& label, PropertyType propertyType);

	void SetEmpty();
	void SetPiece(lcObject* Focus);
	void SetCamera(lcObject* Focus);
	void SetLight(lcObject* Focus);
	void SetMultiple();

	void getPartProperties(lcPartProperties *properties);

	lcPropertyWidgetMode mWidgetMode;
	lcObject* mFocus;

	lcQPropertiesTreeDelegate *m_delegate;
	QIcon m_expandIcon;
	QIcon m_checkedIcon;
	QIcon m_uncheckedIcon;

	QTreeWidgetItem *partPosition;
	QTreeWidgetItem *partPositionX;
	QTreeWidgetItem *partPositionY;
	QTreeWidgetItem *partPositionZ;
	QTreeWidgetItem *partRotation;
	QTreeWidgetItem *partRotationX;
	QTreeWidgetItem *partRotationY;
	QTreeWidgetItem *partRotationZ;
	QTreeWidgetItem *partVisibility;
	QTreeWidgetItem *partShow;
	QTreeWidgetItem *partHide;
	QTreeWidgetItem *partAppearance;
	QTreeWidgetItem *partColor;
	QTreeWidgetItem *partID;

	QTreeWidgetItem *cameraPosition;
	QTreeWidgetItem *cameraPositionX;
	QTreeWidgetItem *cameraPositionY;
	QTreeWidgetItem *cameraPositionZ;
	QTreeWidgetItem *cameraTarget;
	QTreeWidgetItem *cameraTargetX;
	QTreeWidgetItem *cameraTargetY;
	QTreeWidgetItem *cameraTargetZ;
	QTreeWidgetItem *cameraUp;
	QTreeWidgetItem *cameraUpX;
	QTreeWidgetItem *cameraUpY;
	QTreeWidgetItem *cameraUpZ;
	QTreeWidgetItem *cameraSettings;
	QTreeWidgetItem *cameraOrtho;
	QTreeWidgetItem *cameraFOV;
	QTreeWidgetItem *cameraNear;
	QTreeWidgetItem *cameraFar;
	QTreeWidgetItem *cameraName;
};

#endif // _LC_QPROPERTIESWIDGET_H_

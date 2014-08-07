#ifndef _LC_QPROPERTIESWIDGET_H_
#define _LC_QPROPERTIESWIDGET_H_

struct lcPartProperties;
class lcQPropertiesTreeDelegate;

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

	void updateFocusObject(lcObject *newFocusObject);

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

	void setEmpty();
	void setPart(lcObject *newFocusObject);
	void setCamera(lcObject *newFocusObject);
	void setLight(lcObject *newFocusObject);

	void getPartProperties(lcPartProperties *properties);

	lcObject *focusObject;

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

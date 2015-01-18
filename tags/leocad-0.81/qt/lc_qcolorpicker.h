#ifndef _LC_QCOLORPICKER_H_
#define _LC_QCOLORPICKER_H_

#include <QObject>
class lcQColorList;

class lcQColorPickerPopup : public QFrame
{
	Q_OBJECT

public:
	lcQColorPickerPopup(QWidget *parent = 0, int colorIndex = 0);
	~lcQColorPickerPopup();

	void exec();

signals:
	void changed(int colorIndex);
	void selected(int colorIndex);
	void hid();

public slots:
	void colorChanged(int colorIndex);
	void colorSelected(int colorIndex);

protected:
	void showEvent(QShowEvent *e);
	void hideEvent(QHideEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);

private:
	QEventLoop *eventLoop;
	lcQColorList *colorList;
};

class lcQColorPicker : public QPushButton
{
	Q_OBJECT

public:
	lcQColorPicker(QWidget *parent = 0);
	~lcQColorPicker();

	int currentColor() const;
	void setCurrentColor(int colorIndex);

public slots:
	void changed(int colorIndex);
	void selected(int colorIndex);

signals:
	void colorChanged(int colorIndex);

protected:
	void updateIcon();

private slots:
	void buttonPressed(bool toggled);
	void popupClosed();

private:
	int currentColorIndex;
	int initialColorIndex;
};

#endif // _LC_QCOLORPICKER_H_

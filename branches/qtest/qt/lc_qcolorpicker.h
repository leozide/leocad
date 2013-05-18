#ifndef _LC_QCOLORPICKER_H_
#define _LC_QCOLORPICKER_H_

#include <QObject>

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

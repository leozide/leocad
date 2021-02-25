#pragma once

#include <QObject>
class lcQColorList;

class lcQColorPickerPopup : public QFrame
{
	Q_OBJECT

public:
	lcQColorPickerPopup(QWidget* Parent = nullptr, int ColorIndex = 0, bool AllowNoColor = false);
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
	void showEvent(QShowEvent* ShowEvent) override;
	void hideEvent(QHideEvent* HideEvent) override;
	void mouseReleaseEvent(QMouseEvent* MouseEvent) override;

private:
	QEventLoop* eventLoop;
	lcQColorList* colorList;
};

class lcQColorPicker : public QPushButton
{
	Q_OBJECT

public:
	lcQColorPicker(QWidget* Parent = nullptr, bool AllowNoColor = false);
	~lcQColorPicker();

	int currentColor() const;
	int currentColorCode() const;
	void setCurrentColor(int colorIndex);
	void setCurrentColorCode(int colorCode);

public slots:
	void changed(int colorIndex);
	void selected(int colorIndex);

signals:
	void colorChanged(int colorIndex);

private slots:
	void buttonPressed(bool toggled);
	void popupClosed();

protected:
	void UpdateIcon();

	int mCurrentColorIndex = 0;
	int mInitialColorIndex = 0;
	bool mAllowNoColor = false;
};

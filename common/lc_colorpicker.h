#pragma once

class lcColorList;

class lcColorPickerPopup : public QFrame
{
	Q_OBJECT

public:
	lcColorPickerPopup(QWidget* Parent = nullptr, int ColorIndex = 0, bool AllowNoColor = false);
	virtual ~lcColorPickerPopup();

	void exec();

signals:
	void Changed(int ColorIndex);
	void Selected(int ColorIndex);
	void Hid();

public slots:
	void ColorChanged(int ColorIndex);
	void ColorSelected(int ColorIndex);

protected:
	void showEvent(QShowEvent* ShowEvent) override;
	void hideEvent(QHideEvent* HideEvent) override;
	void mouseReleaseEvent(QMouseEvent* MouseEvent) override;

private:
	QEventLoop* mEventLoop = nullptr;
	lcColorList* mColorList = nullptr;
};

class lcColorPicker : public QPushButton
{
	Q_OBJECT

public:
	lcColorPicker(QWidget* Parent = nullptr, bool AllowNoColor = false);
	virtual ~lcColorPicker();

	int GetCurrentColor() const;
	int GetCurrentColorCode() const;
	void SetCurrentColor(int ColorIndex);
	void SetCurrentColorCode(int ColorCode);

public slots:
	void Changed(int ColorIndex);
	void Selected(int ColorIndex);

signals:
	void ColorChanged(int ColorIndex);

private slots:
	void ButtonPressed(bool Toggled);
	void PopupClosed();

protected:
	void UpdateIcon();

	int mCurrentColorIndex = 0;
	int mInitialColorIndex = 0;
	bool mAllowNoColor = false;
};

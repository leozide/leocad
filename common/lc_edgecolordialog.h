#pragma once

#include <QtWidgets>

class lcAutomateEdgeColorDialog : public QDialog
{
	Q_OBJECT
public:
	lcAutomateEdgeColorDialog(QWidget *Parent, bool ShowHighContrastDialog);
	quint32 mStudCylinderColor;
	quint32 mPartEdgeColor;
	quint32 mDarkEdgeColor;
	quint32 mBlackEdgeColor;
	float mPartEdgeContrast;
	float mPartColorValueLDIndex;

protected slots:
	void SliderValueChanged(int);
	void ResetSliderButtonClicked();
	void ColorButtonClicked();
	void ResetColorButtonClicked();

protected:
	QSlider* PartColorValueLDIndexSlider;
	QSlider* PartEdgeContrastSlider;

	QLabel* PartEdgeContrast;
	QLabel* PartColorValueLDIndex;

	QToolButton* ResetPartEdgeContrastButton;
	QToolButton* ResetPartColorValueLDIndexButton;

	QToolButton* StudCylinderColorButton;
	QToolButton* PartEdgeColorButton;
	QToolButton* BlackEdgeColorButton;
	QToolButton* DarkEdgeColorButton;

	QToolButton* ResetStudCylinderColorButton;
	QToolButton* ResetPartEdgeColorButton;
	QToolButton* ResetBlackEdgeColorButton;
	QToolButton* ResetDarkEdgeColorButton;
};


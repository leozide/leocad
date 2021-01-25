#pragma once

#include <QtWidgets>

class lcAutomateEdgeColorDialog : public QDialog
{
	Q_OBJECT
public:
	lcAutomateEdgeColorDialog(QWidget *Parent, bool ShowHighContrastDialog);
	quint32 mStudColor;
	quint32 mStudEdgeColor;
	quint32 mDarkEdgeColor;
	quint32 mBlackEdgeColor;
	float mPartEdgeContrast;
	float mPartEdgeGamma;
	float mPartColorValueLDIndex;

protected slots:
	void SliderValueChanged(int);
	void ColorButtonClicked();
	void ResetColorButtonClicked();

protected:
	QSlider* PartEdgeGammaSlider;
	QSlider* PartColorValueLDIndexSlider;
	QSlider* PartEdgeContrastSlider;

	QLabel* PartEdgeContrast;
	QLabel* PartEdgeGamma;
	QLabel* PartColorValueLDIndex;

	QToolButton* StudColorButton;
	QToolButton* StudEdgeColorButton;
	QToolButton* BlackEdgeColorButton;
	QToolButton* DarkEdgeColorButton;

	QToolButton* ResetStudColorButton;
	QToolButton* ResetStudEdgeColorButton;
	QToolButton* ResetBlackEdgeColorButton;
	QToolButton* ResetDarkEdgeColorButton;
};


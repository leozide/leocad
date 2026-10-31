#pragma once

#include "lc_background.h"
#include "lc_math.h"
#include "camera.h"

class lcCommandLineOptions
{
public:
	// Arguments include the executable name, matching QCoreApplication::arguments().
	explicit lcCommandLineOptions(QStringList Arguments);

	bool ParseOK = true;
	bool Exit = false;
	bool SaveImage = false;
	bool SetImageBackground = false;
	lcBackgroundOptions ImageBackground;
	bool SaveWavefront = false;
	bool Save3DS = false;
	bool SaveCOLLADA = false;
	bool SaveCSV = false;
	bool SaveHTML = false;
	bool SetCameraAngles = false;
	bool SetCameraPosition = false;
	bool Orthographic = false;
	bool SetFoV = false;
	bool SetZPlanes = false;
	bool SetFadeStepsColor = false;
	bool SetHighlightColor = false;
	bool FadeSteps;
	bool ImageHighlight;
	bool AutomateEdgeColor;
	int ImageWidth;
	int ImageHeight;
	int AASamples;
	lcShadingMode ShadingMode;
	float LineWidth;
	lcStudStyle StudStyle;
	lcStep ImageStart = 0;
	lcStep ImageEnd = 0;
	lcVector3 CameraPosition[3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	lcVector2 CameraLatLon = { 0.0f, 0.0f };
	float FoV = 0.0f;
	float PartEdgeContrast;
	float PartColorValueLDIndex;
	lcVector2 ZPlanes = { 0.0f, 0.0f };
	lcViewpoint Viewpoint = lcViewpoint::Count;
	bool StudCylinderColorEnabled;
	quint32 StudCylinderColor;
	bool PartEdgeColorEnabled;
	quint32 PartEdgeColor;
	bool BlackEdgeColorEnabled;
	quint32 BlackEdgeColor;
	bool DarkEdgeColorEnabled;
	quint32 DarkEdgeColor;
	quint32 FadeStepsColor = 0;
	quint32 HighlightColor = 0;
	QString ImageName;
	QString ModelName;
	QString CameraName;
	QString ProjectName;
	QString SaveWavefrontName;
	QString Save3DSName;
	QString SaveCOLLADAName;
	QString SaveCSVName;
	QString SaveHTMLName;
	QList<QPair<QString, bool>> LibraryPaths;
	QString StdOut;
	QString StdErr;

private:
	static bool ParseColorValue(const QString& Parameter, quint32& Color);
	static std::optional<bool> ImageFormatSupportsAlpha(const QString& FileName);
	void LoadPreferences();
	void ParseArguments(QStringList& Arguments);
	void ParseOption(const QString& Option, QStringList& Arguments);
	bool ParseImageOption(const QString& Option, QStringList& Arguments);
	bool ParseCameraOption(const QString& Option, QStringList& Arguments);
	bool ParseColorOption(const QString& Option, QStringList& Arguments);
	bool ParseExportOption(const QString& Option, QStringList& Arguments);
	bool ParseString(const QString& Option, QStringList& Arguments, QString& Value, bool Required);
	bool ParseInteger(const QString& Option, QStringList& Arguments, int& Value, int Min, int Max);
	bool ParseUnsigned(const QString& Option, QStringList& Arguments, uint& Value, uint Min, uint Max);
	bool ParseFloat(const QString& Option, QStringList& Arguments, float& Value, float Min, float Max);
	bool ParseFloatArray(const QString& Option, QStringList& Arguments, int Count, float* ValueArray, bool NegativesValid);
	bool ParseColor(const QString& Option, QStringList& Arguments, quint32& Color);
	void AppendVersion();
	void AppendHelp();
	void ValidateOptions();
	void ValidateColorOptions();
	void ValidateCameraOptions();

	QString mPartEdgeContrastOption;
	QString mLightDarkValueOption;
	QStringList mStudColorOptions;
	QStringList mEdgeColorOptions;

	// Preserve the translation context used by the existing command-line messages.
	Q_DECLARE_TR_FUNCTIONS(lcApplication)
};

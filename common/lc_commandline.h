#pragma once

#include "lc_background.h"
#include "lc_math.h"
#include "camera.h"

class lcCommandLineOptions
{
public:
	// Arguments include the executable name, matching QCoreApplication::arguments().
	explicit lcCommandLineOptions(QStringList Arguments);

	bool HasExportRequest() const;
	QString GetExportFileName(const QString& RequestedName, const QString& Extension) const;
	static QString GetImageFrameName(const QString& FileName, bool MultipleSteps);

	bool mParseOK = true;
	bool mExit = false;
	bool mSaveImage = false;
	bool mSetImageBackground = false;
	lcBackgroundOptions mImageBackground;
	bool mSaveWavefront = false;
	bool mSave3DS = false;
	bool mSaveCOLLADA = false;
	bool mSaveCSV = false;
	bool mSaveHTML = false;
	bool mSetCameraAngles = false;
	bool mSetCameraPosition = false;
	bool mOrthographic = false;
	bool mSetFoV = false;
	bool mSetZPlanes = false;
	bool mSetFadeStepsColor = false;
	bool mSetHighlightColor = false;
	bool mFadeSteps;
	bool mImageHighlight;
	bool mAutomateEdgeColor;
	int mImageWidth;
	int mImageHeight;
	int mAASamples;
	lcShadingMode mShadingMode;
	float mLineWidth;
	lcStudStyle mStudStyle;
	lcStep mImageStart = 0;
	lcStep mImageEnd = 0;
	lcVector3 mCameraPosition[3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	lcVector2 mCameraLatLon = { 0.0f, 0.0f };
	float mFoV = 0.0f;
	float mPartEdgeContrast;
	float mPartColorValueLDIndex;
	lcVector2 mZPlanes = { 0.0f, 0.0f };
	lcViewpoint mViewpoint = lcViewpoint::Count;
	bool mStudCylinderColorEnabled;
	quint32 mStudCylinderColor;
	bool mPartEdgeColorEnabled;
	quint32 mPartEdgeColor;
	bool mBlackEdgeColorEnabled;
	quint32 mBlackEdgeColor;
	bool mDarkEdgeColorEnabled;
	quint32 mDarkEdgeColor;
	quint32 mFadeStepsColor = 0;
	quint32 mHighlightColor = 0;
	QString mImageName;
	QString mModelName;
	QString mCameraName;
	QString mProjectName;
	QString mSaveWavefrontName;
	QString mSave3DSName;
	QString mSaveCOLLADAName;
	QString mSaveCSVName;
	QString mSaveHTMLName;
	QList<QPair<QString, bool>> mLibraryPaths;
	QString mShadowLibraryPath;
	QString mStdOut;
	QString mStdErr;

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

#include "lc_global.h"
#include "lc_commandline.h"
#include "lc_application.h"
#include "lc_library.h"
#include "lc_profile.h"
#include "lc_qutils.h"
#include "camera.h"

lcCommandLineOptions::lcCommandLineOptions(QStringList Arguments)
{
	LoadPreferences();

	if (Arguments.isEmpty())
		return;

	Arguments.removeFirst();
	ParseArguments(Arguments);
	ValidateOptions();
}

bool lcCommandLineOptions::HasExportRequest() const
{
	return mSaveImage || mSaveWavefront || mSave3DS || mSaveCOLLADA || mSaveCSV || mSaveHTML;
}

QString lcCommandLineOptions::GetExportFileName(const QString& RequestedName, const QString& Extension) const
{
	QString FileName = RequestedName.isEmpty() ? mProjectName : RequestedName;
	const QString Suffix = QFileInfo(FileName).suffix();

	if (Suffix.isEmpty())
		FileName += QLatin1Char('.') + Extension;
	else if (Suffix.compare(Extension, Qt::CaseInsensitive) != 0)
		FileName = FileName.left(FileName.length() - Suffix.length()) + Extension;

	return FileName;
}

QString lcCommandLineOptions::GetImageFrameName(const QString& FileName, bool MultipleSteps)
{
	if (!MultipleSteps)
		return FileName;

	const QString Suffix = QFileInfo(FileName).suffix();

	if (Suffix.isEmpty())
	{
		if (FileName.endsWith(QLatin1Char('.')))
			return FileName.left(FileName.length() - 1) + QLatin1String("%1.");
		return FileName + QLatin1String("%1");
	}

	return FileName.left(FileName.length() - Suffix.length() - 1) + QLatin1String("%1.") + Suffix;
}

bool lcCommandLineOptions::ParseColorValue(const QString& Parameter, quint32& Color)
{
	if (Parameter.compare(QLatin1String("transparent"), Qt::CaseInsensitive) == 0)
	{
		Color = 0;
		return true;
	}

	static const QRegularExpression Pattern(QStringLiteral("\\A#?[0-9a-fA-F]{6}([0-9a-fA-F]{2})?\\z"));
	if (!Pattern.match(Parameter).hasMatch())
		return false;

	bool Valid;
	const QString Hex = Parameter.startsWith(QLatin1Char('#')) ? Parameter.mid(1) : Parameter;
	const quint32 Value = Hex.toUInt(&Valid, 16);
	if (!Valid)
		return false;

	if (Hex.size() == 6)
		Color = LC_RGBA((Value >> 16) & 255, (Value >> 8) & 255, Value & 255, 255);
	else
		Color = LC_RGBA((Value >> 24) & 255, (Value >> 16) & 255, (Value >> 8) & 255, Value & 255);
	return true;
}

std::optional<bool> lcCommandLineOptions::ImageFormatSupportsAlpha(const QString& FileName)
{
	QByteArray Format = QFileInfo(FileName).suffix().toLatin1().toLower();
	if (Format.isEmpty())
		Format = "png";

	// Probe the installed codec without touching the output file.
	QImage Probe(2, 1, QImage::Format_ARGB32);
	Probe.setPixel(0, 0, qRgba(100, 50, 25, 0));
	Probe.setPixel(1, 0, qRgba(100, 50, 25, 128));
	QByteArray Data;
	QBuffer Buffer(&Data);
	Buffer.open(QIODevice::WriteOnly);
	QImageWriter Writer(&Buffer, Format);
	if (!Writer.write(Probe))
		return std::nullopt;

	const QImage Decoded = QImage::fromData(Data, Format.constData());
	return Decoded.size() == Probe.size() && Decoded.hasAlphaChannel() &&
		qAlpha(Decoded.pixel(0, 0)) == 0 && qAlpha(Decoded.pixel(1, 0)) == 128;
}

void lcCommandLineOptions::LoadPreferences()
{
	lcPreferences Preferences;
	Preferences.LoadDefaults();

	mFadeSteps = Preferences.mFadeSteps;
	mImageHighlight = Preferences.mHighlightNewParts;
	mImageWidth = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
	mImageHeight = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);
	mShadingMode = Preferences.mShadingMode;
	mLineWidth = Preferences.mLineWidth;
	mAASamples = lcGetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES);
	mStudStyle = lcGetProfileStudStyle();
	mStudCylinderColorEnabled = Preferences.mStudCylinderColorEnabled;
	mStudCylinderColor = Preferences.mStudCylinderColor;
	mPartEdgeColorEnabled = Preferences.mPartEdgeColorEnabled;
	mPartEdgeColor = Preferences.mPartEdgeColor;
	mBlackEdgeColorEnabled = Preferences.mBlackEdgeColorEnabled;
	mBlackEdgeColor = Preferences.mBlackEdgeColor;
	mDarkEdgeColorEnabled = Preferences.mDarkEdgeColorEnabled;
	mDarkEdgeColor = Preferences.mDarkEdgeColor;
	mPartEdgeContrast = Preferences.mPartEdgeContrast;
	mPartColorValueLDIndex = Preferences.mPartColorValueLDIndex;
	mAutomateEdgeColor = Preferences.mAutomateEdgeColor;
}

void lcCommandLineOptions::ParseArguments(QStringList& Arguments)
{
	while (!Arguments.isEmpty())
	{
		const QString Option = Arguments.takeFirst();

		if (Option.isEmpty())
			continue;

		if (Option[0] != '-')
			mProjectName = Option;
		else
			ParseOption(Option, Arguments);
	}
}

void lcCommandLineOptions::ParseOption(const QString& Option, QStringList& Arguments)
{
	if (ParseImageOption(Option, Arguments) || ParseCameraOption(Option, Arguments) ||
		ParseColorOption(Option, Arguments) || ParseExportOption(Option, Arguments))
		return;

	if (Option == QLatin1String("-l") || Option == QLatin1String("--libpath"))
	{
		QString LibPath;

		if (ParseString(Option, Arguments, LibPath, true))
			mLibraryPaths += QPair<QString, bool>(LibPath, false);
	}
	else if (Option == QLatin1String("--shadow-library"))
		ParseString(Option, Arguments, mShadowLibraryPath, true);
	else if (Option == QLatin1String("-v") || Option == QLatin1String("--version"))
	{
		AppendVersion();
		mExit = true;
	}
	else if (Option == QLatin1String("-?") || Option == QLatin1String("--help"))
	{
		AppendHelp();
		mExit = true;
	}
	else
	{
		mStdErr += tr("Unknown option: '%1'.\n").arg(Option);
		mParseOK = false;
	}
}

bool lcCommandLineOptions::ParseImageOption(const QString& Option, QStringList& Arguments)
{
	if (Option == QLatin1String("-i") || Option == QLatin1String("--image"))
	{
		mSaveImage = true;
		ParseString(Option, Arguments, mImageName, false);
	}
	else if (Option == QLatin1String("--background") || Option == QLatin1String("--background-gradient"))
	{
		const bool Gradient = Option == QLatin1String("--background-gradient");
		QString Top, Bottom;
		const bool HasTop = ParseString(Option, Arguments, Top, true);
		bool HasBottom = true;
		if (Gradient)
			HasBottom = ParseString(Option, Arguments, Bottom, true);

		if (mSetImageBackground)
		{
			mStdErr += tr("Specify only one --background or --background-gradient option.\n");
			mParseOK = false;
			return true;
		}

		mSetImageBackground = true;
		if (!HasTop || !HasBottom)
			return true;

		if (!Gradient && Top.compare(QLatin1String("transparent"), Qt::CaseInsensitive) == 0)
			mImageBackground.Mode = lcBackgroundMode::Transparent;
		else
		{
			mImageBackground.Mode = Gradient ? lcBackgroundMode::CustomGradient : lcBackgroundMode::CustomColor;
			if (!ParseColorValue(Top, mImageBackground.Color))
			{
				mStdErr += tr("Invalid background color '%1'; use RRGGBB, RRGGBBAA, or transparent.\n").arg(Top);
				mParseOK = false;
			}
			if (Gradient && !ParseColorValue(Bottom, mImageBackground.GradientBottomColor))
			{
				mStdErr += tr("Invalid background color '%1'; use RRGGBB, RRGGBBAA, or transparent.\n").arg(Bottom);
				mParseOK = false;
			}
		}
	}
	else if (Option == QLatin1String("-w") || Option == QLatin1String("--width"))
		ParseInteger(Option, Arguments, mImageWidth, 1, LC_MAX_IMAGE_DIMENSION);
	else if (Option == QLatin1String("-h") || Option == QLatin1String("--height"))
		ParseInteger(Option, Arguments, mImageHeight, 1, LC_MAX_IMAGE_DIMENSION);
	else if (Option == QLatin1String("-f") || Option == QLatin1String("--from"))
		ParseUnsigned(Option, Arguments, mImageStart, 1, LC_STEP_MAX);
	else if (Option == QLatin1String("-t") || Option == QLatin1String("--to"))
		ParseUnsigned(Option, Arguments, mImageEnd, 1, LC_STEP_MAX);
	else if (Option == QLatin1String("-s") || Option == QLatin1String("--submodel"))
		ParseString(Option, Arguments, mModelName, true);
	else if (Option == QLatin1String("--shading"))
	{
		QString ShadingString;

		if (ParseString(Option, Arguments, ShadingString, true))
		{
			if (ShadingString == QLatin1String("wireframe"))
				mShadingMode = lcShadingMode::Wireframe;
			else if (ShadingString == QLatin1String("flat"))
				mShadingMode = lcShadingMode::Flat;
			else if (ShadingString == QLatin1String("default"))
				mShadingMode = lcShadingMode::DefaultLights;
			else if (ShadingString == QLatin1String("full"))
				mShadingMode = lcShadingMode::Full;
			else
			{
				mStdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, ShadingString);
				mParseOK = false;
			}
		}
	}
	else if (Option == QLatin1String("--line-width"))
		ParseFloat(Option, Arguments, mLineWidth, 0.0f, 10.0f);
	else if (Option == QLatin1String("--aa-samples"))
	{
		if (ParseInteger(Option, Arguments, mAASamples, 1, 8) && mAASamples != 1 && mAASamples != 2 && mAASamples != 4 && mAASamples != 8)
		{
			mStdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, QString::number(mAASamples));
			mParseOK = false;
		}
	}
	else if (Option == QLatin1String("-ss") || Option == QLatin1String("--stud-style"))
	{
		int StudStyleValue;

		if (ParseInteger(Option, Arguments, StudStyleValue, 0, static_cast<int>(lcStudStyle::Count) - 1))
			mStudStyle = static_cast<lcStudStyle>(StudStyleValue);
	}
	else
		return false;

	return true;
}

bool lcCommandLineOptions::ParseCameraOption(const QString& Option, QStringList& Arguments)
{
	if (Option == QLatin1String("-c") || Option == QLatin1String("--camera"))
		ParseString(Option, Arguments, mCameraName, true);
	else if (Option == QLatin1String("--viewpoint"))
	{
		QString ViewpointName;

		if (ParseString(Option, Arguments, ViewpointName, true))
		{
			mViewpoint = lcCamera::GetViewpoint(ViewpointName);

			if (mViewpoint == lcViewpoint::Count)
			{
				mStdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, ViewpointName);
				mParseOK = false;
			}
		}
	}
	else if (Option == QLatin1String("--camera-angles"))
	{
		if ((mSetCameraAngles = ParseFloatArray(Option, Arguments, 2, mCameraLatLon.GetFloats(), true)) && (fabsf(mCameraLatLon[0]) > 360.0f || fabsf(mCameraLatLon[1]) > 360.0f))
		{
			mStdErr += tr("Invalid parameter values specified for the '%1' option: limits are +/- 360.\n").arg(Option);
			mParseOK = false;
		}
	}
	else if (Option == QLatin1String("--camera-position") || Option == QLatin1String("--camera-position-ldraw"))
	{
		if ((mSetCameraPosition = ParseFloatArray(Option, Arguments, 9, mCameraPosition[0].GetFloats(), true)))
		{
			if (Option == QLatin1String("--camera-position-ldraw"))
			{
				mCameraPosition[0] = lcVector3LDrawToLeoCAD(mCameraPosition[0]);
				mCameraPosition[1] = lcVector3LDrawToLeoCAD(mCameraPosition[1]);
				mCameraPosition[2] = lcVector3LDrawToLeoCAD(mCameraPosition[2]);
			}

			lcVector3 Front = mCameraPosition[1] - mCameraPosition[0];

			if (Front.LengthSquared() < 0.1f)
			{
				mStdErr += tr("The camera position is too close to the camera target of the '%1' option.\n").arg(Option);
				mParseOK = false;
			}
			else if (mCameraPosition[2].LengthSquared() < 0.1f)
			{
				mStdErr += tr("Invalid up vector length specified for the '%1' option.\n").arg(Option);
				mParseOK = false;
			}
			else if (fabsf(lcDot(lcNormalize(Front), lcNormalize(mCameraPosition[2]))) > 0.99f)
			{
				mStdErr += tr("The up vector is parallel to the front vector specified for the '%1' option.\n").arg(Option);
				mParseOK = false;
			}
		}
	}
	else if (Option == QLatin1String("--orthographic"))
		mOrthographic = true;
	else if (Option == QLatin1String("--fov"))
		mSetFoV = ParseFloat(Option, Arguments, mFoV, 1.0f, 180.0f);
	else if (Option == QLatin1String("--zplanes"))
	{
		if ((mSetZPlanes = ParseFloatArray(Option, Arguments, 2, mZPlanes.GetFloats(), false)) && (mZPlanes[0] < 1.0 || mZPlanes[0] >= mZPlanes[1]))
		{
			mStdErr += tr("Invalid parameter values specified for the '%1' option: requirements are: 1 <= <near> < <far>.\n").arg(Option);
			mParseOK = false;
		}
	}
	else
		return false;

	return true;
}

bool lcCommandLineOptions::ParseColorOption(const QString& Option, QStringList& Arguments)
{
	if (Option == QLatin1String("-nscc") || Option == QLatin1String("--disable-stud-cylinder-color"))
	{
		mStudCylinderColorEnabled = false;
		mStudColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-scc") || Option == QLatin1String("--stud-cylinder-color"))
	{
		if (ParseColor(Option, Arguments, mStudCylinderColor))
		{
			mStudCylinderColorEnabled = true;
			mStudColorOptions.append(Option);
		}
	}
	else if (Option == QLatin1String("-nec") || Option == QLatin1String("--disable-edge-color"))
	{
		mPartEdgeColorEnabled = false;
		mEdgeColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-ec") || Option == QLatin1String("--edge-color"))
	{
		if (ParseColor(Option, Arguments, mPartEdgeColor))
		{
			mPartEdgeColorEnabled = true;
			mEdgeColorOptions.append(Option);
		}
	}
	else if (Option == QLatin1String("-nbec") || Option == QLatin1String("--disable-black-edge-color"))
	{
		mBlackEdgeColorEnabled = false;
		mEdgeColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-bec") || Option == QLatin1String("--black-edge-color"))
	{
		if (ParseColor(Option, Arguments, mBlackEdgeColor))
		{
			mBlackEdgeColorEnabled = true;
			mEdgeColorOptions.append(Option);
		}
	}
	else if (Option == QLatin1String("-ndec") || Option == QLatin1String("--disable-dark-edge-color"))
	{
		mDarkEdgeColorEnabled = false;
		mEdgeColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-dec") || Option == QLatin1String("--dark-edge-color"))
	{
		if (ParseColor(Option, Arguments, mDarkEdgeColor))
		{
			mDarkEdgeColorEnabled = true;
			mEdgeColorOptions.append(Option);
		}
	}
	else if (Option == QLatin1String("-aec") || Option == QLatin1String("--automate-edge-color"))
		mAutomateEdgeColor = true;
	else if (Option == QLatin1String("-cc") || Option == QLatin1String("--color-contrast"))
	{
		if (ParseFloat(Option, Arguments, mPartEdgeContrast, 0.0f, 1.0f))
			mPartEdgeContrastOption = Option;
	}
	else if (Option == QLatin1String("-ldv") || Option == QLatin1String("--light-dark-value"))
	{
		if (ParseFloat(Option, Arguments, mPartColorValueLDIndex, 0.0f, 1.0f))
			mLightDarkValueOption = Option;
	}
	else if (Option == QLatin1String("--fade-steps"))
		mFadeSteps = true;
	else if (Option == QLatin1String("--no-fade-steps"))
		mFadeSteps = false;
	else if (Option == QLatin1String("--fade-steps-color"))
	{
		if (ParseColor(Option, Arguments, mFadeStepsColor))
		{
			mSetFadeStepsColor = true;
			mFadeSteps = true;
		}
	}
	else if (Option == QLatin1String("--highlight"))
		mImageHighlight = true;
	else if (Option == QLatin1String("--no-highlight"))
		mImageHighlight = false;
	else if (Option == QLatin1String("--highlight-color"))
	{
		if (ParseColor(Option, Arguments, mHighlightColor))
		{
			mSetHighlightColor = true;
			mImageHighlight = true;
		}
	}
	else
		return false;

	return true;
}

bool lcCommandLineOptions::ParseExportOption(const QString& Option, QStringList& Arguments)
{
	if (Option == QLatin1String("-obj") || Option == QLatin1String("--export-wavefront"))
	{
		mSaveWavefront = true;
		ParseString(Option, Arguments, mSaveWavefrontName, false);
	}
	else if (Option == QLatin1String("-3ds") || Option == QLatin1String("--export-3ds"))
	{
		mSave3DS = true;
		ParseString(Option, Arguments, mSave3DSName, false);
	}
	else if (Option == QLatin1String("-dae") || Option == QLatin1String("--export-collada"))
	{
		mSaveCOLLADA = true;
		ParseString(Option, Arguments, mSaveCOLLADAName, false);
	}
	else if (Option == QLatin1String("-csv") || Option == QLatin1String("--export-csv"))
	{
		mSaveCSV = true;
		ParseString(Option, Arguments, mSaveCSVName, false);
	}
	else if (Option == QLatin1String("-html") || Option == QLatin1String("--export-html"))
	{
		mSaveHTML = true;
		ParseString(Option, Arguments, mSaveHTMLName, false);
	}
	else
		return false;

	return true;
}

bool lcCommandLineOptions::ParseString(const QString& Option, QStringList& Arguments, QString& Value, bool Required)
{
	if (!Arguments.isEmpty() && !Arguments.front().startsWith(QLatin1Char('-')))
	{
		QString Parameter = Arguments.takeFirst();
		Value = Parameter;
	}
	else if (Required)
	{
		mStdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);
		mParseOK = false;
		return false;
	}

	return true;
}

bool lcCommandLineOptions::ParseInteger(const QString& Option, QStringList& Arguments, int& Value, int Min, int Max)
{
	if (!Arguments.isEmpty() && Arguments.front()[0] != '-')
	{
		bool Ok = false;
		QString Parameter = Arguments.takeFirst();
		int NewValue = Parameter.toInt(&Ok);

		if (Ok && NewValue >= Min && NewValue <= Max)
		{
			Value = NewValue;
			return true;
		}
		else
			mStdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, Parameter);
	}
	else
		mStdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);

	mParseOK = false;
	return false;
}

bool lcCommandLineOptions::ParseUnsigned(const QString& Option, QStringList& Arguments, uint& Value, uint Min, uint Max)
{
	if (!Arguments.isEmpty() && Arguments.front()[0] != '-')
	{
		bool Ok = false;
		QString Parameter = Arguments.takeFirst();
		uint NewValue = Parameter.toUInt(&Ok);

		if (Ok && NewValue >= Min && NewValue <= Max)
		{
			Value = NewValue;
			return true;
		}
		else
			mStdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, Parameter);
	}
	else
		mStdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);

	mParseOK = false;
	return false;
}

bool lcCommandLineOptions::ParseFloat(const QString& Option, QStringList& Arguments, float& Value, float Min, float Max)
{
	if (!Arguments.isEmpty() && Arguments.front()[0] != '-')
	{
		bool Ok = false;
		QString Parameter = Arguments.takeFirst();
		float NewValue = Parameter.toFloat(&Ok);

		if (Ok && NewValue >= Min && NewValue <= Max)
		{
			Value = NewValue;
			return true;
		}
		else
			mStdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, Parameter);
	}
	else
		mStdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);

	mParseOK = false;
	return false;
}

bool lcCommandLineOptions::ParseFloatArray(const QString& Option, QStringList& Arguments, int Count, float* ValueArray, bool NegativesValid)
{
	if (Arguments.size() < Count)
	{
		mStdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);
		Arguments.clear();
		mParseOK = false;
		return false;
	}

	for (int ParseIndex = 0; ParseIndex < Count; ParseIndex++)
	{
		if (NegativesValid || Arguments.front()[0] != '-')
		{
			bool Ok = false;
			QString Parameter = Arguments.takeFirst();
			float NewValue = Parameter.toFloat(&Ok);

			if (Ok)
			{
				*(ValueArray++) = NewValue;
				continue;
			}

			mStdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, Parameter);
		}
		else
			mStdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);

		mParseOK = false;
		return false;
	}

	return true;
}

bool lcCommandLineOptions::ParseColor(const QString& Option, QStringList& Arguments, quint32& Color)
{
	QString Parameter;

	if (!ParseString(Option, Arguments, Parameter, true))
		return false;

	if (ParseColorValue(Parameter, Color))
		return true;

	mStdErr += tr("Invalid color for the '%1' option: '%2'; use RRGGBB, RRGGBBAA, or transparent.\n").arg(Option, Parameter);
	mParseOK = false;
	return false;
}

void lcCommandLineOptions::AppendVersion()
{
#ifdef LC_CONTINUOUS_BUILD
	mStdOut += tr("LeoCAD Continuous Build %1\n").arg(QT_STRINGIFY(LC_CONTINUOUS_BUILD));
#else
	mStdOut += tr("LeoCAD Version %1\n").arg(LC_VERSION_TEXT);
#endif
	mStdOut += tr("Compiled %1\n").arg(__DATE__);
}

void lcCommandLineOptions::AppendHelp()
{
	mStdOut += tr("Usage: leocad [options] [file]\n");
	mStdOut += tr("  Square brackets mark optional arguments. Put the input file before options with omitted filenames.\n");
	mStdOut += tr("  [options] can be:\n");
	mStdOut += tr("  Colors use RRGGBB or RRGGBBAA (alpha last), with an optional # prefix, or transparent.\n");
	mStdOut += tr("  -l, --libpath <path>: Set the Pieces Library location to path.\n");
	mStdOut += tr("  --shadow-library <path>: Set the LDCad Shadow Library location to path.\n");
	mStdOut += tr("  -i, --image [outfile.ext]: Save a picture in the format specified by ext and exit.\n");
	mStdOut += tr("    Omitted image filename: use the input name and saved image extension. Explicit extensionless filename: PNG.\n");
	mStdOut += tr("  --background <transparent|RRGGBB|RRGGBBAA>: Set the image background; alpha is last.\n");
	mStdOut += tr("  --background-gradient <top> <bottom>: Set a vertical gradient using RRGGBB, RRGGBBAA, or transparent colors.\n");
	mStdOut += tr("    Background options require --image and cannot be combined or repeated. Default: transparent when the format preserves alpha, otherwise white. Alpha requires a format that preserves it.\n");
	mStdOut += tr("  -w, --width <width>: Set the picture width (1 to 32768).\n");
	mStdOut += tr("  -h, --height <height>: Set the picture height (1 to 32768).\n");
	mStdOut += tr("  -f, --from <step>: Set the first step to save pictures.\n");
	mStdOut += tr("  -t, --to <step>: Set the last step to save pictures.\n");
	mStdOut += tr("  -s, --submodel <submodel>: Set the active submodel.\n");
	mStdOut += tr("  -c, --camera <camera name>: Set the active camera.\n");
	mStdOut += tr("  --viewpoint <viewpoint>: Set the view to one of the preset viewpoints (front, back, left, right, top, bottom, home).\n");
	mStdOut += tr("  --camera-angles <latitude> <longitude>: Set the camera angles in degrees around the model (-360 to 360).\n");
	mStdOut += tr("  --camera-position <x> <y> <z> <tx> <ty> <tz> <ux> <uy> <uz>: Set the camera position, target and up vector.\n");
	mStdOut += tr("  --camera-position-ldraw <x> <y> <z> <tx> <ty> <tz> <ux> <uy> <uz>: Set the camera position, target and up vector using LDraw coordinates.\n");
	mStdOut += tr("  --orthographic: Render images using an orthographic projection.\n");
	mStdOut += tr("  --fov <degrees>: Set the vertical field of view used to render images (1 to 180 degrees, inclusive).\n");
	mStdOut += tr("  --zplanes <near> <far>: Set the near and far clipping planes used to render images (1 <= <near> < <far>).\n");
	mStdOut += tr("    Camera precedence: named camera, then viewpoint, then angles, then position.\n");
	mStdOut += tr("  --fade-steps: Render pieces from prior steps faded.\n");
	mStdOut += tr("  --no-fade-steps: Do not render pieces from prior steps faded.\n");
	mStdOut += tr("  --fade-steps-color <RRGGBB|RRGGBBAA|transparent>: Rendering color for prior step pieces; enables fading.\n");
	mStdOut += tr("  --highlight: Highlight pieces in the steps they appear.\n");
	mStdOut += tr("  --no-highlight: Do not highlight pieces in the steps they appear.\n");
	mStdOut += tr("  --highlight-color <RRGGBB|RRGGBBAA|transparent>: Rendering color for highlighted pieces; enables highlighting.\n");
	mStdOut += tr("  --shading <mode>: Select shading mode for rendering (wireframe, flat, default, full).\n");
	mStdOut += tr("  --line-width <width>: Set the width of the edge lines (0 to 10).\n");
	mStdOut += tr("  --aa-samples <samples>: Anti-aliasing sample size (1, 2, 4, or 8).\n");
	mStdOut += tr("  -ss, --stud-style <style>: Set the stud style (0=Plain, 1=Thin Lines Logo, 2=Outline Logo, 3=Sharp Top Logo, 4=Rounded Top Logo, 5=Flattened Logo, 6=High Contrast, 7=High Contrast with Logo).\n");
	mStdOut += tr("  -nscc, --disable-stud-cylinder-color: Disable high contrast stud cylinder color.\n");
	mStdOut += tr("  -scc, --stud-cylinder-color <RRGGBB|RRGGBBAA|transparent>: High contrast stud cylinder color.\n");
	mStdOut += tr("  -nec, --disable-edge-color: Disable high contrast edge color.\n");
	mStdOut += tr("  -ec, --edge-color <RRGGBB|RRGGBBAA|transparent>: High contrast edge color.\n");
	mStdOut += tr("  -nbec, --disable-black-edge-color: Disable high contrast edge color for black pieces.\n");
	mStdOut += tr("  -bec, --black-edge-color <RRGGBB|RRGGBBAA|transparent>: High contrast edge color for black pieces.\n");
	mStdOut += tr("  -ndec, --disable-dark-edge-color: Disable high contrast edge color for dark color pieces.\n");
	mStdOut += tr("  -dec, --dark-edge-color <RRGGBB|RRGGBBAA|transparent>: High contrast edge color for dark color pieces.\n");
	mStdOut += tr("  -aec, --automate-edge-color: Enable automatically adjusted edge colors; overrides manual edge colors.\n");
	mStdOut += tr("  -cc, --color-contrast <contrast>: Color contrast value between 0.0 and 1.0; requires automatic edge colors.\n");
	mStdOut += tr("  -ldv, --light-dark-value <value>: Light/Dark value between 0.0 and 1.0; requires automatic edges or a high contrast stud style.\n");
	mStdOut += tr("    Stud-cylinder and manual edge options require stud style 6 or 7. Automatic edges preserve stud-cylinder settings.\n");
	mStdOut += tr("  -obj, --export-wavefront [outfile.obj]: Export the model to Wavefront OBJ format.\n");
	mStdOut += tr("  -3ds, --export-3ds [outfile.3ds]: Export the model to 3D Studio 3DS format.\n");
	mStdOut += tr("  -dae, --export-collada [outfile.dae]: Export the model to COLLADA DAE format.\n");
	mStdOut += tr("  -csv, --export-csv [outfile.csv]: Export the list of pieces used in CSV format.\n");
	mStdOut += tr("  -html, --export-html [folder]: Create an HTML page for the model.\n");
	mStdOut += tr("  -v, --version: Output version information and exit.\n");
	mStdOut += tr("  -?, --help: Display this help message and exit.\n");
	mStdOut += QLatin1String("\n");
}

void lcCommandLineOptions::ValidateOptions()
{
	ValidateColorOptions();
	ValidateCameraOptions();

	if (!mExit && mSetImageBackground && !mSaveImage)
	{
		mStdErr += tr("Background options require --image.\n");
		mParseOK = false;
	}

	if (!mExit && mParseOK && mSaveImage)
	{
		QString FileName = mImageName;
		if (FileName.isEmpty())
			FileName = QStringLiteral("image") + lcGetProfileString(LC_PROFILE_IMAGE_EXTENSION);
		const std::optional<bool> SupportsAlpha = ImageFormatSupportsAlpha(FileName);
		if (!SupportsAlpha)
		{
			mStdErr += tr("Unsupported image output format for '%1'.\n").arg(FileName);
			mParseOK = false;
		}
		else if (!mSetImageBackground)
		{
			if (*SupportsAlpha)
				mImageBackground.Mode = lcBackgroundMode::Transparent;
			else
			{
				mImageBackground.Mode = lcBackgroundMode::CustomColor;
				mImageBackground.Color = LC_RGBA(255, 255, 255, 255);
			}
		}
		else if (!*SupportsAlpha)
		{
			const lcBackgroundOptions& Background = mImageBackground;
			const bool Transparent = Background.Mode == lcBackgroundMode::Transparent || LC_RGBA_ALPHA(Background.Color) != 255 ||
				(Background.Mode == lcBackgroundMode::CustomGradient && LC_RGBA_ALPHA(Background.GradientBottomColor) != 255);
			if (Transparent)
			{
				mStdErr += tr("The image format for '%1' does not preserve alpha; specify an opaque background.\n").arg(FileName);
				mParseOK = false;
			}
		}
	}

	if (HasExportRequest() && mProjectName.isEmpty())
	{
		mStdErr += tr("No file name specified.\n");
		mParseOK = false;
	}
}

void lcCommandLineOptions::ValidateColorOptions()
{
	const bool HighContrast = lcIsHighContrast(mStudStyle);

	if (!mAutomateEdgeColor && !mPartEdgeContrastOption.isEmpty())
		mStdErr += tr("The '%1' option requires automatic edge colors (--automate-edge-color).\n").arg(mPartEdgeContrastOption);

	if (!mAutomateEdgeColor && !HighContrast && !mLightDarkValueOption.isEmpty())
		mStdErr += tr("The '%1' option requires automatic edge colors or a high contrast stud style.\n").arg(mLightDarkValueOption);

	mStudColorOptions.removeDuplicates();
	mEdgeColorOptions.removeDuplicates();

	if (!HighContrast)
		for (const QString& Option : mStudColorOptions)
			mStdErr += tr("The '%1' option requires a high contrast stud style (--stud-style 6 or 7).\n").arg(Option);

	for (const QString& Option : mEdgeColorOptions)
	{
		if (mAutomateEdgeColor)
			mStdErr += tr("Automatic edge colors take precedence over the '%1' option.\n").arg(Option);
		else if (!HighContrast)
			mStdErr += tr("The '%1' option requires a high contrast stud style (--stud-style 6 or 7).\n").arg(Option);
	}
}

void lcCommandLineOptions::ValidateCameraOptions()
{
	if (!mCameraName.isEmpty())
	{
		if (mViewpoint != lcViewpoint::Count)
			mStdErr += tr("--viewpoint is ignored when --camera is set.\n");

		if (mOrthographic)
			mStdErr += tr("--orthographic is ignored when --camera is set.\n");

		if (mSetCameraAngles)
			mStdErr += tr("--camera-angles is ignored when --camera is set.\n");

		if (mSetCameraPosition)
			mStdErr += tr("--camera-position is ignored when --camera is set.\n");
	}
	else if (mViewpoint != lcViewpoint::Count)
	{
		if (mSetCameraAngles)
			mStdErr += tr("--camera-angles is ignored when --viewpoint is set.\n");

		if (mSetCameraPosition)
			mStdErr += tr("--camera-position is ignored when --viewpoint is set.\n");
	}
	else if (mSetCameraAngles)
	{
		if (mSetCameraPosition)
			mStdErr += tr("--camera-position is ignored when --camera-angles is set.\n");
	}
}

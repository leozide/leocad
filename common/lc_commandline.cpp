#include "lc_global.h"
#include "lc_commandline.h"
#include "lc_application.h"
#include "lc_library.h"
#include "lc_profile.h"
#include "lc_qutils.h"
#include "camera.h"

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

lcCommandLineOptions::lcCommandLineOptions(QStringList Arguments)
{
	LoadDefaults();

	if (Arguments.isEmpty())
		return;

	Arguments.removeFirst();
	ParseArguments(Arguments);
	ValidateOptions();
}

void lcCommandLineOptions::LoadDefaults()
{
	lcPreferences Preferences;
	Preferences.LoadDefaults();

	FadeSteps = Preferences.mFadeSteps;
	ImageHighlight = Preferences.mHighlightNewParts;
	ImageWidth = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
	ImageHeight = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);
	ShadingMode = Preferences.mShadingMode;
	LineWidth = Preferences.mLineWidth;
	AASamples = lcGetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES);
	StudStyle = lcGetProfileStudStyle();
	ImageStart = 0;
	ImageEnd = 0;
	CameraPosition[0] = lcVector3(0.0f, 0.0f, 0.0f);
	CameraPosition[1] = lcVector3(0.0f, 0.0f, 0.0f);
	CameraPosition[2] = lcVector3(0.0f, 0.0f, 0.0f);
	CameraLatLon = lcVector2(0.0f, 0.0f);
	FoV = 0.0f;
	ZPlanes = lcVector2(0.0f, 0.0f);
	Viewpoint = lcViewpoint::Count;
	FadeStepsColor = Preferences.mFadeStepsColor;
	HighlightColor = Preferences.mHighlightNewPartsColor;
	StudCylinderColorEnabled = Preferences.mStudCylinderColorEnabled;
	StudCylinderColor = Preferences.mStudCylinderColor;
	PartEdgeColorEnabled = Preferences.mPartEdgeColorEnabled;
	PartEdgeColor = Preferences.mPartEdgeColor;
	BlackEdgeColorEnabled = Preferences.mBlackEdgeColorEnabled;
	BlackEdgeColor = Preferences.mBlackEdgeColor;
	DarkEdgeColorEnabled = Preferences.mDarkEdgeColorEnabled;
	DarkEdgeColor = Preferences.mDarkEdgeColor;
	PartEdgeContrast = Preferences.mPartEdgeContrast;
	PartColorValueLDIndex = Preferences.mPartColorValueLDIndex;
	AutomateEdgeColor = Preferences.mAutomateEdgeColor;
}

void lcCommandLineOptions::ParseArguments(QStringList& Arguments)
{
	while (!Arguments.isEmpty())
	{
		const QString Option = Arguments.takeFirst();

		if (Option.isEmpty())
			continue;

		if (Option[0] != '-')
		{
			if (QFileInfo(Option).isReadable())
				ProjectName = Option;
			else
				StdErr += tr("The file '%1' is not readable.\n").arg(Option);
		}
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
			LibraryPaths += QPair<QString, bool>(LibPath, false);
	}
	else if (Option == QLatin1String("-v") || Option == QLatin1String("--version"))
	{
		AppendVersion();
		Exit = true;
	}
	else if (Option == QLatin1String("-?") || Option == QLatin1String("--help"))
	{
		AppendHelp();
		Exit = true;
	}
	else
	{
		StdErr += tr("Unknown option: '%1'.\n").arg(Option);
		ParseOK = false;
	}
}

bool lcCommandLineOptions::ParseImageOption(const QString& Option, QStringList& Arguments)
{
	if (Option == QLatin1String("-i") || Option == QLatin1String("--image"))
	{
		SaveImage = true;
		ParseString(Option, Arguments, ImageName, false);
	}
	else if (Option == QLatin1String("--background") || Option == QLatin1String("--background-gradient"))
	{
		const bool Gradient = Option == QLatin1String("--background-gradient");
		QString Top, Bottom;
		const bool HasTop = ParseString(Option, Arguments, Top, true);
		bool HasBottom = true;
		if (Gradient)
			HasBottom = ParseString(Option, Arguments, Bottom, true);

		if (SetImageBackground)
		{
			StdErr += tr("Specify only one --background or --background-gradient option.\n");
			ParseOK = false;
			return true;
		}

		SetImageBackground = true;
		if (!HasTop || !HasBottom)
			return true;

		if (!Gradient && Top.compare(QLatin1String("transparent"), Qt::CaseInsensitive) == 0)
			ImageBackground.Mode = lcBackgroundMode::Transparent;
		else
		{
			ImageBackground.Mode = Gradient ? lcBackgroundMode::CustomGradient : lcBackgroundMode::CustomColor;
			if (!ParseColorValue(Top, ImageBackground.Color))
			{
				StdErr += tr("Invalid background color '%1'; use RRGGBB, RRGGBBAA, or transparent.\n").arg(Top);
				ParseOK = false;
			}
			if (Gradient && !ParseColorValue(Bottom, ImageBackground.GradientBottomColor))
			{
				StdErr += tr("Invalid background color '%1'; use RRGGBB, RRGGBBAA, or transparent.\n").arg(Bottom);
				ParseOK = false;
			}
		}
	}
	else if (Option == QLatin1String("-w") || Option == QLatin1String("--width"))
		ParseInteger(Option, Arguments, ImageWidth, 1, LC_MAX_IMAGE_DIMENSION);
	else if (Option == QLatin1String("-h") || Option == QLatin1String("--height"))
		ParseInteger(Option, Arguments, ImageHeight, 1, LC_MAX_IMAGE_DIMENSION);
	else if (Option == QLatin1String("-f") || Option == QLatin1String("--from"))
		ParseUnsigned(Option, Arguments, ImageStart, 1, LC_STEP_MAX);
	else if (Option == QLatin1String("-t") || Option == QLatin1String("--to"))
		ParseUnsigned(Option, Arguments, ImageEnd, 1, LC_STEP_MAX);
	else if (Option == QLatin1String("-s") || Option == QLatin1String("--submodel"))
		ParseString(Option, Arguments, ModelName, true);
	else if (Option == QLatin1String("--shading"))
	{
		QString ShadingString;

		if (ParseString(Option, Arguments, ShadingString, true))
		{
			if (ShadingString == QLatin1String("wireframe"))
				ShadingMode = lcShadingMode::Wireframe;
			else if (ShadingString == QLatin1String("flat"))
				ShadingMode = lcShadingMode::Flat;
			else if (ShadingString == QLatin1String("default"))
				ShadingMode = lcShadingMode::DefaultLights;
			else if (ShadingString == QLatin1String("full"))
				ShadingMode = lcShadingMode::Full;
			else
			{
				StdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, ShadingString);
				ParseOK = false;
			}
		}
	}
	else if (Option == QLatin1String("--line-width"))
		ParseFloat(Option, Arguments, LineWidth, 0.0f, 10.0f);
	else if (Option == QLatin1String("--aa-samples"))
	{
		if (ParseInteger(Option, Arguments, AASamples, 1, 8) && AASamples != 1 && AASamples != 2 && AASamples != 4 && AASamples != 8)
		{
			StdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, QString::number(AASamples));
			ParseOK = false;
		}
	}
	else if (Option == QLatin1String("-ss") || Option == QLatin1String("--stud-style"))
	{
		int StudStyleValue;

		if (ParseInteger(Option, Arguments, StudStyleValue, 0, static_cast<int>(lcStudStyle::Count) - 1))
			StudStyle = static_cast<lcStudStyle>(StudStyleValue);
	}
	else
		return false;

	return true;
}

bool lcCommandLineOptions::ParseCameraOption(const QString& Option, QStringList& Arguments)
{
	if (Option == QLatin1String("-c") || Option == QLatin1String("--camera"))
		ParseString(Option, Arguments, CameraName, true);
	else if (Option == QLatin1String("--viewpoint"))
	{
		QString ViewpointName;

		if (ParseString(Option, Arguments, ViewpointName, true))
		{
			Viewpoint = lcCamera::GetViewpoint(ViewpointName);

			if (Viewpoint == lcViewpoint::Count)
			{
				StdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, ViewpointName);
				ParseOK = false;
			}
		}
	}
	else if (Option == QLatin1String("--camera-angles"))
	{
		if ((SetCameraAngles = ParseFloatArray(Option, Arguments, 2, CameraLatLon.GetFloats(), true)) && (fabsf(CameraLatLon[0]) > 360.0f || fabsf(CameraLatLon[1]) > 360.0f))
		{
			StdErr += tr("Invalid parameter values specified for the '%1' option: limits are +/- 360.\n").arg(Option);
			ParseOK = false;
		}
	}
	else if (Option == QLatin1String("--camera-position") || Option == QLatin1String("--camera-position-ldraw"))
	{
		if ((SetCameraPosition = ParseFloatArray(Option, Arguments, 9, CameraPosition[0].GetFloats(), true)))
		{
			if (Option == QLatin1String("--camera-position-ldraw"))
			{
				CameraPosition[0] = lcVector3LDrawToLeoCAD(CameraPosition[0]);
				CameraPosition[1] = lcVector3LDrawToLeoCAD(CameraPosition[1]);
				CameraPosition[2] = lcVector3LDrawToLeoCAD(CameraPosition[2]);
			}

			lcVector3 Front = CameraPosition[1] - CameraPosition[0];

			if (Front.LengthSquared() < 0.1f)
			{
				StdErr += tr("The camera position is too close to the camera target of the '%1' option.\n").arg(Option);
				ParseOK = false;
			}
			else if (CameraPosition[2].LengthSquared() < 0.1f)
			{
				StdErr += tr("Invalid up vector length specified for the '%1' option.\n").arg(Option);
				ParseOK = false;
			}
			else if (fabsf(lcDot(lcNormalize(Front), lcNormalize(CameraPosition[2]))) > 0.99f)
			{
				StdErr += tr("The up vector is parallel to the front vector specified for the '%1' option.\n").arg(Option);
				ParseOK = false;
			}
		}
	}
	else if (Option == QLatin1String("--orthographic"))
		Orthographic = true;
	else if (Option == QLatin1String("--fov"))
		SetFoV = ParseFloat(Option, Arguments, FoV, 1.0f, 180.0f);
	else if (Option == QLatin1String("--zplanes"))
	{
		if ((SetZPlanes = ParseFloatArray(Option, Arguments, 2, ZPlanes.GetFloats(), false)) && (ZPlanes[0] < 1.0 || ZPlanes[0] >= ZPlanes[1]))
		{
			StdErr += tr("Invalid parameter values specified for the '%1' option: requirements are: 1 <= <near> < <far>.\n").arg(Option);
			ParseOK = false;
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
		StudCylinderColorEnabled = false;
		mStudColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-scc") || Option == QLatin1String("--stud-cylinder-color"))
	{
		if (ParseColor(Option, Arguments, StudCylinderColor))
			mStudColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-nec") || Option == QLatin1String("--disable-edge-color"))
	{
		PartEdgeColorEnabled = false;
		mEdgeColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-ec") || Option == QLatin1String("--edge-color"))
	{
		if (ParseColor(Option, Arguments, PartEdgeColor))
			mEdgeColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-nbec") || Option == QLatin1String("--disable-black-edge-color"))
	{
		BlackEdgeColorEnabled = false;
		mEdgeColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-bec") || Option == QLatin1String("--black-edge-color"))
	{
		if (ParseColor(Option, Arguments, BlackEdgeColor))
			mEdgeColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-ndec") || Option == QLatin1String("--disable-dark-edge-color"))
	{
		DarkEdgeColorEnabled = false;
		mEdgeColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-dec") || Option == QLatin1String("--dark-edge-color"))
	{
		if (ParseColor(Option, Arguments, DarkEdgeColor))
			mEdgeColorOptions.append(Option);
	}
	else if (Option == QLatin1String("-aec") || Option == QLatin1String("--automate-edge-color"))
		AutomateEdgeColor = true;
	else if (Option == QLatin1String("-cc") || Option == QLatin1String("--color-contrast"))
	{
		if (ParseFloat(Option, Arguments, PartEdgeContrast, 0.0f, 1.0f))
			mPartEdgeContrastOption = Option;
	}
	else if (Option == QLatin1String("-ldv") || Option == QLatin1String("--light-dark-value"))
	{
		if (ParseFloat(Option, Arguments, PartColorValueLDIndex, 0.0f, 1.0f))
			mLightDarkValueOption = Option;
	}
	else if (Option == QLatin1String("--fade-steps"))
		FadeSteps = true;
	else if (Option == QLatin1String("--no-fade-steps"))
		FadeSteps = false;
	else if (Option == QLatin1String("--fade-steps-color"))
	{
		if (ParseColor(Option, Arguments, FadeStepsColor))
		{
			SetFadeStepsColor = true;
			FadeSteps = true;
		}
	}
	else if (Option == QLatin1String("--highlight"))
		ImageHighlight = true;
	else if (Option == QLatin1String("--no-highlight"))
		ImageHighlight = false;
	else if (Option == QLatin1String("--highlight-color"))
	{
		if (ParseColor(Option, Arguments, HighlightColor))
		{
			SetHighlightColor = true;
			ImageHighlight = true;
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
		SaveWavefront = true;
		ParseString(Option, Arguments, SaveWavefrontName, false);
	}
	else if (Option == QLatin1String("-3ds") || Option == QLatin1String("--export-3ds"))
	{
		Save3DS = true;
		ParseString(Option, Arguments, Save3DSName, false);
	}
	else if (Option == QLatin1String("-dae") || Option == QLatin1String("--export-collada"))
	{
		SaveCOLLADA = true;
		ParseString(Option, Arguments, SaveCOLLADAName, false);
	}
	else if (Option == QLatin1String("-csv") || Option == QLatin1String("--export-csv"))
	{
		SaveCSV = true;
		ParseString(Option, Arguments, SaveCSVName, false);
	}
	else if (Option == QLatin1String("-html") || Option == QLatin1String("--export-html"))
	{
		SaveHTML = true;
		ParseString(Option, Arguments, SaveHTMLName, false);
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
		StdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);
		ParseOK = false;
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
			StdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, Parameter);
	}
	else
		StdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);

	ParseOK = false;
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
			StdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, Parameter);
	}
	else
		StdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);

	ParseOK = false;
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
			StdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, Parameter);
	}
	else
		StdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);

	ParseOK = false;
	return false;
}

bool lcCommandLineOptions::ParseFloatArray(const QString& Option, QStringList& Arguments, int Count, float* ValueArray, bool NegativesValid)
{
	if (Arguments.size() < Count)
	{
		StdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);
		Arguments.clear();
		ParseOK = false;
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

			StdErr += tr("Invalid parameter value specified for the '%1' option: '%2'.\n").arg(Option, Parameter);
		}
		else
			StdErr += tr("Not enough parameters for the '%1' option.\n").arg(Option);

		ParseOK = false;
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

	StdErr += tr("Invalid color for the '%1' option: '%2'; use RRGGBB, RRGGBBAA, or transparent.\n").arg(Option, Parameter);
	ParseOK = false;
	return false;
}

void lcCommandLineOptions::AppendVersion()
{
#ifdef LC_CONTINUOUS_BUILD
	StdOut += tr("LeoCAD Continuous Build %1\n").arg(QT_STRINGIFY(LC_CONTINUOUS_BUILD));
#else
	StdOut += tr("LeoCAD Version %1\n").arg(LC_VERSION_TEXT);
#endif
	StdOut += tr("Compiled %1\n").arg(__DATE__);
}

void lcCommandLineOptions::AppendHelp()
{
	StdOut += tr("Usage: leocad [options] [file]\n");
	StdOut += tr("  [options] can be:\n");
	StdOut += tr("  Colors use RRGGBB or RRGGBBAA (alpha last), with an optional # prefix, or transparent.\n");
	StdOut += tr("  -l, --libpath <path>: Set the Pieces Library location to path.\n");
	StdOut += tr("  -i, --image <outfile.ext>: Save a picture in the format specified by ext and exit.\n");
	StdOut += tr("  --background <transparent|RRGGBB|RRGGBBAA>: Set the image background; alpha is last.\n");
	StdOut += tr("  --background-gradient <top> <bottom>: Set a vertical gradient using RRGGBB, RRGGBBAA, or transparent colors.\n");
	StdOut += tr("    Background options require --image and cannot be combined or repeated. Default: transparent when the format preserves alpha, otherwise white.\n");
	StdOut += tr("  -w, --width <width>: Set the picture width.\n");
	StdOut += tr("  -h, --height <height>: Set the picture height.\n");
	StdOut += tr("  -f, --from <step>: Set the first step to save pictures.\n");
	StdOut += tr("  -t, --to <step>: Set the last step to save pictures.\n");
	StdOut += tr("  -s, --submodel <submodel>: Set the active submodel.\n");
	StdOut += tr("  -c, --camera <camera name>: Set the active camera.\n");
	StdOut += tr("  --viewpoint <viewpoint>: Set the view to one of the preset viewpoints (front, back, left, right, top, bottom, home).\n");
	StdOut += tr("  --camera-angles <latitude> <longitude>: Set the camera angles in degrees around the model.\n");
	StdOut += tr("  --camera-position <x> <y> <z> <tx> <ty> <tz> <ux> <uy> <uz>: Set the camera position, target and up vector.\n");
	StdOut += tr("  --camera-position-ldraw <x> <y> <z> <tx> <ty> <tz> <ux> <uy> <uz>: Set the camera position, target and up vector using LDraw coordinates.\n");
	StdOut += tr("  --orthographic: Render images using an orthographic projection.\n");
	StdOut += tr("  --fov <degrees>: Set the vertical field of view used to render images (< 180).\n");
	StdOut += tr("  --zplanes <near> <far>: Set the near and far clipping planes used to render images (1 <= <near> < <far>).\n");
	StdOut += tr("  --fade-steps: Render pieces from prior steps faded.\n");
	StdOut += tr("  --no-fade-steps: Do not render pieces from prior steps faded.\n");
	StdOut += tr("  --fade-steps-color <RRGGBB|RRGGBBAA|transparent>: Rendering color for prior step pieces.\n");
	StdOut += tr("  --highlight: Highlight pieces in the steps they appear.\n");
	StdOut += tr("  --no-highlight: Do not highlight pieces in the steps they appear.\n");
	StdOut += tr("  --highlight-color <RRGGBB|RRGGBBAA|transparent>: Rendering color for highlighted pieces.\n");
	StdOut += tr("  --shading <mode>: Select shading mode for rendering (wireframe, flat, default, full).\n");
	StdOut += tr("  --line-width <width>: Set the width of the edge lines.\n");
	StdOut += tr("  --aa-samples <samples>: Anti-aliasing sample size (1, 2, 4, or 8).\n");
	StdOut += tr("  -ss, --stud-style <style>: Set the stud style (0=No style, 1=LDraw single wire, 2=LDraw double wire, 3=LDraw raised floating, 4=LDraw raised rounded, 5=LDraw subtle rounded, 6=LEGO no logo, 7=LEGO single wire).\n");
	StdOut += tr("  -nscc, --disable-stud-cylinder-color: Disable high contrast stud cylinder color.\n");
	StdOut += tr("  -scc, --stud-cylinder-color <RRGGBB|RRGGBBAA|transparent>: High contrast stud cylinder color.\n");
	StdOut += tr("  -nec, --disable-edge-color: Disable high contrast edge color.\n");
	StdOut += tr("  -ec, --edge-color <RRGGBB|RRGGBBAA|transparent>: High contrast edge color.\n");
	StdOut += tr("  -nbec, --disable-black-edge-color: Disable high contrast edge color for black pieces.\n");
	StdOut += tr("  -bec, --black-edge-color <RRGGBB|RRGGBBAA|transparent>: High contrast edge color for black pieces.\n");
	StdOut += tr("  -ndec, --disable-dark-edge-color: Disable high contrast edge color for dark color pieces.\n");
	StdOut += tr("  -dec, --dark-edge-color <RRGGBB|RRGGBBAA|transparent>: High contrast edge color for dark color pieces.\n");
	StdOut += tr("  -aec, --automate-edge-color: Enable automatically adjusted edge colors.\n");
	StdOut += tr("  -cc, --color-contrast <contrast>: Color contrast value between 0.0 and 1.0.\n");
	StdOut += tr("  -ldv, --light-dark-value <value>: Light/Dark color value between 0.0 and 1.0.\n");
	StdOut += tr("  -obj, --export-wavefront <outfile.obj>: Export the model to Wavefront OBJ format.\n");
	StdOut += tr("  -3ds, --export-3ds <outfile.3ds>: Export the model to 3D Studio 3DS format.\n");
	StdOut += tr("  -dae, --export-collada <outfile.dae>: Export the model to COLLADA DAE format.\n");
	StdOut += tr("  -csv, --export-csv <outfile.csv>: Export the list of pieces used in CSV format.\n");
	StdOut += tr("  -html, --export-html <folder>: Create an HTML page for the model.\n");
	StdOut += tr("  -v, --version: Output version information and exit.\n");
	StdOut += tr("  -?, --help: Display this help message and exit.\n");
	StdOut += QLatin1String("\n");
}

void lcCommandLineOptions::ValidateOptions()
{
	ValidateColorOptions();
	ValidateCameraOptions();

	const bool SaveAndExit = (SaveImage || SaveWavefront || Save3DS || SaveCOLLADA || SaveCSV || SaveHTML);

	if (!Exit && SetImageBackground && !SaveImage)
	{
		StdErr += tr("Background options require --image.\n");
		ParseOK = false;
	}

	if (!Exit && ParseOK && SaveImage)
	{
		QString FileName = ImageName;
		if (FileName.isEmpty())
			FileName = QStringLiteral("image") + lcGetProfileString(LC_PROFILE_IMAGE_EXTENSION);
		const std::optional<bool> SupportsAlpha = ImageFormatSupportsAlpha(FileName);
		if (!SupportsAlpha)
		{
			StdErr += tr("Unsupported image output format for '%1'.\n").arg(FileName);
			ParseOK = false;
		}
		else if (!SetImageBackground)
		{
			if (*SupportsAlpha)
				ImageBackground.Mode = lcBackgroundMode::Transparent;
			else
			{
				ImageBackground.Mode = lcBackgroundMode::CustomColor;
				ImageBackground.Color = LC_RGBA(255, 255, 255, 255);
			}
		}
		else if (!*SupportsAlpha)
		{
			const lcBackgroundOptions& Background = ImageBackground;
			const bool Transparent = Background.Mode == lcBackgroundMode::Transparent || LC_RGBA_ALPHA(Background.Color) != 255 ||
				(Background.Mode == lcBackgroundMode::CustomGradient && LC_RGBA_ALPHA(Background.GradientBottomColor) != 255);
			if (Transparent)
			{
				StdErr += tr("The image format for '%1' does not preserve alpha; specify an opaque background.\n").arg(FileName);
				ParseOK = false;
			}
		}
	}

	if (SaveAndExit && ProjectName.isEmpty())
	{
		StdErr += tr("No file name specified.\n");
		ParseOK = false;
	}
}

void lcCommandLineOptions::ValidateColorOptions()
{
	const bool HighContrast = lcIsHighContrast(StudStyle);

	if (!AutomateEdgeColor && !mPartEdgeContrastOption.isEmpty())
		StdErr += tr("The '%1' option requires automatic edge colors (--automate-edge-color).\n").arg(mPartEdgeContrastOption);

	if (!AutomateEdgeColor && !HighContrast && !mLightDarkValueOption.isEmpty())
		StdErr += tr("The '%1' option requires automatic edge colors or a high contrast stud style.\n").arg(mLightDarkValueOption);

	mStudColorOptions.removeDuplicates();
	mEdgeColorOptions.removeDuplicates();

	if (!HighContrast)
		for (const QString& Option : mStudColorOptions)
			StdErr += tr("The '%1' option requires a high contrast stud style (--stud-style 6 or 7).\n").arg(Option);

	for (const QString& Option : mEdgeColorOptions)
	{
		if (AutomateEdgeColor)
			StdErr += tr("Automatic edge colors take precedence over the '%1' option.\n").arg(Option);
		else if (!HighContrast)
			StdErr += tr("The '%1' option requires a high contrast stud style (--stud-style 6 or 7).\n").arg(Option);
	}
}

void lcCommandLineOptions::ValidateCameraOptions()
{
	if (!CameraName.isEmpty())
	{
		if (Viewpoint != lcViewpoint::Count)
			StdErr += tr("--viewpoint is ignored when --camera is set.\n");

		if (Orthographic)
			StdErr += tr("--orthographic is ignored when --camera is set.\n");

		if (SetCameraAngles)
			StdErr += tr("--camera-angles is ignored when --camera is set.\n");

		if (SetCameraPosition)
			StdErr += tr("--camera-position is ignored when --camera is set.\n");
	}
	else if (Viewpoint != lcViewpoint::Count)
	{
		if (SetCameraAngles)
			StdErr += tr("--camera-angles is ignored when --viewpoint is set.\n");

		if (SetCameraPosition)
			StdErr += tr("--camera-position is ignored when --viewpoint is set.\n");
	}
	else if (SetCameraAngles)
	{
		if (SetCameraPosition)
			StdErr += tr("--camera-position is ignored when --camera-angles is set.\n");
	}
}

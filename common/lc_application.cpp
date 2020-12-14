#include "lc_global.h"
#include <stdio.h>
#include "lc_application.h"
#include "lc_library.h"
#include "lc_profile.h"
#include "project.h"
#include "lc_mainwindow.h"
#include "lc_qpreferencesdialog.h"
#include "lc_partselectionwidget.h"
#include "lc_shortcuts.h"
#include "view.h"
#include "lc_previewwidget.h"

lcApplication* gApplication;

void lcPreferences::LoadDefaults()
{
	mFixedAxes = lcGetProfileInt(LC_PROFILE_FIXED_AXES);
	mMouseSensitivity = lcGetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY);
	mShadingMode = static_cast<lcShadingMode>(lcGetProfileInt(LC_PROFILE_SHADING_MODE));
	mBackgroundGradient = lcGetProfileInt(LC_PROFILE_BACKGROUND_GRADIENT);
	mBackgroundSolidColor = lcGetProfileInt(LC_PROFILE_BACKGROUND_COLOR);
	mBackgroundGradientColorTop = lcGetProfileInt(LC_PROFILE_GRADIENT_COLOR_TOP);
	mBackgroundGradientColorBottom = lcGetProfileInt(LC_PROFILE_GRADIENT_COLOR_BOTTOM);
	mDrawAxes = lcGetProfileInt(LC_PROFILE_DRAW_AXES);
	mAxesColor = lcGetProfileInt(LC_PROFILE_AXES_COLOR);
	mOverlayColor = lcGetProfileInt(LC_PROFILE_OVERLAY_COLOR);
	mActiveViewColor = lcGetProfileInt(LC_PROFILE_ACTIVE_VIEW_COLOR);
	mInactiveViewColor = lcGetProfileInt(LC_PROFILE_INACTIVE_VIEW_COLOR);
	mDrawEdgeLines = lcGetProfileInt(LC_PROFILE_DRAW_EDGE_LINES);
	mLineWidth = lcGetProfileFloat(LC_PROFILE_LINE_WIDTH);
	mAllowLOD = lcGetProfileInt(LC_PROFILE_ALLOW_LOD);
	mMeshLODDistance = lcGetProfileFloat(LC_PROFILE_LOD_DISTANCE);
	mFadeSteps = lcGetProfileInt(LC_PROFILE_FADE_STEPS);
	mFadeStepsColor = lcGetProfileInt(LC_PROFILE_FADE_STEPS_COLOR);
	mHighlightNewParts = lcGetProfileInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS);
	mHighlightNewPartsColor = lcGetProfileInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS_COLOR);
	mDrawGridStuds = lcGetProfileInt(LC_PROFILE_GRID_STUDS);
	mGridStudColor = lcGetProfileInt(LC_PROFILE_GRID_STUD_COLOR);
	mDrawGridLines = lcGetProfileInt(LC_PROFILE_GRID_LINES);
	mGridLineSpacing = lcGetProfileInt(LC_PROFILE_GRID_LINE_SPACING);
	mGridLineColor = lcGetProfileInt(LC_PROFILE_GRID_LINE_COLOR);
	mViewSphereEnabled = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_ENABLED);
	mViewSphereLocation = static_cast<lcViewSphereLocation>(lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_LOCATION));
	mViewSphereSize = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_SIZE);
	mViewSphereColor = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_COLOR);
	mViewSphereTextColor = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_TEXT_COLOR);
	mViewSphereHighlightColor = lcGetProfileInt(LC_PROFILE_VIEW_SPHERE_HIGHLIGHT_COLOR);
	mAutoLoadMostRecent = lcGetProfileInt(LC_PROFILE_AUTOLOAD_MOSTRECENT);
	mRestoreTabLayout = lcGetProfileInt(LC_PROFILE_RESTORE_TAB_LAYOUT);
	mColorTheme = static_cast<lcColorTheme>(lcGetProfileInt(LC_PROFILE_COLOR_THEME));

	mPreviewViewSphereEnabled = lcGetProfileInt(LC_PROFILE_PREVIEW_VIEW_SPHERE_ENABLED);
	mPreviewViewSphereSize = lcGetProfileInt(LC_PROFILE_PREVIEW_VIEW_SPHERE_SIZE);
	mPreviewViewSphereLocation = static_cast<lcViewSphereLocation>(lcGetProfileInt(LC_PROFILE_PREVIEW_VIEW_SPHERE_LOCATION));
	mPreviewEnabled  = lcGetProfileInt(LC_PROFILE_PREVIEW_ENABLED);
	mPreviewSize     = lcGetProfileInt(LC_PROFILE_PREVIEW_SIZE);
	mPreviewLocation = static_cast<lcPreviewLocation>(lcGetProfileInt(LC_PROFILE_PREVIEW_LOCATION));
	mPreviewPosition = static_cast<lcPreviewPosition>(lcGetProfileInt(LC_PROFILE_PREVIEW_POSITION));
	mDrawPreviewAxis = lcGetProfileInt(LC_PROFILE_PREVIEW_DRAW_AXES);
	mDrawPreviewViewSphere = lcGetProfileInt(LC_PROFILE_PREVIEW_DRAW_VIEW_SPHERE);
}

void lcPreferences::SaveDefaults()
{
	lcSetProfileInt(LC_PROFILE_FIXED_AXES, mFixedAxes);
	lcSetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY, mMouseSensitivity);
	lcSetProfileInt(LC_PROFILE_SHADING_MODE, static_cast<int>(mShadingMode));
	lcSetProfileInt(LC_PROFILE_DRAW_AXES, mDrawAxes);
	lcSetProfileInt(LC_PROFILE_AXES_COLOR, mAxesColor);
	lcSetProfileInt(LC_PROFILE_BACKGROUND_GRADIENT, mBackgroundGradient);
	lcSetProfileInt(LC_PROFILE_BACKGROUND_COLOR, mBackgroundSolidColor);
	lcSetProfileInt(LC_PROFILE_GRADIENT_COLOR_TOP, mBackgroundGradientColorTop);
	lcSetProfileInt(LC_PROFILE_GRADIENT_COLOR_BOTTOM, mBackgroundGradientColorBottom);
	lcSetProfileInt(LC_PROFILE_OVERLAY_COLOR, mOverlayColor);
	lcSetProfileInt(LC_PROFILE_ACTIVE_VIEW_COLOR, mActiveViewColor);
	lcSetProfileInt(LC_PROFILE_INACTIVE_VIEW_COLOR, mInactiveViewColor);
	lcSetProfileInt(LC_PROFILE_DRAW_EDGE_LINES, mDrawEdgeLines);
	lcSetProfileFloat(LC_PROFILE_LINE_WIDTH, mLineWidth);
	lcSetProfileInt(LC_PROFILE_ALLOW_LOD, mAllowLOD);
	lcSetProfileFloat(LC_PROFILE_LOD_DISTANCE, mMeshLODDistance);
	lcSetProfileInt(LC_PROFILE_FADE_STEPS, mFadeSteps);
	lcSetProfileInt(LC_PROFILE_FADE_STEPS_COLOR, mFadeStepsColor);
	lcSetProfileInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS, mHighlightNewParts);
	lcSetProfileInt(LC_PROFILE_HIGHLIGHT_NEW_PARTS_COLOR, mHighlightNewPartsColor);
	lcSetProfileInt(LC_PROFILE_GRID_STUDS, mDrawGridStuds);
	lcSetProfileInt(LC_PROFILE_GRID_STUD_COLOR, mGridStudColor);
	lcSetProfileInt(LC_PROFILE_GRID_LINES, mDrawGridLines);
	lcSetProfileInt(LC_PROFILE_GRID_LINE_SPACING, mGridLineSpacing);
	lcSetProfileInt(LC_PROFILE_GRID_LINE_COLOR, mGridLineColor);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_ENABLED, mViewSphereSize ? 1 : 0);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_LOCATION, static_cast<int>(mViewSphereLocation));
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_SIZE, mViewSphereSize);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_COLOR, mViewSphereColor);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_TEXT_COLOR, mViewSphereTextColor);
	lcSetProfileInt(LC_PROFILE_VIEW_SPHERE_HIGHLIGHT_COLOR, mViewSphereHighlightColor);
	lcSetProfileInt(LC_PROFILE_AUTOLOAD_MOSTRECENT, mAutoLoadMostRecent);
	lcSetProfileInt(LC_PROFILE_RESTORE_TAB_LAYOUT, mRestoreTabLayout);
	lcSetProfileInt(LC_PROFILE_COLOR_THEME, static_cast<int>(mColorTheme));

	lcSetProfileInt(LC_PROFILE_PREVIEW_ENABLED, mPreviewViewSphereEnabled);
	lcSetProfileInt(LC_PROFILE_PREVIEW_VIEW_SPHERE_SIZE, mPreviewViewSphereSize);
	lcSetProfileInt(LC_PROFILE_PREVIEW_VIEW_SPHERE_LOCATION, static_cast<int>(mPreviewViewSphereLocation));
	lcSetProfileInt(LC_PROFILE_PREVIEW_ENABLED, mPreviewEnabled);
	lcSetProfileInt(LC_PROFILE_PREVIEW_SIZE, mPreviewSize);
	lcSetProfileInt(LC_PROFILE_PREVIEW_LOCATION, static_cast<int>(mPreviewLocation));
	lcSetProfileInt(LC_PROFILE_PREVIEW_POSITION, static_cast<int>(mPreviewPosition));
	lcSetProfileInt(LC_PROFILE_PREVIEW_DRAW_AXES, mDrawPreviewAxis);
	lcSetProfileInt(LC_PROFILE_PREVIEW_DRAW_VIEW_SPHERE, mDrawPreviewViewSphere);
}

void lcPreferences::SetInterfaceColors(lcColorTheme ColorTheme)
{
	if (ColorTheme == lcColorTheme::Dark)
	{
		mAxesColor = LC_RGBA(0, 0, 0, 255);
		mBackgroundSolidColor = LC_RGB(49, 52, 55);
		mBackgroundGradientColorTop = LC_RGB(0, 0, 191);
		mBackgroundGradientColorBottom = LC_RGB(255, 255, 255);
		mOverlayColor = lcGetProfileInt(LC_PROFILE_OVERLAY_COLOR);
		mActiveViewColor = LC_RGBA(41, 128, 185, 255);
		mGridStudColor = LC_RGBA(24, 24, 24, 192);
		mGridLineColor = LC_RGBA(24, 24, 24, 255);
		mViewSphereColor = LC_RGBA(35, 38, 41, 255);
		mViewSphereTextColor = LC_RGBA(224, 224, 224, 255);
		mViewSphereHighlightColor = LC_RGBA(41, 128, 185, 255);
	}
	else
	{
		mAxesColor = LC_RGBA(0, 0, 0, 255);
		mBackgroundSolidColor = LC_RGB(255, 255, 255);
		mBackgroundGradientColorTop = LC_RGB(54, 72, 95);
		mBackgroundGradientColorBottom = LC_RGB(49, 52, 55);
		mOverlayColor = LC_RGBA(0, 0, 0, 255);
		mActiveViewColor = LC_RGBA(255, 0, 0, 255);
		mGridStudColor = LC_RGBA(64, 64, 64, 192);
		mGridLineColor = LC_RGBA(0, 0, 0, 255);
		mViewSphereColor = LC_RGBA(255, 255, 255, 255);
		mViewSphereTextColor = LC_RGBA(0, 0, 0, 255);
		mViewSphereHighlightColor = LC_RGBA(255, 0, 0, 255);
	}
}

lcApplication::lcApplication(int& Argc, char** Argv)
	: QApplication(Argc, Argv)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	setApplicationDisplayName("LeoCAD");
#endif

	setOrganizationDomain("leocad.org");
	setOrganizationName("LeoCAD Software");
	setApplicationName("LeoCAD");
	setApplicationVersion(LC_VERSION_TEXT);

	gApplication = this;
	mProject = nullptr;
	mLibrary = nullptr;
	mDefaultStyle = style()->objectName();

	mPreferences.LoadDefaults();

	UpdateStyle();
}

lcApplication::~lcApplication()
{
	delete mProject;
	delete mLibrary;
	gApplication = nullptr;
}

void lcApplication::UpdateStyle()
{
	if (mPreferences.mColorTheme == lcColorTheme::Dark)
	{
		if (!QApplication::setStyle("fusion"))
			return;

		QPalette Palette = QApplication::palette();

		Palette.setColor(QPalette::Window, QColor(49, 52, 55));
		Palette.setColor(QPalette::WindowText, QColor(240, 240, 240));
		Palette.setColor(QPalette::Base, QColor(35, 38, 41));
		Palette.setColor(QPalette::AlternateBase, QColor(44, 47, 50));
		Palette.setColor(QPalette::ToolTipBase, QColor(224, 224, 244));
		Palette.setColor(QPalette::ToolTipText, QColor(58, 58, 58));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
		Palette.setColor(QPalette::PlaceholderText, QColor(100, 100, 100));
#endif
		Palette.setColor(QPalette::Text, QColor(224, 224, 224));
		Palette.setColor(QPalette::Button, QColor(45, 48, 51));
		Palette.setColor(QPalette::ButtonText, QColor(224, 224, 244));
		Palette.setColor(QPalette::Light, QColor(65, 65, 65));
		Palette.setColor(QPalette::Midlight, QColor(62, 62, 62));
		Palette.setColor(QPalette::Dark, QColor(35, 35, 35));
		Palette.setColor(QPalette::Mid, QColor(50, 50, 50));
		Palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
//		Palette.setColor(QPalette::Highlight, QColor(46, 108, 219));
		Palette.setColor(QPalette::Highlight, QColor(41, 128, 185));
		Palette.setColor(QPalette::HighlightedText, QColor(232, 232, 232));
		Palette.setColor(QPalette::Link, QColor(41, 128, 185));

		Palette.setColor(QPalette::Disabled, QPalette::Text, QColor(128, 128, 128));
		Palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128));
		Palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));

		QApplication::setPalette(Palette);

		QFile StylesheetFile(QLatin1String(":/stylesheet/stylesheet.qss"));

		if (StylesheetFile.open(QIODevice::ReadOnly))
		{
			QString Stylesheet = QString::fromLatin1(StylesheetFile.readAll());
			qApp->setStyleSheet(Stylesheet);
		}
	}
	else
	{
		QApplication::setStyle(mDefaultStyle);
		QApplication::setPalette(qApp->style()->standardPalette());
		qApp->setStyleSheet(QString());
	}
}

void lcApplication::SaveTabLayout() const
{
	if (!mProject || mProject->GetFileName().isEmpty())
		return;

	QSettings Settings;
	QByteArray TabLayout = gMainWindow->GetTabLayout();

	Settings.setValue(GetTabLayoutKey(), TabLayout);
}

QString lcApplication::GetTabLayoutKey() const
{
	if (mProject)
	{
		QString FileName = mProject->GetFileName();
		if (!FileName.isEmpty())
		{
			FileName.replace('\\', '?');
			FileName.replace('/', '?');
			return QString("TabLayouts/%1").arg(FileName);
		}
	}

	return QString();
}

void lcApplication::SetProject(Project* Project)
{
	SaveTabLayout();

	gMainWindow->RemoveAllModelTabs();

	if (gMainWindow->GetPreviewWidget())
		 gMainWindow->GetPreviewWidget()->ClearPreview();

	delete mProject;
	mProject = Project;

	Project->SetActiveModel(0);
	lcGetPiecesLibrary()->RemoveTemporaryPieces();

	if (mProject && !mProject->GetFileName().isEmpty() && mPreferences.mRestoreTabLayout)
	{
		QSettings Settings;
		QByteArray TabLayout = Settings.value(GetTabLayoutKey()).toByteArray();

		gMainWindow->RestoreTabLayout(TabLayout);
	}
}

void lcApplication::SetClipboard(const QByteArray& Clipboard)
{
	mClipboard = Clipboard;
	gMainWindow->UpdatePaste(!mClipboard.isEmpty());
}

void lcApplication::ExportClipboard(const QByteArray& Clipboard)
{
	QMimeData* MimeData = new QMimeData();

	MimeData->setData("application/vnd.leocad-clipboard", Clipboard);
	QApplication::clipboard()->setMimeData(MimeData);

	SetClipboard(Clipboard);
}

bool lcApplication::LoadPartsLibrary(const QList<QPair<QString, bool>>& LibraryPaths, bool OnlyUsePaths, bool ShowProgress)
{
	if (mLibrary == nullptr)
		mLibrary = new lcPiecesLibrary();

	if (!OnlyUsePaths)
	{
		char* EnvPath = getenv("LEOCAD_LIB");

		if (EnvPath && EnvPath[0])
			return mLibrary->Load(EnvPath, ShowProgress);

		QString CustomPath = lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);

		if (!CustomPath.isEmpty())
			return mLibrary->Load(CustomPath, ShowProgress);
	}

	for (const QPair<QString, bool>& LibraryPathEntry : LibraryPaths)
	{
		if (mLibrary->Load(LibraryPathEntry.first, ShowProgress))
		{
			if (LibraryPathEntry.second)
				mLibrary->SetOfficialPieces();

			return true;
		}
	}

	return false;
}

bool lcApplication::Initialize(QList<QPair<QString, bool>>& LibraryPaths, bool& ShowWindow)
{
	bool OnlyUseLibraryPaths = false;
	bool SaveImage = false;
	bool SaveWavefront = false;
	bool Save3DS = false;
	bool SaveCOLLADA = false;
	bool SaveHTML = false;
	bool SetCameraAngles = false;
	bool SetCameraPosition = false;
	bool Orthographic = false;
	bool SetFoV = false;
	bool SetZPlanes = false;
	bool SetFadeStepsColor = false;
	bool SetHighlightColor = false;
	bool FadeSteps = mPreferences.mFadeSteps;
	bool ImageHighlight = mPreferences.mHighlightNewParts;
	int ImageWidth = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
	int ImageHeight = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);
	int AASamples = lcGetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES);
	int StudLogo = lcGetProfileInt(LC_PROFILE_STUD_LOGO);
	int ImageStart = 0;
	int ImageEnd = 0;
	float CameraPosition[9] = {};
	float CameraLatitude = 0.0f, CameraLongitude = 0.0f;
	float FoV = 0.0f;
	float ZNear = 0.0f, ZFar = 0.0f;
	quint32 FadeStepsColor = mPreferences.mFadeStepsColor;
	quint32	HighlightColor = mPreferences.mHighlightNewPartsColor;
	QString ImageName;
	QString ModelName;
	QString CameraName;
	QString ViewpointName;
	QString ProjectName;
	QString SaveWavefrontName;
	QString Save3DSName;
	QString SaveCOLLADAName;
	QString SaveHTMLName;

	QStringList Arguments = arguments();
	const int NumArguments = Arguments.size();

	for (int ArgIdx = 1; ArgIdx < NumArguments; ArgIdx++)
	{
		const QString& Param = Arguments[ArgIdx];

		if (Param.isEmpty())
			continue;

		if (Param[0] != '-')
		{
			ProjectName = Param;
			continue;
		}

		auto ParseString = [&ArgIdx, &Arguments, NumArguments](QString& Value, bool Required)
		{
			if (ArgIdx < NumArguments - 1 && Arguments[ArgIdx + 1][0] != '-')
			{
				ArgIdx++;
				Value = Arguments[ArgIdx];
			}
			else if (Required)
				printf("Not enough parameters for the '%s' argument.\n", Arguments[ArgIdx].toLatin1().constData());
		};

		auto ParseInteger = [&ArgIdx, &Arguments, NumArguments](int& Value)
		{
			if (ArgIdx < NumArguments - 1 && Arguments[ArgIdx + 1][0] != '-')
			{
				bool Ok = false;
				ArgIdx++;
				int NewValue = Arguments[ArgIdx].toInt(&Ok);

				if (Ok)
					Value = NewValue;
				else
					printf("Invalid value specified for the '%s' argument.\n", Arguments[ArgIdx - 1].toLatin1().constData());
			}
			else
				printf("Not enough parameters for the '%s' argument.\n", Arguments[ArgIdx].toLatin1().constData());
		};

		auto ParseFloat = [&ArgIdx, &Arguments, NumArguments](float& Value)
		{
			if (ArgIdx < NumArguments - 1 && Arguments[ArgIdx + 1][0] != '-')
			{
				bool Ok = false;
				ArgIdx++;
				float NewValue = Arguments[ArgIdx].toFloat(&Ok);

				if (Ok)
				{
					Value = NewValue;
					return true;
				}
				else
					printf("Invalid value specified for the '%s' argument.\n", Arguments[ArgIdx - 1].toLatin1().constData());
			}
			else
				printf("Not enough parameters for the '%s' argument.\n", Arguments[ArgIdx].toLatin1().constData());

			return false;
		};

		auto ParseVector2 = [&ArgIdx, &Arguments, NumArguments](float& Value1, float& Value2)
		{
			if (ArgIdx < NumArguments - 2 && Arguments[ArgIdx + 1][0] != '-' && Arguments[ArgIdx + 2][0] != '-')
			{
				bool Ok1 = false, Ok2 = false;

				ArgIdx++;
				float NewValue1 = Arguments[ArgIdx].toFloat(&Ok1);
				ArgIdx++;
				float NewValue2 = Arguments[ArgIdx].toFloat(&Ok2);

				if (Ok1 && Ok2)
				{
					Value1 = NewValue1;
					Value2 = NewValue2;
					return true;
				}
				else
					printf("Invalid value specified for the '%s' argument.\n", Arguments[ArgIdx - 2].toLatin1().constData());
			}
			else
				printf("Not enough parameters for the '%s' argument.\n", Arguments[ArgIdx].toLatin1().constData());

			return false;
		};

		auto ParseFloatArray = [&ArgIdx, &Arguments, NumArguments](int Count, float* ValueArray)
		{
			if (ArgIdx + Count >= NumArguments)
			{
				printf("Not enough parameters for the '%s' argument.\n", Arguments[ArgIdx].toLatin1().constData());
				return false;
			}

			for (int ParseIndex = 0; ParseIndex < Count; ParseIndex++)
			{
				bool Ok = false;
				float NewValue = Arguments[ArgIdx+ParseIndex+1].toFloat(&Ok);

				if (Ok)
					*(ValueArray++) = NewValue;
				else
				{
					printf("Invalid value specified for the '%s' argument: '%s'.\n", Arguments[ArgIdx].toLatin1().constData(), Arguments[ArgIdx+ParseIndex+1].toLatin1().constData());
					ArgIdx += 1 + Count;
					return false;
				}
			}

			ArgIdx += Count;
			return true;
		};

		auto ParseColor = [&ArgIdx, &Arguments, NumArguments](quint32& Color)
		{
			if (ArgIdx + 1 >= NumArguments)
			{
				printf("Not enough parameters for the '%s' argument.\n", Arguments[ArgIdx].toLatin1().constData());
				return false;
			}

			QColor ParsedColor = QColor(Arguments[ArgIdx+1]);
			if (!ParsedColor.isValid())
			{
				printf("Invalid value specified for the '%s' argument: '%s'.\n", Arguments[ArgIdx].toLatin1().constData(), Arguments[ArgIdx+1].toLatin1().constData());
				return false;
			}

			Color = LC_RGBA(ParsedColor.red(), ParsedColor.green(), ParsedColor.blue(), ParsedColor.alpha());
			return true;
		};

		if (Param == QLatin1String("-l") || Param == QLatin1String("--libpath"))
		{
			QString LibPath;
			ParseString(LibPath, true);
			if (!LibPath.isEmpty())
			{
				LibraryPaths.clear();
				LibraryPaths += qMakePair<QString, bool>(LibPath, false);
				OnlyUseLibraryPaths = true;
			}
		}
		else if (Param == QLatin1String("-i") || Param == QLatin1String("--image"))
		{
			SaveImage = true;
			ParseString(ImageName, false);
		}
		else if (Param == QLatin1String("-w") || Param == QLatin1String("--width"))
			ParseInteger(ImageWidth);
		else if (Param == QLatin1String("-h") || Param == QLatin1String("--height"))
			ParseInteger(ImageHeight);
		else if (Param == QLatin1String("-f") || Param == QLatin1String("--from"))
			ParseInteger(ImageStart);
		else if (Param == QLatin1String("-t") || Param == QLatin1String("--to"))
			ParseInteger(ImageEnd);
		else if (Param == QLatin1String("-s") || Param == QLatin1String("--submodel"))
			ParseString(ModelName, true);
		else if (Param == QLatin1String("-c") || Param == QLatin1String("--camera"))
			ParseString(CameraName, true);
		else if (Param == QLatin1String("--viewpoint"))
			ParseString(ViewpointName, true);
		else if (Param == QLatin1String("--camera-angles"))
			SetCameraAngles = ParseVector2(CameraLatitude, CameraLongitude);
		else if (Param == QLatin1String("--camera-position"))
			SetCameraPosition = ParseFloatArray(9, CameraPosition);
		else if (Param == QLatin1String("--orthographic"))
			Orthographic = true;
		else if (Param == QLatin1String("--fov"))
			SetFoV = ParseFloat(FoV);
		else if (Param == QLatin1String("--zplanes"))
			SetZPlanes = ParseVector2(ZNear, ZFar);
		else if (Param == QLatin1String("--fade-steps"))
			FadeSteps = true;
		else if (Param == QLatin1String("--no-fade-steps"))
			FadeSteps = false;
		else if (Param == QLatin1String("--fade-steps-color"))
		{
			if (ParseColor(FadeStepsColor))
			{
				SetFadeStepsColor = true;
				FadeSteps = true;
			}
		}
		else if (Param == QLatin1String("--highlight"))
			ImageHighlight = true;
		else if (Param == QLatin1String("--no-highlight"))
			ImageHighlight = false;
		else if (Param == QLatin1String("--highlight-color"))
		{
			if (ParseColor(HighlightColor))
			{
				SetHighlightColor = true;
				ImageHighlight = true;
			}
		}
		else if (Param == QLatin1String("--shading"))
		{
			QString ShadingString;
			ParseString(ShadingString, true);

			if (ShadingString == QLatin1String("wireframe"))
				mPreferences.mShadingMode = lcShadingMode::Wireframe;
			else if (ShadingString == QLatin1String("flat"))
				mPreferences.mShadingMode = lcShadingMode::Flat;
			else if (ShadingString == QLatin1String("default"))
				mPreferences.mShadingMode = lcShadingMode::DefaultLights;
			else if (ShadingString == QLatin1String("full"))
				mPreferences.mShadingMode = lcShadingMode::Full;
		}
		else if (Param == QLatin1String("--line-width"))
			ParseFloat(mPreferences.mLineWidth);
		else if (Param == QLatin1String("--aa-samples"))
			ParseInteger(AASamples);
		else if (Param == QLatin1String("-sl") || Param == QLatin1String("--stud-logo"))
		{
			ParseInteger(StudLogo);
			if (StudLogo != lcGetProfileInt(LC_PROFILE_STUD_LOGO))
			{
				lcGetPiecesLibrary()->SetStudLogo(StudLogo, false);
			}
		}
		else if (Param == QLatin1String("-obj") || Param == QLatin1String("--export-wavefront"))
		{
			SaveWavefront = true;
			ParseString(SaveWavefrontName, false);
		}
		else if (Param == QLatin1String("-3ds") || Param == QLatin1String("--export-3ds"))
		{
			Save3DS = true;
			ParseString(Save3DSName, false);
		}
		else if (Param == QLatin1String("-dae") || Param == QLatin1String("--export-collada"))
		{
			SaveCOLLADA = true;
			ParseString(SaveCOLLADAName, false);
		}
		else if (Param == QLatin1String("-html") || Param == QLatin1String("--export-html"))
		{
			SaveHTML = true;
			ParseString(SaveHTMLName, false);
		}
		else if (Param == QLatin1String("-v") || Param == QLatin1String("--version"))
		{
#ifdef LC_CONTINUOUS_BUILD
			printf("LeoCAD Continuous Build " QT_STRINGIFY(LC_CONTINUOUS_BUILD) "\n");
#else
			printf("LeoCAD Version " LC_VERSION_TEXT "\n");
#endif
			printf("Compiled " __DATE__ "\n");

			ShowWindow = false;
			return true;
		}
		else if (Param == QLatin1String("-?") || Param == QLatin1String("--help"))
		{
			printf("Usage: leocad [options] [file]\n");
			printf("  [options] can be:\n");
			printf("  -l, --libpath <path>: Set the Parts Library location to path.\n");
			printf("  -i, --image <outfile.ext>: Save a picture in the format specified by ext and exit.\n");
			printf("  -w, --width <width>: Set the picture width.\n");
			printf("  -h, --height <height>: Set the picture height.\n");
			printf("  -f, --from <step>: Set the first step to save pictures.\n");
			printf("  -t, --to <step>: Set the last step to save pictures.\n");
			printf("  -s, --submodel <submodel>: Set the active submodel.\n");
			printf("  -c, --camera <camera>: Set the active camera.\n");
			printf("  -sl, --stud-logo <type>: Set the stud logo type 0 - 5, 0 is no logo.\n");
			printf("  --viewpoint <front|back|left|right|top|bottom|home>: Set the viewpoint.\n");
			printf("  --camera-angles <latitude> <longitude>: Set the camera angles in degrees around the model.\n");
			printf("  --camera-position <x> <y> <z> <tx> <ty> <tz> <ux> <uy> <uz>: Set the camera position, target and up vector.\n");
			printf("  --orthographic: Render images using an orthographic projection.\n");
			printf("  --fov <degrees>: Set the vertical field of view used to render images.\n");
			printf("  --zplanes <near> <far>: Set the near and far clipping planes used to render images.\n");
			printf("  --fade-steps: Render parts from prior steps faded.\n");
			printf("  --no-fade-steps: Do not render parts from prior steps faded.\n");
			printf("  --fade-steps-color <rgba>: Renderinng color for prior step parts (#AARRGGBB).\n");
			printf("  --highlight: Highlight parts in the steps they appear.\n");
			printf("  --no-highlight: Do not highlight parts in the steps they appear.\n");
			printf("  --highlight-color: Renderinng color for highlighted parts (#AARRGGBB).\n");
			printf("  --shading <wireframe|flat|default|full>: Select shading mode for rendering.\n");
			printf("  --line-width <width>: Set the with of the edge lines.\n");
			printf("  --aa-samples <count>: AntiAliasing sample size (1, 2, 4, or 8).\n");
			printf("  -obj, --export-wavefront <outfile.obj>: Export the model to Wavefront OBJ format.\n");
			printf("  -3ds, --export-3ds <outfile.3ds>: Export the model to 3D Studio 3DS format.\n");
			printf("  -dae, --export-collada <outfile.dae>: Export the model to COLLADA DAE format.\n");
			printf("  -html, --export-html <folder>: Create an HTML page for the model.\n");
			printf("  -v, --version: Output version information and exit.\n");
			printf("  -?, --help: Display this help message and exit.\n");
			printf("  \n");

			ShowWindow = false;
			return true;
		}
		else
			printf("Unknown parameter: '%s'\n", Param.toLatin1().constData());
	}

	gMainWindow = new lcMainWindow();
	lcLoadDefaultKeyboardShortcuts();
	lcLoadDefaultMouseShortcuts();

	ShowWindow = !SaveImage && !SaveWavefront && !Save3DS && !SaveCOLLADA && !SaveHTML;

	if (!LoadPartsLibrary(LibraryPaths, OnlyUseLibraryPaths, ShowWindow))
	{
		QString Message;

		if (mLibrary->LoadBuiltinPieces())
			Message = tr("LeoCAD could not find a compatible Parts Library so only a small number of parts will be available.\n\nPlease visit https://www.leocad.org for information on how to download and install a library.");
		else
			Message = tr("LeoCAD could not load Parts Library.\n\nPlease visit https://www.leocad.org for information on how to download and install a library.");

		if (ShowWindow)
			QMessageBox::information(gMainWindow, tr("LeoCAD"), Message);
		else
			fprintf(stderr, "%s", Message.toLatin1().constData());
	}

	gMainWindow->CreateWidgets(AASamples);

	Project* NewProject = new Project();
	SetProject(NewProject);

	if (ShowWindow && ProjectName.isEmpty() && lcGetProfileInt(LC_PROFILE_AUTOLOAD_MOSTRECENT))
		ProjectName = lcGetProfileString(LC_PROFILE_RECENT_FILE1);

	if (!ProjectName.isEmpty() && gMainWindow->OpenProject(ProjectName))
	{
		if (!ModelName.isEmpty())
			mProject->SetActiveModel(ModelName);

		View* ActiveView = gMainWindow->GetActiveView();

		if (!CameraName.isEmpty())
		{
			ActiveView->SetCamera(CameraName);

			if (!ViewpointName.isEmpty())
				printf("Warning: --viewpoint is ignored when --camera is set.\n");

			if (Orthographic)
				printf("Warning: --orthographic is ignored when --camera is set.\n");

			if (SetCameraAngles)
				printf("Warning: --camera-angles is ignored when --camera is set.\n");

			if (SetCameraPosition)
				printf("Warning: --camera-position is ignored when --camera is set.\n");
		}
		else
		{
			if (!ViewpointName.isEmpty())
			{
				if (ViewpointName == QLatin1String("front"))
					ActiveView->SetViewpoint(lcViewpoint::Front);
				else if (ViewpointName == QLatin1String("back"))
					ActiveView->SetViewpoint(lcViewpoint::Back);
				else if (ViewpointName == QLatin1String("top"))
					ActiveView->SetViewpoint(lcViewpoint::Top);
				else if (ViewpointName == QLatin1String("bottom"))
					ActiveView->SetViewpoint(lcViewpoint::Bottom);
				else if (ViewpointName == QLatin1String("left"))
					ActiveView->SetViewpoint(lcViewpoint::Left);
				else if (ViewpointName == QLatin1String("right"))
					ActiveView->SetViewpoint(lcViewpoint::Right);
				else if (ViewpointName == QLatin1String("home"))
					ActiveView->SetViewpoint(lcViewpoint::Home);
				else
					printf("Unknown viewpoint: '%s'\n", ViewpointName.toLatin1().constData());

				if (SetCameraAngles)
					printf("Warning: --camera-angles is ignored when --viewpoint is set.\n");

				if (SetCameraPosition)
					printf("Warning: --camera-position is ignored when --viewpoint is set.\n");
			}
			else if (SetCameraAngles)
			{
				ActiveView->SetCameraAngles(CameraLatitude, CameraLongitude);

				if (SetCameraPosition)
					printf("Warning: --camera-position is ignored when --camera-angles is set.\n");
			}
			else if (SetCameraPosition)
			{
				ActiveView->SetViewpoint(lcVector3(CameraPosition[0], CameraPosition[1], CameraPosition[2]),
				                         lcVector3(CameraPosition[3], CameraPosition[4], CameraPosition[5]),
				                         lcVector3(CameraPosition[6], CameraPosition[7], CameraPosition[8]));
			}

			ActiveView->SetProjection(Orthographic);

			if (SetFoV)
				ActiveView->GetCamera()->m_fovy = FoV;

			if (SetZPlanes)
			{
				lcCamera* Camera = ActiveView->GetCamera();

				Camera->m_zNear = ZNear;
				Camera->m_zFar = ZFar;
			}
		}

		if (SaveImage)
		{
			lcModel* ActiveModel;

			if (ModelName.isEmpty())
				ActiveModel = mProject->GetMainModel();
			else
				ActiveModel = mProject->GetActiveModel();

			if (ImageName.isEmpty())
				ImageName = mProject->GetImageFileName(true);

			if (ImageEnd < ImageStart)
				ImageEnd = ImageStart;
			else if (ImageStart > ImageEnd)
				ImageStart = ImageEnd;

			if ((ImageStart == 0) && (ImageEnd == 0))
				ImageStart = ImageEnd = ActiveModel->GetCurrentStep();
			else if ((ImageStart == 0) && (ImageEnd != 0))
				ImageStart = ImageEnd;
			else if ((ImageStart != 0) && (ImageEnd == 0))
				ImageEnd = ImageStart;

			if (ImageStart > 255)
				ImageStart = 255;

			if (ImageEnd > 255)
				ImageEnd = 255;

			QString Frame;

			if (ImageStart != ImageEnd)
			{
				QString Extension = QFileInfo(ImageName).suffix();
				Frame = ImageName.left(ImageName.length() - Extension.length() - 1) + QLatin1String("%1.") + Extension;
			}
			else
				Frame = ImageName;

			mPreferences.mFadeSteps = FadeSteps;
			if (SetFadeStepsColor)
				mPreferences.mFadeStepsColor = FadeStepsColor;
			mPreferences.mHighlightNewParts = ImageHighlight;
			if (SetHighlightColor)
				mPreferences.mHighlightNewPartsColor = HighlightColor;

			ActiveModel->SaveStepImages(Frame, ImageStart != ImageEnd, CameraName.isEmpty() && !SetCameraPosition, ImageWidth, ImageHeight, ImageStart, ImageEnd);
		}

		if (SaveWavefront)
		{
			QString FileName;

			if (!SaveWavefrontName.isEmpty())
				FileName = SaveWavefrontName;
			else
				FileName = ProjectName;

			QString Extension = QFileInfo(FileName).suffix().toLower();

			if (Extension.isEmpty())
			{
				FileName += ".obj";
			}
			else if (Extension != "obj")
			{
				FileName = FileName.left(FileName.length() - Extension.length() - 1);
				FileName += ".obj";
			}

			mProject->ExportWavefront(FileName);
		}

		if (Save3DS)
		{
			QString FileName;

			if (!Save3DSName.isEmpty())
				FileName = Save3DSName;
			else
				FileName = ProjectName;

			QString Extension = QFileInfo(FileName).suffix().toLower();

			if (Extension.isEmpty())
			{
				FileName += ".3ds";
			}
			else if (Extension != "3ds")
			{
				FileName = FileName.left(FileName.length() - Extension.length() - 1);
				FileName += ".3ds";
			}

			mProject->Export3DStudio(FileName);
		}

		if (SaveCOLLADA)
		{
			QString FileName;

			if (!SaveCOLLADAName.isEmpty())
				FileName = SaveCOLLADAName;
			else
				FileName = ProjectName;

			QString Extension = QFileInfo(FileName).suffix().toLower();

			if (Extension.isEmpty())
			{
				FileName += ".dae";
			}
			else if (Extension != "dae")
			{
				FileName = FileName.left(FileName.length() - Extension.length() - 1);
				FileName += ".dae";
			}

			mProject->ExportCOLLADA(FileName);
		}

		if (SaveHTML)
		{
			lcHTMLExportOptions Options(mProject);

			if (!SaveHTMLName.isEmpty())
				Options.PathName = SaveHTMLName;

			mProject->ExportHTML(Options);
		}
	}

	if (ShowWindow)
	{
		gMainWindow->SetColorIndex(lcGetColorIndex(7));
		gMainWindow->GetPartSelectionWidget()->SetDefaultPart();
		gMainWindow->UpdateRecentFiles();
		gMainWindow->show();
	}

	return true;
}

void lcApplication::Shutdown()
{
	delete mLibrary;
	mLibrary = nullptr;
}

void lcApplication::ShowPreferencesDialog()
{
	lcPreferencesDialogOptions Options;
	int CurrentAASamples = lcGetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES);
	int CurrentStudLogo = lcGetProfileInt(LC_PROFILE_STUD_LOGO);

	Options.Preferences = mPreferences;

	Options.LibraryPath = lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);
	Options.MinifigSettingsPath = lcGetProfileString(LC_PROFILE_MINIFIG_SETTINGS);
	Options.ColorConfigPath = lcGetProfileString(LC_PROFILE_COLOR_CONFIG);
	Options.POVRayPath = lcGetProfileString(LC_PROFILE_POVRAY_PATH);
	Options.LGEOPath = lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH);
	Options.DefaultAuthor = lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME);
	Options.Language = lcGetProfileString(LC_PROFILE_LANGUAGE);
	Options.CheckForUpdates = lcGetProfileInt(LC_PROFILE_CHECK_UPDATES);

	Options.AASamples = CurrentAASamples;
	Options.StudLogo = CurrentStudLogo;

	Options.Categories = gCategories;
	Options.CategoriesModified = false;
	Options.CategoriesDefault = false;

	Options.KeyboardShortcuts = gKeyboardShortcuts;
	Options.KeyboardShortcutsModified = false;
	Options.KeyboardShortcutsDefault = false;
	Options.MouseShortcuts = gMouseShortcuts;
	Options.MouseShortcutsModified = false;
	Options.MouseShortcutsDefault = false;

	lcPreviewPosition PreviewDockable = Options.Preferences.mPreviewPosition;

	lcQPreferencesDialog Dialog(gMainWindow, &Options);
	if (Dialog.exec() != QDialog::Accepted)
		return;

	bool LanguageChanged = Options.Language != lcGetProfileString(LC_PROFILE_LANGUAGE);
	bool LibraryChanged = Options.LibraryPath != lcGetProfileString(LC_PROFILE_PARTS_LIBRARY);
	bool ColorsChanged = Options.ColorConfigPath != lcGetProfileString(LC_PROFILE_COLOR_CONFIG);
	bool AAChanged = CurrentAASamples != Options.AASamples;
	bool StudLogoChanged = CurrentStudLogo != Options.StudLogo;

	mPreferences = Options.Preferences;

	mPreferences.SaveDefaults();
	UpdateStyle();

	lcSetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME, Options.DefaultAuthor);
	lcSetProfileString(LC_PROFILE_PARTS_LIBRARY, Options.LibraryPath);
	lcSetProfileString(LC_PROFILE_COLOR_CONFIG, Options.ColorConfigPath);
	lcSetProfileString(LC_PROFILE_MINIFIG_SETTINGS, Options.MinifigSettingsPath);
	lcSetProfileString(LC_PROFILE_POVRAY_PATH, Options.POVRayPath);
	lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, Options.LGEOPath);
	lcSetProfileString(LC_PROFILE_LANGUAGE, Options.Language);
	lcSetProfileInt(LC_PROFILE_CHECK_UPDATES, Options.CheckForUpdates);
	lcSetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES, Options.AASamples);
	lcSetProfileInt(LC_PROFILE_STUD_LOGO, Options.StudLogo);

	lcPreviewPosition Dockable = Options.Preferences.mPreviewPosition;
	if (PreviewDockable != Dockable)
		gMainWindow->TogglePreviewWidget(
			Dockable == lcPreviewPosition::Dockable);

	if (LanguageChanged || LibraryChanged || ColorsChanged || AAChanged)
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Some changes will only take effect the next time you start LeoCAD."));

	if (Options.CategoriesModified)
	{
		if (Options.CategoriesDefault)
			lcResetDefaultCategories();
		else
		{
			gCategories = Options.Categories;
			lcSaveDefaultCategories();
		}

		gMainWindow->UpdateCategories();
	}

	if (Options.KeyboardShortcutsModified)
	{
		if (Options.KeyboardShortcutsDefault)
			lcResetDefaultKeyboardShortcuts();
		else
		{
			gKeyboardShortcuts = Options.KeyboardShortcuts;
			lcSaveDefaultKeyboardShortcuts();
		}

		gMainWindow->UpdateShortcuts();
	}

	if (Options.MouseShortcutsModified)
	{
		if (Options.MouseShortcutsDefault)
			lcResetDefaultMouseShortcuts();
		else
		{
			gMouseShortcuts = Options.MouseShortcuts;
			lcSaveDefaultMouseShortcuts();
		}
	}

	if (StudLogoChanged)
	{
		lcSetProfileInt(LC_PROFILE_STUD_LOGO, Options.StudLogo);
		lcGetPiecesLibrary()->SetStudLogo(Options.StudLogo, true);
	}

	// TODO: printing preferences
	/*
	strcpy(opts.strFooter, m_strFooter);
	strcpy(opts.strHeader, m_strHeader);
	*/

	gMainWindow->SetShadingMode(Options.Preferences.mShadingMode);
	gMainWindow->UpdateAllViews();
}

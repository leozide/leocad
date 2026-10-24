#include "lc_global.h"
#include "lc_viewwidget.h"
#include "lc_glextensions.h"
#include "project.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_mainwindow.h"
#include "lc_context.h"
#include "lc_view.h"
#include "lc_texture.h"
#include "lc_mesh.h"
#include "lc_profile.h"
#include "lc_previewwidget.h"
#include "lc_qutils.h"
#include "lc_viewsphere.h"

lcViewWidget::lcViewWidget(QWidget* Parent, lcView* View)
	: QOpenGLWidget(Parent), mView(View)
{
	mView->SetWidget(this);

	setMouseTracking(true);

	if (View->GetViewType() == lcViewType::View)
	{
		setFocusPolicy(Qt::StrongFocus);
		setAcceptDrops(true);

		mOverlay = new QWidget(this);
		mOverlay->setObjectName(QStringLiteral("ViewOverlay"));
		mOverlayLayout = new QVBoxLayout(mOverlay);
		mOverlayLayout->setContentsMargins(0, 0, 0, 0);
		mOverlayLayout->setSpacing(2);
		mOverlayLayout->setSizeConstraint(QLayout::SetNoConstraint);
		mOverlay->installEventFilter(this);

		mModelEditingBar = new QWidget(mOverlay);
		mModelEditingBar->setObjectName(QStringLiteral("ModelEditingBar"));
		mModelEditingBar->setAutoFillBackground(true);
		QPalette Palette = palette();
		Palette.setColor(QPalette::Window, QApplication::palette().color(QPalette::Button));
		mModelEditingBar->setPalette(Palette);
		mModelEditingBar->setWhatsThis(tr("<p><b>In-place Editing</b></p><p>The path shows the model being edited inside this view. The last name is the current editing target. Choose an earlier name to edit that ancestor, or the close button to return to the tab's model.</p>"));

		QHBoxLayout* EditingLayout = new QHBoxLayout(mModelEditingBar);
		EditingLayout->setContentsMargins(5, 3, 5, 3);

		mModelEditingLabel = new QLabel(mModelEditingBar);
		mModelEditingLabel->setObjectName(QStringLiteral("ModelEditingPath"));
		mModelEditingLabel->setTextFormat(Qt::RichText);
		mModelEditingLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
		mModelEditingLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		mModelEditingLabel->setMinimumWidth(0);
		EditingLayout->addWidget(mModelEditingLabel, 1);

		mEndEditingButton = new QToolButton(mModelEditingBar);
		mEndEditingButton->setIcon(QIcon(":/stylesheet/close.svg"));
		mEndEditingButton->setAccessibleName(tr("End Editing"));
		mEndEditingButton->setToolTip(tr("Return to editing the tab's model."));
		mEndEditingButton->setAutoRaise(true);
		EditingLayout->addWidget(mEndEditingButton);
		mOverlayLayout->addWidget(mModelEditingBar);

		connect(mModelEditingLabel, &QLabel::linkActivated, this, [this](const QString& Link)
		{
			bool Valid;
			const uint Depth = Link.toUInt(&Valid);

			if (!Valid || Depth + 1 >= static_cast<uint>(mModelEditingNames.size()))
				return;

			setFocus(Qt::OtherFocusReason);
			mView->SetSubmodelActive(Depth);
		});

		connect(mEndEditingButton, &QToolButton::clicked, this, [this]()
		{
			setFocus(Qt::OtherFocusReason);
			mView->SetTopSubmodelActive();
		});

		lcSetupWidgetHelp(mModelEditingBar, nullptr);
		UpdateModelEditingPath();
	}
}

lcViewWidget::~lcViewWidget()
{
	if (context())
		makeCurrent();

	mView.reset();

	if (context())
		doneCurrent();
}

QSize lcViewWidget::sizeHint() const
{
	return mPreferredSize.isEmpty() ? QOpenGLWidget::sizeHint() : mPreferredSize;
}

lcView* lcViewWidget::GetView() const
{
	return mView.get();
}

void lcViewWidget::SetView(lcView* View)
{
	if (context())
		makeCurrent();

	if (View)
	{
		if (context())
		{
			View->mContext->SetGLContext(context(), this);
		}

		View->SetWidget(this);
		const float Scale = GetDeviceScale();
		View->SetSize(width() * Scale, height() * Scale);

		if (hasFocus())
			View->SetFocus(true);
	}

	mView = std::unique_ptr<lcView>(View);
	UpdateModelEditingPath();
}

void lcViewWidget::UpdateModelEditingPath()
{
	if (!mOverlay || !mView)
		return;

	QStringList Names;

	for (const lcModel* Model : mView->GetModelEditingPath())
		Names.append(Model->GetProperties().mFileName);

	if (Names != mModelEditingNames)
	{
		mModelEditingNames = Names;
		const QString Path = Names.join(QStringLiteral(" \u203a "));
		mModelEditingLabel->setToolTip(Path);
		mModelEditingLabel->setAccessibleName(tr("Editing model: %1").arg(Path));
		mModelEditingBar->setVisible(Names.size() > 1);
	}

	UpdateOverlayGeometry();
}

void lcViewWidget::SetFindReplaceWidget(QWidget* Widget)
{
	if (!mOverlay)
		return;

	mFindReplaceWidget = Widget;
	mOverlayLayout->addWidget(Widget);
	UpdateOverlayGeometry();
}

void lcViewWidget::UpdateOverlayGeometry()
{
	if (!mOverlay)
		return;

	const bool Editing = mModelEditingNames.size() > 1;

	mOverlay->setVisible(Editing || mFindReplaceWidget);

	if (!Editing && !mFindReplaceWidget)
		return;

	int Left = 1;
	int Right = width() - 1;
	const lcPreferences& Preferences = lcGetPreferences();

	if (Preferences.mViewSphereEnabled && Preferences.mViewSphereSize > 0)
	{
		if (Preferences.mViewSphereLocation == lcViewSphereLocation::TopLeft)
			Left += Preferences.mViewSphereSize + 2;
		else if (Preferences.mViewSphereLocation == lcViewSphereLocation::TopRight)
			Right -= Preferences.mViewSphereSize + 2;
	}

	int PreferredWidth = 0;

	if (Editing)
	{
		QFont Font = mModelEditingLabel->font();
		Font.setBold(true);
		const QFontMetrics Metrics(Font);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
		PreferredWidth = Metrics.horizontalAdvance(mModelEditingNames.join(QStringLiteral(" \u203a ")));
#else
		PreferredWidth = Metrics.width(mModelEditingNames.join(QStringLiteral(" \u203a ")));
#endif
		PreferredWidth += mEndEditingButton->sizeHint().width() + mModelEditingBar->layout()->spacing() + 10;
	}

	if (mFindReplaceWidget)
		PreferredWidth = lcMax(PreferredWidth, mFindReplaceWidget->sizeHint().width());

	const int OverlayWidth = lcClamp(PreferredWidth, 1, lcMax(1, Right - Left));

	mOverlay->setGeometry(Left, 1, OverlayWidth, mOverlayLayout->sizeHint().height());
	mOverlayLayout->activate();
	mModelEditingBar->layout()->activate();

	UpdateModelEditingLabel();
}

void lcViewWidget::UpdateModelEditingLabel()
{
	if (mModelEditingNames.size() < 2)
		return;

	const QString Separator = QStringLiteral(" \u203a ");
	QFont Font = mModelEditingLabel->font();
	Font.setBold(true);
	const QFontMetrics Metrics(Font);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
	const int SeparatorWidth = Metrics.horizontalAdvance(Separator);
	const int PathWidth = Metrics.horizontalAdvance(mModelEditingNames.join(Separator));
#else
	const int SeparatorWidth = Metrics.width(Separator);
	const int PathWidth = Metrics.width(mModelEditingNames.join(Separator));
#endif
	const int NameCount = static_cast<int>(mModelEditingNames.size());
	const int NameWidth = lcMax(0, (mModelEditingLabel->width() - SeparatorWidth * (NameCount - 1)) / NameCount);
	QStringList Segments;

	for (int Index = 0; Index < mModelEditingNames.size(); Index++)
	{
		QString Name = mModelEditingNames[Index];

		if (PathWidth > mModelEditingLabel->width())
			Name = Metrics.elidedText(Name, Qt::ElideMiddle, NameWidth);

		Name = Name.toHtmlEscaped();

		if (Index + 1 == mModelEditingNames.size())
			Segments.append(QStringLiteral("<b>%1</b>").arg(Name));
		else
			Segments.append(QStringLiteral("<a href=\"%1\">%2</a>").arg(Index).arg(Name));
	}

	mModelEditingLabel->setText(Segments.join(Separator));
}

bool lcViewWidget::eventFilter(QObject* Object, QEvent* Event)
{
	if (Object == mOverlay && Event->type() == QEvent::LayoutRequest)
		UpdateOverlayGeometry();

	return QOpenGLWidget::eventFilter(Object, Event);
}

void lcViewWidget::resizeEvent(QResizeEvent* Event)
{
	QOpenGLWidget::resizeEvent(Event);
	UpdateOverlayGeometry();
}

void lcViewWidget::UpdateMousePosition()
{
	QPoint MousePosition = mapFromGlobal(QCursor::pos());
	float DeviceScale = GetDeviceScale();

	mView->SetMousePosition(MousePosition.x() * DeviceScale, mView->GetHeight() - MousePosition.y() * DeviceScale - 1);
	mView->SetMouseModifiers(QApplication::keyboardModifiers());
}

void lcViewWidget::initializeGL()
{
	mView->mContext->SetGLContext(context(), this);
}

void lcViewWidget::resizeGL(int Width, int Height)
{
	const float Scale = GetDeviceScale();

	mView->SetSize(Width * Scale, Height * Scale);
}

void lcViewWidget::paintGL()
{
	mView->OnDraw();
}

bool lcViewWidget::event(QEvent* Event)
{
	if (Event->type() == QEvent::NativeGesture)
	{
		QNativeGestureEvent* GestureEvent = static_cast<QNativeGestureEvent*>(Event);

		switch (GestureEvent->gestureType())
		{
		case Qt::ZoomNativeGesture:
			mView->Zoom(GestureEvent->value() * 100.0f);
			break;

		case Qt::SmartZoomNativeGesture:
			mView->ZoomExtents(lcGeometryBoundsMode::IncludeFallback, false);
			break;

		default:
			break;
		}
	}

	return QOpenGLWidget::event(Event);
}

void lcViewWidget::focusInEvent(QFocusEvent* FocusEvent)
{
	if (mView)
		mView->SetFocus(true);

	QOpenGLWidget::focusInEvent(FocusEvent);
}

void lcViewWidget::focusOutEvent(QFocusEvent* FocusEvent)
{
	if (mView)
		mView->SetFocus(false);

	QOpenGLWidget::focusOutEvent(FocusEvent);
}

void lcViewWidget::keyPressEvent(QKeyEvent* KeyEvent)
{
	if (KeyEvent->key() == Qt::Key_Control || KeyEvent->key() == Qt::Key_Shift)
	{
		mView->SetMouseModifiers(KeyEvent->modifiers());
		mView->UpdateCursor();
	}

	QOpenGLWidget::keyPressEvent(KeyEvent);
}

void lcViewWidget::keyReleaseEvent(QKeyEvent* KeyEvent)
{
	if (KeyEvent->key() == Qt::Key_Control || KeyEvent->key() == Qt::Key_Shift)
	{
		mView->SetMouseModifiers(KeyEvent->modifiers());
		mView->UpdateCursor();
	}

	QOpenGLWidget::keyReleaseEvent(KeyEvent);
}

void lcViewWidget::mousePressEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mView->SetMousePosition(MouseEvent->x() * DeviceScale, mView->GetHeight() - MouseEvent->y() * DeviceScale - 1);
	mView->SetMouseModifiers(MouseEvent->modifiers());

	switch (MouseEvent->button())
	{
	case Qt::LeftButton:
		mView->OnLeftButtonDown();
		break;

	case Qt::MiddleButton:
		mView->OnMiddleButtonDown();
		break;

	case Qt::RightButton:
		mView->OnRightButtonDown();
		break;

	case Qt::BackButton:
		mView->OnBackButtonDown();
		break;

	case Qt::ForwardButton:
		mView->OnForwardButtonDown();
		break;

	default:
		break;
	}
}

void lcViewWidget::mouseReleaseEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mView->SetMousePosition(MouseEvent->x() * DeviceScale, mView->GetHeight() - MouseEvent->y() * DeviceScale - 1);
	mView->SetMouseModifiers(MouseEvent->modifiers());

	switch (MouseEvent->button())
	{
	case Qt::LeftButton:
		mView->OnLeftButtonUp();
		break;

	case Qt::MiddleButton:
		mView->OnMiddleButtonUp();
		break;

	case Qt::RightButton:
		mView->OnRightButtonUp();
		break;

	case Qt::BackButton:
		mView->OnBackButtonUp();
		break;

	case Qt::ForwardButton:
		mView->OnForwardButtonUp();
		break;

	default:
		break;
	}
}

void lcViewWidget::mouseDoubleClickEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mView->SetMousePosition(MouseEvent->x() * DeviceScale, mView->GetHeight() - MouseEvent->y() * DeviceScale - 1);
	mView->SetMouseModifiers(MouseEvent->modifiers());

	switch (MouseEvent->button())
	{
	case Qt::LeftButton:
		mView->OnLeftButtonDoubleClick();
		break;

	default:
		break;
	}
}

void lcViewWidget::mouseMoveEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mView->SetMousePosition(MouseEvent->x() * DeviceScale, mView->GetHeight() - MouseEvent->y() * DeviceScale - 1);
	mView->SetMouseModifiers(MouseEvent->modifiers());

	mView->OnMouseMove();
}

void lcViewWidget::wheelEvent(QWheelEvent* WheelEvent)
{
	if (WheelEvent->source() == Qt::MouseEventSynthesizedBySystem)
	{
		switch (WheelEvent->phase())
		{
		case Qt::NoScrollPhase:
			break;

		case Qt::ScrollBegin:
			mView->StartPanGesture();
			WheelEvent->accept();
			return;

		case Qt::ScrollUpdate:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
		case Qt::ScrollMomentum:
#endif
			mView->UpdatePanGesture(WheelEvent->pixelDelta().x(), -WheelEvent->pixelDelta().y());
			WheelEvent->accept();
			return;

		case Qt::ScrollEnd:
			mView->EndPanGesture(true);
			WheelEvent->accept();
			return;
		}
	}

	if (WheelEvent->angleDelta().y() == 0)
	{
		WheelEvent->ignore();
		return;
	}

	float DeviceScale = GetDeviceScale();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	mView->SetMousePosition(WheelEvent->position().x() * DeviceScale, mView->GetHeight() - WheelEvent->position().y() * DeviceScale - 1);
#else
	mView->SetMousePosition(WheelEvent->x() * DeviceScale, mView->GetHeight() - WheelEvent->y() * DeviceScale - 1);
#endif
	mView->SetMouseModifiers(WheelEvent->modifiers());

	mWheelAccumulator += WheelEvent->angleDelta().y();
	int Steps = mWheelAccumulator / 8;

	if (Steps)
	{
		mView->OnMouseWheel(Steps / 15);
		mWheelAccumulator -= Steps * 8;
	}

	WheelEvent->accept();
}

void lcViewWidget::leaveEvent(QEvent* Event)
{
	mView->OnMouseLeave();

	QOpenGLWidget::leaveEvent(Event);
}

void lcViewWidget::dragEnterEvent(QDragEnterEvent* DragEnterEvent)
{
	const QMimeData* MimeData = DragEnterEvent->mimeData();

	if (MimeData->hasFormat("application/vnd.leocad-part"))
	{
		DragEnterEvent->acceptProposedAction();
		mView->BeginDrag(lcDragState::Piece);
		return;
	}
	else if (MimeData->hasFormat("application/vnd.leocad-color"))
	{
		DragEnterEvent->acceptProposedAction();
		mView->BeginDrag(lcDragState::Color);
		return;
	}

	DragEnterEvent->ignore();
}

void lcViewWidget::dragLeaveEvent(QDragLeaveEvent* DragLeaveEvent)
{
	mView->EndDrag(false);
	DragLeaveEvent->accept();
}

void lcViewWidget::dragMoveEvent(QDragMoveEvent* DragMoveEvent)
{
	const QMimeData* MimeData = DragMoveEvent->mimeData();

	if (MimeData->hasFormat("application/vnd.leocad-part") || MimeData->hasFormat("application/vnd.leocad-color"))
	{
		float DeviceScale = GetDeviceScale();

		mView->SetMousePosition(DragMoveEvent->pos().x() * DeviceScale, mView->GetHeight() - DragMoveEvent->pos().y() * DeviceScale - 1);
		mView->SetMouseModifiers(DragMoveEvent->keyboardModifiers());

		mView->OnMouseMove();

		DragMoveEvent->accept();
		return;
	}

	QOpenGLWidget::dragMoveEvent(DragMoveEvent);
}

void lcViewWidget::dropEvent(QDropEvent* DropEvent)
{
	const QMimeData* MimeData = DropEvent->mimeData();

	if (MimeData->hasFormat("application/vnd.leocad-part") || MimeData->hasFormat("application/vnd.leocad-color"))
	{
		mView->EndDrag(true);
		window()->activateWindow();
		setFocus(Qt::MouseFocusReason);

		DropEvent->accept();
		return;
	}

	QOpenGLWidget::dropEvent(DropEvent);
}

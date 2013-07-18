#include "lc_global.h"
#include "lc_qglwidget.h"
#include "glwindow.h"
#include "project.h"
#include "lc_library.h"
#include "lc_application.h"

lcQGLWidget::lcQGLWidget(QWidget *parent, lcQGLWidget *share, GLWindow *window, bool view)
	: QGLWidget(parent, share)
{
	mWindow = window;
	mWindow->mWidget = this;

	mWindow->MakeCurrent();
	mWindow->OnInitialUpdate();

	preferredSize = QSize(0, 0);
	setMouseTracking(true);

	isView = view;
	if (isView)
	{
		setFocusPolicy(Qt::StrongFocus);
		setAcceptDrops(true);
	}
}

lcQGLWidget::~lcQGLWidget()
{
	if (isView)
		delete mWindow;
}

QSize lcQGLWidget::sizeHint() const
{
	if (preferredSize.isEmpty())
		return QGLWidget::sizeHint();
	else
		return preferredSize;
}

void lcQGLWidget::initializeGL()
{
}

void lcQGLWidget::resizeGL(int width, int height)
{
	mWindow->mWidth = width;
	mWindow->mHeight = height;
}

void lcQGLWidget::paintGL()
{
	mWindow->OnDraw();
}

void lcQGLWidget::keyPressEvent(QKeyEvent *event)
{
	if (isView && event->key() == Qt::Key_Control)
	{
		mWindow->mInputState.Control = true;
		mWindow->OnUpdateCursor();
	}

	QGLWidget::keyPressEvent(event);
}

void lcQGLWidget::keyReleaseEvent(QKeyEvent *event)
{
	if (isView && event->key() == Qt::Key_Control)
	{
		mWindow->mInputState.Control = false;
		mWindow->OnUpdateCursor();
	}

	QGLWidget::keyReleaseEvent(event);
}

void lcQGLWidget::mousePressEvent(QMouseEvent *event)
{
	mWindow->mInputState.x = event->x();
	mWindow->mInputState.y = height() - event->y() - 1;
	mWindow->mInputState.Control = (event->modifiers() & Qt::ControlModifier) != 0;
	mWindow->mInputState.Shift = (event->modifiers() & Qt::ShiftModifier) != 0;
	mWindow->mInputState.Alt = (event->modifiers() & Qt::AltModifier) != 0;

	switch (event->button())
	{
	case Qt::LeftButton:
		mWindow->OnLeftButtonDown();
		break;
	case Qt::MidButton:
		mWindow->OnMiddleButtonDown();
		break;
	case Qt::RightButton:
		mWindow->OnRightButtonDown();
		break;
	default:
		break;
	}
}

void lcQGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
	mWindow->mInputState.x = event->x();
	mWindow->mInputState.y = height() - event->y() - 1;
	mWindow->mInputState.Control = (event->modifiers() & Qt::ControlModifier) != 0;
	mWindow->mInputState.Shift = (event->modifiers() & Qt::ShiftModifier) != 0;
	mWindow->mInputState.Alt = (event->modifiers() & Qt::AltModifier) != 0;

	switch (event->button())
	{
	case Qt::LeftButton:
		mWindow->OnLeftButtonUp();
		break;
	case Qt::MidButton:
		mWindow->OnMiddleButtonUp();
		break;
	case Qt::RightButton:
		mWindow->OnRightButtonUp();
		break;
	default:
		break;
	}
}

void lcQGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	mWindow->mInputState.x = event->x();
	mWindow->mInputState.y = height() - event->y() - 1;
	mWindow->mInputState.Control = (event->modifiers() & Qt::ControlModifier) != 0;
	mWindow->mInputState.Shift = (event->modifiers() & Qt::ShiftModifier) != 0;
	mWindow->mInputState.Alt = (event->modifiers() & Qt::AltModifier) != 0;

	mWindow->OnMouseMove();
}

void lcQGLWidget::wheelEvent(QWheelEvent *event)
{
	if (event->orientation() != Qt::Vertical)
	{
		event->ignore();
		return;
	}

	mWindow->mInputState.x = event->x();
	mWindow->mInputState.y = height() - event->y() - 1;
	mWindow->mInputState.Control = (event->modifiers() & Qt::ControlModifier) != 0;
	mWindow->mInputState.Shift = (event->modifiers() & Qt::ShiftModifier) != 0;
	mWindow->mInputState.Alt = (event->modifiers() & Qt::AltModifier) != 0;

	int numDegrees = event->delta() / 8;
	int numSteps = numDegrees / 15;

	mWindow->OnMouseWheel(numSteps);

	event->accept();
}

void lcQGLWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (isView && event->mimeData()->hasFormat("application/vnd.leocad-part"))
	{
		event->acceptProposedAction();

		QByteArray pieceData = event->mimeData()->data("application/vnd.leocad-part");
		QDataStream dataStream(&pieceData, QIODevice::ReadOnly);
		QString id;

		dataStream >> id;

		lcGetActiveProject()->BeginPieceDrop(lcGetPiecesLibrary()->FindPiece(id.toLocal8Bit().data(), false));
	}
	else
		event->ignore();
}

void lcQGLWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	if (!isView)
		return;

	lcGetActiveProject()->EndPieceDrop(false);

	event->accept();
}

void lcQGLWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (!isView || !event->mimeData()->hasFormat("application/vnd.leocad-part"))
		return;

	lcGetActiveProject()->OnPieceDropMove(event->pos().x(), height() - event->pos().y() - 1);

	event->accept();
}

void lcQGLWidget::dropEvent(QDropEvent *event)
{
	if (!isView || !event->mimeData()->hasFormat("application/vnd.leocad-part"))
		return;

	lcGetActiveProject()->EndPieceDrop(true);

	event->accept();
}

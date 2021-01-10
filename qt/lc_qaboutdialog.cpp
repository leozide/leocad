#include "lc_global.h"
#include "lc_qaboutdialog.h"
#include "ui_lc_qaboutdialog.h"
#include "lc_mainwindow.h"
#include "lc_view.h"
#include "lc_glextensions.h"
#include "lc_viewwidget.h"

lcQAboutDialog::lcQAboutDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::lcQAboutDialog)
{
	ui->setupUi(this);

#ifdef LC_CONTINUOUS_BUILD
	ui->version->setText(tr("LeoCAD Continuous Build %1").arg(QString::fromLatin1(QT_STRINGIFY(LC_CONTINUOUS_BUILD))));
#else
	ui->version->setText(tr("LeoCAD Version %1").arg(QString::fromLatin1(LC_VERSION_TEXT)));
#endif

	lcViewWidget* Widget = gMainWindow->GetActiveView()->GetWidget();
	QSurfaceFormat Format = Widget->context()->format();

	int ColorDepth = Format.redBufferSize() + Format.greenBufferSize() + Format.blueBufferSize() + Format.alphaBufferSize();

	const QString QtVersionFormat = tr("Qt Version %1 (compiled with %2)\n\n");
	const QString QtVersion = QtVersionFormat.arg(qVersion(), QT_VERSION_STR);
	const QString VersionFormat = tr("OpenGL Version %1 (GLSL %2)\n%3 - %4\n\n");
	const QString Version = VersionFormat.arg(QString((const char*)glGetString(GL_VERSION)), QString((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION)), QString((const char*)glGetString(GL_RENDERER)), QString((const char*)glGetString(GL_VENDOR)));
	const QString BuffersFormat = tr("Color Buffer: %1 bits\nDepth Buffer: %2 bits\nStencil Buffer: %3 bits\n\n");
	const QString Buffers = BuffersFormat.arg(QString::number(ColorDepth), QString::number(Format.depthBufferSize()), QString::number(Format.stencilBufferSize()));

	const QString ExtensionsFormat = tr("Buffers: %1\nShaders: %2\nFramebuffers: %3\nBlendFuncSeparate: %4\nAnisotropic: %5\n");
	const QString VertexBuffers = gSupportsVertexBufferObject ? tr("Supported") : tr("Not supported");
	const QString Shaders = gSupportsShaderObjects ? tr("Supported") : tr("Not supported");
	const QString Framebuffers = gSupportsFramebufferObject ? tr("Supported") : tr("Not supported");
	const QString BlendFuncSeparate = gSupportsBlendFuncSeparate ? tr("Supported") : tr("Not supported");
	const QString Anisotropic = gSupportsAnisotropic ? tr("Supported (max %1)").arg(gMaxAnisotropy) : tr("Not supported");

	const QString Extensions = ExtensionsFormat.arg(VertexBuffers, Shaders, Framebuffers, BlendFuncSeparate, Anisotropic);

	ui->info->setText(QtVersion + Version + Buffers + Extensions);
}

lcQAboutDialog::~lcQAboutDialog()
{
	delete ui;
}

#include "lc_global.h"
#include "lc_qaboutdialog.h"
#include "ui_lc_qaboutdialog.h"
#include "lc_mainwindow.h"
#include "view.h"
#include "lc_glextensions.h"

lcQAboutDialog::lcQAboutDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::lcQAboutDialog)
{
	ui->setupUi(this);

	ui->version->setText(tr("LeoCAD Version %1").arg(QString::fromLatin1(LC_VERSION_TEXT)));

	QGLWidget* Widget = (QGLWidget*)gMainWindow->GetActiveView()->mWidget;
	QGLFormat Format = Widget->context()->format();

	int ColorDepth = Format.redBufferSize() + Format.greenBufferSize() + Format.blueBufferSize() + Format.alphaBufferSize();

	QString VersionFormat = tr("OpenGL Version %1 (GLSL %2)\n%3 - %4\n\n");
	QString Version = VersionFormat.arg(QString((const char*)glGetString(GL_VERSION)), QString((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION)), QString((const char*)glGetString(GL_RENDERER)), QString((const char*)glGetString(GL_VENDOR)));
	QString BuffersFormat = tr("Color Buffer: %1 bits %2 %3\nDepth Buffer: %4 bits\nStencil Buffer: %5 bits\n\n");
	QString Buffers = BuffersFormat.arg(QString::number(ColorDepth), Format.rgba() ? "RGBA" : tr("indexed"), Format.doubleBuffer() ? tr("double buffered") : QString(), QString::number(Format.depthBufferSize()), QString::number(Format.stencilBufferSize()));

	QString ExtensionsFormat = tr("GL_ARB_vertex_buffer_object extension: %1\nGL_ARB_framebuffer_object extension: %2\nGL_EXT_framebuffer_object extension: %3\nGL_EXT_blend_func_separate: %4\nGL_EXT_texture_filter_anisotropic extension: %5\n");
	QString VertexBufferObject = gSupportsVertexBufferObject ? tr("Supported") : tr("Not supported");
	QString FramebufferObjectARB = gSupportsFramebufferObjectARB ? tr("Supported") : tr("Not supported");
	QString FramebufferObjectEXT = gSupportsFramebufferObjectEXT ? tr("Supported") : tr("Not supported");
	QString BlendFuncSeparateEXT = gSupportsBlendFuncSeparate ? tr("Supported") : tr("Not supported");
	QString Anisotropic = gSupportsAnisotropic ? tr("Supported (max %1)").arg(gMaxAnisotropy) : tr("Not supported");
	QString Extensions = ExtensionsFormat.arg(VertexBufferObject, FramebufferObjectARB, FramebufferObjectEXT, BlendFuncSeparateEXT, Anisotropic);

	ui->info->setText(Version + Buffers + Extensions);
}

lcQAboutDialog::~lcQAboutDialog()
{
	delete ui;
}

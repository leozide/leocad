#include "lc_global.h"
#include "lc_qaboutdialog.h"
#include "ui_lc_qaboutdialog.h"
#include "lc_mainwindow.h"
#include "preview.h"

lcQAboutDialog::lcQAboutDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQAboutDialog)
{
	ui->setupUi(this);

	ui->version->setText(tr("LeoCAD Version %1").arg(QString::fromLatin1(LC_VERSION_TEXT)));

	gMainWindow->mPreviewWidget->MakeCurrent();

	GLint Red, Green, Blue, Alpha, Depth, Stencil;
	GLboolean DoubleBuffer, RGBA;

	glGetIntegerv(GL_RED_BITS, &Red);
	glGetIntegerv(GL_GREEN_BITS, &Green);
	glGetIntegerv(GL_BLUE_BITS, &Blue);
	glGetIntegerv(GL_ALPHA_BITS, &Alpha);
	glGetIntegerv(GL_DEPTH_BITS, &Depth);
	glGetIntegerv(GL_STENCIL_BITS, &Stencil);
	glGetBooleanv(GL_DOUBLEBUFFER, &DoubleBuffer);
	glGetBooleanv(GL_RGBA_MODE, &RGBA);

	QString VersionFormat = tr("OpenGL Version %1\n%2 - %3\n\n");
	QString Version = VersionFormat.arg(QString((const char*)glGetString(GL_VERSION)), QString((const char*)glGetString(GL_RENDERER)), QString((const char*)glGetString(GL_VENDOR)));
	QString BuffersFormat = tr("Color Buffer: %1 bits %2 %3\nDepth Buffer: %4 bits\nStencil Buffer: %5 bits\n\n");
	QString Buffers = BuffersFormat.arg(QString::number(Red + Green + Blue + Alpha), RGBA ? "RGBA" : tr("indexed"), DoubleBuffer ? tr("double buffered") : QString(), QString::number(Depth), QString::number(Stencil));

	QString ExtensionsFormat = tr("GL_ARB_vertex_buffer_object extension: %1\nGL_ARB_framebuffer_object extension: %2\nGL_EXT_framebuffer_object extension: %3\nGL_EXT_texture_filter_anisotropic extension: %4\n");
	QString VertexBufferObject = GL_HasVertexBufferObject() ? tr("Supported") : tr("Not supported");
	QString FramebufferObjectARB = GL_HasFramebufferObjectARB() ? tr("Supported") : tr("Not supported");
	QString FramebufferObjectEXT = GL_HasFramebufferObjectEXT() ? tr("Supported") : tr("Not supported");
	QString Anisotropic = GL_SupportsAnisotropic ? tr("Supported (max %d)").arg(QString::number(GL_MaxAnisotropy)) : tr("Not supported");
	QString Extensions = ExtensionsFormat.arg(VertexBufferObject, FramebufferObjectARB, FramebufferObjectEXT, Anisotropic);

	ui->info->setText(Version + Buffers + Extensions);
}

lcQAboutDialog::~lcQAboutDialog()
{
	delete ui;
}

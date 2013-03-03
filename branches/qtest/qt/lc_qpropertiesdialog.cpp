#include "lc_global.h"
#include "lc_qpropertiesdialog.h"
#include "ui_lc_qpropertiesdialog.h"
#include "basewnd.h"
#include "lc_application.h"
#include "project.h"

lcQPropertiesDialog::lcQPropertiesDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQPropertiesDialog)
{
	ui->setupUi(this);

	options = (lcPropertiesDialogOptions*)data;

	Project *project = lcGetActiveProject();

	if (project->m_strTitle[0])
		setWindowTitle(project->m_strTitle + tr(" Properties"));

	QFileInfo fileInfo(project->m_strPathName);
	if (fileInfo.exists())
	{
		ui->fileName->setText(fileInfo.fileName());
		ui->location->setText(QDir::toNativeSeparators(fileInfo.absoluteDir().absolutePath()));
		ui->size->setText(QString(tr("%1 bytes")).arg(QString::number(fileInfo.size())));
		ui->created->setText(fileInfo.created().toString());
		ui->modified->setText(fileInfo.lastModified().toString());
		ui->accessed->setText(fileInfo.lastRead().toString());
	}

	ui->description->setText(options->Description);
	ui->author->setText(options->Author);
	ui->comments->setText(options->Comments);
}

lcQPropertiesDialog::~lcQPropertiesDialog()
{
	delete ui;
}

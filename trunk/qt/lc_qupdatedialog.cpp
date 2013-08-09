#include "lc_global.h"
#include "lc_qupdatedialog.h"
#include "ui_lc_qupdatedialog.h"
#include "lc_application.h"
#include "lc_library.h"

lcQUpdateDialog::lcQUpdateDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQUpdateDialog)
{
	ui->setupUi(this);

	if (!data)
	{
		ui->status->setText(tr("Connecting to update server..."));

		manager = new QNetworkAccessManager(this);
		connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

		updateReply = manager->get(QNetworkRequest(QUrl("http://www.leocad.org/updates.txt")));
	}
	else
	{
		manager = NULL;
		updateReply = NULL;
		parseUpdate((const char*)data);
	}
}

lcQUpdateDialog::~lcQUpdateDialog()
{
	if (updateReply)
	{
		updateReply->abort();
		updateReply->deleteLater();
	}

	if (manager)
		manager->deleteLater();

	delete ui;
}

void lcQUpdateDialog::cancel()
{
	if (updateReply)
	{
		updateReply->abort();
		updateReply->deleteLater();
		updateReply = NULL;
	}

	QDialog::accept();
}

void lcQUpdateDialog::replyFinished(QNetworkReply *reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{
		parseUpdate(reply->readAll());
		reply->deleteLater();
	}
	else
	{
		ui->status->setText(tr("Error connecting to the update server."));
	}

	updateReply = NULL;
}

void lcQUpdateDialog::parseUpdate(const char *update)
{
	int majorVersion, minorVersion, patchVersion;
	int parts;

	if (sscanf(update, "%d.%d.%d %d", &majorVersion, &minorVersion, &patchVersion, &parts) == 4)
	{
		QString status;
		bool updateAvailable = false;

		if (majorVersion > LC_VERSION_MAJOR)
			updateAvailable = true;
		else if (majorVersion == LC_VERSION_MAJOR)
		{
			if (minorVersion > LC_VERSION_MINOR)
				updateAvailable = true;
			else if (minorVersion == LC_VERSION_MINOR)
			{
				if (patchVersion > LC_VERSION_PATCH)
					updateAvailable = true;
			}
		}

		if (updateAvailable)
			status = QString(tr("<p>There's a newer version of LeoCAD available for download (%1.%2.%3).</p>")).arg(majorVersion, minorVersion, patchVersion);
		else
			status = tr("<p>You are using the latest LeoCAD version.</p>");

		lcPiecesLibrary* library = lcGetPiecesLibrary();

		if (library->mNumOfficialPieces)
		{
			if (parts > library->mNumOfficialPieces)
			{
				status += tr("<p>There are new parts available.</p>");
				updateAvailable = true;
			}
			else
				status += tr("<p>There are no new parts available at this time.</p>");
		}

		if (updateAvailable)
		{
			status += tr("<p>Visit <a href=\"http://www.leocad.org/files/\">http://www.leocad.org/files/</a> to download.</p>");
		}

		ui->status->setText(status);
	}
	else
		ui->status->setText(tr("Error parsing update information."));

	ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
}

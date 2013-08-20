#include "lc_global.h"
#include "lc_qupdatedialog.h"
#include "ui_lc_qupdatedialog.h"
#include "lc_application.h"
#include "lc_library.h"
#include "lc_profile.h"

void lcDoInitialUpdateCheck()
{
	int updateFrequency = lcGetProfileInt(LC_PROFILE_CHECK_UPDATES);

	if (updateFrequency == 0)
		return;

	QSettings settings;
	QDateTime checkTime = settings.value("Updates/LastCheck", QDateTime()).toDateTime();

	if (!checkTime.isNull())
	{
		checkTime.addDays(updateFrequency == 1 ? 1 : 7);

		if (checkTime > QDateTime::currentDateTimeUtc())
			return;
	}

	new lcQUpdateDialog(NULL, (void*)1);
}

lcQUpdateDialog::lcQUpdateDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQUpdateDialog)
{
	ui->setupUi(this);

	initialUpdate = (bool)data;
	connect(this, SIGNAL(finished(int)), this, SLOT(finished(int)));

	ui->status->setText(tr("Connecting to update server..."));

	manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

	updateReply = manager->get(QNetworkRequest(QUrl("http://www.leocad.org/updates.txt")));
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

void lcQUpdateDialog::accept()
{
	QSettings settings;
	settings.setValue("Updates/IgnoreVersion", versionData);

	QDialog::accept();
}

void lcQUpdateDialog::reject()
{
	if (updateReply)
	{
		updateReply->abort();
		updateReply->deleteLater();
		updateReply = NULL;
	}

	QDialog::reject();
}

void lcQUpdateDialog::finished(int result)
{
	if (initialUpdate)
		deleteLater();
}

void lcQUpdateDialog::replyFinished(QNetworkReply *reply)
{
	bool updateAvailable = false;

	if (reply->error() == QNetworkReply::NoError)
	{
		int majorVersion, minorVersion, patchVersion;
		int parts;

		versionData = reply->readAll();
		const char *update = versionData;

		QSettings settings;
		QByteArray ignoreUpdate = settings.value("Updates/IgnoreVersion", QByteArray()).toByteArray();

		if (initialUpdate && ignoreUpdate == versionData)
		{
			updateAvailable = false;
		}
		else if (sscanf(update, "%d.%d.%d %d", &majorVersion, &minorVersion, &patchVersion, &parts) == 4)
		{
			QString status;

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

		settings.setValue("Updates/LastCheck", QDateTime::currentDateTimeUtc());

		updateReply = NULL;
		reply->deleteLater();
	}
	else
		ui->status->setText(tr("Error connecting to the update server."));

	if (initialUpdate)
	{
		if (updateAvailable)
			show();
		else
			deleteLater();
	}

	if (updateAvailable)
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Close | QDialogButtonBox::Ignore);
	else
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
}

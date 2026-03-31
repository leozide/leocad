#include "lc_global.h"
#include "lc_updatedialog.h"
#include "ui_lc_updatedialog.h"
#include "lc_application.h"
#include "lc_library.h"
#include "lc_profile.h"
#include "lc_http.h"

void lcDoInitialUpdateCheck()
{
	int UpdateFrequency = lcGetProfileInt(LC_PROFILE_CHECK_UPDATES);

	if (UpdateFrequency == 0)
		return;

	QSettings Settings;
	QDateTime CheckTime = Settings.value("Updates/LastCheck", QDateTime()).toDateTime();

	if (!CheckTime.isNull())
	{
		QDateTime NextCheckTime = CheckTime.addDays(UpdateFrequency == 1 ? 1 : 7);

		if (NextCheckTime > QDateTime::currentDateTimeUtc())
			return;
	}

	new lcUpdateDialog(nullptr, true);
}

lcUpdateDialog::lcUpdateDialog(QWidget* Parent, bool InitialUpdate)
	: QDialog(Parent), ui(new Ui::lcUpdateDialog), mInitialUpdate(InitialUpdate)
{
	ui->setupUi(this);

	connect(this, SIGNAL(finished(int)), this, SLOT(finished(int)));

	ui->status->setText(tr("Connecting to update server..."));

	mHttpManager = new lcHttpManager(this);
	connect(mHttpManager, &lcHttpManager::DownloadFinished, this, &lcUpdateDialog::DownloadFinished);

	mHttpManager->DownloadFile(QLatin1String("https://www.leocad.org/updates.txt"));
}

lcUpdateDialog::~lcUpdateDialog()
{
	delete ui;
}

void lcUpdateDialog::accept()
{
	QSettings Settings;
	Settings.setValue("Updates/IgnoreVersion", mVersionData);

	QDialog::accept();
}

void lcUpdateDialog::finished(int result)
{
	Q_UNUSED(result);

	if (mInitialUpdate)
		deleteLater();
}

void lcUpdateDialog::DownloadFinished(lcHttpReply* Reply)
{
	bool UpdateAvailable = false;

	if (Reply->error() == QNetworkReply::NoError)
	{
		int MajorVersion, MinorVersion, PatchVersion;
		int Parts;

		mVersionData = Reply->readAll();
		const char* Update = mVersionData;

		QSettings Settings;
		QByteArray IgnoreUpdate = Settings.value("Updates/IgnoreVersion", QByteArray()).toByteArray();

		if (mInitialUpdate && IgnoreUpdate == mVersionData)
		{
			UpdateAvailable = false;
		}
		else if (sscanf(Update, "%d.%d.%d %d", &MajorVersion, &MinorVersion, &PatchVersion, &Parts) == 4)
		{
			QString Status;

			if (MajorVersion > LC_VERSION_MAJOR)
				UpdateAvailable = true;
			else if (MajorVersion == LC_VERSION_MAJOR)
			{
				if (MinorVersion > LC_VERSION_MINOR)
					UpdateAvailable = true;
				else if (MinorVersion == LC_VERSION_MINOR)
				{
					if (PatchVersion > LC_VERSION_PATCH)
						UpdateAvailable = true;
				}
			}

			if (UpdateAvailable)
				Status = QString(tr("<p>There's a newer version of LeoCAD available for download (%1.%2.%3).</p>")).arg(QString::number(MajorVersion), QString::number(MinorVersion), QString::number(PatchVersion));
			else
				Status = tr("<p>You are using the latest LeoCAD version.</p>");

			lcPiecesLibrary* Library = lcGetPiecesLibrary();

			if (Library->mNumOfficialPieces)
			{
				if (Parts > Library->mNumOfficialPieces)
				{
					Status += tr("<p>There are new parts available.</p>");
					UpdateAvailable = true;
				}
				else
					Status += tr("<p>There are no new parts available at this time.</p>");
			}

			if (UpdateAvailable)
			{
				Status += tr("<p>Visit <a href=\"https://github.com/leozide/leocad/releases\">https://github.com/leozide/leocad/releases</a> to download.</p>");
			}

			ui->status->setText(Status);
		}
		else
			ui->status->setText(tr("Error parsing update information."));

		Settings.setValue("Updates/LastCheck", QDateTime::currentDateTimeUtc());
	}
	else
		ui->status->setText(tr("Error connecting to the update server."));

	if (mInitialUpdate)
	{
		if (UpdateAvailable)
			show();
		else
			deleteLater();
	}

	if (UpdateAvailable)
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Close | QDialogButtonBox::Ignore);
	else
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
}

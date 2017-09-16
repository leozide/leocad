#pragma once

#include <QDialog>
#include <QNetworkAccessManager>

namespace Ui {
class lcSetsDatabaseDialog;
}

#ifdef Q_OS_WIN

#include <wininet.h>

class lcHttpReply : public QThread
{
	Q_OBJECT

public:
	lcHttpReply(QObject* Parent, const QString& URL);

	void run();

	bool error() const
	{
		return mError;
	}

	void abort()
	{
		mAbort = true;
	}

	QByteArray readAll() const
	{
		return mBuffer;
	}

protected:
	bool mError;
	bool mAbort;
	QByteArray mBuffer;
	QString mURL;
};

#else

typedef QNetworkReply lcHttpReply;

#endif


class lcSetsDatabaseDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcSetsDatabaseDialog(QWidget* Parent);
	~lcSetsDatabaseDialog();

	QString GetSetName() const;
	QString GetSetDescription() const;

	QByteArray GetSetInventory() const
	{
		return mInventory;
	}

	virtual bool eventFilter(QObject* Object, QEvent* Event) override;

public slots:
	void DownloadFinished(lcHttpReply* Reply);
	void on_SearchButton_clicked();
	void accept() override;
	void Finished(int Result);

protected:
	lcHttpReply* RequestURL(const QString& URL);

#ifndef Q_OS_WIN
	QNetworkAccessManager mNetworkManager;
#endif

	lcHttpReply* mKeyListReply;
	lcHttpReply* mSearchReply;
	lcHttpReply* mInventoryReply;
	QStringList mKeys;
	QByteArray mInventory;

	Ui::lcSetsDatabaseDialog* ui;
};

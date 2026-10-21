#pragma once

#include <QtTest/QtTest>

class lcTests : public QObject
{
	Q_OBJECT

private slots:
	void PropertyKeysAffectEquality();
	void HistoryStoresOnlyChangedObjects();
	void HistoryCancelsPendingEdit();
	void HistoryReplaysAndMergesEdits();
	void HistoryDropsRevertedMerge();
};

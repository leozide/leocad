#pragma once

#include <QtTest/QtTest>

class lcTests : public QObject
{
	Q_OBJECT

private slots:
	void PropertyKeysAffectEquality();
	void LastKeyFrameTracksTimeEdits();
	void PropertyEvaluationDoesNotChangeCurrentValue();
	void HistoryStoresOnlyChangedObjects();
	void HistoryCancelsPendingEdit();
	void HistoryReplaysAndMergesEdits();
	void HistoryDropsRevertedMerge();
	void ShadowParsesLines();
	void ShadowParsesIncludesAndClears();
	void ShadowReadsSubFileReferences();
	void ShadowScalesAndMirrorsSubFileShapes();
	void ConnectivityFitsShapesAlongTheirAxis();
	void ConnectivityConnectsSeatedShapes();
};

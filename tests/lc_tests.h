#pragma once

#include <QtTest/QtTest>

class lcTests : public QObject
{
	Q_OBJECT

private slots:
	void BoundingBoxVolumeIntersection();
	void MathMatrixRoundTrips();
	void MathTriangleIntersections();
	void MathTriangleClipping();
	void MathRayIntersections();
	void MathSphereRayDistances();
	void MathZoomTranslation();
	void PropertyKeysAffectEquality();
	void LastKeyFrameTracksTimeEdits();
	void PropertyEvaluationDoesNotChangeCurrentValue();
	void HistoryStoresOnlyChangedObjects();
	void HistoryCancelsPendingEdit();
	void HistoryReplaysAndMergesEdits();
	void HistoryDropsRevertedMerge();
};

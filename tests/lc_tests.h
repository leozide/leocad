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
	void MathSphereSurfaceIntersections();
	void MathClosestPointDegenerateSegment();
	void MathZoomTranslation();
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

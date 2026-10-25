#include "lc_global.h"
#include "lc_tests.h"
#include "lc_objectproperty.h"

void lcTests::LastKeyFrameTracksTimeEdits()
{
	lcObjectProperty<float> Position(0.0f);
	QCOMPARE(Position.GetLastKeyFrame(), lcStep(0));

	// Animation can extend past the step that introduces the piece.
	QVERIFY(Position.ChangeKey(0.0f, 1, true));
	QVERIFY(Position.ChangeKey(200.0f, 2, true));
	QCOMPARE(Position.GetLastKeyFrame(), lcStep(2));
	Position.Update(Position.GetLastKeyFrame());
	QCOMPARE(static_cast<const float&>(Position), 200.0f);

	Position.InsertTime(2, 3);
	QCOMPARE(Position.GetLastKeyFrame(), lcStep(5));
	Position.RemoveTime(2, 2);
	QCOMPARE(Position.GetLastKeyFrame(), lcStep(3));

	QVERIFY(Position.SetKeyFrame(3, false));
	QCOMPARE(Position.GetLastKeyFrame(), lcStep(1));
	Position.Update(Position.GetLastKeyFrame());
	QCOMPARE(static_cast<const float&>(Position), 0.0f);

	Position.RemoveAllKeys();
	QCOMPARE(Position.GetLastKeyFrame(), lcStep(0));
}

void lcTests::PropertyEvaluationDoesNotChangeCurrentValue()
{
	lcObjectProperty<float> Position(5.0f);
	QCOMPARE(Position.GetValue(LC_STEP_MAX), 5.0f);

	QVERIFY(Position.ChangeKey(10.0f, 2, true));
	QVERIFY(Position.ChangeKey(30.0f, 4, true));
	Position.Update(2);

	const lcObjectProperty<float> Before = Position;
	QCOMPARE(Position.GetValue(1), 10.0f);
	QCOMPARE(Position.GetValue(3), 10.0f);
	QCOMPARE(Position.GetValue(4), 30.0f);
	QCOMPARE(Position.GetValue(LC_STEP_MAX), 30.0f);
	QVERIFY(Position == Before);
	QCOMPARE(static_cast<const float&>(Position), 10.0f);

	Position.Update(4);
	QCOMPARE(static_cast<const float&>(Position), 30.0f);
	QCOMPARE(Position.GetValue(2), 10.0f);
	QCOMPARE(static_cast<const float&>(Position), 30.0f);
}

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

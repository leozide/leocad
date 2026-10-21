#include "lc_global.h"
#include "lc_tests.h"
#include "lc_objecthistory.h"
#include "lc_objectproperty.h"
#include <random>

class TestProperty : public lcObjectProperty<int>
{
public:
	using lcObjectProperty<int>::lcObjectProperty;

	void AddKey(lcStep Step, int Value)
	{
		mKeys.push_back({ Step, Value });
	}
};

struct TestState
{
	uint32_t Id;
	int Value;

	bool operator==(const TestState& Other) const
	{
		return Id == Other.Id && Value == Other.Value;
	}
};

struct TestObject
{
	TestState State = {};
	int Loads = 0;
	inline static int Destructions = 0;

	~TestObject() { Destructions++; }

	uint32_t GetId() const { return State.Id; }
	TestState GetHistoryState() const { return State; }
};

using History = lcObjectHistory<TestState>;
using Objects = std::vector<std::unique_ptr<TestObject>>;

static History Capture(const Objects& Model)
{
	History State;
	State.Save(Model);
	return State;
}

static void Load(const History& State, Objects& Model, int& Removed)
{
	const int DestructionsBefore = TestObject::Destructions;
	const std::unordered_map<uint32_t, TestObject*> ObjectsById = State.RestoreObjects(Model);
	Removed += TestObject::Destructions - DestructionsBefore;

	for (const TestState& Saved : State.States)
	{
		const auto Object = ObjectsById.find(Saved.Id);

		if (Object != ObjectsById.end())
		{
			Object->second->State = Saved;
			Object->second->Loads++;
		}
	}
}

static bool Matches(const Objects& Model, const std::vector<TestState>& States)
{
	if (Model.size() != States.size())
		return false;

	for (size_t Index = 0; Index < States.size(); Index++)
		if (!(Model[Index]->State == States[Index]))
			return false;

	return true;
}

static void Add(Objects& Model, uint32_t Id, int Value)
{
	Model.emplace_back(std::make_unique<TestObject>());
	Model.back()->State = { Id, Value };
}

void lcTests::PropertyKeysAffectEquality()
{
	TestProperty Property(10);
	TestProperty OtherProperty(10);
	QVERIFY(Property == OtherProperty);
	OtherProperty.AddKey(1, 10);
	QVERIFY(!(Property == OtherProperty));
	Property.AddKey(1, 10);
	QVERIFY(Property == OtherProperty);
	OtherProperty.AddKey(2, 20);
	QVERIFY(!(Property == OtherProperty));
}

void lcTests::HistoryStoresOnlyChangedObjects()
{
	Objects Model;

	for (uint32_t Id = 1; Id <= 15000; Id++)
		Add(Model, Id, 0);

	History Start = Capture(Model);
	Model[7500]->State.Value = 1;
	History End = Capture(Model);
	History::Compact(Start, End);
	QCOMPARE(Start.States.size(), size_t(1));
	QCOMPARE(End.States.size(), size_t(1));
	QCOMPARE(Start.States.capacity(), size_t(1));
	QCOMPARE(End.States.capacity(), size_t(1));
	QVERIFY(!Start.Order && !End.Order);
	TestObject* Untouched = Model[0].get();
	int Removed = 0;
	Load(Start, Model, Removed);
	QCOMPARE(Model[7500]->State.Value, 0);
	Load(End, Model, Removed);
	QCOMPARE(Model[7500]->State.Value, 1);
	QVERIFY(Model[0].get() == Untouched && Model[0]->Loads == 0 && Removed == 0);
}

void lcTests::HistoryCancelsPendingEdit()
{
	Objects Model;

	for (uint32_t Id = 1; Id <= 3; Id++)
		Add(Model, Id, 0);

	const History Start = Capture(Model);
	const std::vector<TestState> Expected = Start.States;
	Model.erase(Model.begin());
	Add(Model, 4, 3);

	int Removed = 0;
	Load(Start, Model, Removed);
	QVERIFY(Matches(Model, Expected));
	QCOMPARE(Removed, 1);
}

void lcTests::HistoryReplaysAndMergesEdits()
{
	Objects Model;
	History Start;
	History End;
	int Removed = 0;

	// Compare merged deltas with full snapshots over mixed edits. This covers
	// insert/delete, reordering, no-ops, and changing an object back to its start.
	std::mt19937 Random(1050);
	uint32_t NextId = 1;

	for (int Trial = 0; Trial < 200; Trial++)
	{
		Model.clear();

		for (int Index = 0; Index < 12; Index++)
			Add(Model, NextId++, 0);

		const std::vector<TestState> Initial = Capture(Model).States;
		History MergedStart;
		History MergedEnd;

		for (int Edit = 0; Edit < 40; Edit++)
		{
			Start = Capture(Model);

			switch (Random() % 5)
			{
			case 0:
				Add(Model, NextId++, static_cast<int>(Random() % 10));
				break;
			case 1:
				if (!Model.empty())
					Model.erase(Model.begin() + Random() % Model.size());
				break;
			case 2:
				if (!Model.empty())
					Model[Random() % Model.size()]->State.Value = static_cast<int>(Random() % 10);
				break;
			case 3:
				if (!Model.empty())
					std::swap(Model[Random() % Model.size()], Model[Random() % Model.size()]);
				break;
			case 4:
				break;
			}

			End = Capture(Model);
			const std::vector<TestState> Expected = End.States;
			const std::vector<TestState> Previous = Start.States;
			History::Compact(Start, End);
			Load(Start, Model, Removed);
			QVERIFY2(Matches(Model, Previous), qPrintable(QString("Undo failed: seed 1050, trial %1, edit %2").arg(Trial).arg(Edit)));
			Load(End, Model, Removed);
			QVERIFY2(Matches(Model, Expected), qPrintable(QString("Redo failed: seed 1050, trial %1, edit %2").arg(Trial).arg(Edit)));

			if (!Edit)
			{
				MergedStart = std::move(Start);
				MergedEnd = std::move(End);
			}
			else
				History::Merge(MergedStart, MergedEnd, Start, End);

			Load(MergedStart, Model, Removed);
			QVERIFY2(Matches(Model, Initial), qPrintable(QString("Merged undo failed: seed 1050, trial %1, edit %2").arg(Trial).arg(Edit)));
			Load(MergedEnd, Model, Removed);
			QVERIFY2(Matches(Model, Expected), qPrintable(QString("Merged redo failed: seed 1050, trial %1, edit %2").arg(Trial).arg(Edit)));
		}
	}
}

void lcTests::HistoryDropsRevertedMerge()
{
	Objects Model;
	History Start;
	History End;

	// A merged edit that returns to the initial state retains no object data.
	Add(Model, 1, 0);
	Start = Capture(Model);
	Model[0]->State.Value = 1;
	End = Capture(Model);
	History NextStart = End;
	Model[0]->State.Value = 0;
	History NextEnd = Capture(Model);
	History::Compact(Start, End);
	History::Compact(NextStart, NextEnd);
	History::Merge(Start, End, NextStart, NextEnd);
	QVERIFY(Start.IsEmpty() && End.IsEmpty());
}

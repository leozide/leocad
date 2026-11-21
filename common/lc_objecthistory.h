#pragma once

// Object states are captured while an edit is in progress, then reduced to the
// changed objects. Ordering is only retained for insertions, deletions or reordering.
template<typename StateType>
class lcObjectHistory
{
public:
	std::vector<StateType>& GetStates()
	{
		return mStates;
	}

	const std::vector<StateType>& GetStates() const
	{
		return mStates;
	}

	const std::optional<std::vector<uint32_t>>& GetOrder() const
	{
		return mOrder;
	}

	bool IsEmpty() const
	{
		return mStates.empty() && !mOrder;
	}

	template<typename ObjectType>
	void Save(const std::vector<std::unique_ptr<ObjectType>>& Objects)
	{
		mStates.clear();
		mStates.reserve(Objects.size());
		mOrder.emplace();
		mOrder->reserve(Objects.size());

		for (const std::unique_ptr<ObjectType>& Object : Objects)
		{
			mStates.emplace_back(Object->GetHistoryState());
			mOrder->emplace_back(static_cast<uint32_t>(Object->GetId()));
		}
	}

	template<typename ObjectType>
	std::unordered_map<uint32_t, ObjectType*> RestoreObjects(std::vector<std::unique_ptr<ObjectType>>& Objects) const
	{
		std::unordered_map<uint32_t, ObjectType*> ObjectsById;

		if (!mOrder)
		{
			ObjectsById.reserve(Objects.size());

			for (const std::unique_ptr<ObjectType>& Object : Objects)
				ObjectsById.emplace(static_cast<uint32_t>(Object->GetId()), Object.get());

			return ObjectsById;
		}

		std::unordered_map<uint32_t, size_t> ObjectIndices;
		ObjectIndices.reserve(Objects.size());

		for (size_t Index = 0; Index < Objects.size(); Index++)
			ObjectIndices.emplace(static_cast<uint32_t>(Objects[Index]->GetId()), Index);

		std::unordered_set<uint32_t> StateIds;
		std::vector<std::unique_ptr<ObjectType>> NewObjects;

		for (const StateType& State : mStates)
			StateIds.emplace(static_cast<uint32_t>(State.Id));

		NewObjects.reserve(mOrder->size());
		ObjectsById.reserve(mOrder->size());

		for (uint32_t Id : *mOrder)
		{
			const auto Object = ObjectIndices.find(Id);

			if (Object != ObjectIndices.end())
				NewObjects.emplace_back(std::move(Objects[Object->second]));
			else if (StateIds.find(Id) != StateIds.end())
				NewObjects.emplace_back(std::make_unique<ObjectType>());
			else
				continue; // A non-undoable model-list change may have removed it.

			// New objects do not have their saved IDs until their state is restored.
			ObjectsById.emplace(Id, NewObjects.back().get());
		}

		Objects = std::move(NewObjects);

		return ObjectsById;
	}

	static void Compact(lcObjectHistory& Start, lcObjectHistory& End)
	{
		std::unordered_map<uint32_t, const StateType*> StartStates;
		std::unordered_map<uint32_t, const StateType*> EndStates;

		StartStates.reserve(Start.mStates.size());
		EndStates.reserve(End.mStates.size());

		for (const StateType& State : Start.mStates)
			StartStates.emplace(static_cast<uint32_t>(State.Id), &State);

		for (const StateType& State : End.mStates)
			EndStates.emplace(static_cast<uint32_t>(State.Id), &State);

		std::unordered_set<uint32_t> Changed;

		for (const StateType& State : Start.mStates)
		{
			const auto Other = EndStates.find(static_cast<uint32_t>(State.Id));

			if (Other == EndStates.end() || !(State == *Other->second))
				Changed.emplace(static_cast<uint32_t>(State.Id));
		}

		for (const StateType& State : End.mStates)
			if (StartStates.find(static_cast<uint32_t>(State.Id)) == StartStates.end())
				Changed.emplace(static_cast<uint32_t>(State.Id));

		auto KeepChanges = [&Changed](std::vector<StateType>& States)
		{
			std::vector<StateType> Changes;
			Changes.reserve(std::min(States.size(), Changed.size()));

			for (StateType& State : States)
				if (Changed.find(static_cast<uint32_t>(State.Id)) != Changed.end())
					Changes.emplace_back(std::move(State));

			States = std::move(Changes);
		};

		KeepChanges(Start.mStates);
		KeepChanges(End.mStates);

		if (Start.mOrder == End.mOrder)
		{
			Start.mOrder.reset();
			End.mOrder.reset();
		}
	}

	static void Merge(lcObjectHistory& Start, lcObjectHistory& End, lcObjectHistory& NextStart, lcObjectHistory& NextEnd)
	{
		std::map<uint32_t, StateType> Before;
		std::map<uint32_t, StateType> After;

		for (StateType& State : Start.mStates)
			Before.emplace(static_cast<uint32_t>(State.Id), std::move(State));

		for (StateType& State : End.mStates)
			After.emplace(static_cast<uint32_t>(State.Id), std::move(State));

		for (StateType& State : NextStart.mStates)
		{
			const uint32_t Id = static_cast<uint32_t>(State.Id);

			if (After.find(Id) == After.end())
				Before.emplace(Id, std::move(State));
		}

		for (const StateType& State : NextStart.mStates)
			After.erase(static_cast<uint32_t>(State.Id));

		for (StateType& State : NextEnd.mStates)
			After.insert_or_assign(static_cast<uint32_t>(State.Id), std::move(State));

		Start.mStates.clear();
		End.mStates.clear();

		for (auto& [Id, State] : Before)
			Start.mStates.emplace_back(std::move(State));

		for (auto& [Id, State] : After)
			End.mStates.emplace_back(std::move(State));

		if (!Start.mOrder)
			Start.mOrder = std::move(NextStart.mOrder);

		if (NextEnd.mOrder)
			End.mOrder = std::move(NextEnd.mOrder);

		Compact(Start, End);
	}

protected:
	std::vector<StateType> mStates;
	std::optional<std::vector<uint32_t>> mOrder;
};

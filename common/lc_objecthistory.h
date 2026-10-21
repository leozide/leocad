#pragma once

// Object states are captured while an edit is in progress, then reduced to the
// changed objects. Ordering is only retained for insertions, deletions or reordering.
template<typename StateType>
struct lcObjectHistory
{
	std::vector<StateType> States;
	std::optional<std::vector<uint32_t>> Order;

	template<typename ObjectType>
	void Save(const std::vector<std::unique_ptr<ObjectType>>& Objects)
	{
		States.clear();
		States.reserve(Objects.size());
		Order.emplace();
		Order->reserve(Objects.size());

		for (const std::unique_ptr<ObjectType>& Object : Objects)
		{
			States.emplace_back(Object->GetHistoryState());
			Order->emplace_back(static_cast<uint32_t>(Object->GetId()));
		}
	}

	bool IsEmpty() const
	{
		return States.empty() && !Order;
	}

	static void Compact(lcObjectHistory& Start, lcObjectHistory& End)
	{
		std::unordered_map<uint32_t, const StateType*> StartStates;
		std::unordered_map<uint32_t, const StateType*> EndStates;

		StartStates.reserve(Start.States.size());
		EndStates.reserve(End.States.size());

		for (const StateType& State : Start.States)
			StartStates.emplace(static_cast<uint32_t>(State.Id), &State);

		for (const StateType& State : End.States)
			EndStates.emplace(static_cast<uint32_t>(State.Id), &State);

		std::unordered_set<uint32_t> Changed;

		for (const StateType& State : Start.States)
		{
			const auto Other = EndStates.find(static_cast<uint32_t>(State.Id));

			if (Other == EndStates.end() || !(State == *Other->second))
				Changed.emplace(static_cast<uint32_t>(State.Id));
		}

		for (const StateType& State : End.States)
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

		KeepChanges(Start.States);
		KeepChanges(End.States);

		if (Start.Order == End.Order)
		{
			Start.Order.reset();
			End.Order.reset();
		}
	}

	static void Merge(lcObjectHistory& Start, lcObjectHistory& End, lcObjectHistory& NextStart, lcObjectHistory& NextEnd)
	{
		std::map<uint32_t, StateType> Before;
		std::map<uint32_t, StateType> After;

		for (StateType& State : Start.States)
			Before.emplace(static_cast<uint32_t>(State.Id), std::move(State));

		for (StateType& State : End.States)
			After.emplace(static_cast<uint32_t>(State.Id), std::move(State));

		for (StateType& State : NextStart.States)
		{
			const uint32_t Id = static_cast<uint32_t>(State.Id);

			if (After.find(Id) == After.end())
				Before.emplace(Id, std::move(State));
		}

		for (const StateType& State : NextStart.States)
			After.erase(static_cast<uint32_t>(State.Id));

		for (StateType& State : NextEnd.States)
			After.insert_or_assign(static_cast<uint32_t>(State.Id), std::move(State));

		Start.States.clear();
		End.States.clear();

		for (auto& [Id, State] : Before)
			Start.States.emplace_back(std::move(State));

		for (auto& [Id, State] : After)
			End.States.emplace_back(std::move(State));

		if (!Start.Order)
			Start.Order = std::move(NextStart.Order);

		if (NextEnd.Order)
			End.Order = std::move(NextEnd.Order);

		Compact(Start, End);
	}

	template<typename ObjectType>
	std::unordered_map<uint32_t, ObjectType*> RestoreObjects(std::vector<std::unique_ptr<ObjectType>>& Objects) const
	{
		std::unordered_map<uint32_t, ObjectType*> ObjectsById;

		if (!Order)
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

		for (const StateType& State : States)
			StateIds.emplace(static_cast<uint32_t>(State.Id));

		NewObjects.reserve(Order->size());
		ObjectsById.reserve(Order->size());

		for (uint32_t Id : *Order)
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
};

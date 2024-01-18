#pragma once

template <class T>
class lcArray
{
public:
	lcArray(int Size = 0)
	{
		if (Size > 0)
			elements.reserve(Size);
	}

	const T& operator[](int Index) const
	{
		return elements[Index];
	}

	T& operator[](int Index)
	{
		return elements[Index];
	}

	bool operator==(const lcArray<T>& Array) const
	{
		return elements == Array;
	}

	typename std::vector<T>::iterator begin()
	{
		return elements.begin();
	}

	typename std::vector<T>::iterator end()
	{
		return elements.end();
	}

	typename std::vector<T>::const_iterator begin() const
	{
		return elements.begin();
	}

	typename std::vector<T>::const_iterator end() const
	{
		return elements.end();
	}

	bool IsEmpty() const
	{
		return elements.empty();
	}

	int GetSize() const
	{
		return elements.size();
	}

	void SetSize(size_t NewSize)
	{
		elements.resize(NewSize);
	}

	void AllocGrow(size_t Grow)
	{
		elements.reserve(elements.size() + Grow);
	}

	void Add(const T& NewItem)
	{
		elements.push_back(NewItem);
	}

	T& Add()
	{
		elements.push_back(T());
		return elements.back();
	}

	T& InsertAt(int Index)
	{
		return elements.insert(Index, T());
	}

	void InsertAt(int Index, const T& NewItem)
	{
		elements.insert(elements.begin() + Index, NewItem);
	}

	void RemoveIndex(int Index)
	{
		elements.erase(elements.begin() + Index);
	}

	void Remove(const T& Item)
	{
		for (size_t i = 0; i < elements.size(); i++)
		{
			if (elements[i] == Item)
			{
				RemoveIndex(i);
				return;
			}
		}
	}

	void RemoveAll()
	{
		elements.clear();
	}

	void DeleteAll()
	{
		for (size_t i = 0; i < elements.size(); i++)
			delete elements[i];

		elements.clear();
	}

	int FindIndex(const T& Item) const
	{
		for (size_t i = 0; i < elements.size(); i++)
			if (elements[i] == Item)
				return i;

		return -1;
	}

protected:
	std::vector<T> elements;
};


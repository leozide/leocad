#pragma once

template <class T>
class lcArray
{
public:
	lcArray(int Size = 0, int Grow = 16)
	{
		mData = nullptr;
		mLength = 0;
		mAlloc = 0;
		mGrow = Grow;

		if (Size != 0)
			AllocGrow(Size);
	}

	lcArray(const lcArray<T>& Array)
	{
		mData = nullptr;
		*this = Array;
	}

	~lcArray()
	{
		delete[] mData;
	}

	lcArray<T>& operator=(const lcArray<T>& Array)
	{
		mLength = Array.mLength;
		mAlloc = Array.mAlloc;
		mGrow = Array.mGrow;

		delete[] mData;
		mData = new T[mAlloc];

		for (int i = 0; i < mLength; i++)
			mData[i] = Array.mData[i];

		return *this;
	}

	lcArray(lcArray<T>&& Array)
	{
		mData = nullptr;
		*this = std::move(Array);
	}

	lcArray<T>& operator=(lcArray<T>&& Array)
	{
		delete[] mData;

		mData = Array.mData;
		Array.mData = nullptr;
		mLength = Array.mLength;
		Array.mLength = 0;
		mAlloc = Array.mAlloc;
		Array.mAlloc = 0;
		mGrow = Array.mGrow;
		Array.mGrow = 16;

		return *this;
	}

	const T& operator[](int Index) const
	{
		return mData[Index];
	}

	T& operator[](int Index)
	{
		return mData[Index];
	}

	bool operator==(const lcArray<T>& Array) const
	{
		if (mLength != Array.mLength)
			return false;

		for (int i = 0; i < mLength; i++)
			if (mData[i] != Array.mData[i])
				return false;

		return true;
	}

	T* begin()
	{
		return &mData[0];
	}

	T* end()
	{
		return &mData[0] + mLength;
	}

	const T* begin() const
	{
		return &mData[0];
	}

	const T* end() const
	{
		return &mData[0] + mLength;
	}

	bool IsEmpty() const
	{
		return mLength == 0;
	}

	int GetSize() const
	{
		return mLength;
	}

	void SetSize(size_t NewSize)
	{
		if (NewSize > mAlloc)
			AllocGrow(NewSize - mLength);

		mLength = (int)NewSize;
	}

	void SetGrow(int Grow)
	{
		if (Grow)
			mGrow = Grow;
	}

	void AllocGrow(size_t Grow)
	{
		if ((mLength + Grow) > mAlloc)
		{
			const size_t NewSize = ((mLength + Grow + mGrow - 1) / mGrow) * mGrow;
			T* NewData = new T[NewSize];

			for (int i = 0; i < mLength; i++)
				NewData[i] = mData[i];

			delete[] mData;
			mData = NewData;
			mAlloc = NewSize;
		}
	}

	void Add(const T& NewItem)
	{
		AllocGrow(1);
		mData[mLength++] = NewItem;
	}

	T& Add()
	{
		AllocGrow(1);
		return mData[mLength++];
	}

	T& InsertAt(int Index)
	{
		if (Index >= mLength)
			AllocGrow(Index - mLength + 1);
		else
			AllocGrow(1);

		mLength++;
		for (int i = mLength - 1; i > Index; i--)
			mData[i] = mData[i - 1];

		return mData[Index];
	}

	void InsertAt(int Index, const T& NewItem)
	{
		if (Index >= mLength)
			AllocGrow(Index - mLength + 1);
		else
			AllocGrow(1);

		mLength++;
		for (int i = mLength - 1; i > Index; i--)
			mData[i] = mData[i - 1];

		mData[Index] = NewItem;
	}

	void RemoveIndex(int Index)
	{
		mLength--;

		for (int i = Index; i < mLength; i++)
			mData[i] = mData[i + 1];
	}

	void Remove(const T& Item)
	{
		for (int i = 0; i < mLength; i++)
		{
			if (mData[i] == Item)
			{
				RemoveIndex(i);
				return;
			}
		}
	}

	void RemoveAll()
	{
		mLength = 0;
	}

	void DeleteAll()
	{
		for (int i = 0; i < mLength; i++)
			delete mData[i];

		mLength = 0;
	}

	int FindIndex(const T& Item) const
	{
		for (int i = 0; i < mLength; i++)
			if (mData[i] == Item)
				return i;

		return -1;
	}

protected:
	T* mData;
	int mLength;
	size_t mAlloc;
	int mGrow;
};


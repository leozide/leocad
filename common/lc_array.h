#ifndef _LC_ARRAY_H_
#define _LC_ARRAY_H_

template <class T>
class lcArray
{
public:
	typedef int (*lcArrayCompareFunc)(const T& a, const T& b);

	lcArray(int Size = 0, int Grow = 16)
	{
		mData = NULL;
		mLength = 0;
		mAlloc = 0;
		mGrow = Grow;

		if (Size != 0)
			AllocGrow(Size);
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

	lcArray<T>& operator+=(const lcArray<T>& Array)
	{
		AllocGrow(Array.mLength);

		for (int i = 0; i < Array.mLength; i++)
			mData[mLength + i] = Array.mData[i];

		mLength += Array.mLength;
		return *this;
	}

	T& operator[](int Index) const
	{
		return mData[Index];
	}

	int GetSize() const
	{
		return mLength;
	}

	void SetSize(int NewSize)
	{
		if (NewSize > mAlloc)
			AllocGrow(NewSize - mAlloc);

		mLength = NewSize;
	}

	void AllocGrow(int Grow)
	{
		if ((mLength + Grow) > mAlloc)
		{
			int NewSize = ((mLength + Grow + mGrow - 1) / mGrow) * mGrow;
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

	void AddSorted(const T& Obj, lcArrayCompareFunc CompareFunc)
	{
		for (int i = 0; i < mLength; i++)
		{
			if (CompareFunc(Obj, mData[i]) < 0)
			{
				InsertAt(i, Obj);
				return;
			}
		}

		Add(Obj);
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

	void Sort(lcArrayCompareFunc CompareFunc)
	{
		if (mLength <= 1)
			return;

		int i = 1;
		bool Flipped;

		do
		{
			Flipped = false;

			for (int j = mLength - 1; j >= i; --j)
			{
				T& a = mData[j];
				T& b = mData[j - 1];

				if (CompareFunc(b, a) > 0)
				{
					T Tmp = b;
					mData[j - 1] = a;
					mData[j] = Tmp;
					Flipped = true;
				}
			}
		} while ((++i < mLength) && Flipped);
	}

protected:
	T* mData;
	int mLength;
	int mAlloc;
	int mGrow;
};

#endif // _LC_ARRAY_H_

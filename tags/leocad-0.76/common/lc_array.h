#ifndef _LC_ARRAY_H_
#define _LC_ARRAY_H_

#include <stdlib.h>
#include <string.h>

template <class T>
class lcPtrArray
{
public:
	typedef int (*LC_PTRARRAY_COMPARE_FUNC)(const T* a, const T* b, void* Data);

	lcPtrArray(int Size = 0)
	{
		m_Data = NULL;
		m_Length = 0;
		m_Alloc = 0;

		if (Size)
			Expand(Size);
	}

	~lcPtrArray()
	{
		free(m_Data);
	}

	int GetSize() const
	{
		return m_Length;
	}

	T* RemoveIndex(int Index)
	{
		T* ret = NULL;

		if (Index < m_Length)
		{
			ret = m_Data[Index];

			if (Index != m_Length - 1)
				memmove(m_Data + Index, m_Data + Index + 1, sizeof(T*) * (m_Length - Index - 1));

			m_Length--;
			m_Data[m_Length] = NULL;
		}

		return ret;
	}

	T* RemovePointer(T* Ptr)
	{
		for (int i = 0; i < m_Length; i++)
			if (m_Data[i] == Ptr)
				return RemoveIndex(i);

		return NULL;
	}

	void RemoveAll()
	{
		m_Length = 0;
	}

	void Add(T* Ptr)
	{
		Expand(1);
		m_Data[m_Length] = Ptr;
		m_Length++;
	}

	void AddSorted(T* Ptr, LC_PTRARRAY_COMPARE_FUNC SortFunc, void* SortData)
	{
		for (int i = 0; i < GetSize(); i++)
		{
			if (Func(Ptr, m_Data[i], SortData) < 0)
			{
				InsertAt(i, Ptr);
				return;
			}
		}

		Add(Ptr);
	}

	void SetAt(int Index, T* Ptr)
	{
		m_Data[Index] = Ptr;
	}

	void InsertAt(int Index, T* Ptr)
	{
		if (Index >= m_Length)
			Expand(Index - m_Length + 1);
		else
			Expand(1);

		m_Length++;
		for (int i = m_Length - 1; i > Index; i--)
			m_Data[i] = m_Data[i-1];

		m_Data[Index] = Ptr;
	}

	int FindIndex(T* Ptr) const
	{
		for (int i = 0; i < m_Length; i++)
			if (m_Data[i] == Ptr)
				return i;

		return -1;
	}

	void Sort(LC_PTRARRAY_COMPARE_FUNC SortFunc, void* SortData)
	{
		int Count = GetSize();

		if (Count <= 1)
			return;

		int i = 1;
		bool Flipped;

		do
		{
			Flipped = false;

			for (int j = Count - 1; j >= i; --j)
			{
				T* a = m_Data[j];
				T* b = m_Data[j-1];

				if (SortFunc(b, a, SortData) > 0)
				{
					m_Data[j - 1] = a;
					m_Data[j] = b;
					Flipped = true;
				}
			}
		} while ((++i < Count) && Flipped);
	}


	lcPtrArray<T>& operator=(const lcPtrArray<T>& Array)
	{
		m_Length = Array.m_Length;
		m_Alloc = Array.m_Alloc;
		m_Data = (T**)realloc(m_Data, (m_Alloc) * sizeof(T*));
		memcpy(m_Data, Array.m_Data, (m_Alloc) * sizeof(T*));
		return *this;
	}

	lcPtrArray<T>& operator+=(const lcPtrArray<T>& Array)
	{
		Expand(Array.m_Length);
		memcpy(m_Data + m_Length, Array.m_Data, Array.m_Length * sizeof(T*));
		m_Length += Array.m_Length;
		return *this;
	}

	T* operator [](int Index) const
	{
		return m_Data[Index];
	}

protected:
	void Expand(int Grow)
	{
		if((m_Length + Grow) > m_Alloc)
		{
			m_Data =(T**)realloc(m_Data, (m_Length + Grow) * sizeof(T*));
			memset(m_Data + m_Length, 0, Grow * sizeof(T*));
			m_Alloc = m_Length + Grow;
		}
	}

	T** m_Data;
	int m_Length;
	int m_Alloc;
};

template <class T>
class lcObjArray
{
public:
	typedef int (*LC_OBJARRAY_COMPARE_FUNC)(const T& A, const T& B, void* Data);

	lcObjArray(int Size = 0, int Grow = 16)
	{
		m_Data = NULL;
		m_Length = 0;
		m_Alloc = 0;
		m_Grow = Grow;

		if (Size != 0)
			Expand(Size);
	}
	~lcObjArray()
	{
		delete[] m_Data;
	}

	int GetSize() const
	{
		return m_Length;
	}

	void RemoveIndex(int Index)
	{
		m_Length--;

		for (int i = Index; i < m_Length; i++)
			m_Data[i] = m_Data[i+1];
	}

	void RemoveAll()
	{
		m_Length = 0;
	}

	void Add(const T& Obj)
	{
		Expand(1);
		m_Data[m_Length++] = Obj;
	}

	void AddSorted(const T& Obj, LC_OBJARRAY_COMPARE_FUNC SortFunc, void* SortData)
	{
		for (int i = 0; i < GetSize(); i++)
		{
			if (SortFunc(Obj, m_Data[i], SortData) < 0)
			{
				InsertAt(i, Obj);
				return;
			}
		}

		Add(Obj);
	}

	void InsertAt(int Index, const T& Obj)
	{
		InsertAt(Index);

		m_Data[Index] = Obj;
	}

	void InsertAt(int Index)
	{
		if (Index >= m_Length)
			Expand(Index - m_Length + 1);
		else
			Expand(1);

		m_Length++;
		for (int i = m_Length - 1; i > Index; i--)
			m_Data[i] = m_Data[i-1];
	}

	lcObjArray<T>& operator=(const lcObjArray<T>& Array)
	{
		m_Length = Array.m_Length;
		m_Alloc = Array.m_Alloc;
		m_Grow = Array.m_Grow;

		delete[] m_Data;
		m_Data = new T[m_Alloc];

		for (int i = 0; i < m_Length; i++)
			m_Data[i] = Array.m_Data[i];

		return *this;
	}

	T& operator [](int Index) const
	{
		return m_Data[Index];
	}

protected:
	void Expand(int Grow)
	{
		if ((m_Length + Grow) > m_Alloc)
		{
			int NewSize = ((m_Length + Grow) / m_Grow + 1) * m_Grow;

			T* NewData = new T[NewSize];

			for (int i = 0; i < m_Length; i++)
				NewData[i] = m_Data[i];

			delete[] m_Data;
			m_Data = NewData;
			m_Alloc = NewSize;
		}
	}

	T* m_Data;
	int m_Length;
	int m_Alloc;
	int m_Grow;
};

#endif // _LC_ARRAY_H_

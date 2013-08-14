#ifndef _ARRAY_H_
#define _ARRAY_H_

template <class T>
class PtrArray
{
public:
	PtrArray(int nSize = 0);
	~PtrArray();

	typedef int (*LC_PTRARRAY_COMPARE_FUNC)(const T* a, const T* b, void* data);

	int GetSize() const
		{ return m_nLength; }
	void SetSize(int Size);
	void Expand(int nGrow);

	T* RemoveIndex(int nIndex);
	T* RemovePointer(T* pObj);
	void RemoveAll();
	void Add(T* pObj);
	void AddSorted(T* pObj, LC_PTRARRAY_COMPARE_FUNC pFunc, void* pData);
	void SetAt(int Index, T* Ptr);
	void InsertAt(int nIndex, T* pObj);
	int FindIndex(T* Obj) const;
	void Sort(LC_PTRARRAY_COMPARE_FUNC SortFunc, void* SortData);

	PtrArray<T>& operator=(const PtrArray<T>& Array);
	PtrArray<T>& operator+=(const PtrArray<T>& Array);
	T* operator [](int nIndex) const
		{ return m_pData[nIndex]; }

protected:
	T** m_pData;
	int m_nLength;
	int m_nAlloc;
};

template <class T>
class ObjArray
{
public:
	ObjArray(int Size = 0, int Grow = 16);
	~ObjArray();

	typedef int (*LC_OBJARRAY_COMPARE_FUNC)(const T& A, const T& B, void* SortData);

	int GetSize() const
		{ return m_Length; }

	void RemoveIndex(int Index);
	void RemoveAll();
	void SetSize(int NewSize);
	void Expand(int Grow);
	void Add(const T& Obj);
	T& Add();
	void AddSorted(const T& Obj, LC_OBJARRAY_COMPARE_FUNC Func, void* SortData);
	void InsertAt(int Index, const T& Obj);
	void Sort(LC_OBJARRAY_COMPARE_FUNC SortFunc, void* SortData);

	ObjArray<T>& operator=(const ObjArray<T>& Array)
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
		{ return m_Data[Index]; }

protected:
	T* m_Data;
	int m_Length;
	int m_Alloc;
	int m_Grow;
};

#include "array.cpp"

#endif // _ARRAY_H_

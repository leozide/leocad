//
// Simple array classes
//

#include <stdlib.h>
#include <string.h>

template <class T>
PtrArray<T>::PtrArray(int nSize)
{
	m_pData = NULL;
	m_nLength = 0;
	m_nAlloc = 0;

	if(nSize != 0)
		Expand(nSize);
}

template <class T>
PtrArray<T>::~PtrArray()
{
	free(m_pData);
}

template <class T>
void PtrArray<T>::SetSize(int Size)
{
	if (Size > m_nLength)
		Expand(Size - m_nLength);

	m_nLength = Size;
}

template <class T>
void PtrArray<T>::Expand(int nGrow)
{
	if ((m_nLength + nGrow) > m_nAlloc)
	{
		m_pData =(T**)realloc(m_pData,(m_nLength + nGrow) * sizeof(T*));
		memset(m_pData + m_nLength, 0, nGrow * sizeof(T*));
		m_nAlloc = m_nLength + nGrow;
	}
}

template <class T>
void PtrArray<T>::RemoveAll()
{
	m_nLength = 0;
}

template <class T>
T* PtrArray<T>::RemoveIndex(int nIndex)
{
	T* ret = NULL;

	if(nIndex < m_nLength)
	{
		ret = m_pData[nIndex];

		if(nIndex != m_nLength - 1)
			memmove(m_pData + nIndex, m_pData + nIndex + 1, sizeof(T*) *(m_nLength - nIndex - 1));

		m_nLength--;
		m_pData[m_nLength] = NULL;
	}

	return ret;
}

template <class T>
T* PtrArray<T>::RemovePointer(T* pObj)
{
	int i;

	for(i = 0; i < m_nLength; i++)
		if(m_pData[i] == pObj)
			return RemoveIndex(i);

	return NULL;
}

template <class T>
void PtrArray<T>::Add(T* pObj)
{
	Expand(1);
	m_pData[m_nLength] = pObj;
	m_nLength++;
}

template <class T>
void PtrArray<T>::AddSorted(T* pObj, LC_PTRARRAY_COMPARE_FUNC pFunc, void* pData)
{
	int i;

	for(i = 0; i < GetSize(); i++)
	{
		if(pFunc(pObj, m_pData[i], pData) < 0)
		{
			InsertAt(i, pObj);
			return;
		}
	}

		Add(pObj);
}

template <class T>
void PtrArray<T>::SetAt(int Index, T* Ptr)
{
	m_pData[Index] = Ptr;
}

template <class T>
void PtrArray<T>::InsertAt(int nIndex, T* pObj)
{
	if(nIndex >= m_nLength)
		Expand(nIndex - m_nLength + 1);
	else
		Expand(1);

	m_nLength++;
	for(int i = m_nLength - 1; i > nIndex; i--)
		m_pData[i] = m_pData[i-1];

	m_pData[nIndex] = pObj;
}

template <class T>
int PtrArray<T>::FindIndex(T* Obj) const
{
	for (int i = 0; i < m_nLength; i++)
		if (m_pData[i] == Obj)
			return i;

	return -1;
}

template <class T>
void PtrArray<T>::Sort(LC_PTRARRAY_COMPARE_FUNC SortFunc, void* SortData)
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
			T* a = m_pData[j];
			T* b = m_pData[j-1];

			if (SortFunc(b, a, SortData) > 0)
			{
				m_pData[j - 1] = a;
				m_pData[j] = b;
				Flipped = true;
			}
		}
	} while ((++i < Count) && Flipped);
}

template <class T>
PtrArray<T>& PtrArray<T>::operator=(const PtrArray<T>& Array)
{
	m_nLength = Array.m_nLength;
	m_nAlloc = Array.m_nAlloc;
	m_pData =(T**)realloc(m_pData, (m_nAlloc) * sizeof(T*));
	memcpy(m_pData, Array.m_pData, (m_nAlloc) * sizeof(T*));
}

template <class T>
PtrArray<T>& PtrArray<T>::operator+=(const PtrArray<T>& Array)
{
	Expand(Array.m_nLength);
	memcpy(m_pData + m_nLength, Array.m_pData, Array.m_nLength * sizeof(T*));
	m_nLength += Array.m_nLength;
	return *this;
}

// ============================================================================

template <class T>
ObjArray<T>::ObjArray(int Size, int Grow)
{
	m_Data = NULL;
	m_Length = 0;
	m_Alloc = 0;
	m_Grow = Grow;

	if (Size != 0)
		Expand(Size);
}

template <class T>
ObjArray<T>::~ObjArray ()
{
	delete[] m_Data;
}

template <class T>
void ObjArray<T>::Expand(int Grow)
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

template <class T>
void ObjArray<T>::RemoveAll()
{
	m_Length = 0;
}

template <class T>
void ObjArray<T>::RemoveIndex(int Index)
{
	m_Length--;

	for (int i = Index; i < m_Length; i++)
		m_Data[i] = m_Data[i+1];
}

template <class T>
void ObjArray<T>::Add(const T& Obj)
{
	Expand(1);
	m_Data[m_Length++] = Obj;
}

template <class T>
void ObjArray<T>::AddSorted (const T& Obj, LC_OBJARRAY_COMPARE_FUNC Func, void* SortData)
{
	int i;

	for (i = 0; i < GetSize(); i++)
	{
		if (Func(Obj, m_Data[i], SortData) < 0)
		{
			InsertAt (i, Obj);
			return;
		}
	}

	Add(Obj);
}

template <class T>
void ObjArray<T>::InsertAt(int Index, const T& Obj)
{
	if (Index >= m_Length)
		Expand(Index - m_Length + 1);
	else
		Expand(1);

	m_Length++;
	for (int i = m_Length - 1; i > Index; i--)
		m_Data[i] = m_Data[i-1];

	m_Data[Index] = Obj;
}







/*
// ============================================================================
// ObjectArray class

ObjectArray::ObjectArray (unsigned long nSize)
{
  m_pData = NULL;
  m_nLength = 0;
  m_nAlloc = 0;

  if (nSize != 0)
    Expand (nSize);
}

ObjectArray::~ObjectArray ()
{
  free (m_pData);
}

void ObjectArray::Expand (unsigned long nGrow)
{
  if ((m_nLength + nGrow) > m_nAlloc)
  {
    m_pData = (Object**)realloc (m_pData, (m_nLength + nGrow) * sizeof (Object*));
    memset (m_pData + m_nLength, 0, nGrow * sizeof (Object*));
    m_nAlloc = m_nLength + nGrow;
  }
}

void ObjectArray::SetSize (unsigned long nSize)
{
  if (nSize > m_nLength)
    Expand (nSize - m_nLength);
 
  m_nLength = nSize;
}

Object* ObjectArray::RemoveIndex (unsigned long nIndex)
{
  Object* ret = NULL;

  if (nIndex < m_nLength)
  {
    ret = m_pData[nIndex];

    if (nIndex != m_nLength - 1)
      memmove (m_pData + nIndex, m_pData + nIndex + 1,
               sizeof (Object*) * (m_nLength - nIndex - 1));
 
    m_nLength--;
    m_pData[m_nLength] = NULL;
  }

  return ret;
}

Object* ObjectArray::RemovePointer (Object* pObj)
{
  unsigned long i;

  for (i = 0; i < m_nLength; i++)
    if (m_pData[i] == pObj)
      return RemoveIndex (i);

  return NULL;
}

void ObjectArray::Add (Object* pObj)
{
  Expand (1);
  m_pData[m_nLength++] = pObj;
}
*/

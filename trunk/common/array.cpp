//
// Simple array class
//

#include <stdlib.h>
#include <string.h>

template <class T>
PtrArray<T>::PtrArray (int nSize)
{
  m_pData = NULL;
  m_nLength = 0;
  m_nAlloc = 0;

  if (nSize != 0)
    Expand (nSize);
}

template <class T>
PtrArray<T>::~PtrArray ()
{
  free (m_pData);
}

template <class T>
void PtrArray<T>::Expand (int nGrow)
{
  if ((m_nLength + nGrow) > m_nAlloc)
  {
    m_pData = (T**)realloc (m_pData, (m_nLength + nGrow) * sizeof (T*));
    memset (m_pData + m_nLength, 0, nGrow * sizeof (T*));
    m_nAlloc = m_nLength + nGrow;
  }
}

template <class T>
void PtrArray<T>::SetSize (int nSize)
{
  if (nSize > m_nLength)
    Expand (nSize - m_nLength);
 
  m_nLength = nSize;
}

template <class T>
T* PtrArray<T>::RemoveIndex (int nIndex)
{
  T* ret = NULL;

  if (nIndex < m_nLength)
  {
    ret = m_pData[nIndex];

    if (nIndex != m_nLength - 1)
      memmove (m_pData + nIndex, m_pData + nIndex + 1,
               sizeof (T*) * (m_nLength - nIndex - 1));
 
    m_nLength--;
    m_pData[m_nLength] = NULL;
  }

  return ret;
}

template <class T>
T* PtrArray<T>::RemovePointer (T* pObj)
{
  int i;

  for (i = 0; i < m_nLength; i++)
    if (m_pData[i] == pObj)
      return RemoveIndex (i);

  return NULL;
}

template <class T>
void PtrArray<T>::Add (T* pObj)
{
  Expand (1);
  m_pData[m_nLength++] = pObj;
}




/*
// =============================================================================
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

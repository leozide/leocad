#ifndef _ARRAY_H_
#define _ARRAY_H_

template <class T>
class PtrArray
{
 public:
  PtrArray (unsigned long nSize = 0);
  ~PtrArray ();

  void SetSize (unsigned long nSize);
  unsigned long GetSize () const
    { return m_nLength; }

  T* RemoveIndex (unsigned long nIndex);
  T* RemovePointer (T* pObj);
  void Add (T* pObj);

  T* operator [](unsigned long nIndex) const
    { return m_pData[nIndex]; }

 protected:
  void Expand (unsigned long nGrow);

  T** m_pData;
  unsigned long m_nLength;
  unsigned long m_nAlloc;
};

#include "array.cpp"

#endif // _ARRAY_H_

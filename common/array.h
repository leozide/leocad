#ifndef _ARRAY_H_
#define _ARRAY_H_

template <class T>
class PtrArray
{
 public:
  PtrArray (int nSize = 0);
  ~PtrArray ();

  void SetSize (int nSize);
  int GetSize () const
    { return m_nLength; }

  T* RemoveIndex (int nIndex);
  T* RemovePointer (T* pObj);
  void Add (T* pObj);

  T* operator [](int nIndex) const
    { return m_pData[nIndex]; }

 protected:
  void Expand (int nGrow);

  T** m_pData;
  int m_nLength;
  int m_nAlloc;
};

#include "array.cpp"

#endif // _ARRAY_H_

//
//	file.h
////////////////////////////////////////////////////

#ifndef _FILE_H_
#define _FILE_H_

#include <stdio.h>

class File
{
public:
// Constructors
	File(bool bMemFile);
	~File();

// Implementation
protected:
	bool m_bMemFile;

	// MemFile specific:
	unsigned long m_nGrowBytes;
	unsigned long m_nPosition;
	unsigned long m_nBufferSize;
	unsigned long m_nFileSize;
	unsigned char* m_pBuffer;
	bool m_bAutoDelete;
	void GrowFile(unsigned long nNewLen);

	// DiscFile specific:
	FILE* m_hFile;
	bool m_bCloseOnDelete;

public:
	unsigned long GetPosition() const;
	unsigned long Seek(long lOff, int nFrom);
	void SetLength(unsigned long nNewLen);
	unsigned long GetLength() const;

	char* ReadString(char* pBuf, unsigned long nMax);
	unsigned long Read(void* pBuf, unsigned long nCount);
	unsigned long Write(const void* pBuf, unsigned long nCount);
	int GetChar();
	int PutChar(int c);

	void Abort();
	void Flush();
	void Close();
	bool Open(const char *filename, const char *mode);

public:
// Attributes
//	CString GetFileName() const;
//	CString GetFileTitle() const;
//	CString GetFilePath() const;
//	void SetFilePath(LPCTSTR lpszNewName);

protected:
//	CString m_strFileName;
};


#endif // _FILE_H_

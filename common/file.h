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
	File();
	virtual ~File();

// Implementation
public:
	virtual unsigned long GetPosition() const = 0;
	virtual unsigned long Seek(long lOff, int nFrom) = 0;
	virtual void SetLength(unsigned long nNewLen) = 0;
	virtual unsigned long GetLength() const = 0;

	virtual char* ReadString(char* pBuf, unsigned long nMax)=0;
	virtual unsigned long Read(void* pBuf, unsigned long nCount)=0;
	virtual unsigned long Write(const void* pBuf, unsigned long nCount)=0;
	virtual int GetChar()=0;
	virtual int PutChar(int c)=0;

	unsigned long ReadByte(void* pBuf, unsigned long nCount);
	unsigned long ReadShort(void* pBuf, unsigned long nCount);
	unsigned long ReadLong(void* pBuf, unsigned long nCount);
	unsigned long WriteByte(const void* pBuf, unsigned long nCount);
	unsigned long WriteShort(const void* pBuf, unsigned long nCount);
	unsigned long WriteLong(const void* pBuf, unsigned long nCount);

	virtual void Abort()=0;
	virtual void Flush()=0;
	virtual void Close()=0;
};

class FileMem : public File
{
public:
// Constructors
	FileMem();
	~FileMem();

// Implementation
protected:
	// MemFile specific:
	unsigned long m_nGrowBytes;
	unsigned long m_nPosition;
	unsigned long m_nBufferSize;
	unsigned long m_nFileSize;
	unsigned char* m_pBuffer;
	bool m_bAutoDelete;
	void GrowFile(unsigned long nNewLen);

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
};

class FileDisk : public File
{
public:
// Constructors
	FileDisk();
	~FileDisk();

// Implementation
protected:
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
};











#endif // _FILE_H_

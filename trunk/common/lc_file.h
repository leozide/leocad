#ifndef _FILE_H_
#define _FILE_H_

#include <stdio.h>
#include <string.h>

class String;

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

	virtual char* ReadLine(char* pBuf, unsigned long nMax)=0;
	virtual unsigned long Read(void* pBuf, unsigned long nCount)=0;
	virtual unsigned long Write(const void* pBuf, unsigned long nCount)=0;
	virtual int GetChar()=0;
	virtual int PutChar(int c)=0;

	unsigned long ReadByte(void* pBuf, unsigned long nCount);
	unsigned long ReadShort(void* pBuf, unsigned long nCount);
	unsigned long ReadLong(void* pBuf, unsigned long nCount);
	unsigned long ReadFloat(void* pBuf, unsigned long nCount);
	unsigned long ReadDouble(void* pBuf, unsigned long nCount);
	unsigned long WriteByte(const void* pBuf, unsigned long nCount);
	unsigned long WriteShort(const void* pBuf, unsigned long nCount);
	unsigned long WriteLong(const void* pBuf, unsigned long nCount);
	unsigned long WriteFloat(const void* pBuf, unsigned long nCount);
	unsigned long WriteDouble(const void* pBuf, unsigned long nCount);

	void ReadString(String& Value);
	void ReadInt(lcint32* Value)
	{ ReadLong(Value, 1); }
	void ReadInt(lcuint32* Value)
	{ ReadLong(Value, 1); }

	void WriteString(const String& Value);
	void WriteInt(lcint32 Value)
	{ WriteLong(&Value, 1); }
	void WriteInt(lcuint32 Value)
	{ WriteLong(&Value, 1); }

	void WriteLine(const char* pBuf)
	{ WriteByte(pBuf, strlen(pBuf)); }

	virtual void Abort()=0;
	virtual void Flush()=0;
	virtual void Close()=0;

	const char* GetFileName() const
	{ return FileName; }
	
	void SetFileName(const char* Name)
	{ strncpy(FileName, Name, LC_MAXPATH); }

protected:
	char FileName[LC_MAXPATH];
};

class FileMem : public File
{
public:
// Constructors
	FileMem();
	~FileMem();

// Implementation
public:
	unsigned long GetPosition() const;
	unsigned long Seek(long lOff, int nFrom);
	void SetLength(unsigned long nNewLen);
	unsigned long GetLength() const;

	char* ReadLine(char* pBuf, unsigned long nMax);
	unsigned long Read(void* pBuf, unsigned long nCount);
	unsigned long Write(const void* pBuf, unsigned long nCount);
	int GetChar();
	int PutChar(int c);

	void Abort();
	void Flush();
	void Close();
	bool Open(const char *filename, const char *mode);

	void* GetBuffer() const
	{
		return m_pBuffer;
	}

protected:
	// MemFile specific:
	unsigned long m_nGrowBytes;
	unsigned long m_nPosition;
	unsigned long m_nBufferSize;
	unsigned long m_nFileSize;
	unsigned char* m_pBuffer;
	bool m_bAutoDelete;
	void GrowFile(unsigned long nNewLen);
};

class FileDisk : public File
{
public:
// Constructors
	FileDisk();
	~FileDisk();

// Implementation
public:
	unsigned long GetPosition() const;
	unsigned long Seek(long lOff, int nFrom);
	void SetLength(unsigned long nNewLen);
	unsigned long GetLength() const;

	char* ReadLine(char* pBuf, unsigned long nMax);
	unsigned long Read(void* pBuf, unsigned long nCount);
	unsigned long Write(const void* pBuf, unsigned long nCount);
	int GetChar();
	int PutChar(int c);

	void Abort();
	void Flush();
	void Close();
	bool Open(const char *filename, const char *mode);

protected:
	// DiscFile specific:
	FILE* m_hFile;
	bool m_bCloseOnDelete;
};

#endif // _FILE_H_

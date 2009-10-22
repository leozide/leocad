#ifndef _FILE_H_
#define _FILE_H_

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "defines.h"

class String;

class lcFile
{
public:
	lcFile();
	virtual ~lcFile();

	virtual long GetPosition() const = 0;
	virtual long Seek(long Offset, int From) = 0;
	virtual void SetLength(long Length) = 0;
	virtual long GetLength() const = 0;

	virtual char* ReadLine(char* Buffer, unsigned long Max)=0;
	virtual size_t Read(void* Buffer, size_t Count)=0;
	virtual size_t Write(const void* Buffer, size_t Count)=0;
	virtual int GetChar()=0;
	virtual int PutChar(int c)=0;

	size_t ReadBytes(void* Buffer, size_t Count = 1)
	{ return Read(Buffer, Count); }
	size_t WriteBytes(const void* Buffer, size_t Count = 1)
	{ return Write(Buffer, Count); }

	size_t ReadShorts(void* Buffer, size_t Count = 1);
	size_t WriteShorts(const void* Buffer, size_t Count = 1);

	size_t ReadInts(void* Buffer, size_t Count = 1);
	size_t WriteInts(const void* Buffer, size_t Count = 1);

	size_t ReadFloats(void* Buffer, size_t Count = 1)
	{ return ReadInts(Buffer, Count); }
	size_t WriteFloats(const void* Buffer, size_t Count = 1)
	{ return WriteInts(Buffer, Count); }

	size_t ReadDoubles(void* Buffer, size_t Count = 1);
	size_t WriteDoubles(const void* Buffer, size_t Count = 1);

	void ReadString(String& Value);
	void WriteString(const String& Value);

	virtual void Abort()=0;
	virtual void Flush()=0;
	virtual void Close()=0;

	const char* GetFileName() const
	{ return mFileName; }
	
	void SetFileName(const char* FileName)
	{ strncpy(mFileName, FileName, LC_MAXPATH); }

protected:
	char mFileName[LC_MAXPATH];
};

class lcFileMem : public lcFile
{
public:
	lcFileMem();
	~lcFileMem();

	long GetPosition() const;
	long Seek(long Offset, int From);
	void SetLength(long Length);
	long GetLength() const;

	char* ReadLine(char* Buffer, size_t Max);
	size_t Read(void* Buffer, size_t Count);
	size_t Write(const void* Buffer, size_t Count);
	int GetChar();
	int PutChar(int c);

	void Abort();
	void Flush();
	void Close();
	bool Open(const char* FileName, const char* Mode);

protected:
	void GrowFile(long Length);

	long mGrowBytes;
	long mPosition;
	long mBufferSize;
	long mFileSize;
	unsigned char* mBuffer;
	bool mAutoDelete;
};

class lcFileDisk : public lcFile
{
public:
	lcFileDisk();
	~lcFileDisk();

	long GetPosition() const;
	long Seek(long Offset, int From);
	void SetLength(long Length);
	long GetLength() const;

	char* ReadLine(char* Buffer, unsigned long Max);
	unsigned long Read(void* Buffer, size_t Count);
	unsigned long Write(const void* Buffer, size_t Count);
	int GetChar();
	int PutChar(int c);

	void Abort();
	void Flush();
	void Close();
	bool Open(const char *filename, const char *mode);

protected:
	FILE* mFile;
	bool mCloseOnDelete;
};

#endif // _FILE_H_


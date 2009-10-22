#include "lc_global.h"
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "defines.h"
#include "str.h"

lcFile::lcFile()
{
	strcpy(mFileName, "");
}

lcFile::~lcFile()
{
}

size_t lcFile::ReadShorts(void* Buffer, size_t Count)
{
	size_t ReadCount = Read(Buffer, Count * 2) / 2;

#if LC_BIG_ENDIAN
	for (size_t i = 0, u16* val = (u16*)Buffer; i < ReadCount; i++, val++)
		*val = LCUINT16(*val);
#endif

	return ReadCount;
}

size_t lcFile::ReadInts(void* Buffer, size_t Count)
{
	size_t ReadCount = Read(Buffer, Count * 4) / 4;

#if LC_BIG_ENDIAN
	for (size_t i = 0, u32* val = (u32*)Buffer; i < ReadCount; i++, val++)
		*val = LCUINT32(*val);
#endif

  return ReadCount;
}

size_t lcFile::ReadDoubles(void* Buffer, size_t Count)
{
  size_t ReadCount = Read(Buffer, Count * 8) / 8;

#if LC_BIG_ENDIAN
	union { unsigned char b[8]; double d; } in, out;

	for (size_t i = 0, double* val = (double*)Buffer; i < ReadCount; i++, val++)
	{
		in.d = *val;

		out.b[0] = in.b[7];
		out.b[1] = in.b[6];
		out.b[2] = in.b[5];
		out.b[3] = in.b[4];
		out.b[4] = in.b[3];
		out.b[5] = in.b[2];
		out.b[6] = in.b[1];
		out.b[7] = in.b[0];

		*val = out.d;
	}
#endif

	return ReadCount;
}

size_t lcFile::WriteShorts(const void* Buffer, size_t Count)
{
#if LC_BIG_ENDIAN
	size_t WriteCount = 0;

	for (size_t i = 0, u16* val = (u16*)Buffer; i < Count; i++, val++)
	{
		u16 x = LCUINT16(*val);
		WriteCount += Write(&x, 2) / 2;
	}

	return WriteCount;
#else
	return Write(Buffer, Count * 2) / 2;
#endif
}

size_t lcFile::WriteInts(const void* Buffer, size_t Count)
{
#if LC_BIG_ENDIAN
	size_t WriteCount = 0;

	for (size_t i = 0, u32* val = (u32*)Buffer; i < Count; i++, val++)
	{
		u32 x = LCUINT32(*val);
		WriteCount += Write(&x, 4) / 4;
	}

	return WriteCount;
#else
	return Write(Buffer, Count * 4) / 4;
#endif
}

size_t lcFile::WriteDoubles(const void* Buffer, size_t Count)
{
#if LC_BIG_ENDIAN
	size_t WriteCount = 0;
	union { unsigned char b[8]; double d; } in, out;

	for (size_t i = 0, double* val = (double*)Buffer; i < Count; i++, val++)
	{
		in.d = *val;

		out.b[0] = in.b[7];
		out.b[1] = in.b[6];
		out.b[2] = in.b[5];
		out.b[3] = in.b[4];
		out.b[4] = in.b[3];
		out.b[5] = in.b[2];
		out.b[6] = in.b[1];
		out.b[7] = in.b[0];

		WriteCount += Write(&out, 8) / 8;
	}

	return WriteCount;
#else
	return Write(Buffer, Count * 8) / 8;
#endif
}

void lcFile::ReadString(String& Value)
{
	u32 l;
	ReadInts(&l);
	Read(Value.GetBuffer(l+1), l);
	((char*)Value)[l] = 0;
}

void lcFile::WriteString(const String& Value)
{
	int Length = Value.GetLength();
	WriteInts(&Length);
	Write((const char*)Value, Length);
}

lcFileMem::lcFileMem()
{
	mGrowBytes = 1024;
	mPosition = 0;
	mBufferSize = 0;
	mFileSize = 0;
	mBuffer = NULL;
	mAutoDelete = true;
}

lcFileDisk::lcFileDisk()
{
	mFile = NULL;
	mCloseOnDelete = false;
}

lcFileMem::~lcFileMem()
{
	if (mBuffer)
		Close();

	mGrowBytes = 0;
	mPosition = 0;
	mBufferSize = 0;
	mFileSize = 0;
}

lcFileDisk::~lcFileDisk()
{
	if (mFile && mCloseOnDelete)
		Close();
}

char* lcFileMem::ReadLine(char* Buffer, size_t Max)
{
	int Read = 0;
	unsigned char ch;

	if (Max <= 0)
		return NULL;
	if (mPosition >= mFileSize)
		return NULL;

	while ((--Max))
	{
		if (mPosition == mFileSize)
			break;

		ch = mBuffer[mPosition];
		mPosition++;
		Buffer[Read++] = ch;

		if (ch == '\n')
			break;
	}

	Buffer[Read] = '\0';
	return Buffer;
}

char* lcFileDisk::ReadLine(char* Buffer, size_t Max)
{
	return fgets(Buffer, Max, mFile);
}

size_t lcFileMem::Read(void* Buffer, size_t Count)
{
	if (!Count)
		return 0;

	if (mPosition > mFileSize)
		return 0;

	size_t Read;
	if (mPosition + Count > (size_t)mFileSize)
		Read = mFileSize - mPosition;
	else
		Read = Count;

	memcpy(Buffer, (unsigned char*)mBuffer + mPosition, Read);
	mPosition += Read;

	return Read;
}

size_t lcFileDisk::Read(void* Buffer, size_t Count)
{
	return fread(Buffer, 1, Count, mFile);
}

int lcFileMem::GetChar()
{
	if (mPosition > mFileSize)
		return EOF;

	unsigned char* ret = (unsigned char*)mBuffer + mPosition;
	mPosition++;

	return *ret;
}

int lcFileDisk::GetChar()
{
	return fgetc(mFile);
}

size_t lcFileMem::Write(const void* Buffer, size_t Count)
{
	if (!Count)
		return 0;

	if (mPosition + Count > (size_t)mBufferSize)
		GrowFile(mPosition + Count);

	memcpy((unsigned char*)mBuffer + mPosition, Buffer, Count);

	mPosition += Count;

	if (mPosition > mFileSize)
		mFileSize = mPosition;

	return Count;
}

size_t lcFileDisk::Write(const void* Buffer, size_t Count)
{
  return fwrite(Buffer, 1, Count, mFile);
}

int lcFileMem::PutChar(int c)
{
	if (mPosition + 1 > mBufferSize)
		GrowFile(mPosition + 1);

	unsigned char* bt = (unsigned char*)mBuffer + mPosition;
	*bt = c;
	mPosition++;

	if (mPosition > mFileSize)
		mFileSize = mPosition;

	return 1;
}

int lcFileDisk::PutChar(int c)
{
	return fputc(c, mFile);
}

bool lcFileDisk::Open(const char* FileName, const char* Mode)
{
	strcpy(mFileName, FileName);

	mFile = fopen(mFileName, Mode);
	mCloseOnDelete = true;

	return (mFile != NULL);
}

void lcFileMem::Close()
{
	if (mBuffer && mAutoDelete)
		free(mBuffer);

	mGrowBytes = 0;
	mPosition = 0;
	mBufferSize = 0;
	mFileSize = 0;
	mBuffer = NULL;
	strcpy(mFileName, "");
}

void lcFileDisk::Close()
{
	if (mFile)
		fclose(mFile);

	mFile = NULL;
	mCloseOnDelete = false;
	strcpy(mFileName, "");
}

long lcFileMem::Seek(long Offset, int From)
{
	if (From == SEEK_SET)
		mPosition = Offset;
	else if (From == SEEK_CUR)
		mPosition += Offset;
	else if (From == SEEK_END)
		mPosition = mFileSize + Offset;
	else
		return -1;

	return 0;
}

long lcFileDisk::Seek(long Offset, int From)
{
	fseek(mFile, Offset, From);

	return ftell(mFile);
}

long lcFileMem::GetPosition() const
{
    return mPosition;
}

long lcFileDisk::GetPosition() const
{
	return ftell(mFile);
}

void lcFileMem::GrowFile(long Length)
{
	if (Length > mBufferSize)
	{
		long NewBufferSize = mBufferSize;

		while (NewBufferSize < Length)
			NewBufferSize += mGrowBytes;

		if (!mBuffer)
			mBuffer = (unsigned char*)malloc(NewBufferSize);
		else
			mBuffer = (unsigned char*)realloc(mBuffer, NewBufferSize);

		mBufferSize = NewBufferSize;
	}
}

void lcFileMem::Flush()
{
}

void lcFileDisk::Flush()
{
	if (mFile)
		fflush(mFile);
}

void lcFileMem::Abort()
{
	Close();
}

void lcFileDisk::Abort()
{
	if (mFile)
	{
		if (mCloseOnDelete)
		  fclose(mFile);
		mFile = NULL;
		mCloseOnDelete = false;
	}
}

void lcFileMem::SetLength(long Length)
{
	if (Length > mBufferSize)
		GrowFile(Length);

	if (Length < mPosition)
		mPosition = Length;

	mFileSize = Length;
}

void lcFileDisk::SetLength(long Length)
{
	fseek(mFile, Length, SEEK_SET);
}

long lcFileMem::GetLength() const
{
	return mFileSize;
}

long lcFileDisk::GetLength() const
{
	long Length, Current;

	Current = ftell(mFile);
	fseek(mFile, 0, SEEK_END);
	Length = ftell(mFile);
	fseek(mFile, Current, SEEK_SET);

	return Length;
}


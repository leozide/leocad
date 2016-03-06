#include "lc_global.h"
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include "lc_file.h"

// =============================================================================
// lcFile

lcFile::lcFile()
{
}

lcFile::~lcFile()
{
}

// =============================================================================
// lcMemFile

lcMemFile::lcMemFile()
{
	mGrowBytes = 1024;
	mPosition = 0;
	mBufferSize = 0;
	mFileSize = 0;
	mBuffer = NULL;
}

lcMemFile::~lcMemFile()
{
	Close();
}

void lcMemFile::Seek(long Offset, int From)
{
	if (From == SEEK_SET)
		mPosition = Offset;
	else if (From == SEEK_CUR)
		mPosition += Offset;
	else if (From == SEEK_END)
		mPosition = mFileSize + Offset;
}

long lcMemFile::GetPosition() const
{
	return (long)mPosition;
}

void lcMemFile::SetLength(size_t NewLength)
{
	if (NewLength > mBufferSize)
		GrowFile(NewLength);

	if (NewLength < mPosition)
		mPosition = NewLength;

	mFileSize = NewLength;
}

size_t lcMemFile::GetLength() const
{
	return mFileSize;
}

void lcMemFile::Flush()
{
}

void lcMemFile::Close()
{
	if (!mBuffer)
		return;

	mPosition = 0;
	mBufferSize = 0;
	mFileSize = 0;
	free(mBuffer);
	mBuffer = NULL;
}

size_t lcMemFile::ReadBuffer(void* Buffer, size_t Bytes)
{
	if (Bytes == 0 || mPosition > mFileSize)
		return 0;

	size_t BytesToRead;

	if (mPosition + Bytes > mFileSize)
		BytesToRead = mFileSize - mPosition;
	else
		BytesToRead = Bytes;

	memcpy(Buffer, mBuffer + mPosition, BytesToRead);
	mPosition += BytesToRead;

	return BytesToRead;
}

size_t lcMemFile::WriteBuffer(const void* Buffer, size_t Bytes)
{
	if (Bytes == 0)
		return 0;

	if (mPosition + Bytes > mBufferSize)
		GrowFile(mPosition + Bytes);

	memcpy(mBuffer + mPosition, Buffer, Bytes);

	mPosition += Bytes;

	if (mPosition > mFileSize)
		mFileSize = mPosition;

	return Bytes;
}

void lcMemFile::GrowFile(size_t NewLength)
{
	if (NewLength <= mBufferSize)
		return;

	NewLength = ((NewLength + mGrowBytes - 1) / mGrowBytes) * mGrowBytes;

	if (mBuffer != NULL)
		mBuffer = (unsigned char*)realloc(mBuffer, NewLength);
	else
		mBuffer = (unsigned char*)malloc(NewLength);

	mBufferSize = NewLength;
}

char* lcMemFile::ReadLine(char* Buffer, size_t BufferSize)
{
	int BytesRead = 0;
	unsigned char ch;

	if (BufferSize == 0)
		return NULL;

	if (mPosition >= mFileSize)
		return NULL;

	while ((--BufferSize))
	{
		if (mPosition == mFileSize)
			break;

		ch = mBuffer[mPosition];
		mPosition++;
		Buffer[BytesRead++] = ch;

		if (ch == '\n')
			break;
	}

	Buffer[BytesRead] = 0;
	return Buffer;
}

void lcMemFile::CopyFrom(lcFile& Source)
{
	size_t Length = Source.GetLength();

	SetLength(Length);
	Seek(0, SEEK_SET);

	Source.Seek(0, SEEK_SET);
	Source.ReadBuffer(mBuffer, Length);
}

void lcMemFile::CopyFrom(lcMemFile& Source)
{
	size_t Length = Source.GetLength();

	SetLength(Length);
	Seek(0, SEEK_SET);

	Source.Seek(0, SEEK_SET);
	Source.ReadBuffer(mBuffer, Length);
}

// =============================================================================
// lcDiskFile

lcDiskFile::lcDiskFile()
{
	mFile = NULL;
}

lcDiskFile::~lcDiskFile()
{
	Close();
}

long lcDiskFile::GetPosition() const
{
	return ftell(mFile);
}

void lcDiskFile::Seek(long Offset, int From)
{
	fseek(mFile, Offset, From);
}

void lcDiskFile::SetLength(size_t NewLength)
{
	fseek(mFile, (int)NewLength, SEEK_SET);
}

size_t lcDiskFile::GetLength() const
{
	long Length, Current;

	Current = ftell(mFile);
	fseek(mFile, 0, SEEK_END);
	Length = ftell(mFile);
	fseek(mFile, Current, SEEK_SET);

	return Length;
}

void lcDiskFile::Flush()
{
	if (mFile == NULL)
		return;

	fflush(mFile);
}

void lcDiskFile::Close()
{
	if (mFile == NULL)
		return;

	fclose(mFile);
	mFile = NULL;
}

size_t lcDiskFile::ReadBuffer(void* Buffer, size_t Bytes)
{
	return fread(Buffer, 1, Bytes, mFile);
}

size_t lcDiskFile::WriteBuffer(const void* Buffer, size_t Bytes)
{
	return fwrite(Buffer, 1, Bytes, mFile);
}

bool lcDiskFile::Open(const QString& FileName, const char* Mode)
{
	return Open(FileName.toLatin1().constData(), Mode); // todo: qstring
}

bool lcDiskFile::Open(const char* FileName, const char* Mode)
{
	if (*FileName == 0)
		return false;

	Close();

	mFile = fopen(FileName, Mode);

	return (mFile != NULL);
}

char* lcDiskFile::ReadLine(char* Buffer, size_t BufferSize)
{
	return fgets(Buffer, (int)BufferSize, mFile);
}

void lcDiskFile::CopyFrom(lcMemFile& Source)
{
	size_t Length = Source.GetLength();

	Seek(0, SEEK_SET);
	WriteBuffer(Source.mBuffer, Length);
}

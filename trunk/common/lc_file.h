#ifndef _FILE_H_
#define _FILE_H_

#include <stdio.h>
#include <string.h>
#include "lc_math.h"

#define LC_FOURCC(ch0, ch1, ch2, ch3) (lcuint32)((lcuint32)(lcuint8)(ch0) | ((lcuint32)(lcuint8)(ch1) << 8) | \
												((lcuint32)(lcuint8)(ch2) << 16) | ((lcuint32)(lcuint8)(ch3) << 24 ))

#define LC_FILE_ID LC_FOURCC('L','C','D', 0)

class String;

class lcFile
{
public:
	lcFile();
	virtual ~lcFile();

	virtual long GetPosition() const = 0;
	virtual void Seek(long Offset, int From) = 0;
	virtual void SetLength(size_t NewLength) = 0;
	virtual size_t GetLength() const = 0;

	virtual void Flush() = 0;
	virtual void Close() = 0;

	virtual char* ReadLine(char* Buffer, size_t BufferSize) = 0;
	void WriteLine(const char* Buffer)
	{
		WriteBuffer(Buffer, strlen(Buffer));
	}

	void ReadString(String& Value);
	void WriteString(const String& Value);

	virtual size_t ReadBuffer(void* Buffer, long Bytes) = 0;
	virtual size_t WriteBuffer(const void* Buffer, long Bytes) = 0;
	virtual void CopyFrom(lcMemFile& Source) = 0;

	lcuint8 ReadU8()
	{
		lcuint8 Value;
		Read8(&Value, 1);
		return Value;
	}

	size_t ReadU8(lcuint8* Buffer, size_t Count)
	{
		return Read8(Buffer, Count);
	}

	lcint8 ReadS8()
	{
		lcint8 Value;
		Read8(&Value, 1);
		return Value;
	}

	size_t ReadS8(lcint8* Buffer, size_t Count)
	{
		return Read8(Buffer, Count);
	}

	lcuint16 ReadU16()
	{
		lcuint16 Value;
		Read16(&Value, 1);
		return Value;
	}

	size_t ReadU16(lcuint16* Buffer, size_t Count)
	{
		return Read16(Buffer, Count);
	}

	lcint16 ReadS16()
	{
		lcint16 Value;
		Read16(&Value, 1);
		return Value;
	}

	size_t ReadS16(lcint16* Buffer, size_t Count)
	{
		return Read16(Buffer, Count);
	}

	lcuint32 ReadU32()
	{
		lcuint32 Value;
		Read32(&Value, 1);
		return Value;
	}

	size_t ReadU32(lcuint32* Buffer, size_t Count)
	{
		return Read32(Buffer, Count);
	}

	lcint32 ReadS32()
	{
		lcint32 Value;
		Read32(&Value, 1);
		return Value;
	}

	size_t ReadS32(lcint32* Buffer, size_t Count)
	{
		return Read32(Buffer, Count);
	}

	lcuint64 ReadU64()
	{
		lcuint64 Value;
		Read64(&Value, 1);
		return Value;
	}

	size_t ReadU64(lcuint64* Buffer, size_t Count)
	{
		return Read64(Buffer, Count);
	}

	lcint64 ReadS64()
	{
		lcint64 Value;
		Read64(&Value, 1);
		return Value;
	}

	size_t ReadS64(lcint64* Buffer, size_t Count)
	{
		return Read64(Buffer, Count);
	}

	float ReadFloat()
	{
		float Value;
		Read32(&Value, 1);
		return Value;
	}

	size_t ReadFloats(float* Buffer, size_t Count)
	{
		return Read32(Buffer, Count);
	}

	double ReadDouble()
	{
		double Value;
		Read64(&Value, 1);
		return Value;
	}

	lcVector3 ReadVector3()
	{
		lcVector3 Vector;
		ReadFloats(Vector, 3);
		return Vector;
	}

	size_t ReadDoubles(double* Buffer, size_t Count)
	{
		return Read64(Buffer, Count);
	}

	void WriteU8(const lcuint8& Value)
	{
		Write8(&Value, 1);
	}

	size_t WriteU8(const lcuint8* Buffer, size_t Count)
	{
		return Write8(Buffer, Count);
	}

	void WriteS8(const lcint8& Value)
	{
		Write8(&Value, 1);
	}

	size_t WriteS8(const lcint8* Buffer, size_t Count)
	{
		return Write8(Buffer, Count);
	}

	void WriteU16(const lcuint16& Value)
	{
		Write16(&Value, 1);
	}

	size_t WriteU16(const lcuint16* Buffer, size_t Count)
	{
		return Write16(Buffer, Count);
	}

	void WriteS16(const lcint16& Value)
	{
		Write16(&Value, 1);
	}

	size_t WriteS16(const lcint16* Buffer, size_t Count)
	{
		return Write16(Buffer, Count);
	}

	void WriteU32(const lcuint32& Value)
	{
		Write32(&Value, 1);
	}

	size_t WriteU32(const lcuint32* Buffer, size_t Count)
	{
		return Write32(Buffer, Count);
	}

	void WriteS32(const lcint32& Value)
	{
		Write32(&Value, 1);
	}

	size_t WriteS32(const lcint32* Buffer, size_t Count)
	{
		return Write32(Buffer, Count);
	}

	void WriteU64(const lcuint64& Value)
	{
		Write64(&Value, 1);
	}

	size_t WriteU64(const lcuint64* Buffer, size_t Count)
	{
		return Write64(Buffer, Count);
	}

	void WriteS64(const lcint64& Value)
	{
		Write64(&Value, 1);
	}

	size_t WriteS64(const lcint64* Buffer, size_t Count)
	{
		return Write64(Buffer, Count);
	}

	void WriteFloat(const float& Value)
	{
		Write32(&Value, 1);
	}

	size_t WriteFloats(const float* Buffer, size_t Count)
	{
		return Write32(Buffer, Count);
	}

	void WriteDouble(const double& Value)
	{
		Write64(&Value, 1);
	}

	size_t WriteDoubles(const double* Buffer, size_t Count)
	{
		return Write64(Buffer, Count);
	}

	void WriteVector3(const lcVector3& Vector)
	{
		WriteFloats(Vector, 3);
	}

protected:
	size_t Read8(void* Buffer, size_t Count)
	{
		return ReadBuffer(Buffer, Count);
	}

	size_t Read16(void* Buffer, size_t Count)
	{
		size_t NumRead;

		NumRead = ReadBuffer(Buffer, Count * 2) / 2;

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
		lcuint8 Temp[2];
		lcuint8* Bytes = (lcuint8*)Buffer;

		for (size_t Idx = 0; Idx < NumRead; Idx++)
		{
			Temp[0] = Bytes[0];
			Temp[1] = Bytes[1];

			*Bytes++ = Temp[1];
			*Bytes++ = Temp[0];
		}
#endif

		return NumRead;
	}

	size_t Read32(void* Buffer, size_t Count)
	{
		size_t NumRead;

		NumRead = ReadBuffer(Buffer, Count * 4) / 4;

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
		lcuint8 Temp[4];
		lcuint8* Bytes = (lcuint8*)Buffer;

		for (size_t Idx = 0; Idx < NumRead; Idx++)
		{
			Temp[0] = Bytes[0];
			Temp[1] = Bytes[1];
			Temp[2] = Bytes[2];
			Temp[3] = Bytes[3];

			*Bytes++ = Temp[3];
			*Bytes++ = Temp[2];
			*Bytes++ = Temp[1];
			*Bytes++ = Temp[0];
		}
#endif

		return NumRead;
	}

	size_t Read64(void* Buffer, size_t Count)
	{
		size_t NumRead;

		NumRead = ReadBuffer(Buffer, Count * 8) / 8;

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
		lcuint8 Temp[8];
		lcuint8* Bytes = (lcuint8*)Buffer;

		for (size_t Idx = 0; Idx < NumRead; Idx++)
		{
			Temp[0] = Bytes[0];
			Temp[1] = Bytes[1];
			Temp[2] = Bytes[2];
			Temp[3] = Bytes[3];
			Temp[4] = Bytes[4];
			Temp[5] = Bytes[5];
			Temp[6] = Bytes[6];
			Temp[7] = Bytes[7];

			*Bytes++ = Temp[7];
			*Bytes++ = Temp[6];
			*Bytes++ = Temp[5];
			*Bytes++ = Temp[4];
			*Bytes++ = Temp[3];
			*Bytes++ = Temp[2];
			*Bytes++ = Temp[1];
			*Bytes++ = Temp[0];
		}
#endif

		return NumRead;
	}

	size_t Write8(const void* Buffer, size_t Count)
	{
		return WriteBuffer(Buffer, Count);
	}

	size_t Write16(const void* Buffer, size_t Count)
	{
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
		size_t BytesWritten = 0;
		lcuint8 Temp[2];
		lcuint8* Bytes = (lcuint8*)Buffer;

		for (size_t Idx = 0; Idx < Count; Idx++)
		{
			Temp[1] = *Bytes++;
			Temp[0] = *Bytes++;

			BytesWritten += WriteBuffer(Temp, 2);
		}

		return BytesWritten / 2;
#else
		return WriteBuffer(Buffer, Count * 2) / 2;
#endif
	}

	size_t Write32(const void* Buffer, size_t Count)
	{
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
		size_t BytesWritten = 0;
		lcuint8 Temp[4];
		lcuint8* Bytes = (lcuint8*)Buffer;

		for (size_t Idx = 0; Idx < Count; Idx++)
		{
			Temp[3] = *Bytes++;
			Temp[2] = *Bytes++;
			Temp[1] = *Bytes++;
			Temp[0] = *Bytes++;

			BytesWritten += WriteBuffer(Temp, 4);
		}

		return BytesWritten / 4;
#else
		return WriteBuffer(Buffer, Count * 4);
#endif
	}

	size_t Write64(const void* Buffer, size_t Count)
	{
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
		size_t BytesWritten = 0;
		lcuint8 Temp[8];
		lcuint8* Bytes = (lcuint8*)Buffer;

		for (size_t Idx = 0; Idx < Count; Idx++)
		{
			Temp[7] = *Bytes++;
			Temp[6] = *Bytes++;
			Temp[5] = *Bytes++;
			Temp[4] = *Bytes++;
			Temp[3] = *Bytes++;
			Temp[2] = *Bytes++;
			Temp[1] = *Bytes++;
			Temp[0] = *Bytes++;

			BytesWritten += WriteBuffer(Temp, 8);
		}

		return BytesWritten / 8;
#else
		return WriteBuffer(Buffer, Count * 8);
#endif
	}
};

class lcMemFile : public lcFile
{
public:
	lcMemFile();
	virtual ~lcMemFile();

	long GetPosition() const;
	void Seek(long Offset, int From);
	void SetLength(size_t NewLength);
	size_t GetLength() const;

	void Flush();
	void Close();

	char* ReadLine(char* Buffer, size_t BufferSize);
	size_t ReadBuffer(void* Buffer, long Bytes);
	size_t WriteBuffer(const void* Buffer, long Bytes);

	void CopyFrom(lcFile& Source);
	void CopyFrom(lcMemFile& Source);
	void GrowFile(size_t NewLength);

	size_t mGrowBytes;
	size_t mPosition;
	size_t mBufferSize;
	size_t mFileSize;
	unsigned char* mBuffer;
};

class lcDiskFile : public lcFile
{
public:
	lcDiskFile();
	virtual ~lcDiskFile();

	long GetPosition() const;
	void Seek(long Offset, int From);
	void SetLength(size_t NewLength);
	size_t GetLength() const;

	void Flush();
	void Close();

	char* ReadLine(char* Buffer, size_t BufferSize);
	size_t ReadBuffer(void* Buffer, long Bytes);
	size_t WriteBuffer(const void* Buffer, long Bytes);

	void CopyFrom(lcMemFile& Source);

	bool Open(const char* FileName, const char* Mode);

	FILE* mFile;
};

#endif // _FILE_H_

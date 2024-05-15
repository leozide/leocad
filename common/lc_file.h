#pragma once

#include <stdio.h>
#include <string.h>
#include "lc_math.h"

#define LC_FOURCC(ch0, ch1, ch2, ch3) (quint32)((quint32)(quint8)(ch0) | ((quint32)(quint8)(ch1) << 8) | \
												((quint32)(quint8)(ch2) << 16) | ((quint32)(quint8)(ch3) << 24 ))

class lcFile
{
public:
	lcFile()
	{
	}

	virtual ~lcFile()
	{
	}

	lcFile(const lcFile&) = delete;
	lcFile(lcFile&&) = delete;
	lcFile& operator=(const lcFile&) = delete;
	lcFile& operator=(lcFile&&) = delete;

	virtual long GetPosition() const = 0;
	virtual void Seek(qint64 Offset, int From) = 0;
	virtual size_t GetLength() const = 0;

	virtual void Close() = 0;

	virtual char* ReadLine(char* Buffer, size_t BufferSize) = 0;
	void WriteLine(const char* Buffer)
	{
		WriteBuffer(Buffer, strlen(Buffer));
	}

	virtual size_t ReadBuffer(void* Buffer, size_t Bytes) = 0;
	virtual size_t WriteBuffer(const void* Buffer, size_t Bytes) = 0;

	quint8 ReadU8()
	{
		quint8 Value;
		Read8(&Value, 1);
		return Value;
	}

	size_t ReadU8(quint8* Buffer, size_t Count)
	{
		return Read8(Buffer, Count);
	}

	qint8 ReadS8()
	{
		qint8 Value;
		Read8(&Value, 1);
		return Value;
	}

	size_t ReadS8(qint8* Buffer, size_t Count)
	{
		return Read8(Buffer, Count);
	}

	quint16 ReadU16()
	{
		quint16 Value;
		Read16(&Value, 1);
		return Value;
	}

	size_t ReadU16(quint16* Buffer, size_t Count)
	{
		return Read16(Buffer, Count);
	}

	qint16 ReadS16()
	{
		qint16 Value;
		Read16(&Value, 1);
		return Value;
	}

	size_t ReadS16(qint16* Buffer, size_t Count)
	{
		return Read16(Buffer, Count);
	}

	quint32 ReadU32()
	{
		quint32 Value;
		Read32(&Value, 1);
		return Value;
	}

	size_t ReadU32(quint32* Buffer, size_t Count)
	{
		return Read32(Buffer, Count);
	}

	qint32 ReadS32()
	{
		qint32 Value;
		Read32(&Value, 1);
		return Value;
	}

	size_t ReadS32(qint32* Buffer, size_t Count)
	{
		return Read32(Buffer, Count);
	}

	quint64 ReadU64()
	{
		quint64 Value;
		Read64(&Value, 1);
		return Value;
	}

	size_t ReadU64(quint64* Buffer, size_t Count)
	{
		return Read64(Buffer, Count);
	}

	qint64 ReadS64()
	{
		qint64 Value;
		Read64(&Value, 1);
		return Value;
	}

	size_t ReadS64(qint64* Buffer, size_t Count)
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

	QString ReadQString()
	{
		const quint32 Size = ReadU32();
		char* Buffer = new char[Size];
		ReadBuffer(Buffer, Size);
		QString String = QString::fromUtf8(Buffer, Size);
		delete[] Buffer;
		return String;
	}

	void WriteU8(const quint8& Value)
	{
		Write8(&Value, 1);
	}

	size_t WriteU8(const quint8* Buffer, size_t Count)
	{
		return Write8(Buffer, Count);
	}

	void WriteS8(const qint8& Value)
	{
		Write8(&Value, 1);
	}

	size_t WriteS8(const qint8* Buffer, size_t Count)
	{
		return Write8(Buffer, Count);
	}

	void WriteU16(const quint16& Value)
	{
		Write16(&Value, 1);
	}

	size_t WriteU16(const quint16* Buffer, size_t Count)
	{
		return Write16(Buffer, Count);
	}

	void WriteS16(const qint16& Value)
	{
		Write16(&Value, 1);
	}

	size_t WriteS16(const qint16* Buffer, size_t Count)
	{
		return Write16(Buffer, Count);
	}

	void WriteU32(const quint32& Value)
	{
		Write32(&Value, 1);
	}

	size_t WriteU32(const quint32* Buffer, size_t Count)
	{
		return Write32(Buffer, Count);
	}

	void WriteS32(const qint32& Value)
	{
		Write32(&Value, 1);
	}

	size_t WriteS32(const qint32* Buffer, size_t Count)
	{
		return Write32(Buffer, Count);
	}

	void WriteU64(const quint64& Value)
	{
		Write64(&Value, 1);
	}

	size_t WriteU64(const quint64* Buffer, size_t Count)
	{
		return Write64(Buffer, Count);
	}

	void WriteS64(const qint64& Value)
	{
		Write64(&Value, 1);
	}

	size_t WriteS64(const qint64* Buffer, size_t Count)
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

	void WriteQString(const QString& String)
	{
		QByteArray Data = String.toUtf8();
		WriteU32(Data.size());
		WriteBuffer(Data, Data.size());
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
		quint8 Temp[2];
		quint8* Bytes = (quint8*)Buffer;

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
		quint8 Temp[4];
		quint8* Bytes = (quint8*)Buffer;

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
		quint8 Temp[8];
		quint8* Bytes = (quint8*)Buffer;

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
		quint8 Temp[2];
		quint8* Bytes = (quint8*)Buffer;

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
		quint8 Temp[4];
		quint8* Bytes = (quint8*)Buffer;

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
		quint8 Temp[8];
		quint8* Bytes = (quint8*)Buffer;

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
	~lcMemFile();

	lcMemFile(const lcMemFile&) = delete;
	lcMemFile(lcMemFile&&) = delete;
	lcMemFile& operator=(const lcMemFile&) = delete;
	lcMemFile& operator=(lcMemFile&&) = delete;

	long GetPosition() const override;
	void Seek(qint64 Offset, int From) override;
	void SetLength(size_t NewLength);
	size_t GetLength() const override;

	void Close() override;

	char* ReadLine(char* Buffer, size_t BufferSize) override;
	size_t ReadBuffer(void* Buffer, size_t Bytes) override;
	size_t WriteBuffer(const void* Buffer, size_t Bytes) override;

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
	lcDiskFile()
	{
	}

	lcDiskFile(const QString& FileName)
		: mFile(FileName)
	{
	}

	~lcDiskFile()
	{
		Close();
	}

	lcDiskFile(const lcDiskFile&) = delete;
	lcDiskFile(lcDiskFile&&) = delete;
	lcDiskFile& operator=(const lcDiskFile&) = delete;
	lcDiskFile& operator=(lcDiskFile&&) = delete;

	void SetFileName(const QString& FileName)
	{
		mFile.setFileName(FileName);
	}

	long GetPosition() const override
	{
		return mFile.pos();
	}

	void Seek(qint64 Offset, int From) override
	{
		switch (From)
		{
		case SEEK_CUR:
			Offset += mFile.pos();
			break;
		case SEEK_END:
			Offset += mFile.size();
			break;
		}

		mFile.seek(Offset);
	}

	size_t GetLength() const override
	{
		return mFile.size();
	}

	void Close() override
	{
		mFile.close();
	}

	char* ReadLine(char* Buffer, size_t BufferSize) override
	{
		const qint64 LineLength = mFile.readLine(Buffer, BufferSize);
		return LineLength != -1 ? Buffer : nullptr;
	}

	size_t ReadBuffer(void* Buffer, size_t Bytes) override
	{
		return mFile.read((char*)Buffer, Bytes);
	}

	size_t WriteBuffer(const void* Buffer, size_t Bytes) override
	{
		return mFile.write((const char*)Buffer, Bytes);
	}

	bool Open(QIODevice::OpenMode Flags)
	{
		return mFile.open(Flags);
	}

protected:
	QFile mFile;
};


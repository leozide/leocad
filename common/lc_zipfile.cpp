#include "lc_global.h"
#include "lc_zipfile.h"
#include "lc_file.h"
#include "lc_math.h"
#include <zlib.h>
#include <time.h>

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif

lcZipFile::lcZipFile()
{
	mModified = false;
	mFile = NULL;
}

lcZipFile::~lcZipFile()
{
	Flush();
	delete mFile;
}

bool lcZipFile::OpenRead(const char* FilePath)
{
	lcDiskFile* File = new lcDiskFile();
	mFile = File;

	if (!File->Open(FilePath, "rb") || !Open())
	{
		delete File;
		mFile = NULL;
		return false;
	}

	return true;
}

bool lcZipFile::OpenWrite(const char* FilePath, bool Append)
{
	lcDiskFile* File = new lcDiskFile();
	mFile = File;

	if (Append)
	{
		if (!File->Open(FilePath, "r+b") || !Open())
		{
			delete File;
			mFile = NULL;
			return false;
		}
	}
	else
	{
		mNumEntries = 0;
		mCentralDirSize = 0;
		mCentralDirOffset = 0;
		mBytesBeforeZipFile = 0;
		mCentralPos = 0;

		if (!File->Open(FilePath, "wb"))
		{
			delete File;
			mFile = NULL;
			return false;
		}
	}

	return true;
}

lcuint64 lcZipFile::SearchCentralDir()
{
	lcuint64 SizeFile, MaxBack, BackRead, PosFound;
	const int CommentBufferSize = 1024;
	lcuint8 buf[CommentBufferSize + 4];

	SizeFile = mFile->GetLength();
	MaxBack = lcMin(SizeFile, 0xffffU);
	BackRead = 4;
	PosFound = 0;

	while (BackRead < MaxBack)
	{
		lcuint64 ReadPos, ReadSize;

		if (BackRead + CommentBufferSize > MaxBack)
			BackRead = MaxBack;
		else
			BackRead += CommentBufferSize;
		ReadPos = SizeFile - BackRead;

		ReadSize = ((CommentBufferSize + 4) < (SizeFile - ReadPos)) ? (CommentBufferSize + 4) : (SizeFile - ReadPos);
		mFile->Seek((long)ReadPos, SEEK_SET);

		if (mFile->ReadBuffer(buf, (long)ReadSize) != ReadSize)
			break;

		for (int i = (int)ReadSize - 3; (i--) > 0;)
		{
			if (((*(buf+i)) == 0x50) && ((*(buf+i+1)) == 0x4b) && ((*(buf+i+2)) == 0x05) && ((*(buf+i+3)) == 0x06))
			{
				PosFound = ReadPos + i;
				break;
			}
		}

		if (PosFound != 0)
			break;
	}

	return PosFound;
}

lcuint64 lcZipFile::SearchCentralDir64()
{
	lcuint64 SizeFile, MaxBack, BackRead, PosFound;
	const int CommentBufferSize = 1024;
	lcuint8 buf[CommentBufferSize + 4];

	SizeFile = mFile->GetLength();
	MaxBack = lcMin(SizeFile, 0xffffU);
	BackRead = 4;
	PosFound = 0;

	while (BackRead < MaxBack)
	{
		lcuint64 ReadPos, ReadSize;

		if (BackRead + CommentBufferSize > MaxBack)
			BackRead = MaxBack;
		else
			BackRead += CommentBufferSize;
		ReadPos = SizeFile - BackRead;

		ReadSize = ((CommentBufferSize + 4) < (SizeFile - ReadPos)) ? (CommentBufferSize + 4) : (SizeFile - ReadPos);
		mFile->Seek((long)ReadPos, SEEK_SET);

		if (mFile->ReadBuffer(buf, (long)ReadSize) != ReadSize)
			break;

		for (int i=(int)ReadSize - 3; (i--) > 0; )
		{
			if (((*(buf+i)) == 0x50) && ((*(buf+i+1)) == 0x4b) && ((*(buf+i+2)) == 0x06) && ((*(buf+i+3)) == 0x07))
			{
				PosFound = ReadPos + i;
				break;
			}
		}

		if (PosFound != 0)
			break;
	}

	if (PosFound == 0)
		return 0;

	mFile->Seek((long)PosFound, SEEK_SET);

	lcuint32 Number;
	lcuint64 RelativeOffset;

	// Signature.
	if (mFile->ReadU32(&Number, 1) != 1)
		return 0;

	// Number of the disk with the start of the zip64 end of central directory.
	if (mFile->ReadU32(&Number, 1) != 1)
		return 0;

	if (Number != 0)
		return 0;

	// Relative offset of the zip64 end of central directory record.
	if (mFile->ReadU64(&RelativeOffset, 1) != 1)
		return 0;

	// Total number of disks.
	if (mFile->ReadU32(&Number, 1) != 1)
		return 0;

	if (Number != 0)
		return 0;

	// Go to end of central directory record.
	mFile->Seek((long)RelativeOffset, SEEK_SET);

	// The signature.
	if (mFile->ReadU32(&Number, 1) != 1)
		return 0;

	if (Number != 0x06064b50)
		return 0;

	return RelativeOffset;
}

bool lcZipFile::CheckFileCoherencyHeader(int FileIndex, lcuint32* SizeVar, lcuint64* OffsetLocalExtraField, lcuint32* SizeLocalExtraField)
{
	lcuint16 Number16, Flags;
	lcuint32 Number32, Magic;
	lcuint16 SizeFilename, SizeExtraField;
	const lcZipFileInfo& FileInfo = mFiles[FileIndex];

	*SizeVar = 0;
	*OffsetLocalExtraField = 0;
	*SizeLocalExtraField = 0;

	mFile->Seek((long)(FileInfo.offset_curfile + mBytesBeforeZipFile), SEEK_SET);

	if (mFile->ReadU32(&Magic, 1) != 1 || Magic != 0x04034b50)
		return false;

	if (mFile->ReadU16(&Number16, 1) != 1)
		return false;

	if (mFile->ReadU16(&Flags, 1) != 1)
		return false;

	if (mFile->ReadU16(&Number16, 1) != 1 || Number16 != FileInfo.compression_method)
		return false;

	if (FileInfo.compression_method != 0 && FileInfo.compression_method != Z_DEFLATED)
		return false;

	if (mFile->ReadU32(&Number32, 1) != 1)
		return false;

	if (mFile->ReadU32(&Number32, 1) != 1 || ((Number32 != FileInfo.crc) && ((Flags & 8)==0)))
		return false;

	if (mFile->ReadU32(&Number32, 1) != 1 || (Number32 != 0xFFFFFFFF && (Number32 != FileInfo.compressed_size) && ((Flags & 8)==0)))
		return false;

	if (mFile->ReadU32(&Number32, 1) != 1 || (Number32 != 0xFFFFFFFF && (Number32 != FileInfo.uncompressed_size) && ((Flags & 8)==0)))
		return false;

	if (mFile->ReadU16(&SizeFilename, 1) != 1 || SizeFilename != FileInfo.size_filename)
		return false;

	*SizeVar += SizeFilename;

	if (mFile->ReadU16(&SizeExtraField, 1) != 1)
		return false;

	*OffsetLocalExtraField= FileInfo.offset_curfile + 0x1e + SizeFilename;
	*SizeLocalExtraField = SizeExtraField;

	*SizeVar += SizeExtraField;

	return true;
}

bool lcZipFile::Open()
{
	lcuint64 NumberEntriesCD, CentralPos;

	CentralPos = SearchCentralDir64();

	if (CentralPos)
	{
		lcuint32 NumberDisk, NumberDiskWithCD;

		mZip64 = true;

		// Skip signature, size and versions.
		mFile->Seek((long)CentralPos + 4 + 8 + 2 + 2, SEEK_SET);

		// Number of this disk.
		if (mFile->ReadU32(&NumberDisk, 1) != 1)
			return false;

		// Number of the disk with the start of the central directory.
		if (mFile->ReadU32(&NumberDiskWithCD, 1) != 1)
			return false;

		// Total number of entries in the central directory on this disk.
		if (mFile->ReadU64(&mNumEntries, 1) != 1)
			return false;

		// Total number of entries in the central directory.
		if (mFile->ReadU64(&NumberEntriesCD, 1) != 1)
			return false;

		if ((NumberEntriesCD != mNumEntries) || (NumberDiskWithCD != 0) || (NumberDisk != 0))
			return false;

		// Size of the central directory.
		if (mFile->ReadU64(&mCentralDirSize, 1) != 1)
			return false;

		// Offset of start of central directory with respect to the starting disk number.
		if (mFile->ReadU64(&mCentralDirOffset, 1) != 1)
			return false;

//		us.gi.size_comment = 0;
	}
	else
	{
		lcuint16 NumberDisk, NumberDiskWithCD;
		lcuint16 Number16;
		lcuint32 Number32;

		CentralPos = SearchCentralDir();
		if (CentralPos == 0)
			return false;

		mZip64 = false;

		// Skip signature.
		mFile->Seek((long)CentralPos + 4, SEEK_SET);

		// Number of this disk.
		if (mFile->ReadU16(&NumberDisk, 1) != 1)
			return false;

		// Number of the disk with the start of the central directory.
		if (mFile->ReadU16(&NumberDiskWithCD, 1) != 1)
			return false;

		// Total number of entries in the central dir on this disk.
		if (mFile->ReadU16(&Number16, 1) != 1)
			return false;
		mNumEntries = Number16;

		// Total number of entries in the central dir.
		if (mFile->ReadU16(&Number16, 1) != 1)
			return false;
		NumberEntriesCD = Number16;

		if ((NumberEntriesCD != mNumEntries) || (NumberDiskWithCD != 0) || (NumberDisk != 0))
			return false;

		// Size of the central directory.
		if (mFile->ReadU32(&Number32, 1) != 1)
			return false;
		mCentralDirSize = Number32;

		// Offset of start of central directory with respect to the starting disk number.
		if (mFile->ReadU32(&Number32, 1) != 1)
			return false;
		mCentralDirOffset= Number32;

		// zipfile comment length.
//		if (mFile->ReadU16(&us.z_filefunc, us.filestream,&us.gi.size_comment)!=1)
//			return false;
	}

	if (CentralPos < mCentralDirOffset + mCentralDirSize)
		return false;

	mBytesBeforeZipFile = CentralPos - (mCentralDirOffset + mCentralDirSize);
	mCentralPos = CentralPos;

	return ReadCentralDir();
}

bool lcZipFile::ReadCentralDir()
{
	lcuint64 PosInCentralDir = mCentralDirOffset;

	mFile->Seek((long)(PosInCentralDir + mBytesBeforeZipFile), SEEK_SET);
	mFiles.Expand((int)mNumEntries);

	for (lcuint64 FileNum = 0; FileNum < mNumEntries; FileNum++)
	{
		lcuint32 Magic, Number32;
		lcZipFileInfo& FileInfo = mFiles.Add();
		long Seek = 0;

		FileInfo.write_buffer = NULL;
		FileInfo.deleted = false;

		if (mFile->ReadU32(&Magic, 1) != 1 || Magic != 0x02014b50)
			return false;

		if (mFile->ReadU16(&FileInfo.version, 1) != 1)
			return false;

		if (mFile->ReadU16(&FileInfo.version_needed, 1) != 1)
			return false;

		if (mFile->ReadU16(&FileInfo.flag, 1) != 1)
			return false;

		if (mFile->ReadU16(&FileInfo.compression_method, 1) != 1)
			return false;

		if (mFile->ReadU32(&FileInfo.dosDate, 1) != 1)
			return false;

		lcuint32 Date = FileInfo.dosDate >> 16;
		FileInfo.tmu_date.tm_mday = (lcuint32)(Date & 0x1f);
		FileInfo.tmu_date.tm_mon = (lcuint32)((((Date) & 0x1E0) / 0x20) - 1);
		FileInfo.tmu_date.tm_year = (lcuint32)(((Date & 0x0FE00) / 0x0200) + 1980);

		FileInfo.tmu_date.tm_hour = (lcuint32)((FileInfo.dosDate & 0xF800) / 0x800);
		FileInfo.tmu_date.tm_min = (lcuint32)((FileInfo.dosDate & 0x7E0) / 0x20);
		FileInfo.tmu_date.tm_sec = (lcuint32)(2*(FileInfo.dosDate & 0x1f));

		if (mFile->ReadU32(&FileInfo.crc, 1) != 1)
			return false;

		if (mFile->ReadU32(&Number32, 1) != 1)
			return false;
		FileInfo.compressed_size = Number32;

		if (mFile->ReadU32(&Number32, 1) != 1)
			return false;
		FileInfo.uncompressed_size = Number32;

		if (mFile->ReadU16(&FileInfo.size_filename, 1) != 1)
			return false;

		if (mFile->ReadU16(&FileInfo.size_file_extra, 1) != 1)
			return false;

		if (mFile->ReadU16(&FileInfo.size_file_comment, 1) != 1)
			return false;

		if (mFile->ReadU16(&FileInfo.disk_num_start, 1) != 1)
			return false;

		if (mFile->ReadU16(&FileInfo.internal_fa, 1) != 1)
			return false;

		if (mFile->ReadU32(&FileInfo.external_fa, 1) != 1)
			return false;

		// relative offset of local header
		if (mFile->ReadU32(&Number32, 1) != 1)
			return false;
		FileInfo.offset_curfile = Number32;

		Seek += FileInfo.size_filename;

		lcuint32 SizeRead;
		if (FileInfo.size_filename < sizeof(FileInfo.file_name) - 1)
		{
			*(FileInfo.file_name + FileInfo.size_filename) = '\0';
			SizeRead = FileInfo.size_filename;
		}
		else
		{
			*(FileInfo.file_name + sizeof(FileInfo.file_name) - 1) = '\0';
			SizeRead = sizeof(FileInfo.file_name) - 1;
		}

		if (FileInfo.size_filename > 0)
			if (mFile->ReadBuffer(FileInfo.file_name, SizeRead) != SizeRead)
				return false;
		Seek -= SizeRead;
/*
		// Read extrafield
		if ((err==UNZ_OK) && (extraField!=NULL))
		{
			ZPOS64_T uSizeRead ;
			if (file_info.size_file_extra<extraFieldBufferSize)
				uSizeRead = file_info.size_file_extra;
			else
				uSizeRead = extraFieldBufferSize;

			if (lSeek!=0)
			{
				if (ZSEEK64(s->z_filefunc, s->filestream,lSeek,ZLIB_FILEFUNC_SEEK_CUR)==0)
					lSeek=0;
				else
					err=UNZ_ERRNO;
			}

			if ((file_info.size_file_extra>0) && (extraFieldBufferSize>0))
				if (ZREAD64(s->z_filefunc, s->filestream,extraField,(uLong)uSizeRead)!=uSizeRead)
					err=UNZ_ERRNO;

			lSeek += file_info.size_file_extra - (uLong)uSizeRead;
		}
		else
			lSeek += file_info.size_file_extra;
*/
		Seek += FileInfo.size_file_extra;

		if (FileInfo.size_file_extra != 0)
		{
			lcuint32 acc = 0;

			// since lSeek now points to after the extra field we need to move back
			Seek -= FileInfo.size_file_extra;

			if (Seek != 0)
			{
				mFile->Seek(Seek, SEEK_CUR);
				Seek = 0;
			}

			while (acc < FileInfo.size_file_extra)
			{
				lcuint16 HeaderId, DataSize;

				if (mFile->ReadU16(&HeaderId, 1) != 1)
					return false;

				if (mFile->ReadU16(&DataSize, 1) != 1)
					return false;

				// ZIP64 extra fields.
				if (HeaderId == 0x0001)
				{
					if (FileInfo.uncompressed_size == (lcuint64)(unsigned long)-1)
					{
						if (mFile->ReadU64(&FileInfo.uncompressed_size, 1) != 1)
							return false;
					}

					if (FileInfo.compressed_size == (lcuint64)(unsigned long)-1)
					{
						if (mFile->ReadU64(&FileInfo.compressed_size, 1) != 1)
							return false;
					}

					if (FileInfo.offset_curfile == (lcuint64)-1)
					{
						// Relative Header offset.
						if (mFile->ReadU64(&FileInfo.offset_curfile, 1) != 1)
							return false;
					}

					if (FileInfo.disk_num_start == (lcuint16)-1)
					{
						// Disk Start Number.
						if (mFile->ReadU32(&Number32, 1) != 1)
							return false;
					}
				}
				else
				{
					mFile->Seek(DataSize, SEEK_CUR);
				}

				acc += 2 + 2 + DataSize;
			}
		}
/*
		if ((err==UNZ_OK) && (szComment!=NULL))
		{
			uLong uSizeRead ;
			if (file_info.size_file_comment<commentBufferSize)
			{
				*(szComment+file_info.size_file_comment)='\0';
				uSizeRead = file_info.size_file_comment;
			}
			else
				uSizeRead = commentBufferSize;

			if (lSeek!=0)
			{
				if (ZSEEK64(s->z_filefunc, s->filestream,lSeek,ZLIB_FILEFUNC_SEEK_CUR)==0)
					lSeek=0;
				else
					err=UNZ_ERRNO;
			}

			if ((file_info.size_file_comment>0) && (commentBufferSize>0))
				if (ZREAD64(s->z_filefunc, s->filestream,szComment,uSizeRead)!=uSizeRead)
					err=UNZ_ERRNO;
			lSeek+=file_info.size_file_comment - uSizeRead;
		}
		else
			lSeek+=file_info.size_file_comment;
*/
		Seek += FileInfo.size_file_comment;

		mFile->Seek(Seek, SEEK_CUR);
	}

	return true;
}

bool lcZipFile::ExtractFile(const char* FileName, lcMemFile& File, lcuint32 MaxLength)
{
	for (int FileIdx = 0; FileIdx < mFiles.GetSize(); FileIdx++)
	{
		lcZipFileInfo& FileInfo = mFiles[FileIdx];

		if (!stricmp(FileInfo.file_name, FileName))
			return ExtractFile(FileIdx, File, MaxLength);
	}

	return false;
}

bool lcZipFile::ExtractFile(int FileIndex, lcMemFile& File, lcuint32 MaxLength)
{
	lcuint32 SizeVar;
	lcuint64 OffsetLocalExtraField;
	lcuint32 SizeLocalExtraField;
	const lcZipFileInfo& FileInfo = mFiles[FileIndex];

	if (!CheckFileCoherencyHeader(FileIndex, &SizeVar, &OffsetLocalExtraField, &SizeLocalExtraField))
		return false;

	const int BufferSize = 16384;
	char ReadBuffer[BufferSize];
	z_stream Stream;
	lcuint32 Crc32;
	lcuint64 PosInZipfile;
	lcuint64 RestReadCompressed;
	lcuint64 RestReadUncompressed;

	Crc32 = 0;
	Stream.total_out = 0;

	if (FileInfo.compression_method == Z_DEFLATED)
	{
		Stream.zalloc = (alloc_func)0;
		Stream.zfree = (free_func)0;
		Stream.opaque = (voidpf)0;
		Stream.next_in = 0;
		Stream.avail_in = 0;

		int err = inflateInit2(&Stream, -MAX_WBITS);
		if (err != Z_OK)
			return false;
	}

	RestReadCompressed = FileInfo.compressed_size;
	RestReadUncompressed = FileInfo.uncompressed_size;
	PosInZipfile = FileInfo.offset_curfile + 0x1e + SizeVar;

	Stream.avail_in = (uInt)0;

	lcuint32 Length = lcMin((lcuint32)FileInfo.uncompressed_size, MaxLength);
	File.SetLength(Length);
	File.Seek(0, SEEK_SET);

	Stream.next_out = (Bytef*)File.mBuffer;
	Stream.avail_out = Length;

	lcuint32 Read = 0;

	while (Stream.avail_out > 0)
	{
		if ((Stream.avail_in == 0) && (RestReadCompressed > 0))
		{
			lcuint32 ReadThis = BufferSize;

			if (RestReadCompressed < ReadThis)
				ReadThis = (lcuint32)RestReadCompressed;

			if (ReadThis == 0)
				return false;

			mFile->Seek((long)(PosInZipfile + mBytesBeforeZipFile), SEEK_SET);
			if (mFile->ReadBuffer(ReadBuffer, ReadThis) != ReadThis)
				return false;

			PosInZipfile += ReadThis;

			RestReadCompressed -= ReadThis;

			Stream.next_in = (Bytef*)ReadBuffer;
			Stream.avail_in = (uInt)ReadThis;
		}

		if (FileInfo.compression_method == 0)
		{
			lcuint32 DoCopy, i;

			if ((Stream.avail_in == 0) && (RestReadCompressed == 0))
				return (Read == 0) ? false : true;

			if (Stream.avail_out < Stream.avail_in)
				DoCopy = Stream.avail_out;
			else
				DoCopy = Stream.avail_in;

			for (i = 0; i < DoCopy; i++)
				*(Stream.next_out+i) = *(Stream.next_in+i);

			Crc32 = crc32(Crc32, Stream.next_out, DoCopy);
			RestReadUncompressed -= DoCopy;
			Stream.avail_in -= DoCopy;
			Stream.avail_out -= DoCopy;
			Stream.next_out += DoCopy;
			Stream.next_in += DoCopy;
			Stream.total_out += DoCopy;
			Read += DoCopy;
		}
		else
		{
			lcuint64 TotalOutBefore, TotalOutAfter;
			const Bytef *bufBefore;
			lcuint64 OutThis;
			int flush = Z_SYNC_FLUSH;

			TotalOutBefore = Stream.total_out;
			bufBefore = Stream.next_out;

			int err = inflate(&Stream,flush);

			if ((err >= 0) && (Stream.msg != NULL))
				err = Z_DATA_ERROR;

			TotalOutAfter = Stream.total_out;
			OutThis = TotalOutAfter - TotalOutBefore;

			Crc32 = crc32(Crc32, bufBefore, (uInt)(OutThis));

			RestReadUncompressed -= OutThis;

			Read += (uInt)(TotalOutAfter - TotalOutBefore);

			if (err != Z_OK)
			{
				inflateEnd(&Stream);

				if (RestReadUncompressed == 0)
				{
					if (Crc32 != FileInfo.crc)
						return false;
				}

				if (err == Z_STREAM_END)
					return (Read == 0) ? false : true;

				return false;
			}
		}
	}

	inflateEnd(&Stream);

	return true;
}

bool lcZipFile::AddFile(const char* FileName, lcMemFile& File)
{
	const int BufferSize = 16384;
	char WriteBuffer[BufferSize];
	z_stream Stream;
	lcuint32 Crc32 = 0;

	File.Seek(0, SEEK_SET);

	Stream.zalloc = (alloc_func)0;
	Stream.zfree = (free_func)0;
	Stream.opaque = (voidpf)0;

	if (deflateInit2(&Stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
		return false;

	lcMemFile* OutFile = new lcMemFile();
	lcMemFile& CompressedFile = *OutFile;

	Bytef* BufferIn = File.mBuffer;
	int FlushMode;

	do
	{
		uInt Read = lcMin(File.GetLength() - (BufferIn - File.mBuffer), BufferSize);
		Stream.avail_in = Read;
		Stream.next_in = BufferIn;
		Crc32 = crc32(Crc32, BufferIn, Read);
		BufferIn += Read;

		FlushMode = (BufferIn >= File.mBuffer + File.GetLength()) ? Z_FINISH : Z_NO_FLUSH;

		do
		{
			Stream.avail_out = BufferSize;
			Stream.next_out = (Bytef*)WriteBuffer;
			deflate(&Stream, FlushMode);
			CompressedFile.WriteBuffer(WriteBuffer, BufferSize - Stream.avail_out);
		} while (Stream.avail_out == 0);
	} while (FlushMode != Z_FINISH);

    deflateEnd(&Stream);

	lcZipFileInfo& Info = mFiles.Add();

	Info.version = 0;
    if (CompressedFile.GetLength() >= 0xffffffff || File.GetLength() >= 0xffffffff)
		Info.version_needed = 45;
	else
		Info.version_needed = 20;
	Info.flag = 0;
	Info.compression_method = Z_DEFLATED;
	Info.crc = Crc32;
	Info.compressed_size = CompressedFile.GetLength();
	Info.uncompressed_size = File.GetLength();
	Info.size_filename = strlen(FileName);
	Info.size_file_extra = 0;
	Info.size_file_comment = 0;
	Info.disk_num_start = 0;
	Info.internal_fa = 0;
	Info.external_fa = 0;
	Info.offset_curfile = 0;
	strncpy(Info.file_name, FileName, sizeof(Info.file_name));

	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	Info.tmu_date.tm_sec  = timeinfo->tm_sec;
	Info.tmu_date.tm_min  = timeinfo->tm_min;
	Info.tmu_date.tm_hour = timeinfo->tm_hour;
	Info.tmu_date.tm_mday = timeinfo->tm_mday;
	Info.tmu_date.tm_mon  = timeinfo->tm_mon ;
	Info.tmu_date.tm_year = timeinfo->tm_year - 80;
	Info.dosDate = (((timeinfo->tm_mday) + (32 * (timeinfo->tm_mon + 1)) + (512 * timeinfo->tm_year)) << 16) | ((timeinfo->tm_sec / 2) + (32 * timeinfo->tm_min) + (2048 * (uLong)timeinfo->tm_hour));

	Info.write_buffer = OutFile;
	Info.deleted = false;

	mModified = true;

	return true;
}

bool lcZipFile::DeleteFile(const char* FileName)
{
	for (int FileIdx = 0; FileIdx < mFiles.GetSize(); FileIdx++)
	{
		lcZipFileInfo& FileInfo = mFiles[FileIdx];

		if (!stricmp(FileInfo.file_name, FileName))
		{
			FileInfo.deleted = true;
			mModified = true;
			return true;
		}
	}

	return false;
}

void lcZipFile::Flush()
{
	if (!mFile || !mModified)
		return;

	lcMemFile MemFile;
	lcuint64 NumFiles = 0;

	mFile->Seek(0, SEEK_SET);

	if (mBytesBeforeZipFile)
	{
		MemFile.GrowFile((size_t)mBytesBeforeZipFile);
		mFile->ReadBuffer(MemFile.mBuffer, (long)mBytesBeforeZipFile);
		MemFile.Seek((long)mBytesBeforeZipFile, SEEK_CUR);
	}

	for (int FileIdx = 0; FileIdx < mFiles.GetSize(); FileIdx++)
	{
		lcZipFileInfo& FileInfo = mFiles[FileIdx];

		if (FileInfo.deleted)
			continue;

		lcuint32 SizeVar;
		lcuint64 OffsetLocalExtraField;
		lcuint32 SizeLocalExtraField;
		CheckFileCoherencyHeader(FileIdx, &SizeVar, &OffsetLocalExtraField, &SizeLocalExtraField);
		NumFiles++;

		lcuint64 PosInZipfile = FileInfo.offset_curfile + 0x1e + SizeVar;
		mFile->Seek((long)(PosInZipfile + mBytesBeforeZipFile), SEEK_SET);

		FileInfo.offset_curfile = MemFile.GetPosition();

		if (FileInfo.offset_curfile >= 0xffffffff)
			FileInfo.version_needed = 45;

		MemFile.WriteU32(0x04034b50);
		MemFile.WriteU16(FileInfo.version_needed);
		MemFile.WriteU16(FileInfo.flag);
		MemFile.WriteU16(FileInfo.compression_method);
		MemFile.WriteU32(FileInfo.dosDate);
		MemFile.WriteU32(FileInfo.crc);
		if (FileInfo.compressed_size >= 0xffffffff)
			MemFile.WriteU32(0xffffffff);
		else
			MemFile.WriteU32((lcuint32)FileInfo.compressed_size);
		if (FileInfo.uncompressed_size >= 0xffffffff)
			MemFile.WriteU32(0xffffffff);
		else
			MemFile.WriteU32((lcuint32)FileInfo.uncompressed_size);
		MemFile.WriteU16(FileInfo.size_filename);
		MemFile.WriteU16(FileInfo.size_file_extra);
		MemFile.WriteBuffer(FileInfo.file_name, FileInfo.size_filename);

		if (FileInfo.write_buffer)
		{
			MemFile.WriteBuffer(FileInfo.write_buffer->mBuffer, FileInfo.write_buffer->GetLength());
			delete FileInfo.write_buffer;
			FileInfo.write_buffer = NULL;
		}
		else
		{
			MemFile.GrowFile((size_t)(MemFile.GetPosition() + FileInfo.compressed_size));
			mFile->ReadBuffer(MemFile.mBuffer + MemFile.GetPosition(), (long)FileInfo.compressed_size);
			MemFile.Seek((long)FileInfo.compressed_size, SEEK_CUR);
		}
	}

	lcuint64 CentralDirPos = MemFile.GetPosition();

	for (int FileIdx = 0; FileIdx < mFiles.GetSize(); FileIdx++)
	{
		lcZipFileInfo& FileInfo = mFiles[FileIdx];

		if (FileInfo.deleted)
			continue;

		MemFile.WriteU32(0x02014b50);
		MemFile.WriteU16(FileInfo.version);
		MemFile.WriteU16(FileInfo.version_needed);
		MemFile.WriteU16(FileInfo.flag);
		MemFile.WriteU16(FileInfo.compression_method);
		MemFile.WriteU32(FileInfo.dosDate);
		MemFile.WriteU32(FileInfo.crc);
		if (FileInfo.compressed_size >= 0xffffffff)
			MemFile.WriteU32(0xffffffff);
		else
			MemFile.WriteU32((lcuint32)FileInfo.compressed_size);
		if (FileInfo.uncompressed_size >= 0xffffffff)
			MemFile.WriteU32(0xffffffff);
		else
			MemFile.WriteU32((lcuint32)FileInfo.uncompressed_size);
		MemFile.WriteU16(FileInfo.size_filename);
		MemFile.WriteU16(FileInfo.size_file_extra);
		MemFile.WriteU16(FileInfo.size_file_comment);
		MemFile.WriteU16(FileInfo.disk_num_start);
		MemFile.WriteU16(FileInfo.internal_fa);
		MemFile.WriteU32(FileInfo.external_fa);

		if (FileInfo.offset_curfile >= 0xffffffff)
			MemFile.WriteU32(0xffffffff);
		else
			MemFile.WriteU32((lcuint32)(FileInfo.offset_curfile - mBytesBeforeZipFile));

		MemFile.WriteBuffer(FileInfo.file_name, FileInfo.size_filename);
	}

	lcuint64 CentralDirEnd = MemFile.GetPosition();

	if (CentralDirPos - mBytesBeforeZipFile >= 0xffffffff)
	{
		MemFile.WriteU32(0x6064b50);
		MemFile.WriteU64(44);
		MemFile.WriteU16(45);
		MemFile.WriteU16(45);
		MemFile.WriteU32(0);
		MemFile.WriteU32(0);
		MemFile.WriteU64(NumFiles);
		MemFile.WriteU64(NumFiles);
		MemFile.WriteU64(CentralDirEnd - CentralDirPos);
		MemFile.WriteU64(CentralDirPos - mBytesBeforeZipFile);

		MemFile.WriteU32(0x7064b50);
		MemFile.WriteU32(0);
		MemFile.WriteU64(CentralDirEnd - mBytesBeforeZipFile);
		MemFile.WriteU32(1);
	}

	MemFile.WriteU32(0x06054b50);
	MemFile.WriteU16(0);
	MemFile.WriteU16(0);

	if (NumFiles >= 0xffff)
		MemFile.WriteU16(0xffff);
	else
		MemFile.WriteU16((lcuint16)NumFiles);

	if (NumFiles >= 0xffff)
		MemFile.WriteU16(0xffff);
	else
		MemFile.WriteU16((lcuint16)NumFiles);

	MemFile.WriteU32((lcuint32)(CentralDirEnd - CentralDirPos));
	if (CentralDirPos - mBytesBeforeZipFile >= 0xffffffff)
		MemFile.WriteU32(0xffffffff);
	else
		MemFile.WriteU32((lcuint32)(CentralDirPos - mBytesBeforeZipFile));

	MemFile.WriteU16(0);

	mFile->CopyFrom(MemFile);
/*
	lcuint64 CurrentOffset = mCentralDirOffset + mBytesBeforeZipFile;
	mFile->Seek((long)CurrentOffset, SEEK_SET);

	void* CentralDir = malloc((long)mCentralDirSize);
	mFile->ReadBuffer(CentralDir, (long)mCentralDirSize);
	mFile->Seek((long)CurrentOffset, SEEK_SET);

	for (int FileIdx = FirstFileAdded; FileIdx < mFiles.GetSize(); FileIdx++)
	{
		lcZipFileInfo& FileInfo = mFiles[FileIdx];

		FileInfo.offset_curfile = CurrentOffset;

		if (FileInfo.offset_curfile >= 0xffffffff)
			FileInfo.version_needed = 45;

		mFile->WriteU32(0x04034b50);
		mFile->WriteU16(FileInfo.version_needed);
		mFile->WriteU16(FileInfo.flag);
		mFile->WriteU16(FileInfo.compression_method);
		mFile->WriteU32(FileInfo.dosDate);
		mFile->WriteU32(FileInfo.crc);
		if (FileInfo.compressed_size >= 0xffffffff)
			mFile->WriteU32(0xffffffff);
		else
			mFile->WriteU32((lcuint32)FileInfo.compressed_size);
		if (FileInfo.uncompressed_size >= 0xffffffff)
			mFile->WriteU32(0xffffffff);
		else
			mFile->WriteU32((lcuint32)FileInfo.uncompressed_size);
		mFile->WriteU16(FileInfo.size_filename);
		mFile->WriteU16(FileInfo.size_file_extra);
		mFile->WriteBuffer(FileInfo.file_name, FileInfo.size_filename);

		mFile->WriteBuffer(FileInfo.write_buffer->mBuffer, FileInfo.write_buffer->GetLength());
		delete FileInfo.write_buffer;
		FileInfo.write_buffer = NULL;

		CurrentOffset = mFile->GetPosition();
	}

	lcuint64 CentralDirPos = mFile->GetPosition();
	mFile->WriteBuffer(CentralDir, (long)mCentralDirSize);
	free(CentralDir);

	for (int FileIdx = FirstFileAdded; FileIdx < mFiles.GetSize(); FileIdx++)
	{
		lcZipFileInfo& FileInfo = mFiles[FileIdx];

		mFile->WriteU32(0x02014b50);
		mFile->WriteU16(FileInfo.version);
		mFile->WriteU16(FileInfo.version_needed);
		mFile->WriteU16(FileInfo.flag);
		mFile->WriteU16(FileInfo.compression_method);
		mFile->WriteU32(FileInfo.dosDate);
		mFile->WriteU32(FileInfo.crc);
		if (FileInfo.compressed_size >= 0xffffffff)
			mFile->WriteU32(0xffffffff);
		else
			mFile->WriteU32((lcuint32)FileInfo.compressed_size);
		if (FileInfo.uncompressed_size >= 0xffffffff)
			mFile->WriteU32(0xffffffff);
		else
			mFile->WriteU32((lcuint32)FileInfo.uncompressed_size);
		mFile->WriteU16(FileInfo.size_filename);
		mFile->WriteU16(FileInfo.size_file_extra);
		mFile->WriteU16(FileInfo.size_file_comment);
		mFile->WriteU16(FileInfo.disk_num_start);
		mFile->WriteU16(FileInfo.internal_fa);
		mFile->WriteU32(FileInfo.external_fa);

		if (FileInfo.offset_curfile >= 0xffffffff)
			mFile->WriteU32(0xffffffff);
		else
			mFile->WriteU32((lcuint32)(FileInfo.offset_curfile - mBytesBeforeZipFile));

		mFile->WriteBuffer(FileInfo.file_name, FileInfo.size_filename);
	}

	lcuint64 CentralDirEnd = mFile->GetPosition();

	if (CentralDirPos - mBytesBeforeZipFile >= 0xffffffff)
	{
		mFile->WriteU32(0x6064b50);
		mFile->WriteU64(44);
		mFile->WriteU16(45);
		mFile->WriteU16(45);
		mFile->WriteU32(0);
		mFile->WriteU32(0);
		mFile->WriteU64(mFiles.GetSize());
		mFile->WriteU64(mFiles.GetSize());
		mFile->WriteU64(CentralDirEnd - CentralDirPos);
		mFile->WriteU64(CentralDirPos - mBytesBeforeZipFile);

		mFile->WriteU32(0x7064b50);
		mFile->WriteU32(0);
		mFile->WriteU64(CentralDirEnd - mBytesBeforeZipFile);
		mFile->WriteU32(1);
	}

	mFile->WriteU32(0x06054b50);
	mFile->WriteU16(0);
	mFile->WriteU16(0);

	if (mFiles.GetSize() >= 0xffff)
		mFile->WriteU16(0xffff);
	else
		mFile->WriteU16(mFiles.GetSize());

	if (mFiles.GetSize() >= 0xffff)
		mFile->WriteU16(0xffff);
	else
		mFile->WriteU16(mFiles.GetSize());

	mFile->WriteU32((lcuint32)(CentralDirEnd - CentralDirPos));
	if (CentralDirPos - mBytesBeforeZipFile >= 0xffffffff)
		mFile->WriteU32(0xffffffff);
	else
		mFile->WriteU32((lcuint32)(CentralDirPos - mBytesBeforeZipFile));

	mFile->WriteU16(0);
	*/
}

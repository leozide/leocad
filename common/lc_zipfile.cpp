#include "lc_global.h"
#include "lc_zipfile.h"
#include "lc_file.h"
#include "lc_math.h"
#include <zlib.h>

lcZipFile::lcZipFile()
{
	mFile = NULL;
}

lcZipFile::~lcZipFile()
{
	delete mFile;
}

bool lcZipFile::Open(const char* FilePath)
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

lcuint64 lcZipFile::SearchCentralDir()
{
	lcuint64 SizeFile, MaxBack, BackRead, PosFound;
	const int CommentBufferSize = 1024;
	lcuint8 buf[CommentBufferSize + 4];

	SizeFile = mFile->GetLength();
	MaxBack = lcMin(SizeFile, 0xffff);
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
	MaxBack = lcMin(SizeFile, 0xffff);
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
	mFiles.SetSize((int)mNumEntries);

	for (lcuint64 FileNum = 0; FileNum < mNumEntries; FileNum++)
	{
		lcuint32 Magic, Number32;
		lcZipFileInfo& FileInfo = mFiles[(int)FileNum];
		long Seek = 0;

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
					lcuint32 Number32;

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

					if (FileInfo.offset_curfile == (lcuint64)(unsigned long)-1)
					{
						// Relative Header offset.
						if (mFile->ReadU64(&FileInfo.offset_curfile, 1) != 1)
							return false;
					}

					if (FileInfo.disk_num_start == (unsigned long)-1)
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

	return true;
}

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

//	us.pfile_in_zip_read = NULL;
//	us.encrypted = 0;

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

bool lcZipFile::ExtractFile(int FileIndex, lcMemFile& File)
{
	lcuint32 SizeVar;
	lcuint64 OffsetLocalExtraField;
	lcuint32 SizeLocalExtraField;
	const lcZipFileInfo& FileInfo = mFiles[FileIndex];
	const int BufferSize = 16384;

	if (!CheckFileCoherencyHeader(FileIndex, &SizeVar, &OffsetLocalExtraField, &SizeLocalExtraField))
		return false;

	// file_in_zip_read_info_s contain internal information about a file in zipfile, when reading and decompress it
	struct file_in_zip64_read_info
	{
		char  read_buffer[BufferSize];     // internal buffer for compressed data
		z_stream stream;                   // zLib stream structure for inflate

		lcuint64 pos_in_zipfile;           // position in byte on the zipfile, for fseek
		lcuint32 stream_initialised;       // flag set if stream structure is initialised

		lcuint64 offset_local_extrafield;  // offset of the local extra field
		lcuint32 size_local_extrafield;    // size of the local extra field
		lcuint64 pos_local_extrafield;     // position in the local extra field in read
		lcuint64 total_out_64;

		lcuint32 crc32;                    // crc32 of all data uncompressed
		lcuint32 crc32_wait;               // crc32 we must obtain after decompress all
		lcuint64 rest_read_compressed;     // number of byte to be decompressed
		lcuint64 rest_read_uncompressed;   //number of byte to be obtained after decomp
//		zlib_filefunc64_32_def z_filefunc;
//		voidpf filestream;                 // io structore of the zipfile
		lcuint32 compression_method;       // compression method (0==store)
		lcuint64 byte_before_the_zipfile;  // byte before the zipfile, (>0 for sfx)
		int   raw;
	};

	file_in_zip64_read_info ReadInfo;

	ReadInfo.offset_local_extrafield = OffsetLocalExtraField;
	ReadInfo.size_local_extrafield = SizeLocalExtraField;
	ReadInfo.pos_local_extrafield = 0;
	ReadInfo.raw = 0;

	ReadInfo.stream_initialised=0;

	ReadInfo.crc32_wait = FileInfo.crc;
	ReadInfo.crc32 = 0;
	ReadInfo.total_out_64 = 0;
	ReadInfo.compression_method = FileInfo.compression_method;
//	ReadInfo.filestream = s->filestream;
//	ReadInfo.z_filefunc = s->z_filefunc;
	ReadInfo.byte_before_the_zipfile = mBytesBeforeZipFile;

	ReadInfo.stream.total_out = 0;

	if (FileInfo.compression_method == Z_DEFLATED)
	{
		ReadInfo.stream.zalloc = (alloc_func)0;
		ReadInfo.stream.zfree = (free_func)0;
		ReadInfo.stream.opaque = (voidpf)0;
		ReadInfo.stream.next_in = 0;
		ReadInfo.stream.avail_in = 0;

		int err=inflateInit2(&ReadInfo.stream, -MAX_WBITS);
		if (err == Z_OK)
			ReadInfo.stream_initialised = Z_DEFLATED;
		else
			return false;
	}

	ReadInfo.rest_read_compressed = FileInfo.compressed_size;
	ReadInfo.rest_read_uncompressed = FileInfo.uncompressed_size;
	ReadInfo.pos_in_zipfile = FileInfo.offset_curfile + 0x1e + SizeVar;

	ReadInfo.stream.avail_in = (uInt)0;

//	s->pfile_in_zip_read = pfile_in_zip_read_info;
//	s->encrypted = 0;

	File.SetLength((long)FileInfo.uncompressed_size);
	File.Seek(0, SEEK_SET);

	ReadInfo.stream.next_out = (Bytef*)File.mBuffer;
	ReadInfo.stream.avail_out = File.mBufferSize;

//	if ((len>pfile_in_zip_read_info->rest_read_uncompressed) && (!(pfile_in_zip_read_info->raw)))
//		pfile_in_zip_read_info->stream.avail_out = (uInt)pfile_in_zip_read_info->rest_read_uncompressed;

//	if ((len>pfile_in_zip_read_info->rest_read_compressed+pfile_in_zip_read_info->stream.avail_in) && (pfile_in_zip_read_info->raw))
//		pfile_in_zip_read_info->stream.avail_out = (uInt)pfile_in_zip_read_info->rest_read_compressed+pfile_in_zip_read_info->stream.avail_in;

	lcuint32 Read = 0;

	while (ReadInfo.stream.avail_out > 0)
	{
		if ((ReadInfo.stream.avail_in == 0) && (ReadInfo.rest_read_compressed > 0))
		{
			lcuint32 ReadThis = BufferSize;

			if (ReadInfo.rest_read_compressed < ReadThis)
				ReadThis = (lcuint32)ReadInfo.rest_read_compressed;

			if (ReadThis == 0)
				return false;

			mFile->Seek((long)(ReadInfo.pos_in_zipfile + ReadInfo.byte_before_the_zipfile), SEEK_SET);
			if (mFile->ReadBuffer(ReadInfo.read_buffer, ReadThis) != ReadThis)
				return false;

			ReadInfo.pos_in_zipfile += ReadThis;

			ReadInfo.rest_read_compressed -= ReadThis;

			ReadInfo.stream.next_in = (Bytef*)ReadInfo.read_buffer;
			ReadInfo.stream.avail_in = (uInt)ReadThis;
		}

		if (ReadInfo.compression_method == 0)
		{
			lcuint32 DoCopy, i;

			if ((ReadInfo.stream.avail_in == 0) && (ReadInfo.rest_read_compressed == 0))
				return (Read == 0) ? false : true;

			if (ReadInfo.stream.avail_out <
				ReadInfo.stream.avail_in)
				DoCopy = ReadInfo.stream.avail_out;
			else
				DoCopy = ReadInfo.stream.avail_in;

			for (i = 0; i < DoCopy; i++)
				*(ReadInfo.stream.next_out+i) = *(ReadInfo.stream.next_in+i);

			ReadInfo.total_out_64 = ReadInfo.total_out_64 + DoCopy;

			ReadInfo.crc32 = crc32(ReadInfo.crc32, ReadInfo.stream.next_out, DoCopy);
			ReadInfo.rest_read_uncompressed -= DoCopy;
			ReadInfo.stream.avail_in -= DoCopy;
			ReadInfo.stream.avail_out -= DoCopy;
			ReadInfo.stream.next_out += DoCopy;
			ReadInfo.stream.next_in += DoCopy;
			ReadInfo.stream.total_out += DoCopy;
			Read += DoCopy;
		}
		else
		{
			lcuint64 TotalOutBefore, TotalOutAfter;
			const Bytef *bufBefore;
			lcuint64 OutThis;
			int flush = Z_SYNC_FLUSH;

			TotalOutBefore = ReadInfo.stream.total_out;
			bufBefore = ReadInfo.stream.next_out;

			int err = inflate(&ReadInfo.stream,flush);

			if ((err >= 0) && (ReadInfo.stream.msg != NULL))
				err = Z_DATA_ERROR;

			TotalOutAfter = ReadInfo.stream.total_out;
			OutThis = TotalOutAfter - TotalOutBefore;

			ReadInfo.total_out_64 = ReadInfo.total_out_64 + OutThis;

			ReadInfo.crc32 = crc32(ReadInfo.crc32,bufBefore, (uInt)(OutThis));

			ReadInfo.rest_read_uncompressed -= OutThis;

			Read += (uInt)(TotalOutAfter - TotalOutBefore);

			if (err != Z_OK)
			{
				inflateEnd(&ReadInfo.stream);

				if (ReadInfo.rest_read_uncompressed == 0)
				{
					if (ReadInfo.crc32 != ReadInfo.crc32_wait)
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

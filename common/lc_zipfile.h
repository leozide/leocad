#pragma once

#include "lc_array.h"

#ifdef DeleteFile
#undef DeleteFile
#endif

class lcFile;

// Date/time info.
struct tm_unz
{
	quint32 tm_sec;            // seconds after the minute - [0,59]
	quint32 tm_min;            // minutes after the hour - [0,59]
	quint32 tm_hour;           // hours since midnight - [0,23]
	quint32 tm_mday;           // day of the month - [1,31]
	quint32 tm_mon;            // months since January - [0,11]
	quint32 tm_year;           // years - [1980..2044]
};

// Information about a file in the zipfile.
struct lcZipFileInfo
{
	quint16 version;              // version made by                 2 bytes
	quint16 version_needed;       // version needed to extract       2 bytes
	quint16 flag;                 // general purpose bit flag        2 bytes
	quint16 compression_method;   // compression method              2 bytes
	quint32 dosDate;              // last mod file date in Dos fmt   4 bytes
	quint32 crc;                  // crc-32                          4 bytes
	quint64 compressed_size;      // compressed size                 8 bytes
	quint64 uncompressed_size;    // uncompressed size               8 bytes
	quint16 size_filename;        // filename length                 2 bytes
	quint16 size_file_extra;      // extra field length              2 bytes
	quint16 size_file_comment;    // file comment length             2 bytes

	quint16 disk_num_start;       // disk number start               2 bytes
	quint16 internal_fa;          // internal file attributes        2 bytes
	quint32 external_fa;          // external file attributes        4 bytes

	quint64 offset_curfile;       // relative offset of local header 8 bytes
	char file_name[256];
	tm_unz tmu_date;

	lcMemFile* write_buffer;
	bool deleted;
};

class lcZipFile
{
public:
	lcZipFile();
	~lcZipFile();

	lcZipFile(const lcZipFile&) = delete;
	lcZipFile(lcZipFile&&) = delete;
	lcZipFile& operator=(const lcZipFile&) = delete;
	lcZipFile& operator=(lcZipFile&&) = delete;

	bool OpenRead(const QString& FileName);
	bool OpenRead(lcFile* File);
	bool OpenWrite(const QString& FileName);

	bool ExtractFile(int FileIndex, lcMemFile& File, quint32 MaxLength = 0xffffffff);
	bool ExtractFile(const char* FileName, lcMemFile& File, quint32 MaxLength = 0xffffffff);

	lcArray<lcZipFileInfo> mFiles;

protected:
	bool Open();
	bool ReadCentralDir();
	quint64 SearchCentralDir();
	quint64 SearchCentralDir64();
	bool CheckFileCoherencyHeader(int FileIndex, quint32* SizeVar, quint64* OffsetLocalExtraField, quint32* SizeLocalExtraField);

	QMutex mMutex;
	lcFile* mFile;

	bool mModified;
	bool mZip64;
	quint64 mNumEntries;
	quint64 mCentralDirSize;
	quint64 mCentralDirOffset;
	quint64 mBytesBeforeZipFile;
	quint64 mCentralPos;
};


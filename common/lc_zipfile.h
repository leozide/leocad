#ifndef _LC_ZIPFILE_H_
#define _LC_ZIPFILE_H_

#include "array.h"

class lcFile;

// Date/time info.
struct tm_unz
{
	lcuint32 tm_sec;            // seconds after the minute - [0,59]
	lcuint32 tm_min;            // minutes after the hour - [0,59]
	lcuint32 tm_hour;           // hours since midnight - [0,23]
	lcuint32 tm_mday;           // day of the month - [1,31]
	lcuint32 tm_mon;            // months since January - [0,11]
	lcuint32 tm_year;           // years - [1980..2044]
};

// Information about a file in the zipfile.
struct lcZipFileInfo
{
	lcuint16 version;              // version made by                 2 bytes
	lcuint16 version_needed;       // version needed to extract       2 bytes
	lcuint16 flag;                 // general purpose bit flag        2 bytes
	lcuint16 compression_method;   // compression method              2 bytes
	lcuint32 dosDate;              // last mod file date in Dos fmt   4 bytes
	lcuint32 crc;                  // crc-32                          4 bytes
	lcuint64 compressed_size;      // compressed size                 8 bytes
	lcuint64 uncompressed_size;    // uncompressed size               8 bytes
	lcuint16 size_filename;        // filename length                 2 bytes
	lcuint16 size_file_extra;      // extra field length              2 bytes
	lcuint16 size_file_comment;    // file comment length             2 bytes

	lcuint16 disk_num_start;       // disk number start               2 bytes
	lcuint16 internal_fa;          // internal file attributes        2 bytes
	lcuint32 external_fa;          // external file attributes        4 bytes

	lcuint64 offset_curfile;       // relative offset of local header 8 bytes
	char file_name[256];
	tm_unz tmu_date;
};

class lcZipFile
{
public:
	lcZipFile();
	~lcZipFile();

	bool Open(const char* FilePath);
	bool ExtractFile(int FileIndex, lcMemFile& File);

	ObjArray<lcZipFileInfo> mFiles;

protected:
	bool Open();
	bool ReadCentralDir();
	lcuint64 SearchCentralDir();
	lcuint64 SearchCentralDir64();
	bool CheckFileCoherencyHeader(int FileIndex, lcuint32* SizeVar, lcuint64* OffsetLocalExtraField, lcuint32* SizeLocalExtraField);

	lcFile* mFile;

	bool mZip64;
	lcuint64 mNumEntries;
	lcuint64 mCentralDirSize;
	lcuint64 mCentralDirOffset;
	lcuint64 mBytesBeforeZipFile;
	lcuint64 mCentralPos;
};

#endif // _LC_ZIPFILE_H_

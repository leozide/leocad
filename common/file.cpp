// File class, can be a memory file or in the disk.
// Needed to work with the clipboard and undo/redo easily.
// NOTE: Because of endianess issues, all I/O must be done from a File class.

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "defines.h"
#include "config.h"
#include "str.h"

// =============================================================================
// File construction/destruction

File::File ()
{
  strcpy(FileName, "");
}

File::~File ()
{
}

// =============================================================================
// Endian-safe functions

// reads 1-byte integers
unsigned long File::ReadByte (void* pBuf, unsigned long nCount)
{
  return Read (pBuf, nCount);
}

// reads 2-byte integers
unsigned long File::ReadShort (void* pBuf, unsigned long nCount)
{
  unsigned long read;

  read = Read (pBuf, nCount*2)/2;

#ifdef LC_BIG_ENDIAN
  unsigned long i;
  lcuint16* val = (lcuint16*)pBuf, x;

  for (i = 0; i < read; i++)
  {
    x = *val;
    *val = ((x>>8) | (x<<8));
    val++;
  }
#endif

  return read;
}

// reads 4-byte integers
unsigned long File::ReadLong (void* pBuf, unsigned long nCount)
{
  unsigned long read;

  read = Read (pBuf, nCount*4)/4;

#ifdef LC_BIG_ENDIAN
  unsigned long i;
  lcuint32* val = (lcuint32*)pBuf, x;

  for (i = 0; i < read; i++)
  {
    x = *val;
    *val = ((x>>24) | ((x>>8) & 0xff00) | ((x<<8) & 0xff0000) | (x<<24));
    val++;
  }
#endif

  return read;
}

// reads 4-byte floats
unsigned long File::ReadFloat (void* pBuf, unsigned long nCount)
{
  unsigned long read;

  read = Read (pBuf, nCount*4)/4;

#ifdef LC_BIG_ENDIAN
  unsigned long i;
  float* val = (float*)pBuf;
  union { unsigned char b[4]; float f; } in, out;

  for (i = 0; i < read; i++)
  {
    in.f = *val;

    out.b[0] = in.b[3];
    out.b[1] = in.b[2];
    out.b[2] = in.b[1];
    out.b[3] = in.b[0];

    *val = out.f;
    val++;
  }
#endif

  return read;
}

// reads 8-byte floats
unsigned long File::ReadDouble (void* pBuf, unsigned long nCount)
{
  unsigned long read;

  read = Read (pBuf, nCount*8)/8;

#ifdef LC_BIG_ENDIAN
  unsigned long i;
  double* val = (double*)pBuf;
  union { unsigned char b[8]; double d; } in, out;

  for (i = 0; i < read; i++)
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
    val++;
  }
#endif

  return read;
}

// writes 1-byte integers
unsigned long File::WriteByte (const void* pBuf, unsigned long nCount)
{
  return Write (pBuf, nCount);
}

// writes 2-byte integers
unsigned long File::WriteShort (const void* pBuf, unsigned long nCount)
{
#ifdef LC_BIG_ENDIAN
  unsigned long wrote = 0, i;
  lcuint16* val = (lcuint16*)pBuf, x;

  for (i = 0; i < nCount; i++)
  {
    x = (((*val)>>8) | ((*val)<<8));
    val++;
    wrote += Write (&x, 2)/2;
  }

  return wrote;
#else
  return Write(pBuf, nCount*2)/2;
#endif
}

// writes 4-byte integers
unsigned long File::WriteLong (const void* pBuf, unsigned long nCount)
{
#ifdef LC_BIG_ENDIAN
  unsigned long wrote = 0, i;
  lcuint32* val = (lcuint32*)pBuf, x;

  for (i = 0; i < nCount; i++)
  {
    x = (((*val)>>24) | (((*val)>>8) & 0xff00) | (((*val)<<8) & 0xff0000) | ((*val)<<24));
    val++;
    wrote += Write (&x, 4)/4;
  }

  return wrote;
#else
  return Write (pBuf, nCount*4)/4;
#endif
}

// writes 4-byte floats
unsigned long File::WriteFloat (const void* pBuf, unsigned long nCount)
{
#ifdef LC_BIG_ENDIAN
  unsigned long wrote = 0, i;
  float* val = (float*)pBuf, x;
  union { unsigned char b[4]; float f; } in, out;

  for (i = 0; i < nCount; i++)
  {
    in.f = *val;
    val++;

    out.b[0] = in.b[3];
    out.b[1] = in.b[2];
    out.b[2] = in.b[1];
    out.b[3] = in.b[0];
    x = out.f;

    wrote += Write (&x, 4)/4;
  }

  return wrote;
#else
  return Write (pBuf, nCount*4)/4;
#endif
}

// writes 8-byte floats
unsigned long File::WriteDouble (const void* pBuf, unsigned long nCount)
{
#ifdef LC_BIG_ENDIAN
  unsigned long wrote = 0, i;
  double* val = (double*)pBuf, x;
  union { unsigned char b[8]; double d; } in, out;

  for (i = 0; i < nCount; i++)
  {
    in.d = *val;
    val++;

    out.b[0] = in.b[7];
    out.b[1] = in.b[6];
    out.b[2] = in.b[5];
    out.b[3] = in.b[4];
    out.b[4] = in.b[3];
    out.b[5] = in.b[2];
    out.b[6] = in.b[1];
    out.b[7] = in.b[0];
    x = out.d;

    wrote += Write (&x, 8)/8;
  }

  return wrote;
#else
  return Write (pBuf, nCount*8)/8;
#endif
}

void File::ReadString(String& Value)
{
	lcuint32 l;
	ReadInt(&l);
	Read(Value.GetBuffer(l+1), l);
	((char*)Value)[l] = 0;
}

void File::WriteString(const String& Value)
{
	WriteInt(Value.GetLength());
	Write((const char*)Value, Value.GetLength());
}

// =============================================================================

FileMem::FileMem()
{
  m_nGrowBytes = 1024;
  m_nPosition = 0;
  m_nBufferSize = 0;
  m_nFileSize = 0;
  m_pBuffer = NULL;
  m_bAutoDelete = true;
}

FileDisk::FileDisk()
{
  m_hFile = NULL;
  m_bCloseOnDelete = false;
}

FileMem::~FileMem()
{
  if (m_pBuffer)
    Close();

  m_nGrowBytes = 0;
  m_nPosition = 0;
  m_nBufferSize = 0;
  m_nFileSize = 0;
}

FileDisk::~FileDisk()
{
  if (m_hFile != NULL && m_bCloseOnDelete)
    Close();
}

/////////////////////////////////////////////////////////////////////////////
// File operations

char* FileMem::ReadLine(char* pBuf, unsigned long nMax)
{
  int nRead = 0;
  unsigned char ch;

  if (nMax <= 0)
    return NULL;
  if (m_nPosition >= m_nFileSize)
    return NULL;

  while ((--nMax))
  {
    if (m_nPosition == m_nFileSize)
      break;

    ch = m_pBuffer[m_nPosition];
    m_nPosition++;
    pBuf[nRead++] = ch;

    if (ch == '\n')
      break;
  }

  pBuf[nRead] = '\0';
  return pBuf;
}

char* FileDisk::ReadLine(char* pBuf, unsigned long nMax)
{
  return fgets(pBuf, nMax, m_hFile);
}

unsigned long FileMem::Read(void* pBuf, unsigned long nCount)
{
  if (nCount == 0)
    return 0;

  if (m_nPosition > m_nFileSize)
    return 0;

  unsigned long nRead;
  if (m_nPosition + nCount > m_nFileSize)
    nRead = (unsigned long)(m_nFileSize - m_nPosition);
  else
    nRead = nCount;

  memcpy((unsigned char*)pBuf, (unsigned char*)m_pBuffer + m_nPosition, nRead);
  m_nPosition += nRead;

  return nRead;
}

unsigned long FileDisk::Read(void* pBuf, unsigned long nCount)
{
  return fread(pBuf, 1, nCount, m_hFile);
}

int FileMem::GetChar()
{
  if (m_nPosition > m_nFileSize)
    return EOF;

  unsigned char* ret = (unsigned char*)m_pBuffer + m_nPosition;
  m_nPosition++;

  return *ret;
}

int FileDisk::GetChar()
{
  return fgetc(m_hFile);
}

unsigned long FileMem::Write(const void* pBuf, unsigned long nCount)
{
  if (nCount == 0)
    return 0;

  if (m_nPosition + nCount > m_nBufferSize)
    GrowFile(m_nPosition + nCount);

  memcpy((unsigned char*)m_pBuffer + m_nPosition, (unsigned char*)pBuf, nCount);

  m_nPosition += nCount;

  if (m_nPosition > m_nFileSize)
    m_nFileSize = m_nPosition;

  return nCount;
}

unsigned long FileDisk::Write(const void* pBuf, unsigned long nCount)
{
  return fwrite(pBuf, 1, nCount, m_hFile);
}

int FileMem::PutChar(int c)
{
  if (m_nPosition + 1 > m_nBufferSize)
    GrowFile(m_nPosition + 1);

  unsigned char* bt = (unsigned char*)m_pBuffer + m_nPosition;
  *bt = c;

  m_nPosition++;

  if (m_nPosition > m_nFileSize)
    m_nFileSize = m_nPosition;

  return 1;
}

int FileDisk::PutChar(int c)
{
  return fputc(c, m_hFile);
}

bool FileDisk::Open(const char *filename, const char *mode)
{
	if (*filename == 0)
		return false;

  strcpy(FileName, filename);

  m_hFile = fopen(filename, mode);
  m_bCloseOnDelete = true;

  return (m_hFile != NULL);
}

void FileMem::Close()
{
  m_nGrowBytes = 0;
  m_nPosition = 0;
  m_nBufferSize = 0;
  m_nFileSize = 0;
  if (m_pBuffer && m_bAutoDelete)
    free(m_pBuffer);
  m_pBuffer = NULL;
  strcpy(FileName, "");
}

void FileDisk::Close()
{
  if (m_hFile != NULL)
    fclose(m_hFile);

  m_hFile = NULL;
  m_bCloseOnDelete = false;
  strcpy(FileName, "");
}

unsigned long FileMem::Seek(long lOff, int nFrom)
{
  unsigned long lNewPos = m_nPosition;

  if (nFrom == SEEK_SET)
    lNewPos = lOff;
  else if (nFrom == SEEK_CUR)
    lNewPos += lOff;
  else if (nFrom == SEEK_END)
    lNewPos = m_nFileSize + lOff;
  else
    return (unsigned long)-1;

  m_nPosition = lNewPos;

  return 0;
}

unsigned long FileDisk::Seek(long lOff, int nFrom)
{
  fseek (m_hFile, lOff, nFrom);

  return ftell(m_hFile);
}

unsigned long FileMem::GetPosition() const
{
    return m_nPosition;
}

unsigned long FileDisk::GetPosition() const
{
  return ftell(m_hFile);
}

void FileMem::GrowFile(unsigned long nNewLen)
{
  if (nNewLen > m_nBufferSize)
  {
    // grow the buffer
    unsigned long nNewBufferSize = m_nBufferSize;

    // determine new buffer size
    while (nNewBufferSize < nNewLen)
      nNewBufferSize += m_nGrowBytes;

    // allocate new buffer
    unsigned char* lpNew;
    if (m_pBuffer == NULL)
      lpNew = (unsigned char*)malloc(nNewBufferSize);
    else
      lpNew = (unsigned char*)realloc(m_pBuffer, nNewBufferSize);

    m_pBuffer = lpNew;
    m_nBufferSize = nNewBufferSize;
  }
}

void FileMem::Flush()
{
  // Nothing to be done
}

void FileDisk::Flush()
{
  if (m_hFile == NULL)
    return;

  fflush(m_hFile);
}

void FileMem::Abort()
{
  Close();
}

void FileDisk::Abort()
{
  if (m_hFile != NULL)
  {
    // close but ignore errors
    if (m_bCloseOnDelete)
      fclose(m_hFile);
    m_hFile = NULL;
    m_bCloseOnDelete = false;
  }
}

void FileMem::SetLength(unsigned long nNewLen)
{
  if (nNewLen > m_nBufferSize)
    GrowFile(nNewLen);

  if (nNewLen < m_nPosition)
    m_nPosition = nNewLen;

  m_nFileSize = nNewLen;
}

void FileDisk::SetLength(unsigned long nNewLen)
{
  fseek(m_hFile, nNewLen, SEEK_SET);
}

unsigned long FileMem::GetLength() const
{
  return m_nFileSize;
}

unsigned long FileDisk::GetLength() const
{
  unsigned long nLen, nCur;

  // Seek is a non const operation
  nCur = ftell(m_hFile);
  fseek(m_hFile, 0, SEEK_END);
  nLen = ftell(m_hFile);
  fseek(m_hFile, nCur, SEEK_SET);

  return nLen;
}

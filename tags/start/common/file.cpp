// File class, can be a memory file or in the disk.
// Needed to work with the clipboard and undo/redo easily.

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "file.h"

/////////////////////////////////////////////////////////////////////////////
// File construction/destruction

File::File(bool bMemFile)
{
	m_bMemFile = bMemFile;

	if (m_bMemFile)
	{
		m_nGrowBytes = 1024;
		m_nPosition = 0;
		m_nBufferSize = 0;
		m_nFileSize = 0;
		m_pBuffer = NULL;
		m_bAutoDelete = true;
	}
	else
	{
		m_hFile = NULL;
		m_bCloseOnDelete = false;
	}
}

File::~File()
{
	if (m_bMemFile)
	{
		if (m_pBuffer)
			Close();

		m_nGrowBytes = 0;
		m_nPosition = 0;
		m_nBufferSize = 0;
		m_nFileSize = 0;
	}
	else
	{
		if (m_hFile != NULL && m_bCloseOnDelete)
			Close();
	}
}

/////////////////////////////////////////////////////////////////////////////
// File operations

char* File::ReadString(char* pBuf, unsigned long nMax)
{
	if (m_bMemFile)
	{
		int nRead = 0;
		unsigned char ch;

		if (nMax <= 0)
			return NULL;
		if (m_nPosition >= m_nFileSize)
			return NULL;

		while (--nMax)
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
	else
        return fgets(pBuf, nMax, m_hFile);
}

unsigned long File::Read(void* pBuf, unsigned long nCount)
{
	if (nCount == 0)
		return 0;

	if (m_bMemFile)
	{
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
	else
		return fread(pBuf, 1, nCount, m_hFile);
}

int File::GetChar()
{
	if (m_bMemFile)
	{
		if (m_nPosition > m_nFileSize)
			return 0;

		unsigned char* ret = (unsigned char*)m_pBuffer + m_nPosition;
		m_nPosition++;

		return *ret;
	}
	else
		return fgetc(m_hFile);
}

unsigned long File::Write(const void* pBuf, unsigned long nCount)
{
	if (nCount == 0)
		return 0;

	if (m_bMemFile)
	{
		if (m_nPosition + nCount > m_nBufferSize)
			GrowFile(m_nPosition + nCount);

		memcpy((unsigned char*)m_pBuffer + m_nPosition, (unsigned char*)pBuf, nCount);

		m_nPosition += nCount;

		if (m_nPosition > m_nFileSize)
			m_nFileSize = m_nPosition;

		return nCount;
	}
	else
		return fwrite(pBuf, 1, nCount, m_hFile);
}

int File::PutChar(int c)
{
	if (m_bMemFile)
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
	else
		return fputc(c, m_hFile);
}

bool File::Open(const char *filename, const char *mode)
{
	if (m_bMemFile)
		return false;
	else
	{
		m_hFile = fopen(filename, mode);
		m_bCloseOnDelete = true;

		return (m_hFile != NULL);
	}
}

void File::Close()
{
	if (m_bMemFile)
	{
		m_nGrowBytes = 0;
		m_nPosition = 0;
		m_nBufferSize = 0;
		m_nFileSize = 0;
		if (m_pBuffer && m_bAutoDelete)
			free(m_pBuffer);
		m_pBuffer = NULL;
	}
	else
	{
		if (m_hFile != NULL)
			fclose(m_hFile);

		m_hFile = NULL;
		m_bCloseOnDelete = false;
//		m_strFileName.Empty();
	}
}

unsigned long File::Seek(long lOff, int nFrom)
{
	if (m_bMemFile)
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

		return m_nPosition;
	}
	else
	{
		fseek (m_hFile, lOff, nFrom);

		return ftell(m_hFile);
	}
}

unsigned long File::GetPosition() const
{
	if (m_bMemFile)
		return m_nPosition;
	else
		return ftell(m_hFile);
}

void File::GrowFile(unsigned long nNewLen)
{
	if (m_bMemFile)
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
}

void File::Flush()
{
	if (m_bMemFile)
	{

	}
	else
	{
		if (m_hFile == NULL)
			return;

		fflush(m_hFile);
	}
}

void File::Abort()
{
	if (m_bMemFile)
		Close();
	else
	{
		if (m_hFile != NULL)
		{
			// close but ignore errors
			if (m_bCloseOnDelete)
				fclose(m_hFile);
			m_hFile = NULL;
			m_bCloseOnDelete = false;
		}
//		m_strFileName.Empty();
	}
}

void File::SetLength(unsigned long nNewLen)
{
	if (m_bMemFile)
	{
		if (nNewLen > m_nBufferSize)
			GrowFile(nNewLen);

		if (nNewLen < m_nPosition)
			m_nPosition = nNewLen;

		m_nFileSize = nNewLen;
	}
	else
	{
		fseek(m_hFile, nNewLen, SEEK_SET);
	}
}

unsigned long File::GetLength() const
{
	if (m_bMemFile)
	{
		return m_nBufferSize;
	}
	else
	{
		unsigned long nLen, nCur;

		// Seek is a non const operation
		nCur = ftell(m_hFile);
		fseek(m_hFile, 0, SEEK_END);
		nLen = ftell(m_hFile);
		fseek(m_hFile, nCur, SEEK_SET);

		return nLen;
	}
}

// Texture object.
//

#ifdef LC_WINDOWS
#include "stdafx.h"
#endif
#include "GL/glu.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "texture.h"
#include "project.h"
#include "globals.h"
#include "image.h"

/////////////////////////////////////////////////////////////////////////////
// Texture construction/destruction

// Only called for the background image, use LoadIndex()
Texture::Texture()
{
	m_nRef = 1;
	m_nID = 0;
}

Texture::~Texture()
{
}

/////////////////////////////////////////////////////////////////////////////
// Texture attributes

void Texture::AddRef(bool bFilter)
{
	if (m_nRef == 0)
		Load(bFilter);

	m_nRef++;
}

void Texture::DeRef()
{
	m_nRef--;
	if (m_nRef == 0)
		Unload();
}

/////////////////////////////////////////////////////////////////////////////
// Load methods

void Texture::LoadIndex(File* idx)
{
	unsigned char bt;

	// TODO: don't change ref. if reloading
	m_nRef = 0;
	m_nID = 0;

	idx->Read(m_strName, 8);
	idx->ReadShort(&m_nWidth, 1);
	idx->ReadShort(&m_nHeight, 1);
	idx->ReadByte(&bt, 1);

	switch (bt)
	{
	case LC_INTENSITY: m_nType = GL_LUMINANCE; break;
	case LC_RGB: m_nType = GL_RGB; break;
	case LC_RGBA: m_nType = GL_RGBA; break;
	}

	idx->ReadLong(&m_nOffset, 1);
}

void Texture::Unload()
{
	if (m_nID != 0)
		glDeleteTextures(1, &m_nID);
	m_nID = 0;
}

// Load from textures.bin file
void Texture::Load(bool bFilter)
{
	unsigned char* bits;
	char filename[LC_MAXPATH];
	FileDisk bin;
	int size;

	strcpy(filename, project->GetLibraryPath());
	strcat(filename, "textures.bin");
	if (!bin.Open(filename, "rb"))
		return;

	size = m_nWidth*m_nHeight;
	if (m_nType == GL_RGB)
		size *= 3;
	if (m_nType == GL_RGBA)
		size *= 4;
	bits = (unsigned char*)malloc(size);

	bin.Seek(m_nOffset, SEEK_SET);
	bin.Read(bits, size);
	bin.Close();

	if (m_nID == 0)
		glGenTextures(1, &m_nID);

	glBindTexture(GL_TEXTURE_2D, m_nID);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
//	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (m_nType == GL_LUMINANCE)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY4, m_nWidth, m_nHeight,
			0, m_nType, GL_UNSIGNED_BYTE, bits);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bFilter ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bFilter ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, m_nWidth, m_nHeight,
			0, m_nType, GL_UNSIGNED_BYTE, bits);

		if (bFilter)
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB5_A1, 
				m_nWidth, m_nHeight, m_nType, GL_UNSIGNED_BYTE, bits);
	}

	free(bits);
}

bool Texture::LoadFromFile(char* strFilename, bool bFilter)
{
	LC_IMAGE* image = OpenImage(strFilename);

	if (image == NULL)
	{
		if (m_nID != 0)
		{
			glDeleteTextures(1, &m_nID);
			m_nID = 0;
		}
		m_nWidth = 0;
		m_nHeight = 0;

		return false;
	}

	m_nWidth = image->width;
	m_nHeight = image->height;
	m_nType = GL_RGB;

	if (m_nID == 0)
		glGenTextures(1, &m_nID);

	glBindTexture(GL_TEXTURE_2D, m_nID);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bFilter ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bFilter ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, m_nWidth, m_nHeight,
		0, m_nType, GL_UNSIGNED_BYTE, image->bits);

	if (bFilter)
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, 
			m_nWidth, m_nHeight, m_nType, GL_UNSIGNED_BYTE, image->bits);

	free(image);

	return true;
}

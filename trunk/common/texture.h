//
//	texture.h
////////////////////////////////////////////////////

#ifndef _TEXTURE_H
#define _TEXTURE_H

#ifndef GLuint
#include <GL/gl.h>
#endif

class File;

typedef enum { LC_INTENSITY, LC_RGB, LC_RGBA } LC_TEXTURE_TYPES;

class Texture
{
public:
	Texture();
	~Texture();

	void MakeCurrent()
	{
      if (m_nID != 0)
        { glBindTexture(GL_TEXTURE_2D, m_nID); }
	}

	bool IsLoaded()
		{ return glIsTexture(m_nID) == GL_TRUE; }
	void Load(bool bFilter);
	bool LoadFromFile(char* strFilename, bool bFilter);
	void Unload();

	void LoadIndex(File* idx);
	void AddRef(bool bFilter);
	void DeRef();

	// Read-only
	char m_strName[9];
	unsigned short m_nWidth;
	unsigned short m_nHeight;

protected:
	int m_nRef;
	GLuint m_nID;
	GLenum m_nType;
	unsigned long m_nOffset;
};


#endif // _TEXTURE_H

#ifndef _LC_LIBRARY_H_
#define _LC_LIBRARY_H_

class lcPiecesLibrary
{
public:
	lcPiecesLibrary();
	~lcPiecesLibrary();

	bool OpenArchive(const char* LibPath);
};

#endif // _LC_LIBRARY_H_

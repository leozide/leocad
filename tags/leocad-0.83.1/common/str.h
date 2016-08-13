#ifndef _STR_H_
#define _STR_H_

#include <string.h>

class String
{
public:
	String();
	String(const String& src)
	{ m_pData = NULL; *this = src; }
	String(const char* str)
	{ m_pData = NULL; *this = str; }
	~String();

	size_t GetLength() const
	{ return strlen(m_pData); }
	bool IsEmpty() const
	{ return m_pData[0] == '\0'; }
	void Empty()
	{ m_pData[0] = '\0'; }

	char GetAt(int index) const
	{ return m_pData[index]; }
	char& operator[](int index) const
	{ return m_pData[index]; }
	void SetAt(int index, char ch)
	{ m_pData[index] = ch; }
	operator char*() const
	{ return m_pData; }
	operator const char*() const
	{ return m_pData; }

	// Operators
	const String& operator=(const String& src);
	const String& operator=(char ch);
	const String& operator=(const char *src);
	const String& operator+=(const String& string);
	const String& operator+=(char ch);
	const String& operator+=(const char *src);

	// Comparison
	int Compare(const char *string) const
	{ return strcmp(m_pData, string); }
	int CompareNoCase(const char *string) const;
	int CompareNoCase(const char *string, int count) const;
	bool Match(const String& Expression) const;

	// simple sub-string extraction
	String& Mid(int first, int count) const;
	String& Left(int count) const;
	String& Right(int count) const;

	// upper/lower/reverse conversion
	void MakeUpper();
	void MakeLower();

	// trimming whitespace (either side)
	void TrimRight();
	void TrimLeft();

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(char ch) const
	{
		char *pf = strchr(m_pData, ch);
		return (pf) ? (pf - m_pData) : -1;
	}
	int ReverseFind(char ch) const
	{
		char *pf = strrchr(m_pData, ch);
		return (pf) ? (pf - m_pData) : -1;
	}
	int FindOneOf(const char *set) const
	{
		char *pf = strpbrk(m_pData, set);
		return (pf) ? (pf - m_pData) : -1;
	}

	// look for a specific sub-string
	int Find(const char *str) const
	{
		char *pf = strstr(m_pData, str);
		return (pf) ? (pf - m_pData) : -1;
	}

	char* Buffer()
	{
		return m_pData;
	}

	const char* Buffer() const
	{
		return m_pData;
	}

	char* GetBuffer(int len)
	{
		if (len > (int)strlen(m_pData))
		{
			char *tmp = new char[len+1];
			strcpy(tmp, m_pData);
			delete []m_pData;
			m_pData = tmp;
		}
		return m_pData;
	}

protected:
	char* m_pData;
};

// Concatenation operators
String& operator+(const String& string1, const String& string2);
String& operator+(const String& string, char ch);
String& operator+(char ch, const String& string);
String& operator+(const String& string1, const char *string2);
String& operator+(const char *string1, const String& string2);

// Comparison operators
inline bool operator==(const String& s1, const String& s2)
{ return s1.Compare(s2) == 0; }
inline bool operator==(const String& s1, const char *s2)
{ return s1.Compare(s2) == 0; }
inline bool operator==(const char *s1, const String& s2)
{ return s2.Compare(s1) == 0; }
inline bool operator!=(const String& s1, const String& s2)
{ return s1.Compare(s2) != 0; }
inline bool operator!=(const String& s1, const char *s2)
{ return s1.Compare(s2) != 0; }
inline bool operator!=(const char *s1, const String& s2)
{ return s2.Compare(s1) != 0; }
inline bool operator<(const String& s1, const String& s2)
{ return s1.Compare(s2) < 0; }
inline bool operator<(const String& s1, const char *s2)
{ return s1.Compare(s2) < 0; }
inline bool operator<(const char *s1, const String& s2)
{ return s2.Compare(s1) > 0; }
inline bool operator>(const String& s1, const String& s2)
{ return s1.Compare(s2) > 0; }
inline bool operator>(const String& s1, const char *s2)
{ return s1.Compare(s2) > 0; }
inline bool operator>(const char *s1, const String& s2)
{ return s2.Compare(s1) < 0; }
inline bool operator<=(const String& s1, const String& s2)
{ return s1.Compare(s2) <= 0; }
inline bool operator<=(const String& s1, const char *s2)
{ return s1.Compare(s2) <= 0; }
inline bool operator<=(const char *s1, const String& s2)
{ return s2.Compare(s1) >= 0; }
inline bool operator>=(const String& s1, const String& s2)
{ return s1.Compare(s2) >= 0; }
inline bool operator>=(const String& s1, const char *s2)
{ return s1.Compare(s2) >= 0; }
inline bool operator>=(const char *s1, const String& s2)
{ return s2.Compare(s1) <= 0; }

#endif // _STR_H_

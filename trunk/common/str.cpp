//
// General purpose string class
//

#include <ctype.h>
#include "str.h"

static String aux;

// =============================================================================
// Construction / Destruction

String::String ()
{
  m_pData = new char[1];
  m_pData[0] = '\0';
}

String::~String ()
{
  delete []m_pData;
}

// =============================================================================
// Operators

const String& String::operator= (const String& src)
{
  delete []m_pData;
  m_pData = new char[src.GetLength () + 1];
  strcpy (m_pData, src.m_pData);
  return *this;
}

const String& String::operator= (char ch)
{
  delete []m_pData;
  m_pData = new char[2];
  m_pData[0] = ch;
  m_pData[1] = '\0';
  return *this;
}

const String& String::operator= (const char *src)
{
  delete []m_pData;
  m_pData = new char[strlen (src) + 1];
  strcpy (m_pData, src);
  return *this;
}

const String& String::operator+= (const String& src)
{
  char *tmp = new char[GetLength () + src.GetLength () + 1];
  strcpy (tmp, m_pData);
  strcat (tmp, src.m_pData);
  delete []m_pData;
  m_pData = tmp;
  return *this;
}

const String& String::operator+= (char ch)
{
  int len = GetLength ();
  char *tmp = new char[len + 1 + 1];
  strcpy (tmp, m_pData);
  tmp[len] = ch;
  tmp[len+1] = '\0';
  delete []m_pData;
  m_pData = tmp;
  return *this;
}

const String& String::operator+= (const char *src)
{
  char *tmp = new char[GetLength () + strlen (src) + 1];
  strcpy (tmp, m_pData);
  strcat (tmp, src);
  delete []m_pData;
  m_pData = tmp;
  return *this;
}

// =============================================================================
// Non-member operators

String& operator+ (const String& string1, const String& string2)
{
  String s;
  s = string1;
  s += string2;
  aux = s;
  return aux;
}

String& operator+ (const String& string, char ch)
{
  String s;
  s = string;
  s += ch;
  aux = s;
  return aux;
}

String& operator+ (char ch, const String& string)
{
  String s;
  s = ch;
  s += string;
  aux = s;
  return aux;
}

String& operator+ (const String& string1, const char *string2)
{
  String s;
  s = string1;
  s += string2;
  aux = s;
  return aux;
}

String& operator+ (const char *string1, const String& string2)
{
  String s;
  s = string1;
  s += string2;
  aux = s;
  return aux;
}

// =============================================================================
// Sub-string extraction

String& String::Mid (int first, int count) const
{
  if (count < 0)
    count = 0;
  else if (count > GetLength ())
    count = GetLength ();

  String s;
  strncpy (s.GetBuffer (count+1), m_pData + first, count);
  s.m_pData[count] = '\0';
  aux = s;

  return aux;
}

String& String::Left (int count) const
{
  if (count < 0)
    count = 0;
  else if (count > GetLength ())
    count = GetLength ();

  String s;
  strncpy (s.GetBuffer (count+1), m_pData, count);
  s.m_pData[count] = '\0';
  aux = s;

  return aux;
}

String& String::Right (int count) const
{
  if (count < 0)
    count = 0;
  else if (count > GetLength ())
    count = GetLength ();

  String s;
  strncpy (s.GetBuffer (count+1), m_pData + GetLength () - count, count);
  s.m_pData[count] = '\0';
  aux = s;

  return aux;
}

// =============================================================================
// Other functions

int String::CompareNoCase (const char *string) const
{
  char c1, c2, *ch = m_pData;
  while (*ch && *string)
  {
    c1 = tolower (*ch);
    c2 = tolower (*string);
    if (c1 != c2)
      return (c1 - c2);
    ch++; string++;
  }
  return (((int)*ch) - ((int)*string));
}

void String::MakeUpper ()
{
  for (char *cp = m_pData; *cp; ++cp)
    if ('a' <= *cp && *cp <= 'z')
      *cp += 'A' - 'a';
}

void String::MakeLower ()
{
  for (char *cp = m_pData; *cp; ++cp)
    if ('A' <= *cp && *cp <= 'Z')
	  *cp += 'a' - 'A';
}

void String::MakeReverse ()
{
  register char *h, *t;

  h = m_pData;
  t = m_pData + strlen (m_pData) - 1;

  while (h < t)
  {
    register char c;

    c = *h;
    *h = *t;
    h++;
    *t = c;
    t--;
  }
}

void String::TrimRight ()
{
  for (char *s = m_pData + strlen (m_pData) - 1; s >= m_pData && isspace (*s); s--)
    *s = '\0';
}

void String::TrimLeft ()
{
  char *ch;
  ch = m_pData;
  while (isspace (*ch))
    ch++;
  memmove (m_pData, ch, strlen (ch)+1);
}

//
// General purpose string class
//

#include "lc_global.h"
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

// Evaluates the contents of the string against a boolean expression.
// For example: (^Car | %Animal) & !Parrot
// Will return true for any strings that have the Car word or
// begin with Animal and do not have the word Parrot.
bool String::Match(const String& Expression) const
{
	// Check if we need to split the test expression.
	const char* p = Expression;

	while (*p)
	{
		if (*p == '!')
		{
			return !Match(String(p + 1));
		}
		else if (*p == '(')
		{
//			const char* Start = p;
			int c = 0;

			// Skip what's inside the parenthesis.
			do
			{
				if (*p == '(')
						c++;
				else if (*p == ')')
						c--;
				else if (*p == 0)
					return false; // Mismatched parenthesis.

				p++;
			}
			while (c);

			if (*p == 0)
				break;
		}
		else if ((*p == '|') || (*p == '&'))
		{
			String Left, Right;

			Left = Expression.Left((p - Expression) - 1);
			Right = Expression.Right(Expression.GetLength() - (p - Expression) - 1);

			if (*p == '|')
				return Match(Left) || Match(Right);
			else
				return Match(Left) && Match(Right);
		}

		p++;
	}

	if (Expression.Find('(') != -1)
	{
		p = Expression;
		while (*p)
		{
			if (*p == '(')
			{
				const char* Start = p;
				int c = 0;

				// Extract what's inside the parenthesis.
				do
				{
					if (*p == '(')
							c++;
					else if (*p == ')')
							c--;
					else if (*p == 0)
						return false; // Mismatched parenthesis.

					p++;
				}
				while (c);

				String Expr = Expression.Mid(Start - Expression + 1, p - Start - 2);
				return Match(Expr);
			}

			p++;
		}
	}

	// Testing a simple case.
	String Search = Expression;
	Search.TrimRight();
	Search.TrimLeft();

	const char* Word = Search;

	// Check for modifiers.
	bool WholeWord = 0;
	bool Begin = 0;

	for (;;)
	{
		if (Word[0] == '^')
			WholeWord = true;
		else if (Word[0] == '%')
			Begin = true;
		else
			break;

		Word++;
	}

	int Result = Find(Word);

	if (Result == -1)
		return false;

	if (Begin && (Result != 0))
	{
		if ((Result != 1) || ((GetAt(Result-1) != '_') && (GetAt(Result-1) != '~')))
			return false;
	}

	if (WholeWord)
	{
		char End = GetAt(Result + strlen(Word));

		if ((End != 0) && (End != ' '))
			return false;

		if ((Result != 0) && ((GetAt(Result-1) == '_') || (GetAt(Result-1) == '~')))
			Result--;

		if ((Result != 0) && (GetAt(Result-1) != ' '))
			return false;
	}

	return true;
}

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

int String::CompareNoCase(const char *string, int count) const
{
  char c1, c2, *ch = m_pData;

  while (*ch && *string)
  {
    c1 = tolower (*ch);
    c2 = tolower (*string);

    if (c1 != c2)
      return (c1 - c2);

    ch++;
    string++;
    count--;

    if (!count)
      return 0;
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

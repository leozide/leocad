#include "lc_global.h"
#include "lc_filter.h"

lcFilter::lcFilter(const std::string_view FilterString)
{
	size_t TokenStart = std::string_view::npos;
	FilterOp Op = FilterOp::Or;
	bool InQuotes = false;

	auto EndToken=[this, FilterString, &Op, &TokenStart, &InQuotes](size_t Index)
	{
		if (!InQuotes && TokenStart != std::string_view::npos)
		{
			std::string_view Token = FilterString.substr(TokenStart, Index - TokenStart);

			if (Token == "AND")
				Op = FilterOp::And;
			else if (Token == "OR")
				Op = FilterOp::Or;
			else
			{
				mFilterParts.emplace_back(FilterPart{Op, Token.find_first_of("*?") != std::string_view::npos, std::string(Token)});
				Op = FilterOp::And;
			}

			TokenStart = std::string_view::npos;
		}
	};

	for (size_t Index = 0; Index < FilterString.size(); Index++)
	{
		switch (FilterString[Index])
		{
		case ' ':
			EndToken(Index);
			break;

		case '-':
			if (!InQuotes && TokenStart == std::string_view::npos)
			{
				if (Op == FilterOp::And)
					Op = FilterOp::AndNot;
				else if (Op == FilterOp::Or)
					Op = FilterOp::OrNot;
			}
			break;

		case '\"':
			if (InQuotes)
			{
				std::string_view Token = FilterString.substr(TokenStart + 1, Index - TokenStart - 1);

				if (!Token.empty())
					mFilterParts.emplace_back(FilterPart{Op, Token.find_first_of("*?") != std::string_view::npos, std::string(Token)});

				TokenStart = std::string_view::npos;
				Op = FilterOp::And;
			}
			else
			{
				if (TokenStart == std::string_view::npos)
					TokenStart = Index;
			}
			InQuotes = !InQuotes;
			break;

		default:
			if (TokenStart == std::string_view::npos)
				TokenStart = Index;
			break;
		}
	}

	EndToken(FilterString.size());
}

bool lcFilter::Match(const char* String) const
{
	bool CurrentMatch = mFilterParts.empty();

	for (const FilterPart& FilterPart : mFilterParts)
	{
		bool Match = FilterPart.Wildcard ? FastWildCompare(String, FilterPart.String.c_str()) : strcasestr(String, FilterPart.String.c_str());

		switch (FilterPart.Op)
		{
		case FilterOp::And:
			CurrentMatch &= Match;
			break;

		case FilterOp::AndNot:
			CurrentMatch &= !Match;
			break;

		case FilterOp::Or:
			CurrentMatch |= Match;
			break;

		case FilterOp::OrNot:
			CurrentMatch |= !Match;
			break;
		}
	}

	return CurrentMatch;
}

// Copyright 2018 IBM Corporation
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Compares two text strings.  Accepts '?' as a single-character wildcard.  
// For each '*' wildcard, seeks out a matching sequence of any characters 
// beyond it.  Otherwise compares the strings a character at a time. 
//
bool lcFilter::FastWildCompare(const char* Tame, const char* Wild)
{
    const char* WildSequence;  // Points to prospective wild string match after '*'
    const char* TameSequence;  // Points to prospective tame string match
 
	auto NotEquals=[](char a, char b)
	{
			 // Lowercase the characters to be compared.
                if (a >= 'A' && a <= 'Z')
                    a += ('a' - 'A');
 
                if (b >= 'A' && b <= 'Z')
                    b += ('a' - 'A');

		return a != b;
	};

    // Find a first wildcard, if one exists, and the beginning of any  
    // prospectively matching sequence after it.
    do
    {
        // Check for the end from the start.  Get out fast, if possible.
        if (!*Tame)
        {
            if (*Wild)
            {
                while (*(Wild++) == '*')
                {
                    if (!(*Wild))
                    {
                        return true;   // "ab" matches "ab*".
                    }
                }
 
                return false;          // "abcd" doesn't match "abc".
            }
            else
            {
                return true;           // "abc" matches "abc".
            }
        }
        else if (*Wild == '*')
        {
            // Got wild: set up for the second loop and skip on down there.
            while (*(++Wild) == '*')
            {
                continue;
            }
 
            if (!*Wild)
            {
                return true;           // "abc*" matches "abcd".
            }
 
            // Search for the next prospective match.
            if (*Wild != '?')
            {
                while (NotEquals(*Wild, *Tame))
                {
                    if (!*(++Tame))
                    {
                        return false;  // "a*bc" doesn't match "ab".
                    }
                }
            }
 
            // Keep fallback positions for retry in case of incomplete match.
            WildSequence = Wild;
            TameSequence = Tame;
            break;
        }
        else if (NotEquals(*Wild, *Tame) && *Wild != '?')
        {
            return false;              // "abc" doesn't match "abd".
        }
 
        ++Wild;                       // Everything's a match, so far.
        ++Tame;
    } while (true);
 
    // Find any further wildcards and any further matching sequences.
    do
    {
        if (*Wild == '*')
        {
            // Got wild again.
            while (*(++Wild) == '*')
            {
                continue;
            }
 
            if (!*Wild)
            {
                return true;           // "ab*c*" matches "abcd".
            }
 
            if (!*Tame)
            {
                return false;          // "*bcd*" doesn't match "abc".
            }
 
            // Search for the next prospective match.
            if (*Wild != '?')
            {
                while (NotEquals(*Wild, *Tame))
                {
                    if (!*(++Tame))
                    {
                        return false;  // "a*b*c" doesn't match "ab".
                    }
                }
            }
 
            // Keep the new fallback positions.
            WildSequence = Wild;
            TameSequence = Tame;
        }
        else if (NotEquals(*Wild, *Tame) && *Wild != '?')
        {
            // The equivalent portion of the upper loop is really simple.
            if (!*Tame)
            {
                return false;          // "*bcd" doesn't match "abc".
            }
 
            // A fine time for questions.
            while (*WildSequence == '?')
            {
                ++WildSequence;
                ++TameSequence;
            }
 
            Wild = WildSequence;
 
            // Fall back, but never so far again.
            while (NotEquals(*Wild, *(++TameSequence)))
            {
                if (!*TameSequence)
                {
                    return false;      // "*a*b" doesn't match "ac".
                }
            }
 
            Tame = TameSequence;
        }
 
        // Another check for the end, at the end.
        if (!*Tame)
        {
            if (!*Wild)
            {
                return true;           // "*bc" matches "abc".
            }
            else
            {
                return false;          // "*bc" doesn't match "abcd".
            }
        }
 
        ++Wild;                       // Everything's still a match.
        ++Tame;
    } while (true);
}

#pragma once

char* lcstrcasestr(const char* s, const char* find);
char* lcstrupr(char* string);
size_t lcstrcpy(char* dest, const char* source, size_t count);

template<size_t size>
size_t lcstrcpy(char (&dest)[size], const char* source)
{
	return lcstrcpy(dest, source, size);
}

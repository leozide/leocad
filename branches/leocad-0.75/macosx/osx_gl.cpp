#include <mach-o/dyld.h>
#include <stdlib.h>
#include <string.h>

bool Sys_GLOpenLibrary(const char* LibName)
{
	return true;
}

void Sys_GLCloseLibrary()
{
}

void* Sys_GLGetProc(const char* ProcName)
{
	NSSymbol Symbol = NULL;
	char* SymbolName;

	// Prepend a '_' for the Unix C symbol mangling convention.
	SymbolName = (char*)malloc(strlen(ProcName) + 2);
	strcpy(SymbolName + 1, ProcName);
	SymbolName[0] = '_';

	if (NSIsSymbolNameDefined(SymbolName))
		Symbol = NSLookupAndBindSymbol(SymbolName);
	free(SymbolName);

	return Symbol ? NSAddressOfSymbol(Symbol) : NULL;
}

void* Sys_GLGetExtension(const char* ProcName)
{
	return Sys_GLGetProc(ProcName);
}

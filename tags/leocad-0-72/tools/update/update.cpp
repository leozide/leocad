#ifdef LC_WINDOWS
#include <windows.h>
#endif
#include <stdio.h>
#include "defines.h"
#include "file.h"
#include "update.h"

#define UPDATE_DELETE		0x00
#define UPDATE_DESCRIPTION	0x01
#define UPDATE_DRAWINFO		0x02
#define UPDATE_NEWPIECE		0x04

typedef struct
{
	char oldname[9];
	char newname[9];
} LC_MOVED_DATA;

typedef struct
{
    char name[9];
    char description[65];
    short bricksize[6];
    unsigned char flags;
    unsigned long group;
    unsigned long offset;
    unsigned long size;
} LC_PIECEINFO;

// ========================================================

static void message(File* f, char *fmt, ...)
{
	char s[300];
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(s, fmt, argptr);
	va_end(argptr);

	printf(s);
	f->Write(s, strlen(s));
}

// ========================================================

int main(int argc, char* argv[])
{
	char oldpath[LC_MAXPATH], newpath[LC_MAXPATH], str[LC_MAXPATH];
	FileDisk oldidx, oldbin, newidx, newbin, out, txt;
	int number;

	if (argc != 4)
	{
		printf("LeoCAD Libray Update\n");
		printf("Old path: ");
		scanf("%s", oldpath);
		printf("New path: ");
		scanf("%s", newpath);
		printf("Update number: ");
		scanf("%d", &number);
	}
	else
	{
		strcpy(oldpath, argv[1]);
		strcpy(newpath, argv[2]);
		sscanf(argv[3], "%d", &number);
	}

	if ((oldpath[strlen(oldpath)-1] != '/') &&
		(oldpath[strlen(oldpath)-1] != '\\'))
		strcat(oldpath, "/");

	if ((newpath[strlen(newpath)-1] != '/') &&
		(newpath[strlen(newpath)-1] != '\\'))
		strcat(newpath, "/");

	// Open files
	strcpy(str, oldpath);
	strcat(str, "pieces.bin");
	if (!oldbin.Open(str, "rb"))
		return 1;

	strcpy(str, oldpath);
	strcat(str, "pieces.idx");
	if (!oldidx.Open(str, "rb"))
		return 1;

	strcpy(str, newpath);
	strcat(str, "pieces.bin");
	if (!newbin.Open(str, "rb"))
		return 1;

	strcpy(str, newpath);
	strcat(str, "pieces.idx");
	if (!newidx.Open(str, "rb"))
		return 1;

	sprintf(str, "%supdate%02d.lup", newpath, number);
	if (!out.Open(str, "wb"))
		return 1;

	sprintf(str, "%supdate%02d.txt", newpath, number);
	if (!txt.Open(str, "wt"))
		return 1;

	// Now we start the update
	const char id[32] = "LeoCAD piece library update\0\0\0\0";
	unsigned short oldcount, newcount, changes = 0;
	LC_PIECEINFO *oldinfo, *newinfo;
	void *oldbuf, *newbuf;
	unsigned long sz;
	unsigned char bt;
	int i, j;

	out.Write(id, 32);
	bt = 2;	// version
	out.WriteByte(&bt, 1);
	bt = number; // update number
	out.WriteByte(&bt, 1);

	oldidx.Seek(-2, SEEK_END);
	oldidx.ReadShort(&oldcount, 1);
	oldidx.Seek(34, SEEK_SET);
	newidx.Seek (-2, SEEK_END);
	newidx.ReadShort(&newcount, 1);
	newidx.Seek(34, SEEK_SET);
	message(&txt, "Old count: %d, New count: %d\n\n", oldcount, newcount);

	oldinfo = (LC_PIECEINFO*)malloc(sizeof(LC_PIECEINFO)*oldcount);
	memset(oldinfo, 0, sizeof(LC_PIECEINFO)*oldcount);
	newinfo = (LC_PIECEINFO*)malloc(sizeof(LC_PIECEINFO)*newcount);
	memset(newinfo, 0, sizeof(LC_PIECEINFO)*newcount);

	for (i = 0; i < oldcount; i++)
	{
		oldidx.Read(&oldinfo[i].name, 8);
		oldidx.Read(&oldinfo[i].description, 64);
		oldidx.Read(&oldinfo[i].bricksize, 12);
		oldidx.ReadByte(&oldinfo[i].flags, 1);
		oldidx.ReadLong(&oldinfo[i].group, 1);
		oldidx.ReadLong(&oldinfo[i].offset, 1);
		oldidx.ReadLong(&oldinfo[i].size, 1);
	}

	for (i = 0; i < newcount; i++)
	{
		newidx.Read(&newinfo[i].name, 8);
		newidx.Read(&newinfo[i].description, 64);
		newidx.Read(&newinfo[i].bricksize, 12);
		newidx.ReadByte(&newinfo[i].flags, 1);
		newidx.ReadLong(&newinfo[i].group, 1);
		newidx.ReadLong(&newinfo[i].offset, 1);
		newidx.ReadLong(&newinfo[i].size, 1);
	}

	for (i = 0; i < oldcount; i++)
	{
		unsigned char update = 0;

		for (j = 0; j < newcount; j++)
		{
			if (strcmp(newinfo[j].name, oldinfo[i].name) == 0)
			{
				// Compare Descriptions
				if (strcmp(newinfo[j].description, oldinfo[i].description) != 0)
					update |= UPDATE_DESCRIPTION;

				// Compare drawinfo
				sz = newinfo[j].size;
				if (sz != oldinfo[i].size)
				{
					update |= UPDATE_DRAWINFO;
					newbuf = malloc(sz);
					newbin.Seek(newinfo[j].offset, SEEK_SET);
					newbin.Read(newbuf, sz);
				}
				else
				{
					oldbuf = malloc(sz);
					oldbin.Seek(oldinfo[i].offset, SEEK_SET);
					oldbin.Read(oldbuf, sz);
					newbuf = malloc(sz);
					newbin.Seek(newinfo[j].offset, SEEK_SET);
					newbin.Read(newbuf, sz);

					if (memcmp(newbuf, oldbuf, sz) != 0)
						update |= UPDATE_DRAWINFO;
					else
						free(newbuf);

					free(oldbuf);
				}
				break;
			}
		}

		// deleted
		if (j == newcount)
		{
			message(&txt, "%s (%s) deleted\n", oldinfo[i].name, oldinfo[i].description);
			out.Write(&oldinfo[i].name, 8);
			out.WriteByte(&update, 1);
			changes++;
		}
		else
		{
			if (update != 0)
			{
				message(&txt, "%s (%s) updated ", oldinfo[i].name, oldinfo[i].description);
				if (update & UPDATE_DESCRIPTION)
				{
					strcat(str, "description");
					if (update & UPDATE_DRAWINFO)
						strcat(str, " & ");
				}
				if (update & UPDATE_DRAWINFO)
					strcat(str, "draw info");

				changes++;
				out.Write(&oldinfo[i].name, 8);
				out.WriteByte(&update, 1);

				if (update & UPDATE_DESCRIPTION)
				{
					out.Write(&newinfo[j].description, 64);
					out.WriteLong(&newinfo[j].group, 1);
				}

				if (update & UPDATE_DRAWINFO)
				{
					out.Write(&newinfo[j].bricksize, 12);
					out.WriteByte(&newinfo[j].flags, 1);
					out.WriteLong(&sz, 1);
					out.Write(newbuf, sz);
					free(newbuf);
				}

				message(&txt, "\n");
			}
		}
	}

	message(&txt, "------------\n");

	for (j = 0; j < newcount; j++)
	{
		for (i = 0; i < oldcount; i++)
			if (strcmp(newinfo[j].name, oldinfo[i].name) == 0)
				break;

		// found a new piece
		if (i == oldcount)
		{
			message(&txt, "%s (%s) added\n", newinfo[j].name, newinfo[j].description);

			bt = UPDATE_NEWPIECE;
			out.Write(&newinfo[j].name, 8);
			out.Write(&bt, 1);
			changes++;

			sz = newinfo[j].size;
			newbuf = malloc(sz);
			newbin.Seek(newinfo[j].offset, SEEK_SET);
			newbin.Read(newbuf, sz);
			out.Write(&newinfo[j].description, 64);
			out.Write(&newinfo[j].bricksize, 12);
			out.WriteLong(&newinfo[j].group, 1);
			out.WriteByte(&newinfo[j].flags, 1);
			out.WriteLong(&sz, 1);
			out.Write(newbuf, sz);
			free(newbuf);
		}
	}

	// Moved pieces list
	strcpy(str, newpath);
	strcat(str, "moved.txt");
	FILE* f = fopen(str, "rt");
	char buf[100], f1[9], f2[9], *ptr, *ptr2;
	unsigned short movecount = 0;

	while (fgets(buf, 100, f) != NULL)
	{
		if (ptr = strchr(buf, ' '))
		{
			*ptr = 0;
			memset(f1, 0, 9);
			strcpy(f1, buf);
			ptr++;
			if (ptr2 = strchr(ptr, ' '))
			{
				*ptr2 = 0;
				memset(f2, 0, 9);
				strcpy(f2, ptr);
				movecount++;
				strupr(f1);
				strupr(f2);
				out.Write(f1, 8);
				out.Write(f2, 8);
			}
		}
	}
	fclose(f);

	message(&txt, "\nTotal changes: %d\nMoved files: %d\n", changes, movecount);

	out.WriteShort(&movecount, 1);
	out.WriteShort(&changes, 1);

	free(oldinfo);
	free(newinfo);

	oldidx.Close();
	oldbin.Close();
	newidx.Close();
	newbin.Close();
	out.Close();
	txt.Close();

	return 0;
}

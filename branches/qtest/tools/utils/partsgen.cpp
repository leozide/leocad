#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void GeneratePart(const char* ID, const char* Description)
{
	const int StudSides = 16;

	char FileName[64];
	strcpy(FileName, ID);
	strcat(FileName, ".DAT");

	FILE* output = fopen(FileName, "wt");
	fprintf(output, "0 %s\n", Description);
	fprintf(output, "0 This file is part of LeoCAD's built-in fallback library and released under GPL2\n", Description);

	bool Brick = !strncmp(Description, "Brick ", 6);
	bool Plate = !strncmp(Description, "Plate ", 6);

	if (Brick || Plate)
	{
		int StudsX, StudsY;
		float MinZ = Brick ? -0.96f : -0.32f;

		sscanf(Description + 6, "%d x %d", &StudsY, &StudsX);

		int NumVertices = (StudSides * 2 + 1) * StudsX * StudsY + 16;
		int NumIndices = ((StudSides * 3) * StudsX * StudsY + 28) * 3 + ((StudSides * 2) * StudsX * StudsY + 24) * 2;

		float* VertexBuffer = (float*)malloc(NumVertices * 3 * sizeof(float));
		int* IndexBuffer = (int*)malloc(NumIndices * sizeof(int));

		float* Verts = VertexBuffer;
		int* Indices = IndexBuffer;

		const float OutBoxMin[3] = { -0.4f * StudsX, -0.4f * StudsY, MinZ };
		const float OutBoxMax[3] = { 0.4f * StudsX, 0.4f * StudsY, 0.0f };

		*Verts++ = OutBoxMin[0]; *Verts++ = OutBoxMin[1]; *Verts++ = OutBoxMin[2];
		*Verts++ = OutBoxMin[0]; *Verts++ = OutBoxMax[1]; *Verts++ = OutBoxMin[2];
		*Verts++ = OutBoxMax[0]; *Verts++ = OutBoxMax[1]; *Verts++ = OutBoxMin[2];
		*Verts++ = OutBoxMax[0]; *Verts++ = OutBoxMin[1]; *Verts++ = OutBoxMin[2];
		*Verts++ = OutBoxMin[0]; *Verts++ = OutBoxMin[1]; *Verts++ = OutBoxMax[2];
		*Verts++ = OutBoxMin[0]; *Verts++ = OutBoxMax[1]; *Verts++ = OutBoxMax[2];
		*Verts++ = OutBoxMax[0]; *Verts++ = OutBoxMax[1]; *Verts++ = OutBoxMax[2];
		*Verts++ = OutBoxMax[0]; *Verts++ = OutBoxMin[1]; *Verts++ = OutBoxMax[2];

		const float InBoxMin[3] = { -0.4f * StudsX + 0.16f, -0.4f * StudsY + 0.16f, MinZ };
		const float InBoxMax[3] = { 0.4f * StudsX - 0.16f, 0.4f * StudsY - 0.16f, -0.16f };

		*Verts++ = InBoxMin[0]; *Verts++ = InBoxMin[1]; *Verts++ = InBoxMin[2];
		*Verts++ = InBoxMin[0]; *Verts++ = InBoxMax[1]; *Verts++ = InBoxMin[2];
		*Verts++ = InBoxMax[0]; *Verts++ = InBoxMax[1]; *Verts++ = InBoxMin[2];
		*Verts++ = InBoxMax[0]; *Verts++ = InBoxMin[1]; *Verts++ = InBoxMin[2];
		*Verts++ = InBoxMin[0]; *Verts++ = InBoxMin[1]; *Verts++ = InBoxMax[2];
		*Verts++ = InBoxMin[0]; *Verts++ = InBoxMax[1]; *Verts++ = InBoxMax[2];
		*Verts++ = InBoxMax[0]; *Verts++ = InBoxMax[1]; *Verts++ = InBoxMax[2];
		*Verts++ = InBoxMax[0]; *Verts++ = InBoxMin[1]; *Verts++ = InBoxMax[2];

		for (int x = 0; x < StudsX; x++)
		{
			for (int y = 0; y < StudsY; y++)
			{
				const float Center[3] = { ((float)StudsX / 2.0f - x) * 0.8f - 0.4f, ((float)StudsY / 2.0f - y) * 0.8f - 0.4f, 0.0f };

				*Verts++ = Center[0]; *Verts++ = Center[1]; *Verts++ = 0.16f;

				for (int Step = 0; Step < StudSides; Step++)
				{
					float s = Center[0] + sinf((float)Step / (float)StudSides * 2 * M_PI) * 0.24f;
					float c = Center[1] + cosf((float)Step / (float)StudSides * 2 * M_PI) * 0.24f;

					*Verts++ = s; *Verts++ = c; *Verts++ = 0.16f;
					*Verts++ = s; *Verts++ = c; *Verts++ = 0.0f;
				}
			}
		}

		*Indices++ = 0; *Indices++ = 1; *Indices++ = 8;
		*Indices++ = 1; *Indices++ = 8; *Indices++ = 9;

		*Indices++ = 2; *Indices++ = 3; *Indices++ = 10;
		*Indices++ = 3; *Indices++ = 10; *Indices++ = 11;

		*Indices++ = 0; *Indices++ = 8; *Indices++ = 11;
		*Indices++ = 0; *Indices++ = 11; *Indices++ = 3;

		*Indices++ = 1; *Indices++ = 9; *Indices++ = 10;
		*Indices++ = 1; *Indices++ = 10; *Indices++ = 2;

		*Indices++ = 7; *Indices++ = 6; *Indices++ = 5;
		*Indices++ = 7; *Indices++ = 5; *Indices++ = 4;

		*Indices++ = 0; *Indices++ = 1; *Indices++ = 5;
		*Indices++ = 0; *Indices++ = 5; *Indices++ = 4;

		*Indices++ = 2; *Indices++ = 3; *Indices++ = 7;
		*Indices++ = 2; *Indices++ = 7; *Indices++ = 6;

		*Indices++ = 0; *Indices++ = 3; *Indices++ = 7;
		*Indices++ = 0; *Indices++ = 7; *Indices++ = 4;

		*Indices++ = 1; *Indices++ = 2; *Indices++ = 6;
		*Indices++ = 1; *Indices++ = 6; *Indices++ = 5;

		*Indices++ = 15; *Indices++ = 14; *Indices++ = 13;
		*Indices++ = 15; *Indices++ = 13; *Indices++ = 12;

		*Indices++ = 8; *Indices++ = 9; *Indices++ = 13;
		*Indices++ = 8; *Indices++ = 13; *Indices++ = 12;

		*Indices++ = 10; *Indices++ = 11; *Indices++ = 15;
		*Indices++ = 10; *Indices++ = 15; *Indices++ = 14;

		*Indices++ = 8; *Indices++ = 11; *Indices++ = 15;
		*Indices++ = 8; *Indices++ = 15; *Indices++ = 12;

		*Indices++ = 9; *Indices++ = 10; *Indices++ = 14;
		*Indices++ = 9; *Indices++ = 14; *Indices++ = 13;

		for (int x = 0; x < StudsX; x++)
		{
			for (int y = 0; y < StudsY; y++)
			{
				int CenterIndex = 16 + (StudSides * 2 + 1) * (x + StudsX * y);
				int BaseIndex = CenterIndex + 1;

				for (int Step = 0; Step < StudSides; Step++)
				{
					*Indices++ = CenterIndex;
					*Indices++ = BaseIndex + Step * 2;
					*Indices++ = BaseIndex + ((Step + 1) % StudSides) * 2;

					*Indices++ = BaseIndex + Step * 2;
					*Indices++ = BaseIndex + Step * 2 + 1;
					*Indices++ = BaseIndex + ((Step + 1) % StudSides) * 2;

					*Indices++ = BaseIndex + ((Step + 1) % StudSides) * 2;
					*Indices++ = BaseIndex + Step * 2 + 1;
					*Indices++ = BaseIndex + ((Step + 1) % StudSides) * 2 + 1;
				}
			}
		}

		*Indices++ = 0; *Indices++ = 1; *Indices++ = 1; *Indices++ = 2;
		*Indices++ = 2; *Indices++ = 3; *Indices++ = 3; *Indices++ = 0;

		*Indices++ = 4; *Indices++ = 5; *Indices++ = 5; *Indices++ = 6;
		*Indices++ = 6; *Indices++ = 7; *Indices++ = 7; *Indices++ = 4;

		*Indices++ = 0; *Indices++ = 4; *Indices++ = 1; *Indices++ = 5;
		*Indices++ = 2; *Indices++ = 6; *Indices++ = 3; *Indices++ = 7;

		*Indices++ = 8; *Indices++ = 9; *Indices++ = 9; *Indices++ = 10;
		*Indices++ = 10; *Indices++ = 11; *Indices++ = 11; *Indices++ = 8;

		*Indices++ = 12; *Indices++ = 13; *Indices++ = 13; *Indices++ = 14;
		*Indices++ = 14; *Indices++ = 15; *Indices++ = 15; *Indices++ = 12;

		*Indices++ = 8; *Indices++ = 12; *Indices++ = 9; *Indices++ = 13;
		*Indices++ = 10; *Indices++ = 14; *Indices++ = 11; *Indices++ = 15;

		for (int x = 0; x < StudsX; x++)
		{
			for (int y = 0; y < StudsY; y++)
			{
				int BaseIndex = 16 + (StudSides * 2 + 1) * (x + StudsX * y) + 1;

				for (int Step = 0; Step < StudSides; Step++)
				{
					*Indices++ = BaseIndex + Step * 2;
					*Indices++ = BaseIndex + ((Step + 1) % StudSides) * 2;

					*Indices++ = BaseIndex + Step * 2 + 1;
					*Indices++ = BaseIndex + ((Step + 1) % StudSides) * 2 + 1;
				}
			}
		}

		const int NumTriangles = ((StudSides * 3) * StudsX * StudsY + 28);

		for (int Triangle = 0; Triangle < NumTriangles; Triangle++)
		{
			float* v1 = VertexBuffer + IndexBuffer[Triangle * 3] * 3;
			float* v2 = VertexBuffer + IndexBuffer[Triangle * 3 + 1] * 3;
			float* v3 = VertexBuffer + IndexBuffer[Triangle * 3 + 2] * 3;
			fprintf(output, " 3 16 %f %f %f %f %f %f %f %f %f\n", v1[0] * 25, -v1[2] * 25, v1[1] * 25, v2[0] * 25, -v2[2] * 25, v2[1] * 25, v3[0] * 25, -v3[2] * 25, v3[1] * 25);
		}

		const int NumLines = ((StudSides * 2) * StudsX * StudsY + 24);

		for (int Line = 0; Line < NumLines; Line++)
		{
			float* v1 = VertexBuffer + IndexBuffer[NumTriangles * 3 + Line * 2] * 3;
			float* v2 = VertexBuffer + IndexBuffer[NumTriangles * 3 + Line * 2 + 1] * 3;
			fprintf(output, " 2 24 %f %f %f %f %f %f\n", v1[0] * 25, -v1[2] * 25, v1[1] * 25, v2[0] * 25, -v2[2] * 25, v2[1] * 25);
		}
	}

	fclose(output);
}

int main(int argc, char* argv[])
{
	const char* Parts[][2] =
	{
		{ "3005",  "Brick  1 x  1" },
		{ "3004",  "Brick  1 x  2" },
		{ "3622",  "Brick  1 x  3" },
		{ "3010",  "Brick  1 x  4" },
		{ "3009",  "Brick  1 x  6" },
		{ "3008",  "Brick  1 x  8" },
		{ "6111",  "Brick  1 x 10" },
		{ "6112",  "Brick  1 x 12" },
		{ "2465",  "Brick  1 x 16" },
		{ "3003",  "Brick  2 x  2" },
		{ "3002",  "Brick  2 x  3" },
		{ "3001",  "Brick  2 x  4" },
		{ "2456",  "Brick  2 x  6" },
		{ "3007",  "Brick  2 x  8" },
		{ "3006",  "Brick  2 x 10" },
		{ "2356",  "Brick  4 x  6" },
		{ "6212",  "Brick  4 x 10" },
		{ "4202",  "Brick  4 x 12" },
		{ "30400", "Brick  4 x 18" },
		{ "4201",  "Brick  8 x  8" },
		{ "4204",  "Brick  8 x 16" },
		{ "733",   "Brick 10 x 10" },
		{ "3024",  "Plate  1 x  1" },
		{ "3023",  "Plate  1 x  2" },
		{ "3623",  "Plate  1 x  3" },
		{ "3710",  "Plate  1 x  4" },
		{ "3666",  "Plate  1 x  6" },
		{ "3460",  "Plate  1 x  8" },
		{ "4477",  "Plate  1 x 10" },
		{ "60479", "Plate  1 x 12" },
		{ "3022",  "Plate  2 x  2" },
		{ "3021",  "Plate  2 x  3" },
		{ "3020",  "Plate  2 x  4" },
		{ "3795",  "Plate  2 x  6" },
		{ "3034",  "Plate  2 x  8" },
		{ "3832",  "Plate  2 x 10" },
		{ "2445",  "Plate  2 x 12" },
		{ "91988", "Plate  2 x 14" },
		{ "4282",  "Plate  2 x 16" },
		{ "3031",  "Plate  4 x  4" },
		{ "3032",  "Plate  4 x  6" },
		{ "3035",  "Plate  4 x  8" },
		{ "3030",  "Plate  4 x 10" },
		{ "3029",  "Plate  4 x 12" },
		{ "3958",  "Plate  6 x  6" },
		{ "3036",  "Plate  6 x  8" },
		{ "3033",  "Plate  6 x 10" },
		{ "3028",  "Plate  6 x 12" },
		{ "3456",  "Plate  6 x 14" },
		{ "3027",  "Plate  6 x 16" },
		{ "3026",  "Plate  6 x 24" },
		{ "41539", "Plate  8 x  8" },
		{ "728",   "Plate  8 x 11" },
		{ "92438", "Plate  8 x 16" },
	};

	for (unsigned int PartIdx = 0; PartIdx < sizeof(Parts) / sizeof(Parts[0]); PartIdx++)
		GeneratePart(Parts[PartIdx][0], Parts[PartIdx][1]);
}


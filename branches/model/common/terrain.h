#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#define LC_TERRAIN_FLAT			0x01	// Flat terrain
#define LC_TERRAIN_TEXTURE		0x02	// Use texture
#define LC_TERRAIN_SMOOTH		0x04	// Smooth shading

class TerrainPatch
{
public:
	TerrainPatch ();
	~TerrainPatch ();

	float control[48]; // 4x4 grid
	unsigned short steps;

	float* vertex;
	float* normals;
	float* coords;
	unsigned short* index;

	bool visible;

	void InitBox(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
	bool BoxIsOutside(const float plane[4]) const;
	void Tesselate(bool bNormals);
	void FreeMemory();

protected:
	float corners[8][3];
};

class Terrain
{
public:
	Terrain();
	~Terrain();
	Terrain& operator=(const Terrain& source);

	void LoadTexture();
	void Render(lcCamera* pCam, float aspect);
	void LoadDefaults(bool bLinear);
	void SetSize(float uSize, float vSize);
	void GetSize(float *uSize, float *vSize);
	void FileLoad(lcFile* file);
	void FileSave(lcFile* file);
	void Tesselate();
	void SetControlPoints();
	void GenerateRandom();

	void SetPatchCount(int uPatches, int vPatches);
	void GetPatchCount(int *uCount, int *vCount);
	int GetCountU()
		{ return m_uPatches != 0 ? m_uPatches*3 + 1 : 0; }
	int GetCountV()
		{ return m_vPatches != 0 ? m_vPatches*3 + 1 : 0; }
	float** GetControlPoints()
		{ return m_pControl; }

	lcuint32 m_nOptions;
	char m_strTexture[LC_MAXPATH];
	float m_fColor[3];

protected:
	void FreeMemory();
	void FindVisiblePatches(lcCamera* pCam, float aspect);

	float** m_pControl;
	TerrainPatch** m_Patches;
	int m_uPatches;
	int m_vPatches;
	float m_uSize;
	float m_vSize;
	lcTexture* m_pTexture;
};

#endif // _TERRAIN_H_

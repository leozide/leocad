#ifndef _MINIFIG_H_
#define _MINIFIG_H_

class PieceInfo;

typedef	enum
{
  LC_MFW_HAT,
  LC_MFW_HEAD,
  LC_MFW_TORSO,
  LC_MFW_NECK,
  LC_MFW_LEFT_ARM,
  LC_MFW_RIGHT_ARM,
  LC_MFW_LEFT_HAND,
  LC_MFW_RIGHT_HAND,
  LC_MFW_LEFT_TOOL,
  LC_MFW_RIGHT_TOOL,
  LC_MFW_HIPS,
  LC_MFW_LEFT_LEG,
  LC_MFW_RIGHT_LEG,
  LC_MFW_LEFT_SHOE,
  LC_MFW_RIGHT_SHOE,
  LC_MFW_NUMITEMS
} LC_MFW_TYPES;

typedef struct
{
  char name[9];
  char description[32];
  int type;
  float x, y, z;
  float rx, ry, rz;
} LC_MFW_PIECEINFO;

class MinifigWizard
{
 public:
  MinifigWizard ();
  ~MinifigWizard ();

  void Redraw ();
  void Resize (int width, int height);
  void Calculate ();
  void GetDescriptions (int type, char ***names, int *count);
  void GetSelections (char **names);
  void ChangePiece (int type, const char *description);
  void ChangeColor (int type, int color);
  void ChangeAngle (int type, float angle);

  void GetMinifigNames (char ***names, int *count);
  void SaveMinifig (const char* name);
  bool LoadMinifig (const char* name);
  void DeleteMinifig (const char* name);

 public:
  PieceInfo* m_Info[LC_MFW_NUMITEMS];
  int m_Colors[LC_MFW_NUMITEMS];
  float m_Angles[LC_MFW_NUMITEMS];
  float	m_Position[LC_MFW_NUMITEMS][3];
  float	m_Rotation[LC_MFW_NUMITEMS][4];

 protected:
  // saved minifig templates
  int  m_MinifigCount;
  char **m_MinifigNames;
  char **m_MinifigTemplates;
};

#endif // _MINIFIG_H_

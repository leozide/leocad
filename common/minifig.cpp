//
// Minifig Wizard base class, calculates position/rotation of all pieces.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "minifig.h"
#include "opengl.h"
#include "pieceinf.h"
#include "globals.h"
#include "project.h"
#include "matrix.h"

// =============================================================================
// Static variables

static LC_MFW_PIECEINFO mfw_pieceinfo[] = {
  { "3624", "Police Hat", LC_MFW_HAT },
  { "3626BP01", "Smiley Face", LC_MFW_HEAD },
  { "973", "Plain Torso", LC_MFW_TORSO },
  { "3838", "Airtanks", LC_MFW_NECK },
  { "976", "Left Arm", LC_MFW_LEFT_ARM },
  { "975", "Right Arm", LC_MFW_RIGHT_ARM },
  { "977", "Hand", LC_MFW_LEFT_HAND },
  { "977", "Hand", LC_MFW_LEFT_HAND },
  { "3899", "Cup", LC_MFW_LEFT_TOOL },
  { "4528", "Frypan", LC_MFW_LEFT_TOOL },
  { "970", "Hips", LC_MFW_HIPS },
  { "972", "Left Leg", LC_MFW_LEFT_LEG }, 
  { "971", "Right Leg", LC_MFW_RIGHT_LEG },
  { "2599", "Flipper", LC_MFW_LEFT_SHOE },
  { "6120", "Ski", LC_MFW_LEFT_SHOE },
  { "4485", "Baseball Cap", LC_MFW_HAT },
  { "3626B", "Plain Face", LC_MFW_HEAD },
  { "3626BP02", "Woman Face", LC_MFW_HEAD },
  { "973P11", "Dungarees", LC_MFW_TORSO },
  { "973P47", "Castle Red/Gray Symbol", LC_MFW_TORSO }, 
  { "973P51", "Blacktron II", LC_MFW_TORSO },
  { "973P01", "Vertical Strips Red/Blue", LC_MFW_TORSO },
  { "973P02", "Vertical Strips Blue/Red", LC_MFW_TORSO }, 
  { "973P60", "Shell Logo", LC_MFW_TORSO },
  { "973P61", "Gold Ice Planet Pattern", LC_MFW_TORSO }, 
  { "4349", "Loudhailer", LC_MFW_LEFT_TOOL },
  { "3962", "Radio", LC_MFW_LEFT_TOOL },
  { "4529", "Saucepan", LC_MFW_LEFT_TOOL },
  { "3959", "Space Gun", LC_MFW_LEFT_TOOL },
  { "4360", "Space Laser Gun", LC_MFW_LEFT_TOOL },
  { "4479", "Metal Detector", LC_MFW_LEFT_TOOL },
  { "6246A", "Screwdriver", LC_MFW_LEFT_TOOL },
  { "6246B", "Hammer", LC_MFW_LEFT_TOOL },
  { "6246D", "Box Wrench", LC_MFW_LEFT_TOOL },
  { "6246E", "Open End Wrench", LC_MFW_LEFT_TOOL },
  { "3896", "Castle Helmet with Chin-Guard", LC_MFW_HAT },
  { "3844", "Castle Helmet with Neck Protect", LC_MFW_HAT },
  { "3833", "Construction Helmet", LC_MFW_HAT }, 
  { "82359", "Skeleton Skull", LC_MFW_HEAD },
  { "973P14", "'S' Logo", LC_MFW_TORSO },
  { "973P16", "Airplane Logo", LC_MFW_TORSO }, 
  { "973P52", "Blacktron I Pattern", LC_MFW_TORSO },
  { "973P15", "Horizontal Stripes", LC_MFW_TORSO },
  { "973P68", "Mtron Logo", LC_MFW_TORSO }, 
  { "973P17", "Red V-Neck and Buttons", LC_MFW_TORSO },
  { "973P63", "Robot Pattern", LC_MFW_TORSO },
  { "973P18", "Suit and Tie ", LC_MFW_TORSO }, 
  { "4736", "Jet-Pack with Stud On Front", LC_MFW_NECK },
  { "4522", "Mallet", LC_MFW_LEFT_TOOL },
  { "6246C", "Power Drill", LC_MFW_LEFT_TOOL },
  { "4006", "Spanner/Screwdriver", LC_MFW_LEFT_TOOL },
  { "194", "Hose Nozzle", LC_MFW_LEFT_TOOL },
  { "2446", "Helmet", LC_MFW_HAT },
  { "3840", "Vest", LC_MFW_NECK },
  { "970P63", "Hips with Robot Pattern", LC_MFW_HIPS },
  { "972P63", "Left Leg with Robot Pattern", LC_MFW_LEFT_LEG },
  { "971P63", "Right Leg with Robot Pattern", LC_MFW_RIGHT_LEG },
  { "2524", "Backpack Non-Opening", LC_MFW_NECK },
  { "4497", "Spear", LC_MFW_LEFT_TOOL },
  { "37", "Knife", LC_MFW_LEFT_TOOL },
  { "38", "Harpoon", LC_MFW_LEFT_TOOL },
  { "3626BP03", "Pointed Moustache", LC_MFW_HEAD },
  { "3626BP04", "Sunglasses", LC_MFW_HEAD },
  { "3626BP05", "Grin and Eyebrows", LC_MFW_HEAD }, 
  { "973P19", "Train Chevron", LC_MFW_TORSO },
  { "973P31", "Pirate Strips (Red/Cream)", LC_MFW_TORSO },
  { "973P32", "Pirate Strips (Blue/Cream)", LC_MFW_TORSO },
  { "973P33", "Pirate Strips (Red/Black)", LC_MFW_TORSO },
  { "973P41", "Castle Chainmail", LC_MFW_TORSO },
  { "973P62", "Silver Ice Planet", LC_MFW_TORSO },
  { "6131", "Wizard Hat", LC_MFW_HAT },
  { "973P20", "Waiter", LC_MFW_TORSO },
  { "973P49", "Forestman Blue Collar", LC_MFW_TORSO },
  { "973P48", "Forestman Maroon Collar", LC_MFW_TORSO },
  { "973P50", "Forestman Black Collar", LC_MFW_TORSO },
  { "3841", "Pickaxe", LC_MFW_LEFT_TOOL },
  { "973P21", "Five Button Fire Fighter", LC_MFW_TORSO },
  { "973P22", "Red Shirt and Suit", LC_MFW_TORSO },
  { "973P34", "Open Jacket over Striped Vest", LC_MFW_TORSO },
  { "973P46", "Forestman and Purse", LC_MFW_TORSO },
  { "973P101", "SW Rebel Pilot", LC_MFW_TORSO },
  { "4498", "Arrow Quiver", LC_MFW_NECK },
  { "4499", "Bow with Arrow", LC_MFW_LEFT_TOOL },
  { "3852", "Hairbrush", LC_MFW_LEFT_TOOL },
  { "30152", "Magnifying Glass", LC_MFW_LEFT_TOOL },
  { "973P23", "'S' Logo Yellow / Blue Pattern", LC_MFW_TORSO },
  { "973P42", "Castle Crossed Pikes Pattern", LC_MFW_TORSO },
  { "973P65", "Futuron Pattern", LC_MFW_TORSO },
  { "973P25", "Red Cross & Stethoscope Pattern", LC_MFW_TORSO },
  { "973P24", "Red Cross Pattern", LC_MFW_TORSO },
  { "973P64", "Unitron Pattern", LC_MFW_TORSO },
  { "4524", "Cape", LC_MFW_NECK },
  { "2570", "Crossbow", LC_MFW_LEFT_TOOL },
  { "3834", "Fire Helmet", LC_MFW_HAT },
  { "2614", "Fishing Rod", LC_MFW_LEFT_TOOL }
//	{ "770", "Shield Ovoid", LC_MFW_LEFT_TOOL }, 
//	2447      Minifig Helmet Visor
};

static int mfw_pieces = sizeof (mfw_pieceinfo)/sizeof (LC_MFW_PIECEINFO);

// =============================================================================
// MinifigWizard class

MinifigWizard::MinifigWizard ()
{
  const unsigned char colors[15] = { 0, 6, 4, 22, 0, 0, 6, 6, 22, 22, 9, 9, 9, 22, 22 };
  const float pos[15][3] = { {0,0,3.84f}, {0,0,3.84f}, {0,0,2.88f}, {0,0,2.96f}, {0,0,2.56f},
			     {0,0,2.56f}, {0.9f,-0.62f,1.76f}, {-0.9f,-0.62f,1.76f}, {0.92f,-0.62f,1.76f},
			     {-0.92f,-0.62f,1.76f}, {0,0,1.6f}, {0,0,1.12f}, {0,0,1.12f}, {0.42f,0,0},
			     {-0.42f,0,0} };
  int i;

  for (i = 0; i < 15; i++)
  {
    m_Info[i] = NULL;
    m_Colors[i] = colors[i];
    m_Pos[i][0] = pos[i][0];
    m_Pos[i][1] = pos[i][1];
    m_Pos[i][2] = pos[i][2];
    m_Rot[i][0] = 0;
    m_Rot[i][1] = 0;
    m_Rot[i][2] = 0;
  }

  for (i = 0; i < 13; i++)
  {
    if (i == 3 || i == 7 || i == 8 || i == 9)
      continue;

    PieceInfo* pInfo = project->FindPieceInfo (mfw_pieceinfo[i].name);
    if (pInfo == NULL)
      continue;

    if (i == 6)
    {
      m_Info[6] = pInfo;
      m_Info[7] = pInfo;
      pInfo->AddRef();
      pInfo->AddRef();
      m_Rot[6][0] = 45;
      m_Rot[6][2] = 90;
      m_Rot[7][0] = 45;
      m_Rot[7][2] = 90;
    }
    else
    {
      m_Info[i] = pInfo;
      pInfo->AddRef();
    }
  }
}

MinifigWizard::~MinifigWizard ()
{
}

void MinifigWizard::Resize (int width, int height)
{
  float aspect = (float)width/(float)height;
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(30.0f, aspect, 1.0f, 20.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
	
  gluLookAt (0, -9, 4, 0, 5, 1, 0, 0, 1);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  float *bg = project->GetBackgroundColor();
  glClearColor(bg[0], bg[1], bg[2], bg[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable (GL_DITHER);
  glShadeModel (GL_FLAT);
}

void MinifigWizard::Redraw ()
{
  int i;

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  Calculate ();

  for (i = 0; i < LC_MFW_NUMITEMS; i++)
  {
    if (m_Info[i] == NULL)
      continue;

    glPushMatrix ();
    glTranslatef (m_Position[i][0], m_Position[i][1], m_Position[i][2]);
    glRotatef (m_Rotation[i][3], m_Rotation[i][0], m_Rotation[i][1], m_Rotation[i][2]);
    m_Info[i]->RenderPiece(m_Colors[i]);
    glPopMatrix ();
  }

  glFinish();
}

void MinifigWizard::Calculate ()
{
  Matrix mat, m2, m3;

  // hat
  m_Position[LC_MFW_HAT][0] = m_Pos[LC_MFW_HAT][0];
  m_Position[LC_MFW_HAT][1] = m_Pos[LC_MFW_HAT][1];
  m_Position[LC_MFW_HAT][2] = m_Pos[LC_MFW_HAT][2];
  m_Rotation[LC_MFW_HAT][0] = 0.0f;
  m_Rotation[LC_MFW_HAT][1] = 0.0f;
  m_Rotation[LC_MFW_HAT][2] = -1.0f;
  m_Rotation[LC_MFW_HAT][3] = m_Angles[LC_MFW_HAT] + m_Angles[LC_MFW_HEAD];

  // head
  m_Position[LC_MFW_HEAD][0] = m_Pos[LC_MFW_HEAD][0];
  m_Position[LC_MFW_HEAD][1] = m_Pos[LC_MFW_HEAD][1];
  m_Position[LC_MFW_HEAD][2] = m_Pos[LC_MFW_HEAD][2];
  m_Rotation[LC_MFW_HEAD][0] = 0.0f;
  m_Rotation[LC_MFW_HEAD][1] = 0.0f;
  m_Rotation[LC_MFW_HEAD][2] = -1.0f;
  m_Rotation[LC_MFW_HEAD][3] = m_Angles[LC_MFW_HEAD];

  // neck
  mat.LoadIdentity ();
  mat.CreateOld (0,0,0,m_Rot[LC_MFW_NECK][0], m_Rot[LC_MFW_NECK][1], m_Rot[LC_MFW_NECK][2]);
  mat.Rotate (m_Angles[LC_MFW_NECK], 0, 0, -1);
  mat.SetTranslation (m_Pos[LC_MFW_NECK][0], m_Pos[LC_MFW_NECK][1],
		      m_Pos[LC_MFW_NECK][2]);
  mat.ToAxisAngle (m_Rotation[LC_MFW_NECK]);
  mat.GetTranslation (m_Position[LC_MFW_NECK]);

  // torso
  m_Position[LC_MFW_TORSO][0] = m_Pos[LC_MFW_TORSO][0];
  m_Position[LC_MFW_TORSO][1] = m_Pos[LC_MFW_TORSO][1];
  m_Position[LC_MFW_TORSO][2] = m_Pos[LC_MFW_TORSO][2];
  m_Rotation[LC_MFW_TORSO][0] = 0.0f;
  m_Rotation[LC_MFW_TORSO][1] = 0.0f;
  m_Rotation[LC_MFW_TORSO][2] = 1.0f;
  m_Rotation[LC_MFW_TORSO][3] = 0.0f;

  // left arm/hand/tool
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_LEFT_ARM], -1, 0, 0);
  mat.SetTranslation (m_Pos[LC_MFW_LEFT_ARM][0], m_Pos[LC_MFW_LEFT_ARM][1],
		      m_Pos[LC_MFW_LEFT_ARM][2]);
  mat.ToAxisAngle (m_Rotation[LC_MFW_LEFT_ARM]);
  mat.GetTranslation (m_Position[LC_MFW_LEFT_ARM]);

  mat.Translate (m_Pos[LC_MFW_LEFT_HAND][0]-m_Pos[LC_MFW_LEFT_ARM][0],
		 m_Pos[LC_MFW_LEFT_HAND][1]-m_Pos[LC_MFW_LEFT_ARM][1],
		 m_Pos[LC_MFW_LEFT_HAND][2]-m_Pos[LC_MFW_LEFT_ARM][2]);
  m2.CreateOld (0,0,0,m_Rot[LC_MFW_LEFT_HAND][0],m_Rot[LC_MFW_LEFT_HAND][1],m_Rot[LC_MFW_LEFT_HAND][2]);
  m3.Multiply (mat, m2);
  m3.Translate (0,0,-0.16f);
  mat.LoadIdentity ();
  mat.Translate (0,0,0.16f);
  mat.Rotate (m_Angles[LC_MFW_LEFT_HAND], 1, 0, 0);
  m2.Multiply (m3, mat);
  m2.ToAxisAngle (m_Rotation[LC_MFW_LEFT_HAND]);
  m2.GetTranslation (m_Position[LC_MFW_LEFT_HAND]);

  m2.Translate (m_Pos[LC_MFW_LEFT_TOOL][0]-m_Pos[LC_MFW_LEFT_ARM][0],
		m_Pos[LC_MFW_LEFT_TOOL][1]-m_Pos[LC_MFW_LEFT_ARM][1],
		m_Pos[LC_MFW_LEFT_TOOL][2]-m_Pos[LC_MFW_LEFT_ARM][2]);
  m3.CreateOld (0,0,0,m_Rot[LC_MFW_LEFT_TOOL][0]-m_Rot[LC_MFW_LEFT_HAND][0],
		m_Rot[LC_MFW_LEFT_TOOL][1]-m_Rot[LC_MFW_LEFT_HAND][1],
		m_Rot[LC_MFW_LEFT_TOOL][2]-m_Rot[LC_MFW_LEFT_HAND][2]);
  mat.Multiply (m2, m3);
  m2.LoadIdentity ();
  m2.Rotate (m_Angles[LC_MFW_LEFT_TOOL], 0, 0, 1);
  m3.Multiply (mat, m2);
  m3.ToAxisAngle (m_Rotation[LC_MFW_LEFT_TOOL]);
  m3.GetTranslation (m_Position[LC_MFW_LEFT_TOOL]);

  // right arm/hand/tool
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_RIGHT_ARM], -1, 0, 0);
  mat.SetTranslation (m_Pos[LC_MFW_RIGHT_ARM][0], m_Pos[LC_MFW_RIGHT_ARM][1],
		      m_Pos[LC_MFW_RIGHT_ARM][2]);
  mat.ToAxisAngle (m_Rotation[LC_MFW_RIGHT_ARM]);
  mat.GetTranslation (m_Position[LC_MFW_RIGHT_ARM]);

  mat.Translate (m_Pos[LC_MFW_RIGHT_HAND][0]-m_Pos[LC_MFW_RIGHT_ARM][0],
		 m_Pos[LC_MFW_RIGHT_HAND][1]-m_Pos[LC_MFW_RIGHT_ARM][1],
		 m_Pos[LC_MFW_RIGHT_HAND][2]-m_Pos[LC_MFW_RIGHT_ARM][2]);
  m2.CreateOld (0,0,0,m_Rot[LC_MFW_RIGHT_HAND][0],m_Rot[LC_MFW_RIGHT_HAND][1],m_Rot[LC_MFW_RIGHT_HAND][2]);
  m3.Multiply (mat, m2);
  m3.Translate (0,0,-0.16f);
  mat.LoadIdentity ();
  mat.Translate (0,0,0.16f);
  mat.Rotate (m_Angles[LC_MFW_RIGHT_HAND], 1, 0, 0);
  m2.Multiply (m3, mat);
  m2.ToAxisAngle (m_Rotation[LC_MFW_RIGHT_HAND]);
  m2.GetTranslation (m_Position[LC_MFW_RIGHT_HAND]);

  // hips
  m_Position[LC_MFW_HIPS][0] = m_Pos[LC_MFW_HIPS][0];
  m_Position[LC_MFW_HIPS][1] = m_Pos[LC_MFW_HIPS][1];
  m_Position[LC_MFW_HIPS][2] = m_Pos[LC_MFW_HIPS][2];
  m_Rotation[LC_MFW_HIPS][0] = 0.0f;
  m_Rotation[LC_MFW_HIPS][1] = 0.0f;
  m_Rotation[LC_MFW_HIPS][2] = 1.0f;
  m_Rotation[LC_MFW_HIPS][3] = 0.0f;

  // left leg/shoe
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_LEFT_LEG], -1, 0, 0);
  mat.SetTranslation (m_Pos[LC_MFW_LEFT_LEG][0], m_Pos[LC_MFW_LEFT_LEG][1],
		      m_Pos[LC_MFW_LEFT_LEG][2]);
  mat.ToAxisAngle (m_Rotation[LC_MFW_LEFT_LEG]);
  mat.GetTranslation (m_Position[LC_MFW_LEFT_LEG]);
  mat.Translate (m_Pos[LC_MFW_LEFT_SHOE][0]-m_Pos[LC_MFW_LEFT_LEG][0],
		 m_Pos[LC_MFW_LEFT_SHOE][1]-m_Pos[LC_MFW_LEFT_LEG][1],
		 m_Pos[LC_MFW_LEFT_SHOE][2]-m_Pos[LC_MFW_LEFT_LEG][2]);
  m2.CreateOld (0,0,0,m_Rot[LC_MFW_LEFT_SHOE][0],m_Rot[LC_MFW_LEFT_SHOE][1],m_Rot[LC_MFW_LEFT_SHOE][2]);
  m3.Multiply (mat, m2);
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_LEFT_SHOE], 0, 0, 1);
  m2.Multiply (m3, mat);
  m2.ToAxisAngle (m_Rotation[LC_MFW_LEFT_SHOE]);
  m2.GetTranslation (m_Position[LC_MFW_LEFT_SHOE]);

  // right leg/shoe
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_RIGHT_LEG], -1, 0, 0);
  mat.SetTranslation (m_Pos[LC_MFW_RIGHT_LEG][0], m_Pos[LC_MFW_RIGHT_LEG][1],
		      m_Pos[LC_MFW_RIGHT_LEG][2]);
  mat.ToAxisAngle (m_Rotation[LC_MFW_RIGHT_LEG]);
  mat.GetTranslation (m_Position[LC_MFW_RIGHT_LEG]);
  mat.Translate (m_Pos[LC_MFW_RIGHT_SHOE][0]-m_Pos[LC_MFW_RIGHT_LEG][0],
		 m_Pos[LC_MFW_RIGHT_SHOE][1]-m_Pos[LC_MFW_RIGHT_LEG][1],
		 m_Pos[LC_MFW_RIGHT_SHOE][2]-m_Pos[LC_MFW_RIGHT_LEG][2]);
  m2.CreateOld (0,0,0,m_Rot[LC_MFW_RIGHT_SHOE][0],m_Rot[LC_MFW_RIGHT_SHOE][1],m_Rot[LC_MFW_RIGHT_SHOE][2]);
  m3.Multiply (mat, m2);
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_RIGHT_SHOE], 0, 0, 1);
  m2.Multiply (m3, mat);
  m2.ToAxisAngle (m_Rotation[LC_MFW_RIGHT_SHOE]);
  m2.GetTranslation (m_Position[LC_MFW_RIGHT_SHOE]);
}

void MinifigWizard::GetDescriptions (int type, char ***names, int *count)
{
  char **list = (char**)malloc (sizeof (char*)*mfw_pieces);
  *names = list;
  *count = 0;
  int i, j;

  for (i = 0; i < mfw_pieces; i++)
  {
    PieceInfo* piece_info;

    piece_info = project->FindPieceInfo (mfw_pieceinfo[i].name);
    if (piece_info == NULL)
      continue;

    switch (type)
    {
    case LC_MFW_HAT:
    case LC_MFW_HEAD:
    case LC_MFW_TORSO:
    case LC_MFW_NECK:
    case LC_MFW_LEFT_ARM:
    case LC_MFW_RIGHT_ARM:
    case LC_MFW_HIPS:
    case LC_MFW_LEFT_LEG:
    case LC_MFW_RIGHT_LEG:
    case LC_MFW_LEFT_HAND:
    case LC_MFW_LEFT_TOOL:
    case LC_MFW_LEFT_SHOE:
      if (mfw_pieceinfo[i].type != type)
	continue;
      break;

    case LC_MFW_RIGHT_HAND:
      if (mfw_pieceinfo[i].type != LC_MFW_LEFT_HAND)
	continue;
      break;

    case LC_MFW_RIGHT_TOOL:
      if (mfw_pieceinfo[i].type != LC_MFW_LEFT_TOOL)
	continue;
      break;

    case LC_MFW_RIGHT_SHOE:
      if (mfw_pieceinfo[i].type != LC_MFW_LEFT_SHOE)
	continue;
      break;

    default:
      continue;
    }

    list[(*count)++] = mfw_pieceinfo[i].description;

    if (i == 6) i++; // two hands are listed
  }

  // ugly sort
  for (i = 0; i < (*count) - 1; i++)
    for (j = 0; j < (*count) - 1; j++)
    {
      if (strcmp (list[j], list[j+1]) > 0)
      {
	char *tmp = list[j];
	list[j] = list[j+1];
	list[j+1] = tmp;
      }
    }

  if ((type == LC_MFW_HAT) || (type == LC_MFW_NECK) ||
      (type == LC_MFW_LEFT_TOOL) || (type == LC_MFW_RIGHT_TOOL) ||
      (type == LC_MFW_LEFT_SHOE) || (type == LC_MFW_RIGHT_SHOE))
  {
    memmove (list+1, list, *count*sizeof (char*));
    list[0] = "None";
    (*count)++;
  }
}

void MinifigWizard::ChangePiece (int type, const char *desc)
{
  PieceInfo* piece_info = NULL;
  int j;

  for (j = 0; j < mfw_pieces; j++)
  {
    if (strcmp (desc, mfw_pieceinfo[j].description) == 0)
    {
      piece_info = project->FindPieceInfo (mfw_pieceinfo[j].name);
      if (piece_info == NULL)
	continue;

      if (m_Info[type])
	m_Info[type]->DeRef();
      m_Info[type] = piece_info;
      piece_info->AddRef();
      break;
    }
  }

  // Piece not found ("None")
  if (j == mfw_pieces)
  {
    if (m_Info[type])
      m_Info[type]->DeRef();
    m_Info[type] = NULL;
  }

  // Get the pieces in the right place
  if (type == LC_MFW_NECK)
  {
    if (m_Info[3] != NULL)
    {
      m_Pos[0][2] = 3.92f;
      m_Pos[1][2] = 3.92f;
 
      if (strcmp (piece_info->m_strName,"4498") == 0)
	m_Rot[3][2] = 180.0f;
      else
	m_Rot[3][2] = 0.0f;
    }
    else
    {
      m_Pos[0][2] = 3.84f;
      m_Pos[1][2] = 3.84f;
    }
  }

  if (type == LC_MFW_LEFT_SHOE)
  {
    if (strcmp (desc, "Ski"))
      m_Pos[13][1] = 0;
    else
      m_Pos[13][1] = -0.12f;
  }

  if (type == LC_MFW_RIGHT_SHOE)
  {
    if (strcmp (desc, "Ski"))
      m_Pos[14][1] = 0;
    else
      m_Pos[14][1] = -0.12f;
  }

  if ((type == LC_MFW_LEFT_TOOL) || (type == LC_MFW_RIGHT_TOOL))
    if (piece_info != NULL)
    {
      float rx = 45, ry = 0, rz = 0, x = 0.92f, y = -0.62f, z = 1.76f;

      if (strcmp (piece_info->m_strName,"4529") == 0)
	{ rx = -45; y = -1.14f; z = 2.36f; }
      if (strcmp (piece_info->m_strName,"3899") == 0)
	{ y = -1.64f; z = 1.38f; }
      if (strcmp (piece_info->m_strName,"4528") == 0)
	{ rx = -45; y = -1.26f; z = 2.36f; }
      if (strcmp (piece_info->m_strName,"4479") == 0)
	{ rz = 90; y = -1.22f; z = 2.44f; }
      if (strcmp (piece_info->m_strName,"3962") == 0)
	{ rz = 90; y = -0.7f; z = 1.62f; }
      if (strcmp (piece_info->m_strName,"4360") == 0)
	{ rz = -90; y = -1.22f; z = 2.44f; }
      if (strncmp (piece_info->m_strName,"6246",4) == 0)
	{ y = -1.82f; z = 2.72f; rz = 90; }
      if (strcmp (piece_info->m_strName,"4349") == 0)
	{ y = -1.16f; z = 2.0f; }
      if (strcmp (piece_info->m_strName,"4479") == 0)
	{ y = -1.42f; z = 2.26f; }
      if (strcmp (piece_info->m_strName,"3959") == 0)
	{ y = -1.0f; z = 1.88f; }
      if (strcmp (piece_info->m_strName,"4522") == 0)
	{ y = -1.64f; z = 2.48f; }
      if (strcmp (piece_info->m_strName,"194") == 0)
	{ rz = 180; y = -1.04f; z = 1.94f; }
      if (strcmp (piece_info->m_strName,"4006") == 0)
	{ rz = 180; y = -1.24f; z = 2.18f; }
      if (strcmp (piece_info->m_strName,"6246C") == 0)
	{ rx = 45; rz = 0; y = -0.86f; z = 1.78f; }
      if (strcmp (piece_info->m_strName,"4497") == 0)
	{ y = -2.16f; z = 3.08f; rz = 90; }
      if (strcmp (piece_info->m_strName,"30092") == 0)
	{ x = 0; rz = 180; }
      if (strcmp (piece_info->m_strName,"37") == 0)
	{ z = 1.52f; y = -0.64f; }
      if (strcmp (piece_info->m_strName,"38") == 0)
	{ z = 1.24f; y = -0.34f; }
      if (strcmp (piece_info->m_strName,"3841") == 0)
	{ z = 2.24f; y = -1.34f; rz = 180; }
      if (strcmp (piece_info->m_strName,"4499") == 0)
	{ rz = ((type == LC_MFW_RIGHT_TOOL) ? 10.0f : -10.0f); z = 1.52f; }
      if (strcmp (piece_info->m_strName,"3852") == 0)
	{ rz = -90; x = 0.90f; y = -0.8f; z = 1.84f; }
      if (strcmp (piece_info->m_strName,"30152") == 0)
	{ z = 3.06f; y = -2.16f; }                                      
      if (strcmp (piece_info->m_strName,"2570") == 0)
	{ z = 1.68f; y = -0.8f; }
      if (strcmp (piece_info->m_strName,"2614") == 0)
	{ z = 1.74f; y = -0.86f; }

      if (type == LC_MFW_RIGHT_TOOL)
	x = -x;

      m_Pos[type][0] = x;
      m_Pos[type][1] = y;
      m_Pos[type][2] = z;
      m_Rot[type][0] = rx;
      m_Rot[type][1] = ry;
      m_Rot[type][2] = rz;
    }
}

void MinifigWizard::ChangeColor (int type, int color)
{
  m_Colors[type] = color;
}

void MinifigWizard::ChangeAngle (int type, float angle)
{
  m_Angles[type] = angle;
}

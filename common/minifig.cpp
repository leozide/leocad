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
#include "system.h"
#include "matrix.h"

// =============================================================================
// Static variables

static LC_MFW_PIECEINFO mfw_pieceinfo[] = {
  { "2446", "Helmet", LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3624", "Police Hat", LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3833", "Construction Helmet", LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3834", "Fire Helmet", LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3844", "Castle Helmet with Neck Protect", LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3896", "Castle Helmet with Chin-Guard", LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "4485", "Baseball Cap", LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "6131", "Wizard Hat", LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3626B", "Plain Face", LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3626BP01", "Smiley Face", LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3626BP02", "Woman Face", LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3626BP03", "Pointed Moustache", LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3626BP04", "Sunglasses", LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "3626BP05", "Grin and Eyebrows", LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f }, 
  { "82359", "Skeleton Skull", LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
  { "973", "Plain Torso", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P01", "Vertical Strips Red/Blue", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P02", "Vertical Strips Blue/Red", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f }, 
  { "973P11", "Dungarees", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P14", "'S' Logo", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P15", "Horizontal Stripes", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P16", "Airplane Logo", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P17", "Red V-Neck and Buttons", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P18", "Suit and Tie ", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P19", "Train Chevron", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P20", "Waiter", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P21", "Five Button Fire Fighter", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P22", "Red Shirt and Suit", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P23", "'S' Logo Yellow / Blue Pattern", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P24", "Red Cross Pattern", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P25", "Red Cross & Stethoscope Pattern", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P31", "Pirate Strips (Red/Cream)", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P32", "Pirate Strips (Blue/Cream)", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P33", "Pirate Strips (Red/Black)", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P34", "Open Jacket over Striped Vest", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P41", "Castle Chainmail", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P42", "Castle Crossed Pikes Pattern", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P46", "Forestman and Purse", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P47", "Castle Red/Gray Symbol", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f }, 
  { "973P48", "Forestman Maroon Collar", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P49", "Forestman Blue Collar", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P50", "Forestman Black Collar", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P51", "Blacktron II", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P52", "Blacktron I Pattern", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P60", "Shell Logo", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P61", "Gold Ice Planet Pattern", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P62", "Silver Ice Planet", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P63", "Robot Pattern", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P64", "Unitron Pattern", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P65", "Futuron Pattern", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "973P68", "Mtron Logo", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f }, 
  { "973P101", "SW Rebel Pilot", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
  { "2524", "Backpack Non-Opening", LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
  { "3838", "Airtanks", LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
  { "3840", "Vest", LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
  { "4498", "Arrow Quiver", LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 180.0f },
  { "4524", "Cape", LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
  { "4736", "Jet-Pack with Stud On Front", LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
  { "976", "Left Arm", LC_MFW_LEFT_ARM, 0.0f, 0.0f, 2.56f, 0.0f, 0.0f, 0.0f },
  { "975", "Right Arm", LC_MFW_RIGHT_ARM, 0.0f, 0.0f, 2.56f, 0.0f, 0.0f, 0.0f },
  { "977", "Hand", LC_MFW_LEFT_HAND, 0.9f, -0.62f, 1.76f, 45.0f, 0.0f, 90.0f },
  { "37", "Knife", LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 1.58f, 45.0f, 0.0f, 0.0f },
  { "38", "Harpoon", LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 1.0f, 45.0f, 0.0f, 0.0f },
  { "194", "Hose Nozzle", LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.22f, 45.0f, 0.0f, 180.0f },
  { "2570", "Crossbow", LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 1.82f, 45.0f, 0.0f, 0.0f },
  { "2614", "Fishing Rod", LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.74f, 45.0f, 0.0f, 0.0f },
  { "3841", "Pickaxe", LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 2.24f, 45.0f, 0.0f, 180.0f },
  { "3852", "Hairbrush", LC_MFW_LEFT_TOOL, 0.82f, -0.64f, 1.98f, 45.0f, 0.0f, -90.0f },
  { "3899", "Cup", LC_MFW_LEFT_TOOL, -0.06f, -0.62f, 2.16f, 45.0f, 0.0f, 0.0f },
  { "3959", "Space Gun", LC_MFW_LEFT_TOOL, 0.74f, -0.62f, 2.1f, 45.0f, 0.0f, 0.0f },
  { "3962", "Radio", LC_MFW_LEFT_TOOL, 0.72f, -0.66f, 1.62f, 45.0f, 0.0f, 90.0f },
  { "4006", "Spanner/Screwdriver", LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 180.0f },
  { "4349", "Loudhailer", LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 2.28f, 45.0f, 0.0f, 0.0f },
  { "4360", "Space Laser Gun", LC_MFW_LEFT_TOOL, 0.96f, -0.62f, 2.64f, 45.0f, 0.0f, -90.0f },
  { "4479", "Metal Detector", LC_MFW_LEFT_TOOL, 0.74f, -0.64f, 2.64f, 45.0f, 0.0f, 90.0f },
  { "4497", "Spear", LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 3.48f, 45.0f, 0.0f, 90.0f },
  { "4499", "Bow with Arrow", LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.52f, 45.0f, 0.0f, -10.0f },
  { "4522", "Mallet", LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 2.72f, 45.0f, 0.0f, 0.0f },
  { "4528", "Frypan", LC_MFW_LEFT_TOOL, 0.90f, -0.62f, 2.64f, -45.0f, 90.0f, 90.0f },
  { "4529", "Saucepan", LC_MFW_LEFT_TOOL, 0.96f, -0.62f, 2.56f, -45.0f, 90.0f, 90.0f },
  { "6246A", "Screwdriver", LC_MFW_LEFT_TOOL, 0.72f, -0.61f, 3.12f, 45.0f, 0.0f, 90.0f },
  { "6246B", "Hammer", LC_MFW_LEFT_TOOL, 0.72f, -0.61f, 3.12f, 45.0f, 0.0f, 90.0f },
  { "6246D", "Box Wrench", LC_MFW_LEFT_TOOL, 0.72f, -0.61f, 3.12f, 45.0f, 0.0f, 90.0f },
  { "6246E", "Open End Wrench", LC_MFW_LEFT_TOOL, 0.72f, -0.61f, 3.12f, 45.0f, 0.0f, 90.0f },
  { "6246C", "Power Drill", LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.96f, 45.0f, 0.0f, 0.0f },
  { "30152", "Magnifying Glass", LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 3.76f, 45.0f, 0.0f, 0.0f },
  { "970", "Hips", LC_MFW_HIPS, 0.0f, 0.0f, 1.6f, 0.0f, 0.0f, 0.0f },
  { "970P63", "Hips with Robot Pattern", LC_MFW_HIPS, 0.0f, 0.0f, 1.6f, 0.0f, 0.0f, 0.0f },
  { "972", "Left Leg", LC_MFW_LEFT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f }, 
  { "972P63", "Left Leg with Robot Pattern", LC_MFW_LEFT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
  { "971", "Right Leg", LC_MFW_RIGHT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
  { "971P63", "Right Leg with Robot Pattern", LC_MFW_RIGHT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
  { "2599", "Flipper", LC_MFW_LEFT_SHOE, 0.42f, -0.12f, 0.0f, 0.0f, 0.0f, 0.0f },
  { "6120", "Ski", LC_MFW_LEFT_SHOE, 0.42f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
//	{ "770", "Shield Ovoid", LC_MFW_LEFT_TOOL },
//	2447      Minifig Helmet Visor
};

static int mfw_pieces = sizeof (mfw_pieceinfo)/sizeof (LC_MFW_PIECEINFO);

// =============================================================================
// MinifigWizard class

MinifigWizard::MinifigWizard ()
{
  const unsigned char colors[LC_MFW_NUMITEMS] = { 0, 6, 4, 22, 0, 0, 6, 6, 22, 22, 9, 9, 9, 22, 22 };
  const char *pieces[LC_MFW_NUMITEMS] = { "3624", "3626BP01", "973", "None", "976", "975", "977", "977",
					  "None", "None", "970", "972", "971", "None", "None" };
  int i;

  for (i = 0; i < LC_MFW_NUMITEMS; i++)
  {
    m_Colors[i] = colors[i];
    m_Angles[i] = 0;

    m_Info[i] = project->FindPieceInfo (pieces[i]);
    if (m_Info[i] != NULL)
      m_Info[i]->AddRef();
  }

  m_MinifigCount = 0;
  m_MinifigNames = NULL;
  m_MinifigTemplates = NULL;

  i = Sys_ProfileLoadInt ("MinifigWizard", "Version", 1);
  if (i == 1)
  {
    char *ptr, buf[32];

    m_MinifigCount = Sys_ProfileLoadInt ("MinifigWizard", "Count", 0);
    m_MinifigNames = (char**)realloc (m_MinifigNames, sizeof (char**)*m_MinifigCount);
    m_MinifigTemplates = (char**)realloc (m_MinifigTemplates, sizeof (char**)*m_MinifigCount);

    for (i = 0; i < m_MinifigCount; i++)
    {
      sprintf (buf, "Minifig%.2dName", i);
      ptr = Sys_ProfileLoadString ("MinifigWizard", buf, buf);
      m_MinifigNames[i] = (char*)malloc (strlen (ptr) + 1);
      strcpy (m_MinifigNames[i], ptr);

      m_MinifigTemplates[i] = (char*)malloc (768);
      sprintf (buf, "Minifig%.2dColors", i);
      ptr = Sys_ProfileLoadString ("MinifigWizard", buf, "");
      if (ptr[strlen (ptr) - 1] != ' ')
	strcat (ptr, " ");
      strcpy (m_MinifigTemplates[i], ptr);

      sprintf (buf, "Minifig%.2dPieces", i);
      ptr = Sys_ProfileLoadString ("MinifigWizard", buf, "");
      if (ptr[strlen (ptr) - 1] != ' ')
	strcat (ptr, " ");
      strcat (m_MinifigTemplates[i], ptr);

      sprintf (buf, "Minifig%.2dAngles", i);
      ptr = Sys_ProfileLoadString ("MinifigWizard", buf, "");
      strcat (m_MinifigTemplates[i], ptr);
    }
  }
  else
    Sys_MessageBox ("Unknown Minifig Preferences.");
}

MinifigWizard::~MinifigWizard ()
{
  char *ptr, buf[32];
  int i, j;

  Sys_ProfileSaveInt ("MinifigWizard", "Version", 1);
  Sys_ProfileSaveInt ("MinifigWizard", "Count", m_MinifigCount);

  for (i = 0; i < m_MinifigCount; i++)
  {
    char *value;
    ptr = m_MinifigTemplates[i];

    sprintf (buf, "Minifig%.2dName", i);
    Sys_ProfileSaveString ("MinifigWizard", buf, m_MinifigNames[i]);

    value = ptr;
    for (j = 0; j < LC_MFW_NUMITEMS; j++)
      ptr = strchr (ptr, ' ') + 1;
    *(--ptr) = '\0';

    sprintf (buf, "Minifig%.2dColors", i);
    Sys_ProfileSaveString ("MinifigWizard", buf, value);
    ptr++;

    value = ptr;
    for (j = 0; j < LC_MFW_NUMITEMS; j++)
      ptr = strchr (ptr, ' ') + 1;
    *(--ptr) = '\0';

    sprintf (buf, "Minifig%.2dPieces", i);
    Sys_ProfileSaveString ("MinifigWizard", buf, value);
    ptr++;

    sprintf (buf, "Minifig%.2dAngles", i);
    Sys_ProfileSaveString ("MinifigWizard", buf, ptr);

    free (m_MinifigNames[i]);
    free (m_MinifigTemplates[i]);
  }
  free (m_MinifigNames);
  free (m_MinifigTemplates);
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
  float pos[LC_MFW_NUMITEMS][3];
  float rot[LC_MFW_NUMITEMS][3];
  Matrix mat, m2, m3;

  // FIXME:
  // Saucepan: tool rotate in wrong axis
  // Frypan: tool rotate in wrong axis
  // Cup: tool rotate not centered
  // Metal Detector: tool rotate not centered
  // Space Laser Gun: tool rotate not centered
  // Hairbrush: tool rotate not centered

  // Get the pieces in the right place
  for (int type = 0; type < LC_MFW_NUMITEMS; type++)
  {
    PieceInfo* piece_info = m_Info[type];
    const char *desc;
    int j;

    if (piece_info == NULL)
      continue;

    for (j = 0; j < mfw_pieces; j++)
      if (strcmp (piece_info->m_strName, mfw_pieceinfo[j].name) == 0)
	break;

    pos[type][0] = mfw_pieceinfo[j].x;
    pos[type][1] = mfw_pieceinfo[j].y;
    pos[type][2] = mfw_pieceinfo[j].z;
    rot[type][0] = mfw_pieceinfo[j].rx;
    rot[type][1] = mfw_pieceinfo[j].ry;
    rot[type][2] = mfw_pieceinfo[j].rz;
    desc = mfw_pieceinfo[j].description;

    switch (type)
    {
    case LC_MFW_HAT:
    case LC_MFW_HEAD:
      if (m_Info[LC_MFW_NECK] != NULL)
	pos[type][2] += 0.08f;
      break;

    case LC_MFW_RIGHT_HAND:
    case LC_MFW_RIGHT_SHOE:
      pos[type][0] = -pos[type][0];
      break;

    case LC_MFW_RIGHT_TOOL:
      if (strcmp (piece_info->m_strName, "4499") == 0) // Bow with Arrow
	rot[type][2] = -rot[type][2];
      break;
    }
  }

  // hat
  m_Position[LC_MFW_HAT][0] = pos[LC_MFW_HAT][0];
  m_Position[LC_MFW_HAT][1] = pos[LC_MFW_HAT][1];
  m_Position[LC_MFW_HAT][2] = pos[LC_MFW_HAT][2];
  m_Rotation[LC_MFW_HAT][0] = 0.0f;
  m_Rotation[LC_MFW_HAT][1] = 0.0f;
  m_Rotation[LC_MFW_HAT][2] = -1.0f;
  m_Rotation[LC_MFW_HAT][3] = m_Angles[LC_MFW_HAT] + m_Angles[LC_MFW_HEAD];

  // head
  m_Position[LC_MFW_HEAD][0] = pos[LC_MFW_HEAD][0];
  m_Position[LC_MFW_HEAD][1] = pos[LC_MFW_HEAD][1];
  m_Position[LC_MFW_HEAD][2] = pos[LC_MFW_HEAD][2];
  m_Rotation[LC_MFW_HEAD][0] = 0.0f;
  m_Rotation[LC_MFW_HEAD][1] = 0.0f;
  m_Rotation[LC_MFW_HEAD][2] = -1.0f;
  m_Rotation[LC_MFW_HEAD][3] = m_Angles[LC_MFW_HEAD];

  // neck
  mat.LoadIdentity ();
  mat.CreateOld (0,0,0,rot[LC_MFW_NECK][0], rot[LC_MFW_NECK][1], rot[LC_MFW_NECK][2]);
  mat.Rotate (m_Angles[LC_MFW_NECK], 0, 0, -1);
  mat.SetTranslation (pos[LC_MFW_NECK][0], pos[LC_MFW_NECK][1],
		      pos[LC_MFW_NECK][2]);
  mat.ToAxisAngle (m_Rotation[LC_MFW_NECK]);
  mat.GetTranslation (m_Position[LC_MFW_NECK]);

  // torso
  m_Position[LC_MFW_TORSO][0] = pos[LC_MFW_TORSO][0];
  m_Position[LC_MFW_TORSO][1] = pos[LC_MFW_TORSO][1];
  m_Position[LC_MFW_TORSO][2] = pos[LC_MFW_TORSO][2];
  m_Rotation[LC_MFW_TORSO][0] = 0.0f;
  m_Rotation[LC_MFW_TORSO][1] = 0.0f;
  m_Rotation[LC_MFW_TORSO][2] = 1.0f;
  m_Rotation[LC_MFW_TORSO][3] = 0.0f;

  // left arm/hand/tool
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_LEFT_ARM], -1, 0, 0);
  mat.SetTranslation (pos[LC_MFW_LEFT_ARM][0], pos[LC_MFW_LEFT_ARM][1],
		      pos[LC_MFW_LEFT_ARM][2]);
  mat.ToAxisAngle (m_Rotation[LC_MFW_LEFT_ARM]);
  mat.GetTranslation (m_Position[LC_MFW_LEFT_ARM]);

  mat.Translate (pos[LC_MFW_LEFT_HAND][0]-pos[LC_MFW_LEFT_ARM][0],
		 pos[LC_MFW_LEFT_HAND][1]-pos[LC_MFW_LEFT_ARM][1],
		 pos[LC_MFW_LEFT_HAND][2]-pos[LC_MFW_LEFT_ARM][2]);
  m2.CreateOld (0,0,0,rot[LC_MFW_LEFT_HAND][0],rot[LC_MFW_LEFT_HAND][1],rot[LC_MFW_LEFT_HAND][2]);
  m3.Multiply (mat, m2);
  m3.Translate (0,0,-0.16f);
  mat.LoadIdentity ();
  mat.Translate (0,0,0.16f);
  mat.Rotate (m_Angles[LC_MFW_LEFT_HAND], 1, 0, 0);
  m2.Multiply (m3, mat);
  m2.ToAxisAngle (m_Rotation[LC_MFW_LEFT_HAND]);
  m2.GetTranslation (m_Position[LC_MFW_LEFT_HAND]);

  m2.Translate (pos[LC_MFW_LEFT_TOOL][0]-0.9f,
		pos[LC_MFW_LEFT_TOOL][1]-pos[LC_MFW_LEFT_HAND][1],
		pos[LC_MFW_LEFT_TOOL][2]-pos[LC_MFW_LEFT_HAND][2]);
  m3.CreateOld (0,0,0,rot[LC_MFW_LEFT_TOOL][0]-rot[LC_MFW_LEFT_HAND][0],
		rot[LC_MFW_LEFT_TOOL][1]-rot[LC_MFW_LEFT_HAND][1],
		rot[LC_MFW_LEFT_TOOL][2]-rot[LC_MFW_LEFT_HAND][2]);
  mat.Multiply (m2, m3);
  m2.LoadIdentity ();
  m2.Rotate (m_Angles[LC_MFW_LEFT_TOOL], 0, 0, 1);
  m3.Multiply (mat, m2);
  m3.ToAxisAngle (m_Rotation[LC_MFW_LEFT_TOOL]);
  m3.GetTranslation (m_Position[LC_MFW_LEFT_TOOL]);

  // right arm/hand/tool
  mat.LoadIdentity (); m2.LoadIdentity (); m3.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_RIGHT_ARM], -1, 0, 0);
  mat.SetTranslation (pos[LC_MFW_RIGHT_ARM][0], pos[LC_MFW_RIGHT_ARM][1],
		      pos[LC_MFW_RIGHT_ARM][2]);
  mat.ToAxisAngle (m_Rotation[LC_MFW_RIGHT_ARM]);
  mat.GetTranslation (m_Position[LC_MFW_RIGHT_ARM]);

  mat.Translate (pos[LC_MFW_RIGHT_HAND][0]-pos[LC_MFW_RIGHT_ARM][0],
		 pos[LC_MFW_RIGHT_HAND][1]-pos[LC_MFW_RIGHT_ARM][1],
		 pos[LC_MFW_RIGHT_HAND][2]-pos[LC_MFW_RIGHT_ARM][2]);
  m2.CreateOld (0,0,0,rot[LC_MFW_RIGHT_HAND][0],rot[LC_MFW_RIGHT_HAND][1],rot[LC_MFW_RIGHT_HAND][2]);
  m3.Multiply (mat, m2);
  m3.Translate (0,0,-0.16f);
  mat.LoadIdentity ();
  mat.Translate (0,0,0.16f);
  mat.Rotate (m_Angles[LC_MFW_RIGHT_HAND], 1, 0, 0);
  m2.Multiply (m3, mat);
  m2.ToAxisAngle (m_Rotation[LC_MFW_RIGHT_HAND]);
  m2.GetTranslation (m_Position[LC_MFW_RIGHT_HAND]);

  m2.Translate (pos[LC_MFW_RIGHT_TOOL][0]-0.9f,
		pos[LC_MFW_RIGHT_TOOL][1]-pos[LC_MFW_RIGHT_HAND][1],
		pos[LC_MFW_RIGHT_TOOL][2]-pos[LC_MFW_RIGHT_HAND][2]);
  m3.CreateOld (0,0,0,rot[LC_MFW_RIGHT_TOOL][0]-rot[LC_MFW_RIGHT_HAND][0],
		rot[LC_MFW_RIGHT_TOOL][1]-rot[LC_MFW_RIGHT_HAND][1],
		rot[LC_MFW_RIGHT_TOOL][2]-rot[LC_MFW_RIGHT_HAND][2]);
  mat.Multiply (m2, m3);
  m2.LoadIdentity ();
  m2.Rotate (m_Angles[LC_MFW_RIGHT_TOOL], 0, 0, 1);
  m3.Multiply (mat, m2);
  m3.ToAxisAngle (m_Rotation[LC_MFW_RIGHT_TOOL]);
  m3.GetTranslation (m_Position[LC_MFW_RIGHT_TOOL]);

  // hips
  m_Position[LC_MFW_HIPS][0] = pos[LC_MFW_HIPS][0];
  m_Position[LC_MFW_HIPS][1] = pos[LC_MFW_HIPS][1];
  m_Position[LC_MFW_HIPS][2] = pos[LC_MFW_HIPS][2];
  m_Rotation[LC_MFW_HIPS][0] = 0.0f;
  m_Rotation[LC_MFW_HIPS][1] = 0.0f;
  m_Rotation[LC_MFW_HIPS][2] = 1.0f;
  m_Rotation[LC_MFW_HIPS][3] = 0.0f;

  // left leg/shoe
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_LEFT_LEG], -1, 0, 0);
  mat.SetTranslation (pos[LC_MFW_LEFT_LEG][0], pos[LC_MFW_LEFT_LEG][1],
		      pos[LC_MFW_LEFT_LEG][2]);
  mat.ToAxisAngle (m_Rotation[LC_MFW_LEFT_LEG]);
  mat.GetTranslation (m_Position[LC_MFW_LEFT_LEG]);
  mat.Translate (pos[LC_MFW_LEFT_SHOE][0]-pos[LC_MFW_LEFT_LEG][0],
		 pos[LC_MFW_LEFT_SHOE][1]-pos[LC_MFW_LEFT_LEG][1],
		 pos[LC_MFW_LEFT_SHOE][2]-pos[LC_MFW_LEFT_LEG][2]);
  m2.CreateOld (0,0,0,rot[LC_MFW_LEFT_SHOE][0],rot[LC_MFW_LEFT_SHOE][1],rot[LC_MFW_LEFT_SHOE][2]);
  m3.Multiply (mat, m2);
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_LEFT_SHOE], 0, 0, 1);
  m2.Multiply (m3, mat);
  m2.ToAxisAngle (m_Rotation[LC_MFW_LEFT_SHOE]);
  m2.GetTranslation (m_Position[LC_MFW_LEFT_SHOE]);

  // right leg/shoe
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_RIGHT_LEG], -1, 0, 0);
  mat.SetTranslation (pos[LC_MFW_RIGHT_LEG][0], pos[LC_MFW_RIGHT_LEG][1],
		      pos[LC_MFW_RIGHT_LEG][2]);
  mat.ToAxisAngle (m_Rotation[LC_MFW_RIGHT_LEG]);
  mat.GetTranslation (m_Position[LC_MFW_RIGHT_LEG]);
  mat.Translate (pos[LC_MFW_RIGHT_SHOE][0]-pos[LC_MFW_RIGHT_LEG][0],
		 pos[LC_MFW_RIGHT_SHOE][1]-pos[LC_MFW_RIGHT_LEG][1],
		 pos[LC_MFW_RIGHT_SHOE][2]-pos[LC_MFW_RIGHT_LEG][2]);
  m2.CreateOld (0,0,0,rot[LC_MFW_RIGHT_SHOE][0],rot[LC_MFW_RIGHT_SHOE][1],rot[LC_MFW_RIGHT_SHOE][2]);
  m3.Multiply (mat, m2);
  mat.LoadIdentity ();
  mat.Rotate (m_Angles[LC_MFW_RIGHT_SHOE], 0, 0, 1);
  m2.Multiply (m3, mat);
  m2.ToAxisAngle (m_Rotation[LC_MFW_RIGHT_SHOE]);
  m2.GetTranslation (m_Position[LC_MFW_RIGHT_SHOE]);
}

void MinifigWizard::GetSelections (char **names)
{
  for (int i = 0; i < LC_MFW_NUMITEMS; i++)
  {
    PieceInfo* piece_info = m_Info[i];
    names[i] = "None";

    if (piece_info == NULL)
      continue;

    for (int j = 0; j < mfw_pieces; j++)
      if (strcmp (piece_info->m_strName, mfw_pieceinfo[j].name) == 0)
      {
	names[i] = mfw_pieceinfo[j].description;
	break;
      }
  }
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

    strcpy (mfw_pieceinfo[i].name, piece_info->m_strName);

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
}

void MinifigWizard::ChangeColor (int type, int color)
{
  m_Colors[type] = color;
}

void MinifigWizard::ChangeAngle (int type, float angle)
{
  m_Angles[type] = angle;
}

void MinifigWizard::GetMinifigNames (char ***names, int *count)
{
  *count = m_MinifigCount;
  *names = m_MinifigNames;
}

void MinifigWizard::SaveMinifig (const char* name)
{
  char tmp[16];
  int i, j;

  // check if the name is already being used
  for (i = 0; i < m_MinifigCount; i++)
    if (strcmp (m_MinifigNames[i], name) == 0)
      break;

  if (i == m_MinifigCount)
  {
    m_MinifigCount++;
    m_MinifigNames = (char**)realloc (m_MinifigNames, sizeof (char**)*m_MinifigCount);
    m_MinifigTemplates = (char**)realloc (m_MinifigTemplates, sizeof (char**)*m_MinifigCount);
    m_MinifigNames[i] = (char*)malloc (strlen (name) + 1);
    strcpy (m_MinifigNames[i], name);
    m_MinifigTemplates[i] = (char*)malloc (768);
  }
  strcpy (m_MinifigTemplates[i], "");

  for (j = 0; j < LC_MFW_NUMITEMS; j++)
  {
    sprintf (tmp, "%d ", m_Colors[j]);
    strcat (m_MinifigTemplates[i], tmp);
  }

  for (j = 0; j < LC_MFW_NUMITEMS; j++)
  {
    if (m_Info[j] != NULL)
      sprintf (tmp, "%s ", m_Info[j]->m_strName);
    else
      strcpy (tmp, "None ");
    strcat (m_MinifigTemplates[i], tmp);
  }

  for (j = 0; j < LC_MFW_NUMITEMS; j++)
  {
    sprintf (tmp, "%f ", m_Angles[j]);
    strcat (m_MinifigTemplates[i], tmp);
  }
}

bool MinifigWizard::LoadMinifig (const char* name)
{
  char *ptr;
  int i, j;

  // check if the name is valid
  for (i = 0; i < m_MinifigCount; i++)
    if (strcmp (m_MinifigNames[i], name) == 0)
      break;

  if (i == m_MinifigCount)
  {
    //    Sys_MessageBox ("Unknown Minifig");
    return false;
  }
  else
    ptr = m_MinifigTemplates[i];

  for (j = 0; j < LC_MFW_NUMITEMS; j++)
    if (m_Info[j] != NULL)
      m_Info[j]->DeRef ();

  for (j = 0; j < LC_MFW_NUMITEMS; j++)
    m_Colors[j] = strtol (ptr, &ptr, 10);

  for (j = 0; j < LC_MFW_NUMITEMS; j++)
  {
    char *endptr;
    ptr++;

    endptr = strchr (ptr, ' ');
    *endptr = '\0';
    m_Info[j] = project->FindPieceInfo (ptr);
    *endptr = ' ';
    ptr = endptr;

    if (m_Info[j] != NULL)
      m_Info[j]->AddRef();
  }

  for (j = 0; j < LC_MFW_NUMITEMS; j++)
    m_Angles[j] = strtod (ptr, &ptr);

  return true;
}

void MinifigWizard::DeleteMinifig (const char* name)
{
  int i;

  // check if the name is valid
  for (i = 0; i < m_MinifigCount; i++)
    if (strcmp (m_MinifigNames[i], name) == 0)
      break;

  if (i == m_MinifigCount)
  {
    Sys_MessageBox ("Unknown Minifig");
    return;
  }

  free (m_MinifigNames[i]);
  free (m_MinifigTemplates[i]);
  m_MinifigCount--;

  for (; i < m_MinifigCount; i++)
  {
    m_MinifigNames[i] = m_MinifigNames[i+1];
    m_MinifigTemplates[i] = m_MinifigTemplates[i+1];
  }
}

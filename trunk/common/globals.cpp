// Global variables common to all platforms.
//

#include <stdlib.h>
#include "typedefs.h"
#include "defines.h"

class Project;
Project* project;

const char* colornames[LC_MAXCOLORS] = { "Red", "Orange", "Green",
	"Light Green", "Blue", "Light Blue", "Yellow", "White",
	"Dark Gray", "Black", "Brown", "Pink", "Purple", "Gold",
	"Clear Red", "Clear Orange", "Clear Green", "Clear Light Green",
	"Clear Blue", "Clear Light Blue", "Clear Yellow", "Clear White",
	"Light Gray", "Tan", "Light Brown", "Light Pink", "Turquoise", "Silver" };

const char* altcolornames[LC_MAXCOLORS] = { "Red", "Orange", "Green",
	"LightGreen", "Blue", "LightBlue", "Yellow", "White",
	"DarkGray", "Black", "Brown", "Pink", "Purple", "Gold",
	"ClearRed", "ClearOrange", "ClearGreen", "ClearLightGreen",
	"ClearBlue", "ClearLightBlue", "ClearYellow", "ClearWhite",
	"LightGray", "Tan", "LightBrown", "LightPink", "Turquoise", "Silver" };

unsigned char FlatColorArray[31][3] = {
	{ 166,  25,  25 },  // 0 - Red
	{ 255, 127,  51 },  // 1 - Orange
	{  25, 102,  25 },  // 2 - Green
	{  76, 153,  76 },  // 3 - Light Green
	{   0,  51, 178 },  // 4 - Blue
	{  51, 102, 229 },  // 5 - Light Blue
	{ 204, 204,   0 },  // 6 - Yellow
	{ 242, 242, 242 },  // 7 - White
	{  84,  76,  76 },  // 8 - Dark Gray
	{  51,  51,  51 },  // 9 - Black
	{ 102,  51,  51 },  //10 - Brown
	{ 178,  76, 153 },  //11 - Pink
	{ 153,  51, 153 },  //12 - Purple
	{ 229, 178,  51 },  //13 - Gold
	{ 153,  25,  25 },  //14 - Clear Red
	{ 255, 153,  76 },  //15 - Clear Orange
	{  25, 102,  25 },  //16 - Clear Green
	{ 153, 178,  76 },  //17 - Clear Light Green
	{   0,   0, 127 },  //18 - Clear Blue
	{  51, 102, 229 },  //19 - Clear Light Blue
	{ 229, 229,   0 },  //20 - Clear Yellow
	{ 229, 229, 229 },  //21 - Clear White
	{ 140, 140, 140 },  //22 - Light Gray
	{ 204, 204, 178 },  //23 - Tan
	{ 153, 102, 102 },  //24 - Light Brown
	{ 229, 178, 229 },  //25 - Light Pink
	{  25, 178, 204 },  //26 - Turquoise
	{ 204, 204, 204 },  //27 - Silver
	{   0,   0,   0 },  //28 - Edges
	{ 229,  76, 102 },  //29 - Selected
	{ 102,  76, 229 }}; //30 - Focused

unsigned char ColorArray[31][4] = {
	{ 166,  25,  25, 255 },  // 0 - Red
	{ 255, 127,  51, 255 },  // 1 - Orange
	{  25, 102,  25, 255 },  // 2 - Green
	{  76, 153,  76, 255 },  // 3 - Light Green
	{   0,  51, 178, 255 },  // 4 - Blue
	{  51, 102, 229, 255 },  // 5 - Light Blue
	{ 204, 204,   0, 255 },  // 6 - Yellow
	{ 242, 242, 242, 255 },  // 7 - White
	{  76,  76,  76, 255 },  // 8 - Dark Gray
	{  25,  25,  25, 255 },  // 9 - Black
	{ 102,  51,  51, 255 },  //10 - Brown
	{ 178,  76, 153, 255 },  //11 - Pink
	{ 153,  51, 153, 255 },  //12 - Purple
	{ 229, 178,  51, 255 },  //13 - Gold
	{ 153,  25,  25, 153 },  //14 - Clear Red
	{ 255, 153,  76, 153 },  //15 - Clear Orange
	{  25, 102,  25, 153 },  //16 - Clear Green
	{ 153, 178,  76, 153 },  //17 - Clear Light Green
	{   0,   0, 127, 153 },  //18 - Clear Blue
	{  51, 102, 229, 153 },  //19 - Clear Light Blue
	{ 229, 229,   0, 153 },  //20 - Clear Yellow
	{ 229, 229, 229, 153 },  //21 - Clear White
	{ 127, 127, 127, 255 },  //22 - Light Gray
	{ 204, 204, 178, 255 },  //23 - Tan
	{ 153, 102, 102, 255 },  //24 - Light Brown
	{ 229, 178, 229, 255 },  //25 - Light Pink
	{  25, 178, 204, 255 },  //26 - Turquoise
	{ 204, 204, 204, 255 },  //27 - Silver
	{  51,  51,  51, 255 },  //28 - Edges
	{ 229,  76, 102, 255 },  //29 - Selected
	{ 102,  76, 229, 255 }}; //30 - Focused

// =========================================================
// Minifig Wizard options

MFW_PIECEINFO mfwpieceinfo[] = {
  { "3624", "Police Hat", MF_HAT },
  { "3626BP01", "Smiley Face", MF_HEAD },
  { "973", "Plain Torso", MF_TORSO },
  { "3838", "Airtanks", MF_NECK },
  { "976", "Left Arm", MF_ARML },
  { "975", "Right Arm", MF_ARMR },
  { "977", "Hand", MF_HAND },
  { "977", "Hand", MF_HAND },
  { "3899", "Cup", MF_TOOL },
  { "4528", "Frypan", MF_TOOL },
  { "970", "Hips", MF_HIPS },
  { "972", "Left Leg", MF_LEGL }, 
  { "971", "Right Leg", MF_LEGR },
  { "2599", "Flipper", MF_SHOE },
  { "6120", "Ski", MF_SHOE },
  { "4485", "Baseball Cap", MF_HAT },
  { "3626B", "Plain Face", MF_HEAD },
  { "3626BP02", "Woman Face", MF_HEAD },
  { "973P11", "Dungarees", MF_TORSO },
  { "973P47", "Castle Red/Gray Symbol", MF_TORSO }, 
  { "973P51", "Blacktron II", MF_TORSO },
  { "973P01", "Vertical Strips Red/Blue", MF_TORSO },
  { "973P02", "Vertical Strips Blue/Red", MF_TORSO }, 
  { "973P60", "Shell Logo", MF_TORSO },
  { "973P61", "Gold Ice Planet Pattern", MF_TORSO }, 
  { "4349", "Loudhailer", MF_TOOL },
  { "3962", "Radio", MF_TOOL },
  { "4529", "Saucepan", MF_TOOL },
  { "3959", "Space Gun", MF_TOOL },
  { "4360", "Space Laser Gun", MF_TOOL },
  { "4479", "Metal Detector", MF_TOOL },
  { "6246A", "Screwdriver", MF_TOOL },
  { "6246B", "Hammer", MF_TOOL },
  { "6246D", "Box Wrench", MF_TOOL },
  { "6246E", "Open End Wrench", MF_TOOL },
  { "3896", "Castle Helmet with Chin-Guard", MF_HAT },
  { "3844", "Castle Helmet with Neck Protect", MF_HAT },
  { "3833", "Construction Helmet", MF_HAT }, 
  { "82359", "Skeleton Skull", MF_HEAD },
  { "973P14", "'S' Logo", MF_TORSO },
  { "973P16", "Airplane Logo", MF_TORSO }, 
  { "973P52", "Blacktron I Pattern", MF_TORSO },
  { "973P15", "Horizontal Stripes", MF_TORSO },
  { "973P68", "Mtron Logo", MF_TORSO }, 
  { "973P17", "Red V-Neck and Buttons", MF_TORSO },
  { "973P63", "Robot Pattern", MF_TORSO },
  { "973P18", "Suit and Tie ", MF_TORSO }, 
  { "4736", "Jet-Pack with Stud On Front", MF_NECK },
  { "4522", "Mallet", MF_TOOL },
  { "6246C", "Power Drill", MF_TOOL },
  { "4006", "Spanner/Screwdriver", MF_TOOL },
  { "194", "Hose Nozzle", MF_TOOL },
  { "2446", "Helmet", MF_HAT },
  { "3840", "Vest", MF_NECK },
  { "970P63", "Hips with Robot Pattern", MF_HIPS },
  { "972P63", "Left Leg with Robot Pattern", MF_LEGL },
  { "971P63", "Right Leg with Robot Pattern", MF_LEGR },
  { "2524", "Backpack Non-Opening", MF_NECK },
  { "4497", "Spear", MF_TOOL },
  { "37", "Knife", MF_TOOL },
  { "38", "Harpoon", MF_TOOL },
  { "3626BP03", "Pointed Moustache", MF_HEAD },
  { "3626BP04", "Sunglasses", MF_HEAD },
  { "3626BP05", "Grin and Eyebrows", MF_HEAD }, 
  { "973P19", "Train Chevron", MF_TORSO },
  { "973P31", "Pirate Strips (Red/Cream)", MF_TORSO },
  { "973P32", "Pirate Strips (Blue/Cream)", MF_TORSO },
  { "973P33", "Pirate Strips (Red/Black)", MF_TORSO },
  { "973P41", "Castle Chainmail", MF_TORSO },
  { "973P62", "Silver Ice Planet", MF_TORSO },
  { "6131", "Wizard Hat", MF_HAT },
  { "973P20", "Waiter", MF_TORSO },
  { "973P49", "Forestman Blue Collar", MF_TORSO },
  { "973P48", "Forestman Maroon Collar", MF_TORSO },
  { "973P50", "Forestman Black Collar", MF_TORSO },
  { "3841", "Pickaxe", MF_TOOL },
  { "973P21", "Five Button Fire Fighter", MF_TORSO },
  { "973P22", "Red Shirt and Suit", MF_TORSO },
  { "973P34", "Open Jacket over Striped Vest", MF_TORSO },
  { "973P46", "Forestman and Purse", MF_TORSO },
  { "973P101", "SW Rebel Pilot", MF_TORSO },
  { "4498", "Arrow Quiver", MF_NECK },
  { "4499", "Bow with Arrow", MF_TOOL },
  { "3852", "Hairbrush", MF_TOOL },
  { "30152", "Magnifying Glass", MF_TOOL }
};

//	{ "770", "Shield Ovoid", MF_TOOL }, 
//	2447      Minifig Helmet Visor

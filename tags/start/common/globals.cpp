// Global variables common to all platforms.
//

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

float FlatColorArray[31][3] = {
	{ 0.65f,0.1f, 0.1f },  // 0 - Red
	{ 1.0f, 0.5f, 0.2f },  // 1 - Orange
	{ 0.1f, 0.4f, 0.1f },  // 2 - Green
	{ 0.3f, 0.6f, 0.3f },  // 3 - Light Green
	{ 0.0f, 0.2f, 0.7f },  // 4 - Blue
	{ 0.2f, 0.4f, 0.9f },  // 5 - Light Blue
	{ 0.8f, 0.8f, 0.0f },  // 6 - Yellow
	{ 0.95f,0.95f,0.95f},  // 7 - White
	{ 0.33f,0.3f, 0.3f },  // 8 - Dark Gray
	{ 0.2f, 0.2f, 0.2f },  // 9 - Black
	{ 0.4f, 0.2f, 0.2f },  //10 - Brown
	{ 0.7f, 0.3f, 0.6f },  //11 - Pink
	{ 0.6f, 0.2f, 0.6f },  //12 - Purple
	{ 0.9f, 0.7f, 0.2f },  //13 - Gold
	{ 0.6f, 0.1f, 0.1f },  //14 - Clear Red
	{ 1.0f, 0.6f, 0.3f },  //15 - Clear Orange
	{ 0.1f, 0.4f, 0.1f },  //16 - Clear Green
	{ 0.6f, 0.7f, 0.3f },  //17 - Clear Light Green
	{ 0.0f, 0.0f, 0.5f },  //18 - Clear Blue
	{ 0.2f, 0.4f, 0.9f },  //19 - Clear Light Blue
	{ 0.9f, 0.9f, 0.0f },  //20 - Clear Yellow
	{ 0.9f, 0.9f, 0.9f },  //21 - Clear White
	{ 0.55f,0.55f,0.55f},  //22 - Light Gray
	{ 0.8f, 0.8f, 0.7f },  //23 - Tan
	{ 0.6f, 0.4f, 0.4f },  //24 - Light Brown
	{ 0.9f, 0.7f, 0.9f },  //25 - Light Pink
	{ 0.1f, 0.7f, 0.8f },  //26 - Turquoise
	{ 0.8f, 0.8f, 0.8f },  //27 - Silver
	{ 0.0f, 0.0f, 0.0f },  //28 - Edges
	{ 0.9f, 0.3f, 0.4f },  //29 - Selected
	{ 0.4f, 0.3f, 0.9f }}; //30 - Focused

float ColorArray[31][4] = {
	{ 0.65f,0.1f, 0.1f, 1.0f },  // 0 - Red
	{ 1.0f, 0.5f, 0.2f, 1.0f },  // 1 - Orange
	{ 0.1f, 0.4f, 0.1f, 1.0f },  // 2 - Green
	{ 0.3f, 0.6f, 0.3f, 1.0f },  // 3 - Light Green
	{ 0.0f, 0.2f, 0.7f, 1.0f },  // 4 - Blue
	{ 0.2f, 0.4f, 0.9f, 1.0f },  // 5 - Light Blue
	{ 0.8f, 0.8f, 0.0f, 1.0f },  // 6 - Yellow
	{ 0.95f,0.95f,0.95f,1.0f },  // 7 - White
	{ 0.3f, 0.3f, 0.3f, 1.0f },  // 8 - Dark Gray
	{ 0.1f, 0.1f, 0.1f, 1.0f },  // 9 - Black
	{ 0.4f, 0.2f, 0.2f, 1.0f },  //10 - Brown
	{ 0.7f, 0.3f, 0.6f, 1.0f },  //11 - Pink
	{ 0.6f, 0.2f, 0.6f, 1.0f },  //12 - Purple
	{ 0.9f, 0.7f, 0.2f, 1.0f },  //13 - Gold
	{ 0.6f, 0.1f, 0.1f, 0.6f },  //14 - Clear Red
	{ 1.0f, 0.6f, 0.3f, 0.6f },  //15 - Clear Orange
	{ 0.1f, 0.4f, 0.1f, 0.6f },  //16 - Clear Green
	{ 0.6f, 0.7f, 0.3f, 0.6f },  //17 - Clear Light Green
	{ 0.0f, 0.0f, 0.5f, 0.6f },  //18 - Clear Blue
	{ 0.2f, 0.4f, 0.9f, 0.6f },  //19 - Clear Light Blue
	{ 0.9f, 0.9f, 0.0f, 0.6f },  //20 - Clear Yellow
	{ 0.9f, 0.9f, 0.9f, 0.6f },  //21 - Clear White
	{ 0.5f, 0.5f, 0.5f, 1.0f },  //22 - Light Gray
	{ 0.8f, 0.8f, 0.7f, 1.0f },  //23 - Tan
	{ 0.6f, 0.4f, 0.4f, 1.0f },  //24 - Light Brown
	{ 0.9f, 0.7f, 0.9f, 1.0f },  //25 - Light Pink
	{ 0.1f, 0.7f, 0.8f, 1.0f },  //26 - Turquoise
	{ 0.8f, 0.8f, 0.8f, 1.0f },  //27 - Silver
	{ 0.2f, 0.2f, 0.2f, 1.0f },  //28 - Edges
	{ 0.9f, 0.3f, 0.4f, 1.0f },  //29 - Selected
	{ 0.4f, 0.3f, 0.9f, 1.0f }}; //30 - Focused

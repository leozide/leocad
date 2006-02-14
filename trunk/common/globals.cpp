//
// Global variables common to all platforms.
//

#include <stdlib.h>
#include "globals.h"

Messenger* messenger;
MainWnd* main_window;

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

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
#include "library.h"
#include "lc_application.h"

// =============================================================================
// Static variables

static LC_MFW_PIECEINFO mfw_pieceinfo[] =
{
	// Helms
	{ "193",     "Helmet Classic",                      LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "193A",    "Helmet with Thin Chin Guard",         LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "193B",    "Helmet with Thick Chin Guard",        LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "390",     "Hair Female with Pigtails",           LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "524",     "Darth Vader Helmet",                  LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "526",     "Samurai Helmet",                      LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "526C01",  "Samurai Helmet with Horn",            LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "527",     "Helmet with Chinstrap and Wide Brim", LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "530",     "Knit Cap",                            LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "775",     "Forestman Cap",                       LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "775C01",  "Forestman Cap with Red Plume",        LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "2446",    "Helmet Modern",                       LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "2528",    "Bicorne Hat",                         LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "2544",    "Tricorne Hat",                        LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "2544C01", "Tricorne Hat with White Plume",       LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "2545",    "Imperial Guard Shako",                LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3624",    "Police Hat",                          LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3629",    "Cowboy Hat",                          LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3833",    "Construction Helmet",                 LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3834",    "Fire Helmet",                         LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3844",    "Castle Helmet with Neck Protect",     LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3878",    "Top Hat",                             LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3896",    "Castle Helmet with Chin-Guard",       LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3901",    "Hair Male",                           LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "4485",    "Baseball Cap",                        LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "6093",    "Hair Shoulder Length",                LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "6131",    "Wizard Hat",                          LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "30048",   "Helmet Morion",                       LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "30171",   "Aviator Cap",                         LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "71015",   "Crown",                               LC_MFW_HAT, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },

	// Faces
	{ "3626B",    "Plain Face",                             LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP01", "Smiley Face",                            LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP02", "Woman Face",                             LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP03", "Pointed Moustache",                      LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP04", "Sunglasses",                             LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP05", "Grin and Eyebrows",                      LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f }, 
	{ "3626BP06", "Grin, Eyebrows and Microphone",          LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP30", "Messy Hair and Moustache",               LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP31", "Messy Hair and Eye Patch",               LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP32", "Messy Hair, Moustache and Eye Patch",    LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP33", "Messy Hair, Moustache and Beard",        LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP34", "Messy Hair, Beard and Eye Patch",        LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP39", "Dark Grey Facial Hair",                  LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP3E", "Wiry Moustache and Eyebrows",            LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP3J", "Islander White/Red Painted Face",        LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP3K", "Islander White/Blue Painted Face",       LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP3N", "Sideburns and Droopy Moustache Black",   LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP3Q", "Sideburns and Droopy Moustache Brown",   LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP40", "Messy Hair Female",                      LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP61", "Ice Planet Moustache and Eyebrows",      LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP62", "Ice Planet Messy White Hair",            LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP63", "Silver Robot",                           LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP65", "Ice Planet Female Red Hair",             LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP69", "Headset Over Brown Hair and Eyebrows",   LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP6F", "Red Lips and Black Upswept Eyelashes",   LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP7A", "Brown Hair Over Eye and Black Eyebrows", LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP7B", "Blue Sunglasses",                        LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BP7C", "Blue Wrap-Around Sunglasses",            LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPA1", "Glasses and White Muttonchops",          LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPA2", "Adventurers Mummy",                      LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPA3", "Smirk and Black Moustache",              LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPA4", "Villan Black Facial Hair",               LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPA5", "Stubble Moustache and Smirk",            LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPA6", "Brown Hair, Eyelashes and Lipstick",     LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPA7", "Monacle, Scar and Moustache",            LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPA9", "Villainous Glasses & Black Facial Hair", LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPAC", "Tribal Paint and Frown",                 LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPS2", "SW Brown Eyebrows",                      LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPS3", "SW Small Black Eyebrows",                LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPS4", "SW Grey Beard and Eyebrows",             LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPS5", "SW Smirk and Brown Eyebrows",            LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPS7", "SW Black Eyebrows and Scar",             LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPS8", "SW Darth Maul",                          LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPS9", "SW Brown Eyebrows and Beard",            LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPSB", "SW Alien with Large Black Eyes",         LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPSC", "SW Grey Eyebrows and Implant",           LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPSE", "SW Scout Trooper Black Visor",           LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "3626BPST", "SW Tusken Raider",                       LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },
	{ "82359",    "Skeleton Skull",                         LC_MFW_HEAD, 0.0f, 0.0f, 3.84f, 0.0f, 0.0f, 0.0f },

	// Torsos
	{ "973",    "Plain Torso",                            LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P01", "Vertical Strips Red/Blue",               LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P02", "Vertical Strips Blue/Red",               LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f }, 
	{ "973P03", "White Shirt and Jacket",                 LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P04", "Six Button Suit and Airplane",           LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P05", "Six Button Suit and Anchor",             LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P09", "Anchor Motif",                           LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P0A", "White Diagonal Zip and Pocket",          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P0B", "Black Diagonal Zip and Pocket",          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P11", "Dungarees",                              LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P12", "Riding Jacket",                          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P13", "Straight Zipper Jacket",                 LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P14", "'S' Logo",                               LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P15", "Horizontal Stripes",                     LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P16", "Airplane Logo",                          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P17", "Red V-Neck and Buttons",                 LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P18", "Suit and Tie ",                          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P19", "Train Chevron",                          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P1A", "Black Dungarees",                        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P1B", "Blue Dungarees",                         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P1C", "Red Dungarees",                          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P1D", "Blue Horizontal Stripes",                LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P1E", "Red Horizontal Stripes",                 LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P1H", "Racing Jacket and Two Stars",            LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P1J", "Green Dungarees",                        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P1M", "TV Logo Pattern Large",                  LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P1Q", "Launch Command Logo and Equipment",      LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P20", "Waiter",                                 LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P21", "Five Button Fire Fighter",               LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P22", "Red Shirt and Suit",                     LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P23", "'S' Logo Yellow / Blue Pattern",         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P24", "Red Cross Pattern",                      LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P25", "Red Cross & Stethoscope",                LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P26", "Patch Pocket",                           LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P27", "Autoroute Pattern",                      LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P28", "Leather Jacket",                         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P29", "Air Gauge and Pocket",                   LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P2A", "Chef Pattern",                           LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P2C", "Strapless Suntop",                       LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P2E", "Blue and Mint Green Stripes",            LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P2F", "Spotted Singlet and Necklace",           LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P30", "Pirate Purple Vest and Anchor Tattoo",   LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P31", "Pirate Strips (Red/Cream)",              LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P32", "Pirate Strips (Blue/Cream)",             LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P33", "Pirate Strips (Red/Black)",              LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P34", "Open Jacket over Striped Vest",          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P35", "Imperial Guard",                         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P36", "Pirate Captain",                         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P37", "Imperial Guard Officer",                 LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P38", "Female Pirate",                          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P39", "Pirate Open Jacket over Brown Shirt",    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P3A", "Pirate Ragged Shirt and Dagger",         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P3B", "Brown Vest, Ascot and Belt",             LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P3C", "Pirate Green Vest, Shirt and Belt",      LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P3D", "Medallion, Belt, And Silver Buttons",    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P3N", "Blue Imperial Guard",                    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P3Q", "Red Imperial Guard",                     LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P3R", "Blue Imperial Guard Officer",            LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P3S", "Red Imperial Guard Officer",             LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P40", "Castle Breastplate",                     LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P41", "Castle Chainmail",                       LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P42", "Castle Crossed Pikes",                   LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P43", "Black Falcon Pattern",                   LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P44", "Wolfman Pattern",                        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P45", "Studded Armor",                          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P46", "Forestman and Purse",                    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P47", "Castle Red/Gray Symbol",                 LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f }, 
	{ "973P48", "Forestman Maroon Collar",                LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P49", "Forestman Blue Collar",                  LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P4B", "Dragon Head",                            LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P4D", "Royal Knights Lion-Head Sheild",         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P4E", "Royal Knights Lion-Head & Neck-Chain",   LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P4G", "Castle Female Armor",                    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P4N", "Blue Castle Bodice",                     LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P4Q", "Green Castle Bodice",                    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P4R", "Tri-Colored Shield",                     LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P4S", "Suzerain Goldcrest",                     LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P4T", "Red/Peach Quarters Shield",              LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P4U", "Maroon/Red Quarters Shield",             LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P50", "Forestman Black Collar",                 LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P51", "Blacktron II",                           LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P52", "Blacktron I Pattern",                    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P54", "UFO Alien Orange and Silver",            LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P55", "Explorien Logo",                         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P60", "Shell Logo",                             LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P61", "Gold Ice Planet",                        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P62", "Silver Ice Planet",                      LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P63", "Robot Pattern",                          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P64", "Unitron Pattern",                        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P65", "Futuron Pattern",                        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P66", "Spyrius Pattern",                        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P68", "Mtron Logo",                             LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P69", "Space Police II and Radio",              LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P6B", "Black Futuron",                          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P6C", "Blue Futuron",                           LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P6D", "Red Fututon",                            LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P6E", "Yellow Futuron",                         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P70", "Bomber Jacket and Black Shirt",          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P71", "Red Necklace and Blue Undershirt",       LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P72", "Gold Necklace and Yellow Undershirt",    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P73", "Vest and Patch Pockets",                 LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P74", "Vest, Patch Pockets and Police Badge",   LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P76", "Jacket, Tie and Police Badge",           LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P77", "Modern Firefighter Type 1",              LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P78", "Modern Firefighter Type 2",              LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P7A", "Arctic Parka A1",                        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P7B", "Arctic Parka A2",                        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P83", "Suit, Tie with Train Pattern",           LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P8A", "Extreme Team Jacket Logo",               LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P8B", "RES-Q Orange Pockets and Logo",          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973P90", "Classic Space",                          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PA1", "Suspenders and Red Bowtie",              LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PA2", "Pharoah Breastplate",                    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PA3", "Safari Shirt, Gun & Red Bandana",        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PA4", "White Suit, Brown Vest and Tie",         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PA5", "Bomber Jacket, Belt and Black Shirt",    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PA6", "Safari Shirt, Blue Tee and Red Bandana", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PA7", "Safair Shirt, Black Tee and Holster",    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PA8", "Jacket, White Shirt and Necklace",       LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PAB", "Tank Top, Stains, Wrench and Tattoo",    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PAC", "Mayan Necklace, Tribal Shirt and Naval", LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PAJ", "Rock Raiders Jet",                       LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PDF", "Black Suit, Red Shirt, Gold Clasps",     LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PDG", "White Rope & Patched Collar",            LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PHB", "Purple Greatcoat",                       LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PN0", "Samurai Dragon Robe",                    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PN1", "Samurai, Sash and Dagger",               LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PN5", "Ninja Wrap, Silver Shuriken & Dagger",   LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PN6", "Ninja Wrap, Gold Shuriken, and Armour",  LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PS0", "SW Rebel A-Wing Pilot",                  LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PS1", "SW Rebel Pilot",                         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PS2", "SW Jedi Robes and Sash",                 LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PS3", "SW Wrap-Around Tunic",                   LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PS4", "SW Shirt (Open Collar, No Vest)",        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PS5", "SW Black Vest and White Shirt",          LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PS6", "SW Old Obi-Wan",                         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PS7", "SW Darth Vader",                         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PSA", "SW Rebel Mechanic",                      LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PSB", "SW Blast Armour (Green Plates)",         LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PSC", "SW Pocket-Vest and Techno-Buckle",       LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PSE", "SW Scout Trooper",                       LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PSF", "SW Tunic and Belt",                      LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PSK", "SW Stormtrooper",                        LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PSM", "SW Camouflage Smock",                    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PSN", "SW Imperial Shuttle Pilot",              LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PSQ", "SW Imperial Officer",                    LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "973PSR", "SW Protocol Droid",                      LC_MFW_TORSO, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },

	// Neck
	{ "522",   "Cape Cloth",                  LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
	{ "2524",  "Backpack Non-Opening",        LC_MFW_NECK, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "2526",  "Epaulette",                   LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
	{ "2587",  "Armor Plate",                 LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
	{ "2610",  "Lifevest",                    LC_MFW_NECK, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "3838",  "Airtanks",                    LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
	{ "3840",  "Vest",                        LC_MFW_NECK, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "4498",  "Arrow Quiver",                LC_MFW_NECK, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 180.0f },
	{ "4523",  "D-Basket",                    LC_MFW_NECK, 0.0f, 0.0f, 2.84f, 0.0f, 0.0f, 0.0f },
	{ "4524",  "Cape",                        LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
	{ "4736",  "Jet-Pack with Stud On Front", LC_MFW_NECK, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "6132",  "Hair Beard",                  LC_MFW_NECK, 0.0f, 0.0f, 2.96f, 0.0f, 0.0f, 0.0f },
	{ "30091", "Scuba Tank",                  LC_MFW_NECK, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },
	{ "30174", "Armor Samurai",               LC_MFW_NECK, 0.0f, 0.0f, 2.88f, 0.0f, 0.0f, 0.0f },

	// Arms
	{ "976", "Left Arm",  LC_MFW_LEFT_ARM, 0.0f, 0.0f, 2.56f, 0.0f, 0.0f, 0.0f },
	{ "975", "Right Arm", LC_MFW_RIGHT_ARM, 0.0f, 0.0f, 2.56f, 0.0f, 0.0f, 0.0f },

	// Hands
	{ "977", "Hand", LC_MFW_LEFT_HAND, 0.9f, -0.62f, 1.76f, 45.0f, 0.0f, 90.0f },

	// Accessories
	{ "37",      "Knife",                             LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 1.58f, 45.0f, 0.0f, 0.0f },
	{ "38",      "Harpoon",                           LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 1.0f, 45.0f, 0.0f, 0.0f },
	{ "59",      "Greatsword",                        LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.72f, 45.0f, 0.0f, 0.0f },
	{ "194",     "Hose Nozzle",                       LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.22f, 45.0f, 0.0f, 180.0f },
	{ "375",     "Ice Axe",                           LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.32f, 45.0f, 0.0f, 0.0f },
	{ "577",     "Light Sabre Hilt",                  LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.52f, 45.0f, 0.0f, 0.0f },
	{ "577C01",  "Light Sabre On",                    LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.52f, 45.0f, 0.0f, 0.0f },
	{ "774",     "Handaxe",                           LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.20f, 45.0f, 0.0f, 0.0f },
	{ "2530",    "Cutlass",                           LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.72f, 45.0f, 0.0f, 0.0f },
	{ "2542",    "Oar",                               LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 0.52f, 45.0f, 180.0f, 0.0f },
	{ "2561",    "Musket",                            LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.34f, 45.0f, 0.0f, 0.0f },
	{ "2562",    "Flintlock Pistol",                  LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.72f, 45.0f, 0.0f, 0.0f },
	{ "2570",    "Crossbow",                          LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 1.82f, 45.0f, 0.0f, 0.0f },
	{ "2614",    "Fishing Rod",                       LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.74f, 45.0f, 0.0f, 0.0f },
	{ "3841",    "Pickaxe",                           LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 2.24f, 45.0f, 0.0f, 180.0f },
	{ "3846",    "Shield",                            LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P43", "Shield Black Falcon",               LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P44", "Shield Wolfpack",                   LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P45", "Shield Black Falcon Blue Border",   LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P46", "Shield Black Falcon Yellow Border", LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P47", "Shield Red/Gray",                   LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P48", "Shield Forestman",                  LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P4C", "Shield Blue Dragon",                LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P4D", "Shield Royal Knights Lion",         LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P4E", "Shield Lion Head, Blue & Yellow",   LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P4G", "Shield Blue Lion on Yellow",        LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P4H", "Shield Yellow Lion on Blue",        LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P4T", "Shield Red/Peach Quarters",         LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3846P4U", "Shield Maroon/Red Quarters",        LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3847",    "Shortsword",                        LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.04f, 45.0f, 0.0f, 0.0f },
	{ "3848",    "Battleaxe",                         LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.04f, 45.0f, 0.0f, 180.0f },
	{ "3852",    "Hairbrush",                         LC_MFW_LEFT_TOOL, 0.82f, -0.64f, 1.98f, 45.0f, 0.0f, -90.0f },
	{ "3876",    "Shield Round",                      LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 0.0f },
	{ "3899",    "Cup",                               LC_MFW_LEFT_TOOL, -0.06f, -0.62f, 2.16f, 45.0f, 0.0f, 0.0f },
	{ "3959",    "Space Gun",                         LC_MFW_LEFT_TOOL, 0.74f, -0.62f, 2.1f, 45.0f, 0.0f, 0.0f },
	{ "3962",    "Radio",                             LC_MFW_LEFT_TOOL, 0.72f, -0.66f, 1.62f, 45.0f, 0.0f, 90.0f },
	{ "4006",    "Spanner/Screwdriver",               LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 2.18f, 45.0f, 0.0f, 180.0f },
	{ "4349",    "Loudhailer",                        LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 2.28f, 45.0f, 0.0f, 0.0f },
	{ "4360",    "Space Laser Gun",                   LC_MFW_LEFT_TOOL, 0.96f, -0.62f, 2.64f, 45.0f, 0.0f, -90.0f },
	{ "4479",    "Metal Detector",                    LC_MFW_LEFT_TOOL, 0.74f, -0.64f, 2.64f, 45.0f, 0.0f, 90.0f },
	{ "4497",    "Spear",                             LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 3.48f, 45.0f, 0.0f, 90.0f },
	{ "4499",    "Bow with Arrow",                    LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.52f, 45.0f, 0.0f, -10.0f },
	{ "4522",    "Mallet",                            LC_MFW_LEFT_TOOL, 0.72f, -0.64f, 2.72f, 45.0f, 0.0f, 0.0f },
	{ "4528",    "Frypan",                            LC_MFW_LEFT_TOOL, 0.90f, -0.62f, 2.64f, -45.0f, 90.0f, 90.0f },
	{ "4529",    "Saucepan",                          LC_MFW_LEFT_TOOL, 0.96f, -0.62f, 2.56f, -45.0f, 90.0f, 90.0f },
	{ "6124",    "Magic Wand",                        LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.32f, 45.0f, 0.0f, 0.0f },
	{ "6246A",   "Screwdriver",                       LC_MFW_LEFT_TOOL, 0.72f, -0.61f, 3.12f, 45.0f, 0.0f, 90.0f },
	{ "6246B",   "Hammer",                            LC_MFW_LEFT_TOOL, 0.72f, -0.61f, 3.12f, 45.0f, 0.0f, 90.0f },
	{ "6246D",   "Box Wrench",                        LC_MFW_LEFT_TOOL, 0.72f, -0.61f, 3.12f, 45.0f, 0.0f, 90.0f },
	{ "6246E",   "Open End Wrench",                   LC_MFW_LEFT_TOOL, 0.72f, -0.61f, 3.12f, 45.0f, 0.0f, 90.0f },
	{ "6246C",   "Power Drill",                       LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.96f, 45.0f, 0.0f, 0.0f },
	{ "30132",   "Revolver",                          LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.72f, 45.0f, 0.0f, 0.0f },
	{ "30141",   "Rifle",                             LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.72f, 45.0f, 0.0f, -10.0f },
	{ "30152",   "Magnifying Glass",                  LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 3.76f, 45.0f, 0.0f, 0.0f },
	{ "30173",   "Katana",                            LC_MFW_LEFT_TOOL, 0.72f, -0.62f, 1.72f, 45.0f, 0.0f, 0.0f },

	// Hips
	{ "970",    "Hips",                               LC_MFW_HIPS, 0.0f, 0.0f, 1.6f, 0.0f, 0.0f, 0.0f },
	{ "970P4F", "Hips with Leather Belt (Red Studs)", LC_MFW_HIPS, 0.0f, 0.0f, 1.6f, 0.0f, 0.0f, 0.0f },
	{ "970P63", "Hips with Robot Pattern",            LC_MFW_HIPS, 0.0f, 0.0f, 1.6f, 0.0f, 0.0f, 0.0f },
	{ "970PHB", "Purple Greatcoat Pattern",           LC_MFW_HIPS, 0.0f, 0.0f, 1.6f, 0.0f, 0.0f, 0.0f },
	{ "970PS5", "Hips with SW Gun Belt",              LC_MFW_HIPS, 0.0f, 0.0f, 1.6f, 0.0f, 0.0f, 0.0f },

	// Left Legs
	{ "773",    "Wooden Leg",                               LC_MFW_LEFT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "972",    "Left Leg",                                 LC_MFW_LEFT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f }, 
	{ "972PHB", "Purple Greatcoat",                         LC_MFW_LEFT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "972P3J", "Grass Skirt",                              LC_MFW_LEFT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "972P4F", "Leather Straps (Red Studs)",               LC_MFW_LEFT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "972P63", "Robot Pattern",                            LC_MFW_LEFT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "972PA2", "Green Kilt and Toes",                      LC_MFW_LEFT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },

	// Right Legs
	{ "971",    "Right Leg",                                LC_MFW_RIGHT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "971P3J", "Grass Skirt",                              LC_MFW_RIGHT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "971P4F", "Leather Straps (Red Studs)",               LC_MFW_RIGHT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "971P63", "Robot Pattern",                            LC_MFW_RIGHT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "971PA2", "Kilt and Toes",                            LC_MFW_RIGHT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "971PHB", "Purple Greatcoat",                         LC_MFW_RIGHT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
	{ "971PS5", "SW Gunbelt",                               LC_MFW_RIGHT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },

	// Footwear
	{ "2599", "Flipper", LC_MFW_LEFT_SHOE, 0.42f, -0.12f, 0.0f, 0.0f, 0.0f, 0.0f },
	{ "6120", "Ski",     LC_MFW_LEFT_SHOE, 0.42f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }

//  2447       Minifig Helmet Visor                                            
//  769        Minifig Helmet Visor Space                                      
//  30090      Minifig Diver Mask
//  { "30362", "Robot Leg", LC_MFW_LEFT_LEG, 0.0f, 0.0f, 1.12f, 0.0f, 0.0f, 0.0f },
/*
22       7  30304      Binoculars Space
25       4  30162      Binoculars Town
22   2   5  3836       Pushbroom
*/
};

/*
981.DAT       Minifig Arm Left                                                
982.DAT       Minifig Arm Right                                               
33009.DAT     Minifig Book                                                    
30148.DAT     Minifig Camera Movie                                            
30089.DAT     Minifig Camera Snapshot                                         
4360.DAT      Minifig Camera with Side Sight                                  
30170.DAT     Minifig Cap Aviator Goggles                                     
208C01.DAT    Minifig Chain 17L (Complete)                                    
30090.DAT     Minifig Diver Mask                                              
2343.DAT      Minifig Goblet                                                  
983.DAT       Minifig Hand                                                    
3849.DAT      Minifig Lance                                                   
30377.DAT     Minifig Mechanical Arm                                          
30378.DAT     Minifig Mechanical Head SW Battle Droid                         
30378PS1.DAT  Minifig Mechanical Head with Orange Insignia Pattern            
30378PS2.DAT  Minifig Mechanical Head with Rust Insignia Pattern              
30376.DAT     Minifig Mechanical Legs                                         
30375CS0.DAT  Minifig Mechanical SW Battle Droid (Shortcut)                   
30375CS1.DAT  Minifig Mechanical SW Battle Droid Commander (Shortcut)         
30375CS3.DAT  Minifig Mechanical SW Battle Droid Pilot (Shortcut)             
30375CS2.DAT  Minifig Mechanical SW Battle Droid Security (Shortcut)          
30375.DAT     Minifig Mechanical Torso                                        
30375PS3.DAT  Minifig Mechanical Torso with Blue Insignia Pattern             
30375PS1.DAT  Minifig Mechanical Torso with Orange Insignia Pattern           
30375PS2.DAT  Minifig Mechanical Torso with Tan Insignia Pattern              
4502A.DAT     Minifig Plume Small                                             
6123.DAT      Minifig Polearm Halberd                                         
3962B.DAT     Minifig Radio with Long Handle                                  
3962A.DAT     Minifig Radio with Short Handle                                 
30362.DAT     Minifig Robot Leg                                               
30154.DAT     Minifig Sextant                                                 
3837.DAT      Minifig Shovel                                                  
3900.DAT      Minifig Signal Holder                                           
6260C01.DAT   Minifig Skeleton (Shortcut)                                     
6265.DAT      Minifig Skeleton Arm                                            
6266.DAT      Minifig Skeleton Leg                                            
82359.DAT     Minifig Skeleton Skull                                          
6260.DAT      Minifig Skeleton Torso                                          
770.DAT       Minifig Shield Ovoid                                            
770PW1.DAT    Minifig Shield Ovoid with American Indian Pattern               
770P4C.DAT    Minifig Shield Ovoid with Blue Dragon Pattern                   
770PH1.DAT    Minifig Shield Ovoid with Golden Lion                           
770P4B.DAT    Minifig Shield Ovoid with Green Dragon Pattern                  
770P4D.DAT    Minifig Shield Ovoid with Royal Knights Lion Pattern            
770PS1.DAT    Minifig Shield Ovoid with SW Gungans Patrol Shield Pattern      
30088.DAT     Minifig Speargun                                                
4449.DAT      Minifig Suitcase                                                
3836.DAT      Minifig Tool Pushbroom                                          
30092.DAT     Minifig Underwater Scooter                                      
2488.DAT      Minifig Whip                                                    
*/

static int mfw_pieces = sizeof (mfw_pieceinfo)/sizeof (LC_MFW_PIECEINFO);

// =============================================================================
// MinifigWizard class

MinifigWizard::MinifigWizard (GLWindow *share)
	: GLWindow (share)
{
	const unsigned char colors[LC_MFW_NUMITEMS] = { 0, 6, 4, 22, 0, 0, 6, 6, 22, 22, 9, 9, 9, 22, 22 };
	const char *pieces[LC_MFW_NUMITEMS] = { "3624", "3626BP01", "973", "None", "976", "975", "977", "977",
	                                        "None", "None", "970", "972", "971", "None", "None" };
	int i;

	for (i = 0; i < LC_MFW_NUMITEMS; i++)
	{
		m_Colors[i] = colors[i];
		m_Angles[i] = 0;

		m_Info[i] = lcGetPiecesLibrary()->FindPieceInfo(pieces[i]);
		if (m_Info[i] != NULL)
			m_Info[i]->AddRef();
	}

	m_MinifigCount = 0;
	m_MinifigNames = NULL;
	m_MinifigTemplates = NULL;

	i = Sys_ProfileLoadInt("MinifigWizard", "Version", 1);
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

void MinifigWizard::OnDraw ()
{
	int i;

	if (!MakeCurrent ())
		return;

	float aspect = (float)m_nWidth/(float)m_nHeight;
	glViewport (0, 0, m_nWidth, m_nHeight);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (30.0f, aspect, 1.0f, 20.0f);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	gluLookAt (0, -9, 4, 0, 5, 1, 0, 0, 1);
	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float *bg = lcGetActiveProject()->GetBackgroundColor();
	glClearColor (bg[0], bg[1], bg[2], bg[3]);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable (GL_DITHER);
	glShadeModel (GL_FLAT);

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

	SwapBuffers ();
}

void MinifigWizard::Calculate ()
{
	float pos[LC_MFW_NUMITEMS][3];
	float rot[LC_MFW_NUMITEMS][3];
	Matrix mat, m2, m3;

	// Get the pieces in the right place
	for (int type = 0; type < LC_MFW_NUMITEMS; type++)
	{
		PieceInfo* piece_info = m_Info[type];
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

		switch (type)
		{
		case LC_MFW_HAT:
		case LC_MFW_HEAD:
			if (m_Info[LC_MFW_NECK] != NULL)
			{
				if (strcmp (m_Info[LC_MFW_NECK]->m_strName, "522") == 0) // Cape Cloth
					pos[type][2] += 0.02f;
				else if (strcmp (m_Info[LC_MFW_NECK]->m_strName, "30174") == 0) // Armor Samurai
					pos[type][2] += 0.04f;
				else
					pos[type][2] += 0.08f;
			}
			break;

		case LC_MFW_RIGHT_HAND:
		case LC_MFW_RIGHT_SHOE:
			pos[type][0] = -pos[type][0];
			break;

		case LC_MFW_RIGHT_TOOL:
			if ((strcmp (piece_info->m_strName, "4499") == 0) || // Bow with Arrow
			    (strcmp (piece_info->m_strName, "30141") == 0))  // Rifle
				rot[type][2] = -rot[type][2];
			break;

		case LC_MFW_LEFT_LEG:
			if (strcmp (piece_info->m_strName, "773") == 0) // Wooden Leg
			{
				rot[type][2] += 180;//= -pos[type][0];
//				pos[type][0] += 0.8f;
			}
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
	mat.LoadIdentity();
	mat.CreateOld(0,0,0,rot[LC_MFW_NECK][0], rot[LC_MFW_NECK][1], rot[LC_MFW_NECK][2]);
	mat.Rotate(m_Angles[LC_MFW_NECK], 0, 0, -1);
	mat.SetTranslation (pos[LC_MFW_NECK][0], pos[LC_MFW_NECK][1], pos[LC_MFW_NECK][2]);
	mat.ToAxisAngle(m_Rotation[LC_MFW_NECK]);
	mat.GetTranslation(m_Position[LC_MFW_NECK]);

	// torso
	m_Position[LC_MFW_TORSO][0] = pos[LC_MFW_TORSO][0];
	m_Position[LC_MFW_TORSO][1] = pos[LC_MFW_TORSO][1];
	m_Position[LC_MFW_TORSO][2] = pos[LC_MFW_TORSO][2];
	m_Rotation[LC_MFW_TORSO][0] = 0.0f;
	m_Rotation[LC_MFW_TORSO][1] = 0.0f;
	m_Rotation[LC_MFW_TORSO][2] = 1.0f;
	m_Rotation[LC_MFW_TORSO][3] = 0.0f;

	// left arm/hand/tool
	mat.LoadIdentity();
	mat.Rotate(m_Angles[LC_MFW_LEFT_ARM], -1, 0, 0);
	mat.SetTranslation(pos[LC_MFW_LEFT_ARM][0], pos[LC_MFW_LEFT_ARM][1], pos[LC_MFW_LEFT_ARM][2]);
	mat.ToAxisAngle(m_Rotation[LC_MFW_LEFT_ARM]);
	mat.GetTranslation(m_Position[LC_MFW_LEFT_ARM]);

	mat.Translate(pos[LC_MFW_LEFT_HAND][0]-pos[LC_MFW_LEFT_ARM][0],
	              pos[LC_MFW_LEFT_HAND][1]-pos[LC_MFW_LEFT_ARM][1],
	              pos[LC_MFW_LEFT_HAND][2]-pos[LC_MFW_LEFT_ARM][2]);
	m2.CreateOld(0,0,0,rot[LC_MFW_LEFT_HAND][0],rot[LC_MFW_LEFT_HAND][1],rot[LC_MFW_LEFT_HAND][2]);
	m3.Multiply(mat, m2);
	m3.Translate(0,0,-0.16f);
	mat.LoadIdentity();
	mat.Translate(0,0,0.16f);
	mat.Rotate(m_Angles[LC_MFW_LEFT_HAND], 1, 0, 0);
	m2.Multiply(m3, mat);
	m2.ToAxisAngle(m_Rotation[LC_MFW_LEFT_HAND]);
	m2.GetTranslation(m_Position[LC_MFW_LEFT_HAND]);

	if (m_Info[LC_MFW_LEFT_TOOL] != NULL)
	{
		m2.Translate(pos[LC_MFW_LEFT_TOOL][0]-0.9f,
		             pos[LC_MFW_LEFT_TOOL][1]-pos[LC_MFW_LEFT_HAND][1],
		             pos[LC_MFW_LEFT_TOOL][2]-pos[LC_MFW_LEFT_HAND][2]);
		m3.CreateOld(0,0,0,rot[LC_MFW_LEFT_TOOL][0]-rot[LC_MFW_LEFT_HAND][0],
		             rot[LC_MFW_LEFT_TOOL][1]-rot[LC_MFW_LEFT_HAND][1],
		             rot[LC_MFW_LEFT_TOOL][2]-rot[LC_MFW_LEFT_HAND][2]);
		mat.Multiply (m2, m3);
		m2.LoadIdentity ();

		// Center the rotation points
		if (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "3852") == 0) // Hairbrush
			mat.Translate (0.11f, 0, 0);
		else if (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "3899") == 0) // Cup
			mat.Translate (0, 0.8f, 0);
		else if (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "4360") == 0) // Space Laser Gun
			mat.Translate (0.26f, 0, 0);
		else if (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "4528") == 0) // Frypan
			mat.Translate (0, 0, -0.16f);
		else if (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "4529") == 0) // Saucepan
			mat.Translate (0, 0, -0.24f);

		// Saucepan and Frypan have a different rotation axis
		if ((strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "4528") == 0) ||
	      (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "4529") == 0))
			m2.Rotate (m_Angles[LC_MFW_LEFT_TOOL], 0, -1, 0);
		else
			m2.Rotate (m_Angles[LC_MFW_LEFT_TOOL], 0, 0, 1);

		m3.Multiply (mat, m2);

		if (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "3852") == 0) // Hairbrush
			m3.Translate (-0.11f, 0, 0);
		else if (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "3899") == 0) // Cup
			m3.Translate (0, -0.8f, 0);
		else if (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "4360") == 0) // Space Laser Gun
			m3.Translate (-0.26f, 0, 0);
		else if (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "4528") == 0) // Frypan
			m3.Translate (0, 0, 0.16f);
		else if (strcmp (m_Info[LC_MFW_LEFT_TOOL]->m_strName, "4529") == 0) // Saucepan
			m3.Translate (0, 0, 0.24f);

		m3.ToAxisAngle (m_Rotation[LC_MFW_LEFT_TOOL]);
		m3.GetTranslation (m_Position[LC_MFW_LEFT_TOOL]);
	}

	// right arm/hand/tool
	mat.LoadIdentity (); m2.LoadIdentity (); m3.LoadIdentity ();
	mat.Rotate (m_Angles[LC_MFW_RIGHT_ARM], -1, 0, 0);
	mat.SetTranslation (pos[LC_MFW_RIGHT_ARM][0], pos[LC_MFW_RIGHT_ARM][1], pos[LC_MFW_RIGHT_ARM][2]);
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

	if (m_Info[LC_MFW_RIGHT_TOOL] != NULL)
	{
		m2.Translate (pos[LC_MFW_RIGHT_TOOL][0]-0.9f,
		              pos[LC_MFW_RIGHT_TOOL][1]-pos[LC_MFW_RIGHT_HAND][1],
		              pos[LC_MFW_RIGHT_TOOL][2]-pos[LC_MFW_RIGHT_HAND][2]);
		m3.CreateOld (0,0,0,rot[LC_MFW_RIGHT_TOOL][0]-rot[LC_MFW_RIGHT_HAND][0],
		              rot[LC_MFW_RIGHT_TOOL][1]-rot[LC_MFW_RIGHT_HAND][1],
		              rot[LC_MFW_RIGHT_TOOL][2]-rot[LC_MFW_RIGHT_HAND][2]);
		mat.Multiply (m2, m3);
		m2.LoadIdentity ();

		// Center the rotation points
		if (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "3852") == 0) // Hairbrush
			mat.Translate (0.11f, 0, 0);
		else if (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "3899") == 0) // Cup
			mat.Translate (0, 0.8f, 0);
		else if (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "4360") == 0) // Space Laser Gun
			mat.Translate (0.26f, 0, 0);
		else if (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "4528") == 0) // Frypan
			mat.Translate (0, 0, -0.16f);
		else if (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "4529") == 0) // Saucepan
			mat.Translate (0, 0, -0.24f);

		// Saucepan and Frypan have a different rotation axis
		if ((strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "4528") == 0) ||
		    (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "4529") == 0))
			m2.Rotate (m_Angles[LC_MFW_RIGHT_TOOL], 0, -1, 0);
		else
			m2.Rotate (m_Angles[LC_MFW_RIGHT_TOOL], 0, 0, 1);

		m3.Multiply (mat, m2);

		if (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "3852") == 0) // Hairbrush
			m3.Translate (-0.11f, 0, 0);
		else if (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "3899") == 0) // Cup
			m3.Translate (0, -0.8f, 0);
		else if (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "4360") == 0) // Space Laser Gun
			m3.Translate (-0.26f, 0, 0);
		else if (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "4528") == 0) // Frypan
			m3.Translate (0, 0, 0.16f);
		else if (strcmp (m_Info[LC_MFW_RIGHT_TOOL]->m_strName, "4529") == 0) // Saucepan
			m3.Translate (0, 0, 0.24f);

		m3.ToAxisAngle (m_Rotation[LC_MFW_RIGHT_TOOL]);
		m3.GetTranslation (m_Position[LC_MFW_RIGHT_TOOL]);
	}

	// hips
	m_Position[LC_MFW_HIPS][0] = pos[LC_MFW_HIPS][0];
	m_Position[LC_MFW_HIPS][1] = pos[LC_MFW_HIPS][1];
	m_Position[LC_MFW_HIPS][2] = pos[LC_MFW_HIPS][2];
	m_Rotation[LC_MFW_HIPS][0] = 0.0f;
	m_Rotation[LC_MFW_HIPS][1] = 0.0f;
	m_Rotation[LC_MFW_HIPS][2] = 1.0f;
	m_Rotation[LC_MFW_HIPS][3] = 0.0f;

	// left leg/shoe
	mat.CreateOld (0,0,0,rot[LC_MFW_LEFT_LEG][0],rot[LC_MFW_LEFT_LEG][1],rot[LC_MFW_LEFT_LEG][2]);
	m2.LoadIdentity ();
	m2.Rotate (m_Angles[LC_MFW_LEFT_LEG], -1, 0, 0);
	m2.SetTranslation (pos[LC_MFW_LEFT_LEG][0], pos[LC_MFW_LEFT_LEG][1], pos[LC_MFW_LEFT_LEG][2]);
	m3.Multiply (m2, mat);
	m3.ToAxisAngle (m_Rotation[LC_MFW_LEFT_LEG]);
	m3.GetTranslation (m_Position[LC_MFW_LEFT_LEG]);
	m3.Translate (pos[LC_MFW_LEFT_SHOE][0]-pos[LC_MFW_LEFT_LEG][0],
	              pos[LC_MFW_LEFT_SHOE][1]-pos[LC_MFW_LEFT_LEG][1],
	              pos[LC_MFW_LEFT_SHOE][2]-pos[LC_MFW_LEFT_LEG][2]);
	if (strcmp (m_Info[LC_MFW_LEFT_LEG]->m_strName, "773") == 0)
		m3.Translate (-0.8f, 0, 0);
	mat.CreateOld (0,0,0,rot[LC_MFW_LEFT_SHOE][0]-rot[LC_MFW_LEFT_LEG][0],
	               rot[LC_MFW_LEFT_SHOE][1]-rot[LC_MFW_LEFT_LEG][1],
	               rot[LC_MFW_LEFT_SHOE][2]-rot[LC_MFW_LEFT_LEG][2]);
	m2.Multiply (m3, mat);
	m3.LoadIdentity ();
	m3.Rotate (m_Angles[LC_MFW_LEFT_SHOE], 0, 0, 1);
	mat.Multiply (m2, m3);
	mat.ToAxisAngle (m_Rotation[LC_MFW_LEFT_SHOE]);
	mat.GetTranslation (m_Position[LC_MFW_LEFT_SHOE]);

	// right leg/shoe
	mat.LoadIdentity ();
	mat.Rotate (m_Angles[LC_MFW_RIGHT_LEG], -1, 0, 0);
	mat.SetTranslation (pos[LC_MFW_RIGHT_LEG][0], pos[LC_MFW_RIGHT_LEG][1], pos[LC_MFW_RIGHT_LEG][2]);
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

		piece_info = lcGetPiecesLibrary()->FindPieceInfo(mfw_pieceinfo[i].name);
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

		case LC_MFW_RIGHT_LEG:
			if ((mfw_pieceinfo[i].type != LC_MFW_RIGHT_LEG) &&
				((mfw_pieceinfo[i].type != LC_MFW_LEFT_LEG) ||
				(strcmp (mfw_pieceinfo[i].name, "773") != 0))) // Wooden Leg
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
		if (strcmp(desc, mfw_pieceinfo[j].description) == 0)
		{
			piece_info = lcGetPiecesLibrary()->FindPieceInfo(mfw_pieceinfo[j].name);
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
    m_Info[j] = lcGetPiecesLibrary()->FindPieceInfo (ptr);
    *endptr = ' ';
    ptr = endptr;

    if (m_Info[j] != NULL)
      m_Info[j]->AddRef();
  }

  for (j = 0; j < LC_MFW_NUMITEMS; j++)
    m_Angles[j] = (float)strtod (ptr, &ptr);

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

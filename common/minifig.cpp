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

// Settings for the 2010.3 update (from holly-wood@holly-wood.it).
static const char* DefaultSettings = 
	"[HATS]\n"
	"\"Cap\" \"4485.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Cap Aviator\" \"30171.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Castle Helmet with Chin-Guard\" \"3896.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Castle Helmet with Neck Protector\" \"3844.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Construction Helmet\" \"3833.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Cook's Hat\" \"3898.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Fire Helmet\" \"3834.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Forestman Cap\" \"775.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Forestman Cap with Small Plume (Shortcut)\" \"775c01.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hair Female with Pigtails\" \"390.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hair Male\" \"3901.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hair Long Straight\" \"40239.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hair Long with Headband\" \"30114.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hair Long with Headband and Feathers\" \"30114C01.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hair Ponytail\" \"6093a.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hair Ponytail with Long Bangs\" \"62696.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hair Shoulder Length\" \"4530.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hair Spiky Long\" \"53982.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hair Spiky Short\" \"53981.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Bicorne\" \"2528.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Cowboy\" \"3629.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Cowboy with Cavalry Logo\" \"3629PW1.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Cowboy with Silver Star\" \"3629PW2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Crown\" \"71015.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Imperial Guard Shako\" \"2545.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Fez\" \"85975.dat\" 0 1 0 0 0 1 0 0 0 1 0 -14 0\n"
	"\"Hat Kepi\" \"30135.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Knit Cap\" \"41334.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Rag\" \"2543.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Tricorne\" \"2544.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Tricorne with Plume\" \"2544C01.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hat Wide Brim Flat\" \"30167.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Alien Skull with Fangs\" \"85945.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Castle with Fixed Face Grille\" \"4503.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Classic with Thin Chin Guard\" \"3842a.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Classic with Thick Chin Guard\" \"3842b.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Darth Vader\" \"30368.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Imperial AT-ST Pilot\" \"57900.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Modern\" \"2446.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Morion\" \"30048.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Samurai\" \"30175.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Samurai with Horn (Shortcut)\" \"30175c01.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet Skateboard\" \"46303.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Helmet with Chinstrap and Wide Brim\" \"30273.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Police Hat\" \"3624.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Top Hat\" \"3878.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Wizards Hat\" \"6131.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[HEAD]\n"
	"\"Stud Solid\" \"3626A.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Stud Solid with Standard Grin Pattern\" \"3626AP01.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Standard Grin Pattern\" \"3626BP01.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Adventurers Mummy Pattern\" \"3626BPA2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Alien with Green Brain and Yellow Mouth\" \"3626BP6Y.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Alien with Large Blue Mask\" \"3626BP6V.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Alien with Silver Mask and Mouth Grille\" \"3626BP6X.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Alien with Small Blue Mask\" \"3626BP6W.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Sunglasses Pattern\" \"3626bp7b.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Wrap-Around Sunglasses Pattern\" \"3626bp7c.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Brown Hair over Eye and Black Eyebrows Pattern\" \"3626BP7A.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Brown Hair, Eyelashes, and Lipstick Pattern\" \"3626BPA6.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Dark Grey Facial Hair Pattern\" \"3626BP39.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Evil Skeleton Skull Pattern\" \"3626BPA8.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Glasses and White Muttonchops Pattern\" \"3626BPA1.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Gold Robot Pattern\" \"3626BP64.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Half-Moon Glasses and Grey Eyebrows Pattern\" \"3626BPHA.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Headset Over Brown Hair & Eyebrows Pattern\" \"3626BP69.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Ice Planet Female Red Hair Pattern\" \"3626BP65.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Ice Planet Messy White Hair\" \"3626BP62.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Ice Planet Moustache and Eyebrows Pattern\" \"3626BP61.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Islander White/Blue Painted Face Pattern\" \"3626BP3K.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Islander White/Red Painted Face Pattern\" \"3626BP3J.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Messy Hair and Eye Patch Pattern\" \"3626BP31.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Messy Hair and Moustache Pattern\" \"3626BP30.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Messy Hair Female Pattern\" \"3626BP40.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Messy Hair, Beard and Eye Patch Pattern\" \"3626BP34.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Messy Hair, Moustache, and Beard Pattern\" \"3626BP33.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Messy Hair, Moustache, and Eye Patch Pattern\" \"3626BP32.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Monocle, Scar, and Moustache Pattern\" \"3626BPA7.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Ron Weasley Pattern\" \"3626bph3.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Pursed Lips and White Forehead Pattern\" \"3626BPB1.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Lips and Black Upswept Eyelashes Pattern\" \"3626bp6f.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Severus Snape Pattern\" \"3626BPHB.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Sideburns and Droopy Moustache Black Pattern\" \"3626BP3N.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Sideburns and Droopy Moustache Brown Pattern\" \"3626BP3Q.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Silver Robot Pattern\" \"3626BP63.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Skull Type 1 (Happy) Pattern\" \"82359.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Smirk & Black Moustache Pattern\" \"3626BPA3.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Standard Grin and Eyebrows Pattern\" \"3626BP05.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Standard Grin and Pointed Moustache Pattern\" \"3626BP03.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Standard Grin and Sunglasses Pattern\" \"3626BP04.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Standard Grin, Eyebrows and Microphone Pattern\" \"3626BP06.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Standard Woman Pattern\" \"3626BP02.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Stubble, Moustache and Smirk Pattern\" \"3626BPA5.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Alien with Large Black Eyes Pattern\" \"3626BPSB.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Black Eyebrows and Scars Pattern\" \"3626BPS7.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Brown Eyebrows and Beard Pattern\" \"3626BPS9.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Brown Eyebrows Pattern\" \"3626BPS2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Darth Maul Pattern\" \"3626BPS8.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Grey Beard and Eyebrows Pattern\" \"3626BPS4.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Grey Eyebrows & Implant Pattern\" \"3626BPSC.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Scout Trooper Black Visor Pattern\" \"3626BPSE.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Small Black Eyebrows Pattern\" \"3626BPS3.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Smirk and Brown Eyebrows Pattern\" \"3626BPS5.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Tusken Raider Pattern\" \"3626BPST.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Tan Eyebrows and Frown\" \"3626BPH2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Tribal Paint and Frown Pattern\" \"3626BPAC.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Villian Black Facial Hair Pattern\" \"3626BPA4.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Villainous Glasses & Black Facial Hair Pattern\" \"3626bpa9.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vincent Crabbe/Ron Weasley Pattern\" \"3626bph5.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"White Hair, Eyebrows, and Moustache Pattern\" \"3626BPN1.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Wiry Moustache, Goatee and Eyebrows Pattern\" \"3626bp3e.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Head SW Battle Droid\" \"30378.DAT\" 0 1 0 0 0 1 0 0 0 1 0 32 0\n"
	"\"Stud Hollow\" \"3626B.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[BODY]\n"
	"\"Plain\" \"973.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"'S' Logo Grey / Blue Pattern\" \"973P23.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"'S' Logo Red / Black Pattern\" \"973P14.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"2 Chinese Letters Yellow Stripe Pattern\" \"973PAQ.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"3-Piece Suit, White Shirt and Red Tie Pattern\" \"973PDB.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Air Gauge and Pocket Pattern\" \"973P29.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Airplane Logo and 'AIR' Badge Pattern\" \"973P85.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Airplane Logo Pattern\" \"973P16.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Anchor Motif Pattern\" \"973P09.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Arctic Parka A1 Pattern\" \"973P7A.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Arctic Parka A2 Pattern\" \"973P7B.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Astro Pattern\" \"973P6F.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Autoroute Pattern\" \"973P27.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Black Diagonal Zip and Pocket Pattern\" \"973p0b.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Black Dungaree Pattern\" \"973P1A.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Black Falcon Pattern\" \"973P43.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Black Futuron Pattern\" \"973P6B.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blacktron I Pattern\" \"973P52.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blacktron II Pattern\" \"973P51.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue and Mint Green Stripes Pattern\" \"973p2e.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Castle Bodice Pattern\" \"973P4N.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Dungaree Pattern\" \"973P1B.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Futuron Pattern\" \"973P6C.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Flowers on Tied Shirt Pattern\" \"973P2G.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Horizontal Stripes Pattern\" \"973p1d.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Imperial Guard Officer Pattern\" \"973P3R.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Imperial Guard Pattern\" \"973P3N.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Shirt and Safety Stripes Pattern\" \"973P8G.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Striped Dungarees Pattern\" \"973P1R.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Blue Undershirt Green Bow and Gun Pattern\" \"973PW5.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Bomber Jacket & Black Shirt Pattern\" \"973P70.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Bomber Jacket, Belt, & Black Shirt Pattern\" \"973PA5.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Brown Vest, Ascot and Belt Pattern\" \"973P3B.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Brown Vest, Buckle and String Bowtie Pattern\" \"973PW9.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Buttons and Old Police Badge Pattern\" \"973P1F.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Card, Suit, Vest, and Gold Fob Pattern\" \"973PW8.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Castle Breastplate Pattern\" \"973P40.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Castle Bodice and Cloak Pattern\" \"973P4H.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Castle Chainmail Pattern\" \"973P41.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Castle Crossed Pikes Pattern\" \"973P42.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Castle Female Armor Pattern\" \"973P4G.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Castle Red/Gray Pattern\" \"973P47.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Chef Pattern\" \"973p2a.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Classic Space Pattern\" \"973P90.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"DkGray, Black, and Yellow Batman Pattern\" \"973PB1.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Dragon Head Pattern\" \"973P4B.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Explorien Logo Pattern\" \"973p55.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Extreme Team Jacket Logo Pattern\" \"973P8A.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Female Pirate Pattern\" \"973P38.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Five Button Fire Fighter Pattern\" \"973P21.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Forestman and Purse Pattern\" \"973P46.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Forestman Black Collar Pattern\" \"973P50.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Forestman Blue Collar Pattern\" \"973P49.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Forestman Maroon Collar Pattern\" \"973P48.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Four Button Suit and Train Logo Pattern\" \"973P84.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Gold Fob and 100 Dollar Bills Pattern\" \"973PWA.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Gold Ice Planet Pattern\" \"973P61.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Gold Necklace and Yellow Undershirt Pattern\" \"973P72.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Green Castle Bodice Pattern\" \"973P4Q.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Green Dungaree Pattern\" \"973P1J.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Gryffindor Uniform Pattern\" \"973PH1.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hogwarts Uniform Pattern\" \"973PH0.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Jacket, Orange Vest, Green Neck-chief Pattern\" \"973PB3.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Jacket, Tie and Police Badge Pattern\" \"973p76.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Jacket, White Shirt, and Necklace Pattern\" \"973PA8.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Launch Command Logo and Equipment Pattern\" \"973p1q.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Leather Jacket Pattern\" \"973P28.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Leather Jacket and Light Gray Shirt Pattern\" \"973PA9.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Lifebelt Logo and ID Card Pattern\" \"973P79.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Maroon/Red Quarters Shield Pattern\" \"973P4U.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mayan Necklace, Tribal Shirt, & Navel Pattern\" \"973PAC.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Medallion, Belt, And Silver Buttons Pattern\" \"973p3d.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Modern Firefighter Type 1 Pattern\" \"973p77.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Modern Firefighter Type 2 Pattern\" \"973p78.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"MTron Logo Pattern\" \"973P68.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Ninja Wrap, Gold Shuriken, and Armour Pattern\" \"973PN6.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Ninja Wrap, Silver Shuriken & Dagger Pattern\" \"973PN5.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Octan OIL Badge\" \"973P81.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Octan Logo Pattern\" \"973PT2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Old Obi-Wan Pattern\" \"973PS6.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Open Jacket over Striped Vest Pattern\" \"973P34.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Patch Pocket Pattern\" \"973P26.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Pharaoh Breastplate Pattern\" \"973PA2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Pirate Captain Pattern\" \"973P36.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Pirate Green Vest, Shirt, and Belt Pattern\" \"973P3C.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Pirate Open Jacket over Brown Vest Pattern\" \"973P39.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Pirate Purple Vest and Anchor Tattoo Pattern\" \"973P30.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Pirate Ragged Shirt and Dagger Pattern\" \"973P3A.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Pirate Stripes Pattern\" (Blue/Cream) \"973P32.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Pirate Stripes Pattern\" (Red/Black) \"973P33.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Pirate Stripes Pattern\" (Red/Cream) \"973P31.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Plain Shirt with Pockets Pattern\" \"973P7G.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Purple Greatcoat Pattern\" \"973phb.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Racing Jacket and Two Stars Pattern\" \"973P1H.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Racing Jacket and Two Stars Red Pattern\" \"973P1N.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Cross and Stethoscope Pattern\" \"973P25.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Cross Pattern\" \"973P24.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Dungaree Pattern\" \"973P1C.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Futuron Pattern\" \"973P6D.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Horizontal Stripes Pattern\" \"973p1E.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Imperial Guard Officer Pattern\" \"973P3S.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Imperial Guard Pattern\" \"973P3Q.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Necklace and Blue Undershirt Pattern\" \"973P71.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Shirt and Suit Pattern\" \"973P22.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Undershirt and Fringe Pattern\" \"973PW7.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red V-Neck and Buttons Pattern\" \"973P17.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red Vest and Train Logo Pattern\" \"973P82.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Red/Peach Quarters Shield Pattern\" \"973P4T.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"RES-Q Orange Pockets and Logo Pattern\" \"973P8B.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Riding Jacket Pattern\" \"973P12.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Robot Pattern\" \"973P63.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Rock Raiders Jet Pattern\" \"973PAJ.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Royal Knights Lion Head & Neck-Chain Pattern\" \"973P4E.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Royal Knights Lion-Head Shield Pattern\" \"973P4D.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Safari Shirt, Black Tee, and Holster Pattern\" \"973PA7.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Safari Shirt, Gun & Red Bandana Pattern\" \"973PA3.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Safari Shirt,Blue Tee & Red Bandana Pattern\" \"973PA6.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Samurai Dragon Robe Pattern\" \"973PN0.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Samurai Robe, Sash and Dagger Pattern\" \"973PN1.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Shell Logo Pattern\" \"973P60.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Sheriff Pattern\" \"973PW4.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Silver Ice Planet Pattern\" \"973P62.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Six Button Suit and Airplane Pattern\" \"973p04.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Six Button Suit and Anchor Pattern\" \"973p05.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Slytherin Uniform Pattern\" \"973PH2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Space Police II and Radio Pattern\" \"973P69.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Space Police II Chief\" \"973P6A.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Spotted Singlet and Necklace Pattern\" \"973p2f.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Spyrius Pattern\" \"973P66.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Stethoscope and Pocket with Pens Pattern\" \"973P86.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Straight Zipper Jacket Pattern\" \"973P13.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Strapless Suntop Pattern\" \"973P2C.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Striped Shirt and Silver Buttons Pattern\" \"973P3F.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Studded Armor Pattern\" \"973P45.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Suit and Tie Pattern\" \"973P18.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Suit and Tie with Train Pattern\" \"973P83.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Suspenders and Red Bowtie Pattern\" \"973PA1.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Black Vest & White Shirt Pattern\" \"973PS5.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Blast Armor (Green Plates) Pattern\" \"973PSB.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Blast Armor (Silver Plates) Pattern\" \"973PSJ.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Camouflage Smock Pattern\" \"973PSM.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Darth Vader Pattern\" \"973PS7.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Hoth Trooper Pattern\" \"973PSH.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Imperial Officer Pattern\" \"973PSQ.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Imperial Shuttle Pilot Pattern\" \"973PSN.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Jawa Pattern\" \"973PSS.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Jedi Robes and Sash Pattern\" \"973PS2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Moisture Farmer Pattern\" \"973PSV.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Pocket-Vest and Techno-Buckle Pattern\" \"973PSC.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Protocol Droid Pattern\" \"973psr.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Rebel A-Wing Pilot Pattern\" \"973PS0.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Rebel Mechanic Pattern\" \"973PSA.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Rebel Pilot Pattern\" \"973PS1.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Scout Trooper Pattern\" \"973PSE.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Shirt (Open Collar, No Vest) Pattern\" \"973PS4.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Stormtrooper Pattern\" \"973PSK.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Tunic and Belt Pattern\" \"973psf.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Wrap-Around Tunic Pattern\" \"973PS3.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Tank Top, Stains, Wrench, and Tattoo Pattern\" \"973PAB.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Telephone\" \"973P8F.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Train Chevron Pattern\" \"973P19.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Tri-Coloured Shield and Gold Trim Pattern\" \"973p4s.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Tri-Coloured Shield Large Pattern\" \"973P4R.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"TV Logo Pattern Large\" \"973p1m.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"TV Logo Pattern Small\" \"973P1K.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"UFO Alien Orange and Silver Pattern\" \"973P54.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"UFO Alien Triangular Insignia\" \"973P6U.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"UFO Alien Yellow Insignia, 3 Blue Bars\" \"973P6V.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"UFO Alien Circuitry, Red Level\" \"973P6W.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"UFO Silver and Gold Circuitry\" \"973P6X.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Unitron Pattern\" \"973P64.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"US Cavalry General Pattern\" \"973PW1.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"US Cavalry Officer Pattern\" \"973PW2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"US Cavalry Soldier Pattern\" \"973PW3.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vertical Striped Blue/Red Pattern\" \"973P02.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vertical Striped Red/Blue Pattern\" \"973P01.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest with Patch Pockets Pattern\" \"973P73.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest, Patch Pockets and Police Badge Pattern\" \"973P74.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Waiter Pattern\" \"973P20.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"White Braces and Cartridge Belt Pattern\" \"973PW6.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"White Diagonal Zip and Pocket Pattern\" \"973p0a.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"White Rope & Patched Collar Pattern\" \"973pdg.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0 \n"
	"\"White Shirt and Jacket Pattern\" \"973P03.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"White Suit, Brown Vest and Tie Pattern\" \"973PA4.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Windsurfboard Pattern\" \"973P2D.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Wolfpack Pattern\" \"973P44.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Yellow Futuron Pattern\" \"973P6E.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Zipper and Old Police Badge Pattern\" \"973P1G.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Zipper Jacket and Police Logo\" \"973P75.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Torso\" \"30375.DAT\" 0 1 0 0 0 1 0 0 0 1 0 40 0\n"
	"\"Mechanical Torso with 4 Side Attachment Cylinders\" \"54275.DAT\" 0 1 0 0 0 1 0 0 0 1 0 10 0\n"
	"\"Skeleton Torso\" \"6260.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Torso Old\" \"17.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[BODY2]\n"
	"\"Plain\" \"3815.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Gold Belt and Orange Cable\" \"3815P6U.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Leather Belt (Red Studs) Pattern\" \"3815p4f.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Purple Greatcoat Pattern\" \"970phb.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Robot Pattern\" \"970P63.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Silver Belt\" \"3815P6W.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Silver Belt and Salmon Cable\" \"3815P6V.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Gunbelt Pattern\" \"970PS5.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hips and Legs Short\" \"41879.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Legs Old\" \"15.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Legs\" \"30376.DAT\" 0 1 0 0 0 1 0 0 0 1 0 46 0\n"
	"\"Slope Brick 65 2 x 2 x 2\" \"3678a.DAT\" 0 -1 0 0 0 1 0 0 0 -1 0 0 0\n"
	"\"Slope Brick 65 2 x 2 x 2 with Queen's Dress Pattern\" \"3678p4h.DAT\" 0 -1 0 0 0 1 0 0 0 -1 0 0 0\n"
	"\"Slope Brick 65 2 x 2 x 2 with Witch's Dress Pattern\" \"3678ap01.DAT\" 0 -1 0 0 0 1 0 0 0 -1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[NECK]\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Armour Plate\" \"2587.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Armour Samurai\" \"30174.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Bandana\" \"30133.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Beard Long Forked\" \"60750.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Beard Long with Five Braids\" \"60749.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Bracket 1 x 1 - 1 x 1\" \"42446.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Cape Cloth\" \"522.DAT\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Epaulette\" \"2526.DAT\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Airtanks\" \"3838.DAT\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Arrow Quiver\" \"4498.DAT\" 0 -1 0 0 0 1 0 0 0 -1 0 0 0\n"
	"\"Backpack Non-Opening\" \"2524.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Cape\" \"4524.DAT\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Container D-Basket\" \"4523.DAT\" 0 1 0 0 0 1 0 0 0 1 0 1 0\n"
	"\"Hair Beard\" \"6132.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Jet-Pack with Stud On Front\" \"4736.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Lifevest\" \"2610.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Scuba Tank\" \"30091.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest\" \"3840.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest with Crown on Dark Pink Sticker\" \"3840D01.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest with Crown on Violet Sticker\" \"3840D05.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest with Green Chevrons on Yellow Sticker\" \"3840D03.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest with Green Chevrons on Yellow/LtGrey Sticker\" \"3840D07.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest with White Maltese Cross on Red Sticker\" \"3840D02.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest with White Maltese Cross on Red/LtGrey Sticker\" \"3840D06.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest with Yellow Trefoils on Blue Sticker\" \"3840D04.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Vest with Yellow Trefoils on DkBlue Sticker\" \"3840D08.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[LARM]\n"
	"\"Right\" \"3818.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Bionicle Arm Barraki\" \"57588.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Arm\" \"30377.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Arm Straight\" \"59230.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Arm with Clip and Rod Hole\" \"53989.DAT\" 0 0 0 1 0 1 0 -1 0 0 0 0 0\n"
	"\"Skeleton Arm\" \"6265.DAT\" 0 1 0 0 0 1 0 0 0 1 -6 0 0.5\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[RARM]\n"
	"\"Left\" \"3819.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Bionicle Arm Barraki\" \"57588.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Arm\" \"30377.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Arm Straight\" \"59230.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Arm with Clip and Rod Hole\" \"53989.DAT\" 0 0 0 1 0 1 0 -1 0 0 0 0 0\n"
	"\"Skeleton Arm\" \"6265.DAT\" 0 1 0 0 0 1 0 0 0 1 6 0 0.5\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[LHAND]\n"
	"\"Hand\" \"3820.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hand Hook\" \"2531.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0.4 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[RHAND]\n"
	"\"Hand\" \"3820.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hand Hook\" \"2531.DAT\" 0 -1 0 0 0 1 0 0 0 1 0 0.4 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[LHANDA]\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Animal Snake\" \"30115.dat\" 0 0.469472 0 -0.882948 0.882948 0 0.469472 0 -1 0 0 -4 -4\n"
	"\"Animal Starfish\" \"33122.dat\" 0 -1 0 0 0 0 1 0 1 0 0 -26 -6\n"
	"\"Battleaxe\" \"3848.dat\" 0 -1 0 0 0 1 0 0 0 -1 0 0 0\n"
	"\"Bar 1.5L with Clip\" \"48729.dat\" 0 1 0 0 0 1 0 0 0 1 0 -20 0\n"
	"\"Bar 3L\" \"87994.dat\" 0 1 0 0 0 1 0 0 0 1 0 -40 0\n"
	"\"Bar 3L with White Ends\" \"87994p01.dat\" 0 1 0 0 0 1 0 0 0 1 0 -40 0\n"
	"\"Bar 4L Light Sabre Blade\" \"30374.dat\" 0 1 0 0 0 -1 0 0 0 -1 0 12 0\n"
	"\"Bar 4.5L Straight\" \"71184.dat\" 0 1 0 0 0 1 0 0 0 1 0 20 0\n"
	"\"Bar 4.5L with Handle\" \"87618.dat\" 0 -1 0 0 0 -1 0 0 0 1 0 -80 0\n"
	"\"Bar 6L with Thick Stop\" \"63965.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Bar 6.6L with Stop\" \"4095.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Bow with Arrow\" \"4499.dat\" 0 0 0 -1 0 1 0 1 0 0 0 1 0\n"
	"\"Bugle\" \"71342.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Camera Movie\" \"30148.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Camera Snapshot\" \"30089.dat\" 0 0 0.5 0.866025 0 0.866025 -0.5 -1 0 0 -4.062 2.5 -18\n"
	"\"Camera with Side Sight\" \"4360.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -24 6.5\n"
	"\"Castle Lance\" \"3849.dat\" 0 1 0 0 0 0 1 0 -1 0 0 40 0\n"
	"\"Circular Blade Saw\" \"30194.dat\" 0 -1 0 0 0 -0.422618 0.906308 0 0.906308 0.422618 0 15 -17\n"
	"\"Coin with 10 Mark\" \"70501a.dat\" 0 0 1 0 0 0 -1 -1 0 0 -2 -4 -10\n"
	"\"Coin with 20 Mark\" \"70501b.dat\" 0 0 1 0 0 0 -1 -1 0 0 -2 -4 -10\n"
	"\"Coin with 30 Mark\" \"70501c.dat\" 0 0 1 0 0 0 -1 -1 0 0 -2 -4 -10\n"
	"\"Coin with 40 Mark\" \"70501d.dat\" 0 0 1 0 0 0 -1 -1 0 0 -2 -4 -10\n"
	"\"Compass\" \"889c01.dat\" 0 1 0 0 0 1 0 0 0 1 0 -3 0\n"
	"\"Crossbow\" \"2570.dat\" 0 1 0 0 0 1 0 0 0 1 0 -3 0\n"
	"\"Cup\" \"3899.dat\" 0 1 0 0 0 1 0 0 0 1 0 -15 -20\n"
	"\"Dinner Plate\" \"6256.dat\" 0 0.0954045 -0.866025 -0.490814 0.981627 0 0.190809 -0.165245 -0.5 0.850114 7 -5 -26\n"
	"\"Dynamite Sticks Bundle\" \"64728.dat\" 0 0.5 0 0.866025 0 1 0 -0.866025 0 0.5 0 -28 -9\n"
	"\"Figur Club\" \"60659.dat\" 0 1 0 0 0 1 0 0 0 1 0 3 0\n"
	"\"Food Banana\" \"33085.dat\" 0 0 -1 0 1 0 0 0 0 1 0 0 0\n"
	"\"Food Carrot\" \"33172.dat\" 0 1 0 0 0 1 0 0 0 1 0 -50 0\n"
	"\"Food Carrot Top\" \"33183.dat\" 0 1 0 0 0 1 0 0 0 1 0 12 0\n"
	"\"Food Cherry\" \"22667.dat\" 0 1 0 0 0 1 0 0 0 1 0 -11 0\n"
	"\"Food Croissant\" \"33125.dat\" 0 0 1 0 -0.819152 0 0.573576 0.573576 0 0.819152 4 -27 -9\n"
	"\"Food French Bread\" \"4342.dat\" 0 0 -0.292372 0.956305 1 0 0 0 0.956305 0.292372 4.5 0 5\n"
	"\"Food Popsicle\" \"30222.dat\" 0 1 0 0 0 1 0 0 0 1 0 -3 0\n"
	"\"Food Turkey Leg\" \"33057.dat\" 0 0 0.985 -0.174 0 0.174 0.985 1 0 0 9 -24 -1\n"
	"\"Frypan\" \"4528.dat\" 0 0 1 0 0 0 1 1 0 0 -4 -24 0\n"
	"\"Hairbrush\" \"3852.dat\" 0 -1 0 0 0 1 0 0 0 -1 2.7 -8 0\n"
	"\"Hand Truck (Complete)\" \"2495c01.dat\" 0 1 0 0 0 0 1 0 -1 0 22 -4 -58\n"
	"\"Harpoon\" \"57467.dat\" 0 1 0 0 0 1 0 0 0 1 0 28 0\n"
	"\"Hose Nozzle with Side String Hole\" \"58367.dat\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Hose Nozzle with Side String Hole Simplified\" \"60849.dat\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Goblet\" \"2343.dat\" 0 1 0 0 0 1 0 0 0 1 0 -26 0\n"
	"\"Gun Flintlock Pistol\" \"2562.dat\" 0 1 0 0 0 1 0 0 0 1 0 -1 0\n"
	"\"Gun Musket\" \"2561.dat\" 0 0 0.707 0.707 0 0.707 -0.707 -1 0 0 -25.1 -33.7 0\n"
	"\"Gun Revolver\" \"30132.dat\" 0 1 0 0 0 1 0 0 0 1 0 -3 0\n"
	"\"Gun Rifle\" \"30141.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -8 0\n"
	"\"Gun Semiautomatic Pistol\" \"55707a.dat\" 0 1 0 0 0 1 0 0 0 1 0 -20 0\n"
	"\"Gun SW Small Blaster DC-17\" \"61190a.dat\" 0 1 0 0 0 1 0 0 0 1 0 -3 0\n"
	"\"Ice Axe\" \"30193.dat\" 0 1 0 0 0 1 0 0 0 1 0 6 0\n"
	"\"Jackhammer\" \"30228.dat\" 0 0.326 0 0.946 -0.899 -0.309 0.31 0.292 -0.951 0.101 2.5 -18.5 11\n"
	"\"Knife\" \"37.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Ladle\" \"4337.dat\" 0 1 0 0 0 1 0 0 0 1 0 -36 0\n"
	"\"Lightning\" \"59233.dat\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Loudhailer\" \"4349.dat\" 0 1 0 0 0 1 0 0 0 1 0 -16 0\n"
	"\"Magic Wand\" \"6124.dat\" 0 1 0 0 0 1 0 0 0 1 0 8 0\n"
	"\"Metal Detector\" \"4479.dat\" 0 1 0 0 0 1 0 0 0 1 0 -24 0\n"
	"\"Mug\" \"33054.dat\" 0 1 0 0 0 1 0 0 0 1 0 -12 -20\n"
	"\"Polearm Halberd\" \"6123.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Radio with Long Handle\" \"3962b.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -1 0\n"
	"\"Radio with Short Handle\" \"3962a.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -1 0\n"
	"\"Rock 1 x 1 Gem Facetted\" \"30153.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Saucepan\" \"4529.dat\" 0 0 1 0 0 0 1 1 0 0 -6 -24 0\n"
	"\"Sextant\" \"30154.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -35 0\n"
	"\"Shield Octagonal with Stud\" \"48494.dat\" 0 0 0 1 -1 0 0 0 -1 0 0 -2 0\n"
	"\"Shield Octagonal without Stud\" \"61856.dat\" 0 0 0 1 -1 0 0 0 -1 0 0 -2 0\n"
	"\"Shield Ovoid\" \"2586.dat\" 0 0 0 1 -1 0 0 0 -1 0 -4 -1 0\n"
	"\"Shield Ovoid with American Indian Pattern\" \"2586PW1.dat\" 0 0 0 1 -1 0 0 0 -1 0 -4 -1 0\n"
	"\"Shield Ovoid with Batlord Pattern\" \"2586P4F.dat\" 0 0 0 1 -1 0 0 0 -1 0 -4 -1 0\n"
	"\"Shield Ovoid with Blue Dragon Pattern\" \"2586P4C.dat\" 0 0 0 1 -1 0 0 0 -1 0 -4 -1 0\n"
	"\"Shield Ovoid with Bull Head Pattern\" \"2586P4G.DAT\" 0 0 0 1 -1 0 0 0 -1 0 -4 -1 0\n"
	"\"Shield Ovoid with Golden Lion Pattern\" \"2586PH1.DAT\" 0 0 0 1 -1 0 0 0 -1 0 -4 -1 0\n"
	"\"Shield Ovoid with Green Dragon Pattern\" \"2586P4B.dat\" 0 0 0 1 -1 0 0 0 -1 0 -4 -1 0\n"
	"\"Shield Ovoid with Royal Knights Lion Pattern\" \"2586P4D.dat\" 0 0 0 1 -1 0 0 0 -1 0 -4 -1 0\n"
	"\"Shield Ovoid with Silver Snake Pattern\" \"2586PH2.DAT\" 0 0 0 1 -1 0 0 0 -1 0 -4 -1 0\n"
	"\"Shield Ovoid with SW Gungans Patrol Pattern\" \"2586PS1.DAT\" 0 0 0 1 -1 0 0 0 -1 0 -4 -1 0\n"
	"\"Shield Round\" \"3876.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular\" \"3846.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Batlord Pattern\" \"3846p4f.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Black Falcon Pattern\" \"3846p43.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Black Falcon Blue Border Pattern\" \"3846p45.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Black Falcon Yellow Border Pattern\" \"3846p46.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Blue Dragon Pattern\" \"3846p4c.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Blue Lion on Yellow Background\" \"3846p4g.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Forestman Pattern\" \"3846p48.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Green Chevrons on Yellow Sticker\" \"3846d03.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Green Chevrons on Yellow/LtGray\" \"3846d06.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Crown on Dark-Pink Sticker\" \"3846d01.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Crown on Violet Sticker\" \"3846d05.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Lion Head, Blue & Yellow Pattern\" \"3846p4e.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Maroon/Red Quarters Pattern\" \"3846p4u.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Red and Gray Pattern, Blue Frame\" \"3846p47.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Red/Peach Quarters Pattern\" \"3846p4t.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Royal Knights Lion Pattern\" \"3846p4d.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with White Maltese Cross on Red Sticker\" \"3846d02.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Wolfpack Pattern\" \"3846p44.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Yellow Lion on Blue Background\" \"3846p4h.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Yellow Trefoils on Blue Sticker\" \"3846d04.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shield Triangular with Yellow Trefoils on DkBlue Sticker\" \"3846d07.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -12 0\n"
	"\"Shovel\" \"3837.dat\" 0 1 0 0 0 1 0 0 0 1 0 -12 0\n"
	"\"Signal Holder\" \"3900.dat\" 0 1 0 0 0 0 -1 0 1 0 0 -36 -2\n"
	"\"Ski Pole\" \"90514.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Space Scanner Tool\" \"30035.dat\" 0 1 0 0 0 1 0 0 0 1 0 -19 -10\n"
	"\"Spear\" \"4497.dat\" 0 1 0 0 0 1 0 0 0 1 0 -40 0\n"
	"\"Spear with Four Side Blades\" \"43899.dat\" 0 1 0 0 0 1 0 0 0 1 0 -144 0\n"
	"\"Speargun\" \"30088.dat\" 0 1 0 0 0 1 0 0 0 1 0 -13 0\n"
	"\"Statuette\" \"90398.dat\" 0 1 0 0 0 1 0 0 0 1 0 -1 0\n"
	"\"Suitcase\" \"4449.dat\" 0 0 0 -1 1 0 0 0 -1 0 0 0 0\n"
	"\"Sword Cutlass\" \"2530.dat\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Sword Greatsword\" \"59.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Sword Katana\" \"30173.dat\" 0 1 0 0 0 1 0 0 0 1 0 6 0\n"
	"\"Sword Scimitar\" \"43887.dat\" 0 1 0 0 0 1 0 0 0 1 0 -18 0\n"
	"\"Sword Shortsword\" \"3847.dat\" 0 1 0 0 0 1 0 0 0 1 0 -12 0\n"
	"\"Syringe\" \"87989.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Telescope\" \"64644.dat\" 0 1 0 0 0 1 0 0 0 1 0 -11 0\n"
	"\"Tool Binoculars Space\" \"30304.dat\" 0 1 0 0 0 0 -1 0 1 0 -5 -1 0\n"
	"\"Tool Binoculars Town\" \"30162.dat\" 0 1 0 0 0 0 -1 0 1 0 -5 -1.6 0\n"
	"\"Tool Box Wrench\" \"6246d.dat\" 0 1 0 0 0 1 0 0 0 1 0 -36 0\n"
	"\"Tool Hammer\" \"6246b.dat\" 0 1 0 0 0 1 0 0 0 1 0 -36 0\n"
	"\"Tool Handaxe\" \"3835.dat\" 0 1 0 0 0 1 0 0 0 1 0 -16 0\n"
	"\"Tool Light Sabre Hilt\" \"577.dat\" 0 1 0 0 0 1 0 0 0 1 0 -20 0\n"
	"\"Tool Light Sabre - On (Shortcut)\" \"577c01.dat\" 0 1 0 0 0 1 0 0 0 1 0 -20 0\n"
	"\"Tool Light Sabre - Dual On (Shortcut)\" \"577c02.dat\" 0 1 0 0 0 1 0 0 0 1 0 -20 0\n"
	"\"Tool Magnifying Glass\" \"30152.dat\" 0 1 0 0 0 1 0 0 0 1 0 -52 0\n"
	"\"Tool Mallet\" \"4522.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -28 0\n"
	"\"Tool Oar\" \"2542.dat\" 0 -1 0 0 0 -1 0 0 0 1 0 40 0\n"
	"\"Tool Oilcan\" \"55296.DAT\" 0 1 0 0 0 1 0 0 0 1 0 -6 0\n"
	"\"Tool Open End Wrench\" \"6246e.dat\" 0 1 0 0 0 1 0 0 0 1 0 -36 0\n"
	"\"Tool Pickaxe\" \"3841.dat\" 0 1 0 0 0 1 0 0 0 1 0 -12 0\n"
	"\"Tool Power Drill\" \"6246c.dat\" 0 1 0 0 0 1 0 0 0 1 0 -6 0\n"
	"\"Tool Screwdriver\" \"6246a.dat\" 0 1 0 0 0 1 0 0 0 1 0 -34 0\n"
	"\"Tool Spanner/Screwdriver\" \"4006.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -14 0\n"
	"\"Tool Pushbroom\" \"3836.dat\" 0 0 0 -1 0 -1 0 -1 0 0 0 44 0\n"
	"\"Tool Fishing Rod\" \"2614.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Tool Hose Nozzle with Handle\" \"4210a.dat\" 0 -1 0 0 0 1 0 0 0 -1 0 -12 0\n"
	"\"Torch\" \"3959.dat\" 0 1 0 0 0 1 0 0 0 1 0 -13 0\n"
	"\"Underwater Scooter\" \"30092.dat\" 0 -1 0 0 0 1 0 0 0 -1 20 -22 -8.5\n"
	"\"Whip\" \"2488.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Whip in Latched Position\" \"2488c01.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Wine Glass\" \"33061.dat\" 0 1 0 0 0 1 0 0 0 1 0 -32 0\n"
	"\"Minifig Zip Line Handle\" \"30229.dat\" 0 1 0 0 0 1 0 0 0 1 0 -12 0\n"
	"\n"
	"[RHANDA]\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Animal Snake\" \"30115.dat\" 0 -0.469472 0 0.882948 0.882948 0 0.469472 0 1 0 0 -4 4\n"
	"\"Animal Starfish\" \"33122.dat\" 0 -1 0 0 0 0 1 0 1 0 0 -26 -6\n"
	"\"Battleaxe\" \"3848.dat\" 0 -1 0 0 0 1 0 0 0 -1 0 0 0\n"
	"\"Bar 1.5L with Clip\" \"48729.dat\" 0 1 0 0 0 1 0 0 0 1 0 -20 0\n"
	"\"Bar 3L\" \"87994.dat\" 0 1 0 0 0 1 0 0 0 1 0 -40 0\n"
	"\"Bar 3L with White Ends\" \"87994p01.dat\" 0 1 0 0 0 1 0 0 0 1 0 -40 0\n"
	"\"Bar 4L Light Sabre Blade\" \"30374.dat\" 0 1 0 0 0 -1 0 0 0 -1 0 12 0\n"
	"\"Bar 4.5L Straight\" \"71184.dat\" 0 1 0 0 0 1 0 0 0 1 0 20 0\n"
	"\"Bar 4.5L with Handle\" \"87618.dat\" 0 -1 0 0 0 -1 0 0 0 1 0 -80 0\n"
	"\"Bar 6L with Thick Stop\" \"63965.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Bar 6.6L with Stop\" \"4095.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Bugle\" \"71342.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Bow with Arrow\" \"4499.dat\" 0 0 0 -1 0 1 0 1 0 0 0 1 0\n"
	"\"Camera Movie\" \"30148.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Camera Snapshot\" \"30089.dat\" 0 0 0.5 0.866025 0 0.866025 -0.5 -1 0 0 -4.062 2.5 -18\n"
	"\"Camera with Side Sight\" \"4360.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -24 6.5\n"
	"\"Castle Lance\" \"3849.dat\" 0 1 0 0 0 0 1 0 -1 0 0 40 0\n"
	"\"Circular Blade Saw\" \"30194.dat\" 0 -1 0 0 0 -0.422618 0.906308 0 0.906308 0.422618 0 15 -17\n"
	"\"Coin with 10 Mark\" \"70501a.dat\" 0 0 1 0 0 0 -1 -1 0 0 -2 -4 -10\n"
	"\"Coin with 20 Mark\" \"70501b.dat\" 0 0 1 0 0 0 -1 -1 0 0 -2 -4 -10\n"
	"\"Coin with 30 Mark\" \"70501c.dat\" 0 0 1 0 0 0 -1 -1 0 0 -2 -4 -10\n"
	"\"Coin with 40 Mark\" \"70501d.dat\" 0 0 1 0 0 0 -1 -1 0 0 -2 -4 -10\n"
	"\"Compass\" \"889c01.dat\" 0 1 0 0 0 1 0 0 0 1 0 -3 0\n"
	"\"Crossbow\" \"2570.dat\" 0 1 0 0 0 1 0 0 0 1 0 -3 0\n"
	"\"Cup\" \"3899.dat\" 0 1 0 0 0 1 0 0 0 1 0 -15 -20\n"
	"\"Dinner Plate\" \"6256.dat\" 0 -0.0954045 0.866025 0.490814 -0.981627 0 -0.190809 -0.165245 -0.5 0.850114 -7 -5 -26\n"
	"\"Dynamite Sticks Bundle\" \"64728.dat\" 0 0.5 0 0.866025 0 1 0 -0.866025 0 0.5 0 -28 -9\n"
	"\"Figur Club\" \"60659.dat\" 0 1 0 0 0 1 0 0 0 1 0 3 0\n"
	"\"Food Banana\" \"33085.dat\" 0 0 -1 0 1 0 0 0 0 1 0 0 0\n"
	"\"Food Carrot\" \"33172.dat\" 0 1 0 0 0 1 0 0 0 1 0 -50 0\n"
	"\"Food Cherry\" \"22667.dat\" 0 1 0 0 0 1 0 0 0 1 0 -11 0\n"
	"\"Food Croissant\" \"33125.dat\" 0 0 1 0 -0.819152 0 0.573576 0.573576 0 0.819152 4 -27 -9\n"
	"\"Food French Bread\" \"4342.dat\" 0 0 0.292372 0.956305 1 0 0 0 0.956305 -0.292372 -4.5 0 5\n"
	"\"Food Popsicle\" \"30222.dat\" 0 1 0 0 0 1 0 0 0 1 0 -3 0\n"
	"\"Food Turkey Leg\" \"33057.dat\" 0 0 -0.985 0.174 0 0.174 0.985 -1 0 0 -9 -24 -1\n"
	"\"Frypan\" \"4528.dat\" 0 0 1 0 0 0 1 1 0 0 -4 -24 0\n"
	"\"Hairbrush\" \"3852.dat\" 0 -1 0 0 0 1 0 0 0 -1 2.7 -8 0\n"
	"\"Hand Truck (Complete)\" \"2495c01.dat\" 0 1 0 0 0 0 1 0 -1 0 -22 -4 -58\n"
	"\"Harpoon\" \"57467.dat\" 0 1 0 0 0 1 0 0 0 1 0 28 0\n"
	"\"Hose Nozzle with Side String Hole\" \"58367.dat\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Hose Nozzle with Side String Hole Simplified\" \"60849.dat\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Goblet\" \"2343.dat\" 0 1 0 0 0 1 0 0 0 1 0 -26 0\n"
	"\"Gun Flintlock Pistol\" \"2562.dat\" 0 1 0 0 0 1 0 0 0 1 0 -1 0\n"
	"\"Gun Musket\" \"2561.dat\" 0 0 0.707 0.707 0 0.707 -0.707 -1 0 0 -25.1 -33.7 0\n"
	"\"Gun Revolver\" \"30132.dat\" 0 1 0 0 0 1 0 0 0 1 0 -3 0\n"
	"\"Gun Rifle\" \"30141.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -8 0\n"
	"\"Gun Semiautomatic Pistol\" \"55707a.dat\" 0 1 0 0 0 1 0 0 0 1 0 -20 0\n"
	"\"Gun SW Small Blaster DC-17\" \"61190a.dat\" 0 1 0 0 0 1 0 0 0 1 0 -3 0\n"
	"\"Ice Axe\" \"30193.dat\" 0 1 0 0 0 1 0 0 0 1 0 6 0\n"
	"\"Jackhammer\" \"30228.dat\" 0 0.326 0 -0.946 0.899 -0.309 0.31 -0.292 -0.951 -0.101 -2.5 -18.5 11\n"
	"\"Knife\" \"37.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Ladle\" \"4337.dat\" 0 1 0 0 0 1 0 0 0 1 0 -36 0\n"
	"\"Lightning\" \"59233.dat\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Loudhailer\" \"4349.dat\" 0 1 0 0 0 1 0 0 0 1 0 -16 0\n"
	"\"Magic Wand\" \"6124.dat\" 0 1 0 0 0 1 0 0 0 1 0 8 0\n"
	"\"Metal Detector\" \"4479.dat\" 0 1 0 0 0 1 0 0 0 1 0 -24 0\n"
	"\"Mug\" \"33054.dat\" 0 1 0 0 0 1 0 0 0 1 0 -12 -20\n"
	"\"Polearm Halberd\" \"6123.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Radio with Long Handle\" \"3962b.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -1 0\n"
	"\"Radio with Short Handle\" \"3962a.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -1 0\n"
	"\"Rock 1 x 1 Gem Facetted\" \"30153.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Saucepan\" \"4529.dat\" 0 0 1 0 0 0 1 1 0 0 -6 -24 0\n"
	"\"Sextant\" \"30154.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -35 0\n"
	"\"Shield Octagonal with Stud\" \"48494.dat\" 0 0 0 -1 1 0 0 0 -1 0 0 -2 0\n"
	"\"Shield Octagonal without Stud\" \"61856.dat\" 0 0 0 -1 1 0 0 0 -1 0 0 -2 0\n"
	"\"Shield Ovoid\" \"2586.dat\" 0 0 0 -1 1 0 0 0 -1 0 4 -1 0\n"
	"\"Shield Ovoid with American Indian Pattern\" \"2586pw1.dat\" 0 0 0 -1 1 0 0 0 -1 0 4 -1 0\n"
	"\"Shield Ovoid with Batlord Pattern\" \"2586P4F.dat\" 0 0 0 -1 1 0 0 0 -1 0 4 -1 0\n"
	"\"Shield Ovoid with Blue Dragon Pattern\" \"2586p4c.dat\" 0 0 0 -1 1 0 0 0 -1 0 4 -1 0\n"
	"\"Shield Ovoid with Bull Head Pattern\" \"2586P4G.DAT\" 0 0 0 -1 1 0 0 0 -1 0 4 -1 0\n"
	"\"Shield Ovoid with Golden Lion Pattern\" \"2586ph1.DAT\" 0 0 0 -1 1 0 0 0 -1 0 4 -1 0\n"
	"\"Shield Ovoid with Green Dragon Pattern\" \"2586p4b.dat\" 0 0 0 -1 1 0 0 0 -1 0 4 -1 0\n"
	"\"Shield Ovoid with Royal Knights Lion Pattern\" \"2586p4d.dat\" 0 0 0 -1 1 0 0 0 -1 0 4 -1 0\n"
	"\"Shield Ovoid with Silver Snake Pattern\" \"2586PH2.DAT\" 0 0 0 -1 1 0 0 0 -1 0 4 -1 0\n"
	"\"Shield Ovoid with SW Gungans Patrol Pattern\" \"2586ps1.DAT\" 0 0 0 -1 1 0 0 0 -1 0 4 -1 0\n"
	"\"Shield Round\" \"3876.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular\" \"3846.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Batlord Pattern\" \"3846p4f.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Black Falcon Pattern\" \"3846p43.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Black Falcon Blue Border Pattern\" \"3846p45.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Black Falcon Yellow Border Pattern\" \"3846p46.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Blue Dragon Pattern\" \"3846p4c.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Blue Lion on Yellow Background\" \"3846p4g.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Forestman Pattern\" \"3846p48.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Green Chevrons on Yellow Sticker\" \"3846d03.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Green Chevrons on Yellow/LtGray\" \"3846d06.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Crown on Dark-Pink Sticker\" \"3846d01.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Crown on Violet Sticker\" \"3846d05.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Lion Head, Blue & Yellow Pattern\" \"3846p4e.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Maroon/Red Quarters Pattern\" \"3846p4u.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Red and Gray Pattern, Blue Frame\" \"3846p47.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Red/Peach Quarters Pattern\" \"3846p4t.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Royal Knights Lion Pattern\" \"3846p4d.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with White Maltese Cross on Red Sticker\" \"3846d02.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Wolfpack Pattern\" \"3846p44.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Yellow Lion on Blue Background\" \"3846p4h.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Yellow Trefoils on Blue Sticker\" \"3846d04.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shield Triangular with Yellow Trefoils on DkBlue Sticker\" \"3846d07.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -12 0\n"
	"\"Shovel\" \"3837.dat\" 0 1 0 0 0 1 0 0 0 1 0 -12 0\n"
	"\"Signal Holder\" \"3900.dat\" 0 1 0 0 0 0 -1 0 1 0 0 -36 -2\n"
	"\"Ski Pole\" \"90514.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Space Scanner Tool\" \"30035.dat\" 0 1 0 0 0 1 0 0 0 1 0 -19 -10\n"
	"\"Spear\" \"4497.dat\" 0 1 0 0 0 1 0 0 0 1 0 -40 0\n"
	"\"Spear with Four Side Blades\" \"43899.dat\" 0 1 0 0 0 1 0 0 0 1 0 -144 0\n"
	"\"Speargun\" \"30088.dat\" 0 1 0 0 0 1 0 0 0 1 0 -13 0\n"
	"\"Statuette\" \"90398.dat\" 0 1 0 0 0 1 0 0 0 1 0 -1 0\n"
	"\"Suitcase\" \"4449.dat\" 0 0 0 -1 1 0 0 0 -1 0 0 0 0\n"
	"\"Sword Cutlass\" \"2530.dat\" 0 1 0 0 0 1 0 0 0 1 0 -2 0\n"
	"\"Sword Greatsword\" \"59.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Sword Katana\" \"30173.dat\" 0 1 0 0 0 1 0 0 0 1 0 6 0\n"
	"\"Sword Scimitar\" \"43887.dat\" 0 1 0 0 0 1 0 0 0 1 0 -18 0\n"
	"\"Sword Shortsword\" \"3847.dat\" 0 1 0 0 0 1 0 0 0 1 0 -12 0\n"
	"\"Syringe\" \"87989.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Telescope\" \"64644.dat\" 0 1 0 0 0 1 0 0 0 1 0 -11 0\n"
	"\"Tool Binoculars Space\" \"30304.dat\" 0 1 0 0 0 0 -1 0 1 0 -5 -1 0\n"
	"\"Tool Binoculars Town\" \"30162.dat\" 0 1 0 0 0 0 -1 0 1 0 -5 -1.6 0\n"
	"\"Tool Box Wrench\" \"6246d.dat\" 0 1 0 0 0 1 0 0 0 1 0 -36 0\n"
	"\"Tool Hammer\" \"6246b.dat\" 0 1 0 0 0 1 0 0 0 1 0 -36 0\n"
	"\"Tool Handaxe\" \"3835.dat\" 0 1 0 0 0 1 0 0 0 1 0 -16 0\n"
	"\"Tool Light Sabre Hilt\" \"577.dat\" 0 1 0 0 0 1 0 0 0 1 0 -20 0\n"
	"\"Tool Light Sabre - On (Shortcut)\" \"577c01.dat\" 0 1 0 0 0 1 0 0 0 1 0 -20 0\n"
	"\"Tool Light Sabre - Dual On (Shortcut)\" \"577c02.dat\" 0 1 0 0 0 1 0 0 0 1 0 -20 0\n"
	"\"Tool Magnifying Glass\" \"30152.dat\" 0 1 0 0 0 1 0 0 0 1 0 -52 0\n"
	"\"Tool Mallet\" \"4522.dat\" 0 0 0 1 0 1 0 -1 0 0 0 -28 0\n"
	"\"Tool Oar\" \"2542.dat\" 0 -1 0 0 0 -1 0 0 0 1 0 40 0\n"
	"\"Tool Oilcan\" \"55296.DAT\" 0 1 0 0 0 1 0 0 0 1 0 -6 0\n"
	"\"Tool Open End Wrench\" \"6246e.dat\" 0 1 0 0 0 1 0 0 0 1 0 -36 0\n"
	"\"Tool Pickaxe\" \"3841.dat\" 0 1 0 0 0 1 0 0 0 1 0 -12 0\n"
	"\"Tool Power Drill\" \"6246c.dat\" 0 1 0 0 0 1 0 0 0 1 0 -6 0\n"
	"\"Tool Screwdriver\" \"6246a.dat\" 0 1 0 0 0 1 0 0 0 1 0 -34 0\n"
	"\"Tool Spanner/Screwdriver\" \"4006.dat\" 0 0 0 -1 0 1 0 1 0 0 0 -14 0\n"
	"\"Tool Pushbroom\" \"3836.dat\" 0 0 0 -1 0 -1 0 -1 0 0 0 44 0\n"
	"\"Tool Fishing Rod\" \"2614.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Tool Hose Nozzle with Handle\" \"4210a.dat\" 0 -1 0 0 0 1 0 0 0 -1 0 -12 0\n"
	"\"Torch\" \"3959.dat\" 0 1 0 0 0 1 0 0 0 1 0 -13 0\n"
	"\"Underwater Scooter\" \"30092.dat\" 0 -1 0 0 0 1 0 0 0 -1 -20 -22 -8.5\n"
	"\"Whip\" \"2488.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Whip in Latched Position\" \"2488c01.dat\" 0 1 0 0 0 1 0 0 0 1 0 -8 0\n"
	"\"Wine Glass\" \"33061.dat\" 0 1 0 0 0 1 0 0 0 1 0 -32 0\n"
	"\"Minifig Zip Line Handle\" \"30229.dat\" 0 1 0 0 0 1 0 0 0 1 0 -12 0\n"
	"\n"
	"[LLEG]\n"
	"\"Plain Leg\" \"3816.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Astro Pattern\" \"3816P6F.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Buttoned Pocket Pattern\" \"3816PA3.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Golden Circuit Pattern\" \"3816P6W.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Grass Skirt Pattern\" \"3816p3j.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Green Kilt and Toes Pattern\" \"3816pa2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Laboratory Smock Pattern\" \"3816PDE.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Leather Straps (Red Studs) Pattern\" \"3816p4f.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Orange Cable Pattern\" \"3816P6u.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Purple Greatcoat Pattern\" \"971phb.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Robot Pattern\" \"971P63.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Salmon Cable Pattern\" \"3816P6V.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"SW Gunbelt Pattern\" \"971PS5.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Leg Skeleton\" \"6266.DAT\" 0 1 0 0 0 1 0 0 0 1 -10 0 0\n"
	"\"Leg Wooden\" \"2532.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hips and Legs Short -> Hips:\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Legs Old -> Hips:\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Legs -> Hips:\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Skirts (Slope Brick 65 2 x 2 x 2) -> Hips:\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[RLEG]\n"
	"\"Plain Leg\" \"3817.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Astro Pattern\" \"3817P6F.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Buttoned Pocket Pattern\" \"3817PA3.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Golden Circuit Pattern\" \"3817P6W.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Grass Skirt Pattern\" \"3817p3j.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Green Kilt and Toes Pattern\" \"3817pa2.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Laboratory Smock Pattern\" \"3817PDE.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Leather Straps (Red Studs) Pattern\" \"3817p4f.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Orange Cable Pattern\" \"3817P6u.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Purple Greatcoat Pattern\" \"972phb.dat\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Robot Pattern\" \"972P63.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Salmon Cable Pattern\" \"3817P6V.DAT\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Leg Wooden\" \"2532.DAT\" 0 -1 0 0 0 1 0 0 0 -1 0 0 0\n"
	"\"Leg Skeleton\" \"6266.DAT\" 0 1 0 0 0 1 0 0 0 1 10 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Hips and Legs Short -> Hips:\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Legs Old -> Hips:\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Mechanical Legs -> Hips:\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Skirts (Slope Brick 65 2 x 2 x 2) -> Hips:\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\n"
	"[LLEGA]\n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Flipper\" \"2599.DAT\" 0 0.996 0 0.087 0 1 0 -0.087 0 0.996 -10 28 -1\n"
	"\"Skakeboard with Black Wheels\" \"42511c01.DAT\" 0 1 0 0 0 1 0 0 0 1 0 28 1\n"
	"\"Snowshoe\" \"30284.DAT\" 0 1 0 0 0 1 0 0 0 1 -10 28 -1\n"
	"\"Ski\" \"6120.DAT\" 0 1 0 0 0 1 0 0 0 1 -10 28 1\n"
	"\"Ski 6L\" \"90509.DAT\" 0 1 0 0 0 1 0 0 0 1 -10 28 -1\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Plate 2x4 with Curved Beveled Sides\" \"88000.DAT\" 0 1 0 0 0 1 0 0 0 1 0 28 0\n"
	"\n"
	"[RLEGA] \n"
	"\"None\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Flipper\" \"2599.DAT\" 0 0.996 0 -0.087 0 1 0 0.087 0 0.996 10 28 -1\n"
	"\"Skakeboard with Black Wheels\" \"42511c01.DAT\" 0 1 0 0 0 1 0 0 0 1 0 28 1\n"
	"\"Snowshoe\" \"30284.DAT\" 0 1 0 0 0 1 0 0 0 1 10 28 -1\n"
	"\"Ski\" \"6120.dat\" 0 1 0 0 0 1 0 0 0 1 10 28 1\n"
	"\"Ski 6L\" \"90509.DAT\" 0 1 0 0 0 1 0 0 0 1 -10 28 -1\n"
	"\"--------------------------------------------------------------------------------\" \"\" 0 1 0 0 0 1 0 0 0 1 0 0 0\n"
	"\"Plate 2x4 with Curved Beveled Sides\" \"88000.DAT\" 0 1 0 0 0 1 0 0 0 1 0 28 0\n"
	"\n";

// =============================================================================
// MinifigWizard class

MinifigWizard::MinifigWizard (GLWindow *share)
	: GLWindow (share)
{
	char Filename[LC_MAXPATH];
	strcpy(Filename, lcGetPiecesLibrary()->GetLibraryPath());
	strcat(Filename, "mlcad.ini");

	FileDisk DiskSettings;
	if (DiskSettings.Open(Filename, "rt"))
	{
		ParseSettings(DiskSettings);
	}
	else
	{
		FileMem MemSettings;
		MemSettings.Write(DefaultSettings, strlen(DefaultSettings)+1);
		ParseSettings(MemSettings);
	}

	const unsigned char colors[LC_MFW_NUMITEMS] = { 0, 6, 4, 22, 0, 0, 6, 6, 22, 22, 9, 9, 9, 22, 22 };
	const char *pieces[LC_MFW_NUMITEMS] = { "3624", "3626BP01", "973", "None", "3819", "3818", "3820", "3820",
	                                        "None", "None", "3815", "3817", "3816", "None", "None" };
	int i;

	for (i = 0; i < LC_MFW_NUMITEMS; i++)
	{
		m_Colors[i] = colors[i];
		m_Angles[i] = 0;

		m_Info[i] = lcGetPiecesLibrary()->FindPieceInfo(pieces[i]);
		if (m_Info[i] != NULL)
			m_Info[i]->AddRef();
	}

	Calculate();

	m_MinifigCount = 0;
	m_MinifigNames = NULL;
	m_MinifigTemplates = NULL;

	int Version = Sys_ProfileLoadInt("MinifigWizard", "Version", 1);
	if (Version == 1)
	{
		char *ptr, buf[32];

		m_MinifigCount = Sys_ProfileLoadInt ("MinifigWizard", "Count", 0);
		m_MinifigNames = (char**)realloc(m_MinifigNames, sizeof(char**) * (m_MinifigCount+1));
		m_MinifigTemplates = (char**)realloc(m_MinifigTemplates, sizeof(char**) * (m_MinifigCount+1));

		for (int i = 0; i < m_MinifigCount; i++)
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

void MinifigWizard::ParseSettings(File& Settings)
{
	const char* SectionNames[LC_MFW_NUMITEMS] =
	{
		"[HATS]", // LC_MFW_HAT
		"[HEAD]", // LC_MFW_HEAD
		"[BODY]", // LC_MFW_TORSO
		"[NECK]", // LC_MFW_NECK
		"[RARM]", // LC_MFW_LEFT_ARM
		"[LARM]", // LC_MFW_RIGHT_ARM
		"[RHAND]", // LC_MFW_LEFT_HAND
		"[LHAND]", // LC_MFW_RIGHT_HAND
		"[RHANDA]", // LC_MFW_LEFT_TOOL
		"[LHANDA]", // LC_MFW_RIGHT_TOOL
		"[BODY2]", // LC_MFW_HIPS
		"[RLEG]", // LC_MFW_LEFT_LEG
		"[LLEG]", // LC_MFW_RIGHT_LEG
		"[RLEGA]", // LC_MFW_LEFT_SHOE
		"[LLEGA]", // LC_MFW_RIGHT_SHOE
	};

	for (int SectionIndex = 0; SectionIndex < LC_MFW_NUMITEMS; SectionIndex++)
	{
		ObjArray<lcMinifigPieceInfo>& InfoArray = mSettings[SectionIndex];

		InfoArray.RemoveAll();
		Settings.Seek(0, SEEK_SET);

		char Line[1024];
		bool FoundSection = false;
		const char* SectionName = SectionNames[SectionIndex];
		int SectionNameLength = strlen(SectionName);

		// Find start of section
		while (Settings.ReadLine(Line, sizeof(Line)))
		{
			if (!strncmp(Line, SectionName, SectionNameLength))
			{
				FoundSection = true;
				break;
			}
		}

		if (!FoundSection)
			continue;

		// Parse section.
		while (Settings.ReadLine(Line, sizeof(Line)))
		{
			if (Line[0] == '[')
				break;

			char* DescriptionStart = strchr(Line, '"');
			if (!DescriptionStart)
				continue;
			DescriptionStart++;
			char* DescriptionEnd = strchr(DescriptionStart, '"');
			if (!DescriptionEnd)
				continue;
			*DescriptionEnd = 0;
			DescriptionEnd++;

			char* NameStart = strchr(DescriptionEnd, '"');
			if (!NameStart)
				continue;
			NameStart++;
			char* NameEnd = strchr(NameStart, '"');
			if (!NameEnd)
				continue;
			*NameEnd = 0;
			NameEnd++;

			strupr(NameStart);
			char* Ext = strrchr(NameStart, '.');
			if (Ext != NULL)
			{
				if (!strcmp(Ext, ".DAT"))
					*Ext = 0;
			}

			PieceInfo* Info = lcGetPiecesLibrary()->FindPieceInfo(NameStart);
			if (!Info && *NameStart)
				continue;

			float Mat[12];
			int Flags;

			if (sscanf(NameEnd, "%d %g %g %g %g %g %g %g %g %g %g %g %g",
					   &Flags, &Mat[0], &Mat[1], &Mat[2], &Mat[3], &Mat[4], &Mat[5], &Mat[6], 
					   &Mat[7], &Mat[8], &Mat[9], &Mat[10], &Mat[11]) != 13)
				continue;

			Matrix44 Offset;
			Offset.LoadIdentity();
			float* OffsetMatrix = &Offset[0][0];

			OffsetMatrix[0] =  Mat[0];
			OffsetMatrix[8] = -Mat[1];
			OffsetMatrix[4] =  Mat[2];
			OffsetMatrix[2] = -Mat[3];
			OffsetMatrix[10] = Mat[4];
			OffsetMatrix[6] = -Mat[5];
			OffsetMatrix[1] =  Mat[6];
			OffsetMatrix[9] = -Mat[7];
			OffsetMatrix[5] =  Mat[8];
			OffsetMatrix[12] =  Mat[9] / 25.0f;
			OffsetMatrix[14] = -Mat[10] / 25.0f;
			OffsetMatrix[13] =  Mat[11] / 25.0f;

			lcMinifigPieceInfo MinifigInfo;
			strncpy(MinifigInfo.Description, DescriptionStart, sizeof(MinifigInfo.Description));
			MinifigInfo.Description[sizeof(MinifigInfo.Description)-1] = 0;
			MinifigInfo.Offset = Offset;
			MinifigInfo.Info = Info;

			InfoArray.Add(MinifigInfo);
		}
	}
}

void MinifigWizard::OnDraw()
{
	int i;

	if (!MakeCurrent())
		return;

	float aspect = (float)m_nWidth/(float)m_nHeight;
	glViewport(0, 0, m_nWidth, m_nHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0f, aspect, 1.0f, 20.0f);
	glMatrixMode(GL_MODELVIEW);
	Matrix44 WorldView;
	WorldView.CreateLookAt(Vector3(0, -9, 4), Vector3(0, 5, 1), Vector3(0, 0, 1));
	glLoadMatrixf(WorldView);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float *bg = lcGetActiveProject()->GetBackgroundColor();
	glClearColor(bg[0], bg[1], bg[2], bg[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DITHER);
	glShadeModel(GL_FLAT);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Calculate();

	for (i = 0; i < LC_MFW_NUMITEMS; i++)
	{
		if (m_Info[i] == NULL)
			continue;

		glPushMatrix();
		glMultMatrixf(m_Matrices[i]);
		m_Info[i]->RenderPiece(m_Colors[i]);
		glPopMatrix();
	}

	glFinish();

	SwapBuffers();
}

void MinifigWizard::Calculate()
{
	float HeadOffset = 0.0f;
	Matrix44 Root, Mat, Mat2;

	bool DroidTorso = m_Info[LC_MFW_TORSO] && !strcmp(m_Info[LC_MFW_TORSO]->m_strName, "30375");
	bool SkeletonTorso = m_Info[LC_MFW_TORSO] && !strcmp(m_Info[LC_MFW_TORSO]->m_strName, "6260");

	Root.LoadIdentity();
	Root.SetTranslation(Vector3(0, 0, 2.88f));
	m_Matrices[LC_MFW_TORSO] = Mul(mSettings[LC_MFW_TORSO][GetSelectionIndex(LC_MFW_TORSO)].Offset, Root);

	if (m_Info[LC_MFW_NECK])
	{
		m_Matrices[LC_MFW_NECK] = Mul(mSettings[LC_MFW_NECK][GetSelectionIndex(LC_MFW_NECK)].Offset, Root);
		HeadOffset = 0.08f;
	}

	if (m_Info[LC_MFW_HEAD])
	{
		Mat.CreateFromAxisAngle(Vector3(0, 0, 1), -LC_DTOR * m_Angles[LC_MFW_HEAD]);
		Mat.SetTranslation(Vector3(0.0f, 0.0f, 0.96f + HeadOffset));
		Mat = Mul(mSettings[LC_MFW_HEAD][GetSelectionIndex(LC_MFW_HEAD)].Offset, Mat);
		m_Matrices[LC_MFW_HEAD] = Mul(Mat, Root);
	}

	if (m_Info[LC_MFW_HAT])
	{
		Mat.CreateFromAxisAngle(Vector3(0, 0, 1), -LC_DTOR * m_Angles[LC_MFW_HAT]);
		Mat = Mul(mSettings[LC_MFW_HAT][GetSelectionIndex(LC_MFW_HAT)].Offset, Mat);
		m_Matrices[LC_MFW_HAT] = Mul(Mat, m_Matrices[LC_MFW_HEAD]);
	}

	if (m_Info[LC_MFW_RIGHT_ARM])
	{
		Mat.CreateFromAxisAngle(Vector3(1, 0, 0), -LC_DTOR * m_Angles[LC_MFW_RIGHT_ARM]);

		if (DroidTorso || SkeletonTorso)
			Mat2.LoadIdentity();
		else
			Mat2.CreateFromAxisAngle(Vector3(0, 1, 0), LC_DTOR * 9.791f);
		Mat2.SetTranslation(Vector3(-0.62f, 0, -0.32f));

		Mat = Mul(mSettings[LC_MFW_RIGHT_ARM][GetSelectionIndex(LC_MFW_RIGHT_ARM)].Offset, Mat);
		Mat = Mul(Mat, Mat2);
		m_Matrices[LC_MFW_RIGHT_ARM] = Mul(Mat, Root);
	}

	if (m_Info[LC_MFW_RIGHT_HAND])
	{
		Mat.CreateFromAxisAngle(Vector3(0, 1, 0), -LC_DTOR * m_Angles[LC_MFW_RIGHT_HAND]);
		Mat2.CreateFromAxisAngle(Vector3(1, 0, 0), LC_DTOR * 45);
		Mat = Mul(mSettings[LC_MFW_RIGHT_HAND][GetSelectionIndex(LC_MFW_RIGHT_HAND)].Offset, Mat);
		Mat = Mul(Mat, Mat2);
		Mat.SetTranslation(Vector3(-0.2f, -0.4f, -0.76f));
		m_Matrices[LC_MFW_RIGHT_HAND] = Mul(Mat, m_Matrices[LC_MFW_RIGHT_ARM]);
	}

	if (m_Info[LC_MFW_RIGHT_TOOL])
	{
		Mat.CreateFromAxisAngle(Vector3(0, 0, 1), LC_DTOR * m_Angles[LC_MFW_RIGHT_TOOL]);
		Mat.SetTranslation(Vector3(0, -0.4f, 0));
		Mat = Mul(mSettings[LC_MFW_RIGHT_TOOL][GetSelectionIndex(LC_MFW_RIGHT_TOOL)].Offset, Mat);
		m_Matrices[LC_MFW_RIGHT_TOOL] = Mul(Mat, m_Matrices[LC_MFW_RIGHT_HAND]);
	}

	if (m_Info[LC_MFW_LEFT_ARM])
	{
		Mat.CreateFromAxisAngle(Vector3(1, 0, 0), -LC_DTOR * m_Angles[LC_MFW_LEFT_ARM]);

		if (DroidTorso || SkeletonTorso)
			Mat2.LoadIdentity();
		else
			Mat2.CreateFromAxisAngle(Vector3(0, 1, 0), -LC_DTOR * 9.791f);
		Mat2.SetTranslation(Vector3(0.62f, 0.0f, -0.32f));

		Mat = Mul(mSettings[LC_MFW_LEFT_ARM][GetSelectionIndex(LC_MFW_LEFT_ARM)].Offset, Mat);
		Mat = Mul(Mat, Mat2);
		m_Matrices[LC_MFW_LEFT_ARM] = Mul(Mat, Root);
	}

	if (m_Info[LC_MFW_LEFT_HAND])
	{
		Mat.CreateFromAxisAngle(Vector3(0, 1, 0), -LC_DTOR * m_Angles[LC_MFW_LEFT_HAND]);
		Mat2.CreateFromAxisAngle(Vector3(1, 0, 0), LC_DTOR * 45);
		Mat = Mul(mSettings[LC_MFW_LEFT_HAND][GetSelectionIndex(LC_MFW_LEFT_HAND)].Offset, Mat);
		Mat = Mul(Mat, Mat2);
		Mat.SetTranslation(Vector3(0.2f, -0.4f, -0.76f));
		m_Matrices[LC_MFW_LEFT_HAND] = Mul(Mat, m_Matrices[LC_MFW_LEFT_ARM]);
	}

	if (m_Info[LC_MFW_LEFT_TOOL])
	{
		Mat.CreateFromAxisAngle(Vector3(0, 0, 1), LC_DTOR * m_Angles[LC_MFW_LEFT_TOOL]);
		Mat.SetTranslation(Vector3(0, -0.4f, 0));
		Mat = Mul(mSettings[LC_MFW_LEFT_TOOL][GetSelectionIndex(LC_MFW_LEFT_TOOL)].Offset, Mat);
		m_Matrices[LC_MFW_LEFT_TOOL] = Mul(Mat, m_Matrices[LC_MFW_LEFT_HAND]);
	}

	if (m_Info[LC_MFW_HIPS])
	{
		Mat.LoadIdentity();
		Mat.SetTranslation(Vector3(0, 0, -1.28f));
		Mat = Mul(mSettings[LC_MFW_HIPS][GetSelectionIndex(LC_MFW_HIPS)].Offset, Mat);
		m_Matrices[LC_MFW_HIPS] = Mul(Mat, Root);
	}

	if (m_Info[LC_MFW_RIGHT_LEG])
	{
		Mat.CreateFromAxisAngle(Vector3(1, 0, 0), -LC_DTOR * m_Angles[LC_MFW_RIGHT_LEG]);
		Mat.SetTranslation(Vector3(0, 0, -1.76f));
		Mat = Mul(mSettings[LC_MFW_RIGHT_LEG][GetSelectionIndex(LC_MFW_RIGHT_LEG)].Offset, Mat);
		m_Matrices[LC_MFW_RIGHT_LEG] = Mul(Mat, Root);
	}

	if (m_Info[LC_MFW_RIGHT_SHOE])
	{
		Vector3 Center(-0.4f, -0.04f, -1.12f);
		Mat.CreateFromAxisAngle(Vector3(0, 0, 1), LC_DTOR * m_Angles[LC_MFW_RIGHT_SHOE]);
		Mat2 = mSettings[LC_MFW_RIGHT_SHOE][GetSelectionIndex(LC_MFW_RIGHT_SHOE)].Offset;
		Mat2.SetTranslation(Mul31(-Center, Mat2));
		Mat = Mul(Mat2, Mat);
		Mat.SetTranslation(Mul31(Center, Mat2));
		m_Matrices[LC_MFW_RIGHT_SHOE] = Mul(Mat, m_Matrices[LC_MFW_RIGHT_LEG]);
	}

	if (m_Info[LC_MFW_LEFT_LEG])
	{
		Mat.CreateFromAxisAngle(Vector3(1, 0, 0), -LC_DTOR * m_Angles[LC_MFW_LEFT_LEG]);
		Mat.SetTranslation(Vector3(0, 0, -1.76f));
		Mat = Mul(mSettings[LC_MFW_LEFT_LEG][GetSelectionIndex(LC_MFW_LEFT_LEG)].Offset, Mat);
		m_Matrices[LC_MFW_LEFT_LEG] = Mul(Mat, Root);
	}

	if (m_Info[LC_MFW_LEFT_SHOE])
	{
		Vector3 Center(0.4f, -0.04f, -1.12f);
		Mat.CreateFromAxisAngle(Vector3(0, 0, 1), LC_DTOR * m_Angles[LC_MFW_LEFT_SHOE]);
		Mat2 = mSettings[LC_MFW_LEFT_SHOE][GetSelectionIndex(LC_MFW_LEFT_SHOE)].Offset;
		Mat2.SetTranslation(Mul31(-Center, Mat2));
		Mat = Mul(Mat2, Mat);
		Mat.SetTranslation(Mul31(Center, Mat2));
		m_Matrices[LC_MFW_LEFT_SHOE] = Mul(Mat, m_Matrices[LC_MFW_LEFT_LEG]);
	}
}

int MinifigWizard::GetSelectionIndex(int Type) const
{
	const ObjArray<lcMinifigPieceInfo>& InfoArray = mSettings[Type];

	for (int Index = 0; Index < InfoArray.GetSize(); Index++)
		if (InfoArray[Index].Info == m_Info[Type])
			return Index;

	return 0;
}

void MinifigWizard::SetSelectionIndex(int Type, int Index)
{
	if (m_Info[Type])
		m_Info[Type]->DeRef();

	m_Info[Type] = mSettings[Type][Index].Info;

	if (m_Info[Type])
		m_Info[Type]->AddRef();

	Calculate();
}

void MinifigWizard::SetColor(int Type, int Color)
{
	m_Colors[Type] = Color;
}

void MinifigWizard::SetAngle(int Type, float Angle)
{
	m_Angles[Type] = Angle;
}

void MinifigWizard::GetMinifigNames(char ***names, int *count)
{
	*count = m_MinifigCount;
	*names = m_MinifigNames;
}

void MinifigWizard::SaveMinifig(const char* name)
{
	char tmp[LC_PIECE_NAME_LEN];
	int i, j;

	// check if the name is already being used
	for (i = 0; i < m_MinifigCount; i++)
		if (strcmp(m_MinifigNames[i], name) == 0)
			break;

	if (i == m_MinifigCount)
	{
		m_MinifigCount++;
		m_MinifigNames = (char**)realloc(m_MinifigNames, sizeof(char**)*m_MinifigCount);
		m_MinifigTemplates = (char**)realloc(m_MinifigTemplates, sizeof(char**)*m_MinifigCount);
		m_MinifigNames[i] = (char*)malloc(strlen(name) + 1);
		strcpy(m_MinifigNames[i], name);
		m_MinifigTemplates[i] = (char*)malloc(768);
	}
	strcpy(m_MinifigTemplates[i], "");

	for (j = 0; j < LC_MFW_NUMITEMS; j++)
	{
		sprintf(tmp, "%d ", m_Colors[j]);
		strcat(m_MinifigTemplates[i], tmp);
	}

	for (j = 0; j < LC_MFW_NUMITEMS; j++)
	{
		if (m_Info[j] != NULL)
			sprintf(tmp, "%s ", m_Info[j]->m_strName);
		else
			strcpy(tmp, "None ");
		strcat(m_MinifigTemplates[i], tmp);
	}

	for (j = 0; j < LC_MFW_NUMITEMS; j++)
	{
		sprintf(tmp, "%f ", m_Angles[j]);
		strcat(m_MinifigTemplates[i], tmp);
	}
}

bool MinifigWizard::LoadMinifig(const char* name)
{
	char *ptr;
	int i, j;

	// check if the name is valid
	for (i = 0; i < m_MinifigCount; i++)
		if (strcmp(m_MinifigNames[i], name) == 0)
			break;

	if (i == m_MinifigCount)
	{
		//    Sys_MessageBox("Unknown Minifig");
		return false;
	}
	else
		ptr = m_MinifigTemplates[i];

	for (j = 0; j < LC_MFW_NUMITEMS; j++)
		if (m_Info[j] != NULL)
			m_Info[j]->DeRef();

	for (j = 0; j < LC_MFW_NUMITEMS; j++)
		m_Colors[j] = strtol(ptr, &ptr, 10);

	for (j = 0; j < LC_MFW_NUMITEMS; j++)
	{
		char *endptr;
		ptr++;

		endptr = strchr(ptr, ' ');
		*endptr = '\0';
		m_Info[j] = lcGetPiecesLibrary()->FindPieceInfo(ptr);
		*endptr = ' ';
		ptr = endptr;

		if (m_Info[j] != NULL)
			m_Info[j]->AddRef();
	}

	for (j = 0; j < LC_MFW_NUMITEMS; j++)
		m_Angles[j] = (float)strtod(ptr, &ptr);

	return true;
}

void MinifigWizard::DeleteMinifig(const char* name)
{
	int i;

	// check if the name is valid
	for (i = 0; i < m_MinifigCount; i++)
		if (strcmp(m_MinifigNames[i], name) == 0)
			break;

	if (i == m_MinifigCount)
	{
		Sys_MessageBox("Unknown Minifig");
		return;
	}

	free(m_MinifigNames[i]);
	free(m_MinifigTemplates[i]);
	m_MinifigCount--;

	for (; i < m_MinifigCount; i++)
	{
		m_MinifigNames[i] = m_MinifigNames[i+1];
		m_MinifigTemplates[i] = m_MinifigTemplates[i+1];
	}
}

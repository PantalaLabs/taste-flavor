/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef tf_Defines_h
#define tf_Defines_h

#define DO_WT //do everything related to WaveTrigger ,comment to not
#define DEB   //debug mode, comment to not

//encoders
#define MAXENCODERS 8  //total of encoders
#define MOODENCODER 0  //MOOD encoder id
#define CROSSENCODER 1 //CROSS encoder id

#define RAM 0              //BKP pattern
#define ROM 1              //mute pattern
#define G_MAXINSTRUMENTS 6 //total of instruments
#define G_MAXSTEPS 64      //max step sequence
#define G_BLOCKBITS 32     //number os bits of each pattern bitblock
//#define G_MAXBLOCKS 8              //number os blobks of each pattern
#define G_MAXGATELENGHTS 12        //max number of different gatelenghts
#define G_DEFAULTGATELENGHT 5000   //first gatelenght in ms
#define G_EXTENDEDGATELENGHT 20000 //gate lenght size increment

#define G_MAXMEMORYMOODS 300 //max number of moods
#define G_MAXBPM 160
#define G_MINBPM 60

//#define I2C_ADDRESS 0x3D
#define I2C_ADDRESS 0x3C
#define DISPLAY_WIDTH 128 // display width, in pixels
#define DISPLAY_HEIGHT 64 // display height, in pixels
#define TEXTLINE_HEIGHT 9 // text line height, in pixels

//ladder menu
#define MAXPARAMETERS 9
#define COMMANDSMP 0
#define COMMANDGHO 1
#define COMMANDTAP 2
#define COMMANDUND 3
#define COMMANDSIL 4
#define COMMANDERS 5
#define COMMANDGTL 6
#define COMMANDLAT 7
#define COMMANDSOL 8
#define COMMANDNUL 9

#endif

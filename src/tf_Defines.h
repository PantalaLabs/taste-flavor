/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef tf_Defines_h
#define tf_Defines_h

#define DO_SERIAL false            //debugging mode
#define DO_SD false                //do everything related to SD card
#define DO_WT true                 //do everything related to WaveTrigger

#define SD_CS 43                   //sd card chip selelect
#define RAM 0                      //changeble patterns
#define ROM 1                      //original reference patterns
#define G_MAXINSTRUMENTS 6         //total of instruments
#define G_MAXMEMORYMOODS 300       //max number of moods
#define G_MAXSTEPS 64              //max step sequence
#define G_MAXGATELENGHTS 8         //max number of different gatelenghts
#define G_INTERNALMOODS 6          //internal moods
#define G_INTERNALINSTR1PATTERNS 2 //only valid patterns + mute
#define G_INTERNALINSTR2PATTERNS 6
#define G_INTERNALINSTR3PATTERNS 5
#define G_INTERNALINSTR4PATTERNS 6
#define G_INTERNALINSTR5PATTERNS 6
#define G_INTERNALINSTR6PATTERNS 4
#define G_DEFAULTGATELENGHT 5000   //first gatelenght in ms
#define G_EXTENDEDGATELENGHT 40000 //gate lenght size increment
#define MOODFILE "MOODS.TXT"
#define INTRUMENT1PATTERNFILE "INSTR1.TXT"
#define INTRUMENT2PATTERNFILE "INSTR2.TXT"
#define INTRUMENT3PATTERNFILE "INSTR3.TXT"
#define INTRUMENT4PATTERNFILE "INSTR4.TXT"
#define INTRUMENT5PATTERNFILE "INSTR5.TXT"
#define INTRUMENT6PATTERNFILE "INSTR6.TXT"
//#define I2C_ADDRESS 0x3D
#define I2C_ADDRESS 0x3C
#define DISPLAY_WIDTH 128 // display width, in pixels
#define DISPLAY_HEIGHT 64 // display height, in pixels
#define TEXTLINE_HEIGHT 9 // text line height, in pixels

#define MAXMELODYPARMS 7 //max ppotentiometer arameters

#endif

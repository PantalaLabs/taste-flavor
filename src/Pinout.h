/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#ifndef Pinout_h
#define Pinout_h

//=======================communication pins=======================
#define RXPIN 17
#define TXPIN 18
#define SDAPIN 20
#define SCLPIN 21

//=======================digital pins=======================
//triggers
#define TRIGGERINPIN 51
#define RESETINPIN 53
#define TRIGOUTPATTERNPIN1 7
#define TRIGOUTPIN1 5
#define TRIGOUTPIN2 3
#define TRIGOUTPIN3 16
#define TRIGOUTPIN4 29
#define TRIGOUTPIN5 23
#define TRIGOUTPIN6 27

#define PARAMACTIVITY 47

//action buttons
#define ACTIONPININSTR1 6
#define ACTIONPININSTR2 4
#define ACTIONPININSTR3 2
#define ACTIONPININSTR4 17
#define ACTIONPININSTR5 31
#define ACTIONPININSTR6 25

//encoders
#define ENCPINAMOOD 13
#define ENCPINBMOOD 12
#define ENCBUTPINMOOD 11
#define ENCPINACROSS 10
#define ENCPINBCROSS 9
#define ENCBUTPINCROSS 8
#define ENCPINAINSTR1 38
#define ENCPINBINSTR1 36
#define ENCBUTPININSTR1 40
#define ENCPINAINSTR2 32
#define ENCPINBINSTR2 30
#define ENCBUTPININSTR2 34
#define ENCPINAINSTR3 44
#define ENCPINBINSTR3 42
#define ENCBUTPININSTR3 46
#define ENCPINAINSTR4 14
#define ENCPINBINSTR4 15
#define ENCBUTPININSTR4 22
#define ENCBUTPININSTR5 52
#define ENCPINAINSTR5 50
#define ENCPINBINSTR5 48
#define ENCBUTPININSTR6 28
#define ENCPINAINSTR6 26
#define ENCPINBINSTR6 24

//encoder buttons
#define ENCBUTMOOD 0
#define ENCBUTCROSS 1
#define ENCBUTINSTR1 2
#define ENCBUTINSTR2 3
#define ENCBUTINSTR3 4
#define ENCBUTINSTR4 5
#define ENCBUTINSTR5 6
#define ENCBUTINSTR6 7

//sd card chip selelect
#define SD_CS 43

//=======================analog pins=======================
#define G_LADDERMENUPIN A6   //laddr menu
#define G_MELODYPARAMPIN0 A0 //melody parameters
#define G_MELODYPARAMPIN1 A1
#define G_MELODYPARAMPIN2 A2
#define G_MELODYPARAMPIN3 A3
#define G_MELODYPARAMPIN4 A4
#define G_MELODYPARAMPIN5 A5
#define G_MELODYPARAMPIN6 A7

#endif

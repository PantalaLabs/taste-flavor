#define BASE10BITVOLTAGE 512
#define BASE10BITMIDISTEPVOLTAGE 17.06

int16_t mapMidi2Vol(int16_t note, uint8_t dac);

String midiNote[] = {
    "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0", //
    "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1", //
    "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2", //
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3", //
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4", //
    "C5"};

#define MIDIMAXNOTES 60
int8_t midiTable[] = {
    //C  C# D  D# E  F  F# G  G# A  A#  B
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59};

#define MAJMAXNOTES 35
uint8_t majTable[] = {
    0, 2, 4, 5, 7, 9, 11, //scale01 Major
    12, 14, 16, 17, 19, 21, 23,
    24, 26, 28, 29, 31, 33, 35,
    36, 38, 40, 41, 43, 45, 47,
    48, 50, 52, 53, 55, 57, 59};

#define MINMAXNOTES 35
uint8_t minTable[] = {
    0, 2, 3, 5, 7, 8, 10, //scale02 Minor
    12, 14, 15, 17, 19, 20, 22,
    24, 26, 27, 29, 31, 32, 34,
    36, 38, 39, 41, 43, 44, 46,
    48, 50, 51, 53, 55, 56, 58};

#define PENTAMAXNOTES 30
uint8_t pentaTable[] = {
    0, 2, 3, 5, 7, 10, //scale03 Pentatonic
    12, 14, 15, 17, 19, 22,
    24, 26, 27, 29, 31, 34,
    36, 38, 39, 41, 43, 46,
    48, 50, 51, 53, 55, 58};

#define DORIANMAXNOTES 35
uint8_t dorianTable[] = {
    0, 2, 3, 5, 7, 9, 10, //scale04 Dorian
    12, 14, 15, 17, 19, 21, 22,
    24, 26, 27, 29, 31, 33, 34,
    36, 38, 39, 41, 43, 45, 46,
    48, 50, 51, 53, 55, 57, 58};

#define MAJ3RDMAXNOTES 21
uint8_t maj3rdTable[] = {
    0, 4, 7, 11, //scale05Maj7(9)
    12, 16, 19, 23,
    24, 26, 31, 35,
    36, 40, 43, 47,
    48, 50, 54, 55, 59};

#define MIN3RDMAXNOTES 22
uint8_t min3rdTable[] = {
    0, 3, 7, 10, //scale06 Minor7(9,11)
    14, 15, 19, 22,
    24, 26, 27, 31, 34,
    36, 39, 41, 46,
    48, 50, 51, 55, 58};

#define WTMAXNOTES 30
uint8_t whTable[] = {
    0, 2, 4, 6, 8, 10, //scale07 (WholeTone)
    12, 14, 16, 18, 20, 22,
    24, 26, 28, 30, 32, 34,
    36, 38, 40, 42, 44, 46,
    48, 50, 52, 54, 56, 58};

#define CHROMAMAXNOTES 60
uint8_t chromaTable[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, //scale08 Chromatic
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59};

// uint8_t scaleArray[8] = {#define MAJMAXNOTES 35
// #define MINMAXNOTES 35
// #define PENTAMAXNOTES 30
// #define DORIANMAXNOTES 35
// #define MAJ3RDMAXNOTES 21
// #define MIN3RDMAXNOTES 22
// #define WTMAXNOTES 30
// #define CHROMAMAXNOTES 60
// }
int16_t mapMidi2Volt(int16_t note, uint8_t dac)
{
  return map(note, 0, 60, 0, 4095);
}

uint16_t map10bitAnalog2Scaled5octMidiNote(uint16_t analogReadValue, uint8_t scaleTable[])
{
  return (scaleTable[map(analogReadValue, 0, 1024, 0, sizeof(scaleTable))]);
}

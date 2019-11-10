/*SdComm
 
  Creative Commons License CC-BY-SA
  Planets by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.

Usage:

#include <SdComm.h>
SdComm *sdc;

String moodKitName[5] = {"Test-teste"};
uint16_t moodKitPatterns[5][6] = {{1, 1, 1, 1, 1, 1}};
uint8_t originalMoodQtty = 1;
..
Serial.begin(9600);
sdc = new SdComm(43, true);
//sdc->createTestMoods();
sdc->importAllMoods(moodKitName, moodKitPatterns, originalMoodQtty);
uint16_t imp = sdc->getImportedMoods();
for (uint8_t j = 0; j < (originalMoodQtty + imp); j++)
{
  Serial.print(moodKitName[j]);
  Serial.print(",");
  for (uint8_t i = 0; i < 6; i++)
  {
    Serial.print(moodKitPatterns[j][i]);
    Serial.print(",");
  }
  Serial.println("");
}

*/

#ifndef SdComm_h
#define SdComm_h

#include "Arduino.h"
#include <SPI.h>
#include <SD.h>

class SdComm
{
public:
  SdComm();
  SdComm(uint8_t _cspin);
  SdComm(uint8_t _cspin, boolean _dbg);
  boolean createTestMoods();
  boolean dumpOneMood(String _name, uint8_t _p1, uint8_t _p2, uint8_t _p3, uint8_t _p4, uint8_t _p5, uint8_t _p6);
  void importAllMoods(String refKitName[], uint16_t refKitPatterns[][7], uint16_t moods);
  uint16_t getImportedMoods();

private:
  File myFile;
  boolean _debug;
  void init();
  boolean deleteFile();
  boolean openFileToWrite();
  boolean openFileToAppend();
  boolean openFileToRead();
  boolean closeFile();
  uint16_t importedMoods;
  void debug(String _str);
};

#endif

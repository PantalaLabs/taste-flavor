/*SdComm
  
  Creative Commons License CC-BY-SA
  Planets by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "Arduino.h"
#include "SdComm.h"
#include "PantalaDefines.h"
#include <SPI.h>
#include <SD.h>

SdComm::SdComm()
{
  _debug = false;
}

SdComm::SdComm(uint8_t _cspin)
{
  _debug = false;
  init();
}

SdComm::SdComm(uint8_t _cspin, boolean _dbg)
{
  _debug = _dbg;
  init();
}

void SdComm::init()
{
  debug("Initializing SD card...");
  randomSeed(analogRead(0));
  if (!SD.begin(43))
    debug("initialization failed!");
  else
    debug("initialization done!");
  delay(100);
}

boolean SdComm::deleteFile()
{
  if (!SD.remove("MOODS.TXT"))
  {
    debug("remove failed!");
    return false;
  }
  return true;
}

boolean SdComm::openFileToRead()
{
  myFile = SD.open("MOODS.TXT", O_READ);
  if (myFile)
    return true;
  debug("open to read failed");
  return false;
}

boolean SdComm::openFileToAppend()
{
  myFile = SD.open("MOODS.TXT", O_APPEND);
  if (myFile)
    return true;
  debug("open to append failed");
  return false;
}

boolean SdComm::closeFile()
{
  myFile.close();
  debug("closed");
}

boolean SdComm::dumpOneMood(String _name, uint8_t _p1, uint8_t _p2, uint8_t _p3, uint8_t _p4, uint8_t _p5, uint8_t _p6)
{
  if (!myFile)
  {
    debug("dump mood failed");
    return false;
  }
  debug("dump mood");
  String moodString = String(_name);
  moodString += ",";
  moodString += String(_p1);
  moodString += ",";
  moodString += String(_p2);
  moodString += ",";
  moodString += String(_p3);
  moodString += ",";
  moodString += String(_p4);
  moodString += ",";
  moodString += String(_p5);
  moodString += ",";
  moodString += String(_p6);
  moodString += ",";
  myFile.println(String(moodString));
  debug(moodString);
  debug("dump mood end");
}

uint16_t SdComm::importAllMoods(String refKitName[], uint16_t refKitPatterns[][6], uint16_t _intMoods)
{
  if (openFileToRead())
  {
    debug("load data");
    char character;
    uint16_t importedMood = 0;
    while (myFile.available())
    {
      for (uint8_t i = 0; i < 7; i++)
      {
        //clear token
        String token = "";
        uint8_t strLen = 0;

        //read one entire token
        character = myFile.read();
        while ((character != '\n') && (character != ',') && (myFile.available()))
        {
          strLen++;
          token.concat(character);
          character = myFile.read();
        }
        //if string or int
        if (i == 0)
        {
          refKitName[_intMoods + importedMood] = token.substring(0, strLen);
          Serial.print("-");
          Serial.println(token.c_str());
        }
        else if ((i >= 1) && (i <= 6))
        {
          refKitPatterns[_intMoods + importedMood][i - 1] = token.toInt();
          Serial.print("int ");
          Serial.println(token.c_str());
        }
        importedMood++;
      }
      //end of reading one mood config
      character = myFile.read();
      character = myFile.read();
    }
    closeFile();
    return importedMood;
  }
  return 0;
}

boolean SdComm::createTestArray()
{
  //  myFile = SD.open("MOODS.TXT", FILE_WRITE);
  //O_READ | O_WRITE | O_CREAT | O_APPEND | FILE_WRITE
  if (!deleteFile())
  {
    debug("delete failed");
    return false;
  }
  if (!openFileToAppend())
  {
    debug("open to append failed");
    return false;
  }
  if (myFile)
  {
    debug("Creating new file");
    for (uint8_t j = 0; j < 3; j++)
    {
      String moodString = "Moodname";
      moodString += String(j);
      dumpOneMood(moodString, random(8), random(8), random(8), random(8), random(8), random(8));
    }
    closeFile();
    return true;
  }
  return false;
}

void SdComm::debug(String _str)
{
  if (_debug)
    Serial.println(_str);
}

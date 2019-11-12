/*SdComm
  
  Creative Commons License CC-BY-SA
  Planets by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_defines.h"
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
  delay(500);
}

boolean SdComm::deleteFile()
{
  if (!SD.remove("MOODS.TXT"))
  {
    debug("delete failed!");
    return false;
  }
  else
  {
    debug("deletet ok!");
    return true;
  }
}

boolean SdComm::openFileToRead(String _fileName)
{
  myFile = SD.open(_fileName, O_READ);
  if (myFile)
    return true;
  debug("open to read failed");
  return false;
}

boolean SdComm::openFileToAppend(String _fileName)
{
  myFile = SD.open(_fileName, O_APPEND);
  if (myFile)
    return true;
  debug("open to append failed");
  return false;
}

boolean SdComm::openFileToWrite(String _fileName)
{
  myFile = SD.open(_fileName, FILE_WRITE);
  if (myFile)
    return true;
  debug("open to write failed");
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

void SdComm::importInstrumentPatterns(uint8_t instr, uint16_t refPatternTable[][G_MAXSTEPS], uint16_t startIndex)
{
  String patternFile = "PATTERN";
  patternFile += String(instr);
  patternFile += ".TXT";
  if (openFileToRead(patternFile))
  {
    debug("loading patterns from instrument");
    char character;
    importedRecords = 0;
    while (myFile.available())
    {
      for (uint8_t i = 0; i < G_MAXSTEPS; i++)
      {
        //clear token
        String token = "";
        uint8_t strLen = 0;
        //read one full token
        character = myFile.read();
        while ((character != '\n') && (character != ',') && (myFile.available()))
        {
          strLen++;
          token.concat(character);
          character = myFile.read();
        }
        refPatternTable[G_INTERNALINSTR1PATTERNS + importedRecords][i] = token.toInt();
        Serial.println(token.c_str());
      }
      importedRecords++;
      //read "crlf"
      character = myFile.read();
      character = myFile.read();
    }
    closeFile();
  }
}

void SdComm::importMoods(String refMoodName[], uint16_t refMoodData[][7], uint16_t startIndex)
{
  if (openFileToRead("MOODS.TXT"))
  {
    debug("load data");
    char character;
    importedRecords = 0;
    while (myFile.available())
    {
      for (uint8_t i = 0; i < 7; i++)
      {
        //clear token
        String token = "";
        uint8_t strLen = 0;
        //read one full token
        character = myFile.read();
        while ((character != '\n') && (character != ',') && (myFile.available()))
        {
          strLen++;
          token.concat(character);
          character = myFile.read();
        }
        //if token is string or int
        if (i == 0)
        {
          refMoodName[startIndex + importedRecords] = token.substring(0, strLen);
          Serial.print("-");
          Serial.println(token.c_str());
        }
        else if ((i >= 1) && (i <= 6))
        {
          refMoodData[startIndex + importedRecords][i - 1] = token.toInt();
          Serial.print("int ");
          Serial.println(token.c_str());
        }
      }
      importedRecords++;
      //read "crlf"
      character = myFile.read();
      character = myFile.read();
    }
    closeFile();
  }
}

boolean SdComm::createTestMoods()
{
  deleteFile();
  if (!openFileToWrite(MOODFILE))
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
uint16_t SdComm::getimportedRecords()
{
  return importedRecords;
}

void SdComm::debug(String _str)
{
  if (_debug)
    Serial.println(_str);
}

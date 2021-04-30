#include <PantalaDefines.h>

#define DOTGRIDINIT 36
#define GRIDPATTERNHEIGHT 4

void checkDefaultDisplay();
void displayEraseInstrumentPattern(uint8_t _instr);
void displayShowInstrPattern(uint8_t _instr, bool _src);
void displayShowBrowsedMood();
void displayUpdateLineArea(uint8_t _line, String _content);
void displayEraseLineBlockAndSetText(uint8_t _line);
void displayShowCornerInfo(uint8_t _parm, int16_t _val);
void displayShowCrossBar(int8_t _size);
void displayClockSource();
void displaySaved();
void displayWelcome();
bool crossfadedDeck(uint8_t _instr);

//check if default display wasnt show yet
void checkDefaultDisplay()
{
  if (defaultDisplayNotActiveYet)
  {
    defaultDisplayNotActiveYet = false;
    display.clearDisplay();
    display.drawRect(0, 0, 60, TEXTLINE_HEIGHT - 2, WHITE);
    displayShowBrowsedMood();
    displayShowCrossBar(-1);
  }
}

void displayShowCrossBar(int8_t _size) //update crossing status / 6 steps of 10 pixels each
{
  if (lastCrossBarGraphValue != _size)
  {
    display.fillRect(1, 1, 58, TEXTLINE_HEIGHT - 4, BLACK);
    for (int8_t i = 0; i < _size; i++)
      display.fillRect(10 * i, 0, 10, TEXTLINE_HEIGHT - 2, WHITE);
    lastCrossBarGraphValue = _size;
  }
}

void displayShowCornerInfo(String _str) //update display right upper corner with the actual sample or pattern number
{
  display.fillRect(70, 0, DISPLAY_WIDTH - 70, TEXTLINE_HEIGHT, BLACK);
  display.setCursor(70, 0);
  display.print(_str);
}

void displayShowCornerInfo(String _str, int16_t _val) //update display right upper corner with the actual sample or pattern number
{
  displayShowCornerInfo(_str);
  // display.fillRect(70, 0, DISPLAY_WIDTH - 70, TEXTLINE_HEIGHT, BLACK);
  // display.setCursor(70, 0);
  // display.print(_str);
  display.print(":");
  display.print(_val);
}

void displayEraseLineBlockAndSetText(uint8_t _line)
{
  display.fillRect(0, _line * TEXTLINE_HEIGHT, DISPLAY_WIDTH, TEXTLINE_HEIGHT - 1, BLACK);
  display.setCursor(0, _line * TEXTLINE_HEIGHT); //set cursor position
}

void displayUpdateLineArea(uint8_t _line, String _content)
{
  displayEraseLineBlockAndSetText(_line);
  display.print(_content); //print previous mood name
}

int16_t lastBrowsedMood;
void displayShowBrowsedMood() //update almost all bottom display area with the name of the selected mood and all 6 available instruments
{
  if (lastBrowsedMood != selectedMood)
  {
    lastBrowsedMood = selectedMood;
    displayUpdateLineArea(3, moodarray[selectedMood].name);
    for (uint8_t pat = 0; pat < G_MAXINSTRUMENTS; pat++) //for each pattern
      displayShowInstrPattern(pat, ROM);
  }
}

void displayShowInstrPattern(uint8_t _pat, bool _src)
{
  uint8_t patternSrc;

  displayEraseInstrumentPattern(_pat);
  patternSrc = (_src == ROM) ? moodarray[selectedMood].pattern[_pat] : mood[thisDeck]->patterns[_pat]->patternIndex->getValue();
  for (byte step = 0; step < (G_MAXSTEPS - 1); step++) //for each step
    if (mood[thisDeck]->patterns[_pat]->getStep(patternSrc, step))
      display.fillRect(step * 2, DOTGRIDINIT + (_pat * GRIDPATTERNHEIGHT), 2, GRIDPATTERNHEIGHT - 1, WHITE);
}

//erase exatctly one line pattern
void displayEraseInstrumentPattern(uint8_t _instr)
{
  display.fillRect(0, DOTGRIDINIT + (_instr * GRIDPATTERNHEIGHT), DISPLAY_WIDTH, GRIDPATTERNHEIGHT - 1, BLACK);
}

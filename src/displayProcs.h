void checkDefaultDisplay();
void displayEraseInstrumentBlock(uint8_t _instr);
void displayShowInstrPattern(uint8_t _instr, boolean _src);
void displayShowBrowsedMood();
void displayUpdateLineArea(uint8_t _line, String _content);
void displayEraseLineBlockAndSetText(uint8_t _line);
void displayShowCornerInfo(uint8_t _parm, int16_t _val);
void displayShowCrossBar(int8_t _size);
void displayWelcome();
boolean crossfadedDeck(uint8_t _instr);

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

void displayWelcome()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("<-- select your mood"));
  display.println(F("    and cross it -->"));
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

void displayShowCornerInfo(uint8_t _parm, int16_t _val) //update display right upper corner with the actual sample or pattern number
{
  String rightCornerInfo[5] = {"pat", "smp", "bpm", "ms", "len"};
  display.fillRect(70, 0, DISPLAY_WIDTH - 70, TEXTLINE_HEIGHT, BLACK);
  display.setCursor(70, 0);
  display.print(rightCornerInfo[_parm]);
  display.print(":");
  switch (_parm)
  {
  case 0:
  case 1:
  case 2:
  case 3:
    display.print(_val);
    break;
  case 4:
    display.print((G_DEFAULTGATELENGHT + (mood[crossfadedDeck(_val)]->patterns[_val]->gateLenghtSize * G_EXTENDEDGATELENGHT)) / 1000);
  }
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

uint8_t lastBrowsedMood;
void displayShowBrowsedMood() //update almost all bottom display area with the name of the selected mood and all 6 available instruments
{
  if (lastBrowsedMood != selectedMood)
  {
    lastBrowsedMood = selectedMood;
    displayUpdateLineArea(3, moodKitName[selectedMood]);
    for (uint8_t instr = 0; instr < G_MAXINSTRUMENTS; instr++) //for each instrument
      displayShowInstrPattern(instr, ROM);
  }
}

void displayShowInstrPattern(uint8_t _instr, boolean _src)
{
  displayEraseInstrumentBlock(_instr);
  for (int8_t step = 0; step < (G_MAXSTEPS - 1); step++) //for each step
  {
    //if browsed mood (ROM) or individual pattern browse (RAM)
    // if (((_src == ROM) && (originalPattern[_instr]->getStep(moodKitData[selectedMood][_instr], step))) ||
    //     ((_src == RAM) && (mood[thisDeck]->patterns[_instr]->getStep(mood[thisDeck]->patterns[_instr]->id->getValue(), step))))
    if (((_src == ROM) && (mood[thisDeck]->patterns[_instr]->getStep(moodKitData[selectedMood][_instr], step, ROM))) ||
        ((_src == RAM) && (mood[thisDeck]->patterns[_instr]->getStep(mood[thisDeck]->patterns[_instr]->id->getValue(), step))))
      display.fillRect(step * 2, DOTGRIDINIT + (_instr * GRIDPATTERNHEIGHT), 2, GRIDPATTERNHEIGHT - 1, WHITE);
  }
}

//erase exatctly one line pattern
void displayEraseInstrumentBlock(uint8_t _instr)
{
  display.fillRect(0, DOTGRIDINIT + (_instr * GRIDPATTERNHEIGHT), DISPLAY_WIDTH, GRIDPATTERNHEIGHT - 1, BLACK);
}

/*  
* Contains all the functions for the user interface.
*
* Reads the buttons and controls the lcd / buzzer.
*
* Released under WTFPL license.
*/


void InitializeDisplay()
{
  // Degree symbol for LCD
  unsigned char degree[8]  = {140,146,146,140,128,128,128,128};

  hardware.LCD.begin(16, 2);
  hardware.LCD.createChar(0, degree);
  hardware.LCD.clear();
  delay(3000);
}

void DisplaySplashScreen()
{
  hardware.LCD.print(BRAND_ID);
  hardware.LCD.setCursor(0, 1);
  hardware.LCD.print(PRODUCT_ID);
  delay(3000);
  hardware.LCD.clear();
  
  DisplayProfile();
  UpdateDisplayedTemperature();
}

void PrintAt(int line, int column, int maxWidth, boolean rightAlign, char*msg)
{
  int msgLen = strlen(msg);
  if (msgLen > maxWidth)
    msgLen = maxWidth;
  int startCol = column;
  if (rightAlign && (msgLen < maxWidth))
    startCol += (maxWidth - msgLen);

  hardware.LCD.setCursor(column, line);
  for (int i = 0; i < (startCol - column); i++)
    hardware.LCD.print(" ");
  hardware.LCD.print(msg);
  for (int i = startCol + msgLen; i < maxWidth; i++)
    hardware.LCD.print(" ");
}

void WriteAt(int line, int column, int glyph)
{
  hardware.LCD.setCursor(column, line);
  hardware.LCD.write(glyph);
}

void DisplayProfile()
{
  PrintAt(0, 0, 16, false, profiles[currentState.SelectedProfile].Name);
}

void DisplayPhase()
{
  PrintAt(0, 0, 11, false, currentState.PhaseSchedule[currentState.ActivePhase].Name);
}

void DisplayElapsedInPhase(boolean forceUpdate)
{
  int secInPhase = (millis() - currentState.EnteredCurrentPhase) / 1000;
  if (forceUpdate || (secInPhase != currentState.SecInPhase)) {
    currentState.SecInPhase = secInPhase;
    char msg[6];
    sprintf(msg, " %ds", secInPhase);
    PrintAt(0, 11, 5, true, msg);
  }
}

void UpdateDisplayedTemperature()
{
  // if the thermocouple is in a faulted state, then display error
  if (currentState.IsFaulted) {
    PrintAt(1, 0, 16, false, "fault detected");
    return;
  }
  
  // Print current temperature
  char buffer[12];
  dtostrf(currentState.TemperatureC, 3, 1, buffer);
  int sz = strlen(buffer);
  char msg[12];
  sprintf(msg, "%s %s", buffer, "C");
  PrintAt(1, 0, 12, false, msg);
  
  // draw the special glyph for degree symbol
  WriteAt(1, sz, 0);
}


void CheckForButtonPress()
{
  int button = GetRisingEdgeOfButtonPress();
  if (button == CONTROLEO_BUTTON_NONE) return;
  
  if (button == CONTROLEO_BUTTON_TOP) {
    Serial.println("Button Pressed: CONTROLEO_BUTTON_TOP");
  } else {
    Serial.println("Button Pressed: CONTROLEO_BUTTON_BOTTOM");
  }
  
  if (button == CONTROLEO_BUTTON_TOP) {
    // top button was pressed
    
    // if the oven is idle, the top button will cycle through the known profiles
    // but is ignored while the oven is operating
    if (!currentState.IsActive) {
      AdvanceProfile(false);
    }
  }
  
  if (button == CONTROLEO_BUTTON_BOTTOM) {
    // bottom button was pressed
    
    // bottom button starts/stops the oven
    if (currentState.IsActive) {
      End(true);
    } else {
      Start();
    }
  }
}

#define DEBOUNCE_DETECT     50 // minimum time button needs be pressed
#define DEBOUNCE_RESET     500 // minimum time between button signals
int GetRisingEdgeOfButtonPress()
{
  static int heldButton = CONTROLEO_BUTTON_NONE;
  static int lastEvent = CONTROLEO_BUTTON_NONE;
  static unsigned long lastReset = 0;
  static unsigned long heldSince = 0;
  
  unsigned long now = millis();
  int signal = CONTROLEO_BUTTON_NONE;
  int buttonEvent = CONTROLEO_BUTTON_NONE;
  
  // Read the current button status
  signal = CONTROLEO_BUTTON_NONE;
  if (digitalRead(CONTROLEO_BUTTON_TOP_PIN) == LOW)
    signal = CONTROLEO_BUTTON_TOP;
  else if (digitalRead(CONTROLEO_BUTTON_BOTTOM_PIN) == LOW)
    signal = CONTROLEO_BUTTON_BOTTOM;

  if (heldButton == signal && heldButton != CONTROLEO_BUTTON_NONE) {
    // a button is currently pressed, but is it pressed for long enough?
    if ((lastEvent == CONTROLEO_BUTTON_NONE) && (now - heldSince >= DEBOUNCE_DETECT)) {
      // trigger it only once
      buttonEvent = signal;
      lastEvent = signal;
    }
  }
  
  if (heldButton != CONTROLEO_BUTTON_NONE && signal == CONTROLEO_BUTTON_NONE) {
    // button was released
    heldButton = CONTROLEO_BUTTON_NONE;
    lastReset = now;
    heldSince = 0;
    lastEvent = CONTROLEO_BUTTON_NONE;
  }
  
  if (heldButton == CONTROLEO_BUTTON_NONE && signal != CONTROLEO_BUTTON_NONE && (now - lastReset >= DEBOUNCE_RESET)) {
    // new button is being pressed
    heldButton = signal;
    heldSince = now;
  }
  
  return buttonEvent;
}

void EnableBuzzer(uint8_t onOrOff)
{
  analogWrite(CONTROLEO_BUZZER_PIN, onOrOff? 15:0);
  if (onOrOff == ON) {
    Serial.println("*** Alarm ***");
    hardware.DisableBuzzerAt = millis() + BUZZER_DURATION;
  } else {
    hardware.DisableBuzzerAt = 0;
  }
}

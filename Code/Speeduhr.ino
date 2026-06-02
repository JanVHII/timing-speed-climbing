#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "Adafruit_MPR121.h"

#define DEBUG_MODE 0
#if DEBUG_MODE
  #define DEBUG_BEGIN(baud) Serial.begin(baud)
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_BEGIN(baud)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

/* altes Kabel und Pad 0, neues Setup 1*/
#define IRQ_TOUCH_MODE 1

hd44780_I2Cexp lcd;
Adafruit_MPR121 touch, foot;

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

/* Pins MPR121 */
const uint8_t TOUCH_PIN   = 0;  /*Achtung altes Pad 1 und neues Pad 0*/
const uint8_t FOOT_PIN    = 0;
/* Pins Arduino */
const uint8_t PIN_TASTER  = 4;
const uint8_t TOUCH_IRQ   = 3;
const uint8_t FOOT_IRQ    = 2;
const uint8_t BUZZER      = 8;


/* ---------- State Machine ---------- */
enum State {
  READY,
  FOOT_ON_PAD,
  BEEP,
  REACTION,
  RUNNING,
  FALSE_START,
  SHOW_RESULT,
  SHOW_REACTION
};

State state = READY;
State lastState = READY;

/* ---------- Timing ---------- */
const unsigned long DISPLAY_INTERVALL = 100UL;
unsigned long now = 0;
unsigned long startTime = 0;
unsigned long waitTime = 0;
unsigned long beepStart = 0;
unsigned long displayTime = 0;
unsigned long stateTime = 0;
unsigned long reactTime = 0;
unsigned long runTime = 0;
unsigned long maxLoop = 0;

/* ---------- Input ---------- */
uint16_t lastTouch = 0, currTouch = 0;
uint16_t lastFoot  = 0, currFoot  = 0;

bool buttonPressed = false;
bool lastButtonPressed = false;
bool rawButtonPressed = false;
bool lastRawButtonPressed = false;
unsigned long lastButtonChangeTime = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 15;

bool footInstalled = false;
bool startByButton = false;

/* IRQ MPR121 verwenden */
volatile bool mpr_event_touch = false;
void IRQ_mpr_touch() {
  DEBUG_PRINT("IRQ TOUCH at ms = ");
  DEBUG_PRINTLN(now);
  mpr_event_touch = true;
}
volatile bool mpr_event_foot = false;
void IRQ_mpr_foot() {
  DEBUG_PRINT("IRQ FOOT at ms = ");
  DEBUG_PRINTLN(now);
  mpr_event_foot = true;
}

/* ---------- LCD Cache ---------- */
char line0[17] = "";
char line1[17] = "";

/* ---------- Buzzer ---------- */
bool buzzerOn = false;
uint16_t buzzerFreq = 0;

/* ---------- Helper ---------- */
void setBuzzer(bool on, uint16_t freq = 0) {
  if (on) {
    if (!buzzerOn || buzzerFreq != freq) {
      tone(BUZZER, freq);
      buzzerOn = true;
      buzzerFreq = freq;
    }
  } else {
    if (buzzerOn) {
      noTone(BUZZER);
      buzzerOn = false;
      buzzerFreq = 0;
    }
  }
}

void printLine(uint8_t row, const char* text) {
  char buffer[17];
  snprintf(buffer, sizeof(buffer), "%-16s", text);

  char* cache = (row == 0) ? line0 : line1;

  // Unveraendert -> nichts schreiben
  if (strcmp(buffer, cache) == 0) {
    return;
  }

  uint8_t start = 0;
  while (start < 16) {
    // Bis zur ersten Aenderung vorspulen
    while (start < 16 && buffer[start] == cache[start]) {
      start++;
    }

    if (start >= 16) {
      break;
    }

    // Zusammenhaengenden geaenderten Block finden
    uint8_t end = start;
    while (end < 16 && buffer[end] != cache[end]) {
      end++;
    }

    // Nur diesen geaenderten Block schreiben
    lcd.setCursor(start, row);
    for (uint8_t i = start; i < end; i++) {
      lcd.print(buffer[i]);
      cache[i] = buffer[i];
    }

    start = end;
  }

  cache[16] = '\0';
  #if DEBUG_MODE
    DEBUG_PRINT("LCD[");
    DEBUG_PRINT(row);
    DEBUG_PRINT("]: '");
    DEBUG_PRINT(buffer);
    DEBUG_PRINTLN("'");
  #endif
}

void showReadyScreen() {
  printLine(0, "Kann losgehen!");
  printLine(1, "");
}

void formatSeconds(char* buffer, size_t len, unsigned long ms) {
  unsigned long sec = ms / 1000;
  unsigned long hund = (ms % 1000) / 10;   // Hundertstel
  snprintf(buffer, len, "%6lu.%02lu s", sec, hund);
}

void falseStart(unsigned long now) {
  reactTime = 2100UL - (now - beepStart);
  state = FALSE_START;
  stateTime = now;

  char buf[17];
  formatSeconds(buf, sizeof(buf), reactTime);

  printLine(0, "Fruehstart!!");
  printLine(1, buf);

  setBuzzer(true, 1568);
}

/* ---------- Setup ---------- */
void setup() {
  DEBUG_BEGIN(115200);
  DEBUG_PRINTLN("Systemstart");

  pinMode(PIN_TASTER, INPUT_PULLUP);
  pinMode(TOUCH_IRQ, INPUT_PULLUP);
  pinMode(FOOT_IRQ, INPUT_PULLUP);

  lcd.begin(16, 2);
  lcd.backlight();

  printLine(0, "Initialisieren");
  printLine(1, "");

  /* Touchpad muss vorhanden sein */
  if (!touch.begin(0x5A)) {
    printLine(0, "Touchpad fehlt");
    printLine(1, "Code gestoppt");
    while (1) {
      /* stop */
    }
  }

  touch.setAutoconfig(1);
  printLine(0, "Touchpad OK");
  printLine(1, "");

  /* Footpad optional */
  if (foot.begin(0x5B)) {
    foot.setAutoconfig(1);
    footInstalled = true;
    printLine(1, "Footpad OK");
  } else {
    footInstalled = false;
    printLine(1, "Kein Footpad");
  }

  /* IRQ initialisieren, Initialen Status lesen und IRQs quittieren */
  attachInterrupt(digitalPinToInterrupt(FOOT_IRQ), IRQ_mpr_foot, FALLING);
  #if IRQ_TOUCH_MODE
    attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), IRQ_mpr_touch, FALLING);
    currTouch = touch.touched();
    lastTouch = currTouch;
  #endif

  if (footInstalled) {
    currFoot = foot.touched();
    lastFoot = currFoot;
  } else {
    currFoot = 0;
    lastFoot = 0;
  }

  delay(2000);
  showReadyScreen();

  rawButtonPressed = (digitalRead(PIN_TASTER) == LOW);
  lastRawButtonPressed = rawButtonPressed;
  buttonPressed = rawButtonPressed;
  lastButtonPressed = buttonPressed;
  lastButtonChangeTime = millis();

  displayTime = millis();
  stateTime = millis();
}

/* ---------- Loop ---------- */
void loop() {
  now = millis();

  if (state != lastState) {
    DEBUG_PRINT("State change: ");
    DEBUG_PRINT(lastState);
    DEBUG_PRINT(" -> ");
    DEBUG_PRINTLN(state);
    lastState = state;
  }

  /* Inputs lesen */
  #if IRQ_TOUCH_MODE
    if (mpr_event_touch) {
      mpr_event_touch = false;
      currTouch = touch.touched();
    }
  #else
    currTouch = touch.touched();
  #endif
  
  if (footInstalled && mpr_event_foot) {
    mpr_event_foot = false;
    currFoot = foot.touched();
  }
  
 /* Tasterstart/-stop abfgragen, entprellt! */
  rawButtonPressed = (digitalRead(PIN_TASTER) == LOW);

  if (rawButtonPressed != lastRawButtonPressed) {
    lastButtonChangeTime = now;
    lastRawButtonPressed = rawButtonPressed;
  }

  if ((now - lastButtonChangeTime) >= BUTTON_DEBOUNCE_MS) {
    buttonPressed = rawButtonPressed;
  }
  const bool buttonHit = buttonPressed && !lastButtonPressed;

  const bool touchHit =
      (currTouch & _BV(TOUCH_PIN)) && !(lastTouch & _BV(TOUCH_PIN));

  const bool footHit =
      footInstalled &&
      (currFoot & _BV(FOOT_PIN)) && !(lastFoot & _BV(FOOT_PIN));

  const bool footRelease =
      footInstalled &&
      !(currFoot & _BV(FOOT_PIN)) && (lastFoot & _BV(FOOT_PIN));

  /*
   * Fehlstart-Fenster wie vom IFSC Speed Klerren vorgeschrieben:
   * - während state == BEEP
   * - oder wenn state >= REACTION und seit beepStart < 2100 ms
   * Nur sinnvoll bei Footpad-Start, nicht bei Tasterstart.
   */
  const bool inFalseStartWindow =
      footInstalled &&
      !startByButton &&
      (
        (state == BEEP) ||
        ((state >= REACTION) && ((now - beepStart) < 2100UL))
      ) && 
      state != FALSE_START;
  
  if (footRelease && inFalseStartWindow) falseStart(now);

  /* ---------- State Machine ---------- */
  switch (state) {
    case READY:
      if (buttonHit || footHit) {
        state = FOOT_ON_PAD;
        stateTime = now;
        waitTime = now;
        startByButton = buttonHit;

        printLine(0, "Bereit machen!");
        printLine(1, "");
      }
      break;

    case FOOT_ON_PAD:
      if (footRelease) {
        state = READY;
        showReadyScreen();
      } else if ((now - waitTime) > 5000UL) {
        state = BEEP;
        stateTime = now;
        beepStart = now;
        printLine(0, "Countdown!");
        printLine(1, "");
      }
      break;

    case BEEP: {
      unsigned long t = now - beepStart;

      if (t < 200UL) {
        setBuzzer(true, 880);
      } else if (t < 1000UL) {
        setBuzzer(false);
      } else if (t < 1200UL) {
        setBuzzer(true, 880);
      } else if (t < 2000UL) {
        setBuzzer(false);
      } else {
        /*
         * Start der Zeitmessung beim Beginn des letzten Tons,
         * danach sofort REACTION oder RUNNING wie im Original.
         */
        setBuzzer(true, 1760);
        startTime = now;
        displayTime = now;
        state = startByButton ? RUNNING : REACTION;
        stateTime = now;
        DEBUG_PRINT("Last beep started at ms = ");
        DEBUG_PRINTLN(startTime);
      }
      break;
    }

    case REACTION:
      if (footRelease) {
        reactTime = now - startTime;
        state = RUNNING;
        stateTime = now;
        displayTime = now;
      }
      /* Letzten Startton ausschalten */
      if (buzzerOn && (now - beepStart) > 2100UL) {
        setBuzzer(false);
      }      
      break;

    case RUNNING:
      if (touchHit || buttonHit) {
        runTime = now - startTime;

        char buf[17];
        formatSeconds(buf, sizeof(buf), runTime);

        printLine(0, "Gestoppte Zeit:");
        printLine(1, buf);

        state = SHOW_RESULT;
        stateTime = now;

        DEBUG_PRINT("Finish at ms = ");
        DEBUG_PRINTLN(now);
      } else if ((now - displayTime) > DISPLAY_INTERVALL) {
        char buf[17];
        formatSeconds(buf, sizeof(buf), now - startTime);

        printLine(0, "Laufende Zeit:");
        printLine(1, buf);

        displayTime = now;
      }
      /* Letzten Startton ausschalten */
      if (buzzerOn && (now - beepStart) > 2100UL) {
        setBuzzer(false);
      }      
      break;

    case SHOW_RESULT:
      if ((now - stateTime) > 2000UL) {
        if (footInstalled && !startByButton) {
          char buf[17];
          formatSeconds(buf, sizeof(buf), reactTime);

          //printLine(0, "Reaktionszeit:");
          printLine(0, buf);

          state = SHOW_REACTION;
          stateTime = now;
        } else {
          state = READY;
          stateTime = now;
          //showReadyScreen();
        }
      }
      break;

    case SHOW_REACTION:
      if ((now - stateTime) > 2000UL) {
        state = READY;
        stateTime = now;
        delay(10000);
        /* reset foot state */
        currFoot = foot.touched();
        currFoot = 0;
        lastFoot = currFoot;
        //showReadyScreen();
      }
      break;

    case FALSE_START:
      if ((now - stateTime) > 4000UL) {
        setBuzzer(false);
        state = READY;
        stateTime = now;
        showReadyScreen();
      }
      break;
  }

  /* Zustand für nächsten Schleifendurchlauf merken */
  lastTouch = currTouch;
  lastFoot = currFoot;
  lastButtonPressed = buttonPressed;

  #if DEBUG_MODE
    if (millis()-now > maxLoop) {  
      maxLoop = millis()-now;
      DEBUG_PRINT("Loop ms = ");
      DEBUG_PRINTLN(maxLoop);
    }
  #endif
}
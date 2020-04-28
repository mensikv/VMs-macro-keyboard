/*
  ####
  ## VM's keyboard
  ####
  Board: Arduino Micro

  Tested with versions:
    Arduino IDE 1.8.12

  Libraries (and version used to develop the project):
    ClickEncoder        - https://github.com/0xPIT/encoder
    TimerOne     v1.1   - https://github.com/PaulStoffregen/TimerOne
    HID-Project  v2.6.1 - https://github.com/NicoHood/HID

  Base on project from Prusa - Volume Control Knob - https://blog.prusaprinters.org/3d-print-an-oversized-media-control-volume-knob-arduino-basics/

  Macro keyboard with rotary encoder. Offers 8 charry MX buttons and one rotary encoder with click.
  The functions of buttons can be set by defining the arrya key.

  If you are unable to upload the code, then briefly connect RST pin to GND when you see "Uploading" in Arduino IDE
*/

#include <Arduino.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <HID-Project.h>



#define NUMBER_OF_KEYS         8    // Count of keys in the keyboard
#define MAX_COMBINATION_KEYS   4    // Maximum number of key codes that can be pressed at the same time (does dont correspond to actually pressed keys)
#define MAX_SEQUENCE_KEYS     16    // Maximum length of key combination sequence (that means first you send CTRL + Z (1. combination), then SHIFT + ALT + X (2. combination), then A (3. combination) ... )

#define DEBOUNCING_MS         20    // wait in ms when key can oscilate
#define FIRST_REPEAT_CODE_MS 500    // after FIRST_REPEAT_CODE_MS ,s if key is still pressed, start sending the command again
#define REPEAT_CODE_MS       150    // when sending command by holding down key, wait this long before sending command egain

// Rotary encoder connections
#define ENCODER_CLK           A0
#define ENCODER_DT            A1
#define ENCODER_SW            A2



// Defining types
enum TKeyState {INACTIVE, DEBOUNCING, ACTIVE, HOLDING};         // Key states - INACTIVE -> DEBOUNCING -> ACTIVE -> HOLDING -> INACTIVE
//                                                                                                               -> INACTIVE
enum TKeyType {KEYBOARD, MOUSE, CONSUMER, SYSTEM, MODIFIER};    // Types of key codes - simulating keyboard, mouse, multimedia or modifier that alters the rotary encoder behavior

typedef struct  TActions {
  uint16_t durationMs;
  uint16_t key[MAX_COMBINATION_KEYS];
} TAction;

typedef struct TKeys {
  uint8_t pin;
  enum TKeyType type;
  enum TKeyState state;
  uint32_t stateStartMs;
  uint16_t modificatorKeys[MAX_COMBINATION_KEYS];
  TAction action[MAX_SEQUENCE_KEYS];
} TKey;
// Different types expect different actions:
//   KEYBOARD - keys + modifiers, for example: {.pin =  8, .type = KEYBOARD, .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {KEY_LEFT_SHIFT}, .action = {{.durationMs = 100, .key = {KEY_H}}, {.durationMs = 100, .key = {KEY_I}}}}
//   CONSUMER - only keys,        for example: {.pin =  8, .type = KEYBOARD, .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {}              , .action = {{.durationMs = 100, .key = {MEDIA_VOLUME_MUTE, CONSUMER_BRIGHTNESS_UP}}, {.durationMs = 100, .key = {CONSUMER_CALCULATOR}}}}
//   SYSTEM   - only 1 key,       for example: {.pin =  8, .type = KEYBOARD, .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {}              , .action = {{.durationMs = 100, .key = {HID_SYSTEM_SLEEPP}}}}
//   MODIFIER - nothing           for example: {.pin =  8, .type = KEYBOARD, .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {}              , .action = {}}
//   MOUSE    - only keys         for example: {.pin =  8, .type = KEYBOARD, .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {}              , .action = {{.durationMs = 10100, .key = {10100, 10000, 9800}}, {.durationMs = 200, .key = {MOUSE_LEFT, MOUSE_MIDDLE}}}} = first move 100px in X-direction and scroll -200px and wait 100ms, then click left and middle mouse button and wait 200ms
//                if durationMs >= 10000; then in key is tripplet- delta movement in X, Y and SCROLL (zero is mapped to 10000, so 9800 is -200px) - {{.durationMs = 10200, .key = {10000, 10000, 10300}} - scroll 300px and wait 200ms
//                if durationMs < 10000; then in key are mouse keys to press  - {{.durationMs = 150, .key = {MOUSE_LEFT, MOUSE_MIDDLE}} - press left and middle mouse buttons for 150ms

// Supported key commands:
//  KEYBOARD - c:\Users\XXX\Documents\Arduino\libraries\HID-Project\src\KeyboardLayouts\ImprovedKeylayouts.h - see section enum KeyboardKeycode : uint8_t
//  CONSUMER - c:\Users\XXX\Documents\Arduino\libraries\HID-Project\src\HID-APIs\ConsumerAPI.h               - see section enum ConsumerKeycode : uint16_t
//  SYSTEM   - c:\Users\XXX\Documents\Arduino\libraries\HID-Project\src\HID-APIs\SystemAPI.h                 - see section enum SystemKeycode : uint8_t
//  MOUSE    - c:\Users\XXX\Documents\Arduino\libraries\HID-Project\src\HID-APIs\MouseAPI.h                  - see defines - MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE

// Define actions for your keys
TKey key[NUMBER_OF_KEYS] = {
  {.pin =  6, .type = KEYBOARD, .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {}, .action = {{.durationMs = 50, .key = {KEY_H}}, {.durationMs = 50, .key = {KEY_E}}, {.durationMs = 50, .key = {KEY_L}}, {.durationMs = 50, .key = {KEY_L}}, {.durationMs = 50, .key = {KEY_O}}}},
  {.pin =  7, .type = KEYBOARD, .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {KEY_LEFT_SHIFT}, .action = {{.durationMs = 50, .key = {KEY_H}}, {.durationMs = 50, .key = {KEY_E}}, {.durationMs = 50, .key = {KEY_L}}, {.durationMs = 50, .key = {KEY_L}}, {.durationMs = 50, .key = {KEY_O}}}},
  {.pin =  8, .type = KEYBOARD, .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {}, .action = {{.durationMs = 50, .key = {KEY_LEFT_SHIFT, KEY_H}}, {.durationMs = 50, .key = {KEY_E}}, {.durationMs = 50, .key = {KEY_L}}, {.durationMs = 50, .key = {KEY_L}}, {.durationMs = 50, .key = {KEY_O}}}},
  {.pin =  9, .type = KEYBOARD, .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {}, .action = {{.durationMs = 0, .key = {KEY_LEFT_CTRL, KEY_C}}}},
  {.pin = 15, .type = MODIFIER, .state = INACTIVE, .stateStartMs = 0},
  {.pin = 14, .type = CONSUMER, .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {}, .action = {{.durationMs = 50, .key = {CONSUMER_CALCULATOR}}}},
  {.pin = 16, .type = SYSTEM,   .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {}, .action = {{.durationMs = 50, .key = {HID_SYSTEM_HIBERNATE}}}},
  {.pin = 10, .type = MOUSE,   .state = INACTIVE, .stateStartMs = 0, .modificatorKeys = {}, .action = {{.durationMs = 10050, .key = {10050, 10000, 10004}}, {.durationMs = 50, .key = {MOUSE_RIGHT}}}},
};

// Actions for rotary encoder are later in the code (search for ROTARY_ACTIONS)


// global variables
ClickEncoder *encoder;     // variable representing the rotary encoder
int16_t last, value;       // variables for current and last rotation value
uint8_t globalModifier;    // when holding down key with MODIFIER type, this is set to true - can be used to change the behaviour of other keys or rotary encoder


// Capture rotary encoder pulses
void timerIsr() {
  encoder->service();
}

// Execute key commands
uint8_t processKey(uint8_t keyIndex) {
  TKey *lkey = &key[keyIndex];
  if (lkey->type == KEYBOARD) {
    // Press modificators
    for (uint8_t i = 0; i < MAX_COMBINATION_KEYS; i++) {
      if (lkey->modificatorKeys[i]) Keyboard.press((KeyboardKeycode) lkey->modificatorKeys[i]);
      else break;
    }
    for (uint8_t i = 0; i < MAX_SEQUENCE_KEYS; i++) {
      TAction *laction = &lkey->action[i];
      if ((laction->durationMs) || (laction->key[0])) {
        //press keys
        for (uint8_t j = 0; j < MAX_COMBINATION_KEYS; j++) {
          if (laction->key[j]) Keyboard.press((KeyboardKeycode) laction->key[j]);
          else break;
        }
        // wait
        if (laction->durationMs) delay(laction->durationMs);
        //release keys
        for (uint8_t j = 0; j < MAX_COMBINATION_KEYS; j++) {
          if (laction->key[j]) Keyboard.release((KeyboardKeycode) laction->key[j]);
          else break;
        }
      } else {
        break;
      }
    }
    Keyboard.releaseAll();
  } else if (lkey->type == CONSUMER) {
    for (uint8_t i = 0; i < MAX_SEQUENCE_KEYS; i++) {
      TAction *laction = &lkey->action[i];
      if ((laction->durationMs) || (laction->key[0])) {
        //press keys
        for (uint8_t j = 0; j < MAX_COMBINATION_KEYS; j++) {
          if (laction->key[j]) Consumer.press((ConsumerKeycode) laction->key[j]);
          else break;
        }
        // wait
        if (laction->durationMs) delay(laction->durationMs);
        //release keys
        for (uint8_t j = 0; j < MAX_COMBINATION_KEYS; j++) {
          if (laction->key[j]) Consumer.release((ConsumerKeycode) laction->key[j]);
          else break;
        }
      } else {
        break;
      }
    }
    Consumer.releaseAll();
  } else if (lkey->type == SYSTEM) {
    if (lkey->action[0].key[0]) {
      System.write((SystemKeycode) lkey->action[0].key[0]);
    }
    } else if (lkey->type == MOUSE) {
    for (uint8_t i = 0; i < MAX_SEQUENCE_KEYS; i++) {
      // MOUSE - no modifiers,
      // if durationMs >= 10000; then in key is tripplet- delta movement in X, Y and SCROLL (zero is mapped to 10000, so 9800 is -200px) - {{.durationMs = 10200, .key = {10000, 10000, 10300}} - scroll 300px and wait 200ms
      // if durationMs < 10000; then in key are mouse keys to press  - {{.durationMs = 150, .key = {MOUSE_LEFT, MOUSE_MIDDLE}} - press left and middle mouse buttons for 150ms
      TAction *laction = &lkey->action[i];
      if ((laction->durationMs) || (laction->key[0])) {
        if (laction->durationMs < 10000) { // button's clicks
          //press keys
          for (uint8_t j = 0; j < MAX_COMBINATION_KEYS; j++) {
            if (laction->key[j]) Mouse.press((uint8_t) laction->key[j]);
            else break;
          }
          // wait
          if (laction->durationMs) delay(laction->durationMs);
          //release keys
          for (uint8_t j = 0; j < MAX_COMBINATION_KEYS; j++) {
            if (laction->key[j]) Mouse.release((uint8_t) laction->key[j]);
            else break;
          }
        } else {  // cursor movement and scrolling
          int8_t x = laction->key[0] - 10000;
          int8_t y = laction->key[1] - 10000;
          int8_t wheel = laction->key[2] - 10000;
          Mouse.move(x, y, wheel);
          if (laction->durationMs) delay(laction->durationMs - 10000);
        }
      } else {
        break;
      }
    }
    Mouse.releaseAll();
  } else if (lkey->type == MODIFIER) {
    globalModifier = true;
  } else {
    //Serial.println(F("Error - unsupported keyboard type"));
    ;
  }
}

/*
// Serial communication is not started, you must first uncomment it in the setup function
// Print out all keys and their associated commands - good for debugging
void dumpAllKeys () {
  for (uint8_t i = 0; i < NUMBER_OF_KEYS; i++ ) dumpKey(i);
}

// Print out a key's associated commands - good for debugging
void dumpKey (uint8_t keyIndex) {
  TKey *lkey  = &key[keyIndex];
  char msg[100];
  msg[0] = '\0';
  char type[15];
  type[0] = '\0';
  switch (lkey->type) {
    case KEYBOARD: strcpy(type, "KEYBOARD"); break;
    case MOUSE:    strcpy(type, "MOUSE"); break;
    case CONSUMER: strcpy(type, "CONSUMER"); break;
    case SYSTEM:   strcpy(type, "SYSTEM"); break;
    case MODIFIER: strcpy(type, "MODIFIER"); break;
  }
  char state[15];
  state[0] = '\0';
  switch (lkey->state) {
    case INACTIVE:   strcpy(state, "INACTIVE"); break;
    case DEBOUNCING: strcpy(state, "DEBOUNCING"); break;
    case ACTIVE:     strcpy(state, "ACTIVE"); break;
    case HOLDING:    strcpy(state, "HOLDING"); break;
  }
  sprintf(msg, "Key: %i; Pin: %i; Type: %s; State: %s; StartMs: %li\n", keyIndex, lkey->pin, type, state, lkey->stateStartMs);
  Serial.print(msg);
  Serial.print(F("  Modificator keys: "));
  for (uint8_t i = 0; i < MAX_COMBINATION_KEYS; i++) {
    sprintf(msg, "%i, ", lkey->modificatorKeys[i]);
    Serial.print(msg);
  }
  Serial.println();

  for (uint8_t i = 0; i < MAX_SEQUENCE_KEYS; i++) {
    sprintf(msg, "  %i. cmd: DurationMs: %i; Keys: ", i, lkey->action[i].durationMs);
    Serial.print(msg);
    for (uint8_t j = 0; j < MAX_COMBINATION_KEYS; j++) {
      sprintf(msg, "%i, ", lkey->action[i].key[j]);
      Serial.print(msg);
    }
    Serial.println();
  }
  Serial.println();
}
*/

void setup() {
  //Uncomment if you want some debug messages
  //Serial.begin(115200); // Opens the serial connection used for communication with the PC.
  //while (!Serial) delay(1);
  //Serial.println(F("VM's keyboard v0.1"));

  //dumpAllKeys();  // must enable Serial

  Consumer.begin();     // Initializes the media keyboard
  Keyboard.begin();
  Mouse.begin();
  System.begin();

  encoder = new ClickEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW); // Initializes the rotary encoder with the mentioned pins

  for (uint8_t i = 0; i < NUMBER_OF_KEYS; i++) pinMode(key[i].pin, INPUT_PULLUP);

  Timer1.initialize(1000);                                         // Initializes the timer, which the rotary encoder uses to detect rotation - 1000us = 1ms
  Timer1.attachInterrupt(timerIsr);

  last = -1;
  globalModifier = false;
}

void loop() {
  // read the key's states and if one is pressed, execute the associated command
  for (uint8_t i = 0; i < NUMBER_OF_KEYS; i++) {
    uint8_t keyState = digitalRead(key[i].pin);
    if ((key[i].state == INACTIVE) && (keyState == LOW)) {
      key[i].state = DEBOUNCING;
      key[i].stateStartMs = millis();
    } else if (key[i].state == DEBOUNCING) {
      if (keyState == HIGH) key[i].stateStartMs = millis();
      else if ((millis() - key[i].stateStartMs) > DEBOUNCING_MS) {
        key[i].state = ACTIVE;
        processKey(i);
      }
    } else if (key[i].state == ACTIVE) {
      if (keyState == HIGH) {
        key[i].state = INACTIVE;
        key[i].stateStartMs = millis();
        if (key[i].type == MODIFIER) {
          globalModifier = false;
        }
      } else if ((millis() - key[i].stateStartMs) > FIRST_REPEAT_CODE_MS) {
        key[i].state = HOLDING;
        key[i].stateStartMs = millis();
        processKey(i);
      }
    } else if (key[i].state == HOLDING) {
      if (keyState == HIGH) {
        key[i].state = INACTIVE;
        key[i].stateStartMs = millis();
        if (key[i].type == MODIFIER) {
          globalModifier = false;
        }
      } else if ((millis() - key[i].stateStartMs) > REPEAT_CODE_MS) {
        key[i].stateStartMs = millis();
        processKey(i);
      }
    }
  }

  // ROTARY_ACTIONS
  // Turning the rotary encoder
  value += encoder->getValue();

  // This part of the code is responsible for the actions when you rotate the encoder
  if (value != last) {                          // New value is different than the last one, that means to encoder was rotated
    uint16_t diff = abs(value - last);
    if (globalModifier) {                       // The rotary encoder is used as volume control or with the Modifier key as a scroll
      signed char wheel = (last < value) ? 4 : -4;
      for (uint8_t i = 0; i < diff; i++) Mouse.move(0, 0, wheel);
    } else {
      ConsumerKeycode cmd = (last < value) ? MEDIA_VOLUME_UP : MEDIA_VOLUME_DOWN;
      for (uint8_t i = 0; i < diff; i++) Consumer.write(cmd);
    }
    last = value;                                 // Refreshing the "last" varible for the next loop with the current value
  }

  // Pressing the rotary encoder - detects single and double clicks
  // This next part handles the rotary encoder BUTTON
  ClickEncoder::Button b = encoder->getButton(); // Asking the button for it's current state
  if (b != ClickEncoder::Open) {                 // If the button is unpressed, we'll skip to the end of this if block
    switch (b) {
      case ClickEncoder::Clicked:                // Button was clicked once
        Consumer.write(MEDIA_PLAY_PAUSE);        // Replace this line to have a different function when clicking button once
        break;
      case ClickEncoder::DoubleClicked:          // Button was double clicked
        Consumer.write(MEDIA_VOLUME_MUTE);       // Replace this line to have a different function when double-clicking
        break;
    }
  }
  //delay(10);                                    // I think this is not needed
}

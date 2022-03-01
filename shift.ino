#include "DebouncedInput.h"

#define PIN_UIPIXEL       9
#define PIN_BTN_THRU      10
#define PIN_BTN_CHANNEL   11
#define PIN_BTN_REC       12

using ThruBtn = DebouncedInput<PIN_BTN_THRU, 0>;
using ChnlBtn = DebouncedInput<PIN_BTN_CHANNEL, 1>;
using PlayBtn = DebouncedInput<PIN_BTN_REC, 2>;

ThruBtn thruButton;
ChnlBtn channelButton;
PlayBtn recButton;


void uiAction(uint8_t action, uint8_t mode) {
  Serial.print(action);
  Serial.print("/");
  Serial.print(mode);
}

void updateUI() {
  uint16_t nowMs = millis();

  uint8_t oldDown = thruButton.downFlag() | channelButton.downFlag() | recButton.downFlag();
  uint8_t oldShift = thruButton.shiftFlag() | channelButton.shiftFlag() | recButton.shiftFlag();

  uint8_t changes = 0;
  if (thruButton.update(nowMs))
    changes |= ThruBtn::Flag;
  if (channelButton.update(nowMs))
    changes |= ChnlBtn::Flag;
  if (recButton.update(nowMs))
    changes |= PlayBtn::Flag;

  uint8_t pressed = ~oldDown & changes;
  uint8_t newDown = oldDown ^ changes;

  uint8_t released = thruButton.releasedFlag() | channelButton.releasedFlag() | recButton.releasedFlag();

  if (changes) {
    Serial.print("oD=");
    Serial.print(oldDown, HEX);
    Serial.print(", oS=");
    Serial.print(oldShift, HEX);
    Serial.print(", chg=");
    Serial.print(changes, HEX);
    Serial.print(", nD=");
    Serial.print(newDown, HEX);
    Serial.print(", press=");
    Serial.print(pressed, HEX);
    Serial.print(", rel=");
    Serial.print(released, HEX);
    
    // first: figure out if any buttons have just become shift
    if (pressed) {
      uint8_t addedShift = oldDown & ~released & ~oldShift;
      if (addedShift) {
        Serial.print("    NEW SHIFT=");
        Serial.print(addedShift, HEX);

        if (addedShift & ThruBtn::Flag)
          thruButton.setShift();
        if (addedShift & ChnlBtn::Flag)
          channelButton.setShift();
        if (addedShift & PlayBtn::Flag)
          recButton.setShift();
      }
    }
    
    uint8_t mode = thruButton.shiftFlag() | channelButton.shiftFlag() | recButton.shiftFlag();    

    // then figure out what mode/button combo just got released
    if (released) {
      Serial.print("    ACTION: ");
      if (released & thruButton.getFlag())
        uiAction(thruButton.getId(), mode);
      else if (released & channelButton.getFlag())
        uiAction(channelButton.getId(), mode);
      else if (released & recButton.getFlag())
        uiAction(recButton.getId(), mode);
    }
    
    Serial.println();
  }

}



void setup() {
  Serial.begin(115200);
  Serial.println("knock knock, neo");
  
  pinMode(13, OUTPUT);

  channelButton.init();
  thruButton.init();
  recButton.init();
}


void loop() {
  random(100);
  
  updateUI();
}

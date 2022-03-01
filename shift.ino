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

enum UIMode {
  UIMode_Default = 0,
  
  UIMode_Thru = ThruBtn::Flag,
  UIMode_Chnl = ChnlBtn::Flag,
  UIMode_Play = PlayBtn::Flag,
  
  UIMode_ThruChnl = ThruBtn::Flag | ChnlBtn::Flag,
  UIMode_ThruPlay = ThruBtn::Flag | PlayBtn::Flag,
  UIMode_ChnlPlay = ChnlBtn::Flag | PlayBtn::Flag,
};


void uiAction(uint8_t action, uint8_t mode) {
  const char* modeName = "UNKNOWN";
  switch (mode) {
    case UIMode_Default:  modeName = "d"; break;     
    case UIMode_Thru:     modeName = "t"; break;     
    case UIMode_Chnl:     modeName = "c"; break;     
    case UIMode_Play:     modeName = "p"; break;     
    case UIMode_ThruChnl: modeName = "tc"; break;     
    case UIMode_ThruPlay: modeName = "tp"; break;     
    case UIMode_ChnlPlay: modeName = "cp"; break;      
  }
  Serial.print("ACTION: ");
  Serial.print(action);
  Serial.print("/");
  Serial.println(modeName);
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

  uint8_t released = thruButton.releasedFlag() | channelButton.releasedFlag() | recButton.releasedFlag();

  if (changes) {
    // first: figure out if any buttons have just become shift
    if (pressed) {
      uint8_t addedShift = oldDown & ~released & ~oldShift;
      if (addedShift) {
        if (addedShift & ThruBtn::Flag)
          thruButton.setShift();
        if (addedShift & ChnlBtn::Flag)
          channelButton.setShift();
        if (addedShift & PlayBtn::Flag)
          recButton.setShift();
      }
    }
    
    uint8_t mode = thruButton.shiftFlag() | channelButton.shiftFlag() | recButton.shiftFlag();
    uint8_t action = released & ~(mode | oldShift);

    // FIXME: the following sequence results in a surprising action:
    //    - hold A
    //    - hold B  (A is now SHIFT)
    //    - release A
    //    - release B   --> ACTION B/default    

    // then figure out what mode/button combo just got released
    if (action) {
      if (action & thruButton.getFlag())
        uiAction(thruButton.getId(), mode);
      else if (action & channelButton.getFlag())
        uiAction(channelButton.getId(), mode);
      else if (action & recButton.getFlag())
        uiAction(recButton.getId(), mode);
    }
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

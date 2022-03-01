#include "DebouncedInput.h"

class ModalPot {
public:
  static constexpr uint8_t Pin = 0;
  static constexpr uint8_t MaxModes = 8;
  static constexpr int     Threshold = 10;

  ModalPot() {
    m_vals[0] = analogRead(Pin);
  }

  int getVal(uint8_t mode) const { return m_vals[mode]; }
  int getLastMode() const { return m_lastMode; }

  bool hasMoved() const {
    int currVal = analogRead(Pin);
    int prevVal = m_vals[m_lastMode];
    int delta = abs(currVal - prevVal);
    return (delta > Threshold);
  }

  void update(uint8_t mode) {
    int currVal = analogRead(Pin);

    if (mode == m_lastMode) {
      m_vals[mode] = currVal;
      return;
    }

    int prevVal = m_vals[m_lastMode];
    int delta = abs(currVal - prevVal);
    if (delta < Threshold) {
      return;
    }

    m_vals[mode] = currVal;
    m_lastMode = mode;
  }
  

private:
  int m_vals[MaxModes] = {};
  uint8_t m_lastMode = 0;
};


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
ModalPot pot;

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

  uint8_t unshiftedButtons = oldDown & ~oldShift;
  if (pot.hasMoved() && unshiftedButtons) {
    changes |= 0x10;
    pressed |= 0x10;
  }
  
  uint8_t mode = oldShift;

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
    
    mode = thruButton.shiftFlag() | channelButton.shiftFlag() | recButton.shiftFlag();
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

  // note: if we have any buttons down but haven't yet marked them as shifted,
  //       we don't want to update the existing pot, or we might miss the edge
  //       where it should latch and shift the button
  if (!unshiftedButtons)
    pot.update(mode);
}



void setup() {
  Serial.begin(115200);
  Serial.println("knock knock, neo");
  
  pinMode(13, OUTPUT);
  digitalWrite(13, 0);

  channelButton.init();
  thruButton.init();
  recButton.init();

  pot.update(0);
}


uint32_t lastUpdate = millis();

void loop() {  
  updateUI();
  
  if ((millis() - lastUpdate) > 1000) {
    Serial.print("POTS: ");
    for(byte i=0; i<8; ++i) {
      Serial.print(pot.getVal(i));
      Serial.print("  ");
    }
    Serial.print("    lastMode=");
    Serial.println(pot.getLastMode());
    lastUpdate = millis();
  }
}

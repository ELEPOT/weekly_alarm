#include <Arduino.h>
#include <LedControl.h>

auto seg7 = LedControl(12, 11, 10, 1);

static const byte digitMap[] = {
    0b01111110,0b00110000,0b01101101,0b01111001,0b00110011,0b01011011,0b01011111,0b01110010,0b01111111,0b01111011
};

void setSeg7Digit(const int index, const int value) {
    // setCol for common anode 7-segments
    seg7.setRow(0, index, digitMap[value]);
}

void setup() {
    seg7.shutdown(0, false);
    seg7.setIntensity(0, 8);
    seg7.clearDisplay(0);

    setSeg7Digit(0, 1);
    // setSeg7Digit(1, 0);
}

int i = 0;

void loop() {
    // setSeg7Digit(0, i / 10);
    // setSeg7Digit(1, i % 10);
    //
    // i ++;
    // i = i % 100;
    //
    // delay(500);
}
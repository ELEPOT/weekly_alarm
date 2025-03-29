#include "run_config.h"

#ifdef RUN_TEST_INPUT

#include <Arduino.h>

#define CENTER_BTN 5
#define UP_BTN 6
#define DOWN_BTN 7
#define LEFT_BIN 8
#define RIGHT_BTN 9

const uint8_t inputPins[5] = {CENTER_BTN, UP_BTN, DOWN_BTN, LEFT_BIN, RIGHT_BTN};

void setup() {
    for (const auto pin: inputPins) {
        pinMode(pin, INPUT);
    }

    Serial.begin(9600);
}

void loop() {
    for (const auto pin: inputPins) {
        Serial.print(digitalRead(pin));
        Serial.print(" ");
    }
    Serial.println("");
}

#endif
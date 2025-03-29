#include "run_config.h"

#ifdef RUN_TEST_CLOCK

#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include <Adafruit_LEDBackpack.h>
auto matrix = Adafruit_7segment();

#include <FastLED.h>
#define LED_N 8
#define LED_PIN 4
CRGB leds[LED_N];

#include <RTClib.h>
RTC_DS3231 rtc;

bool isColonLit;

ISR(WDT_vect) {}; // Handle Watch dog Interrupt

void setup() {
    matrix.begin(0x70);

    CFastLED::addLeds<WS2812, LED_PIN, GRB>(leds, LED_N);
    FastLED.setBrightness(10);

    rtc.begin();
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        fill_gradient(leds, LED_N, CHSV(0, 255, 255), CHSV(255, 255, 255), LONGEST_HUES);
        FastLED.show();
        delay(1000);
        FastLED.clear();
    }

    // Disable ADC
    ADCSRA = 0;
}

void loop() {
    const auto now = rtc.now();

    const auto timestamp = now.timestamp(DateTime::TIMESTAMP_TIME); //hh:mm:ss

    matrix.writeDigitAscii(0, timestamp[0]);
    matrix.writeDigitAscii(1, timestamp[1]);
    matrix.writeDigitAscii(3, timestamp[3]);
    matrix.writeDigitAscii(4, timestamp[4]);

    isColonLit = !isColonLit;
    matrix.drawColon(isColonLit);

    matrix.writeDisplay();

    wdt_enable(WDTO_1S);
    // Enable the WD interrupt (note no reset)
    WDTCSR |= _BV(WDIE);

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    noInterrupts();
    sleep_enable();
    interrupts();
    sleep_cpu();

    // will be woken up by watchdog and continue next loop
}

#endif

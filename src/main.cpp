#include "run_config.h"

#ifdef RUN_MAIN

#include <Arduino.h>


/* Display */
#include <Adafruit_LEDBackpack.h>
auto seg7 = Adafruit_7segment();

#include <FastLED.h>
#define WS_LED_NUM 8
#define WS_LED_PIN 9
CRGB ws_leds[WS_LED_NUM];

constexpr CRGB week_colors[7] = {CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Cyan, CRGB::Purple, CRGB::White};

/* Timer */
#include <RTClib.h>
RTC_DS3231 rtc;

/* Sound */
#include <DFPlayerMini_Fast.h>
DFPlayerMini_Fast df;

#include <SoftwareSerial.h>
SoftwareSerial df_serial(10, 11);

/* Input */
#define BTN_MIDDLE 0
#define BTN_UP 1
#define BTN_DOWN 2
#define BTN_LEFT 3
#define BTN_RIGHT 4

#define MILLIS_HOLDING_BEFORE_AUTOPRESS 1000
#define MILLIS_BETWEEN_AUTOPRESS 100

/* Power */
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

#define DFPLAYER_PWR_PIN 13
#define DISPLAY_PWR_PIN 12
#define INTERRUPT_BTN_PIN 3

#define MILLIS_IDLING_BEFORE_SLEEPING 5000

bool waken_up_by_btn = false;

ISR(WDT_vect) { // Handle watch dog interrupt
    sleep_disable();
};

void btn_pin_interrupt() {
    waken_up_by_btn = true;
    sleep_disable();
}

/* System */
enum class State { Clock, DayAlarm, DayAlarm_SetHour, DayAlarm_SetMinute };

#define switch_to_state(state) curr_state = state; state_changed = true; break

class DayAlarm {
public:
    DayAlarm(const int hour, const int minute, const bool enabled = false)
        :   enabled(enabled),
            hour(hour),
            minute(minute) {
    }

    bool enabled;
    int hour;
    int minute;
};

auto day_alarm = DayAlarm(8, 0);
auto curr_state = State::Clock;
bool alarm_activated_this_minute = false; // TODO: need better way to handle

void setup() {
    /* Timer */
    rtc.begin();

    /* Sound */


    /* Input */
    pinMode(2, INPUT);
    pinMode(INTERRUPT_BTN_PIN, INPUT);

    pinMode(4, INPUT);
    pinMode(5, INPUT);
    pinMode(6, INPUT);
    pinMode(7, INPUT);
    pinMode(8, INPUT);

    /* Power */

    pinMode(DFPLAYER_PWR_PIN, OUTPUT);
    pinMode(DISPLAY_PWR_PIN, OUTPUT);

    digitalWrite(DFPLAYER_PWR_PIN, LOW);
    digitalWrite(DISPLAY_PWR_PIN, LOW);

    attachInterrupt(digitalPinToInterrupt(INTERRUPT_BTN_PIN), btn_pin_interrupt, RISING);

    ADCSRA = 0; // Disable ADC, which won't be used anywhere
}

void check_alarm() {
    const auto now = rtc.now();
    const auto hour = now.hour();
    const auto minute = now.minute();

    if (day_alarm.hour == hour && day_alarm.minute == minute) {
        if (curr_state != State::DayAlarm_SetHour && curr_state != State::DayAlarm_SetMinute && !alarm_activated_this_minute) {
            digitalWrite(DFPLAYER_PWR_PIN, HIGH);

            df_serial.begin(9600);
            df.begin(df_serial, true);

            df.volume(20);
            df.repeatFolder(1);
            while (!digitalRead(4)) {}

            df.stop();

            digitalWrite(DFPLAYER_PWR_PIN, LOW);

            alarm_activated_this_minute = false;
        }
    }

    else if (alarm_activated_this_minute) {
        alarm_activated_this_minute = false;
    }
}

void enter_awake_loop() {
    /* Enable Display Module */
    delay(100);

    digitalWrite(DISPLAY_PWR_PIN, HIGH);

    seg7.begin(0x70);

    CFastLED::addLeds<WS2812, WS_LED_PIN, GRB>(ws_leds, WS_LED_NUM);
    FastLED.setBrightness(10);

    /* Main Loop */
    curr_state = State::Clock;

    unsigned long curr_millis = millis();
    unsigned long last_autopress_millis = 0;
    unsigned long last_pressed_millis = curr_millis;
    unsigned long clock_1s_millis = 0;
    unsigned long clock_500ms_millis = 0;

    // State will be toggled every tick. For blinking LED.
    bool clock_1s_ticked_on = false;
    bool clock_500ms_ticked_on = false;

    byte prev_btn_state = 0b00000;

    while (curr_state != State::Clock || curr_millis - last_pressed_millis < MILLIS_IDLING_BEFORE_SLEEPING) {
        check_alarm();

        bool state_changed = false;

        /* Clock */
        bool clock_1s_ticked = false;
        if (curr_millis - clock_1s_millis >= 1000) {
            clock_1s_millis = curr_millis;
            clock_1s_ticked = true;
            clock_1s_ticked_on = !clock_1s_ticked_on;
        }

        bool clock_500ms_ticked = false;
        if (curr_millis - clock_500ms_millis >= 500) {
            clock_500ms_millis = curr_millis;
            clock_500ms_ticked = true;
            clock_500ms_ticked_on = !clock_500ms_ticked_on;
        }

        /* Check Input */
        const byte curr_btn_state =
            digitalRead(4) << BTN_MIDDLE |
            digitalRead(5) << BTN_UP |
            digitalRead(6) << BTN_DOWN |
            digitalRead(7) << BTN_LEFT |
            digitalRead(8) << BTN_RIGHT;

        const byte btn_pressed = curr_btn_state & ~prev_btn_state;

        if (btn_pressed) {
            last_pressed_millis = curr_millis;
        }

        // Hack to autopress when holding button
        byte btn_pressed_auto = btn_pressed;
        if (curr_btn_state && curr_millis - last_pressed_millis > MILLIS_HOLDING_BEFORE_AUTOPRESS && curr_millis - last_autopress_millis > MILLIS_BETWEEN_AUTOPRESS) {
            btn_pressed_auto |= curr_btn_state;
            last_autopress_millis = curr_millis;
        }

        prev_btn_state = curr_btn_state;

        /* Handle States According to Input */
        if (btn_pressed & bit(BTN_MIDDLE)) {
            switch (curr_state) {
                case State::DayAlarm:
                    switch_to_state(State::DayAlarm_SetHour);
                case State::DayAlarm_SetHour:
                    switch_to_state(State::DayAlarm_SetMinute);
                case State::DayAlarm_SetMinute:
                    switch_to_state(State::DayAlarm);
                default:
                    break;
            }
        }

        else if (btn_pressed & bit(BTN_RIGHT)) {
            switch (curr_state) {
                case State::Clock:
                    switch_to_state(State::DayAlarm);
                case State::DayAlarm_SetHour:
                    switch_to_state(State::DayAlarm_SetMinute);
                default:
                    break;
            }
        }

        else if (btn_pressed & bit(BTN_LEFT)) {
            switch (curr_state) {
                case State::DayAlarm:
                    switch_to_state(State::Clock);
                case State::DayAlarm_SetMinute:
                    switch_to_state(State::DayAlarm_SetHour);
                default:
                    break;
            }
        }

        if (btn_pressed_auto & bit(BTN_UP)) {
            switch (curr_state) {
                case State::DayAlarm_SetHour:
                    day_alarm.hour++;
                    day_alarm.hour %= 24;
                    state_changed = true;
                    break;
                case State::DayAlarm_SetMinute:
                    day_alarm.minute++;
                    day_alarm.minute %= 60;
                    state_changed = true;
                    break;
                default:
                    break;
            }
        }

        else if (btn_pressed_auto & bit(BTN_DOWN)) {
            switch (curr_state) {
                case State::DayAlarm_SetHour:
                    day_alarm.hour--;
                    day_alarm.hour += 24; // Modulo breaks with negative numbers. This is to prevent that.
                    day_alarm.hour %= 24;
                    state_changed = true;
                    break;
                case State::DayAlarm_SetMinute:
                    day_alarm.minute--;
                    day_alarm.minute += 60;
                    day_alarm.minute %= 60;
                    state_changed = true;
                    break;
                default:
                    break;
            }
        }

        /* Display */
        switch (curr_state) {
            case State::Clock:
                if (state_changed) {
                    clock_1s_ticked_on = false;
                    clock_1s_millis = 0;
                }
                if (clock_1s_ticked) {
                    const auto now = rtc.now();
                    const auto hour = now.hour();
                    const auto minute = now.minute();
                    const auto day = now.dayOfTheWeek(); // 1 is Monday

                    for (int i = 0; i < 7; i ++) {
                        ws_leds[i] = week_colors[i];
                    }

                    FastLED.show();

                    seg7.writeDigitNum(0, hour / 10);
                    seg7.writeDigitNum(1, hour % 10);
                    seg7.writeDigitNum(3, minute / 10);
                    seg7.writeDigitNum(4, minute % 10);

                    seg7.drawColon(clock_1s_ticked_on);

                    seg7.writeDisplay();
                }
                break;
            case State::DayAlarm:
                if (state_changed) {
                    seg7.writeDigitNum(0, day_alarm.hour / 10);
                    seg7.writeDigitNum(1, day_alarm.hour % 10);
                    seg7.writeDigitNum(3, day_alarm.minute / 10);
                    seg7.writeDigitNum(4, day_alarm.minute % 10);

                    seg7.drawColon(true);

                    seg7.writeDisplay();
                }
                break;
            case State::DayAlarm_SetHour:
                if (state_changed) {
                    clock_500ms_ticked_on = false;
                    clock_500ms_millis = 0;
                }

                if (clock_500ms_ticked) {
                    if (clock_500ms_ticked_on) {
                        seg7.writeDigitNum(0, day_alarm.hour / 10);
                        seg7.writeDigitNum(1, day_alarm.hour % 10);
                        seg7.writeDigitNum(3, day_alarm.minute / 10);
                        seg7.writeDigitNum(4, day_alarm.minute % 10);
                    } else {
                        seg7.writeDigitRaw(0, 0);
                        seg7.writeDigitRaw(1, 0);
                    }

                    seg7.writeDisplay();
                }
                break;
            case State::DayAlarm_SetMinute:
                if (state_changed) {
                    clock_500ms_ticked_on = false;
                    clock_500ms_millis = 0;
                }

                if (clock_500ms_ticked) {
                    if (clock_500ms_ticked_on) {
                        seg7.writeDigitNum(0, day_alarm.hour / 10);
                        seg7.writeDigitNum(1, day_alarm.hour % 10);
                        seg7.writeDigitNum(3, day_alarm.minute / 10);
                        seg7.writeDigitNum(4, day_alarm.minute % 10);
                    } else {
                        seg7.writeDigitRaw(3, 0);
                        seg7.writeDigitRaw(4, 0);
                    }

                    seg7.writeDisplay();
                }
                break;
        }

        curr_millis = millis();
    }

    /* Disable Input Module */
    for (int brightness = 15; brightness > 0; brightness --) {
        seg7.setBrightness(brightness);
        delay(10);
    }

    seg7.clear();
    seg7.setDisplayState(false);

    digitalWrite(DISPLAY_PWR_PIN, LOW);
}

void loop() {
    /* Code below will run once after waking up */

    power_all_enable();
    wdt_disable();

    if (waken_up_by_btn) {
        enter_awake_loop();
        waken_up_by_btn = false;
    } else {
        check_alarm();
    }

    /* Put controller to sleep */

    wdt_enable(WDTO_1S);
    WDTCSR |= bit(WDIE); // enable watch dog interrupt

    power_all_disable();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    noInterrupts();
    sleep_enable();
    interrupts();
    sleep_cpu();
}


#endif
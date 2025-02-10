#include <Arduino.h>
#include <DFPlayerMini_Fast.h>

#if !defined(UBRR1H)
#include <SoftwareSerial.h>
SoftwareSerial dfSerial(2, 3);
#endif

DFPlayerMini_Fast df;

void setup() {
    Serial.begin(115200);

#if !defined(UBRR1H)
    dfSerial.begin(9600);
    df.begin(dfSerial, true);
#else
    Serial1.begin(9600)
    df.begin(Serial1, true);
#endif

    df.volume(2);
    delay(1000);
    df.repeatFolder(1);
}

void loop() {

}
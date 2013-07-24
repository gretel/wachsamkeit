/*--------------------------------------------------------------------------------------------------
 "WACHSAMKEIT"
 Arduino based vehicle alarm system - Prototype Code
 https://github.com/gretel/wachsamkeit
 ɔ 2012-2013 Tom Hensel <tom@interpol8.net> Hamburg, Germany
 CC BY-SA 3.0 http://creativecommons.org/licenses/by-sa/3.0/
 --------------------------------------------------------------------------------------------------*/

// id
#define ID "WACHSAMKEIT"
#define VERSION 06072013
#define DEBUG 1

// includes
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include "Arduino.h"
#include "Wire.h"

// 3rd party includes
#include <Streaming.h>
#include <EEPROM.h>
#include <EEPROMAnything.h>
#include <movingAvg.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <DHT.h>
#include <toneAC.h>

//#include <Button.h>

#if DEBUG
#include <SoftwareSerial.h>
#define DEBUG_RX 11
#define DEBUG_TX 12
#define DEBUG_SPEED 57600
#define DEBUG_INTERVAL 333
#endif

// hardware
#define BOARD_LED 13
#define CUR_PIN A2
#define DHT_PIN A1
#define GAS_PIN A0
#define PIR_PIN 5

DHT dht;

struct config_t
{
    uint16_t version;
} config;

#if DEBUG
SoftwareSerial debugSerial(DEBUG_RX, DEBUG_TX);
movingAvg profilingFilter;
uint32_t debugTime;
uint32_t loopCycle;
void printDiag()
{
    const uint16_t duration = micros() - loopCycle;
    const uint16_t average = profilingFilter.reading(duration);

    debugSerial << " COUNT: " << micros()
                << " LOOP: " << duration
                << " AVG: " << average
                << endl;
}
#endif

void resetConfig()
{
#if DEBUG
    debugSerial << "CONFIG:RESET" << endl;
#endif
    // initialize configuration structure
    config.version = (uint16_t)VERSION;
    // write to non-volatile memory
    EEPROM_writeAnything(0, config);
    delay(100); // paranoia
}

void writeConfig()
{
    config.version = (uint16_t)VERSION;
#if DEBUG
    debugSerial << "CONFIG:WRITE";
#endif
    // write to non-volatile memory
    EEPROM_writeAnything(0, config);
    delay(100); // paranoia
}

extern void __attribute__((noreturn))
setup()
{
    // disable unused
//    power_spi_disable();PI
//    power_twi_disable();

    // begin of setup() - enable board led
    pinMode(BOARD_LED, OUTPUT);
    digitalWrite(BOARD_LED, HIGH);
#if DEBUG
    // setup software serial for debugging output
    pinMode(DEBUG_RX, INPUT);
    pinMode(DEBUG_TX, OUTPUT);
    debugSerial.begin(DEBUG_SPEED);
    delay(10);
    // init terminal, clear screen, disable cursor
    debugSerial << "\x1b\x63" << "\x1b[2J" << "\x1b[?25l";
    // output some basic info
    debugSerial << endl << ID << ":" << VERSION << endl;
#endif

    // read configuration from non-volatile memory
    EEPROM_readAnything(0, config);
    // check if data has been written and loaded using the same firmware version
    // or if the reset combo is being pressed on startup
    if ((config.version != (uint16_t)VERSION))
    {
        resetConfig();
    }
#if DEBUG
    debugSerial << "CONFIG:READ:" << config.version << endl;
    debugTime = millis() + DEBUG_INTERVAL;
#endif

    debugSerial << "DHT:INIT:" << DHT_PIN << endl;
    dht.setup(DHT_PIN);

    pinMode(PIR_PIN, INPUT);

    toneAC(600, 4, 75);
    toneAC(400, 6, 75);
    toneAC(200, 8, 125);

    digitalWrite(BOARD_LED, LOW);
}

extern void __attribute__((noreturn))
loop()
{
  tmElements_t tm;
  if (RTC.read(tm)) {
    debugSerial << tm.Hour << ":" << tm.Minute << "." << tm.Second << " " << tm.Day << "." << tm.Month << "." << tmYearToCalendar(tm.Year) << endl;
  } else {
    if (RTC.chipPresent()) {
        debugSerial << "RTC:STOPPED" << endl;
    } else {
        debugSerial << "RTC:FAIL" << endl;
    }
  delay(500);
  }

  // MQ-2
  int sensorValue = analogRead(GAS_PIN);
  float vol = (float) sensorValue / 1024 * 5.0;
  debugSerial << "GAS:" << vol << endl;

  // DHT-11
  delay(dht.getMinimumSamplingPeriod());
  debugSerial << "HMD:" << dht.getStatusString() << " " << dht.getHumidity() << " " << dht.getTemperature() << endl;

  // CS
  float amplitude_current = (float) (analogRead(CUR_PIN) - 512) / 1024 * 5 / 185 * 1000000;
  float effective_value = amplitude_current / 1.414;
  debugSerial << "CUR:" << effective_value << endl;

  // PIR
  uint8_t move = digitalRead(PIR_PIN);
  debugSerial << "PIR:" << move << endl;
  if(move == LOW)
  {
    for(uint8_t c = 0; c < 10; c++)
    {
      toneAC(2000, 10, 50);
      toneAC(4000, 10, 50);
      toneAC(6000, 10, 100);
    }
  }

#if DEBUG
    if ((millis() - debugTime) > DEBUG_INTERVAL)
    {
        printDiag();
        debugTime = millis();
        toneAC(30, 1, 1, false);
    }
    loopCycle = micros();
#endif
}

// end

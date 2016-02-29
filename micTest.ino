#include <DS3231.h>

#include <ArduinoJson.h>

#include <LowPower.h>

#include <MemoryFree.h>
#include <pgmStrToRAM.h>

#include <Average.h>
#include <Adafruit_ADS1015.h>
#include <Wire.h>
#define ADS1015_REG_CONFIG_DR_3300SPS (0x00C0);

Adafruit_ADS1115 ads;
DS3231 clock;
RTCDateTime dt;

const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
const int wakeUpPin = 2;
const int XBee_wake = 9; // Arduino pin used to sleep the XBee
int sample;
int reading;
int battery_in = A6;

unsigned long current_time;               //Stores the current time in timestamp form
unsigned long start_time;
unsigned long time_store[60];
unsigned long value_store[60];
String csv_separator = ",";


void wakeUp()
{

}


void setup()
{
  Serial.begin(9600);

  //Initialize 16bit ADC
  ads.setGain(GAIN_EIGHT);
  ads.begin();

  // Initialize DS3231
  clock.begin();
  clock.enableOutput(false);
  // Set sketch compiling time
  clock.setDateTime(__DATE__, __TIME__);

  clock.armAlarm1(false);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();
  clock.setAlarm1(0, 0, 0, 20, DS3231_MATCH_S);

  pinMode(wakeUpPin, INPUT);
  pinMode(13, OUTPUT);
}


void loop()
{
  pinMode(XBee_wake, INPUT); // put pin in a high impedence state
  digitalWrite(XBee_wake, HIGH);
  
  //Get the current time to see if we should carry on looping
  dt = clock.getDateTime();
  start_time = dt.unixtime;

  int z = 0;

  //Stop looping when it has been an hour
  while (z < 1)
  {
    int high = 0;
    //Take 20 readings and store the loudest one
    for (int x = 0; x < 20 ; x++)
    {
      reading = getAmplitude();
      if (reading > high)
      {
        high = reading;
      }
    }

    dt = clock.getDateTime();
    current_time = dt.unixtime;
    time_store[z] = current_time;
    value_store[z] = high;

    z++;

    //Set interrupt on signal from clock
    attachInterrupt(0, wakeUp, FALLING);

    // Enter power down, wake when clock signals
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

    // Disable external pin interrupt on wake up pin.
    detachInterrupt(0);

    //Clear the alarm so it fires future interrupts
    clock.clearAlarm1();
  }

  // wake up the XBee
  pinMode(XBee_wake, OUTPUT);
  digitalWrite(XBee_wake, LOW);
  delay(1000);
  for (int i = 0; i < 60; i++)
  {
    Serial.print(time_store[i]);
    delay(20);
    Serial.print(",");
    delay(20);
    Serial.println(value_store[i]);
    delay(20);
  }

  //Print battery voltage
  int raw_voltage = analogRead(battery_in);
  float true_voltage = raw_voltage * (5.0 / 1023.0) - 0.25;

  Serial.print("batt,");
  Serial.println(true_voltage);
  Serial.print("!");

  //MMake sure everything is written out
  Serial.flush();

  
  current_time = 0;
}


int getAmplitude()
{
  int signalMax = -32767;
  int signalMin = 32767;
  unsigned long startMillis = millis(); // Start of sample window
  long sample;
  // collect data for 50
  while (millis() - startMillis < sampleWindow)
  {
    sample = ads.readADC_Differential_0_1();
    if (sample < 32767 && sample > -32767)  // toss out spurious readings
    {
      if (sample > signalMax)
      {
        signalMax = sample;  // save just the max levels
      }
      else if (sample < signalMin)
      {
        signalMin = sample;  // save just the min levels
      }
    }
  }



  return signalMax - signalMin;  // max - min = peak-peak amplitude
}

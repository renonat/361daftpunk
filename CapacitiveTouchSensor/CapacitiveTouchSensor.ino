#include <CapacitiveSensor.h>

CapacitiveSensor   cs_1_15 = CapacitiveSensor(1,15);
CapacitiveSensor   cs_1_16 = CapacitiveSensor(1,16);
CapacitiveSensor   cs_1_17 = CapacitiveSensor(1,17);
CapacitiveSensor   cs_1_18 = CapacitiveSensor(1,18);
CapacitiveSensor   cs_1_19 = CapacitiveSensor(1,19);
CapacitiveSensor   cs_1_22 = CapacitiveSensor(1,22);

void setup()                    
{
//   cs_1_15.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
//   cs_1_16.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
//   cs_1_17.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
//   cs_1_18.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
//   cs_1_19.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
//   cs_1_22.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   Serial.begin(9600);

   pinMode(3, OUTPUT);
   pinMode(4, OUTPUT);
   pinMode(5, OUTPUT);
   pinMode(6, OUTPUT);
   pinMode(7, OUTPUT);
   pinMode(8, OUTPUT);
}

const int TOUCH_THRESHOLD = 1000;

void loop()                    
{
    long total1 =  cs_1_15.capacitiveSensor(30);
    long total2 =  cs_1_16.capacitiveSensor(30);
    long total3 =  cs_1_17.capacitiveSensor(30);
    long total4 =  cs_1_18.capacitiveSensor(30);
    long total5 =  cs_1_19.capacitiveSensor(30);
    long total6 =  cs_1_22.capacitiveSensor(30);

    if(total1 > TOUCH_THRESHOLD)
    {
      Serial.print("pin 15: ");
      Serial.println(total1);
      digitalWrite(3, 1);
    }
    else
    {
      digitalWrite(3, 0);
    }

    if (total2> TOUCH_THRESHOLD)
    {
      Serial.print("pin 16: ");
      Serial.println(total2);
      digitalWrite(4, 1);
    }
    else
    {
      digitalWrite(4, 0);
    }

    if(total3 > TOUCH_THRESHOLD)
    {
      Serial.print("pin 17: ");
      Serial.println(total3);
      digitalWrite(5, 1);
    }
    else
    {
      digitalWrite(5, 0);
    }

    if (total4> TOUCH_THRESHOLD)
    {
      Serial.print("pin 18: ");
      Serial.println(total4);
      digitalWrite(6, 1);
    }
    else
    {
      digitalWrite(6, 0);
    }

    if(total5 > TOUCH_THRESHOLD)
    {
      Serial.print("pin 19: ");
      Serial.println(total5);
      digitalWrite(7, 1);
    }
    else
    {
      digitalWrite(7, 0);
    }

    if (total6 > TOUCH_THRESHOLD)
    {
      Serial.print("pin 22: ");
      Serial.println(total6);
      digitalWrite(8, 1);
    }
    else
    {
      digitalWrite(8, 0);
    }

      delay(10);                             // arbitrary delay to limit data to serial port 
}

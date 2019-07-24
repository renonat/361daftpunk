#include <CapacitiveSensor.h>

CapacitiveSensor   cs_2_0 = CapacitiveSensor(2,0);
CapacitiveSensor   cs_2_1 = CapacitiveSensor(2,1);

void setup()                    
{
   cs_2_0.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   cs_2_1.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example

   Serial.begin(9600);

   pinMode(22, OUTPUT);
   pinMode(23, OUTPUT);
}


void loop()                    
{
    long total0 =  cs_2_0.capacitiveSensor(30);
    long total1 =  cs_2_1.capacitiveSensor(30);


    if(total0 > 10000 && total1 < 10000)
    {
      Serial.print("pin 0: ");
      Serial.println(total0);
      digitalWrite(22, 1);
    }
    else
    {
      digitalWrite(22, 0);
    }

    if (total1 > 10000 && total0 < 10000)
    {
      Serial.print("pin 1: ");
      Serial.println(total1);
      digitalWrite(23, 1);
    }
    else
    {
      digitalWrite(23, 0);
    }

      delay(10);                             // arbitrary delay to limit data to serial port 
}

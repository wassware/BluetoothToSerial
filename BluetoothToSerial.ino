// interfaces a connect to bluetooth to the serial and serial2 in parallel
// used either as a bluetooth client or as an interface to a mqtt client
// any Serial sent to bluetooth
// any BT input shows as serial
// and Serial2 input shown in [..]

// single property - server to connect to

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

#include <PubSubClient.h>
#include "FS.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
// for json props
DynamicJsonDocument doc(100);
const int bsize = 60;
char b[bsize];

int ledPin = 2;

unsigned long retryAt = 0;
unsigned long retryInterval = 30000;

void readProps(fs::FS &fs, const char * path)
{
   Serial.printf("Reading file: %s\r\n", path);   
   File file = fs.open(path);
   if(!file || file.isDirectory()){
       Serial.printf("− failed to open %s for reading\r\n", path);
       return;
   }
   int ptr = 0; 
   while(file.available() && ptr < bsize-1)
   {
      b[ptr++] = file.read();
   }
   b[ptr]=0;
   Serial.println(b);
   Serial.printf("Read buffer %d / %d \r\n", ptr, bsize);
   // Deserialize the JSON document
   DeserializationError error = deserializeJson(doc, b);
  // Test if parsing succeeds.
  if (error) 
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
}

void setup() 
{ 
 Serial.begin(115200);
 Serial.println("Bluetooth to Serial/Serial2 connector");
 if(!SPIFFS.begin(false))
 {
  Serial.println("SPIFFS Mount Failed");
  return;
 }
 pinMode (ledPin, OUTPUT);
 readProps(SPIFFS, "/properties.txt");
 Serial2.begin(115200);
 SerialBT.begin("interface",true);
}



void loop() 
{
  delay(10);
  digitalWrite (ledPin, LOW);
  if (!SerialBT.connected())
  {
    if (millis() > retryAt)
    {
      String btserver = doc["btserver"]; 
      Serial.printf("Connecting to BT %s\r\n", btserver);
      digitalWrite (ledPin, HIGH);
      bool ok = SerialBT.connect(btserver);
      if (!SerialBT.connected())
      {
        Serial.println("BT connection failed..");
        retryAt = millis() + retryInterval;
        return;
      }
      Serial.println("connected..");
    }
  }
  if (Serial2.available())
  {
    digitalWrite (ledPin, HIGH);
    // put mqqt incoming in square brackets..
    Serial.write('[');
    while (Serial2.available())
    {
      char c = Serial2.read();
      if (SerialBT.connected())
      {
        SerialBT.write(c);
      }
    }
    Serial.write('[');
  }
  
  while (SerialBT.available())
  {
    digitalWrite (ledPin, HIGH);
    char c = SerialBT.read();
    Serial2.write(c);
    Serial.write(c);
  }
  while (Serial.available())
  {
     digitalWrite (ledPin, HIGH);
    char c = Serial.read();
    if (SerialBT.connected())
    {
      SerialBT.write(c);
    }
  }
}

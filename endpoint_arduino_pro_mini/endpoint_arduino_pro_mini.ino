#include <Q3i2c.h>

#define MY_I2C_ADDR 11

#define MOTION_DETECTOR true
#ifdef MOTION_DETECTOR
  #define pinHCSR 3 /* pin do sensora ruchu */
  volatile bool HCSRstate = false;
#endif

//#define SOUND_DETECTOR true
#ifdef SOUND_DETECTOR
  #define pinSoundsensor 2 /* pin do sensora dzwieku */
  volatile bool soundState = false;
#endif

#define SENSOR_DS1820 true
#ifdef SENSOR_DS1820
  #include <OneWire.h> /* for ds18b20 */
  #define pinDS1820 6 /* pin DQ 1wire od DS18B20 */
  OneWire ds(pinDS1820);
  byte dsAddr[8];
  boolean dsAddrFound = false;
#endif

//#define SENSOR_DHT11 true
#ifdef SENSOR_DHT11
  #include <SimpleDHT.h>
  #define pinDHT 4 /* pin do DHT11 */
  SimpleDHT11 dht11;
#endif

#define SENSOR_PHOTORESISTOR true
#ifdef SENSOR_PHOTORESISTOR
  #define pinPhotoresistor A0 /* pin fotorezystora */
#endif

#define MOTION_LED true
#ifdef MOTION_LED
  #define pinLed 5 /* pin do lampki LED ktora sie zapali jak bedzie sensor ruchu aktywny */
  unsigned int ledCounter = 0;
#endif

Q3i2c I2C;

unsigned int secCounter = 1200;

#ifdef SENSOR_DS1820
void findDs() {
  byte address[8];
  ds.reset_search();
  while(ds.search(address)) {
    if (address[0] != 0x28)
      continue;

    if (OneWire::crc8(address, 7) != address[7]) {
      break; /* wrong address, probably checksum invalid, shit */
    }
    for (byte i=0;i<8;i++) {
      dsAddr[i] = address[i];
    }
    dsAddrFound = true;
  } /* while ds.search */
}

float getTempDs() {

  byte data[2];

  ds.reset();
  ds.select(dsAddr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  delay(1000);

  ds.reset();
  ds.select(dsAddr);
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) { /* we need 9 bytes (not really, we need only first 2) */
    if (i > 1) ds.read(); /* we need to read it even if we dont need it, just because it is */
    else data[i] = ds.read();
  }

  ds.reset_search();

  /* we need to calculate in this two steps to still have good precision */
  float tempRead = ((data[1] << 8) | data[0]);
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}

void readDS() {
  float temperature = getTempDs();
  I2C.sendData(MY_I2C_ADDR, "TEMP", temperature);
}
#endif

#ifdef MOTION_DETECTOR
void HCSRchange() {
  HCSRstate = true;
}
#endif

#ifdef SOUND_DETECTOR
void soundChange() {
  soundState = true;
}
#endif

#ifdef SENSOR_DHT11
void readDHT() {
  byte temperature;
  byte humidity;
  if (!dht11.read(pinDHT, &temperature, &humidity, NULL)) {
      /*I2C.sendData(MY_I2C_ADDR, "TEMP", (float)temperature);*/ /* we dont care about temperature from this shit, because its shitty */
      I2C.sendData(MY_I2C_ADDR, "HUMI", (float)humidity);
    }
}
#endif

#ifdef SENSOR_PHOTORESISTOR
void readPhotoresistor() {
  float light;
  light = analogRead(pinPhotoresistor);
  I2C.sendData(MY_I2C_ADDR, "LIGH", light);
}
#endif

void setup() {
  Wire.begin(MY_I2C_ADDR);

  #ifdef MOTION_DETECTOR
    pinMode(pinHCSR, INPUT);
  #endif
  #ifdef SOUND_DETECTOR
    pinMode(pinSoundsensor, INPUT);
  #endif
  #ifdef SENSOR_DHT11
    pinMode(pinDHT, INPUT);
  #endif
  #ifdef MOTION_LED
    pinMode(pinLed, OUTPUT);
    digitalWrite(pinLed, LOW);
  #endif
  #ifdef SENSOR_PHOTORESISTOR
    pinMode(pinPhotoresistor, INPUT);
  #endif
  
  delay(3000); // give the sensors time to startup

  #ifdef SENSOR_DS1820
  /* search for ds18b20 */
  findDs();
  #endif
  
  /* attach sensors interrupts */
  #ifdef MOTION_DETECTOR
    attachInterrupt(digitalPinToInterrupt(pinHCSR), HCSRchange, RISING);
  #endif
  #ifdef SOUND_DETECTOR
    attachInterrupt(digitalPinToInterrupt(pinSoundsensor), soundChange, RISING);
  #endif

}

void loop() {
  #ifdef MOTION_DETECTOR
    if (HCSRstate) {
      I2C.sendData(MY_I2C_ADDR, "MOVE", (float)1);
      #ifdef MOTION_LED
        digitalWrite(pinLed, HIGH);
      #endif
      /*ledCounter = 0;*/
      HCSRstate = false;
    }
  #endif

  #ifdef SOUND_DETECTOR
  if (soundState) {
    I2C.sendData(MY_I2C_ADDR, "SNDS", (float)1);
    soundState = false;
  }
  #endif
  
  if (secCounter >= 1200) {
    #ifdef SENSOR_DHT11
      readDHT(); /* read only the humidity form DHT11, because temperature is a shit */
    #endif
    #ifdef SENSOR_DS1820
      if (dsAddrFound) readDS(); /* read the temperature from DS18B20 (only if the sensor has been found) */
    #endif
    #ifdef SENSOR_PHOTORESISTOR
      readPhotoresistor(); /* read the level of light */
    #endif
    secCounter = 0;
  }
  #ifdef MOTION_LED
    if (ledCounter >= 300) { /* it was set at 6000 but the light was ALMOST always ON */
      digitalWrite(pinLed, LOW);
      ledCounter = 0;
    }
    ledCounter++;
  #endif
  secCounter++;
  delay(50);
}


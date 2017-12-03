#ifndef q3i2c_h
#define q3i2c_h
#include <Arduino.h>
#include <Wire.h>
#include <String.h>

struct I2CPacket {
  byte sourceAddr;
  char sensorName[4];
  float sensorValue;
};

template <typename T> int I2C_writeAnything(const T& value) {
  int size = sizeof value;
  byte vals[size];
  const byte * p = (const byte*) &value;
  unsigned int i;
  for (i = 0; i < sizeof value; i++)
    vals[i] = *p++;

  Wire.write(vals, size);
  return i;
}
template <typename T> int I2C_readAnything(T& value) {
  byte * p = (byte*) &value;
  unsigned int i;
  for (i = 0; i < sizeof value; i++)
    *p++ = Wire.read();
  return i;
}

class Q3i2c {
	public:
		bool sendData(int, String, float);
		I2CPacket readData();
};

#endif
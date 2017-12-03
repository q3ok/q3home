#include <Q3i2c.h>

bool Q3i2c::sendData(int my_addr, String sensor_name, float sensor_value) {
	I2CPacket pck;
	if (sensor_name.length() != 4) return false;
	pck.sourceAddr = my_addr;
	pck.sensorValue = sensor_value;
	sensor_name.toCharArray(pck.sensorName, 5);
	
	Wire.beginTransmission(8);
	I2C_writeAnything(pck);
	Wire.endTransmission();
	
	return true;
}

I2CPacket readData(int bytes) {
	I2CPacket pck;
	pck.sourceAddr = 0;
	if (bytes == sizeof(pck)) {
		I2C_readAnything(pck);
	}
	return pck;
}
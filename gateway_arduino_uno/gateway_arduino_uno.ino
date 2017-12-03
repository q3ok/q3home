/**
   Q3 433+I2C Gateway to ESP8266
*/
#include <VirtualWire.h>
#include <Wire.h>
#include <Q3i2c.h>

/* 433mhz conf */
#define QRF_SPEED 2000
#define QRF_RX_PIN 3
#define QRF_TX_PIN 2

void setup() {
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
  Serial.begin(115200);
  vw_set_ptt_inverted(true);
  vw_setup(QRF_SPEED);
  vw_set_rx_pin(QRF_RX_PIN);
  vw_set_tx_pin(QRF_TX_PIN);
  vw_rx_start();
  Serial.println("Arduino gateway ready");
}

void loop() {
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  if (vw_get_message(buf, &buflen)) {
    int i;
    for (i = 0; i < buflen; i++) {
      Serial.write(buf[i]);
    }
    Serial.println();
  }
  /*if (Serial.available() > 0) {
    String data;
    data = Serial.readStringUntil('\n');
    parseInputData( data );
  }*/
}

void parseInputData(String input) {
  String t1, t2, t3, t4;
  int t1d, t2d, t3d;
  String delimiter = "|";
  t1d = input.indexOf(delimiter);
  if ( t1d == -1 ) return;
  t1 = input.substring(0, t1d);
  if ( t1 != "Q3" ) return;
  
  t2d = input.indexOf(delimiter, t1d + 1);
  t2 = input.substring(t1d + 1, t2d);

  t3d = input.indexOf(delimiter,t2d+1);
  t3 = input.substring(t2d + 1, t3d);

  t4 = input.substring(t3d + 1, input.length());

  /* Receiving protocol: Q3|DESTINATION|ACTION|DATA */
  /* not implemented yet at all */
  int destination = t2.toInt();
  char msg[input.length()+1];
  input.toCharArray(msg, input.length());
  if (destination > 433) {
    /* we have to use wireless way */
    vw_send((uint8_t *)msg, strlen(msg));
    vw_wait_tx();
    
  } else {
    /* sending data thru i2c */
    Wire.begin(destination);
    Wire.write(msg, sizeof(msg));
    Wire.endTransmission();
  }
  
}
/**
 * Get the information from I2C line
 * and transfer it to ESP8266 by Serial
 */
void receiveEvent(int howMany) {
  if (howMany != sizeof(I2CPacket)) {
    return; // something wrong with received packet, maybe some hackers!
  }
  I2CPacket packet;
  I2C_readAnything(packet);
  Serial.print("Q3|");
  Serial.print(packet.sourceAddr);
  Serial.print(packet.sensorName);
  Serial.print("|");
  Serial.print(packet.sensorValue);
  Serial.println();
}


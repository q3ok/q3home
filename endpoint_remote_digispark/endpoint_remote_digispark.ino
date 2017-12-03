#define __AVR_ATtiny85__
#include <VirtualWire.h>
#include <OneWire.h>
#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/wdt.h> //Needed to enable/disable watch dog timer

#define ONEWIRE_PIN 0
#define RF_PIN 1
#define LIGHT_PIN 3

/**
   Prescaler division
*/
#define CLOCK_PRESCALER_1   (0x0)
#define CLOCK_PRESCALER_2   (0x1)
#define CLOCK_PRESCALER_4   (0x2)
#define CLOCK_PRESCALER_8   (0x3)
#define CLOCK_PRESCALER_16  (0x4)
#define CLOCK_PRESCALER_32  (0x5)
#define CLOCK_PRESCALER_64  (0x6)
#define CLOCK_PRESCALER_128 (0x7)
#define CLOCK_PRESCALER_256 (0x8)

volatile uint8_t ticks = 8;

const byte addr[8] = {0x28, 0xFF, 0x27, 0x20, 0x35, 0x16, 0x4, 0x5D};

OneWire ds(ONEWIRE_PIN);

float getTemp() {

  byte data[2];

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  delay(1000);

  ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) { // we need 9 bytes (not really, we need only first 2)
    if (i > 1) ds.read();
    else data[i] = ds.read();
  }

  ds.reset_search();

  /* we need to calculate in this two steps to still have good precision */
  float tempRead = ((data[1] << 8) | data[0]);
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}

void setup() {
  pinMode(LIGHT_PIN, INPUT);
  vw_set_ptt_inverted(true); // Required by the RF module
  vw_setup(2000); // bps connection speed
  vw_set_tx_pin(RF_PIN); // Arduino pin to connect the receiver data pin
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); //Power down everything, wake up from WDT
}

ISR(WDT_vect) {
  ticks++;
}

void loop() {
  if (ticks > 7) {
    ADCSRA |= (1 << ADEN); //Enable ADC
    cli();
    ticks = 0;

    char tmp_buf[5];
    float temperature;

    temperature = getTemp();
    dtostrf(temperature, 5, 2, tmp_buf);

    char msg[13] = "Q3|D01T|";
    strcat(msg, tmp_buf);

    vw_send((uint8_t *)msg, strlen(msg));
    vw_wait_tx();

    temperature = analogRead(LIGHT_PIN);
    dtostrf(temperature, 5, 0, tmp_buf);

    char msg2[13] = "Q3|D01L|";
    strcat(msg2, tmp_buf);

    vw_send((uint8_t *)msg2, strlen(msg2));
    vw_wait_tx();
    sei();
  }
  goToSleep();

}

void goToSleep() {
  ADCSRA &= ~(1 << ADEN); // disable ADC

  setup_watchdog(9);
  sleep_enable();
  sleep_cpu();
  sleep_disable();
}

//Sets the watchdog timer to wake us up, but not reset
//0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
//6=1sec, 7=2sec, 8=4sec, 9=8sec
//From: http://interface.khm.de/index.php/lab/experiments/sleep_watchdog_battery/
void setup_watchdog(int timerPrescaler) {

  if (timerPrescaler > 9 ) timerPrescaler = 9; //Limit incoming amount to legal settings

  byte bb = timerPrescaler & 7;
  if (timerPrescaler > 7) bb |= (1 << 5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~(1 << WDRF); //Clear the watch dog reset
  WDTCR |= (1 << WDCE) | (1 << WDE); //Set WD_change enable, set WD enable
  WDTCR = bb; //Set new watchdog timeout value
  WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}



/*
 * 597LP Final Project
 * -------------------
 * Measures temperature and humidity 
 * Controls fans based on sensor readings 
 * Connected to an OLED display to communicate the readings/warn user 
 * Authors: Akshat Sahay, Jessica Peters, Stephen Morelli 
 */

#include <Wire.h>
#include <SPI.h>  
#include "SparkFun_Si7021_Breakout_Library.h"
#include <Adafruit_SSD1306.h>

// object for SSD1306 display
Adafruit_SSD1306 display(128,64, &Wire, 4); 

// object for Si7021 sensor 
Weather sensor;

/* define pins for hook-up */
#define LED     PD6
#define BUTTON  PD2

/* define variables */
float humidity = 0;
float tempC = 0;

bool goToSleep = 0;   // if 1, don't sleep
int delayTime = 10;   // time to delay OLED

void setup() {
  // set button as INPUT_PULLUP
  DDRD  &= ~(1 << BUTTON);  
  PORTD |= (1 << BUTTON);
  
  Serial.begin(9600); // initialize serial module 
  sensor.begin();     // initialize sensor 

  // set all pins other than TX, RX and INT0 to OUTPUT, reduces power 
  for(int i = 3; i < 20; i++) pinMode(i, OUTPUT); 

  // enable sleep and power-down mode 
  SMCR |= (1 << SM1);   // power-down mode
  SMCR |= (1 << SE);    // enable sleep

  // enable digital interrupts on PIN2
  attachInterrupt(0, digitalInterrupt, FALLING); // enable interrupts on INT0

  // initialise display, set attributes
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);  
  display.display(); 
}

void loop() {
  Serial.println("Main loop"); 
  
  humidity = sensor.getRH();
  tempC = sensor.getTemp();

  display.setCursor(1, 1);
  display.print("Temperature: ");
  display.print(tempC);
  display.println("C");
  display.print("Humidity: ");
  display.print(humidity);
  display.println("%");
  display.display();

  delay(delayTime);

  display.clearDisplay();
  display.display(); 

  delayTime = 10; 
  powerDownWatchdog(4); 
}

/*
 * Function: powerDownWatchdog
 * ---------------------------
 *  Puts device to sleep and triggers wake-up using watchdog interrupt   
 *  
 *  sleep_time: amount of time the device sleeps for
 *  
 *  returns: none
 */
void powerDownWatchdog(int sleep_time) {
  WDTCSR |= (1 << WDCE) | (1 << WDE);               // set watchdog enable (WDE) and watchdog change enable (WDCE)
  WDTCSR = (1 << WDP0)| (1 << WDP1)| (1 << WDP2);   // prescaler, set for 2s counter, get rid of WDE and WDCE
  WDTCSR |= (1 << WDIE);                            // set interrupt enable
  
  sleep_time = sleep_time/2; // set prescaler for sleep loop 
  
  delay(10); // add minor delay to make sure any previous code executes
  
  // 1 minute of sleep time, 60 = 4 * 15, where 15 is the number of loops
  for(unsigned char i = 0; i < sleep_time; i++) {
    if(goToSleep == 1) break; 
    asm("sleep"); // inline assembler to go to sleep
  }
  
  WDTCSR |= (1 << WDE) | (1 << WDCE); // set watchdog enable (WDE) and watchdog change enable (WDCE)
  WDTCSR = 0x00;                      // turn off WDT
  goToSleep = 0;  
}

/*
 * Function: ISR
 * --------------
 *  Interrupt Service Routine for WDT 
 *  Any code included will be executed whenever watchdog interrupt occurs
 * 
 *  returns: none 
 */
ISR(WDT_vect) {
  // watchdog interrupt
}

/*
 * Function: digitalInterrupt
 * --------------------------
 *  Interrupt Service Routine for digital interrupts on INT0, INT1
 *  Any code included will be executed whenever external interrupts occur
 *  
 *  returns: none 
 */
void digitalInterrupt() {
  // digital button-controlled interrupt
  goToSleep = 1; 
  delayTime = 5000; 
  
  Serial.println("Digital interrupt!");  
}

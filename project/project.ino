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

/* define pins for hook-up */
#define LED     PD6
#define BUTTON  PD2

#define BEEP    A0 

/* define range for temperature and humidity */
#define TEMP_HIGH 25
#define TEMP_LOW  15

#define HUM_HIGH  50
#define HUM_LOW   30 

// object for SSD1306 display
Adafruit_SSD1306 display(128,64, &Wire, 4); 

// object for Si7021 sensor 
Weather sensor;

/* define variables */
float humidity = 0;
float tempC = 0;

bool goToSleep = false;       // if 1, don't sleep
bool updateDisplay = false;   // if 1, update display

void setup() {
  // set button as INPUT_PULLUP
  DDRD  &= ~(1 << BUTTON);  
  PORTD |= (1 << BUTTON);

  // set LED as OUTPUT
  DDRD |= (1 << LED);

  // set BEEP as OUTPUT
  pinMode(BEEP, OUTPUT);

  sensor.begin(); // initialize sensor 

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
  // sample sensor values from Si7021
  humidity = sensor.getRH();
  tempC = sensor.getTemp();

  // compare temperature and humidity ratings 
  if((tempC < TEMP_LOW || tempC > TEMP_HIGH) || (humidity < HUM_LOW || humidity > HUM_HIGH)) {
    // warn user about their surroundings 
    PORTD |= (1 << LED);

    tone(BEEP, 1000);
    delay(500);
    noTone(BEEP); 
  }

  else {
    // surroundings are comfortable 
    PORTD &= ~(1 << LED);
  }

  // woken up by external interrupt, activate OLED
  if(updateDisplay == true) {
    // write temperature and humidity to sensor 
    display.setCursor(1, 1);
    
    display.print("Temperature: ");
    display.print(tempC);
    display.println("C");
    
    display.print("Humidity: ");
    display.print(humidity);
    display.println("%");
    display.display();

    // wait for 5 seconds 
    delay(5000);

    // clear display 
    display.clearDisplay();
    display.display(); 
  } 

  // disable OLED activation and sleep for 10 seconds 
  updateDisplay = false; 
  powerDownWatchdog(60); 
}

/*
 * Function: powerDownWatchdog
 * ---------------------------
 *  Puts device to sleep and triggers wake-up using watchdog interrupt   
 *  
 *  sleep_time: amount of time the device sleeps for, in seconds 
 *  
 *  returns: none
 */
void powerDownWatchdog(int sleep_time) {
  // initialize watchdog timer 
  WDTCSR |= (1 << WDCE) | (1 << WDE);               // set watchdog enable (WDE) and watchdog change enable (WDCE)
  WDTCSR = (1 << WDP0)| (1 << WDP1)| (1 << WDP2);   // prescaler, set for 2s counter, get rid of WDE and WDCE
  WDTCSR |= (1 << WDIE);                            // set interrupt enable
  
  sleep_time = sleep_time/2; // set prescaler for sleep loop 
  
  delay(10); // add minor delay to make sure any previous code executes
  
  // device sleeps for sleep_time seconds 
  for(unsigned char i = 0; i < sleep_time; i++) {
    // if woken up by external interrupt, break out of loop 
    if(goToSleep == true) break; 
    asm("sleep"); // inline assembler to go to sleep
  }
  
  WDTCSR |= (1 << WDE) | (1 << WDCE); // set watchdog enable (WDE) and watchdog change enable (WDCE)
  WDTCSR = 0x00;                      // turn off WDT
  goToSleep = false;  
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
  goToSleep = true; 
  updateDisplay = true;  
}

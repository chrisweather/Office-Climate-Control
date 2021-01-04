// ################################################################################
// # CO2-, Temp-, Hum-Sensor with Display and visual warnings                     #
// # Based on Wemos D1 mini, MH-Z19B CO2-Sensor, OLED RGB 1.5" Display 128x128,   #
// # BME280, Touch-Sensor                                                         #
// # https://github.com/chrisweather/Office-Climate-Control                       #
// ################################################################################

#define _TASK_SLEEP_ON_IDLE_RUN
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TaskScheduler.h>
//#include "DHT.h"
//#include <Fonts/FreeSans12pt7b.h>

// Screen dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

// You can use any (4 or) 5 pins 
#define SCLK_PIN D5 // D0 // D5 // 2 yellow 
#define MOSI_PIN D7 // D7 // D7 // 3 blue
#define DC_PIN   D2 // D2 // D5 // 4 green
#define CS_PIN   D8 // D1 // D8 // 5 orange
#define RST_PIN  D3 // D3 // D3 // 6 white

// Color definitions
#define BLACK           0x0000
#define WHITE           0xFFFF
#define RED             0xF800
#define GREEN           0x07E0
#define BLUE            0x001F
#define YELLOW          0xFFE0  
#define CYAN            0x07FF
//#define MAGENTA         0xF81F
//#define ORANGE          0xCD6600
//#define PURPLE          0x330066

// Option 1: use any pins but a little slower
Adafruit_SSD1351 display = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN);  

// Option 2: must use the hardware SPI pins 
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be 
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
//Adafruit_SSD1351 display = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);

// Define scheduler
Scheduler ts;

// Callback methods prototypes for scheduler
void ReadSensor1();
void ReadSensor2();

// Tasks for scheduler
Task tS1(9000L, TASK_FOREVER, &ReadSensor1, &ts, true);  // CO2-Sensor
Task tS2(9000L, TASK_FOREVER, &ReadSensor2, &ts, true);  // Temp-Hum-Sensor

// Define Touch-Sensor
const uint8_t interruptPin = 12;
volatile byte interruptCounter = 0;
int numberOfInterrupts = 0;
void ICACHE_RAM_ATTR handleInterrupt();

// Touch-Sensor interrupt handler
void handleInterrupt() {
  interruptCounter++;
}

// Define Temp-Hum-Sensor
// DHT 11
//#define DHTPIN 3  // RX(3)/D9  TX(1) / D10
//#define DHTTYPE DHT11
//DHT dht(DHTPIN, DHTTYPE);

// BME280
#define SEALEVELPRESSURE_HPA (1019.0) // default 1013.25, used value to be verified
Adafruit_BME280 bme; // I2C
float t = -1;
float h = -1;
float p = -1;
float a = -1;

// Define CO2-Sensor
int CO2value;                      // CO2 in ppm
SoftwareSerial co2Serial(D1, D0);  // MH-Z19B RX and TX Pin  D1, D0

// Scheduled function calls
void ReadSensor1() {
  CO2value = readCO2sensor();
}

void ReadSensor2() {
  //t = dht.readTemperature();                 // Read temperature from DHT sensor
  //h = dht.readHumidity();                    // Read rel. Humidity from DHT sensor
  t = bme.readTemperature();                   // Read temperature in °C from BME sensor
  h = bme.readHumidity();                      // Read rel. Humidity in % from BME sensor
  p = bme.readPressure() / 100.0F;             // Read Pressure in hPa from BME sensor
  a = bme.readAltitude(SEALEVELPRESSURE_HPA);  // Read Altitude in m from BME sensor

  // set to -1 if no data sent from sensor
  if (isnan(t) || isnan(h) || isnan(p) || isnan(a)) {
    t = -1;
    h = -1;
    p = -1;
    a = -1;
    }
}

//##### SETUP #####
void setup() {
  Wire.begin(1, 3); // I2C moved to these pins 1 SDA, 3 SCL
  Serial.begin(9600);
  display.dim(5);

  // Intitialize Touch-Sensor
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);

  // Initialize Display
  //display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize with the I2C addr 0x3C (for the 64x48)
  display.begin();
  display.enableDisplay(true);
  display.fillScreen(BLACK);
  display.dim(15);  // Display contrast 0-15, requires modified Adafruit_SSD1351 library
  //display.setFont(&FreeSans12pt7b);

  // Initialize Temp-Hum-Sensor
  //dht.begin();
  bme.begin(0x76); 
  
  // Initialize CO2-Sensor
  co2Serial.begin(9600);
  displayFixed();
}

//##### MAIN LOOP #####
void loop() {
  ts.execute();

  if(interruptCounter>0) {
    interruptCounter--;
    numberOfInterrupts++;
    setBreakTimer();
    }

  // Display current CO2 value
  // Green <700, Yellow >=700, Red >=1000, Cyan >=1400
  display.setTextSize(2);
  if (CO2value < 1000) {
    display.setCursor(20,1);  // x, y
    display.print(" ");
    }
  else {
    display.setCursor(20,1);  // x, y
    }

  if (CO2value >= 1400) {
    display.setTextColor(CYAN, BLACK);
    display.dim(15);
    }
  else if (CO2value >= 1000 && CO2value < 1400) {
    display.setTextColor(RED, BLACK);
    display.dim(8);
    }
  else if (CO2value >= 700 && CO2value < 1000) {
    display.setTextColor(YELLOW, BLACK);
    display.dim(3);
    }
  else {
    display.setTextColor(GREEN, BLACK);
    display.dim(1);
    }
  if (CO2value < 0) {
    display.print("---");
    }
  else {
    display.print(CO2value);
    }

  if (CO2value >= 1000) {  // Display current CO2 value and warning >=1000 ppm
    display.setCursor(0,66);  // x, y
    display.setTextSize(2);
    display.setTextColor(RED, BLACK);
    display.print("   Bitte  ");     // "Please"
    display.setCursor(0,82);         // x, y
    display.print("  l\201ften! ");  // "ventilate!"
    }
  else {
    clearMessage(BLACK, BLACK);
    }

  // Temperature in °C
  if (t >= 25) {
    display.setTextColor(RED, BLACK);
    }
  else if ((t > 22 && t < 25) || t < 21){
    display.setTextColor(YELLOW, BLACK);
    }
  else {
    display.setTextColor(GREEN, BLACK);
    }

  display.setCursor(20,22);  // x, y
  display.setTextSize(2);
  if (t < 0) {
    display.print("--.-");
    }
  else {
    display.print(t,1);
    }

  // rel. Humidity in %
  if (h < 20)
    display.setTextColor(RED, BLACK);
  else if ((h >= 20 && h < 40) || h > 60)
    display.setTextColor(YELLOW, BLACK);
  else
    display.setTextColor(GREEN, BLACK);

  display.setCursor(20,43);  // x, y
  display.setTextSize(2);
  if (h < 0) {
    display.print("--.-");
    }
  else {
    display.print(h,1);
    }
} //##### END MAIN LOOP #####


// Show fixed elements on display
void displayFixed() {
  display.setTextColor(WHITE, BLACK);
  display.setTextSize(1);
  display.setCursor(78,7);   // x, y
  display.print("ppm");
  display.setTextSize(1);
  display.setCursor(78,26);  // x, y
  display.print("o");
  display.setTextSize(1);
  display.setCursor(84,28);  // x, y
  display.print("C");
  display.setCursor(78,49);  // x, y
  display.print("%");
}

// Set Break-Timer
void setBreakTimer() {
  if (numberOfInterrupts > 8) {
    numberOfInterrupts = 0;
    clearTimer(BLACK, BLACK);
    }
  if (numberOfInterrupts > 0) {
    //display.dim(15);
    display.setTextColor(BLUE, BLACK);
    display.setTextSize(1);
    display.setCursor(1,120);  // x, y
    display.print("Pause in");
    display.setTextSize(2);
    display.setCursor(65,114);  // x, y
    display.print(numberOfInterrupts * 15);
    display.setTextSize(1);
    display.setCursor(110,120);  // x, y
    display.print("min");
    }  
}

// Read CO2-Sensor
// MH-Z19B commands from sensor datasheet:
// 0x86 Read CO2 concentration
// byte cmdread[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
// Self calibration:
// After the module works for some time, it can judge the zero point intelligently and do the zero calibration automatically.
// The calibration cycle is every 24 hours since the module is power on. The zero point is 400ppm.
// Default is ON.
// 0x79 ON/OFF Self calibration function for zero point (Byte3 0xA0=ON,0x00=OFF) 
// byte cmdcalib[9] = {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};
// byte cmdcalib[9] = {0xFF, 0x01, 0x79, 0xA0, 0x00, 0x00, 0x00, 0x00, 0xE6};
int readCO2sensor() {  // Communication with MH-Z19B CO2-Sensor
  byte cmdread[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  byte answer[9];
  co2Serial.write(cmdread, 9);
  co2Serial.readBytes(answer, 9);
  if (answer[0] != 0xFF) return -1;
  if (answer[1] != 0x86) return -1;
  int answerHigh = (int) answer[2];  // CO2 High Byte
  int answerLow = (int) answer[3];   // CO2 Low Byte
  int correction = -10;  // correction in case the sensor self calibration doesn't show 400 at startup.
  int ppm = (answerHigh * 256) + answerLow + correction;
  return ppm;  // Return CO2 value in ppm
}

// Clear message area on display
void clearMessage(uint16_t color1, uint16_t color2) {
  for (uint16_t x=(display.height()-1)/2; x > 6; x-=6) {
    display.fillRect(0, display.height()/2, display.width(), display.height()/4, color1);  // x, y, w, h, color
    }
}

// Clear timer area on display
void clearTimer(uint16_t color1, uint16_t color2) {
  for (uint16_t x=(display.height()-1)/2; x > 6; x-=6) {
    display.fillRect(0, 113, display.width(), 15, color1);  // x, y, w, h, color
    }
}

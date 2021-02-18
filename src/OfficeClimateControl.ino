// ###################################################################################
// # Office-Climate-Control                                                          #
// # Monitor the climate in your office and get warned when it's unhealthy           #
// # or you should take a break.                                                     #
// # Set a timer in 15min steps (15, 30, ..., 120min) to remind you to take a break. #
// #                                                                                 #
// # CO2-, Temp-, Hum-Sensor with Display and visual warnings and Break Timer.       #
// # Based on Wemos D1 mini, MH-Z19B CO2-Sensor, OLED RGB 1.5" SPI Display 128x128,  #
// # BME280, Touch-Sensor                                                            #
// #                                                                                 #
// # Version: 1.00   18.02.2021                                                      #
// # https://github.com/chrisweather/Office-Climate-Control                          #
// ###################################################################################

#define VER  "1.0"
#define _TASK_SLEEP_ON_IDLE_RUN
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>           // Adafruit Grafics Library
#include <Adafruit_SSD1351.h>       // Adafruit Display Library
#include <U8g2_for_Adafruit_GFX.h>  // U8g2 font lib for Adafruit by Oliver Kraus
#include <Adafruit_Sensor.h>        // Adafruit Sensor Library
#include <Adafruit_BME280.h>        // Adafruit BME280 Library
#include <TaskScheduler.h>          // TaskScheduler by Anatoli Arkhipenko 

// Screen dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

// You can use any (4 or) 5 pins 
#define SCLK_PIN D5  // yellow 
#define MOSI_PIN D7  // blue
#define DC_PIN   D2  // green
#define CS_PIN   D8  // orange
#define RST_PIN  D3  // white

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

// Option 2: must use the hardware SPI pins (for UNO thats sclk = 13 and sid = 11)
// and pin 10 must be an output. This is much faster.
//Adafruit_SSD1351 display = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);

U8G2_FOR_ADAFRUIT_GFX u8g2ada;

// Define scheduler
Scheduler ts;
int numberOfInterrupts = 0;

// Callback methods prototypes for scheduler
void ReadSensor1();
void ReadSensor2();
void BreakTimer();
void BreakMsg();

// Tasks for scheduler
Task tS1(9000L, TASK_FOREVER, &ReadSensor1, &ts, true);              // CO2-Sensor
Task tS2(9000L, TASK_FOREVER, &ReadSensor2, &ts, true);              // Temp-Hum-Sensor
Task tS3(60000L, numberOfInterrupts * 15, &BreakTimer, &ts, false);  // Break Timer
Task tS4(200L, TASK_FOREVER, &BreakMsg, &ts, false);                 // Break Message

// Define Touch-Sensor
const uint8_t interruptPin = 12;
volatile byte interruptCounter = 0;

void ICACHE_RAM_ATTR handleInterrupt();

// Touch-Sensor interrupt handler
void handleInterrupt()
{
  interruptCounter++;
}

// Define BME280 Temp-Hum-Sensor
#define SEALEVELPRESSURE_HPA (1019.0)  // default 1013.25, current value to be verified
Adafruit_BME280 bme;  // I2C
float t = -1;
float h = -1;
float p = -1;
float a = -1;

// Define CO2-Sensor
int CO2value;                      // CO2 in ppm
SoftwareSerial co2Serial(D1, D0);  // MH-Z19B RX and TX Pin  D1, D0

// Scheduled function calls
// Read CO2-Sensor
void ReadSensor1() 
{
  CO2value = readCO2sensor();

  // Display current CO2 value
  // Green <700, Yellow >=700, Red >=1000, Cyan >=1400
  u8g2ada.setFont(u8g2_font_inb21_mf);
  u8g2ada.setFontMode(0);
  String lgth = String(CO2value);
  u8g2ada.setCursor(85 - u8g2ada.getUTF8Width(lgth.c_str()), 21);

  if (CO2value >= 1400) {
    u8g2ada.setForegroundColor(CYAN);
    display.dim(15);
  }
  else if (CO2value >= 1000 && CO2value < 1400) {
    u8g2ada.setForegroundColor(RED);
    display.dim(15);
  }
  else if (CO2value >= 700 && CO2value < 1000) {
    u8g2ada.setForegroundColor(YELLOW);
    display.dim(7);
  }
  else {
    u8g2ada.setForegroundColor(GREEN);
    display.dim(4);
  }
  if (CO2value < 400) {
    u8g2ada.setForegroundColor(WHITE);
    u8g2ada.print("");
  }
  else {
    u8g2ada.print(CO2value);
    u8g2ada.setFont(u8g2_font_7x13_mf);
    u8g2ada.setForegroundColor(WHITE);
    u8g2ada.setCursor(90, 21);
    u8g2ada.print("ppm");
  }

  if (CO2value >= 1000) {  // Display current CO2 value and warning >=1000 ppm
    u8g2ada.setFont(u8g2_font_inb19_mf);
    u8g2ada.setForegroundColor(RED);
    u8g2ada.setCursor(64 - (u8g2ada.getUTF8Width("Bitte")) / 2, 74);
    u8g2ada.print("Bitte");    // "Please"
    u8g2ada.setCursor(64 - (u8g2ada.getUTF8Width("lüften!")) / 2, 103);
    u8g2ada.print("lüften!");  // "ventilate!"
  }
  else {
    clearArea(0, 0, 20, 21, BLACK);    // Clear left digit of CO2value
    clearArea(0, 54, 128, 58, BLACK);  // Clear message area
  }
}

// Read Temp-Hum-Sensor
void ReadSensor2() {
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

  // Temperature in °C
  u8g2ada.setFont(u8g2_font_10x20_mf);
  u8g2ada.setBackgroundColor(BLACK);
  if (t >= 25) {
    u8g2ada.setForegroundColor(RED);
  }
  else if ((t > 22 && t < 25) || t < 21){
    u8g2ada.setForegroundColor(YELLOW);
  }
  else {
    u8g2ada.setForegroundColor(GREEN);
  }

  u8g2ada.setCursor(3, 44);
  if (t < 0) {
    u8g2ada.print("--.-");
  }
  else {
    char bufft[10];
    dtostrf(t, 3, 1, bufft);
    char tc[strlen(bufft)+1];
    for (uint8_t i = 0; i < strlen(bufft) + 1; i++){
      if (bufft[i] == '.'){
        tc[i] = ',';
      }
      else {
        tc[i] = bufft[i];  
      }
    }
    u8g2ada.print(tc);
  }
  u8g2ada.setFont(u8g2_font_7x13_mf);
  u8g2ada.setForegroundColor(WHITE);
  u8g2ada.setCursor(46, 44);
  u8g2ada.print("°C");

  // rel. Humidity in %
  if (h < 20)
    u8g2ada.setForegroundColor(RED);
  else if ((h >= 20 && h < 40) || h > 60)
    u8g2ada.setForegroundColor(YELLOW);
  else
    u8g2ada.setForegroundColor(GREEN);

  u8g2ada.setFont(u8g2_font_10x20_mf);
  u8g2ada.setCursor(77, 44);
  if (h < 0) {
    u8g2ada.print("--.-");
  }
  else {
    char buffh[10];
    dtostrf(h, 3, 1, buffh);
    char hc[strlen(buffh)+1];
    for (uint8_t i = 0; i < strlen(buffh) + 1; i++){
      if (buffh[i] == '.'){
        hc[i] = ',';
      }
      else {
        hc[i] = buffh[i];  
      }
    }
    u8g2ada.print(hc);
  }
  u8g2ada.setFont(u8g2_font_7x13_mf);
  u8g2ada.setForegroundColor(WHITE);
  u8g2ada.setCursor(121, 44);
  u8g2ada.print("%");
}

// Break Timer
void BreakTimer()
{
  if (tS3.isLastIteration() == false){
    u8g2ada.setFont(u8g2_font_8x13_mf);
    u8g2ada.setForegroundColor(BLUE);
    u8g2ada.setCursor(1,125);
    u8g2ada.print("Pause in ");
    u8g2ada.print(tS3.getIterations());
    u8g2ada.print(" min ");
  }
  else {
    numberOfInterrupts = 0;
    tS3.disable();
    clearArea(0, 115, 128, 13, BLACK);  // Clear timer area
    tS4.enable();
  }
}

// Break Message
void BreakMsg()
{
  display.dim(15);
  if (CO2value < 1000) {
    u8g2ada.setFont(u8g2_font_inb19_mf);
    u8g2ada.setForegroundColor(RED);
    u8g2ada.setCursor(64 - (u8g2ada.getUTF8Width("Pause!")) / 2, 103);
    u8g2ada.print("Pause!");
  }
  else {
    u8g2ada.setFont(u8g2_font_inb19_mf);
    u8g2ada.setForegroundColor(RED);
    u8g2ada.setCursor(64 - (u8g2ada.getUTF8Width("Bitte")) / 2, 74);
    u8g2ada.print("Bitte");
    u8g2ada.setCursor(64 - (u8g2ada.getUTF8Width("lüften!")) / 2, 103);
    u8g2ada.print("lüften!");
    delay(1000);
    clearArea(0, 54, 128, 58, BLACK);  // Clear message area
    u8g2ada.setFont(u8g2_font_inb19_mf);
    u8g2ada.setForegroundColor(RED);
    u8g2ada.setCursor(64 - (u8g2ada.getUTF8Width("Pause!")) / 2, 103);
    u8g2ada.print("Pause!");
    delay(1000);
  }
}


//##### SETUP #####
void setup() {
  // Initialize CO2-Sensor
  co2Serial.begin(9600);
  delay(1000);

  Wire.begin(1, 3);  // I2C moved to these pins 1 SDA, 3 SCL
  
  //Serial.begin(9600);
  //Serial.println("");
  //Serial.println("Office Climate Control");
  //Serial.println(VER);

  // Initialize Temp-Hum-Sensor
  bme.begin(0x76); 

  // Intitialize Touch-Sensor
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);

  // Initialize Display
  //display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize with the I2C addr 0x3C (for the 64x48)
  display.begin();
  u8g2ada.begin(display);
  display.enableDisplay(true);
  u8g2ada.setFontMode(0);
  display.fillScreen(BLACK);
  display.dim(15);  // Display contrast 0-15, requires modified Adafruit_SSD1351 library
  u8g2ada.setFont(u8g2_font_inb16_mf);
  u8g2ada.setForegroundColor(BLUE);
  u8g2ada.setCursor(0, 25);
  u8g2ada.print("Office");
  u8g2ada.setCursor(15, 50);
  u8g2ada.print("Climate");
  u8g2ada.setCursor(30, 75);
  u8g2ada.print("Control");
  u8g2ada.setFont(u8g2_font_inb16_mf);
  u8g2ada.setCursor(64 - (u8g2ada.getUTF8Width(VER)) / 2, 110);
  u8g2ada.print(VER);
  delay(2000);
  display.fillScreen(BLACK);
}

//##### MAIN LOOP #####
void loop() {
  ts.execute();  // Task Scheduler

  if(interruptCounter>0) {
    interruptCounter--;
    numberOfInterrupts++;
    setBreakTimer();
    }
} //##### END MAIN LOOP #####


// Set Break-Timer
void setBreakTimer()
{ 
  if (tS4.isEnabled() == true){        // push the touch button once to disable the break reminder
    clearArea(0, 54, 128, 58, BLACK);  // Clear message area
    tS4.disable();
    tS4.cancel();
    numberOfInterrupts = 0;
  }
  else {
    if (numberOfInterrupts > 8) {
      numberOfInterrupts = 0;
      tS3.disable();
      tS3.cancel();
      clearArea(0, 115, 128, 13, BLACK);  // Clear timer area
    }
    if (numberOfInterrupts > 0) {
      //display.dim(15);
      u8g2ada.setFontMode(0);
      u8g2ada.setFont(u8g2_font_8x13_mf);
      u8g2ada.setForegroundColor(BLUE);
      u8g2ada.setCursor(1,125);
      u8g2ada.print("Pause in ");
      u8g2ada.print(numberOfInterrupts * 15);
      u8g2ada.print(" min ");
      tS3.setIterations((numberOfInterrupts * 15) + 1);
      tS3.restart();
    }
    else {
      tS3.disable();
      tS3.cancel();
      clearArea(0, 115, 128, 13, BLACK);  // Clear timer area
    }
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

// Clear display area
void clearArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color1)
{
  display.fillRect(x, y, w, h, color1);  // x, y, w, h, color
}

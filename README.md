# Office-Climate-Control  
A work in progress device to get familiar with ESP82xx, misc sensors, Github, etc.  

CO2-, Temp-, Hum-Sensor with Display and visual warnings  
Based on Wemos D1 mini, MH-Z19B CO2-Sensor, OLED RGB 1.5" Display 128x128,
BME280 (DHT11), Touch-Sensor  

The BME280 sensor is more accurate and reliable than DHT11.


![Office-Climate-Control](doc/front.jpg "Front View")  

![Office-Climate-Control](doc/back.jpg "Back View")  

The goal is to display the sensor values in green, yellow, red, cyan based on their values.
Also include a timer to remind me having a break from my work from time to time.  

**To Do List**
 * Develop a break timer which reminds me to have a break after x minutes.  
  This timer will be controlled by the touch sensor inside the cover.  
 * Include u8g2 lib for better font handling
 * Think about how to use the additional sensor values Pressure and Altitude from BME280.


**To protect the display from burn in effects I have extended the Adafruit_SSD1351 lib to include a dim function.**  

 * Adafruit_SSD1351.h  
    ```
    void dim(uint8 contrastlevel = 15);  // Display contrast 0-15
   ```


 * Adafruit_SSD1306.cpp  
   ```
    // Dim the display via CONTRASTMASTER 0-15
    void Adafruit_SSD1351::dim(uint8_t contrastlevel) {
      sendCommand(SSD1351_CMD_CONTRASTMASTER, &contrastlevel, 1);
    }
   ```


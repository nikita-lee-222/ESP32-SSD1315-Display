# ESP32 Multi-Mode OLED Display

## Description
This project implements a multi-mode ESP32 application using an I2C OLED display. It includes:

- Sleep static animation
- Weather display using OpenWeatherMap API
- Bitcoin price display using CoinGecko API
- Stopwatch/Timer mode

The system supports 4 physical buttons for switching modes and controlling the stopwatch. Data is stored in non-volatile memory for persistence.

## Features
- WiFi connection and API requests
- I2C OLED display with optimized update
- Debounced button inputs
- Persistent storage using `Preferences`
- Stopwatch with start/stop and timer display
- Real-time weather and cryptocurrency data

## Hardware
- ESP32 (any board)
- I2C OLED 128x64 (SSD1315/SSD1312 compatible)
- 4 push buttons connected to GPIOs 4, 16, 17, 5

## Wiring
- SDA → D2
- SCL → D15
- K1 → D4
- K2 → D16
- K3 → D17
- K4 → D5
(K1-4 means Button1-4)

## Usage
1. Install the required libraries:
- ESP32 WiFi Library [Arduino](https://docs.arduino.cc/libraries/wifi/)
- U8g2 OLED Library [olikraus](https://github.com/olikraus/u8g2/wiki)
- ArduinoJson Library [arduinojson](https://arduinojson.org/)
- Preferences (non-volatile storage on ESP32) [espressif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html)
2. Configure WiFi credentials in `ssid` and `password`.
3. Add your OpenWeatherMap API key. You can find public free key [here](https://gist.github.com/lalithabacies/c8f973dc6754384d6cade282b64a8cb1)
4. Upload sketch to ESP32 using Arduino IDE.
5. Press buttons to switch between modes and control stopwatch.

## Performance Notes
Some users may experience occasional performance issues such as slow updates or flickering on the OLED. Common reasons include:

- **Frequent API calls**: The weather and Bitcoin data are fetched every 5 seconds. Using slower WiFi connections can delay updates.
- **OLED refresh rate**: SSD1315/SSD1312 is not very fast; frequent full-screen redraws may cause visible flickering.
- **Multiple simultaneous tasks**: Running WiFi requests, JSON parsing, and stopwatch calculations in the main loop without asynchronous handling can slow down the display.
- **I2C speed and wiring**: Long wires or electrical noise on SDA/SCL lines can cause communication delays or flickering.
- **Large JSON payloads**: Weather API responses are parsed fully on the ESP32, which may slightly impact loop timing.

This project was developed with guidance and reference materials from online resources and tools.

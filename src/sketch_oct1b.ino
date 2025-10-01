#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <U8g2lib.h>
#include <Wire.h>

//I2C OLED SSD1315
#define SDA_PIN 2
#define SCL_PIN 15
//SSD1312 driver compatible with SSD1315
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

//Buttons
#define BUTTON1 4
#define BUTTON2 16
#define BUTTON3 17
#define BUTTON4 5
#define DEBOUNCE_DELAY 50

unsigned long lastPress1 = 0;
unsigned long lastPress2 = 0;
unsigned long lastPress3 = 0;
unsigned long lastPress4 = 0;

int mode = 0;  //Current mode

//WiFi and API
const char* ssid = "SSID"; //Here just put your WIFI name
const char* password = "Password"; //Here just put your WIFI password
const char* weatherAPIKey = "bd5e378503939ddaee76f12ad7a97608"; //You can use your API or look for free one (additional info in README)
const char* city = "Cologne";

Preferences preferences;

//Timers
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 5000; //ms
unsigned long lastStopwatchUpdate = 0;

//Display data
String displayText = "Booting...";
bool displayChanged = true;

//Stopwatch variables
bool stopwatchRunning = false;
unsigned long stopwatchStart = 0;
unsigned long stopwatchElapsed = 0; //ms

//Setup
void setup() {
  Serial.begin(115200);

  //Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  //Initialize I2C and OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.setFlipMode(true);   //Correct mirrored display
  Wire.setClock(800000);    //Increase I2C speed. If your screen feels itself bad - just put a lower speed (100000, 400000). 

  //Initialize buttons
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);
  pinMode(BUTTON4, INPUT_PULLUP);

  //Initialize preferences
  preferences.begin("logs", false);

  //Load last saved data
  displayText = preferences.getString("last_data", "No data");
  displayChanged = true;
}

//Main loop
void loop() {
  handleButtons();
  unsigned long now = millis();

  //Update modes other than stopwatch
  if (mode != 4 && now - lastUpdate > updateInterval) {
    lastUpdate = now;
    updateData();
  }

  //Stopwatch mode updates every second
  if (mode == 4 && now - lastStopwatchUpdate >= 1000) {
    lastStopwatchUpdate = now;
    updateData();
  }

  //Redraw display only if text changed. Helps with optimization.
  if (displayChanged) {
    drawDisplay();
    displayChanged = false;
  }
}

//Handle buttons with debounce
void handleButtons() {
  unsigned long now = millis();
  if (digitalRead(BUTTON1) == LOW && now - lastPress1 > DEBOUNCE_DELAY) { mode = 0; lastPress1 = now; displayChanged = true; }
  if (digitalRead(BUTTON2) == LOW && now - lastPress2 > DEBOUNCE_DELAY) { mode = 1; lastPress2 = now; displayChanged = true; }
  if (digitalRead(BUTTON3) == LOW && now - lastPress3 > DEBOUNCE_DELAY) { mode = 2; lastPress3 = now; displayChanged = true; }
  if (digitalRead(BUTTON4) == LOW && now - lastPress4 > DEBOUNCE_DELAY) { 
    if(mode != 4) {
      //Enter stopwatch mode
      mode = 4;
      stopwatchRunning = false;
      stopwatchElapsed = 0;
    } else {
      //Start/Stop stopwatch
      if(stopwatchRunning) {
        stopwatchElapsed += millis() - stopwatchStart;
        stopwatchRunning = false;
      } else {
        stopwatchStart = millis();
        stopwatchRunning = true;
      }
    }
    lastPress4 = now;
    displayChanged = true;
  }
}

//Update display data
void updateData() {
  switch(mode) {
    case 0: //Sleep animation
      displayText = "💤 Sleep Mode\nZ z z ...";
      displayChanged = true;
      break;

    case 1: //Weather
    {
      String payload = getWeatherJSON();
      if(payload.length() > 0){
        StaticJsonDocument<1024> doc;
        DeserializationError err = deserializeJson(doc, payload);
        if(!err){
          float temp = doc["main"]["temp"];
          const char* condition = doc["weather"][0]["main"];
          displayText = String("Weather: ") + condition + " " + temp + "C";
          preferences.putString("last_data", displayText);
          displayChanged = true;
        } else {
          displayText = "Weather JSON Error";
          displayChanged = true;
        }
      } else {
        displayText = "Weather Fetch Error";
        displayChanged = true;
      }
    }
      break;

    case 2: //Bitcoin price
    {
      float btcPrice = getBitcoinPrice();
      if(btcPrice > 0){
        displayText = String("BTC: $") + btcPrice;
        preferences.putString("last_data", displayText);
        displayChanged = true;
      } else {
        displayText = "BTC Fetch Error";
        displayChanged = true;
      }
    }
      break;

    case 3: //Logs
      displayText = preferences.getString("last_data", "No data");
      displayChanged = true;
      break;

    case 4: //Stopwatch
    {
      unsigned long total = stopwatchElapsed;
      if(stopwatchRunning) total += millis() - stopwatchStart;

      unsigned long seconds = total / 1000;
      unsigned long minutes = seconds / 60;
      seconds = seconds % 60;

      char buffer[20];
      sprintf(buffer, "Timer:\n%02lu:%02lu", minutes, seconds);
      displayText = buffer;
      displayChanged = true;
    }
      break;
  }
}

//Draw OLED
void drawDisplay() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  int line = 20;
  int pos = 0;
  while(pos < displayText.length()){
    int next = displayText.indexOf('\n', pos);
    if(next == -1) next = displayText.length();
    String lineText = displayText.substring(pos, next);
    u8g2.drawStr(0, line, lineText.c_str());
    pos = next + 1;
    line += 15;
  }
  u8g2.sendBuffer();
}

//API functions
String getWeatherJSON(){
  if(WiFi.status() != WL_CONNECTED) return "";
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "&appid=" + String(weatherAPIKey) + "&units=metric";
  http.begin(url);
  int code = http.GET();
  String payload = "";
  if(code > 0) payload = http.getString();
  http.end();
  return payload;
}

float getBitcoinPrice(){
  if(WiFi.status() != WL_CONNECTED) return 0;
  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd");
  int code = http.GET();
  float price = 0;
  if(code > 0){
    String payload = http.getString();
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if(!err) price = doc["bitcoin"]["usd"];
  }
  http.end();
  return price;
}

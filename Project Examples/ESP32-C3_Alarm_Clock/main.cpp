/********************************************************************************
Transistor used for piezo buzzer: 
https://media.digikey.com/pdf/Data%20Sheets/ON%20Semiconductor%20PDFs/PN2222.pdf
********************************************************************************/

/*******************************************
Level shifter used for LCD display:
https:www.ti.com/lit/ds/symlink/txs0108e.pdf
********************************************/

/*****************************
Time API
https://www.worldtimeapi.org/
*****************************/

/***********************
Weather API
https://open-meteo.com/
***********************/

#include <Keypad.h>
#include "Clock.h"
#include "Weather.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR    0x27
#define LCD_COLUMNS 16
#define LCD_LINES   2

uint8_t state;
unsigned long clockTimer;
unsigned long weatherAPItimer;
unsigned long weatherDisplayTimer;

const char* password = "";
const char* ssid = "Wokwi-GUEST";

uint8_t valIndex;
uint8_t cursorPos;
char entered_value [6];
const uint8_t ROWS = 4;
const uint8_t COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
uint8_t colPins[COLS] = { 1, 0, 3, 2 };
uint8_t rowPins[ROWS] = { 4, 5, 6, 7 };

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);
Clock rtc(&lcd);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void enterTime()
{
  state = 1;
  memset(&entered_value[0], 0, sizeof(entered_value));
  cursorPos = 0;
  valIndex = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set clock then  ");
  lcd.setCursor(0, 1);
  lcd.print("press # to save.");
  delay(3000);
  lcd.clear();
}

void enterAlarm()
{
  state = 2;
  memset(&entered_value[0], 0, sizeof(entered_value));
  cursorPos = 0;
  valIndex = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter alarm time");
  lcd.setCursor(0, 1);
  lcd.print("press # to save.");
  delay(3000);
  lcd.clear();
}

void nextChar(char key)
{
  if (valIndex < 6)
  {
    entered_value[valIndex] = key;
    lcd.setCursor(0, 0);
    lcd.print(entered_value);
    cursorPos++;
    valIndex++;
  }
}

void eraseChar()
{
  if (valIndex > 0 )
  {
    valIndex--;
    cursorPos--;
    entered_value[valIndex] = '\0';
    lcd.setCursor(cursorPos, 0);
    lcd.print(' ');
    lcd.setCursor(cursorPos, 0);
  }
}

void keyPadState0()
{
  char key = keypad.getKey();
  switch(key)
  {
    case 'A':
      enterAlarm();
      break;
    case 'C':
      enterTime();
      break;
    case '#':
      rtc.silence();
      break;
    case '*':
      rtc.addToSnooze();
      break;
  }
}

void keyPadState1()
{
  char key = keypad.getKey();
  switch(key)
  {
    case '#':
      rtc.setTime(entered_value);
      state = 0;
      break;
    case '*':
      eraseChar();
      break;
    default:
      if (isDigit(key))
      {
        nextChar(key);
      }
      break;
  }
}

void keyPadState2()
{
  char key = keypad.getKey();
  switch(key)
  {
    case '#':
      rtc.setAlarm(entered_value);
      state = 0;
      break;
    case '*':
      eraseChar();
      break;
    default:
      if (isDigit(key))
      {
        nextChar(key);
      }
      break;
  }
}

void getInput()
{
  switch (state)
  {
    case 0:
      keyPadState0();
      break;
    case 1:
      keyPadState1();
      break;
    case 2:
      keyPadState2();
      break;
  }
}

void setup() 
{
  pinMode(8, OUTPUT);
  pinMode(10, OUTPUT);
  Wire.begin(18, 19);
  lcd.init();
  lcd.backlight();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    lcd.print(".");
    delay(1000);
  }
  lcd.clear();
  if (!rtc.setTimeFromAPI())
  {
    char t [] = __TIME__;
    char compileTime [] = { t[0], t[1], t[3], t[4], t[6], t[7] };
    rtc.setTime(compileTime);
  }
  getWeather(lcd);
  printWeather(lcd);
}

void loop() 
{
  if (state == 0)
  {
    unsigned long millisNow = millis();
    if (millisNow - clockTimer >= 1000)
    {
      clockTimer = millisNow;
      rtc.updateClock();
    }
    if (millisNow - weatherDisplayTimer >= 10000)
    {
      weatherDisplayTimer = millisNow;
      printWeather(lcd);
    }
    if (millisNow - weatherAPItimer >= 3600000)
    {
      weatherAPItimer = millisNow;
      getWeather(lcd);
    }
  }
  getInput(); 
}
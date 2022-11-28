/* Simpel demo af OLED LCD Display med I2C */
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LiquidCrystal.h>
#include <dht.h>
#include <MFRC522.h>
#include <DS3231.h>

// Clock
DS3231 rtc;
DateTime date;

//RFID - Card scanner
#define RST_PIN A3
#define SS_PIN 53
MFRC522 RFID(SS_PIN, RST_PIN);

//LCD Displays
String WrongID = "    WRONG ID";
String CorrectID = "   CORRECT ID";
String ReadyToScan = "   SKAN KORT";
#define CORRECT_ID "615486115"

bool LoggedIn = false;

// Water level sensor
#define WATER_PIN A10

// Temp & Humidity sensor
#define DHT_PIN 8
dht DHT;

//Sound sensor
#define DO_PIN 36 // Digital pin
#define AO_PIN A0 // Analog pin
int soundSensorValue = 0;
int maxSoundValue = 0;

//LCD Display 1602
#define RS_PIN 28
#define E_PIN 27
#define D4_PIN 26
#define D5_PIN 25
#define D6_PIN 24
#define D7_PIN 23

LiquidCrystal lcd(RS_PIN, E_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

//Ultrasonic sensor
#define ECHO_PIN 44
#define TRIG_PIN 45
int ultraSonicState = 0;
int distance = 0;

/*Start if joystick*/
#define JOYSTICKY_PIN A1 //Pin of Y Pos of joystick
#define JOYSTICKSW_PIN 40 //Pin of Switch/Button of joystick

//If Y Pos is minus it is down, if its in plus its up
int yPosition = 0;
int swState = 0;

/*End of joystick*/

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



int selectedItem = 8; //Item in row / menu
int itemSelected = 0; //What item HAS been selected
/*Millis*/
unsigned long prevMillis05 = 0;
unsigned long prevMillisSW = 0;
unsigned long prevMillisDistance = 0;
unsigned long prevMillisTemp = 0;
unsigned long ScanMillis = 0;


//BTN for Exit
#define BTNEXIT_PIN 3
int btnState = 0;

/*  TEST    */
String test[] = {"Distance Reader","water detector","tempature"};


void setup()
{
  Serial.begin(9600);
  SPI.begin();
  Wire.begin();
  //Clock
  Serial.println(test[0].length());
  
  pinMode(JOYSTICKY_PIN, INPUT);
  pinMode(JOYSTICKSW_PIN, INPUT_PULLUP);

  /*LED'S FOR TESTS*/
  pinMode(4, OUTPUT); //GREEN LED - DISTANCE
  pinMode(5, OUTPUT); //WHITE LED - SOUND
  pinMode(6, OUTPUT); //RED LED - WATER
  pinMode(7, OUTPUT); //BLUE LED - TEMP

  //Ultrasonic sensor
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);

  pinMode(BTNEXIT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BTNEXIT_PIN), ExitSelectedItem, HIGH);

  lcd.begin(16, 2);
  lcd.clear();


  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  RFID.PCD_Init();
  RFID.PCD_DumpVersionToSerial();
  
}
unsigned long  tempMills = 0;
void loop()
{
  //yPosition = analogRead(JOYSTICKY_PIN); //map(analogRead(JOYSTICKY_PIN), 0, 1023, 0, 7)
  
  if (LoggedIn) {
    yPosition = map(analogRead(JOYSTICKY_PIN), 0, 1023, 0, 7);
    swState = digitalRead(JOYSTICKSW_PIN);
    btnState = digitalRead(BTNEXIT_PIN);
    
    display.clearDisplay();
    display.setTextColor(WHITE); // Draw white text
    display.setTextSize(1); // Normal 1:1 pixel scale
    display.setCursor(0, 0); // Start at top-left corner
    display.println(F("Start distance reader"));
    display.setCursor(0, 12); // Start at top-left corner
    display.println(F("Start water detector"));
    display.setCursor(0, 24); // Start at top-left corner
    display.println(F("Start temperatur"));
    display.setCursor(0, 36); // Start at top-left corner
    display.println(F("Logout"));
    
    if (millis() - prevMillis05 >= 125 && swState && itemSelected == 0) {
      prevMillis05 = millis();
      if (yPosition > 3 || yPosition < 3) {
        if (yPosition > 3 && (selectedItem + 12 <= 44)) {
          selectedItem += 12;
        }
        else if (yPosition < 3 && (selectedItem - 12 >= 8)) {
          selectedItem -= 12;
        }
      }
    }
    CreateUnderLine(selectedItem);
    if (! swState && millis() - prevMillisSW >= 500 && itemSelected == 0) {
      prevMillisSW = millis();
      itemSelected = selectedItem;
    }
    if (itemSelected != 0) {
      switch (selectedItem) {
        case 8: //for distance reader
          if (millis() - prevMillisDistance >= 500) {
            prevMillisDistance = millis();
            StartDistanceReader();
          }
          break;
        case 20: //for Water detector
          if (millis() - prevMillisDistance >= 150) {
            prevMillisDistance = millis();
            StartWaterReader();
          }
          break;
        case 32: //for Temp
          if (millis() - prevMillisTemp >= 2000) {
            prevMillisTemp = millis();
            StartTempReader();
          }
          break;
        case 44: //for Logout
          LoggedIn = false;
          itemSelected = 0;
          selectedItem = 8;
          Serial.println("Logging out");
          break;
        default:
          break;
      }
    }
    display.display();
    return;
  }
  lcd.clear();
  lcd.print(ReadyToScan);
  
  display.clearDisplay();
  display.setTextColor(WHITE); // Draw white text
  display.setTextSize(1.5); // Normal 1:1 pixel scale
  display.setCursor(32,32); // Start at top-left corner
  display.println(F("Wating..."));
  display.display();
  
  if ( ! RFID.PICC_ReadCardSerial() && ! RFID.PICC_IsNewCardPresent()) {
    return;
  }
  if (millis() - ScanMillis >= 1000 && LoggedIn == 0){
    ScanMillis = millis();
    if (RFID.PICC_ReadCardSerial()) {
      String id = "";
      for (byte i = 0; i < RFID.uid.size; i++) {
        id.concat(String(RFID.uid.uidByte[i], DEC));
      }
      Serial.print(id);
      Serial.print(" | ");
      Serial.println(CORRECT_ID);
      lcd.clear();
      if (id == CORRECT_ID) {
        LoggedIn = true;
        lcd.print(CorrectID);
        Serial.println("Correct");
        delay(1000);
        return;
      }
      Serial.println("Wrong");
      lcd.print(WrongID);
      delay(2500);
    }
  }
}

//CreateUnderLine(0, 8, 100); // Starts on 8 goes up 12 for every "text"
void CreateUnderLine(int y) { //38 Logout
  display.drawFastHLine(0, y, (selectedItem < 32 ? 120 : (selectedItem == 32 ? 94 : 38)), WHITE);
}

void ExitSelectedItem() {
  if (itemSelected != 0 && millis() - tempMills >= 500 && btnState) {
    tempMills = millis();
    switch (selectedItem) {
      case 8: //for distance reader
        digitalWrite(TRIG_PIN, LOW);
        break;
      case 20: //for Sound detector
        digitalWrite(AO_PIN, LOW);
        break;
      case 32: //for Water detector
        digitalWrite(WATER_PIN, LOW);
        break;
      case 44: //for Temp
        digitalWrite(DHT_PIN, LOW);
        break;
      default:
        break;
    }
    lcd.clear();
    itemSelected = 0;
    maxSoundValue = 0;
  }
}

void StartDistanceReader() {
  digitalWrite(TRIG_PIN, HIGH);
  digitalWrite(TRIG_PIN, LOW);
  ultraSonicState = pulseIn(ECHO_PIN, HIGH);
  distance = ultraSonicState * 0.034 / 2;
  lcd.clear();
  lcd.print("Distance: ");
  lcd.print(distance);
  lcd.print("cm");
}

void StartSoundReader() {
  soundSensorValue = analogRead(AO_PIN);
  if (maxSoundValue < soundSensorValue) {
    maxSoundValue = soundSensorValue;
  }
  lcd.clear();
  lcd.print("Sound DBs: ");
  lcd.print(soundSensorValue);
  lcd.setCursor(0, 1);
  lcd.print("Max DBs: ");
  lcd.print(maxSoundValue);

}

void StartWaterReader() {
  int sensorValue = analogRead(WATER_PIN);
  lcd.clear();
  lcd.print("Water: ");
  lcd.print(sensorValue);
  Serial.println(sensorValue);
}

void StartTempReader() {
  int chk = DHT.read11(DHT_PIN);
  lcd.clear();
  lcd.print("Tempature: ");
  lcd.print(DHT.temperature);
  lcd.print("C");
  lcd.setCursor(0,1);
  lcd.print("Humidity: ");
  lcd.print(DHT.humidity);
  lcd.print("%");
}

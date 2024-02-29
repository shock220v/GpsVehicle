//This Code Written by Morteza Abdollahi And it's Open Source!
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
#include <AltSoftSerial.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <TinyGPS++.h>
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
#define SIM800_RX 3 // Pin 2 for SIM800L RX
#define SIM800_TX 4 // Pin 3 for SIM800L TX
#define PASSWORD "123"
#define EEPROM_ADDR_1 0
#define EEPROM_ADDR_2 50
#define EEPROM_ADDR_3 100
#define EEPROM_ADDR_4 150
#define EEPROM_ADDR_5 200
#define EEPROM_ADDR_6 250
#define EEPROM_ADDR_7 300
#define EEPROM_ADDR_8 350
#define EEPROM_ADDR_9 400
#define NUMBER_LENGTH 13
#define MAX_STORED_NUMBERS 3
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
SoftwareSerial sim800(SIM800_TX, SIM800_RX);
AltSoftSerial neoGPS; // RX, TX , 8 , 9
TinyGPSPlus gps;
unsigned long previousMillis = 0;  // Stores the last time the timer was updated
const long interval = 20000;        // Interval in milliseconds (e.g., 1000ms = 1 second)
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void setup() {
  Serial.begin(9600);
  neoGPS.begin(9600);
  sim800.begin(9600);
  delay(20000);
  Serial.println("Setup complete");
  sim800.println("AT"); //Once the handshake test is successful, it will back to OK
  sim800.println("AT+CMGF=1"); // Configuring TEXT mode
  sim800.println("AT+CNMI=1,2,0,0,0"); // Decides how newly arrived SMS messages should be handled
  // Read stored phone numbers from EEPROM
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String storedNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    Serial.print("Stored number ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(storedNumber);
  }
  welcome();
  delay(4000);
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void loop() {
  if (neoGPS.available()) {
    if (updateGPS()) {
      saveLocationToEEPROM(gps.location.lat(), gps.location.lng());
      saveTimeToEEPROM(gps.time.hour() , gps.time.minute() , gps.time.second());
      saveDateToEEPROM(gps.date.month(), gps.date.day(), gps.date.year());
      displayInfo(); // Display GPS information
    }
  }
  while (sim800.available()) {
    String msg = sim800.readString();
    String senderNumber = extractSenderNumber(msg);
    Serial.println("Message received: " + msg);
    Messageprocess(senderNumber, msg);
  }
  delay(1000);
} //end loop
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void Messageprocess(String senderNumber, String msg) {
  // Check if the message contains the password and the sender is not a stored number
  if (msg.indexOf(PASSWORD) != -1) {
    // Extract sender's number from the message
    Serial.println("Sender's number: " + senderNumber);
    // Print sender's phone number
    Serial.println("Sender's phone number: " + senderNumber);
    // Store sender's number in EEPROM if there is space
    if (!isMemoryFull()) {
      storeNumber(senderNumber);
    }
  }
  // Check if the message is to delete the saved number and the sender is a stored number
  if (msg.indexOf("delete") != -1 && isNumberStored(extractSenderNumber(msg))) {
    if (msg.indexOf("all") != -1) {
      deleteAllNumbers();
      Serial.println("All numbers deleted from EEPROM");
    } else {
      if (isNumberStored(senderNumber)) {
        deleteNumber(senderNumber);
        Serial.println("Number " + senderNumber + " deleted from EEPROM");
      }
    }
  }
  // Check if the message is to send location and the sender is a stored number
  if (msg.indexOf("location") != -1 && isNumberStored(extractSenderNumber(msg))) {
    inputgpsforsms();
  }
  if (msg.indexOf("numbers") != -1 && isNumberStored(extractSenderNumber(msg))) {
    sendSaveNumbersViaSMS();
  }
  if (msg.indexOf("whoami") != -1 && isNumberStored(extractSenderNumber(msg))) {
    whoami(senderNumber);
  }
  if (msg.indexOf("help") != -1 && isNumberStored(extractSenderNumber(msg))) {
    welcome();
  }
  // If GPS data is valid, print the location
  delay(2000);
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void whoami(String number) {
  String detail = number;
  sendSMS(number, detail);
  delay(2000);
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void welcome() {
  String numbersList = "Saved Numbers:\n";
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String storedNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    if (storedNumber.length() > 0) {
      numbersList += storedNumber + "\n";
    }
  }
  // Send the numbers list to all saved numbers
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String recipientNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    if (recipientNumber.length() > 0) {
      sendSMS(recipientNumber, "Commands = >> whoami-numbers-delete-deleteall-Password-location");
      delay(4000);
    }
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void sendSaveNumbersViaSMS() {
  String numbersList = "Saved Numbers:\n";
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String storedNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    if (storedNumber.length() > 0) {
      numbersList += storedNumber + "\n";
    }
  }
  // Send the numbers list to all saved numbers
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String recipientNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    if (recipientNumber.length() > 0) {
      sendSMS(recipientNumber, numbersList);
      delay(4000);
    }
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
String extractSenderNumber(String msg) {
  int startPos = msg.indexOf("+");
  int secondPos = msg.indexOf("+", startPos + 1);
  int startPos2 = secondPos;
  int endPos = msg.indexOf("\"", startPos2);
  return msg.substring(startPos2, endPos);
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void sendSMS(String number, String message) {
  sim800.println("AT+CMGF=1"); // Set SMS mode to text
  delay(1000);
  sim800.print("AT+CMGS=\""); // Set recipient's number
  sim800.print(number);
  sim800.println("\"");
  delay(1000);
  sim800.print(message); // Message content
  delay(100);
  sim800.println((char)26); // End AT command with Ctrl+Z
  delay(1000);
  sim800.println();
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
String readNumberFromEEPROM(int addr) {
  String storedNumber;
  for (int i = 0; i < NUMBER_LENGTH; i++) {
    char digit = EEPROM.read(addr + i);
    if (digit != '\0') {
      storedNumber += digit;
    }
  }
  return storedNumber;
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
bool isNumberStored(String number) {
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String storedNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    if (number == storedNumber) {
      return true;
    }
  }
  return false;
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
bool isMemoryFull() {
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String storedNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    if (storedNumber.length() == 0) {
      return false;
    }
  }
  return true;
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void storeNumber(String number) {
  // Check if the number already exists
  if (isNumberStored(number)) {
    Serial.println("Number already stored: " + number);
    sendSMS(number, "Number already stored!");
    delay(1000);
    return;
  }
  else {
    // Find an empty slot to store the number
    for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
      if (readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1)).length() == 0) {
        writeNumberToEEPROM(number, EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
        Serial.println("Number stored: " + number);
        sendSMS(number, "Welcome!");
        delay(1000);
        break;
      }
    }
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void writeNumberToEEPROM(String number, int addr) {
  // Clear stored number in EEPROM
  for (int i = 0; i < NUMBER_LENGTH; i++) {
    EEPROM.write(addr + i, '\0');
  }
  // Store number in EEPROM
  for (int i = 0; i < number.length(); i++) {
    EEPROM.write(addr + i, number[i]);
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void deleteNumber(String number) {
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String storedNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    if (number == storedNumber) {
      for (int j = 0; j < NUMBER_LENGTH; j++) {
        EEPROM.write(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1) + j, '\0');
      }
      break;
    }
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void deleteAllNumbers() {
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    for (int j = 0; j < NUMBER_LENGTH; j++) {
      EEPROM.write(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1) + j, '\0');
    }
  }

}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void sendHiDudeMessage() {
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String storedNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    if (storedNumber.length() > 0) {
      sendSMS(storedNumber, "Hi dude.");
      delay(1000);
    }
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void sendAllLocations() {
  String latitude = readLocationFromEEPROM(EEPROM_ADDR_2);
  String longitude = readLocationFromEEPROM(EEPROM_ADDR_3);
  // Create Google Maps link
  String googleMapsLink = "https://maps.google.com/maps?q=" + latitude + "," + longitude;
  String locationMsg = "Current Location: " + googleMapsLink;
  // Send the location message via SMS to all saved numbers
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String storedNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    if (storedNumber.length() > 0) {
      sendSMS(storedNumber, locationMsg);
      delay(4000);
    }
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
String readLocationFromEEPROM(int addr) {
  String location;
  for (int i = 0; i < NUMBER_LENGTH; i++) {
    char digit = EEPROM.read(addr + i);
    if (digit != '\0') {
      location += digit;
    }
  }
  return location;
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void saveLocationToEEPROM(float latitude, float longitude) {
  // Check if GPS data is valid and not all values are 0
  if (gps.location.isValid()) {
    EEPROM.put(EEPROM_ADDR_2, latitude);
    EEPROM.put(EEPROM_ADDR_3, longitude);
  }
}
void saveTimeToEEPROM(float hour, float minute, float second) {
  // Check if GPS data is valid and not all values are 0
  if (gps.date.isValid()) {
    EEPROM.put(EEPROM_ADDR_4, hour);
    EEPROM.put(EEPROM_ADDR_5, minute);
    EEPROM.put(EEPROM_ADDR_6, second);
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void saveDateToEEPROM(float month, float day, float year) {
  // Check if GPS data is valid and not all values are 0
  if (gps.time.isValid()) {
    EEPROM.put(EEPROM_ADDR_7, month);
    EEPROM.put(EEPROM_ADDR_8, day);
    EEPROM.put(EEPROM_ADDR_9, year);
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void updateLocation() {
  byte gpsData = neoGPS.read();
  gps.encode(neoGPS.read());
  if (gps.encode(neoGPS.read())) {
    displayInfo();
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void displayInfo()
{
  if (gps.location.isValid())
  {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());
  }
  else
  {
    Serial.println("Location: Not Available");
  }

  Serial.print("Date: ");
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.println(gps.date.year());
  }
  else
  {
    Serial.println("Not Available");
  }
  Serial.print("Time: ");
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(":");
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(":");
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(".");
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.println(gps.time.centisecond());
  }
  else
  {
    Serial.println("Not Available");
  }
  Serial.println();
  Serial.println();
  delay(1000);
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
void inputgpsforsms() {
  float latitude;
  float longitude;
  float hour;
  float minute;
  float second;
  float month;
  float day;
  float year;
  String latitude1 = String(EEPROM.get(EEPROM_ADDR_2, latitude), 6);
  String latitude2 = String(EEPROM.get(EEPROM_ADDR_3, longitude), 6);
  String hour1 = String(EEPROM.get(EEPROM_ADDR_4, hour), 0);
  String minute1 = String(EEPROM.get(EEPROM_ADDR_5, minute), 0);
  String second1 = String(EEPROM.get(EEPROM_ADDR_6, second), 0);
  String month1 = String(EEPROM.get(EEPROM_ADDR_7, month), 0);
  String day1 = String(EEPROM.get(EEPROM_ADDR_8, day), 0);
  String year1 = String(EEPROM.get(EEPROM_ADDR_9, year), 0);
  String numbersList = "Saved Numbers:\n";
  // Send the numbers list to all saved numbers
  for (int i = 0; i < MAX_STORED_NUMBERS; i++) {
    String recipientNumber = readNumberFromEEPROM(EEPROM_ADDR_1 + i * (NUMBER_LENGTH + 1));
    if (recipientNumber.length() > 0) {
      String googleMapsLink = "https://maps.google.com/maps?q=" + latitude1 + "," + latitude2;
      sendSMS(recipientNumber, hour1 + ":" + minute1 + ":" + second1 + "-" + month1 + "/" + day1 + "/" + year1 + " "  + googleMapsLink);
      delay(1000);
    }
  }
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
bool updateGPS() {
  while (neoGPS.available()) {
    if (gps.encode(neoGPS.read())) {
      return true; // Valid GPS data received
    }
  }
  return false; // No valid GPS data received
}
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=

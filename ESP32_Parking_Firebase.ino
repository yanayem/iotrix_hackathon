// =====================================================
// SMART PARKING SYSTEM
// TASK 1 + TASK 2 + TASK 3 + TASK 4
//
// ESP32
// IR + RFID + Servo + OLED + MQ Gas + Fire Sensor
// WiFi + Firebase Realtime Database
// =====================================================

#include <WiFi.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

#include <Firebase_ESP_Client.h>


// =====================================================
// WIFI CONFIGURATION
// =====================================================

#define WIFI_SSID       "Cafeteria"
#define WIFI_PASSWORD   "bubt1234"


// =====================================================
// FIREBASE CONFIGURATION
// =====================================================

#define DATABASE_SECRET "O7nB9c0kCv6bASM4h5Dl7AF6VovLVZeIdZgXLQ7G"

#define DATABASE_URL    "https://iotrix-hackathon-d0919-default-rtdb.firebaseio.com/"


// Firebase objects

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;


// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);


// =====================================================
// PARKING IR SENSORS
// =====================================================

const int slotA_sensor = 18;
const int slotB_sensor = 19;


// LOW = vehicle detected

bool slotAOccupied = false;
bool slotBOccupied = false;

bool previousSlotA = false;
bool previousSlotB = false;


// =====================================================
// RFID RC522
// =====================================================

#define RFID_SS    5
#define RFID_RST   27

#define RFID_SCK   14
#define RFID_MISO  12
#define RFID_MOSI  13

MFRC522 rfid(
  RFID_SS,
  RFID_RST
);


// =====================================================
// SERVO
// =====================================================

const int servoPin = 26;

Servo gateServo;

const int GATE_CLOSED = 0;
const int GATE_OPEN = 90;

bool gateOpen = false;

unsigned long gateOpenedAt = 0;

const unsigned long gateOpenTime = 5000;


// =====================================================
// MQ GAS SENSOR
// =====================================================

const int gasSensorPin = 34;


// IMPORTANT:
// First observe your MQ sensor's normal reading.
// Then change this value according to your sensor.
//
// Example only:
const int GAS_DANGER_LEVEL = 1500;


// =====================================================
// FIRE SENSOR
// =====================================================

const int fireSensorPin = 25;


// Most flame modules output LOW when flame detected.
// If yours works opposite, change LOW to HIGH.

const int FIRE_ACTIVE_STATE = LOW;


// =====================================================
// GAS / FIRE STATES
// =====================================================

int gasValue = 0;

bool gasDanger = false;
bool fireDetected = false;

bool previousGasDanger = false;
bool previousFireDetected = false;


// =====================================================
// SENSOR TIMING
// =====================================================

unsigned long lastSensorCheck = 0;

const unsigned long sensorInterval = 100;


// =====================================================
// FIREBASE TIMING
// =====================================================

unsigned long lastFirebaseUpdate = 0;

const unsigned long firebaseInterval = 2000;


// =====================================================
// WIFI STATUS TIMING
// =====================================================

unsigned long lastWiFiCheck = 0;

const unsigned long wifiCheckInterval = 5000;


// =====================================================
// OLED ACCESS MESSAGE
// =====================================================

bool showingAccessMessage = false;

unsigned long accessMessageStarted = 0;

const unsigned long accessMessageTime = 2500;


// =====================================================
// OLED GATE CLOSED MESSAGE
// =====================================================

bool showingGateClosedMessage = false;

unsigned long gateClosedMessageStarted = 0;

const unsigned long gateClosedMessageTime = 2000;


// =====================================================
// OLED SAFETY ALERT
// =====================================================

bool showingSafetyAlert = false;


// =====================================================
// OLED STATE
// =====================================================

int lastOLEDState = -1;


// =====================================================
// VALID RFID CARD
//
// UID = 70:91:A0:55
// =====================================================

byte validUID[] = {

  0x70,
  0x91,
  0xA0,
  0x55
};

const byte validUIDLength = sizeof(validUID);


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);


  // =================================================
  // IR SENSORS
  // =================================================

  pinMode(slotA_sensor, INPUT);
  pinMode(slotB_sensor, INPUT);


  // =================================================
  // GAS SENSOR
  // =================================================

  pinMode(gasSensorPin, INPUT);


  // =================================================
  // FIRE SENSOR
  // =================================================

  pinMode(
    fireSensorPin,
    INPUT
  );


  // =================================================
  // OLED
  // =================================================

  Wire.begin(21, 22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {

    Serial.println(
      "OLED initialization failed!"
    );

    while (true) {
    }
  }


  // =================================================
  // WELCOME SCREEN
  // =================================================

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(2);

  display.setCursor(10, 8);

  display.println("SMART");

  display.setCursor(10, 32);

  display.println("PARKING");

  display.display();

  delay(1500);


  // =================================================
  // SERVO
  // =================================================

  gateServo.attach(
    servoPin
  );

  gateServo.write(
    GATE_CLOSED
  );

  gateOpen = false;


  // =================================================
  // RFID
  // =================================================

  SPI.begin(
    RFID_SCK,
    RFID_MISO,
    RFID_MOSI,
    RFID_SS
  );

  rfid.PCD_Init();

  delay(50);


  // =================================================
  // INITIAL PARKING STATUS
  // =================================================

  slotAOccupied =
    (digitalRead(slotA_sensor) == LOW);

  slotBOccupied =
    (digitalRead(slotB_sensor) == LOW);

  previousSlotA =
    slotAOccupied;

  previousSlotB =
    slotBOccupied;


  // =================================================
  // INITIAL GAS / FIRE STATUS
  // =================================================

  gasValue =
    analogRead(gasSensorPin);

  gasDanger =
    (gasValue >= GAS_DANGER_LEVEL);

  fireDetected =
    (digitalRead(fireSensorPin)
     == FIRE_ACTIVE_STATE);


  previousGasDanger =
    gasDanger;

  previousFireDetected =
    fireDetected;


  // =================================================
  // WIFI
  // =================================================

  Serial.println();
  Serial.println(
    "Connecting to WiFi..."
  );

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  unsigned long wifiStart =
    millis();


  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - wifiStart < 15000
  ) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();


  if (WiFi.status() == WL_CONNECTED) {

    Serial.println(
      "WiFi Connected!"
    );

    Serial.print(
      "IP Address: "
    );

    Serial.println(
      WiFi.localIP()
    );
  }

  else {

    Serial.println(
      "WiFi connection failed!"
    );
  }


  // =================================================
  // FIREBASE
  // =================================================

  config.database_url =
    DATABASE_URL;

  config.signer.tokens.legacy_token =
    DATABASE_SECRET;

  Firebase.begin(
    &config,
    &auth
  );

  Firebase.reconnectWiFi(true);


  // =================================================
  // STARTUP SERIAL
  // =================================================

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "     SMART PARKING SYSTEM"
  );

  Serial.println(
    "================================"
  );

  Serial.println(
    "System Started"
  );

  Serial.println(
    "OLED Ready"
  );

  Serial.println(
    "RFID Reader Ready"
  );

  Serial.println(
    "Gas Sensor Ready"
  );

  Serial.println(
    "Fire Sensor Ready"
  );

  Serial.println(
    "Gate Status: CLOSED"
  );


  printParkingStatus();

  printSafetyStatus();

  updateNormalOLED();


  Serial.println(
    "Waiting for RFID card..."
  );

  Serial.println(
    "================================"
  );
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  unsigned long currentMillis =
    millis();


  // =================================================
  // PARKING + SAFETY SENSOR CHECK
  // =================================================

  if (
    currentMillis -
    lastSensorCheck >=
    sensorInterval
  ) {

    lastSensorCheck =
      currentMillis;


    // ---------------------------------------------
    // PARKING
    // ---------------------------------------------

    slotAOccupied =
      (digitalRead(slotA_sensor)
       == LOW);

    slotBOccupied =
      (digitalRead(slotB_sensor)
       == LOW);


    // ---------------------------------------------
    // GAS
    // ---------------------------------------------

    gasValue =
      analogRead(gasSensorPin);

    gasDanger =
      (gasValue >=
       GAS_DANGER_LEVEL);


    // ---------------------------------------------
    // FIRE
    // ---------------------------------------------

    fireDetected =
      (digitalRead(fireSensorPin)
       == FIRE_ACTIVE_STATE);


    // =================================================
    // PARKING STATE CHANGE
    // =================================================

    if (
      slotAOccupied != previousSlotA ||
      slotBOccupied != previousSlotB
    ) {

      Serial.println();

      Serial.println(
        "PARKING STATUS CHANGED"
      );

      printParkingStatus();


      previousSlotA =
        slotAOccupied;

      previousSlotB =
        slotBOccupied;


      if (
        !showingAccessMessage &&
        !showingGateClosedMessage &&
        !showingSafetyAlert
      ) {

        updateNormalOLED();
      }
    }


    // =================================================
    // SAFETY STATE CHANGE
    // =================================================

    if (
      gasDanger != previousGasDanger ||
      fireDetected != previousFireDetected
    ) {

      Serial.println();

      Serial.println(
        "SAFETY STATUS CHANGED"
      );

      printSafetyStatus();


      previousGasDanger =
        gasDanger;

      previousFireDetected =
        fireDetected;


      updateSafetyOLED();
    }
  }


  // =================================================
  // AUTOMATIC GATE CLOSE
  // =================================================

  if (gateOpen) {

    if (
      currentMillis -
      gateOpenedAt >=
      gateOpenTime
    ) {

      closeGate();
    }
  }


  // =================================================
  // ACCESS MESSAGE TIMEOUT
  // =================================================

  if (showingAccessMessage) {

    if (
      currentMillis -
      accessMessageStarted >=
      accessMessageTime
    ) {

      showingAccessMessage =
        false;


      if (
        !gateOpen &&
        !showingGateClosedMessage &&
        !showingSafetyAlert
      ) {

        lastOLEDState = -1;

        updateNormalOLED();
      }
    }
  }


  // =================================================
  // GATE CLOSED MESSAGE TIMEOUT
  // =================================================

  if (showingGateClosedMessage) {

    if (
      currentMillis -
      gateClosedMessageStarted >=
      gateClosedMessageTime
    ) {

      showingGateClosedMessage =
        false;


      if (!showingSafetyAlert) {

        lastOLEDState = -1;

        updateNormalOLED();
      }
    }
  }


  // =================================================
  // RFID
  // =================================================

  if (
    !gateOpen &&
    !showingAccessMessage &&
    !showingGateClosedMessage
  ) {

    if (
      rfid.PICC_IsNewCardPresent()
    ) {

      if (
        rfid.PICC_ReadCardSerial()
      ) {

        processRFIDCard();


        rfid.PICC_HaltA();

        rfid.PCD_StopCrypto1();
      }
    }
  }


  // =================================================
  // FIREBASE
  // =================================================

  if (
    currentMillis -
    lastFirebaseUpdate >=
    firebaseInterval
  ) {

    lastFirebaseUpdate =
      currentMillis;


    sendDataToFirebase();
  }


  // =================================================
  // WIFI CHECK
  // =================================================

  if (
    currentMillis -
    lastWiFiCheck >=
    wifiCheckInterval
  ) {

    lastWiFiCheck =
      currentMillis;


    if (
      WiFi.status() !=
      WL_CONNECTED
    ) {

      Serial.println(
        "WiFi disconnected. Reconnecting..."
      );

      WiFi.reconnect();
    }
  }
}


// =====================================================
// PRINT PARKING STATUS
// =====================================================

void printParkingStatus() {

  int takenSlots = 0;

  int freeSlots = 0;


  // Slot A

  Serial.print(
    "Slot A: "
  );

  if (slotAOccupied) {

    Serial.println(
      "OCCUPIED"
    );

    takenSlots++;
  }

  else {

    Serial.println(
      "EMPTY"
    );

    freeSlots++;
  }


  // Slot B

  Serial.print(
    "Slot B: "
  );

  if (slotBOccupied) {

    Serial.println(
      "OCCUPIED"
    );

    takenSlots++;
  }

  else {

    Serial.println(
      "EMPTY"
    );

    freeSlots++;
  }


  // Total

  Serial.println(
    "-------------------------------"
  );

  Serial.print(
    "Taken Slots : "
  );

  Serial.println(
    takenSlots
  );

  Serial.print(
    "Free Slots  : "
  );

  Serial.println(
    freeSlots
  );


  Serial.print(
    "Lot Status  : "
  );

  if (freeSlots == 0) {

    Serial.println(
      "FULL"
    );
  }

  else {

    Serial.println(
      "SPACE AVAILABLE"
    );
  }

  Serial.println(
    "-------------------------------"
  );
}


// =====================================================
// PRINT GAS + FIRE STATUS
// =====================================================

void printSafetyStatus() {

  Serial.println(
    "========== SAFETY =========="
  );


  Serial.print(
    "Gas Value: "
  );

  Serial.println(
    gasValue
  );


  Serial.print(
    "Gas Alert: "
  );

  if (gasDanger) {

    Serial.println(
      "DANGER"
    );
  }

  else {

    Serial.println(
      "NORMAL"
    );
  }


  Serial.print(
    "Fire Sensor: "
  );

  if (fireDetected) {

    Serial.println(
      "FIRE DETECTED"
    );
  }

  else {

    Serial.println(
      "NO FIRE"
    );
  }


  Serial.print(
    "Safety Alert: "
  );

  if (
    gasDanger ||
    fireDetected
  ) {

    Serial.println(
      "ACTIVE"
    );
  }

  else {

    Serial.println(
      "NORMAL"
    );
  }


  Serial.println(
    "============================"
  );
}


// =====================================================
// NORMAL OLED
// =====================================================

void updateNormalOLED() {

  int takenSlots = 0;

  int freeSlots = 0;


  if (slotAOccupied) {
    takenSlots++;
  }

  else {
    freeSlots++;
  }


  if (slotBOccupied) {
    takenSlots++;
  }

  else {
    freeSlots++;
  }


  int currentState = 0;


  if (slotAOccupied) {
    currentState += 1;
  }


  if (slotBOccupied) {
    currentState += 2;
  }


  if (currentState ==
      lastOLEDState) {

    return;
  }


  lastOLEDState =
    currentState;


  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  // Title

  display.setTextSize(1);

  display.setCursor(
    0,
    0
  );

  display.println(
    "SMART PARKING"
  );


  display.drawLine(
    0,
    10,
    127,
    10,
    SSD1306_WHITE
  );


  // Slot A

  display.setCursor(
    0,
    18
  );

  display.print(
    "Slot A: "
  );


  if (slotAOccupied) {

    display.println(
      "OCCUPIED"
    );
  }

  else {

    display.println(
      "EMPTY"
    );
  }


  // Slot B

  display.setCursor(
    0,
    30
  );

  display.print(
    "Slot B: "
  );


  if (slotBOccupied) {

    display.println(
      "OCCUPIED"
    );
  }

  else {

    display.println(
      "EMPTY"
    );
  }


  // Free/Taken

  display.setCursor(
    0,
    44
  );

  display.print(
    "Free: "
  );

  display.print(
    freeSlots
  );


  display.print(
    "  Taken: "
  );

  display.print(
    takenSlots
  );


  // Status

  display.setCursor(
    0,
    56
  );


  if (freeSlots == 0) {

    display.print(
      "PARKING FULL"
    );
  }

  else {

    display.print(
      "SPACE AVAILABLE"
    );
  }


  display.display();
}


// =====================================================
// SAFETY OLED
// =====================================================

void updateSafetyOLED() {

  // ---------------------------------------------
  // FIRE OR GAS DANGER
  // ---------------------------------------------

  if (
    gasDanger ||
    fireDetected
  ) {

    showingSafetyAlert =
      true;


    display.clearDisplay();

    display.setTextColor(
      SSD1306_WHITE
    );


    display.setTextSize(2);

    display.setCursor(
      5,
      2
    );

    display.println(
      "WARNING!"
    );


    display.setTextSize(1);


    display.setCursor(
      5,
      25
    );


    if (gasDanger) {

      display.println(
        "GAS DANGER!"
      );
    }


    if (fireDetected) {

      display.setCursor(
        5,
        38
      );

      display.println(
        "FIRE DETECTED!"
      );
    }


    display.setCursor(
      5,
      52
    );

    display.println(
      "CHECK AREA"
    );


    display.display();


    return;
  }


  // ---------------------------------------------
  // RETURN TO NORMAL
  // ---------------------------------------------

  showingSafetyAlert =
    false;


  lastOLEDState = -1;

  updateNormalOLED();
}


// =====================================================
// ACCESS OLED
// =====================================================

void showAccessOLED(
  const char* line1,
  const char* line2,
  const char* line3
) {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  display.setTextSize(2);

  display.setCursor(
    5,
    5
  );

  display.println(
    line1
  );


  display.setTextSize(1);

  display.setCursor(
    5,
    32
  );

  display.println(
    line2
  );


  display.setCursor(
    5,
    47
  );

  display.println(
    line3
  );


  display.display();


  showingAccessMessage =
    true;

  accessMessageStarted =
    millis();
}


// =====================================================
// GATE CLOSED OLED
// =====================================================

void showGateClosedOLED() {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  display.setTextSize(2);

  display.setCursor(
    5,
    8
  );

  display.println(
    "GATE"
  );


  display.setCursor(
    5,
    32
  );

  display.println(
    "CLOSED"
  );


  display.display();


  showingGateClosedMessage =
    true;

  gateClosedMessageStarted =
    millis();
}


// =====================================================
// RFID PROCESS
// =====================================================

void processRFIDCard() {

  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "        RFID CARD SCANNED"
  );

  Serial.println(
    "================================"
  );


  // UID with colon

  Serial.print(
    "Card UID: "
  );


  for (
    byte i = 0;
    i < rfid.uid.size;
    i++
  ) {

    if (
      rfid.uid.uidByte[i] < 0x10
    ) {

      Serial.print(
        "0"
      );
    }


    Serial.print(
      rfid.uid.uidByte[i],
      HEX
    );


    if (
      i <
      rfid.uid.size - 1
    ) {

      Serial.print(
        ":"
      );
    }
  }


  Serial.println();


  // UID without colon

  Serial.print(
    "UID without colon: "
  );


  for (
    byte i = 0;
    i < rfid.uid.size;
    i++
  ) {

    if (
      rfid.uid.uidByte[i] < 0x10
    ) {

      Serial.print(
        "0"
      );
    }


    Serial.print(
      rfid.uid.uidByte[i],
      HEX
    );
  }


  Serial.println();


  // Check card

  bool validCard =
    checkValidUID();


  Serial.print(
    "Card Status: "
  );


  if (validCard) {

    Serial.println(
      "VALID"
    );
  }

  else {

    Serial.println(
      "INVALID"
    );
  }


  // Count slots

  int takenSlots = 0;

  int freeSlots = 0;


  if (slotAOccupied) {
    takenSlots++;
  }

  else {
    freeSlots++;
  }


  if (slotBOccupied) {
    takenSlots++;
  }

  else {
    freeSlots++;
  }


  Serial.print(
    "Taken Slots: "
  );

  Serial.println(
    takenSlots
  );


  Serial.print(
    "Free Slots: "
  );

  Serial.println(
    freeSlots
  );


  // =================================================
  // INVALID CARD
  // =================================================

  if (!validCard) {

    Serial.println(
      "-------------------------------"
    );

    Serial.println(
      "ACCESS DENIED"
    );

    Serial.println(
      "Reason: INVALID RFID CARD"
    );

    Serial.println(
      "Gate: CLOSED"
    );


    showAccessOLED(
      "DENIED",
      "Invalid Card",
      "Gate Closed"
    );
  }


  // =================================================
  // PARKING FULL
  // =================================================

  else if (freeSlots == 0) {

    Serial.println(
      "-------------------------------"
    );

    Serial.println(
      "ACCESS DENIED"
    );

    Serial.println(
      "Reason: PARKING LOT FULL"
    );

    Serial.println(
      "Valid card detected."
    );

    Serial.println(
      "Gate remains CLOSED."
    );


    showAccessOLED(
      "DENIED",
      "Parking Full",
      "Gate Closed"
    );
  }


  // =================================================
  // ACCESS GRANTED
  // =================================================

  else {

    Serial.println(
      "-------------------------------"
    );

    Serial.println(
      "ACCESS GRANTED"
    );

    Serial.println(
      "Parking space available."
    );

    Serial.println(
      "Gate: OPENING"
    );


    showAccessOLED(
      "GRANTED",
      "Welcome!",
      "Gate Opening"
    );


    openGate();
  }


  Serial.println(
    "================================"
  );
}


// =====================================================
// CHECK RFID UID
// =====================================================

bool checkValidUID() {

  if (
    rfid.uid.size !=
    validUIDLength
  ) {

    return false;
  }


  for (
    byte i = 0;
    i < validUIDLength;
    i++
  ) {

    if (
      rfid.uid.uidByte[i]
      != validUID[i]
    ) {

      return false;
    }
  }


  return true;
}


// =====================================================
// OPEN GATE
// =====================================================

void openGate() {

  gateServo.write(
    GATE_OPEN
  );

  gateOpen = true;

  gateOpenedAt =
    millis();


  Serial.println(
    "Servo: 90 degrees"
  );

  Serial.println(
    "GATE OPEN"
  );

  Serial.println(
    "Auto close: 5 seconds"
  );
}


// =====================================================
// CLOSE GATE
// =====================================================

void closeGate() {

  gateServo.write(
    GATE_CLOSED
  );

  gateOpen = false;


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "GATE AUTOMATICALLY CLOSED"
  );

  Serial.println(
    "Servo: 0 degrees"
  );

  Serial.println(
    "================================"
  );


  if (!showingSafetyAlert) {

    showGateClosedOLED();
  }
}


// =====================================================
// SEND ALL DATA TO FIREBASE
// =====================================================

void sendDataToFirebase() {

  // Don't attempt if WiFi unavailable

  if (
    WiFi.status() !=
    WL_CONNECTED
  ) {

    Serial.println(
      "Firebase skipped: WiFi offline"
    );

    return;
  }


  if (!Firebase.ready()) {

    Serial.println(
      "Firebase not ready"
    );

    return;
  }


  // =================================================
  // PARKING COUNTS
  // =================================================

  int takenSlots = 0;

  int freeSlots = 0;


  if (slotAOccupied) {
    takenSlots++;
  }

  else {
    freeSlots++;
  }


  if (slotBOccupied) {
    takenSlots++;
  }

  else {
    freeSlots++;
  }


  // =================================================
  // PARKING DATA
  // =================================================

  Firebase.RTDB.setString(
    &fbdo,
    "/parkingSystem/parking/slotA/status",
    slotAOccupied
      ? "OCCUPIED"
      : "EMPTY"
  );


  Firebase.RTDB.setString(
    &fbdo,
    "/parkingSystem/parking/slotB/status",
    slotBOccupied
      ? "OCCUPIED"
      : "EMPTY"
  );


  Firebase.RTDB.setInt(
    &fbdo,
    "/parkingSystem/parking/freeSlots",
    freeSlots
  );


  Firebase.RTDB.setInt(
    &fbdo,
    "/parkingSystem/parking/takenSlots",
    takenSlots
  );


  // =================================================
  // GAS DATA
  // =================================================

  Firebase.RTDB.setInt(
    &fbdo,
    "/parkingSystem/safety/gas/value",
    gasValue
  );


  Firebase.RTDB.setInt(
    &fbdo,
    "/parkingSystem/safety/gas/dangerLevel",
    GAS_DANGER_LEVEL
  );


  Firebase.RTDB.setBool(
    &fbdo,
    "/parkingSystem/safety/gas/alert",
    gasDanger
  );


  // =================================================
  // FIRE DATA
  // =================================================

  Firebase.RTDB.setBool(
    &fbdo,
    "/parkingSystem/safety/fire/detected",
    fireDetected
  );


  Firebase.RTDB.setBool(
    &fbdo,
    "/parkingSystem/safety/fire/alert",
    fireDetected
  );


  // =================================================
  // OVERALL SAFETY ALERT
  // =================================================

  Firebase.RTDB.setBool(
    &fbdo,
    "/parkingSystem/safety/overallAlert",
    gasDanger || fireDetected
  );


  // =================================================
  // GATE
  // =================================================

  Firebase.RTDB.setString(
    &fbdo,
    "/parkingSystem/gate/status",
    gateOpen
      ? "OPEN"
      : "CLOSED"
  );


  Firebase.RTDB.setInt(
    &fbdo,
    "/parkingSystem/gate/angle",
    gateOpen
      ? GATE_OPEN
      : GATE_CLOSED
  );


  // =================================================
  // WIFI
  // =================================================

  Firebase.RTDB.setString(
    &fbdo,
    "/parkingSystem/system/wifi",
    WiFi.status() ==
      WL_CONNECTED
      ? "CONNECTED"
      : "DISCONNECTED"
  );


  // =================================================
  // LAST UPDATE
  // =================================================

  Firebase.RTDB.setInt(
    &fbdo,
    "/parkingSystem/system/lastUpdate",
    millis()
  );


  Serial.println(
    "Firebase: Data updated"
  );
}

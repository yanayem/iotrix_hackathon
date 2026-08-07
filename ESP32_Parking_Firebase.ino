#include <WiFi.h>
#include <FirebaseESP32.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// =====================================================
// WIFI & FIREBASE CONFIG
// =====================================================
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define FIREBASE_HOST "your-project-id.firebaseio.com"
#define FIREBASE_AUTH "YOUR_FIREBASE_SECRET"

FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// =====================================================
// PIN DEFINITIONS
// =====================================================
const int slotA_sensor = 18;
const int slotB_sensor = 19;
const int gas_sensor = 34; // Gas sensor pin
#define RFID_SS    5
#define RFID_RST   27
#define RFID_SCK   14
#define RFID_MISO  12
#define RFID_MOSI  13

MFRC522 rfid(RFID_SS, RFID_RST);
const int servoPin = 26;
Servo gateServo;
const int GATE_CLOSED = 0;
const int GATE_OPEN   = 90;

// =====================================================
// VARIABLES
// =====================================================
bool slotAOccupied = false;
bool slotBOccupied = false;
bool gateOpen = false;
bool gasAlert = false;
unsigned long gateOpenedAt = 0;
const unsigned long gateOpenTime = 5000;
unsigned long lastSensorCheck = 0;
const unsigned long sensorInterval = 500;

byte validUID[] = {0x70, 0x91, 0xA0, 0x55};

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);

  // WiFi Setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected to WiFi");

  // Firebase Setup
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  pinMode(slotA_sensor, INPUT);
  pinMode(slotB_sensor, INPUT);
  pinMode(gas_sensor, INPUT);
  gateServo.attach(servoPin);
  gateServo.write(GATE_CLOSED);

  SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_SS);
  rfid.PCD_Init();

  Serial.println("System Ready");
  updateFirebase();
}

void loop() {
  unsigned long currentMillis = millis();

  // Sensor Check (Slots and Gas)
  if (currentMillis - lastSensorCheck >= sensorInterval) {
    lastSensorCheck = currentMillis;
    bool currentA = (digitalRead(slotA_sensor) == LOW);
    bool currentB = (digitalRead(slotB_sensor) == LOW);

    // Gas Detection logic (threshold 400)
    int gasValue = analogRead(gas_sensor);
    bool currentGasAlert = (gasValue > 400);

    if (currentA != slotAOccupied || currentB != slotBOccupied || currentGasAlert != gasAlert) {
      slotAOccupied = currentA;
      slotBOccupied = currentB;
      gasAlert = currentGasAlert;
      updateFirebase();
      printParkingStatus();
    }
  }

  // Automatic Gate Close
  if (gateOpen && (currentMillis - gateOpenedAt >= gateOpenTime)) {
    closeGate();
    updateFirebase();
  }

  // RFID Check
  if (!gateOpen && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    processRFIDCard();
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

void updateFirebase() {
  Firebase.setBool(firebaseData, "/parking/slotA", slotAOccupied);
  Firebase.setBool(firebaseData, "/parking/slotB", slotBOccupied);
  Firebase.setBool(firebaseData, "/parking/gateOpen", gateOpen);
  Firebase.setBool(firebaseData, "/parking/gasAlert", gasAlert);
  Firebase.setInt(firebaseData, "/parking/freeSlots", (slotAOccupied ? 0 : 1) + (slotBOccupied ? 0 : 1));
}

void openGate() {
  gateServo.write(GATE_OPEN);
  gateOpen = true;
  gateOpenedAt = millis();
}

void closeGate() {
  gateServo.write(GATE_CLOSED);
  gateOpen = false;
}

void processRFIDCard() {
  String uidStr = "";
  bool validCard = true;
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(rfid.uid.uidByte[i], HEX);
    if (rfid.uid.uidByte[i] != validUID[i]) validCard = false;
  }
  uidStr.toUpperCase();

  int freeSlots = (slotAOccupied ? 0 : 1) + (slotBOccupied ? 0 : 1);

  if (validCard && freeSlots > 0) {
    openGate();
    updateFirebase();
    logToFirebase(uidStr, "allowed");

    // Notification Data
    FirebaseJson json;
    json.add("cardID", uidStr);
    json.add("timestamp", (double)millis());
    Firebase.set(firebaseData, "/parking/recentEntry", json);

  } else {
    logToFirebase(uidStr, !validCard ? "denied" : "lot full");
  }
}

void logToFirebase(String cardID, String status) {
  FirebaseJson json;
  json.add("cardID", cardID);
  json.add("status", status);
  json.add("time", "12:00 PM"); // Ideally use NTP for real time

  Firebase.push(firebaseData, "/parking/logs", json);
}

void printParkingStatus() {
  Serial.print("A: "); Serial.print(slotAOccupied ? "OCC" : "EMP");
  Serial.print(" | B: "); Serial.println(slotBOccupied ? "OCC" : "EMP");
}

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// =====================================================
// WIFI & FIREBASE CONFIG
// =====================================================
#define WIFI_SSID "Cafeteria"
#define WIFI_PASSWORD "bubt1234"
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

    if (currentA != slotAOccupied || currentB != slotBOccupied ||
currentGasAlert != gasAlert) {
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
  Firebase.setInt(firebaseData, "/parking/freeSlots", (slotAOccupied ?
0 : 1) + (slotBOccupied ? 0 : 1));
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

On Fri, Aug 7, 2026 at 3:26 PM Yeasin Arafat Nayem Bhuiyan
<arafatnayem01@gmail.com> wrote:
>
> <!DOCTYPE html>
> <html lang="en">
> <head>
>     <meta charset="UTF-8">
>     <meta name="viewport" content="width=device-width, initial-scale=1.0">
>     <title>Smart Parking Dashboard</title>
>     <script src="https://cdn.tailwindcss.com"></script>
>     <script src="https://www.gstatic.com/firebasejs/9.6.1/firebase-app-compat.js"></script>
>     <script src="https://www.gstatic.com/firebasejs/9.6.1/firebase-database-compat.js"></script>
>     <style>
>         .fade-in { animation: fadeIn 0.5s ease-in; }
>         @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
>     </style>
> </head>
> <body class="bg-gray-900 text-white font-sans">
>     <!-- Gas Alert Banner -->
>     <div id="gasBanner" class="hidden bg-red-600 text-white
> text-center py-3 font-bold text-xl animate-pulse">
>         ⚠️ GAS LEAK DETECTED! PLEASE EVACUATE! ⚠️
>     </div>
>
>     <div class="min-h-screen flex flex-col items-center p-6">
>         <h1 class="text-4xl font-bold my-8 text-blue-400">Smart
> Parking System Dashboard</h1>
>
>         <!-- Slot Status Grid -->
>         <div class="grid grid-cols-1 md:grid-cols-2 gap-8 w-full max-w-4xl">
>             <!-- Slot A -->
>             <div id="cardA" class="bg-gray-800 p-8 rounded-2xl
> shadow-lg border-t-4 border-green-500 transition-all duration-500">
>                 <h2 class="text-2xl font-semibold mb-4
> text-center">Parking Slot A</h2>
>                 <div id="statusA" class="text-5xl font-black
> text-center text-green-500">EMPTY</div>
>             </div>
>
>             <!-- Slot B -->
>             <div id="cardB" class="bg-gray-800 p-8 rounded-2xl
> shadow-lg border-t-4 border-green-500 transition-all duration-500">
>                 <h2 class="text-2xl font-semibold mb-4
> text-center">Parking Slot B</h2>
>                 <div id="statusB" class="text-5xl font-black
> text-center text-green-500">EMPTY</div>
>             </div>
>         </div>
>
>         <!-- Live Info Cards -->
>         <div class="mt-8 grid grid-cols-1 md:grid-cols-3 gap-4 w-full
> max-w-4xl">
>             <div class="bg-gray-800 p-6 rounded-xl text-center shadow-md">
>                 <p class="text-gray-400 uppercase text-sm font-bold
> tracking-wider">Free Slots</p>
>                 <p id="freeSlots" class="text-4xl font-bold text-blue-400">2</p>
>                 <p class="text-xs text-gray-500 mt-1">out of 2 total</p>
>             </div>
>             <div class="bg-gray-800 p-6 rounded-xl text-center shadow-md">
>                 <p class="text-gray-400 uppercase text-sm font-bold
> tracking-wider">Gate Status</p>
>                 <p id="gateStatus" class="text-4xl font-bold
> text-red-500">CLOSED</p>
>             </div>
>             <div class="bg-gray-800 p-6 rounded-xl text-center shadow-md">
>                 <p class="text-gray-400 uppercase text-sm font-bold
> tracking-wider">System Status</p>
>                 <p id="systemStatus" class="text-2xl font-bold
> text-green-400">ONLINE</p>
>             </div>
>         </div>
>
>         <!-- Live Activity Log -->
>         <div class="mt-12 w-full max-w-4xl">
>             <h3 class="text-2xl font-bold mb-4 text-blue-300">Live
> Entry Logs</h3>
>             <div class="bg-gray-800 rounded-2xl shadow-xl overflow-hidden">
>                 <table class="w-full text-left">
>                     <thead class="bg-gray-700">
>                         <tr>
>                             <th class="p-4 font-semibold
> text-gray-300">Time</th>
>                             <th class="p-4 font-semibold
> text-gray-300">Card ID</th>
>                             <th class="p-4 font-semibold
> text-gray-300">Status</th>
>                         </tr>
>                     </thead>
>                     <tbody id="logTableBody">
>                         <!-- Logs will be injected here -->
>                     </tbody>
>                 </table>
>             </div>
>         </div>
>     </div>
>
>     <!-- Notification Toast -->
>     <div id="notification" class="fixed bottom-10 right-10 transform
> translate-y-20 transition-transform duration-500 bg-blue-600
> text-white p-4 rounded-lg shadow-2xl flex items-center space-x-3
> pointer-events-none opacity-0">
>         <div class="bg-white text-blue-600 rounded-full p-2">
>             <svg xmlns="http://www.w3.org/2000/svg" class="h-6 w-6"
> fill="none" viewBox="0 0 24 24" stroke="currentColor">
>                 <path stroke-linecap="round" stroke-linejoin="round"
> stroke-width="2" d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0
> 0118 0z" />
>             </svg>
>         </div>
>         <div>
>             <p class="font-bold">Entry Notification</p>
>             <p id="notifText" class="text-sm">Car entered the parking lot.</p>
>         </div>
>     </div>
>
>     <script>
>         // --- FIREBASE CONFIGURATION ---
>         // Replace with your actual Firebase config
>         const firebaseConfig = {
>             apiKey: "YOUR_API_KEY",
>             authDomain: "your-project-id.firebaseapp.com",
>             databaseURL: "https://your-project-id.firebaseio.com",
>             projectId: "your-project-id",
>             storageBucket: "your-project-id.appspot.com",
>             messagingSenderId: "YOUR_SENDER_ID",
>             appId: "YOUR_APP_ID"
>         };
>
>         // Initialize Firebase
>         firebase.initializeApp(firebaseConfig);
>         const db = firebase.database();
>
>         // Variables for Notification tracking
>         let lastEntryTime = 0;
>
>         // Listen for Real-time updates
>         db.ref('parking').on('value', (snapshot) => {
>             const data = snapshot.val();
>             if (data) {
>                 // 1. Update Slots
>                 updateSlot('statusA', 'cardA', data.slotA);
>                 updateSlot('statusB', 'cardB', data.slotB);
>
>                 // 2. Update Gate Status
>                 const gate = document.getElementById('gateStatus');
>                 gate.innerText = data.gateOpen ? "OPEN" : "CLOSED";
>                 gate.className = `text-4xl font-bold ${data.gateOpen ?
> 'text-green-500' : 'text-red-500'}`;
>
>                 // 3. Update Free Slots Count
>                 document.getElementById('freeSlots').innerText = data.freeSlots;
>
>                 // 4. Gas Alert Warning Banner
>                 const gasBanner = document.getElementById('gasBanner');
>                 if (data.gasAlert) {
>                     gasBanner.classList.remove('hidden');
>                 } else {
>                     gasBanner.classList.add('hidden');
>                 }
>
>                 // 5. Live Notification for RFID Entry
>                 if (data.recentEntry && data.recentEntry.timestamp >
> lastEntryTime) {
>                     showNotification(`Car (ID:
> ${data.recentEntry.cardID}) entered the lot!`);
>                     lastEntryTime = data.recentEntry.timestamp;
>                 }
>
>                 // 6. Update Activity Logs Table
>                 if (data.logs) {
>                     const logBody = document.getElementById('logTableBody');
>                     logBody.innerHTML = '';
>
>                     // Convert logs object to array and sort by time
> (assuming time is a string or number)
>                     // For better sorting, use a timestamp if available
>                     const logsArray =
> Object.values(data.logs).reverse().slice(0, 10); // Show last 10
>
>                     logsArray.forEach(log => {
>                         const row = document.createElement('tr');
>                         row.className = "border-t border-gray-700
> hover:bg-gray-750 transition-colors fade-in";
>                         row.innerHTML = `
>                             <td class="p-4 text-gray-400">${log.time}</td>
>                             <td class="p-4 font-mono">${log.cardID}</td>
>                             <td class="p-4">
>                                 <span class="px-2 py-1 rounded text-xs
> font-bold ${log.status === 'allowed' ? 'bg-green-900 text-green-300' :
> 'bg-red-900 text-red-300'}">
>                                     ${log.status.toUpperCase()}
>                                 </span>
>                             </td>
>                         `;
>                         logBody.appendChild(row);
>                     });
>                 }
>             }
>         });
>
>         function updateSlot(statusId, cardId, isOccupied) {
>             const status = document.getElementById(statusId);
>             const card = document.getElementById(cardId);
>
>             if (isOccupied) {
>                 status.innerText = "OCCUPIED";
>                 status.className = "text-5xl font-black text-center
> text-red-500";
>                 card.className = "bg-gray-800 p-8 rounded-2xl
> shadow-lg border-t-4 border-red-500 transition-all duration-500";
>             } else {
>                 status.innerText = "EMPTY";
>                 status.className = "text-5xl font-black text-center
> text-green-500";
>                 card.className = "bg-gray-800 p-8 rounded-2xl
> shadow-lg border-t-4 border-green-500 transition-all duration-500";
>             }
>         }
>
>         function showNotification(message) {
>             const notif = document.getElementById('notification');
>             const text = document.getElementById('notifText');
>
>             text.innerText = message;
>             notif.classList.remove('translate-y-20', 'opacity-0');
>             notif.classList.add('translate-y-0', 'opacity-100');
>
>             setTimeout(() => {
>                 notif.classList.add('translate-y-20', 'opacity-0');
>                 notif.classList.remove('translate-y-0', 'opacity-100');
>             }, 5000);
>         }
>     </script>
> </body>
> </html>

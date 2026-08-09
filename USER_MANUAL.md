# User Manual: Smart Parking System

Welcome to the **Smart Parking System** user manual. This document provides instructions on how to use, set up, and maintain the system.

---

## 1. Introduction
The Smart Parking System is an IoT-based solution designed to manage parking spaces efficiently. It uses an ESP32 microcontroller for real-time monitoring of parking slots and a web-based dashboard for live data visualization and user interaction.

---

## 2. Dashboard Interface Guide
The web dashboard is the primary interface for users and administrators.

### 2.1 Dashboard Overview
*   **Parking Slots (Slot A & Slot B):** Large colored cards indicate the current status of each slot.
    *   **GREEN (EMPTY):** The slot is available for parking.
    *   **RED (OCCUPIED):** The slot is currently taken.
*   **Live Info Cards:**
    *   **Free Slots:** Displays the total number of available parking spaces.
    *   **Gate Status:** Shows if the entry gate is currently **OPEN** or **CLOSED**.
    *   **System Status:** Indicates if the system is **ONLINE** and connected to the database.

### 2.2 Live Notifications
*   When a car enters the parking lot after a successful RFID scan, a notification toast will appear at the bottom right of the screen.

### 2.3 Live Entry Logs
*   The table at the bottom shows a history of recent access attempts.
*   **Time:** The timestamp of the event.
*   **Card ID:** The unique ID of the scanned RFID card.
*   **Status:** Indicates whether access was **ALLOWED** or **DENIED**.

### 2.4 Gas Alert System (Emergency)
*   If the system detects a gas leak, a **flashing red banner** will appear at the top of the dashboard with the message: `⚠️ GAS LEAK DETECTED! PLEASE EVACUATE! ⚠️`.

---

## 3. Administrator & Setup Guide

### 3.1 Software Requirements
*   A modern web browser (Chrome, Firefox, Edge, etc.).
*   Arduino IDE (for flashing the ESP32 firmware).
*   A Firebase project (Realtime Database).

### 3.2 Firebase Configuration
To connect the dashboard to your data:
1.  Open `index.html` in a text editor.
2.  Locate the `firebaseConfig` object (around line 90).
3.  Replace the placeholder values (`YOUR_API_KEY`, `your-project-id`, etc.) with your actual Firebase credentials.

### 3.3 Hardware Setup
*   **Microcontroller:** ESP32.
*   **Sensors:** 
    *   IR/Ultrasonic sensors for Slot A (Pin 18) and Slot B (Pin 19).
    *   Gas Sensor (Pin 34).
    *   RFID-RC522 Module for entry control.
*   **Actuator:** Servo motor (Pin 26) for the gate.
*   **Firmware:** Flash `ESP32_Parking_Firebase.ino` to the ESP32. Ensure you update the `WIFI_SSID`, `WIFI_PASSWORD`, `FIREBASE_HOST`, and `FIREBASE_AUTH` in the code.

---

## 4. Operational Workflow
1.  **Entry:** The driver scans their RFID card at the gate.
2.  **Validation:** The system checks if the card is valid and if there are free slots.
3.  **Gate Operation:** If valid, the gate opens for 5 seconds and then automatically closes.
4.  **Live Update:** The dashboard immediately updates the "Gate Status" and adds an entry to the "Live Entry Logs".
5.  **Parking:** Once the car parks, the corresponding Slot Card on the dashboard turns red.

---

## 5. Troubleshooting
*   **Dashboard shows "OFFLINE":** Check your internet connection and ensure your Firebase database is active.
*   **Slots not updating:** Check the wiring of the sensors on the ESP32 and ensure the ESP32 is connected to Wi-Fi.
*   **Gate not opening:** Ensure the Servo motor has sufficient power and the RFID card UID matches the `validUID` in the firmware.
*   **Gas Alert stuck on:** Check the sensitivity threshold of your gas sensor in the firmware (currently set to 400).

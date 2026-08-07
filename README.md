# Smart Parking System - Web Dashboard

This project is a **Web Application** designed for real-time monitoring of a Smart Parking System.

## Overview

The system uses an ESP32 microcontroller to detect parking occupancy and update a Firebase Realtime Database. This web dashboard provides a live interface for users and administrators to monitor parking slot availability and gate status.

## Features

- **Real-time Monitoring:** Live updates of parking slot occupancy (Slot A and Slot B).
- **Gate Status:** Displays whether the parking gate is open or closed.
- **Availability Tracking:** Shows the count of free slots.
- **Activity Logging:** Displays the last recorded activity.

## Tech Stack

- **Frontend:** HTML5, Tailwind CSS, JavaScript
- **Backend/Database:** Firebase Realtime Database
- **Hardware:** ESP32, IR/Ultrasonic Sensors (for detection)

## Getting Started

1.  **Firebase Setup:**
    - Create a Firebase project.
    - Enable Realtime Database.
    - Copy your Firebase configuration into `index.html`.
2.  **Hardware Setup:**
    - Flash the `ESP32_Parking_Firebase.ino` code to your ESP32.
    - Ensure the database paths in the Arduino code match the web app.
3.  **Run the Web App:**
    - Simply open `index.html` in a web browser.

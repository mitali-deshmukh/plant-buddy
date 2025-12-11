# PlantBuddy – Smart Plant Watering System 🌱💧

PlantBuddy is an Internet-of-Things (IoT) project that monitors the health of an indoor plant and helps you water it at the right time.  
It continuously measures **soil moisture**, **temperature**, and **humidity** using sensors connected to an ESP32 board, sends data to the **Blynk Cloud**, and lets you trigger a water pump from the **Blynk mobile/web dashboard** when the plant is dry.

---

## Project Motivation

Indoor plants improve air quality and reduce stress, but most people rely on guesswork to decide when to water them.  
Soil moisture depends on temperature, humidity, and soil type; this often leads to **over-watering** (root rot) or **under-watering** (stress and slow growth).  
PlantBuddy turns this into a **data-driven** process by sensing the environment, logging it in the cloud, and providing clear, actionable feedback through an app dashboard. :contentReference[oaicite:1]{index=1}

---

## Features

- **Real-time monitoring** of soil moisture, temperature, and humidity.
- **Sensor-to-cloud pipeline** using Wi-Fi and the Blynk IoT platform.
- **Threshold-based alerts** when soil moisture falls below a user-defined level.
- **Remote watering** via a mini water pump controlled from the Blynk app.
- **History charts** and widgets for trends and long-term plant health tracking.
- **Modular design**: sensors, relay, and pump are logically separated for easy upgrades. 

---

## System Architecture

The system follows a **sensor-to-cloud** architecture:

1. A **LilyGO TTGO ESP32** reads:
   - A **capacitive soil moisture sensor** (analog).
   - A **DHT20** temperature & humidity sensor (digital).
2. The ESP32 connects to a **2.4 GHz Wi-Fi** network.
3. Sensor readings are sent to **Blynk Cloud** using the Blynk IoT library.
4. The user views **live values** and **history graphs** on the Blynk **mobile** or **web** dashboard.
5. A **relay module** driven by the ESP32 controls a **mini water pump**.
6. A **Blynk button widget** toggles the pump virtual pin; the ESP32 receives this event and activates the relay for a safe duration.
7. **Blynk Events** generate notifications (e.g., email/SMS) when moisture is low.


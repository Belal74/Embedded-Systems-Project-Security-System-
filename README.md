🚨 Embedded Security Monitoring System using TivaC & ESP8266
This project is a real-time embedded security system built using the TM4C123GH6PM (TivaC) microcontroller. It integrates multiple sensors and communicates with an ESP8266 Wi-Fi module to send alert data to Firebase and a Flutter mobile application. It also captures photos on motion detection using a laptop camera, leveraging BLIP (Bootstrapped Language Image Pretraining) for image captioning.

🛠️ Features
Sensor Integration:

Smoke Sensor – triggers a fire alert with buzzer activation.

Sound Sensor – detects unusual loud sounds.

PIR Sensor – detects human motion.

Data Communication:

TivaC sends alerts via UART to ESP8266.

ESP8266:

Displays alerts on an I2C LCD.

Uploads data to Firebase Realtime Database.

Camera Integration:

When motion is detected, a Python script on a laptop captures an image using the webcam.

The image is uploaded to Google Drive.

A BLIP model generates a caption describing the image.

Caption, timestamp, and image link are pushed to Firebase.

Mobile App (Flutter):

Displays real-time alerts.

Shows image descriptions with date/time and Google Drive links.

Push notifications for critical events.

📦 Technologies Used
Tiva C Series TM4C123GH6PM

ESP8266 NodeMCU

Firebase Realtime Database & Storage

Python (OpenCV, BLIP)

Flutter (Mobile App)

Google Drive API

UART, I2C Communication

FreeRTOS

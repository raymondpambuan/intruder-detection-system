# Intruder Detection and Alert System with RFID Key Using ATmega328P
Arduino code repository for our design project.

During closing time, security breaches in small-to-medium restaurant chains pose serious threats that involve theft, vandalism, and unauthorized entry. This project introduces the design of a low-cost Intruder Detection and Alert System using an ATmega328P microcontroller, RFID authentication, and a multi-sensor network. The system uses a Passive Infrared (PIR) motion sensor, sound sensor, and limit switch to detect unauthorized access through a combinational logic circuit that demands motion detection and either door opening or suspicious sound activity. Once detected, the system alerts locally through a high-decibel buzzer and remotely through SMS using a SIM800L V2 GSM module. RFID-based access control using the PN532 NFC module enables permissioned staff to power the system on or off. 

Credits to [Mamtaz Alam](https://how2electronics.com/interfacing-pn532-nfc-rfid-module-with-arduino/) for pn532_read code.

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <PN532_SWHSU.h>
#include <PN532.h>

// --- CONFIGURATION ---
#define RFID_SIGNAL_PIN 7
#define LED_PIN 4
#define COMBINATION_INPUT_PIN 8
#define SOUND_INPUT_PIN 5
#define SOUND_SIGNAL_PIN 6

#define GSM_RX 2
#define GSM_TX 3
#define RFID_RX 13
#define RFID_TX 12

#define EEPROM_RFID_STATE_ADDR 0  // Store at byte 0

#define DEBUG false

String validTag = "149.158.44.3";
String number = "+639278537713"; // Update with client number

// --- GLOBALS ---
SoftwareSerial SWSerial(RFID_RX, RFID_TX);
SoftwareSerial sim(GSM_RX, GSM_TX);
PN532_SWHSU pn532swhsu(SWSerial);
PN532 nfc(pn532swhsu);

String tagId = "None", dispTag = "None";
byte nuidPICC[4];

String _buffer;

unsigned long soundStartTime = 0;
const unsigned long soundHoldTime = 6000; // Keep sound signal HIGH for XXXX ms
bool soundDetected = false;

// --- SETUP ---
void setup() {
  pinMode(RFID_SIGNAL_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(COMBINATION_INPUT_PIN, INPUT);
  pinMode(SOUND_INPUT_PIN, INPUT);
  pinMode(SOUND_SIGNAL_PIN, OUTPUT);
  
  byte savedState = EEPROM.read(EEPROM_RFID_STATE_ADDR);
  digitalWrite(RFID_SIGNAL_PIN, savedState);
  digitalWrite(LED_PIN, savedState);
  digitalWrite(SOUND_SIGNAL_PIN, LOW);

  if (DEBUG) Serial.begin(9600);
  
}

// --- LOOP ---
void loop() {
  checkSound();
  
  initNFC();
  readNFC();
  initGSM();

  if (tagId == validTag) {
    toggleRFIDState();
  }

  if (digitalRead(COMBINATION_INPUT_PIN) == HIGH) {
    sendSMS("ALERT: Suspicious activity detected at restaurant entrance. Please check immediately.");
    delay(800);
  }

  endGSM();
}

// --- NFC SETUP ---
void initNFC() {
  static int failCount = 0;
  static int recoverCount = 0;
  static bool alarmTriggered = false;

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();

  if (!versiondata) {
    // PN532 not responding
    failCount++;
    recoverCount = 0; // reset recovery attempts

    if (failCount >= 10 && !alarmTriggered) {
      // Trigger failsafe alarm
      digitalWrite(RFID_SIGNAL_PIN, HIGH);
      digitalWrite(LED_PIN, HIGH);
      EEPROM.update(EEPROM_RFID_STATE_ADDR, HIGH);
      sendSMS("ALERT: RFID Module Unresponsive (Possible Power Cut)");
      alarmTriggered = true;
    }

    delay(300);
    return;
  }

  // PN532 responded successfully
  failCount = 0;

  if (alarmTriggered) {
    recoverCount++;
    if (recoverCount >= 3) {
      // Restore state from EEPROM on recovery
      byte savedState = EEPROM.read(EEPROM_RFID_STATE_ADDR);
      digitalWrite(RFID_SIGNAL_PIN, savedState);
      digitalWrite(LED_PIN, savedState);

      // Reset flags and counters
      alarmTriggered = false;
      failCount = 0;
      recoverCount = 0;

      sendSMS("INFO: RFID Module Recovered");
    }
  }

  nfc.SAMConfig();
  delay(50);  // Allow PN532 to stabilize
  tagId = ""; // Clear stale tag reads
}

// --- NFC READ ---
void readNFC() {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success) {
    for (uint8_t i = 0; i < uidLength; i++) {
      nuidPICC[i] = uid[i];
    }
    tagId = tagToString(nuidPICC);
    if (DEBUG) Serial.println("Tag ID: " + tagId);
    delay(200);
  }
}

// --- TOGGLE RFID STATE ---
void toggleRFIDState() {
  int currentState = digitalRead(RFID_SIGNAL_PIN);
  int newState = !currentState;

  digitalWrite(RFID_SIGNAL_PIN, newState);
  digitalWrite(LED_PIN, newState);

  // Store new state in EEPROM
  EEPROM.update(EEPROM_RFID_STATE_ADDR, newState);

  tagId = "";
  delay(800);

  if (DEBUG) Serial.println(newState ? "System ARMED" : "System DISARMED");
}

// --- CHECK SOUND ---
void checkSound() {
  // Check if sound input is LOW => sound detected
  if (digitalRead(SOUND_INPUT_PIN) == LOW) {
    if (!soundDetected) {
      soundDetected = true;                 // Mark sound event
      soundStartTime = millis();            // Record start time
      digitalWrite(SOUND_SIGNAL_PIN, HIGH); // Turn on signal pin
      if (DEBUG) Serial.println("Sound detected, signal ON");
    }
  }

  // If we are holding the signal ON, check timeout
  if (soundDetected) {
    if (millis() - soundStartTime >= soundHoldTime) {
      soundDetected = false;                // Reset
      digitalWrite(SOUND_SIGNAL_PIN, LOW);  // Turn off signal pin
      if (DEBUG) Serial.println("Sound hold time elapsed, signal OFF");
    }
  }
}

// --- GSM INIT ---
void initGSM() {
  _buffer.reserve(50);
  delay(100);
  sim.begin(9600);
}

// --- SEND SMS ---
void sendSMS(String message) {
  sim.println("AT+CMGF=1"); // Set SMS to text mode
  delay(200);

  sim.println("AT+CMGS=\"" + number + "\"\r");
  delay(200);

  sim.print(message);
  delay(100);

  sim.println((char)26);
  delay(200);

  // Wait up to 5 seconds for SIM800L response
  unsigned long startTime = millis();
  while (!sim.available() && millis() - startTime < 5000) {
    checkSound(); // Keep system responsive
  }

  if (sim.available()) {
    _buffer = sim.readString();
    if (DEBUG) Serial.println("SMS Sent: " + message);
  } 
  else {
    if (DEBUG) Serial.println("SMS sending timed out.");
  }
}

void endGSM() {
  sim.end();
}

// --- TAG CONVERSION ---
String tagToString(byte id[4]) {
  String tagId = "";
  for (byte i = 0; i < 4; i++) {
    if (i < 3) tagId += String(id[i]) + ".";
    else tagId += String(id[i]);
  }
  return tagId;
}

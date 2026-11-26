#include "config.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"
#include <TinyGPSPlus.h>

// =====================================================
// SMART HELMET
// MPU6050 + MAX30102 + NEO-6M GPS
// AUTO IMPACT SOS + BUTTON SOS + BUZZER + SIM7600
// =====================================================

// =====================================================
// I2C
// =====================================================

#define SDA_PIN 32
#define SCL_PIN 33

// =====================================================
// SOS BUTTON
// =====================================================

#define SOS_BUTTON 27

// =====================================================
// LED
// =====================================================

#define RED_LED 26
#define GREEN_LED 25

// =====================================================
// BUZZER
// =====================================================

#define BUZZER_PIN 14

// =====================================================
// NEO-6M GPS
//
// GPS TX -> ESP32 GPIO34
// GPS RX -> ESP32 GPIO13
// =====================================================

#define GPS_RX 34
#define GPS_TX 13

HardwareSerial gpsSerial(2);

TinyGPSPlus gps;

// =====================================================
// SIM7600
//
// SIM7600 TX -> ESP32 GPIO16
// SIM7600 RX -> ESP32 GPIO17
// =====================================================

#define MODEM_RX 16
#define MODEM_TX 17

HardwareSerial modem(1);

// =====================================================
// SENSOR OBJECTS
// =====================================================

Adafruit_MPU6050 mpu;

MAX30105 maxSensor;

// =====================================================
// EMERGENCY NUMBER
// =====================================================

String emergencyNumber = EMERGENCY_PHONE_NUMBER;

// =====================================================
// GPS DATA
// =====================================================

double gpsLat = 0.0;
double gpsLon = 0.0;

bool gpsValid = false;

int gpsSatellites = 0;

// =====================================================
// IMPACT SETTINGS
// =====================================================

// Normal helmet at rest is around 1G.
//
// 1.2G = relatively sensitive
// Increase to 1.5 / 2.0 if false alarms occur.

#define IMPACT_THRESHOLD 1.2

// Rotation speed threshold
// Unit: degrees per second

#define ROTATION_THRESHOLD 150.0

// =====================================================
// SOS STATE
// =====================================================

bool sosActive = false;

// =====================================================
// READ GPS
// =====================================================

void readGPS() {

  while (gpsSerial.available()) {

    char c = gpsSerial.read();

    gps.encode(c);
  }

  // -----------------------------
  // LOCATION
  // -----------------------------

  if (gps.location.isValid()) {

    gpsLat = gps.location.lat();

    gpsLon = gps.location.lng();

    gpsValid = true;
  }

  // -----------------------------
  // SATELLITES
  // -----------------------------

  if (gps.satellites.isValid()) {

    gpsSatellites = gps.satellites.value();
  }
}

// =====================================================
// PRINT GPS
// =====================================================

void printGPS() {

  readGPS();

  Serial.println();
  Serial.println("==============================");
  Serial.println("       GPS LOCATION");
  Serial.println("==============================");

  if (gpsValid) {

    Serial.println("FIX: YES");

    Serial.print("SATELLITES: ");
    Serial.println(gpsSatellites);

    Serial.print("LATITUDE:  ");
    Serial.println(gpsLat, 6);

    Serial.print("LONGITUDE: ");
    Serial.println(gpsLon, 6);

    Serial.println();

    Serial.println("Google Maps:");

    Serial.print("https://maps.google.com/?q=");

    Serial.print(gpsLat, 6);

    Serial.print(",");

    Serial.println(gpsLon, 6);
  }

  else {

    Serial.println("FIX: NO");

    Serial.print("SATELLITES: ");
    Serial.println(gpsSatellites);

    Serial.println("Open sky'da GPS fix kuting...");
  }

  Serial.println("==============================");
}

// =====================================================
// READ MODEM
// =====================================================

String readModem(unsigned long timeout = 3000) {

  String response = "";

  unsigned long start = millis();

  while (millis() - start < timeout) {

    while (modem.available()) {

      char c = modem.read();

      response += c;

      Serial.write(c);
    }
  }

  return response;
}

// =====================================================
// SEND AT COMMAND
// =====================================================

String sendAT(
  String command,
  unsigned long timeout = 3000
) {

  Serial.println();

  Serial.print(">>> ");

  Serial.println(command);

  modem.println(command);

  return readModem(timeout);
}

// =====================================================
// SIM7600 INIT
// =====================================================

void initSIM7600() {

  Serial.println();
  Serial.println("==============================");
  Serial.println("       SIM7600 INIT");
  Serial.println("==============================");

  sendAT("AT", 2000);

  sendAT("ATE0", 2000);

  sendAT("AT+CPIN?", 3000);

  sendAT("AT+CPSI?", 3000);

  sendAT("AT+COPS?", 3000);

  sendAT("AT+CMGF=1", 2000);

  Serial.println();

  Serial.println("SIM7600 READY");
}

// =====================================================
// SEND SMS
// =====================================================

bool sendSMS(String message) {

  if (emergencyNumber.length() == 0) {
    Serial.println("SMS disabled: configure EMERGENCY_PHONE_NUMBER in secrets.h");
    return false;
  }

  Serial.println();
  Serial.println("==============================");
  Serial.println("       SENDING SMS");
  Serial.println("==============================");

  // -----------------------------------------------
  // Clear old modem data
  // -----------------------------------------------

  while (modem.available()) {

    modem.read();
  }

  // -----------------------------------------------
  // TEXT MODE
  // -----------------------------------------------

  modem.println("AT+CMGF=1");

  delay(500);

  while (modem.available()) {

    modem.read();
  }

  // -----------------------------------------------
  // SMS CHARACTER SET
  // -----------------------------------------------
  //
  // Use GSM charset.
  // This avoids Unicode/emoji problems.
  //

  modem.println("AT+CSCS=\"GSM\"");

  delay(500);

  while (modem.available()) {

    modem.read();
  }

  // -----------------------------------------------
  // SET RECIPIENT
  // -----------------------------------------------

  Serial.print("Recipient: ");

  Serial.println(emergencyNumber);

  modem.print("AT+CMGS=\"");

  modem.print(emergencyNumber);

  modem.println("\"");

  // -----------------------------------------------
  // WAIT FOR >
  // -----------------------------------------------

  Serial.println("Waiting for SMS prompt...");

  unsigned long start = millis();

  bool prompt = false;

  while (millis() - start < 10000) {

    while (modem.available()) {

      char c = modem.read();

      Serial.write(c);

      if (c == '>') {

        prompt = true;

        break;
      }
    }

    if (prompt) {

      break;
    }
  }

  // -----------------------------------------------
  // PROMPT FAILED
  // -----------------------------------------------

  if (!prompt) {

    Serial.println();
    Serial.println("❌ SMS PROMPT ERROR");

    return false;
  }

  Serial.println();
  Serial.println("✅ PROMPT OK");

  // -----------------------------------------------
  // SEND SMS TEXT
  // -----------------------------------------------

  modem.print(message);

  delay(500);

  // -----------------------------------------------
  // CTRL + Z
  // -----------------------------------------------

  modem.write(26);

  Serial.println();

  Serial.println("Waiting for modem response...");

  // -----------------------------------------------
  // WAIT RESULT
  // -----------------------------------------------

  String response = "";

  start = millis();

  while (millis() - start < 30000) {

    while (modem.available()) {

      char c = modem.read();

      response += c;

      Serial.write(c);
    }

    if (response.indexOf("+CMGS:") >= 0) {

      break;
    }

    if (response.indexOf("+CMS ERROR") >= 0) {

      break;
    }
  }

  Serial.println();

  // -----------------------------------------------
  // SUCCESS
  // -----------------------------------------------

  if (response.indexOf("+CMGS:") >= 0) {

    Serial.println("================================");
    Serial.println("          SMS SENT");
    Serial.println("================================");

    return true;
  }

  // -----------------------------------------------
  // ERROR
  // -----------------------------------------------

  if (response.indexOf("+CMS ERROR") >= 0) {

    Serial.println("================================");
    Serial.println("          SMS FAILED");
    Serial.println("================================");

    return false;
  }

  Serial.println("================================");
  Serial.println("          SMS FAILED");
  Serial.println("================================");

  return false;
}

// =====================================================
// CREATE + SEND EMERGENCY SMS
// =====================================================

void sendEmergencySMS() {

  // Update GPS first
  readGPS();

  // -----------------------------------------------
  // SMS MESSAGE
  // -----------------------------------------------

  String message = "";

  message += "SMART HELMET SOS\n";

  message += "Emergency detected!\n";

  if (gpsValid) {

    message += "\nGPS:\n";

    message += String(gpsLat, 6);

    message += ",";

    message += String(gpsLon, 6);

    message += "\n\n";

    message += "https://maps.google.com/?q=";

    message += String(gpsLat, 6);

    message += ",";

    message += String(gpsLon, 6);
  }

  else {

    message += "\nGPS: NO FIX";
  }

  // -----------------------------------------------
  // PRINT SMS
  // -----------------------------------------------

  Serial.println();

  Serial.println("==============================");

  Serial.println("       SMS CONTENT");

  Serial.println("==============================");

  Serial.println(message);

  Serial.println("==============================");

  // -----------------------------------------------
  // SEND
  // -----------------------------------------------

  sendSMS(message);
}

// =====================================================
// INIT MPU6050
// =====================================================

bool initMPU() {

  Serial.println();
  Serial.println("==============================");
  Serial.println("       MPU6050");
  Serial.println("==============================");

  if (!mpu.begin(0x68, &Wire)) {

    Serial.println("❌ MPU6050 ERROR");

    return false;
  }

  Serial.println("✅ MPU6050 OK");

  mpu.setAccelerometerRange(
    MPU6050_RANGE_8_G
  );

  mpu.setGyroRange(
    MPU6050_RANGE_500_DEG
  );

  mpu.setFilterBandwidth(
    MPU6050_BAND_21_HZ
  );

  return true;
}

// =====================================================
// INIT MAX30102
// =====================================================

bool initMAX30102() {

  Serial.println();
  Serial.println("==============================");
  Serial.println("       MAX30102");
  Serial.println("==============================");

  if (!maxSensor.begin(
    Wire,
    I2C_SPEED_FAST
  )) {

    Serial.println("❌ MAX30102 ERROR");

    return false;
  }

  Serial.println("✅ MAX30102 OK");

  maxSensor.setup(
    0x1F,
    4,
    2,
    100,
    411,
    4096
  );

  maxSensor.setPulseAmplitudeRed(0x1F);

  maxSensor.setPulseAmplitudeIR(0x1F);

  return true;
}

// =====================================================
// CHECK CANCEL BUTTON
// =====================================================

bool cancelPressed() {

  if (digitalRead(SOS_BUTTON) == LOW) {

    delay(50);

    if (digitalRead(SOS_BUTTON) == LOW) {

      return true;
    }
  }

  return false;
}

// =====================================================
// WAIT BUTTON RELEASE
// =====================================================

void waitButtonRelease() {

  while (digitalRead(SOS_BUTTON) == LOW) {

    delay(10);
  }
}

// =====================================================
// CANCEL SOS
// =====================================================

void cancelSOS() {

  Serial.println();

  Serial.println("==============================");

  Serial.println("       SOS CANCELLED");

  Serial.println("==============================");

  digitalWrite(
    RED_LED,
    LOW
  );

  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  sosActive = false;

  // Wait until button is released
  waitButtonRelease();
}

// =====================================================
// SENSOR MONITOR
// =====================================================

void printSensors() {

  sensors_event_t a;

  sensors_event_t g;

  sensors_event_t temp;

  mpu.getEvent(
    &a,
    &g,
    &temp
  );

  // ===================================================
  // ACCELERATION
  // ===================================================

  float accMagnitude = sqrt(

    a.acceleration.x *
    a.acceleration.x +

    a.acceleration.y *
    a.acceleration.y +

    a.acceleration.z *
    a.acceleration.z
  );

  float accG =
    accMagnitude / 9.80665;

  // ===================================================
  // GYROSCOPE
  // ===================================================

  float gyroMagnitude = sqrt(

    g.gyro.x *
    g.gyro.x +

    g.gyro.y *
    g.gyro.y +

    g.gyro.z *
    g.gyro.z
  );

  float gyroDeg =
    gyroMagnitude * 57.2958;

  // ===================================================
  // SERIAL OUTPUT
  // ===================================================

  Serial.println();

  Serial.println("------------------------------");

  Serial.print("ACC: ");

  Serial.print(accG, 2);

  Serial.println(" G");

  Serial.print("GYRO: ");

  Serial.print(gyroDeg, 1);

  Serial.println(" deg/s");

  // ===================================================
  // IMPACT DETECTION
  // ===================================================

  if (
    accG >= IMPACT_THRESHOLD ||
    gyroDeg >= ROTATION_THRESHOLD
  ) {

    Serial.println(
      "⚠️ IMPACT / ROTATION DETECTED"
    );

    Serial.println(
      "AUTOMATIC SOS STARTING..."
    );

    startSOS();
  }

  else {

    Serial.println("NORMAL");
  }
}

// =====================================================
// SOS COUNTDOWN
// =====================================================

void startSOS() {

  // -----------------------------------------------
  // Already active?
  // -----------------------------------------------

  if (sosActive) {

    return;
  }

  sosActive = true;

  Serial.println();

  Serial.println("==============================");

  Serial.println("       SOS COUNTDOWN");

  Serial.println("==============================");

  // ===================================================
  // 10 SECOND COUNTDOWN
  // ===================================================

  for (
    int i = 10;
    i >= 1;
    i--
  ) {

    // -----------------------------------------------
    // CANCEL CHECK
    // -----------------------------------------------

    if (cancelPressed()) {

      cancelSOS();

      return;
    }

    // -----------------------------------------------
    // UPDATE GPS
    // -----------------------------------------------

    readGPS();

    // -----------------------------------------------
    // PRINT COUNTDOWN
    // -----------------------------------------------

    Serial.print("SOS in ");

    Serial.print(i);

    Serial.println(" seconds...");

    // -----------------------------------------------
    // RED LED + BUZZER ON
    // -----------------------------------------------

    digitalWrite(
      RED_LED,
      HIGH
    );

    digitalWrite(
      BUZZER_PIN,
      HIGH
    );

    // -----------------------------------------------
    // 500 ms ON
    // -----------------------------------------------

    for (int j = 0; j < 50; j++) {

      if (cancelPressed()) {

        cancelSOS();

        return;
      }

      readGPS();

      delay(10);
    }

    // -----------------------------------------------
    // RED LED + BUZZER OFF
    // -----------------------------------------------

    digitalWrite(
      RED_LED,
      LOW
    );

    digitalWrite(
      BUZZER_PIN,
      LOW
    );

    // -----------------------------------------------
    // 500 ms OFF
    // -----------------------------------------------

    for (int j = 0; j < 50; j++) {

      if (cancelPressed()) {

        cancelSOS();

        return;
      }

      readGPS();

      delay(10);
    }
  }

  // ===================================================
  // SOS ACTIVATED
  // ===================================================

  Serial.println();

  Serial.println("==============================");

  Serial.println("       🔴 SOS ACTIVATED");

  Serial.println("==============================");

  // -----------------------------------------------
  // BUZZER + RED LED
  // -----------------------------------------------

  digitalWrite(
    RED_LED,
    HIGH
  );

  digitalWrite(
    BUZZER_PIN,
    HIGH
  );

  delay(2000);

  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  digitalWrite(
    RED_LED,
    LOW
  );

  // -----------------------------------------------
  // GPS
  // -----------------------------------------------

  printGPS();

  // -----------------------------------------------
  // SEND SMS
  // -----------------------------------------------

  sendEmergencySMS();

  // -----------------------------------------------
  // FINISH
  // -----------------------------------------------

  sosActive = false;
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(2000);

  // ===================================================
  // PINS
  // ===================================================

  pinMode(
    SOS_BUTTON,
    INPUT_PULLUP
  );

  pinMode(
    RED_LED,
    OUTPUT
  );

  pinMode(
    GREEN_LED,
    OUTPUT
  );

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  digitalWrite(
    RED_LED,
    LOW
  );

  digitalWrite(
    GREEN_LED,
    LOW
  );

  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  // ===================================================
  // I2C
  // ===================================================

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  // ===================================================
  // HEADER
  // ===================================================

  Serial.println();

  Serial.println("================================");

  Serial.println("       SMART HELMET");

  Serial.println("================================");

  // ===================================================
  // MPU6050
  // ===================================================

  initMPU();

  // ===================================================
  // MAX30102
  // ===================================================

  initMAX30102();

  // ===================================================
  // GPS
  // ===================================================

  gpsSerial.begin(

    9600,

    SERIAL_8N1,

    GPS_RX,

    GPS_TX
  );

  Serial.println();

  Serial.println("==============================");

  Serial.println("       NEO-6M GPS");

  Serial.println("==============================");

  Serial.println(
    "GPS TX -> GPIO34"
  );

  Serial.println(
    "GPS RX -> GPIO13"
  );

  Serial.println(
    "Waiting for GPS fix..."
  );

  // ===================================================
  // SIM7600
  // ===================================================

  modem.begin(

    115200,

    SERIAL_8N1,

    MODEM_RX,

    MODEM_TX
  );

  delay(1000);

  initSIM7600();

  // ===================================================
  // SYSTEM READY
  // ===================================================

  digitalWrite(
    GREEN_LED,
    HIGH
  );

  Serial.println();

  Serial.println("================================");

  Serial.println("       SYSTEM READY");

  Serial.println("================================");

  Serial.println(
    "GPS SOURCE: NEO-6M"
  );

  Serial.println(
    "GPS: SEARCHING..."
  );

  Serial.println(
    "SIM7600: READY"
  );

  Serial.println(
    "MPU6050: READY"
  );

  Serial.println(
    "MAX30102: READY"
  );

  Serial.println(
    "BUZZER: READY"
  );

  Serial.println(
    "AUTO IMPACT SOS: READY"
  );

  Serial.println(
    "BUTTON SOS: READY"
  );

  Serial.println(
    "BUTTON CANCEL: READY"
  );

  Serial.println();
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // GPS
  // ===================================================

  readGPS();

  // ===================================================
  // MANUAL SOS BUTTON
  // ===================================================

  if (
    digitalRead(SOS_BUTTON) == LOW
  ) {

    delay(50);

    if (
      digitalRead(SOS_BUTTON) == LOW
    ) {

      startSOS();

      // Wait button release
      waitButtonRelease();
    }
  }

  // ===================================================
  // SENSOR MONITOR
  // ===================================================

  static unsigned long lastSensor = 0;

  if (
    millis() - lastSensor > 1000
  ) {

    lastSensor = millis();

    printSensors();
  }

  // ===================================================
  // GPS STATUS EVERY 5 SEC
  // ===================================================

  static unsigned long lastGPS = 0;

  if (
    millis() - lastGPS > 5000
  ) {

    lastGPS = millis();

    printGPS();
  }

  delay(10);
}
/*
 * ╔══════════════════════════════════════════════════════════════════╗
 * ║        IoT DISTRESS DETECTION SYSTEM                             ║
 * ║  Sensors  : MAX30102 (HR + SpO2) · DS18B20 (Temperature)         ║
 * ║  Alerts   : Firebase RTDB + Twilio SMS (User + Doctor)           ║
 * ║  Hardware : ESP32                                                ║
 * ╚══════════════════════════════════════════════════════════════════╝
 */

// ================================================================
//  INCLUDES
// ================================================================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// ================================================================
//  CREDENTIALS
//  Copy secrets.h.example -> secrets.h and fill in your own values.
//  secrets.h is git-ignored and must never be committed.
// ================================================================
#include "secrets.h"

// ================================================================
//  PIN DEFINITIONS
// ================================================================
#define ONE_WIRE_BUS  4    // DS18B20 data pin
#define BUTTON_PIN   13    // Panic button (active LOW, internal pull-up)

// ================================================================
//  DISTRESS THRESHOLDS
// ================================================================
#define HR_HIGH       120   // bpm  above = distress
#define HR_LOW         40   // bpm  below = distress
#define SPO2_LOW       90   // %    below = distress
#define TEMP_HIGH      39.0 // °C   above = distress
#define PRE_ALERT_MS 20000  // ms before PRE_ALERT → ALERT (20 sec)
#define IR_FINGER_MIN 8000  // IR threshold for finger-on-sensor detection

// ================================================================
//  FIREBASE OBJECTS
// ================================================================
FirebaseData   fbdo;
FirebaseAuth   auth;
FirebaseConfig firebaseConfig;

// ================================================================
//  SENSOR OBJECTS
// ================================================================
MAX30105 particleSensor;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

// MAX30102 sample buffers
uint32_t irBuffer[100];
uint32_t redBuffer[100];

// MAX30102 algorithm outputs
int32_t  spo2Value      = 0;
int8_t   validSPO2      = 0;
int32_t  heartRateValue = 0;
int8_t   validHeartRate = 0;

// Last known temperature
float lastTemperature = 0.0;

// ================================================================
//  STATE MACHINE
// ================================================================
String state = "NORMAL";
unsigned long distressStartTime = 0;

// ================================================================
//  SMS ONE-SHOT FLAG
// ================================================================
bool smsSent = false;

// ================================================================
//  PANIC BUTTON — INTERRUPT FLAG
//  volatile: modified inside ISR, read in main loop
// ================================================================
volatile bool panicTriggered = false;

void IRAM_ATTR onPanicButton() {
  panicTriggered = true;
}

// ================================================================
//  HELPER — URL ENCODER
// ================================================================
String urlEncode(String str) {
  String encoded = "";
  for (int i = 0; i < (int)str.length(); i++) {
    char c = str[i];
    if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      char buf[4];
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

// ================================================================
//  HELPER — SEND ONE SMS VIA TWILIO REST API
// ================================================================
bool sendTwilioSMS(const char* toNumber, String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SMS] WiFi not connected — skipping.");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = "https://api.twilio.com/2010-04-01/Accounts/";
  url += TWILIO_ACCOUNT_SID;
  url += "/Messages.json";

  http.begin(client, url);
  http.setAuthorization(TWILIO_ACCOUNT_SID, TWILIO_AUTH_TOKEN);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String payload  = "To="   + urlEncode(String(toNumber));
  payload        += "&From=" + urlEncode(String(TWILIO_FROM_NUMBER));
  payload        += "&Body=" + urlEncode(message);

  Serial.println("[SMS] POST to Twilio → " + String(toNumber));

  int httpCode = http.POST(payload);
  Serial.printf("[SMS] Response code: %d\n", httpCode);

  if (httpCode != 201) {
    Serial.println("[SMS] Body: " + http.getString());
  }

  http.end();
  return (httpCode == 201);
}

// ================================================================
//  HELPER — SEND DISTRESS ALERT SMS (auto state machine)
// ================================================================
void sendAlertToAll(int hr, int sp, float temp) {
  String msg  = "*** DISTRESS ALERT *** ";
  msg        += "Patient vitals abnormal! ";
  msg        += "HR: "   + String(hr)      + " bpm | ";
  msg        += "SpO2: " + String(sp)      + "% | ";
  msg        += "Temp: " + String(temp, 1) + " C. ";
  msg        += "Please respond immediately.";

  Serial.println("[SMS] Sending distress alert to User and Doctor...");
  bool u = sendTwilioSMS(TWILIO_TO_USER,   msg);
  bool d = sendTwilioSMS(TWILIO_TO_DOCTOR, msg);

  if (u && d) {
    Serial.println("[SMS] Both delivered. smsSent locked.");
    smsSent = true;
  } else {
    Serial.println("[SMS] Partial/full failure — will retry next cycle.");
  }
}

// ================================================================
//  HELPER — SEND PANIC SMS (manual button, always fires immediately)
// ================================================================
void sendPanicSMS(int hr, int sp, float temp) {
  String msg  = "*** PANIC BUTTON PRESSED *** ";
  msg        += "Patient manually triggered alert! ";
  msg        += "HR: "   + String(hr)      + " bpm | ";
  msg        += "SpO2: " + String(sp)      + "% | ";
  msg        += "Temp: " + String(temp, 1) + " C. ";
  msg        += "Please respond immediately.";

  Serial.println("[PANIC] Sending panic SMS to User and Doctor...");
  bool u = sendTwilioSMS(TWILIO_TO_USER,   msg);
  bool d = sendTwilioSMS(TWILIO_TO_DOCTOR, msg);

  if (u && d) {
    Serial.println("[PANIC] Both delivered.");
  } else {
    Serial.println("[PANIC] Partial/full failure.");
  }
}

// ================================================================
//  HELPER — DISTRESS CONDITION CHECK
// ================================================================
bool isDistress(int hr, int sp, float temp) {
  return (hr > HR_HIGH || hr < HR_LOW || sp < SPO2_LOW || temp > TEMP_HIGH);
}

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========== DISTRESS DETECTION SYSTEM BOOT ==========");

  // --- Panic Button with Interrupt ---
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onPanicButton, FALLING);
  Serial.println("Panic button interrupt attached.");

  // --- WiFi ---
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  // --- Firebase ---
  firebaseConfig.api_key      = FIREBASE_API_KEY;
  firebaseConfig.database_url = FIREBASE_DATABASE_URL;

  if (Firebase.signUp(&firebaseConfig, &auth, "", "")) {
    Serial.println("Firebase: Anonymous sign-up OK");
  } else {
    Serial.printf("Firebase: Sign-up failed: %s\n",
                  firebaseConfig.signer.signupError.message.c_str());
  }

  Firebase.begin(&firebaseConfig, &auth);
  Firebase.setDoubleDigits(5);
  Firebase.reconnectWiFi(true);

  // --- MAX30102 ---
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 NOT FOUND — check wiring!");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A);
  Serial.println("MAX30102 initialised.");

  // --- DS18B20 ---
  tempSensor.begin();
  Serial.println("DS18B20 initialised.");

  Serial.println("====================================================\n");
}

// ================================================================
//  MAIN LOOP
// ================================================================
void loop() {

  Serial.println("[LOOP] tick");

  // ================================================================
  //  PANIC BUTTON — checked via interrupt flag
  //  Fires immediately regardless of sensor state or delays.
  // ================================================================
  if (panicTriggered) {
    panicTriggered = false;   // clear flag immediately
    Serial.println("[PANIC] Button pressed!");
    sendPanicSMS(heartRateValue, spo2Value, lastTemperature);
    Serial.println("[PANIC] SMS dispatched.");
  }

  // ── READ TEMPERATURE ──────────────────────────────────────────
  tempSensor.requestTemperatures();
  lastTemperature = tempSensor.getTempCByIndex(0);

  // ── READ MAX30102 (100 samples) ───────────────────────────────
  delay(2000);
  for (int i = 0; i < 100; i++) {
    while (!particleSensor.available())
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();
  }

  maxim_heart_rate_and_oxygen_saturation(
    irBuffer, 100, redBuffer,
    &spo2Value,     &validSPO2,
    &heartRateValue, &validHeartRate
  );

  Serial.printf("[SENSOR] IR: %d\n", irBuffer[99]);

  // ── FINGER DETECTION ─────────────────────────────────────────
  if (irBuffer[99] < IR_FINGER_MIN) {
    Serial.println("[SENSOR] No finger detected.");

    state   = "NORMAL";
    smsSent = false;

    FirebaseJson json;
    json.set("heart_rate",  0);
    json.set("spo2",        0);
    json.set("temperature", lastTemperature);
    json.set("button",      (int)digitalRead(BUTTON_PIN));
    json.set("state",       "NO_FINGER");
    json.set("timestamp",   (int)millis());
    Firebase.RTDB.setJSON(&fbdo, "/patient1/data", &json);

    delay(5000);
    return;
  }

  // ── VALIDITY CHECK ────────────────────────────────────────────
  bool distress = false;
  if (validHeartRate && validSPO2 &&
      heartRateValue > 30 && heartRateValue < 200) {
    distress = isDistress(heartRateValue, spo2Value, lastTemperature);
  } else {
    Serial.println("[SENSOR] Unstable reading — skipping distress check.");
  }

  // ── DEBUG SERIAL OUTPUT ───────────────────────────────────────
  Serial.println("-------------- VITALS ---------------");
  Serial.printf("  Heart Rate : %d bpm  (valid: %s)\n",
                heartRateValue, validHeartRate ? "YES" : "NO");
  Serial.printf("  SpO2       : %d %%   (valid: %s)\n",
                spo2Value, validSPO2 ? "YES" : "NO");
  Serial.printf("  Temperature: %.2f C\n", lastTemperature);
  Serial.printf("  Button     : %s\n",
                digitalRead(BUTTON_PIN) == 0 ? "PRESSED" : "open");
  Serial.printf("  State      : %s\n", state.c_str());
  Serial.printf("  smsSent    : %s\n", smsSent ? "true" : "false");
  Serial.printf("  Distress   : %s\n", distress ? "YES" : "no");
  Serial.println("-------------------------------------");

  // ================================================================
  //  STATE MACHINE
  // ================================================================

  // NORMAL → PRE_ALERT
  if (state == "NORMAL" && distress) {
    state = "PRE_ALERT";
    distressStartTime = millis();
    Serial.println("[STATE] NORMAL → PRE_ALERT");
  }

  // PRE_ALERT logic
  if (state == "PRE_ALERT") {
    if (millis() - distressStartTime >= PRE_ALERT_MS) {
      state = "ALERT";
      Serial.println("[STATE] PRE_ALERT → ALERT");
    }
    else if (!distress) {
      state   = "NORMAL";
      smsSent = false;
      Serial.println("[STATE] PRE_ALERT → NORMAL (recovered)");
    }
  }

  // ALERT → NORMAL if vitals recover
  if (state == "ALERT" && !distress) {
    state   = "NORMAL";
    smsSent = false;
    Serial.println("[STATE] ALERT → NORMAL (recovered)");
  }

  // ── TRIGGER DISTRESS SMS (once per alert event) ───────────────
  if (state == "ALERT" && !smsSent) {
    Serial.println("[SMS] ALERT active — triggering distress SMS now...");
    sendAlertToAll(heartRateValue, spo2Value, lastTemperature);
  }

  // ── SEND DATA TO FIREBASE ─────────────────────────────────────
  FirebaseJson json;
  json.set("heart_rate",  heartRateValue);
  json.set("spo2",        spo2Value);
  json.set("temperature", lastTemperature);
  json.set("button",      (int)digitalRead(BUTTON_PIN));
  json.set("state",       state);
  json.set("timestamp",   (int)millis());

  if (Firebase.RTDB.setJSON(&fbdo, "/patient1/data", &json)) {
    Serial.println("[FIREBASE] Data sent OK.");
  } else {
    Serial.println("[FIREBASE] Send failed: " + fbdo.errorReason());
  }

  delay(5000);
}


#include "USB.h"
#include "USBMIDI.h"
#include <WiFi.h>
#include <ArduinoOTA.h>

// === Wi-Fi Credentials ===
const char* WIFI_SSID = "FritzseBadmuts";
const char* WIFI_PASS = "There is no sp00n!";

USBMIDI MIDI;  // Native USB MIDI interface

// === Hardware Configuration ===
const int POT_PIN = 4;           // ADC pin (avoid USB pins 19/20)
const int LED_PIN = 2;           // Built-in LED or external indicator

// === MIDI Settings ===
const uint8_t MIDI_CHANNEL   = 11;  // MIDI Channel 11
const uint8_t CONTROLLER_NUM = 4;   // CC#4 (Foot Controller)

// === Variables ===
uint8_t lastValue = 255;  // Last sent MIDI value
float smoothRaw = 0;      // Smoothed analog reading

void setup() {
  // --- System Init ---
  USB.manufacturerName("Egberts");
  USB.productName("ESP32-S3 Foot Controller");
  USB.serialNumber("FC-002");      // change if Windows keeps caching the old name
  USB.begin();
  MIDI.begin();
  Serial.begin(115200);
  delay(200);

  // --- Hardware Init ---
  analogReadResolution(12);  // 0–4095 range
  pinMode(POT_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

 // --- Wi-Fi Connection ---
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.printf("\nConnected! IP address: %s\n", WiFi.localIP().toString().c_str());

  // --- OTA Setup ---
  ArduinoOTA.setHostname("ESP32S3-FootCtrl");
  ArduinoOTA.setPassword("Nexus8");  // Optional password for security

  ArduinoOTA
    .onStart([]() {
      Serial.println("\nOTA update starting...");
    })
    .onEnd([]() {
      Serial.println("\nOTA update finished!");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress * 100) / total);
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]\n", error);
    });

  ArduinoOTA.begin();

  Serial.println("ESP32-S3 USB MIDI Foot Controller ready.");
}

void loop() {

  // Handle OTA updates
  ArduinoOTA.handle();

  // --- Read and Smooth Analog Input ---
  int raw = analogRead(POT_PIN);
  smoothRaw = (smoothRaw * 0.9f) + (raw * 0.1f);  // Simple exponential smoothing

  // --- Scale to MIDI Range ---
  uint8_t midiValue = map((int)smoothRaw, 0, 4095, 0, 127);

  // --- Send MIDI Only if Change is Significant ---
  if (abs(midiValue - lastValue) >= 1) {
    MIDI.controlChange(CONTROLLER_NUM, 127 - midiValue, MIDI_CHANNEL);
    Serial.printf("CC %u -> %u (Ch %u)\r\n", CONTROLLER_NUM, midiValue, MIDI_CHANNEL);
    lastValue = midiValue;
  }

  // --- LED Feedback ---
  // Reflect pedal position with LED brightness
  int ledBrightness = map(midiValue, 0, 127, 0, 255);
  analogWrite(LED_PIN, ledBrightness);  // Works on PWM-capable pins

  delay(10);  // Update ~100×/s
}

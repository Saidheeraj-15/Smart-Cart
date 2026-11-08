// SmartCart ESP32 (ESP32-WROOM-32): UART barcode scanner -> Supabase scans table
// Wiring: scanner TX -> ESP32 GPIO16 (RX2), scanner RX -> GPIO17 (TX2), GND common.
// Power the scanner from the power bank / VIN (5 V). If the scanner's TX is 5 V logic,
// put a voltage divider (e.g. 1k + 2k) on the line going into GPIO16.
// The scanner must be in UART/serial mode and send CR or LF after each barcode.

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* WIFI_SSID    = "YOUR_HOTSPOT_NAME";
const char* WIFI_PASS    = "YOUR_HOTSPOT_PASSWORD";
const char* SUPABASE_URL = "https://YOURPROJECT.supabase.co";
const char* SUPABASE_KEY = "YOUR_ANON_KEY";
const char* CART_ID      = "CART01";   // must match the ?cart= value in the QR code

#define SCANNER_RX 16
#define SCANNER_TX 17
#define SCANNER_BAUD 9600              // change if your scanner uses another baud rate

HardwareSerial scanner(2);
String buf, lastCode;
unsigned long lastTime = 0;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi");
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) { delay(500); Serial.print("."); }
  Serial.println(WiFi.status() == WL_CONNECTED ? " connected" : " FAILED");
}

bool postScan(const String& code) {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure client;
  client.setInsecure();                // prototype only: skips certificate check
  HTTPClient http;
  http.begin(client, String(SUPABASE_URL) + "/rest/v1/scans");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  http.addHeader("Prefer", "return=minimal");
  String body = String("{\"cart_id\":\"") + CART_ID + "\",\"barcode\":\"" + code + "\"}";
  int r = http.POST(body);
  Serial.printf("POST %s -> %d\n", code.c_str(), r);
  http.end();
  return r >= 200 && r < 300;
}

void setup() {
  Serial.begin(115200);
  scanner.begin(SCANNER_BAUD, SERIAL_8N1, SCANNER_RX, SCANNER_TX);
  connectWiFi();
  Serial.println("Ready: scan a barcode");
}

void loop() {
  while (scanner.available()) {
    char c = scanner.read();
    if (c == '\r' || c == '\n') {
      buf.trim();
      if (buf.length() >= 8) {
        Serial.println("Scanned: " + buf);
        // ignore the same code repeated within 1.5 s (scanner double reads)
        if (buf != lastCode || millis() - lastTime > 1500) {
          postScan(buf);
          lastCode = buf;
          lastTime = millis();
        }
      }
      buf = "";
    } else if (c >= 32 && c < 127) {
      buf += c;
      if (buf.length() > 40) buf = "";  // guard against garbage
    }
  }
}

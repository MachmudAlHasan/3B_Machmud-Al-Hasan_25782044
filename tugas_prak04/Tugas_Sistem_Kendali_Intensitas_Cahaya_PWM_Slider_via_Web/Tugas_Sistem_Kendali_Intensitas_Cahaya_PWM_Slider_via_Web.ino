#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <DHT.h>

const char* ssid = "MACHMUDALHASAN";
const char* password = "Dodol Garut";

/* --- KONFIGURASI PIN & SENSOR --- */
#define PIN_LED 5
#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);#include "DHT.h"

// Definisi Pin untuk NodeMCU ESP8266
#define DHTPIN D5         // GPIO 14
#define DHTTYPE DHT22

#define RELAY_PIN D1      // GPIO 5
#define LED_MERAH D2      // GPIO 4
#define LED_HIJAU D6      // GPIO 12
#define LED_BIRU D7       // GPIO 13
#define BUZZER_PIN D0     // GPIO 16

// Inisialisasi Sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Ambang batas suhu sesuai proyek kandang DOC
const float SUHU_MIN = 30.0;
const float SUHU_MAX = 33.0;
const float SUHU_KRITIS_DINGIN = 28.0;

// Pewaktu non-blocking dengan millis()
unsigned long previousMillis = 0;
const long interval = 2000; // Baca sensor tiap 2 detik

unsigned long buzzerMillis = 0;
bool buzzerState = false;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=================================================");
  Serial.println("  SISTEM PENGATUR SUHU KANDANG AYAM (ESP8266)   ");
  Serial.println("=================================================");

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_BIRU, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Default awal: Relai Active LOW (HIGH = Pemanas Mati)
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(LED_BIRU, LOW);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();
}

void loop() {
  unsigned long currentMillis = millis();

  // Pembacaan berkala setiap 2 detik
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println("[ERROR] Sensor DHT22 tidak terbaca! Periksa kabel pin D5.");
      return;
    }

    // Cetak ke Serial Monitor
    Serial.print("Suhu: ");
    Serial.print(t, 1);
    Serial.print(" °C | Kelembapan: ");
    Serial.print(h, 1);
    Serial.print(" % | Status: ");

    // Logika kendali otonom
    if (t < SUHU_MIN) {
      // Kondisi Dingin: Pemanas ON
      digitalWrite(RELAY_PIN, LOW);   // LOW = Relai ON
      digitalWrite(LED_BIRU, HIGH);
      digitalWrite(LED_HIJAU, LOW);
      digitalWrite(LED_MERAH, LOW);
      Serial.print("[DINGIN] -> Pemanas AKTIF");

    } else if (t >= SUHU_MAX) {
      // Kondisi Panas: Pemanas OFF
      digitalWrite(RELAY_PIN, HIGH);  // HIGH = Relai OFF
      digitalWrite(LED_BIRU, LOW);
      digitalWrite(LED_HIJAU, LOW);
      digitalWrite(LED_MERAH, HIGH);
      Serial.print("[PANAS] -> Pemanas MATI");

    } else {
      // Kondisi Optimal: Pemanas OFF
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(LED_BIRU, LOW);
      digitalWrite(LED_HIJAU, HIGH);
      digitalWrite(LED_MERAH, LOW);
      Serial.print("[OPTIMAL] -> Rentang Aman");
    }

    Serial.println();
  }

  // Logika peringatan buzzer (alarm putus-putus tanpa delay)
  float currentTemp = dht.readTemperature();
  if (!isnan(currentTemp) && (currentTemp < SUHU_KRITIS_DINGIN || currentTemp > 35.0)) {
    if (currentMillis - buzzerMillis >= 250) {
      buzzerMillis = currentMillis;
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

unsigned long lastTime = 0;     

/* --- HALAMAN WEB (HTML & JS) --- */
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Lighting PWM</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; background-color: #eef2f3; color: #333; }
        .card { max-width: 400px; margin: 30px auto; padding: 20px; background: white; border-radius: 15px; box-shadow: 0 10px 20px rgba(0,0,0,0.1); }
        h1 { color: #007bff; font-size: 24px; }
        .slider { width: 100%; height: 15px; border-radius: 5px; background: #ddd; outline: none; -webkit-appearance: none; margin: 20px 0; }
        .slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 25px; height: 25px; border-radius: 50%; background: #007bff; cursor: pointer; }
        .value-display { font-size: 20px; font-weight: bold; color: #555; }
        .sensor-box { display: flex; justify-content: space-around; margin-top: 20px; padding: 10px; background: #f8f9fa; border-radius: 10px; }
        .sensor-item { font-size: 14px; }
        .sensor-val { display: block; font-size: 18px; color: #28a745; font-weight: bold; }
    </style>
</head>
<body>
    <div class="card">
        <h1>Smart Lighting</h1>
        <div class="sensor-box">
            <div class="sensor-item">Temp: <span id="temp" class="sensor-val">0</span> °C</div>
            <div class="sensor-item">Hum: <span id="hum" class="sensor-val">0</span> %</div>
        </div>
        <hr>
        <h3>LED Intensity</h3>
        <input type="range" min="0" max="1023" value="0" class="slider" id="pwmSlider" oninput="sendPWM(this.value)">
        <div class="value-display">Brightness: <span id="pwmVal">0</span></div>
    </div>

    <script>
        var gateway = `ws://${window.location.hostname}:81/`;
        var websocket;

        window.onload = function() {
            websocket = new WebSocket(gateway);
            websocket.onmessage = function(event) {
                var data = event.data.split(',');
                if(data[0] === "dht") {
                    document.getElementById('temp').innerHTML = data[1];
                    document.getElementById('hum').innerHTML = data[2];
                }
            };
        };

        function sendPWM(value) {
            document.getElementById('pwmVal').innerHTML = value;
            // Kirim format data: pwm,nilai (Contoh: pwm,512)
            websocket.send("pwm," + value);
        }
    </script>
</body>
</html>
)rawliteral";

/* --- FUNGSI WEBSOCKET HANDLER --- */
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = String((char*)(payload));
    
    // Parsing pesan: "pwm,512"
    if (message.startsWith("pwm,")) {
      String valStr = message.substring(4); // Ambil karakter setelah "pwm,"
      int pwmValue = valStr.toInt();        // Ubah string ke angka
      
      analogWrite(PIN_LED, pwmValue);       // Atur intensitas LED
      Serial.printf("Slider: %d\n", pwmValue);
    }
  }
}

/* --- SETUP --- */
void setup() {
  Serial.begin(115200);
  
  // Konfigurasi Pin
  pinMode(PIN_LED, OUTPUT);
  analogWriteRange(1023); // Pastikan range PWM NodeMCU 0-1023
  
  // Koneksi WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP Address: " + WiFi.localIP().toString());

  // Inisialisasi DHT
  dht.begin();

  // Server Routing
  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });

  // Start Server & WebSocket
  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
}

/* --- LOOP --- */
void loop() {
  server.handleClient();
  webSocket.loop();

  // Kirim data Sensor setiap 2 detik (Non-Blocking)
  if (millis() - lastTime > 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      // Format kirim: dht,suhu,lembab
      String payload = "dht," + String(t, 1) + "," + String(h, 0);
      webSocket.broadcastTXT(payload);
    }
    lastTime = millis();
  }
}
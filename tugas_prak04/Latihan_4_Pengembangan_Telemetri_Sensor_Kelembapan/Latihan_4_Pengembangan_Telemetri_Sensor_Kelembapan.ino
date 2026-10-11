#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

const char* ssid = "MACHMUDALHASAN";
const char* password = "Dodol Garut";

// Konfigurasi Pin
const byte dhtPin = 2;        // D4 / GPIO 2
const byte buttonPin = 4;    // D2 / GPIO 4
const byte ledPin = 12;      // D6 / GPIO 12

// DHT22
#define DHTTYPE DHT22
DHT dht(dhtPin, DHTTYPE);

// Variabel Status
bool ledState = false;

String currentTemp = "--";
String currentHum = "--";

int buttonState;
int lastButtonState = LOW;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

unsigned long lastTime = 0;

// Web Server dan WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------------- HTML & JAVASCRIPT ----------------

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <title>Real-Time IoT Web</title>

  <style>
    body {
      font-family: Arial;
      text-align: center;
    }

    .card {
      background: #f0f0f0;
      margin: 20px auto;
      padding: 20px;
      max-width: 300px;
      border-radius: 10px;
    }

    button {
      padding: 15px 30px;
      font-size: 20px;
      border-radius: 5px;
      cursor: pointer;
      color: white;
    }

    .btn-on {
      background-color: #4CAF50;
    }

    .btn-off {
      background-color: #f44336;
    }
  </style>
</head>

<body>

  <h1>Smart Room</h1>

  <!-- Suhu -->
  <div class="card">
    <h2>
      Suhu:
      <span id="tempValue">--</span>
      Celcius
    </h2>
  </div>

  <!-- Kelembapan -->
  <div class="card">
    <h2>
      Kelembapan:
      <span id="humValue">--</span>
      %
    </h2>
  </div>

  <!-- LED -->
  <div class="card">
    <h2>
      LED:
      <span id="ledStatus">OFF</span>
    </h2>

    <button
      id="toggleBtn"
      class="btn-off"
      onclick="toggleLed()">
      Turn ON
    </button>
  </div>

  <script>

    // Koneksi WebSocket
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;

    // Saat halaman dibuka
    window.addEventListener('load', onLoad);

    function onLoad(event) {
      initWebSocket();
    }

    // Inisialisasi WebSocket
    function initWebSocket() {

      websocket = new WebSocket(gateway);

      websocket.onopen = onOpen;
      websocket.onclose = onClose;
      websocket.onmessage = onMessage;
    }

    // WebSocket terhubung
    function onOpen(event) {
      console.log("WebSocket Terkoneksi");
    }

    // Jika koneksi terputus
    function onClose(event) {

      console.log("WebSocket Terputus");

      setTimeout(initWebSocket, 2000);
    }

    // Tombol LED ditekan
    function toggleLed() {

      websocket.send("toggle");
    }

    // Menerima data dari ESP8266
    function onMessage(event) {

      var dataObj = JSON.parse(event.data);

      // Menampilkan suhu
      if (dataObj.suhu !== undefined) {

        document.getElementById("tempValue").innerHTML =
          dataObj.suhu;
      }

      // Menampilkan kelembapan
      if (dataObj.hum !== undefined) {

        document.getElementById("humValue").innerHTML =
          dataObj.hum;
      }

      // Mengubah tampilan LED
      if (dataObj.led !== undefined) {

        var btn =
          document.getElementById("toggleBtn");

        var status =
          document.getElementById("ledStatus");

        if (dataObj.led == "1") {

          status.innerHTML = "ON";

          btn.innerHTML = "Turn OFF";

          btn.className = "btn-on";

        } else {

          status.innerHTML = "OFF";

          btn.innerHTML = "Turn ON";

          btn.className = "btn-off";
        }
      }
    }

  </script>

</body>
</html>
)rawliteral";

// ---------------- BACK-END & WEBSOCKET ----------------

// Mengirim data ke semua browser
void notifyClients() {

  String jsonString =
    "{\"led\":\"" +
    String(ledState ? 1 : 0) +
    "\", ";

  jsonString +=
    "\"suhu\":\"" +
    currentTemp +
    "\", ";

  jsonString +=
    "\"hum\":\"" +
    currentHum +
    "\"}";

  ws.textAll(jsonString);
}

// Memproses pesan dari browser
void handleWebSocketMessage(
  void *arg,
  uint8_t *data,
  size_t len
) {

  AwsFrameInfo *info =
    (AwsFrameInfo*)arg;

  if (
    info->final &&
    info->index == 0 &&
    info->len == len &&
    info->opcode == WS_TEXT
  ) {

    data[len] = 0;

    if (
      strcmp((char*)data, "toggle") == 0
    ) {

      ledState = !ledState;

      notifyClients();
    }
  }
}

// Event WebSocket
void onEvent(
  AsyncWebSocket *server,
  AsyncWebSocketClient *client,
  AwsEventType type,
  void *arg,
  uint8_t *data,
  size_t len
) {

  switch (type) {

    case WS_EVT_CONNECT:

      Serial.printf(
        "Client WebSocket #%u terhubung\n",
        client->id()
      );

      // Kirim status awal
      notifyClients();

      break;

    case WS_EVT_DISCONNECT:

      Serial.printf(
        "Client WebSocket #%u terputus\n",
        client->id()
      );

      break;

    case WS_EVT_DATA:

      handleWebSocketMessage(
        arg,
        data,
        len
      );

      break;
  }
}

// ---------------- SETUP ----------------

void setup() {

  Serial.begin(115200);

  // Tombol
  pinMode(buttonPin, INPUT);

  // LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // DHT22
  dht.begin();

  // WiFi
  WiFi.mode(WIFI_STA);

  WiFi.begin(
    ssid,
    password
  );

  Serial.print("Menghubungkan WiFi");

  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.print("IP Address: ");

  Serial.println(
    WiFi.localIP()
  );

  // Halaman web
  server.on(
    "/",
    HTTP_GET,
    [](AsyncWebServerRequest *request) {

      request->send_P(
        200,
        "text/html",
        index_html
      );
    }
  );

  // WebSocket
  ws.onEvent(onEvent);

  server.addHandler(&ws);

  server.begin();

  Serial.println(
    "Web Server dan WebSocket aktif"
  );
}

// ---------------- LOOP ----------------

void loop() {

  // Membersihkan client WebSocket
  ws.cleanupClients();

  // Mengatur LED
  digitalWrite(
    ledPin,
    ledState ? HIGH : LOW
  );

  // Membaca tombol
  int reading =
    digitalRead(buttonPin);

  // Debounce
  if (
    reading != lastButtonState
  ) {

    lastDebounceTime =
      millis();
  }

  if (
    (millis() - lastDebounceTime)
    > debounceDelay
  ) {

    if (
      reading != buttonState
    ) {

      buttonState = reading;

      // Tombol ditekan
      if (
        buttonState == HIGH
      ) {

        ledState = !ledState;

        // Kirim status ke browser
        notifyClients();
      }
    }
  }

  lastButtonState = reading;

  // Baca DHT22 setiap 3 detik
  // menggunakan millis() tanpa delay
  if (
    (millis() - lastTime) > 3000
  ) {

    float t =
      dht.readTemperature();

    float h =
      dht.readHumidity();

    // Pastikan data valid
    if (
      !isnan(t) &&
      !isnan(h)
    ) {

      currentTemp =
        String(t);

      currentHum =
        String(h);

      // Kirim suhu dan kelembapan
      // ke browser
      notifyClients();
    }

    lastTime =
      millis();
  }
}
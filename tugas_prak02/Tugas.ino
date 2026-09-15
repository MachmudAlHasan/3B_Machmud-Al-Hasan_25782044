#include "DHT.h"

#define DHTPIN D7           // GPIO13 - Data DHT22
#define DHTTYPE DHT22       // DHT 22
#define LDR_PIN A0          // A0 - Analog LDR
#define RELAY_PIN D6        // GPIO12 - Relay Signal
#define LED_PIN D1          // GPIO5 - LED Indikator

DHT dht(DHTPIN, DHTTYPE);
float temperature = 0;
float humidity = 0;
int ldrValue = 0;

const int TEMP_THRESHOLD = 34;    // Batas suhu (°C)
const int LDR_THRESHOLD = 300;    // Batas cahaya (ADC value)

int relayState = HIGH;            // HIGH = OFF (Active-Low)
int ledState = LOW;               // LOW = OFF

void setup() {
  delay(2000);
  
  Serial.begin(115200);
  delay(100);
  
  // Tampilkan header
  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════════╗");
  Serial.println("║   SMART WAREHOUSE - SISTEM OTOMASI   ║");
  Serial.println("║     Kontrol Lokal Terpadu (2.7)      ║");
  Serial.println("╚═══════════════════════════════════════╝\n");
  
  // Inisialisasi pin
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Set awal OFF
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF (Active-Low)
  digitalWrite(LED_PIN, LOW);     // LED OFF
  
  // Inisialisasi DHT
  dht.begin();
  
  Serial.println("[INIT] Relay OFF - LED OFF");
  Serial.println("[INIT] Sensor DHT22 & LDR Ready\n");
  
  Serial.println("THRESHOLD SETTINGS:");
  Serial.print("  Temp Threshold: ");
  Serial.print(TEMP_THRESHOLD);
  Serial.println("°C");
  Serial.print("  LDR Threshold: ");
  Serial.println(LDR_THRESHOLD);
  Serial.println();
}

// LOOP UTAMA
void loop() {
  // 1. BACA SENSOR DHT22
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // 2. BACA SENSOR LDR
  ldrValue = analogRead(LDR_PIN);
  
  // 3. TAMPILKAN DATA SENSOR
  displaySensorData();
  
  // 4. LOGIC ENGINE - Operator OR (||)
  // Relay ON jika: Suhu > 34°C ATAU LDR < 300
  if (temperature > TEMP_THRESHOLD || ldrValue < LDR_THRESHOLD) {
    relayState = LOW;   // Relay ON (Active-Low)
    ledState = HIGH;    // LED ON
  } else {
    relayState = HIGH;  // Relay OFF
    ledState = LOW;     // LED OFF
  }
  
  // 5. KONTROL ACTUATOR
  digitalWrite(RELAY_PIN, relayState);
  digitalWrite(LED_PIN, ledState);
  
  // 6. TAMPILKAN STATUS SISTEM
  displaySystemStatus();
  
  delay(2000);  // Interval 2 detik
}

// TAMPILKAN DATA SENSOR
void displaySensorData() {
  Serial.println("┌─────────────────────────────────┐");
  Serial.println("│     SENSOR DATA READING         │");
  Serial.println("├─────────────────────────────────┤");
  
  // DHT22
  Serial.print("│ Suhu (DHT22):     ");
  Serial.print(temperature);
  Serial.println(" °C");
  
  Serial.print("│ Kelembaban:       ");
  Serial.print(humidity);
  Serial.println(" %");
  
  // LDR
  Serial.print("│ Cahaya (LDR):     ");
  Serial.print(ldrValue);
  Serial.println(" (ADC)");
  
  Serial.println("└─────────────────────────────────┘");
}

// STATUS SISTEM
void displaySystemStatus() {
  Serial.println("\n┌─────────────────────────────────┐");
  Serial.println("│     KONDISI SISTEM OTOMASI      │");
  Serial.println("├─────────────────────────────────┤");
  
  // Cek kondisi 1: Suhu tinggi
  Serial.print("│ Suhu Tinggi (>34°C)? ");
  if (temperature > TEMP_THRESHOLD) {
    Serial.println(" ✓ YA");
  } else {
    Serial.println(" ✗ TIDAK");
  }
  
  // Cek kondisi 2: Cahaya rendah
  Serial.print("│ Cahaya Rendah (<300)? ");
  if (ldrValue < LDR_THRESHOLD) {
    Serial.println(" ✓ YA");
  } else {
    Serial.println(" ✗ TIDAK");
  }
  
  // Status Relay & LED
  Serial.print("│ Status Relay:     ");
  if (relayState == LOW) {
    Serial.println("ON (AKTIF)");
  } else {
    Serial.println("OFF (MATI)");
  }
  
  Serial.print("│ Status LED:       ");
  if (ledState == HIGH) {
    Serial.println("ON (MENYALA)");
  } else {
    Serial.println("OFF (MATI)");
  }
  
  // Status Keseluruhan
  Serial.print("│ KONDISI SISTEM:   ");
  if (relayState == LOW) {
    Serial.println("⚠️  PERINGATAN AKTIF!");
  } else {
    Serial.println("✓ AMAN");
  }
  
  Serial.println("└─────────────────────────────────┘\n");
}
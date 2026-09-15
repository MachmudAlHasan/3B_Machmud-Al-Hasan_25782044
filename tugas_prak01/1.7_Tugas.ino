const int buttonPin = 4;
const int ledPin = 5;

int ledState = LOW;
int lastButtonState = HIGH;
int currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 200;

void setup() {
  delay(2000);
  
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n");
  Serial.println("=====================================");
  Serial.println("  ESP8266 Toggle Button System");
  Serial.println("  (Sistem Sakelar Lampu Ruangan)");
  Serial.println("=====================================\n");
  
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  
  digitalWrite(ledPin, ledState);
  Serial.println("[INIT] LED OFF pada awal startup\n");
}

void loop() {
  currentButtonState = digitalRead(buttonPin);
  
  if (currentButtonState != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    
    if (currentButtonState == LOW && lastButtonState == HIGH) {
      
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      
      Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━");
      Serial.print("[TOMBOL DITEKAN] ");
      if (ledState == HIGH) {
        Serial.println("→ LED MENYALA!");
      } else {
        Serial.println("→ LED MATI!");
      }
      Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    }
  }
  
  lastButtonState = currentButtonState;
  
  delay(50);
}
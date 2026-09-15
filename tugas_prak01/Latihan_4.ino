const int buttonPin = 4;
const int ledPin = 5;

int buttonState = 0;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  buttonState = digitalRead(buttonPin);
  
  if (buttonState == LOW) {  // ← Ubah dari HIGH ke LOW
    digitalWrite(ledPin, HIGH);
    Serial.println("Tombol TIDAK ditekan! -> LED ON");
  } else {
    digitalWrite(ledPin, LOW);
    Serial.println("Tombol ditekan! -> LED OFF");  // ← Tambah feedback
  }
}
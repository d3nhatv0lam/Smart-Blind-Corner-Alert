
const int PIRLeft = 18;
const int LedRight = 17;

const int PIRRight = 16;
const int LedLeft = 19;

void setup() {
  Serial.begin(115200);

  pinMode(PIRLeft, INPUT);
  pinMode(PIRRight, INPUT);
  pinMode(LedLeft, OUTPUT);
  pinMode(LedRight, OUTPUT);
}

void loop() {
  bool left = digitalRead(PIRLeft);
  bool right = digitalRead(PIRRight);

  if (left) {
    digitalWrite(LedRight, HIGH);
  } else {
    digitalWrite(LedRight, LOW);
  }

   if (right) {
    digitalWrite(LedLeft, HIGH);
  } else {
    digitalWrite(LedLeft, LOW);
  }

  delay(100);
}